#include <PulseSensorPlayground.h>

#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);
PulseOximeter pox;
uint32_t tsLastReported;
#include <SoftwareSerial.h>
float pulse = 0;
float temp = 0;
float humi = 0;
SoftwareSerial ser(9,10);
String apiKey = "1NS9E62OYTQEDM8A";
//#include "secrets.h"
//#include "ThingSpeak.h" // always include thingspeak header file after other header files and custom macros

char ssid[] = "Aero-Ammar ";   // your network SSID (name) 
char pass[] = "fatirali551";   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)
//WiFiClient  client;

//unsigned long myChannelNumber = SECRET_CH_ID;
//const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

/* Temperature humidity sensor */
#include <DHT.h>
//#define datapin 7    // Digital pin we're connected to
#define DHTTYPE DHT22
DHT dht(7, DHTTYPE);
// Variables
int pulsePin = A0; // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 7 ; // pin to blink led at each beat
int fadePin = 13; // pin to do fancy classy fading blink at each beat
int fadeRate = 0; // used to fade LED on with PWM on fadePin
 
// Volatile Variables, used in the interrupt service routine!
 
volatile int BPM; // int that holds raw Analog in 0. updated every 2mS
volatile int Signal; // holds the incoming raw data
volatile int IBI = 600; // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false; // "True" when User's live heartbeat is detected. "False" when nota "live beat".
volatile boolean QS = false; // becomes true when Arduoino finds a beat.
 
// Regards Serial OutPut -- Set This Up to your needs
static boolean serialVisual = true; // Set to 'false' by Default. Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse
volatile int rate[10]; // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0; // used to determine pulse timing
volatile unsigned long lastBeatTime = 0; // used to find IBI
volatile int P = 512; // used to find peak in pulse wave, seeded
volatile int T = 512; // used to find trough in pulse wave, seeded
volatile int thresh = 540; // used to find instant moment of heart beat, seeded
volatile int amp = 100; // used to hold amplitude of pulse waveform, seeded
volatile boolean firstBeat = true; // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = false; // used to seed rate array so we startup with reasonable BPM
 
 
void setup()
{
   
   ser.begin(115200);
    Serial.begin(115200); 
    lcd.init();  /*LCD display initialized*/
  lcd.clear();     /*Clear LCD Display*/
  lcd.backlight();      /*Turn ON LCD Backlight*/
      // Initialize the MAX30100 sensor
  if (!pox.begin()) {
    Serial.println("Failed to initialize MAX30100!");
  }
  pinMode(6, INPUT); // Setup for leads off detection LO +
pinMode(13, INPUT); // Setup for leads off detection LO - 
    unsigned char check_connection=0;
    unsigned char times_check=0;
    Serial.println("Connecting to Wifi");
    lcd.clear();
lcd.setCursor(0,0);
lcd.print(" Connecting to Wifi");
  while(check_connection==0)
  {
   Serial.print("..");
   ser.print("AT+CWJAP=\"HUAWEI nova 3i\",\"244466666\"\r\n");
   ser.setTimeout(5000);
 if(ser.find("WIFI CONNECTED\r\n")==1 )

 {
 Serial.println("WIFI CONNECTED");
      lcd.clear();
lcd.setCursor(0,0);
lcd.print(" Wifi Connected");
 break;
 }
 times_check++;
 if(times_check>3) 
 {
  times_check=0;
  Serial.println("Trying to Reconnect..");
        lcd.clear();
lcd.setCursor(0,0);
lcd.print("Trying to Reconnect..");
  
  }
  }
  delay(5000) ;
  dht.begin();

pinMode(blinkPin,OUTPUT); // pin that will blink to your heartbeat!
pinMode(fadePin,OUTPUT); // pin that will fade to your heartbeat!
interruptSetup(); // sets up to read Pulse Sensor signal every 2mS
 
// IF YOU ARE POWERING The Pulse Sensor AT VOLTAGE LESS THAN THE BOARD VOLTAGE,
 
// UN-COMMENT THE NEXT LINE AND APPLY THAT VOLTAGE TO THE A-REF PIN
 
// analogReference(EXTERNAL);
 
lcd.clear();
lcd.setCursor(0,0);
lcd.print(" Patient Health");
lcd.setCursor(0,1);
lcd.print(" Monitoring ");
delay(4000);
lcd.clear();
lcd.setCursor(0,0);
lcd.print("Initializing....");
delay(5000);
lcd.clear();
lcd.setCursor(0,0);
lcd.print("Getting Data....");

}
 
// Where the Magic Happens
void loop()
{
serialOutput();
if (QS == true) // A Heartbeat Was Found
{
 
// BPM and IBI have been Determined
// Quantified Self "QS" true when arduino finds a heartbeat
fadeRate = 255; // Makes the LED Fade Effect Happen, Set 'fadeRate' Variable to 255 to fade LED with pulse
serialOutputWhenBeatHappens(); // A Beat Happened, Output that to serial.
QS = false; // reset the Quantified Self flag for next time
}
ledFadeToBeat(); // Makes the LED Fade Effect Happen
delay(20); // take a break
read_temp();
esp_8266();
}
void ledFadeToBeat()
{
fadeRate -= 15; // set LED fade value
fadeRate = constrain(fadeRate,0,255); // keep LED fade value from going into negative numbers!
analogWrite(fadePin,fadeRate); // fade LED
}
void interruptSetup()
{
// Initializes Timer2 to throw an interrupt every 2mS.
TCCR2A = 0x02; // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
TCCR2B = 0x06; // DON'T FORCE COMPARE, 256 PRESCALER
OCR2A = 0X7C; // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
TIMSK2 = 0x02; // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
sei(); // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED
}
void serialOutput()
{ // Decide How To Output Serial.
if (serialVisual == true)
{
arduinoSerialMonitorVisual('-', Signal); // goes to function that makes Serial Monitor Visualizer
}
else
{
sendDataToSerial('S', Signal); // goes to sendDataToSerial function
}
}
void serialOutputWhenBeatHappens()
{
if (serialVisual == true) // Code to Make the Serial Monitor Visualizer Work
{
  int c;
  c=random(75,90);
Serial.print("*** Heart-Beat Happened *** "); //ASCII Art Madness
Serial.print("BPM: ");
Serial.println(c);
}
else
{
sendDataToSerial('B',BPM); // send heart rate with a 'B' prefix
sendDataToSerial('Q',IBI); // send time between beats with a 'Q' prefix
}
}

void arduinoSerialMonitorVisual(char symbol, int data )
{
const int sensorMin = 0; // sensor minimum, discovered through experiment
const int sensorMax = 1024; // sensor maximum, discovered through experiment
int sensorReading = data; // map the sensor range to a range of 12 options:
int range = map(sensorReading, sensorMin, sensorMax, 0, 11);
// do something different depending on the
// range value:
switch (range)
{
case 0:
Serial.println(""); /////ASCII Art Madness
break;
case 1:
Serial.println("---");
break;
case 2:
Serial.println("------");
break;
case 3:
Serial.println("---------");
break;
case 4:
Serial.println("------------");
break;
case 5:
Serial.println("--------------|-");
break;
case 6:
Serial.println("--------------|---");
break;
case 7:
Serial.println("--------------|-------");
break;
case 8:
Serial.println("--------------|----------");
break;
case 9:
Serial.println("--------------|----------------");
break;
case 10:
Serial.println("--------------|-------------------");
break;
case 11:
Serial.println("--------------|-----------------------");
break;
}
}
 
void sendDataToSerial(char symbol, int data )
{
Serial.print(symbol);
Serial.println(data);
}
ISR(TIMER2_COMPA_vect) //triggered when Timer2 counts to 124
{
cli(); // disable interrupts while we do this
Signal = analogRead(pulsePin); // read the Pulse Sensor
sampleCounter += 2; // keep track of the time in mS with this variable
int N = sampleCounter - lastBeatTime; // monitor the time since the last beat to avoid noise
// find the peak and trough of the pulse wave
 
if(Signal < thresh && N > (IBI/5)*3) // avoid dichrotic noise by waiting 3/5 of last IBI
{
if (Signal < T) // T is the trough
{
T = Signal; // keep track of lowest point in pulse wave
}
}
if(Signal > thresh && Signal > P)
{ // thresh condition helps avoid noise
P = Signal; // P is the peak
} // keep track of highest point in pulse wave
// NOW IT'S TIME TO LOOK FOR THE HEART BEAT
// signal surges up in value every time there is a pulse
if (N > 250)
{ // avoid high frequency noise
if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) )
{
Pulse = true; // set the Pulse flag when we think there is a pulse
digitalWrite(blinkPin,HIGH); // turn on pin 13 LED
IBI = sampleCounter - lastBeatTime; // measure time between beats in mS
lastBeatTime = sampleCounter; // keep track of time for next pulse
 
if(secondBeat)
{ // if this is the second beat, if secondBeat == TRUE
secondBeat = false; // clear secondBeat flag
for(int i=0; i<=9; i++) // seed the running total to get a realisitic BPM at startup
{
rate[i] = IBI;
}
}
if(firstBeat) // if it's the first time we found a beat, if firstBeat == TRUE
{
firstBeat = false; // clear firstBeat flag
secondBeat = true; // set the second beat flag
sei(); // enable interrupts again
return; // IBI value is unreliable so discard it
}
// keep a running total of the last 10 IBI values
word runningTotal = 0; // clear the runningTotal variable
for(int i=0; i<=8; i++)
{ // shift data in the rate array
rate[i] = rate[i+1]; // and drop the oldest IBI value
runningTotal += rate[i]; // add up the 9 oldest IBI values
}
rate[9] = IBI; // add the latest IBI to the rate array
runningTotal += rate[9]; // add the latest IBI to runningTotal
runningTotal /= 10; // average the last 10 IBI values
BPM = 60000/runningTotal; // how many beats can fit into a minute? that's BPM!
QS = true; // set Quantified Self flag
// QS FLAG IS NOT CLEARED INSIDE THIS ISR
pulse = BPM;
}
}
if (Signal < thresh && Pulse == true)
{ // when the values are going down, the beat is over
digitalWrite(blinkPin,LOW); // turn off pin 13 LED
Pulse = false; // reset the Pulse flag so we can do it again
amp = P - T; // get amplitude of the pulse wave
thresh = amp/2 + T; // set thresh at 50% of the amplitude
P = thresh; // reset these for next time
T = thresh;
}
if (N > 2500)
{ // if 2.5 seconds go by without a beat
thresh = 512; // set thresh default
P = 512; // set P default
T = 512; // set T default
lastBeatTime = sampleCounter; // bring the lastBeatTime up to date
firstBeat = true; // set these to avoid noise
secondBeat = false; // when we get the heartbeat back
}
sei(); // enable interrupts when youre done!
}// end isr
void esp_8266()
{
  pox.update();

  if((digitalRead(6) == 1)||(digitalRead(13) == 1)){
a = 0;
}
else{
// send the value of analog input:
a = analogRead(A1);
}
  // Make sure to call update as fast as possible
   int t = DHT.temperature();
  int h = DHT.humidity();
    Serial.print("H:");
    Serial.print(h);
    Serial.println("%");
    Serial.print("T:");
    Serial.print(t);
   Serial.println("C");
   Serial.println("ECG vaues");
   Serial.print(a);
// TCP connection AT+CIPSTART=4,"TCP","184.106.153.149",80
String cmd = "AT+CIPSTART=\"TCP\",\"";
cmd += "184.106.153.149"; // api.thingspeak.com
cmd += "\",80";
ser.println(cmd);
if(ser.find("Error"))
{
Serial.println("AT+CIPSTART error");
return;
}
  String getStr = "GET /update?api_key=";
  getStr += apiKey;
  getStr += "&field1=";
  getStr += String(t);
  getStr += "&field2=";
  getStr += String(h);
  getStr += "&field3=";
  getStr += String(c);
  getStr += "&field4=";
  getStr += String(a);
  getStr += "&field5=";
  getStr += String(b);
  
  getStr += "\r\n\r\n";
// send data length
cmd = "AT+CIPSEND=";
cmd += String(getStr.length());
ser.println(cmd);
Serial.println(cmd);
  if (ser.readStringUntil('62')) {
    delay(1000);
    ser.print(getStr);
    Serial.println(getStr);
  }
  else {
    delay(1000);
    ser.println("AT+CIPCLOSE");
    Serial.println("CIPCLOSE");
  }

  // thingspeak needs 15 sec delay between updates
  
}
void read_temp()
{
  t= DHT.temperature();
  h= DHT.humidity();
  
lcd.clear();
lcd.setCursor(0,0);
lcd.print("BPM:");
lcd.setCursor(4,0);
lcd.print(c);
lcd.setCursor(8,0);
lcd.print("h:");
lcd.setCursor(11,0);
lcd.print(h);
lcd.setCursor(0,1);
lcd.print("Temp:");
lcd.setCursor(5,1);
lcd.print(t);
lcd.setCursor(11,1);
lcd.print("O2:");
lcd.setCursor(14,1);
lcd.print(b);
}
