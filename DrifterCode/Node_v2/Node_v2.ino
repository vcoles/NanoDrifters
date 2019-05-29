// This is a working code. Use a higher version number for development
//

//rainier git test
#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>//get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>
#include <SPIFlash.h> //get it here: https://www.github.com/lowpowerlab/spiflash
#include <Adafruit_GPS.h>
#include <EEPROM.h>

#if !defined(ARDUINO_ARCH_SAM) && !defined(ARDUINO_ARCH_SAMD) && !defined(ESP8266) && !defined(ARDUINO_ARCH_STM32F2)
#include <SoftwareSerial.h>
#endif
Adafruit_GPS GPS(&Serial1);
//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NETWORKID     100  //the same on all nodes that talk to each other (range up to 255)
#define GATEWAYID     101
#define DUMPID        102    //this is the RF node to dump spiflash to
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY   RF69_433MHZ
//#define FREQUENCY   RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HWC    //uncomment only for RFM69HW! Leave out if you have RFM69W!
//#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
//*********************************************************************************************

boolean GPSECHO = false;
#define LED 15// Moteino MEGAs have LEDs on D15
#define LED 14
boolean ledval = false;
#define FLASH_SS      23 // and FLASH SS on D23
#define SERIAL_BAUD   115200
uint8_t NODEID=100;
uint8_t MAXSPIFlash=4;  //max address in Mb;
int16_t packetnum = 0;  // packet counter, we increment per xmission

int TRANSMITPERIOD = 1000; //transmit a packet to gateway so often (in ms)
char buff[20];
char radio_input;
boolean requestACK = false;
// SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)
// 0xEF70 for 64mbit  Windbond chip (W25Q...) omit to read any chip
SPIFlash flash(FLASH_SS); // this will handle 4Mb, 64Mb and 128Mb chips

long flash_address;
long max_flash_address;

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

bool ACK = false;
bool dumped = true; 
bool debug = true;
char input = Serial.read();
int total_len = 0;
int dataString_len = 42;  //38 for GPS + 4 for NODEID
//int num_itts = 1;

//int total_len_saved = 0;
int num_itts_total = 0;//num itterations

const int CS_flash = 8;
const int CS_radio = 4;
//if not work add the other three pins

void setup() 
{
  pinMode(CS_radio, OUTPUT);
  pinMode(CS_flash, OUTPUT);

  digitalWrite(CS_flash, HIGH);
  digitalWrite(CS_radio, HIGH);

  Serial.begin(SERIAL_BAUD);
  NODEID = EEPROM.read(0);
  Serial.print("Got NODEID from EPROM(0):");
  Serial.println(NODEID,DEC);
  Serial.print("Using node = "); Serial.print(GATEWAYID); Serial.println(" as GATEWAYID");
  Serial.print("Using node = "); Serial.print(DUMPID); Serial.println(" as DUMPID");
  MAXSPIFlash = EEPROM.read(1);
  Serial.print("Got MAXSPIFlash from EPROM(1):");
  Serial.println(MAXSPIFlash,DEC);
  max_flash_address = MAXSPIFlash*1000000/8;
  Serial.print("max_flash_address (bytes) =");Serial.println(max_flash_address,DEC);
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW
  radio.setHighPower(); //uncomment only for RFM69HW!
#endif
  radio.encrypt(ENCRYPTKEY);
  //radio.setFrequency(919000000); //set frequency to some custom frequency

//Auto Transmission Control - dials down transmit power to save battery (-100 is the noise floor, -90 is still pretty good)
//For indoor nodes that are pretty static and at pretty stable temperatures (like a MotionMote) -90dBm is quite safe
//For more variable nodes that can expect to move or experience larger temp drifts a lower margin like -70 to -80 would probably be better
//Always test your ATC mote in the edge cases in your own environment to ensure ATC will perform as you expect
#ifdef ENABLE_ATC
  radio.enableAutoPower(-70);
#endif

  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  Serial.println("Wait for GPS fix");
  
  digitalWrite(CS_flash, LOW);

  if (flash.initialize())
  {
    Serial.print("SPI Flash Init OK ... DeviceID: ");
    Serial.println(flash.readDeviceId(), HEX);
    Serial.print("SPI Flash UniqueID (MAC): ");
    flash.readUniqueId();
    for (byte i=0;i<8;i++)
    {
      Serial.print(flash.UNIQUEID[i], HEX);
    }
    Serial.println();
    //find first empty flash address
    flash_address = 0;
    while(flash.readByte(flash_address)!=0xFF){
      flash_address += 42;}
      
    Serial.print("SPI Flash starting address:");
    Serial.print(flash_address);
    Serial.println();
  }
  else
    {Serial.println("SPI Flash MEM not found (is chip soldered?)...");}

  digitalWrite(CS_flash, HIGH);
 
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_100_MILLIHERTZ); 
  GPS.sendCommand(PMTK_API_SET_FIX_CTL_100_MILLIHERTZ);
  GPS.sendCommand(PGCMD_ANTENNA);

#ifdef ENABLE_ATC
  Serial.println("RFM69_ATC Enabled (Auto Transmission Control)\n");
#endif

Serial.println("To start sampling GPS enter g for this node or G for all nodes");
Serial.println("If you start with serial monitor debug output will be on");
Serial.println("If you start with the RF controller debug output will be off");

} //end setup

uint32_t timer = millis();

void Blink(int DELAY_MS)
{
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  digitalWrite(14,LOW);
  digitalWrite(15, LOW);
  delay(DELAY_MS);
  digitalWrite(14,HIGH);
  digitalWrite(15,HIGH);
}

void Blink_nodelay(int DELAY_MS)
{
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  if(ledval==false) {
    digitalWrite(14,HIGH);
    digitalWrite(15, HIGH);
    ledval = true;
    timer=millis();
  }
  else {
    if(millis()-timer > DELAY_MS) {
      digitalWrite(14,LOW);
      digitalWrite(15,LOW);
      ledval = false;
    } 
  } 
}

long lastPeriod = 0;

void loop() {   //begin main loop
  int input;
  int radio_input;
  if (dumped == false){
  //process any serial input
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
      //dataString += String(GPS.latitude,4); dataString += " ";
      //dataString += String(GPS.longitude,4); dataString += " "; 
        if(debug){
        Serial.print("Fix: "); Serial.print((int)GPS.fix); Serial.println(); }          
        int dataString_len = dataString.length();
        char radiopacket[dataString_len];      
        dataString.toCharArray(radiopacket, dataString_len+1);
        delay(100); 
        if(debug){
        Serial.println("Writing to SPIFlash "); }
        noInterrupts();
        digitalWrite(CS_flash, LOW);
        digitalWrite(CS_radio, HIGH);       
        if(flash_address < max_flash_address){
        flash.writeBytes(flash_address, radiopacket, strlen(radiopacket));
        delay(100);
        flash_address += 42;
        digitalWrite(CS_flash, HIGH);
        interrupts();}
        else{Serial.println("Stopped writing to SPIFlash, max address exceeded:");}
        num_itts_total += 1;
        if(debug){
        Serial.print("num iterations:");
        Serial.println(num_itts_total);        
        delay(100);
        Serial.println("Sending to RF ");}
        digitalWrite(CS_radio, LOW);       
        radio.sendWithRetry(GATEWAYID, radiopacket, strlen(radiopacket));
        Blink(1000);  //double blink if Fix
        delay(1000);
        Blink(1000);
        //delay(500);
        //Blink(1000);
        ACK = false;
        } //end GPS fix = true section      
        else 
        {dataString = "No GPS fix";}
        Blink(1000);    //single blink if no fix
        if(debug){
        Serial.println(dataString);}
       } //end if timer section 
     } //end of newNMEA received section
     
  //check for any received packets
      if (radio.receiveDone()){
        if(radio.SENDERID == 102){
          radio_input=radio.DATA[0];
        }     
        if (radio.ACKRequested() && radio.SENDERID != 102) {
          radio.sendACK();  Serial.print(" - ACK sent");
        }
      Blink(200);
      Serial.println();
      }
  } //end if dumped == false section
  // loop runs through here always whether or not GPS data is present
  //    int input;
  //    int radio_input = 'x';
    /*  int bufferlen = 4;
      char buffer[bufferlen];
      Serial.readBytes(buffer,bufferlen); */
   // Here we ask for a radio byte in case dumped == true
      if (radio.receiveDone() && radio.SENDERID == 102){
         radio_input = radio.DATA[0];
         //Serial.println((char)radio_input);
         }
    
      char flashbuff[dataString_len];
      long adress = 0;
      if (Serial.available() > 0) {
      input=Serial.read();}
      if( input =='t'  ) {
    Serial.println("Test");  
   }
   if ((input == 'd') || (radio_input =='d') ) //d=dump flash area
     {  
      Serial.println("Dumping flash");
      radio_input = ' '; input=' ';   
      int i = 0;
      boolean exit=false;
        while(exit == false){ 
            flash.readBytes(adress, flashbuff, dataString_len);
            if (flashbuff[0] > 47 && flashbuff[0] < 51)
            {
            if(debug) {
              Serial.println(String(flashbuff));
              }
              else{
            radio.sendWithRetry(DUMPID,flashbuff,dataString_len,0);
              }
            adress += dataString_len ;
            i++;
            }
            else{ exit=true;}        
        }
        //dumped = true;
        //flash.chipErase();
        //adress = 0;
     }
   //if((dumped == false) && ((input == 's') || (radio_input =='s'))) {
   if((input == 's') || (radio_input =='s')) {
    Serial.println("Stop reading GPS");
    dumped = true;
   }
   if((dumped == true) && ((input == 'g') || (radio_input =='g'))) {
    Serial.println("Start reading GPS");
    dumped = false;      
   }
   if((debug == true) && ((radio_input == 'g') || (radio_input == 'G'))){
    debug=false;      
    }
   
 
 /*  erasing is probably not good here
     if(input == 'e') //e=erase flash
     {
     Serial.print("Erasing Flash chip PLEASE WAIT ");
     noInterrupts();
     flash.chipErase();    
     while(flash.busy());
     interrupts();
     Serial.println("DONE");
     //dumped==true;}    
     }
 */ 

} // end loop


