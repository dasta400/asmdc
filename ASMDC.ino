/******************************************************************************
 * ASMDC.ino
 *
 * Arduino Serial Multi-Device Controller (ASMDC)
 * for dastaZ80's dzOS
 * by David Asta (Oct 2022)
 * 
 * It reads/writes data from/to an SD card, and communicates with the dastaZ80
 * via serial port.
 * 
 * The dastaZ80's SIO/2 Channel B is connected to an Arduino. The dastaZ80
 * sends commands to the Arduino, via serial communication, for reading/writting
 * to/from an SD card attached to an SPI Card reader that is connected to the
 * Arduino.
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

#include "ASMDC_defs.h"

// Serial port
static byte rcvd_buffer[SERIAL_BUFFER];

/*****************************************************************************/
/* Start Serial1, and set up devices                                         */
/*****************************************************************************/
void setup() {
    if(DEBUG){
        Serial.begin(SERIAL_DBPS);
        Serial.println("Initialising ASMDC...");
    }

    Serial1.begin(SERIAL_DBPS);  // Serial port to dastaZ80
    Serial1.setTimeout(100);

    rtc_setup();        // Set up RTC Module (RTC)
    nvram_setup();      // Set up RTC Module (NVRAM)
    sd_setup();         // Set up SD Card Module
    fdd_setup();        // Set up FDD Module

    if(DEBUG){ Serial.println("ASMDC Initialised."); }
}

/*****************************************************************************/
/* Receives all bytes (max. = SERIAL_BUFFER) from Serial1                    */
/* Identifies the command (first byte), and calls corresponding function     */
/* with received bytes                                                       */
/*****************************************************************************/
void loop() {
    int rcvd_pos;
    size_t rcvd;

    memset(rcvd_buffer, 0, sizeof(rcvd_buffer));
    rcvd_pos = 0;

    while(Serial1.available() <= 0){}   // Wait until there is serial data

    rcvd = Serial1.readBytes(rcvd_buffer, SERIAL_BUFFER); // Receive the package of bytes

    if(DEBUG){
        Serial.print("Received: ");
        Serial.print(rcvd);
        Serial.println(" bytes");
        // for(int b=0; b<rcvd; b++){
        //     Serial.print(rcvd_buffer[b], HEX); Serial.print(" ");
        //     if(b % 16 == 0) Serial.println("");
        // }
    }

    while(rcvd_pos < SERIAL_BUFFER){
        if(DEBUG){
            Serial.print("Processing: ");
            Serial.println(rcvd_buffer[rcvd_pos], HEX);
        }

        switch(rcvd_buffer[rcvd_pos]){
        // SD card Controller
        case SD_CMD_GET_STATUS:     sd_cmd_status();                    rcvd_pos++;                 break;
        case SD_CMD_BUSY:           sd_cmd_busy();                      rcvd_pos++;                 break;
        case SD_CMD_READ_SEC:       sd_cmd_read_sector(rcvd_buffer);    rcvd_pos = SERIAL_BUFFER;   break;
        case SD_CMD_WRITE_SEC:      rcvd_pos = sd_cmd_write_sector(rcvd_buffer);                    break;
        case SD_CMD_CLOSE_IMG:      sd_cmd_close_img(rcvd_buffer);      rcvd_pos += 2;              break;
        case SD_CMD_OPEN_IMG:       sd_cmd_open_img(rcvd_buffer);       rcvd_pos += 2;              break;
        case SD_CMD_IMG_INFO:       sd_cmd_get_img_info(rcvd_buffer);   rcvd_pos += 2;              break;
        // FDD Controller
        case FDD_CMD_GET_STATUS:    fdd_cmd_status();                   rcvd_pos++;                 break;
        case FDD_CMD_BUSY:          fdd_cmd_busy();                     rcvd_pos++;                 break;
        case FDD_CMD_READ_SEC:      fdd_cmd_read_sector(rcvd_buffer);   rcvd_pos = SERIAL_BUFFER;   break;
        case FDD_CMD_WRITE_SEC:     rcvd_pos = fdd_cmd_write_sector(rcvd_buffer);                   break;
        case FDD_CMD_CHKDISKIN:     fdd_check_disk_in();                rcvd_pos++;                 break;
        case FDD_CMD_CHKWPROTECT:   fdd_check_wr_protect();             rcvd_pos++;                 break;
        case FDD_CMD_SETYPE_DD:     fdd_set_disk_type(false);           rcvd_pos++;                 break;
        case FDD_CMD_SETYPE_HD:     fdd_set_disk_type(true);            rcvd_pos++;                 break;
        case FDD_CMD_FORMAT:        fdd_lowlvl_format();                rcvd_pos++;                 break;
        case FDD_CMD_MOTOR_ON:      fdd_cmd_motor(true);                rcvd_pos++;                 break;
        case FDD_CMD_MOTOR_OFF:     fdd_cmd_motor(false);               rcvd_pos++;                 break;
        // case FDD_CMD_CHANGE_DISK:   fdd_select_drive();                 rcvd_pos++;                 break;
        // RTC Controller
        case RTC_CMD_GET_INFO:      rtc_get_info();                     rcvd_pos++;                 break;
        case RTC_CMD_GET_BATT:      rtc_get_batt();                     rcvd_pos++;                 break;
        case RTC_CMD_GET_DATE:      rtc_get_date();                     rcvd_pos++;                 break;
        case RTC_CMD_GET_TIME:      rtc_get_time();                     rcvd_pos++;                 break;
        case RTC_CMD_SET_DATE:      rtc_set_date(rcvd_buffer);          rcvd_pos += 5;              break;
        case RTC_CMD_SET_TIME:      rtc_set_time(rcvd_buffer);          rcvd_pos += 4;              break;
        // NVRAM Controller
        case NVRAM_CMD_TEST:        nvram_test();                       rcvd_pos++;                 break;
        case NVRAM_CLEAR:           nvram_clear();                      rcvd_pos++;                 break;
        default:
            if(DEBUG){ Serial.print("Unknown: "); Serial.println(rcvd_buffer[rcvd_pos], HEX); }
            rcvd_pos++;
            break;
        }

        if(rcvd_buffer[rcvd_pos] == 0) break;
    }
}
