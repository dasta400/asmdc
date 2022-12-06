/*******************************************************************************
 * FDD.ino
 *                           _____ ____  ____
 *                          |  ___|  _ \|  _ \
 *                          | |_  | | | | | | |
 *                          |  _| | | | | | | |
 *                          | |   | |_| | |_| |
 *                          |_|   |____/|____/
 *
 * for dastaZ80's dzOS
 * by David Asta (Oct 2022)
 * 
 * Version 1.0.0
 * Created on 28 Nov 2022
 * Last Modification 28 Nov 2022
 *******************************************************************************
 * CHANGELOG
 *   -
 *******************************************************************************
 */

/* ---------------------------LICENSE NOTICE--------------------------------
 *  MIT License
 *  
 *  Copyright (c) 2022 David Asta
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

/*****************************************************************************/
/* FDD Cable IBM PC-compatible standard FDD (tested with SAMSUNG SFD-321B/E) */
/*      FDD Connector   Function                    Arduino Mega2560         */
/*           2          Density Select                    42                 */
/*           4          No connection                     No connection      */
/*           6          No connection                     No connection      */
/*           8          Index                             47                 */
/*          10          MO-0                              39                 */
/*          12          DS1                               No connection      */
/*          14          DS0                               38                 */
/*          16          MO-1                              No connection      */
/*          18          Direction                         40                 */
/*          20          Step                              41                 */
/*          22          Write Data                        46                 */
/*          24          Write Enable                      45                 */
/*          26          Track 00                          44                 */
/*          28          Write Protect                     43                 */
/*          30          Read Data                         48                 */
/*          32          Head Select                       49                 */
/*          34          Disk Change                       No connection      */
/*          1,3,5,...,31,33                               GND                */
/*****************************************************************************/
/* FDD Cable original A3010 FDD (Citizen OSDA-72C)                           */
/*      FDD Connector   Function                    Arduino Mega2560         */
/*           1          No connection                     No connection      */
/*           2          Mode Select (1MB/2MB)             42                 */
/*           4          No connection                     No connection      */
/*           6          No connection                     No connection      */
/*           8          Index                             47                 */
/*          10          DS0                               38                 */
/*          12          No connection                     No connection      */
/*          14          No connection                     No connection      */
/*          16          MOTOR ON                          39                 */
/*          18          Direction                         40                 */
/*          20          Step                              41                 */
/*          22          Write Data                        46                 */
/*          24          Write Gate                        45                 */
/*          26          Track 0                           44                 */
/*          28          Write Protect                     43                 */
/*          30          Read Data                         48                 */
/*          32          Side 1                            49                 */
/*          34          Disk Change                       No connection      */
/*          3,5,...,31,33                                 GND                */
/*****************************************************************************/

#include "ArduinoFDC.h"
#include "ASMDC_defs.h"

byte disk_buffer[FDD_SECSIZE];
uint8_t SectorsPerTrack;
uint16_t SectorsPerSide;
uint8_t TracksPerSide;
bool lastFDDcmd_ok;
bool fdd_isbusy;
uint8_t fdderrcode;
uint8_t fdd_side;
uint8_t fdd_track;
uint8_t fdd_sector;

/*****************************************************************************/
/* Convert LBA to Side-Track-Sector                                          */
/*      SectorsPerTrack = 18 [HD] / 9 [DD]                                   */
/*      TracksPerSide = 80 [HD/DD]                                           */
/*      SectorsPerSide = 1440 (18 * 80) [HD] / 720 (9 * 80) [DD]             */
/*                                                                           */
/*      Side = int(LBA / SectorsPerSide)                                     */
/*      Track = int((LBA / SectorsPerTrack) - (Side * TracksPerSide))        */
/*      Sector = (LBA mod SectorsPerTrack) + 1                               */
/*****************************************************************************/
void LBA_to_STS(uint16_t lba){
    if((lba / SectorsPerTrack) < 18) fdd_side = 0;
    else fdd_side = 1;

    if((lba / SectorsPerTrack) <= 79) fdd_track = lba / SectorsPerTrack;
    else fdd_track = (lba / SectorsPerTrack) - TracksPerSide;

    fdd_sector  = (lba % SectorsPerTrack) + 1;
}

/*****************************************************************************/
/* Initial set up of the SD Card Module                                      */
/*****************************************************************************/
void fdd_setup(){

    lastFDDcmd_ok  = true;
    fdd_isbusy = false;
    fdderrcode = 0;

    ArduinoFDC.begin(ArduinoFDCClass::DT_3_HD, ArduinoFDCClass::DT_3_HD);
    SectorsPerTrack = 18;
    SectorsPerSide = 1440;
    TracksPerSide = 80;

    if(DEBUG){ Serial.println("    Floppy Drive initialised as 3.5\" HD"); }
}

/*****************************************************************************/
/* Turns the FDD motor ON                                                    */
/*****************************************************************************/
void fdd_cmd_motor(bool motoron){
    if(motoron) ArduinoFDC.motorOn();
    else        ArduinoFDC.motorOff();
}

/*****************************************************************************/
/* Sends a byte that tells the status of the FDD                             */
/*      Low Nibble (0x00 if all OK)                                          */
/*          bit 0 = not used                                                 */
/*          bit 1 = not used                                                 */
/*          bit 2 = set if last command resulted in error                    */
/*          bit 3 = not used                                                 */
/*      High Nibble (error code)                                             */
/*        Error codes: https://github.com/dhansel/ArduinoFDC#troubleshooting */
/*****************************************************************************/
void fdd_cmd_status(){
    uint8_t status = 0;

    // Put in High Nibble the error code
    status = fdderrcode << 4;

    if(!lastFDDcmd_ok){    // Last command was not successful
        status = status | 0x04;
    }

    if(DEBUG){ Serial.print("FDD Status: "); Serial.println(status); }
    Serial1.write(status);
}

/*****************************************************************************/
/* Sends a byte that tells if the FDD is busy or not                         */
/* 0x00 = Not busy                                                           */
/* 0x01 = Busy                                                               */
/*****************************************************************************/
void fdd_cmd_busy(){
    if(fdd_isbusy) Serial1.write(0x01);
    else Serial1.write(0x00);

    if(DEBUG){ Serial.print("Busy: "); Serial.println(fdd_isbusy); }
}

/*****************************************************************************/
/* Read a sector (512 bytes) from the Floppy Disk and send the bytes via the */
/* serial port.                                                              */
/*****************************************************************************/
void fdd_cmd_read_sector(byte *rcvd_bytes){
    uint8_t sector_num_msb, sector_num_lsb;
    uint32_t sector_num;
    int b;

    sector_num_lsb  = rcvd_bytes[2];
    sector_num_msb  = rcvd_bytes[3];
    sector_num = ((0 << 24) + (0 << 16) + (sector_num_msb << 8) + sector_num_lsb);

    LBA_to_STS(sector_num);

    if(DEBUG){
        Serial.println("Reading FDD");
        Serial.print("  Sector Offset: ");
        Serial.print(sector_num);
        Serial.print(" (0x");
        Serial.print(sector_num, HEX);
        Serial.println(")");
        Serial.print("  Side  : ");
        Serial.println(fdd_side);
        Serial.print("  Track : ");
        Serial.println(fdd_track);
        Serial.print("  Sector: ");
        Serial.println(fdd_sector);
    }

    // Read Sector
    fdd_isbusy = true;
    fdderrcode = ArduinoFDC.readSector(fdd_track, fdd_side, fdd_sector, disk_buffer);

    // If there was an error, dastaZ80 is still expecting to receive 512 bytes
    // so we send the error code 512 times. Oops!
    if(fdderrcode == 0){
        lastFDDcmd_ok = true;
        for(b=1; b<=FDD_SECSIZE; b++){  // For FDD buffer must be 1..512
            Serial1.write(disk_buffer[b]);
        }
    }else{
        if(DEBUG){ Serial.print("   ERROR: "); Serial.println(fdderrcode); }
        lastFDDcmd_ok = false;
        for(b=0; b<FDD_SECSIZE; b++){
            Serial1.write(fdderrcode);
        }
    }

    fdd_isbusy = false;
}

/*****************************************************************************/
/* Write a sector (512 bytes) to the Floppy Disk                             */
/* It verifies that that was written correctly by reading it back            */
/* Returns the position of the last byte used from the buffer rcvd_bytes     */
/*  last byte used = 517 = 1 (AMDC command) + 2 (sector number)              */
/*                         + 1 (track number) + 1 (side) + 512 (data)        */
/*****************************************************************************/
int fdd_cmd_write_sector(byte *rcvd_bytes){
    uint8_t sector_num_msb, sector_num_lsb;
    uint32_t sector_num;
    int b;

    sector_num_lsb  = rcvd_bytes[2];
    sector_num_msb  = rcvd_bytes[3];
    sector_num = ((0 << 24) + (0 << 16) + (sector_num_msb << 8) + sector_num_lsb);

    LBA_to_STS(sector_num);

    for(b=1; b<=FDD_SECSIZE; b++){   // For FDD buffer must be 1..512
        disk_buffer[b] = rcvd_bytes[b + 3]; // Discard first 4 bytes (command, img number, sector number (lsb & msb)
                                            // But for FDD buffer is 1..512
    }

    if(DEBUG){
        Serial.println("Writing FDD");
        Serial.print("  Sector Offset: ");
        Serial.print(sector_num);
        Serial.print(" (0x");
        Serial.print(sector_num, HEX);
        Serial.println(")");
        Serial.print("  Side  : ");
        Serial.print(fdd_side);
        Serial.print("  Track : ");
        Serial.print(fdd_track);
        Serial.print("  Sector: ");
        Serial.println(fdd_sector);
    }

    // Write Sector and return code
    fdd_isbusy = true;
    fdderrcode = ArduinoFDC.writeSector(fdd_track, fdd_side, fdd_sector, disk_buffer, true);

    if(fdderrcode == 0){
        lastFDDcmd_ok = true;
    }else{
        lastFDDcmd_ok = false;
        if(DEBUG){ Serial.print("   ERROR: "); Serial.println(fdderrcode); }
    }

    fdd_isbusy = false;

    return FDD_SECSIZE + 4;
}

/*****************************************************************************/
/* Formats a floppy disk according to the format specified by the drive type */
/* The format is low-level, meaning that no fily system is stored in the disk*/
/* but just a low-level sector structure that allows reading and writing.    */
/* Data is filled with 0xF6 bytes.                                           */
/*****************************************************************************/
void fdd_lowlvl_format(){
    byte buffer[144];

    if(DEBUG){ Serial.println("Low-Level Formatting Floppy Disk"); }

    fdderrcode = ArduinoFDC.formatDisk(buffer, 0, 255);

    if(fdderrcode == 0){
        lastFDDcmd_ok = true;
        Serial1.write(0x00);
        if(DEBUG){ Serial.println("   Success!"); }
    }else{
        Serial1.write(0xFF);
        lastFDDcmd_ok = false;
        if(DEBUG){ Serial.print("   ERROR: "); Serial.println(fdderrcode); }
    }
}

/*****************************************************************************/
/* Set drive drive type                                                      */
/*   isHD = true  => High-Density                                            */
/*   isHD = false => Double-Density                                          */
/*****************************************************************************/
void fdd_set_disk_type(bool isHD){
    if(isHD){
        ArduinoFDC.setDriveType(ArduinoFDCClass::DT_3_HD);
        if(DEBUG){ Serial.print("Floppy Drive set to 3.5\" HD"); }
    }else{
        ArduinoFDC.setDriveType(ArduinoFDCClass::DT_3_DD);
        if(DEBUG){ Serial.print("Floppy Drive set to 3.5\" DD"); }
    }
}

/*****************************************************************************/
/* Checks if a disk is in the drive                                          */
/* Sends 0x00 if yes. Otherwise, sends 0xFF                                  */
/*****************************************************************************/
void fdd_check_disk_in(){
    if(ArduinoFDC.haveDisk()){
        Serial1.write(0x00);
        if(DEBUG){ Serial.println("Disk is in the Floppy Drive"); }
    }else{
        Serial1.write(0xFF);
        if(DEBUG){ Serial.println("The Floppy Drive is empty"); }
    }
}

/*****************************************************************************/
/* Checks if a disk is Write Protected                                       */
/* Sends 0x00 if yes. Otherwise, sends 0xFF                                  */
/*****************************************************************************/
void fdd_check_wr_protect(){
    if(ArduinoFDC.isWriteProtected()){
        Serial1.write(0x00);
        if(DEBUG){ Serial.println("Disk is Write Protected"); }
    }else{
        Serial1.write(0xFF);
        if(DEBUG){ Serial.println("Disk is NOT Write Protected"); }
    }
}