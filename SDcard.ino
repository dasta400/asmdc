/*******************************************************************************
 * SDcard.ino
 *               ___________   _____               _
 *              /  ___|  _  \ /  __ \             | |
 *              \ `--.| | | | | /  \/ __ _ _ __ __| |
 *               `--. \ | | | | |    / _` | '__/ _` |
 *              /\__/ / |/ /  | \__/\ (_| | | | (_| |
 *              \____/|___/    \____/\__,_|_|  \__,_|
 *
 * for dastaZ80's dzOS
 * by David Asta (Oct 2022)
 * 
 * Version 1.0.0
 * Created on 30 Oct 2022
 * Last Modification 30 Oct 2022
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

#include <SD.h>
#include <SPI.h>
#include "ASMDC_defs.h"

Sd2Card card;
SdVolume volume;
File root;
File sdimg;
bool sdcard_ok;
bool sdimg_ok;
bool lastcmd_ok;
bool sd_isbusy;
byte sector[SD_SECSIZE];

/*****************************************************************************/
/* Initial set up of the SD Card Module                                      */
/*****************************************************************************/
void sd_setup(){
    pinMode(SD_BUSY_LED, OUTPUT);
    digitalWrite(SD_BUSY_LED, LED_OFF);

    lastcmd_ok  = true;
    sd_isbusy   = false;

    // Detect SD Card
    digitalWrite(SD_BUSY_LED, LED_ON);
    if(sdcard_ok = SD.begin(SD_CS)){
        if(DEBUG){ Serial.println("    SD card found."); }
        // Detect (and open) Image file
        if(sdimg_ok = SD.exists("DASTAZ80.IMG")){
            if(DEBUG){ Serial.println("    Image found."); }
            sdimg = SD.open(SD_IMGNAME, O_RDWR);
            sdimg.seek(0);
        }
    }
    digitalWrite(SD_BUSY_LED, LED_OFF);

    if(DEBUG && !sdcard_ok){ Serial.println("    SD card NOT found."); }
    if(DEBUG && !sdimg_ok){ Serial.println("    Image NOT found."); }
}

/*****************************************************************************/
/* Sends a byte that tells the status of the SD Card reader                  */
/* bit 0 = set if SD card was not found                                      */
/* bit 1 = set if image file was not found                                   */
/* bit 2 = set if last command resulted in error                             */
/*****************************************************************************/
void sd_cmd_status(){
    uint8_t status = 0;

    if(!sdcard_ok){     // SD card was found
        status = status | 0x01;
    }

    if(!sdimg_ok){      // Image file was found
        status = status | 0x02;
    }

    if(!lastcmd_ok){    // Last command was not successful
        status = status | 0x04;
    }

    if(DEBUG){ Serial.print("Status: "); Serial.println(status); }
    Serial1.write(status);
}

/*****************************************************************************/
/* Closes the image file                                                     */
/*****************************************************************************/
void smd_cmd_close_img(){
    if(DEBUG){ Serial.print("Closing Image file: "); Serial.println(sdimg.name()); }

    sdimg.close();

    if(DEBUG){ Serial.println("   Closed"); }
}

/*****************************************************************************/
/* Opens the image file                                                      */
/*****************************************************************************/
void smd_cmd_open_img(){
    if(DEBUG){ Serial.print("Opening Image file: "); Serial.println(SD_IMGNAME); }

    sdimg = SD.open(SD_IMGNAME, O_RDWR);
    sdimg.seek(0);

    if(DEBUG){ Serial.println("   Opened"); }
}

/*****************************************************************************/
/* Sends a byte that tells if the SD Card reader is busy or not              */
/* 0x00 = Not busy                                                           */
/* 0x01 = Busy                                                               */
/*****************************************************************************/
void sd_cmd_busy(){
    if(sd_isbusy) Serial1.write(0x01);
    else Serial1.write(0x00);

    if(DEBUG){ Serial.print("Busy: "); Serial.println(sd_isbusy); }
}

/*****************************************************************************/
/* Read a sector (512 bytes) from the SD Card and send the bytes via the     */
/* serial port.                                                              */
/*****************************************************************************/
void sd_cmd_read_sector(byte *rcvd_bytes){
    uint8_t sector_num_msb, sector_num_lsb;
    uint32_t sector_num;

    sector_num_lsb = rcvd_bytes[1];
    sector_num_msb = rcvd_bytes[2];

    sector_num = 0;
    sector_num = ((0 << 24) + (0 << 16) + (sector_num_msb << 8) + sector_num_lsb);
    sector_num *= SD_SECSIZE;

    if(DEBUG){
        Serial.print("Reading Sector Offset: ");
        Serial.print(sector_num);
        Serial.print(" (0x");
        Serial.print(sector_num, HEX);
        Serial.println(")");
    }

    // Read Sector
    sd_isbusy = true;
    digitalWrite(SD_BUSY_LED, LED_ON);
    sdimg.seek(sector_num);
    sdimg.read(sector, SD_SECSIZE);

    // Send read bytes to dastaZ80
    for(int b=0; b<SD_SECSIZE; b++){
        Serial1.write(sector[b]);
    }

    sd_isbusy = false;
    digitalWrite(SD_BUSY_LED, LED_OFF);
}

/*****************************************************************************/
/* Write a sector (512 bytes) to the SD Card                                 */
/* Returns the position of the last byte used from the buffer rcvd_bytes     */
/*  last byte used = 515 = 1 (AMDC command) + 2 (sector number) + 512 (data) */
/*****************************************************************************/
int sd_cmd_write_sector(byte *rcvd_bytes){
    uint8_t sector_num_msb, sector_num_lsb;
    uint8_t buffer[SD_SECSIZE];
    uint32_t sector_num;
    size_t written_bytes;
    int b;

    sector_num_lsb = rcvd_bytes[1];
    sector_num_msb = rcvd_bytes[2];

    for(b=0; b<SD_SECSIZE; b++){
        buffer[b] = rcvd_bytes[b + 3]; // Discard first 3 bytes (command, sector number)
    }

    sector_num = 0;
    sector_num = ((0 << 24) + (0 << 16) + (sector_num_msb << 8) + sector_num_lsb);
    sector_num *= SD_SECSIZE;

    if(DEBUG){
        Serial.print("Writing in Sector Offset: ");
        Serial.print(sector_num);
        Serial.print(" (0x");
        Serial.print(sector_num, HEX);
        Serial.println(")");
    }

    // Write Sector
    sd_isbusy = true;
    digitalWrite(SD_BUSY_LED, LED_ON);

    sdimg.seek(sector_num);
    written_bytes = sdimg.write(buffer, SD_SECSIZE);
    sdimg.flush();

    if(DEBUG){
        Serial.print("    Written: ");
        Serial.print(written_bytes);
        Serial.println(" bytes");
    }

    sd_isbusy = false;
    digitalWrite(SD_BUSY_LED, LED_OFF);

    return b + 3;
}
