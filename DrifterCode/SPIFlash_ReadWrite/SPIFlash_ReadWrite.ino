// **********************************************************************************
// This version has been modified to dump drifter data to screen
//
// This sketch is an example of using the SPIFlash library with a Moteino
// that has an onboard SPI Flash chip. This sketch listens to a few serial commands
// Hence type the following commands to interact with the SPI flash memory array:
// - 'd' dumps the first 256bytes of the flash chip to screen
// - 'e' erases the entire memory chip
// - 'i' print manufacturer/device ID
// - [0-9] writes a random byte to addresses [0-9] (either 0xAA or 0xBB)
// Get the SPIFlash library from here: https://github.com/LowPowerLab/SPIFlash
// **********************************************************************************
// Copyright Felix Rusu, LowPowerLab.com
// Library and code by Felix Rusu - felix@lowpowerlab.com
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// You should have received a copy of the GNU General    
// Public License along with this program.
// If not, see <http://www.gnu.org/licenses/>.
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************


#include <SPIFlash.h>    //get it here: https://github.com/LowPowerLab/SPIFlash
#include <SPI.h>
#include <EEPROM.h>

#define SERIAL_BAUD      115200
//#define SERIAL_BAUD      9600

char input = 0;
long lastPeriod = -1;

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif

//////////////////////////////////////////
// flash(SPI_CS, MANUFACTURER_ID)
// SPI_CS          - CS pin attached to SPI flash chip (8 in case of Moteino)
// MANUFACTURER_ID - OPTIONAL, 0x1F44 for adesto(ex atmel) 4mbit flash
//                             0xEF30 for windbond 4mbit flash
//////////////////////////////////////////
SPIFlash flash(FLASH_SS);

void setup(){
  Serial.begin(SERIAL_BAUD);
  Serial.print("Start...");
  byte stored_nodeid = EEPROM.read(0);
  Serial.println("Stored NODEID in EPROM(0):");
  Serial.println(stored_nodeid,DEC);
  byte stored_maxspiflash = EEPROM.read(1);
  Serial.println("Stored MAXSPIFlash in EPROM(1):");
  Serial.print(stored_maxspiflash,DEC); Serial.println(" Mb");

  if (flash.initialize())
  {
    Serial.println("Init OK!");
    Blink(LED, 20, 10);
  }
  else
    Serial.println("Init FAIL!");
  Serial.println("i for info, d for dump, e for erase");
  delay(1000);
}

void loop(){
  // Handle serial input (to allow basic DEBUGGING of FLASH chip)
  // ie: display first 256 bytes in FLASH, erase chip, write bytes at first 10 positions, etc
  if (Serial.available() > 0) {
    input = Serial.read();
    if (input == 'd') //d=dump flash area
    {
      int lengthData = 42;
      char flashbuff[lengthData];
      long adress = 0;
      Serial.println("Flash content:");
      long counter = 0;
      long cmax = 100;

      while(counter<=cmax){
        flash.readBytes(adress,flashbuff,lengthData);
        //flashbuff[37]=0;
        if (flashbuff[0] > 47 && flashbuff[0] < 51) 
        {
        Serial.println(flashbuff);
        counter++;
        adress += lengthData;}
        else
        {counter=cmax;}
        }
    }
    else if (input == 'e')
    {
      Serial.print("Erasing Flash chip ... ");
      flash.chipErase();
      while(flash.busy());
      Serial.println("DONE");
    }
    else if (input == 'i')
    {
      Serial.print("DeviceID: ");
      Serial.println(flash.readDeviceId(), HEX);
    }
    else if (input >= 48 && input <= 57) //0-9
    {
      Serial.print("\nWriteByte("); Serial.print(input); Serial.print(")");
      flash.writeByte(input-48, millis()%2 ? 0xaa : 0xbb);
    }
    else if (input == 'b')
    {
    Serial.print(flash.readByte(0),HEX);
    Serial.print(flash.readByte(1),HEX);
    Serial.print(flash.readByte(2),HEX);
    Serial.print(flash.readByte(3),HEX);
    }

  }

  // Periodically blink the onboard LED while listening for serial commands
  if ((int)(millis()/500) > lastPeriod)
  {
    lastPeriod++;
    pinMode(LED, OUTPUT);
    digitalWrite(LED, lastPeriod%2);
  }
}

void Blink(byte PIN, int DELAY_MS, byte loops)
{
  pinMode(PIN, OUTPUT);
  while (loops--)
  {
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);  
  }
}
