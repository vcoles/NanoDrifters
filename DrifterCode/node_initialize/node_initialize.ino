// Sketch to initialize drifter with its NODEID in EEPROM(0) as one byte

#include <EEPROM.h>

#if !defined(ARDUINO_ARCH_SAM) && !defined(ARDUINO_ARCH_SAMD) && !defined(ESP8266) && !defined(ARDUINO_ARCH_STM32F2)
#include <SoftwareSerial.h>
#endif
#define SERIAL_BAUD   115200
byte NODEID = 101;
byte MAXSPIFlash = 4; //Mb

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_BAUD);
  EEPROM.write(0,NODEID);
  EEPROM.write(1,MAXSPIFlash);
  Serial.println("Write NODEID to EPROM(0) and MAXSPIFlash to EEPROM(1):");
  Serial.print(NODEID,DEC); Serial.print(", ");Serial.println(MAXSPIFlash);
  //Now check that it is there correctly
  byte stored_nodeid = EEPROM.read(0);
  Serial.println("Stored NODEID in EPROM(0):");
  Serial.println(stored_nodeid,DEC);
  byte stored_maxspiflash = EEPROM.read(1);
  Serial.println("Stored MAXSPIFlash in EPROM(1):");
  Serial.println(stored_maxspiflash,DEC);
}

void loop() {
  // Dummy loop

}
