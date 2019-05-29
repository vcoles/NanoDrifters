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
Adafruit_GPS GPS(&Serial1);

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NODEID        1    //unique for each node on same network
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

int TRANSMITPERIOD = 1000; //transmit a packet to gateway so often (in ms)
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
const int CS_radio = 4; //varios chipselect pins needed so they dont interfere. Set to "HIGH" 
const int CS_flash = 23;  //to turn off

void setup() {
  Blink(100);
  pinMode(CS_radio, OUTPUT);
  pinMode(CS_flash, OUTPUT);
  pinMode(CS_SD, OUTPUT);
  
  digitalWrite(CS_flash, HIGH);
  digitalWrite(CS_radio, HIGH);
  
  
  Serial.begin(SERIAL_BAUD);
  delay(10);
  if (!SD.begin(CS_SD)) {
           Serial.println("failed");
           return;
         }
        Serial.println("init OK.");
 
  digitalWrite(CS_SD, HIGH);
  
  delay(100);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HWC
  radio.setHighPower(); //only for RFM69HW!
#endif
  radio.encrypt(ENCRYPTKEY);
  radio.promiscuous(promiscuousMode);
  //radio.setFrequency(919000000); //set frequency to some custom frequency
  char buff[50];
  sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  if (flash.initialize())
  {
    Serial.print("SPI Flash Init OK. Unique MAC = [");
    flash.readUniqueId();
    for (byte i=0;i<8;i++)
    {
      Serial.print(flash.UNIQUEID[i], HEX);
      if (i!=8) Serial.print(':');
    }
    Serial.println(']');
    
    //alternative way to read it:
    //byte* MAC = flash.readUniqueId();
    //for (byte i=0;i<8;i++)
    //{
    //  Serial.print(MAC[i], HEX);
    //  Serial.print(' ');
    //}
  }
  else
    Serial.println("SPI Flash MEM not found (is chip soldered?)...");

 GPS.begin(9600);
 GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
 GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); 
 GPS.sendCommand(PGCMD_ANTENNA);
    
#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)");
#endif
}

uint32_t timer = millis();


byte ackCount=0;
uint32_t packetCount = 0;
void loop() {
  //process any serial input
  char c = GPS.read();
  String dataString = "";
    if (GPSECHO)
      if (c) Serial.print(c);
    if (GPS.newNMEAreceived()) {
      if (!GPS.parse(GPS.lastNMEA()))
      return;
    if (millis() - timer > 500*20) {
      timer = millis();
      String dataString = "";
      if (GPS.fix) {
        if (String(GPS.hour,DEC).length() < 2){ //making sure that every time
          dataString += "0";                    //the data is the same length for
        }                                       //memory formatting's sake
        dataString += String(GPS.hour,DEC);
        dataString += " ";
     
        if (String(GPS.minute,DEC).length() < 2){
          dataString += "0";
        }
        dataString += String(GPS.minute,DEC);
        dataString += " ";
      
        if (String(GPS.seconds,DEC).length() < 2){
          dataString += "0";
        }
        dataString += String(GPS.seconds,DEC);
        dataString += " ";
      
        if (String(GPS.day,DEC).length() < 2){
          dataString += "0";
        }
        dataString += String(GPS.day,DEC);
        dataString += " ";
      
        if (String(GPS.month,DEC).length() < 2){
          dataString += "0";
        }
        dataString += String(GPS.month,DEC);
        dataString += " ";
      
        if (String(GPS.year,DEC).length() < 2){
          dataString += "0";
        }
        dataString += String(GPS.year,DEC);
        dataString += " ";
        dataString += String(GPS.latitude,4);
        dataString += " ";
        dataString += String(GPS.longitude,4);
        dataString += " ";     
        
        
        Blink(100);
        
      }
      else Serial.println("no fix");
      Blink(100);  

      Serial.println(dataString);
        delay(1000);
        noInterrupts();
        digitalWrite(CS_SD, LOW);
        if (File dataFile = SD.open("gateway.txt", FILE_WRITE)){
          Serial.println("SDopen");
        
          if (dataFile){
            Serial.println("yes");
            dataFile.print(dataString); //writes string to datafile
            }
            dataFile.print("\n");
            dataFile.close();
        }
      digitalWrite(CS_SD, HIGH);
      interrupts();
    }
   }
  if (radio.receiveDone())
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
      Serial.print(" - ACK sent.");
    }
    
    delay(1000);
    noInterrupts();
    digitalWrite(CS_SD, LOW);
    File dataFile = SD.open("datalog" + String(radio.SENDERID) + ".txt", FILE_WRITE);
    Serial.println("SDopen");
    if (dataFile){
      dataFile.print((char*)radio.DATA);dataFile.print("[");dataFile.print(radio.SENDERID, DEC);dataFile.print("]"); //writes string to datafile
       }
      dataFile.print("\n");
      dataFile.close();
      Serial.println("Saved");
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
      }
    
    
}

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
