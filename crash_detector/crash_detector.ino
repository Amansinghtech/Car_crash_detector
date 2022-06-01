#include<Wire.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
SoftwareSerial mySerial(9, 10);

const int MPU_addr=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
 
int minVal=265;
int maxVal=402;
 
double x;
double y;
double z;
bool reset = false;
int resetCounter = 10;

int Sensor = 12;
int Abort = 8;

//GPS code
// Choose two Arduino pins to use for software serial
int RXPin = 4;
int TXPin = 3;
int LED_loc = 2; 
int GPSBaud = 9600;
int GPSERR = 5;
double lastLocation[3];

// Create a TinyGPS++ object
TinyGPSPlus gps;

// Create a software serial port called "gpsSerial"
SoftwareSerial gpsSerial(RXPin, TXPin);

void setup(){

  pinMode(13, OUTPUT); //indication LED
  pinMode (Sensor, INPUT_PULLUP); //Crash Sensor
  pinMode(Abort, INPUT_PULLUP); // Abort Button
  mySerial.begin(9600);   // Setting the baud rate of GSM Module  
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(9600);
  delay(100);
  
  // Start the software serial port at the GPS's default baud
  pinMode(LED_loc, OUTPUT);
  pinMode(GPSERR, OUTPUT);
  gpsSerial.begin(GPSBaud);
  digitalWrite(GPSERR, LOW);
}

void BlinkLED() {
    digitalWrite(LED_loc, HIGH);
    delay(100);
    digitalWrite(LED_loc, LOW);
    delay(100);
    Serial.println("BLED");
  }

void crashDetected() {
  
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
    delay(500);
  }

void sendMessage() {
        if (!reset) {
           mySerial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
           delay(1000);  // Delay of 1 second
           mySerial.println("AT+CMGS=\"+918770262013\"\r"); // Replace x with mobile number
           delay(1000);
           mySerial.println("SOS: Car got crashed!");// The SMS text you want to send
           mySerial.print("loc: ");
           mySerial.print(lastLocation[0], 6);
           mySerial.print(" ");
           mySerial.print(lastLocation[1], 6);
           delay(100);
           mySerial.println((char)26);// ASCII code of CTRL+Z for saying the end of sms to  the module 
            delay(1000);
          Serial.println("message Sent");
          reset = true;
          if (mySerial.available()>0)
             Serial.write(mySerial.read());
        }else {
          Serial.println("message already Sent, Please reset the device");
        }
  }

bool checkOrientation(){

  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);
  AcX=Wire.read()<<8|Wire.read();
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
  int xAng = map(AcX,minVal,maxVal,-90,90);
  int yAng = map(AcY,minVal,maxVal,-90,90);
  int zAng = map(AcZ,minVal,maxVal,-90,90);
   
  x= RAD_TO_DEG * (atan2(-yAng, -zAng)+PI);
  y= RAD_TO_DEG * (atan2(-xAng, -zAng)+PI);
  z= RAD_TO_DEG * (atan2(-yAng, -xAng)+PI);
  // 
  //Serial.print("AngleX= ");
  //Serial.println(x);
  // 
  //Serial.print("AngleY= ");
  //Serial.println(y);
  // 
  //Serial.print("AngleZ= ");
  //Serial.println(z);
  Serial.print("X: ");
  Serial.print(x);
  Serial.print(", Y: ");
  Serial.print(y);
  Serial.print(", Z: ");
  Serial.print(z);
  Serial.println("");
   
  if (x>310 || x < 45) return false;
  return true;
  }


void mainCrashDetector() {
 if (checkOrientation()) {
  Serial.println("Car is upside down");
  int i = 0;
  for (i = 0; i <= resetCounter ; i++) {
      if (!checkOrientation()) {
          reset = false;
          break;
        };
       Serial.println(i);
       crashDetected();
    }
    if (i >9){
       Serial.println("Sending SOS message...");
       sendMessage();
       reset = true;
      }
  };

  if (!digitalRead(Sensor)) {
      int i = 0;
      Serial.println("Car is hit by something");
      for (i=0; i<=resetCounter; i++) {
//          Serial.println(digitalRead(Abort));
          if (!digitalRead(Abort)) {
//              reset = false;
              break;
            }
          Serial.println(i);
          crashDetected();
        }
        if (i>9) {
            Serial.println("Sending an SOS message");
            sendMessage();
            reset = true;
          }
    }
  }

void loop(){
  // This sketch displays information every time a new sentence is correctly encoded.
  while (gpsSerial.available() > 0)
    if (gps.encode(gpsSerial.read()))
      displayInfo();

  // If 5000 milliseconds pass and there are no characters coming in
  // over the software serial port, show a "No GPS detected" error
  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println("No GPS detected");
    digitalWrite(GPSERR, HIGH);
    while(true);
  }
  
  mainCrashDetector();
}


void displayInfo()
{
  if (gps.location.isValid())
  {
    lastLocation[0] = gps.location.lat();
    lastLocation[1] = gps.location.lng();
    lastLocation[2] = gps.altitude.meters();
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6);
    Serial.print("Altitude: ");
    Serial.println(gps.altitude.meters());
  }
  else
  {
    BlinkLED();
    Serial.println("Location: Not Available");
  }
  
  Serial.print("Date: ");
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.day());
    Serial.print("/");
    Serial.println(gps.date.year());
  }
  else
  {
    Serial.println("Not Available");
  }

  Serial.print("Time: ");
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(":");
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(":");
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(".");
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.println(gps.time.centisecond());
  }
  else {
    Serial.println("Not Available");
  }

  Serial.println();
  Serial.println();
//  delay(1000);
}
