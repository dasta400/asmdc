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
bool sdcard_ok;
bool sdimg_ok;
bool lastSDcmd_ok;
bool sd_isbusy;
byte sector[SD_SECSIZE];        // Sector buffer
String disks_names[16];         // list of disk images (0 not used. It's the FDD)
int disks_capacities[16];       // disk images' capacities (0 not used. It's the FDD)
int disks_found;                // total number of disk images found
File disks_images[16];          // disks images (0 not used. It's the FDD)
                                // Is it OK to do it like this,
                                //   or will risk to lose data if power failure?
                                // Would it be better to open() and close() for
                                //   each disk operation?

/*****************************************************************************/
/* Initial set up of the SD Card Module                                      */
/*****************************************************************************/
void sd_setup(){
    pinMode(SD_BUSY_LED, OUTPUT);
    digitalWrite(SD_BUSY_LED, LED_OFF);

    lastSDcmd_ok  = true;
    sd_isbusy   = false;

    // Detect SD Card
    digitalWrite(SD_BUSY_LED, LED_ON);
    if(sdcard_ok = SD.begin(SD_CS)){
        if(DEBUG){ Serial.println("    SD card found."); }
        sd_read_diskscfg();
    }
    digitalWrite(SD_BUSY_LED, LED_OFF);

    if(DEBUG && !sdcard_ok){ Serial.println("    SD card NOT found."); }
    if(DEBUG && !sdimg_ok){ Serial.println("    Image NOT found."); }
}

/*****************************************************************************/
/* Read the _disks.cfg file, which contains the list of Image Disk Files     */
/* The list is stored in array starting with disks_found + 1, beacuse disk 0 */
/*   is always the FDD                                                       */
/*****************************************************************************/
void sd_read_diskscfg(){
    File diskslist;
    String diskname;

    if(sdimg_ok = SD.exists("_disks.cfg")){
        if(DEBUG){ Serial.println("    Reading _disks.cfg"); }
        diskslist = SD.open("_disks.cfg", O_RDONLY);
        if(diskslist){
            while(diskslist.available()){
                diskname = diskslist.readStringUntil('\n');
                if(diskname.startsWith("#") == false){  // Test for commented lines
                    if(DEBUG){ Serial.print("       Found: "); }
                    diskname.remove(diskname.length() - 1);

                    if(SD.exists(diskname)){
                        disks_images[disks_found + 1] = SD.open(diskname, O_RDWR);
                        disks_names[disks_found+ 1] = diskname;
                        Serial.print(disks_names[disks_found+ 1]);
                        disks_capacities[disks_found+ 1] = (disks_images[disks_found + 1].size() / 1024) / 1024;
                        if(DEBUG){ Serial.print(" - Size: "); Serial.print(disks_capacities[disks_found + 1]); Serial.println(" MB"); }
                        disks_found++;
                    }else{
                        sdimg_ok = false;
                        if(DEBUG){ Serial.println("            ERROR: Couldn't find the file."); }
                    }
                }
            }
        }else{
            if(DEBUG){ Serial.println("    ERROR: Cannot open _disks.cfg"); }
        }
    }else{
        if(DEBUG){ Serial.println("    ERROR: _disks.cfg not found"); }
    }
    
    diskslist.close();
}

/*****************************************************************************/
/* Sends a byte that tells the status of the SD Card reader                  */
/*      Low Nibble (0x00 if all OK)                                          */
/*          bit 0 = set if SD card was not found                             */
/*          bit 1 = set if image file was not found                          */
/*          bit 2 = set if last command resulted in error                    */
/*          bit 3 = not used                                                 */
/*      High Nibble (number of disk image files found)
/*****************************************************************************/
void sd_cmd_status(){
    uint8_t status = 0;

    // Put in High Nibble the number of image files found
    status = disks_found << 4;

    if(!sdcard_ok){     // SD card was found
        status = status | 0x01;
    }

    if(!sdimg_ok){      // Image file was found
        status = status | 0x02;
    }

    if(!lastSDcmd_ok){    // Last command was not successful
        status = status | 0x04;
    }

    if(DEBUG){ Serial.print("SD Status: "); Serial.println(status); }
    Serial1.write(status);
}

/*****************************************************************************/
/* Sends the image name and capacity of a specified image number             */
/*      Image name is 12 characters (padded with spaces)                     */
/*      Image capacity is 1 byte representing MB                             */
/*****************************************************************************/
void sd_cmd_get_img_info(byte *rcvd_bytes){
    uint8_t img_num;
    int c, p;

    img_num = rcvd_bytes[1];

    if(DEBUG){
        Serial.print("Info for image: "); Serial.print(img_num);
        Serial.print(" -> "); Serial.print(disks_names[img_num]);
        Serial.print(" ("); Serial.print(disks_capacities[img_num]);
        Serial.println(" MB)");
    }

    // Send characters
    for(c=0; c<disks_names[img_num].length(); c++){
        Serial1.write(disks_names[img_num].charAt(c));
    }
    // And now add padding
    for(p=c; p<12; p++){
        Serial1.write(0x20);
    }

    // Send capacity
    Serial1.write(disks_capacities[img_num]);
}

/*****************************************************************************/
/* Closes the image file                                                     */
/*****************************************************************************/
void sd_cmd_close_img(byte *rcvd_bytes){
    uint8_t img_num;

    img_num = rcvd_bytes[1];

    if(DEBUG){ Serial.print("Closing Image file: "); Serial.println(disks_names[img_num]); }

    disks_images[img_num].close();

    if(DEBUG){ Serial.println("   Closed"); }
}

/*****************************************************************************/
/* Opens the image file                                                      */
/*****************************************************************************/
void sd_cmd_open_img(byte *rcvd_bytes){
    uint8_t img_num;

    img_num = rcvd_bytes[1];

    if(DEBUG){ Serial.print("Opening Image file: "); Serial.println(disks_names[img_num]); }

    disks_images[img_num] = SD.open(disks_names[img_num], O_RDWR);
    disks_images[img_num].seek(0);

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
    uint8_t img_num, sector_num_msb, sector_num_lsb;
    uint32_t sector_num;

    img_num         = rcvd_bytes[1];
    sector_num_lsb  = rcvd_bytes[2];
    sector_num_msb  = rcvd_bytes[3];

    sector_num = 0;
    sector_num = ((0 << 24) + (0 << 16) + (sector_num_msb << 8) + sector_num_lsb);
    sector_num *= SD_SECSIZE;

    if(DEBUG){
        Serial.print("Reading Sector Offset: ");
        Serial.print(sector_num);
        Serial.print(" (0x");
        Serial.print(sector_num, HEX);
        Serial.print(") of image: ");
        Serial.println(img_num);
    }

    // Read Sector
    sd_isbusy = true;
    digitalWrite(SD_BUSY_LED, LED_ON);
    disks_images[img_num].seek(sector_num);
    disks_images[img_num].read(sector, SD_SECSIZE);

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
    uint8_t img_num, sector_num_msb, sector_num_lsb;
    uint8_t buffer[SD_SECSIZE];
    uint32_t sector_num;
    size_t written_bytes;
    int b;

    img_num         = rcvd_bytes[1];
    sector_num_lsb  = rcvd_bytes[2];
    sector_num_msb  = rcvd_bytes[3];

    for(b=0; b<SD_SECSIZE; b++){
        buffer[b] = rcvd_bytes[b + 4]; // Discard first 4 bytes (command, img number, sector number (lsb & msb)
    }

    sector_num = 0;
    sector_num = ((0 << 24) + (0 << 16) + (sector_num_msb << 8) + sector_num_lsb);
    sector_num *= SD_SECSIZE;

    if(DEBUG){
        Serial.print("Writing in Sector Offset: ");
        Serial.print(sector_num);
        Serial.print(" (0x");
        Serial.print(sector_num, HEX);
        Serial.print(") of image: ");
        Serial.println(img_num);
    }

    // Write Sector
    sd_isbusy = true;
    digitalWrite(SD_BUSY_LED, LED_ON);

    disks_images[img_num].seek(sector_num);
    written_bytes = disks_images[img_num].write(buffer, SD_SECSIZE);
    disks_images[img_num].flush();

    if(DEBUG){ Serial.print("    Written: "); Serial.print(written_bytes); Serial.println(" bytes"); }

    sd_isbusy = false;
    digitalWrite(SD_BUSY_LED, LED_OFF);

    // if(DEBUG){ Serial.print("Returning: "); Serial.println( b + 4); }
    return SD_SECSIZE + 4;
}
