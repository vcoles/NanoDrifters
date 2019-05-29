// Sample RFM69 receiver/gateway sketch, with ACK and optional encryption, and Automatic Transmission Control
// Passes through any wireless received messages to the serial port & responds to ACKs
// It also looks for an onboard FLASH chip, if present
// RFM69 library and sample code by Felix Rusu - http://LowPowerLab.com/contact
// Copyright Felix Rusu (2015)

#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>//get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>      //comes with Arduino IDE (www.arduino.cc)
#include <SPIFlash.h> //get it here: https://www.github.com/lowpowerlab/spiflash
#include <SD.h>
#include <Adafruit_GPS.h>

#if !defined(ARDUINO_ARCH_SAM) && !defined(ARDUINO_ARCH_SAMD) && !defined(ESP8266) && !defined(ARDUINO_ARCH_STM32F2)
#include <SoftwareSerial.h>
#endif

Adafruit_GPS GPS(&Serial1); //if this stops working, check whAT BOARD arduino thinks you are using

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NODEID        101    //unique for each node on same network
#define NETWORKID     100  //the same on all nodes that talk to each other
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HWC    //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
//*********************************************************************************************
boolean GPSECHO = false;
String dataString = "";
//int TRANSMITPERIOD = 1000; //transmit a packet to gateway so often (in ms)
char buff[20];
#define SERIAL_BAUD   115200
#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif
#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif
SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)
bool promiscuousMode = true; //set to 'true' to sniff all packets on the same network

const int CS_SD = 13;
const int CS_radio = 4; // Set to "HIGH" to turn off any of these CS
const int CS_flash = 23; 

File dataFile;  // declare file for SD writing

void setup() {      //BEGIN SETUP
//  Blink(100);
  pinMode(CS_radio, OUTPUT);
  pinMode(CS_flash, OUTPUT);
  pinMode(CS_SD, OUTPUT);
  digitalWrite(CS_flash, HIGH);   //HIGH TURNS CHIP SELECT OFF
  digitalWrite(CS_radio, HIGH);
  digitalWrite(CS_flash, HIGH);
  
  Serial.begin(SERIAL_BAUD);
  delay(10);
  digitalWrite(CS_SD, LOW);
  if (!SD.begin(CS_SD)) {  Serial.println("failed"); return; }
  Serial.println("init OK.");
  // now open the data file gateway.txt and keep it open forever, simply flush after writing
  if ( dataFile = SD.open("gateway.txt", FILE_WRITE)); {
    Serial.println("Failed to open gateway.txt OK"); return;}
  
  digitalWrite(CS_SD, HIGH);  //Turn off SD card
  
  delay(100);
  digitalWrite(CS_radio,LOW);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  #ifdef IS_RFM69HWC
    radio.setHighPower(); //only for RFM69HW!
  #endif
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  digitalWrite(CS_radio,HIGH);
  
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  digitalWrite(CS_flash,LOW);
  if (flash.initialize())
  {
    Serial.print("SPI Flash Init OK. Unique MAC = [");
    flash.readUniqueId();
    for (byte i=0;i<8;i++)
    {
      Serial.print(flash.UNIQUEID[i], HEX);
      if (i!=8) Serial.print(':');
    }
    digitalWrite(CS_flash,HIGH);  //SPIFlash won't be used in this code
    Serial.println(']');
  }
  else
    Serial.println("SPI Flash MEM not found (is chip soldered?)...");

 GPS.begin(9600);
 GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
 //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); 
 GPS.sendCommand(PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ); 
 GPS.sendCommand(PMTK_API_SET_FIX_CTL_100_MILLIHERTZ);
 GPS.sendCommand(PGCMD_ANTENNA);
    
#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)");
#endif
}   //end setup

uint32_t timer = millis();
uint32_t timer2 = millis();

byte ackCount=0;
uint32_t packetCount = 0;



void loop() {
  if (millis() - timer2 > 5000) { //5 s blink rate
      timer2 = millis();
      Blink(100);
    }
  //process any serial input from GPS
  char c = GPS.read();
  String dataString = "";
    if (GPSECHO)
      if (c) Serial.print(c);
      
    if (GPS.newNMEAreceived()) {
      if (!GPS.parse(GPS.lastNMEA()))
      return;
    if (millis() - timer > 10*10) {
      timer = millis();    
      String dataString;  
      if (GPS.fix)   //assemble the dataString   
      {        //GPS Fix is true
      dataString = String(NODEID,DEC); dataString += " ";   
      if (String(GPS.hour,DEC).length() < 2) dataString += "0"; 
      dataString += String(GPS.hour,DEC); dataString += " ";
      if (String(GPS.minute,DEC).length() < 2) dataString += "0";
      dataString += String(GPS.minute,DEC); dataString += " ";
      if (String(GPS.seconds,DEC).length() < 2) dataString += "0";
      dataString += String(GPS.seconds,DEC); dataString += " ";
      if (String(GPS.day,DEC).length() < 2) dataString += "0"; 
      dataString += String(GPS.day,DEC); dataString += " ";
      if (String(GPS.month,DEC).length() < 2) dataString += "0";
      dataString += String(GPS.month,DEC); dataString += " ";
      if (String(GPS.year,DEC).length() < 2) dataString += "0";
      dataString += String(GPS.year,DEC); dataString += " ";
      float dlat = GPS.latitude;
      dlat = int(dlat/100) + (dlat-int(dlat/100)*100.)/60.;
      float dlong = GPS.longitude;
      dlong = int(dlong/100) + (dlong-int(dlong/100)*100.)/60.;
      //Serial.print(dlat,6); Serial.print(", "); Serial.println(dlong,6);
      dataString += String(dlat,6); dataString += " ";
      dataString += String(dlong,6); dataString += " ";  
      //Blink(100);        
      }
      else {
        dataString="No Fix on Gateway GPS";
      }
      //Blink(100);  
      Serial.println(dataString);
      delay(100);
      noInterrupts();
        digitalWrite(CS_SD, LOW);
          if (dataFile){
            dataFile.print(dataString); //writes string to datafile          
            dataFile.print("\n");
            dataFile.flush();
            }
  //        }
        } //end of ms timer section
        digitalWrite(CS_SD, HIGH);
      interrupts();
    
   } //end GPS.newNMEA section
   
  if (radio.receiveDone())  //check for node input
  {
    Serial.print("#[");
    Serial.print(++packetCount);
    Serial.print(']');
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    if (promiscuousMode)
    {
      Serial.print("to [");Serial.print(radio.TARGETID, DEC);Serial.print("] ");
    }
     
    Serial.print((char*)radio.DATA);
    Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");
    
    Serial.println();
    if (radio.ACKRequested())
    {
      byte theNodeID = radio.SENDERID;
      radio.sendACK();
//      Serial.println(" - ACK sent.");
    }
    
    delay(100);
    noInterrupts();
    digitalWrite(CS_SD, LOW);
    int senderID = radio.SENDERID;
    if (dataFile){
//      Serial.println("file made");
//      dataFile.print((char*)radio.DATA);dataFile.print("[");dataFile.print(radio.SENDERID, DEC);dataFile.print("]"); //writes string to datafile
        dataFile.print((char*)radio.DATA);
      dataFile.print("\n");
      dataFile.flush();
    }
//      Serial.println("Saved");
    digitalWrite(CS_SD, HIGH);
    interrupts();

      // When a node requests an ACK, respond to the ACK
      // and also send a packet requesting an ACK (every 3rd one only)
      // This way both TX/RX NODE functions are tested on 1 end at the GATEWAY
      /*if (ackCount++%3==0)
      {
        Serial.print(" Pinging node ");
        Serial.print(theNodeID);
        Serial.print(" - ACK...");
        delay(3); //need this when sending right after reception .. ?
        if (radio.sendWithRetry(theNodeID, "ACK TEST", 8, 0))  // 0 = only 1 attempt, no retries
          Serial.print("ok!");
        else Serial.print("nothing");

            }*/
        //flash.writeBytes(0, "hello world", 12);
        /*delay(1000);
        noInterrupts();
        digitalWrite(CS_SD, LOW);
        File dataFile = SD.open("datalog.txt", FILE_WRITE);
        if (dataFile){
          for(int i= 0; i < radio.DATALEN; i++){
            dataFile.print((char)radio.DATA[i]); //writes string to datafile
          }
          dataFile.print("\n");
          dataFile.close();
        digitalWrite(CS_SD, HIGH);*/
       
        //Blink(LED,3);
      } //end receive node data section
     
 }  //end loop

void Blink( int DELAY_MS)
{
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  digitalWrite(14,HIGH);
  digitalWrite(15,HIGH);
  delay(DELAY_MS);
  digitalWrite(14,LOW);
  digitalWrite(15,LOW);
}
