/*******************************************************************************
 * RTC_DS3231.ino
 * ______           _      _____ _                  _____ _            _
 * | ___ \         | |    |_   _(_)                /  __ \ |          | |
 * | |_/ /___  __ _| |______| |  _ _ __ ___   ___  | /  \/ | ___   ___| | __
 * |    // _ \/ _` | |______| | | | '_ ` _ \ / _ \ | |   | |/ _ \ / __| |/ /
 * | |\ \  __/ (_| | |      | | | | | | | | |  __/ | \__/\ | (_) | (__|   <
 * \_| \_\___|\__,_|_|      \_/ |_|_| |_| |_|\___|  \____/_|\___/ \___|_|\_\
 *                   _   ___      _______            __  __ 
 *                  | \ | \ \    / /  __ \     /\   |  \/  |
 *                  |  \| |\ \  / /| |__) |   /  \  | \  / |
 *                  | . ` | \ \/ / |  _  /   / /\ \ | |\/| |
 *                  | |\  |  \  /  | | \ \  / ____ \| |  | |
 *                  |_| \_|   \/   |_|  \_\/_/    \_\_|  |_|
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

#include <I2C_RTC.h>
#include <Wire.h>
#include "ASMDC_defs.h"

static DS3231 rtc;
static NVRAM nvram;

/*****************************************************************************
 * ______           _      _____ _                  _____ _            _
 * | ___ \         | |    |_   _(_)                /  __ \ |          | |
 * | |_/ /___  __ _| |______| |  _ _ __ ___   ___  | /  \/ | ___   ___| | __
 * |    // _ \/ _` | |______| | | | '_ ` _ \ / _ \ | |   | |/ _ \ / __| |/ /
 * | |\ \  __/ (_| | |      | | | | | | | | |  __/ | \__/\ | (_) | (__|   <
 * \_| \_\___|\__,_|_|      \_/ |_|_| |_| |_|\___|  \____/_|\___/ \___|_|\_\
 ****************************************************************************/

/*****************************************************************************/
/* Initial set up of the RTC Module (RTC)                                    */
/*****************************************************************************/
void rtc_setup(){
    rtc.begin();
    if(DEBUG){ Serial.println("    RTC initialised."); }
}

/*****************************************************************************/
/* Reads the DS3231 configuration                                            */
/* Returns 1 byte:                                                           */
/*                bit 0 = Set if clock is running (OSF)                      */
/*                bit 1 = Set if clock is in 24 hours mode                   */
/*                        Otherwise, clock is in 12 hours mode               */
/*                bit 2 = Set if alarm 1 is ON                               */
/*                bit 3 = Set if alarm 2 is ON                               */
/*****************************************************************************/
void rtc_get_info(){
    uint8_t info = 0;

    if(rtc.isRunning()) bitSet(info, 0);

    if(rtc.getHourMode() == CLOCK_H24) bitSet(info, 1);

    if(rtc.isAlarm1Tiggered()) bitSet(info, 2);

    if(rtc.isAlarm2Tiggered()) bitSet(info, 3);

    Serial1.write(info);

    if(DEBUG){ Serial.print("C0 Info: "); Serial.println(info); }
}

/*****************************************************************************/
/* Reads the DS3231 Temperature Sensor                                       */
/* Returns 1 byte:                                                           */
/*               bits 0-6 = Temperature in Celsius                           */
/*               bit 7 = Sign (1 for below zero)                             */
/*****************************************************************************/
void rtc_get_temp(){
    int temp = rtc.getTemp();

    if(temp < 0) bitSet(temp, 7);
    else bitClear(temp, 7);

    Serial1.write(temp);

    if(DEBUG){ Serial.print("C6 Temperature: "); Serial.println(temp); }
}

/*****************************************************************************/
/* Reads the DS3231 Oscillator Stop Flag (OSF)                               */
/* If the oscillator OSF is 1, it means the clock is not running. Usually    */
/* because the battery is dead and needs replacement                         */
/* Returns 1 byte: 0x0A = Healthy / 0x00 = Dead                              */
/*****************************************************************************/
void rtc_get_batt(){
    if(rtc.isRunning()){
        Serial1.write(0xA0);    // Battery seems to be OK
        if(DEBUG) Serial.println("Battery is OK");
    }else{
        Serial1.write(0x00);    // Battery seems to be dead
        if(DEBUG) Serial.println("Battery is DEAD");
    }
}

/*****************************************************************************/
/* Read the date from the Real-Time Clock and sends it via serial (9 bytes)  */
/* The bytes sent are CCYY MMDDW (in Hexadecimal)                            */
/*      where CC = century (hardcoded to 20), YY = year, MM = month,         */
/*            DD = day, W = day of the week (Sunday = 1)                     */
/*****************************************************************************/
void rtc_get_date(){
    uint8_t month, day, dow;
    uint16_t year;
    bool century;

    year    = rtc.getYear();
    month   = rtc.getMonth();
    day     = rtc.getDay();
    dow     = rtc.getWeek();

    if(DEBUG){
        Serial.print("Current date is: ");
        Serial.print(day);
        Serial.print('/');
        Serial.print(month);
        Serial.print('/');
        Serial.print(year);
        Serial.print(" DoW: ");
        Serial.println(dow);
    }

    Serial1.write(year / 100);      // Century
    Serial1.write(year - 2000);     // Year
    Serial1.write(month);
    Serial1.write(day);
    Serial1.write(dow);
}

/*****************************************************************************/
/* Read the time from the Real-Time Clock and sends it via serial (6 bytes)  */
/* The bytes sent are HHMMSS (in Hexadecimal)                                */
/*      where HH = hours, MM = minutes, SS = seconds,                        */
/*****************************************************************************/
void rtc_get_time(){
    uint8_t hours, minutes, seconds;

    hours   = rtc.getHours();
    minutes = rtc.getMinutes();
    seconds = rtc.getSeconds();

    if(DEBUG){
        Serial.print("Current time is: ");
        Serial.print(hours);
        Serial.print(':');
        Serial.print(minutes);
        Serial.print(':');
        Serial.println(seconds);
    }

    Serial1.write(hours);
    Serial1.write(minutes);
    Serial1.write(seconds);
}

/*****************************************************************************/
/* Changes the date stored in the RTC, by receiving (6 bytes) the new time   */
/* via the serial                                                            */
/* The received bytes (in Hexadecimal) are YYMMDDW                           */
/*      where YY = year, MM = month, DD = day,                               */
/*             W = day of the week (Monday = 1)                              */
/*****************************************************************************/
void rtc_set_date(byte *rcvd_bytes){
    uint8_t in_year, in_month, in_day, in_dow;

    in_year     = rcvd_bytes[1];
    in_month    = rcvd_bytes[2];
    in_day      = rcvd_bytes[3];
    in_dow      = rcvd_bytes[4];

    if(DEBUG){
        Serial.print("Rcvd: ");
        Serial.print(in_day, DEC);
        Serial.print('/');
        Serial.print(in_month, DEC);
        Serial.print('/');
        Serial.print(in_year, DEC);
        Serial.print(" DoW: ");
        Serial.println(in_dow, DEC);
    }

    rtc.setDate(in_day, in_month, in_year);
    rtc.setWeek(in_dow);
}

/*****************************************************************************/
/* Changes the time stored in the RTC, by receiving (6 bytes) the new time   */
/* via the serial                                                            */
/* The received bytes (in Hexadecimal) are HHMMSS                            */
/*      where HH = hours, MM = minutes, SS = seconds                         */
/*****************************************************************************/
void rtc_set_time(byte *rcvd_bytes){
    uint8_t in_hours, in_minutes, in_seconds;

    in_hours    = rcvd_bytes[1];
    in_minutes  = rcvd_bytes[2];
    in_seconds  = rcvd_bytes[3];

    if(DEBUG){
        Serial.print("Rcvd: ");
        Serial.print(in_hours);
        Serial.print(':');
        Serial.print(in_minutes);
        Serial.print(':');
        Serial.print(in_seconds);
    }

    rtc.setTime(in_hours, in_minutes, in_seconds);
}

/*****************************************************************************
 *                   _   ___      _______            __  __ 
 *                  | \ | \ \    / /  __ \     /\   |  \/  |
 *                  |  \| |\ \  / /| |__) |   /  \  | \  / |
 *                  | . ` | \ \/ / |  _  /   / /\ \ | |\/| |
 *                  | |\  |  \  /  | | \ \  / ____ \| |  | |
 *                  |_| \_|   \/   |_|  \_\/_/    \_\_|  |_|
 ****************************************************************************/

/*****************************************************************************/
/* Initial set up of the RTC Module (NVRAM)                                  */
/*****************************************************************************/
void nvram_setup(){
    nvram.begin();
    if(DEBUG){ Serial.println("    NVRAM initialised."); }
}

/*****************************************************************************/
/* Tests if the NVRAM is present, by trying to write a value to address 0    */
/* Sends the length (in bytes) of the NVRAM                                  */
/*       or 0xFF if not detected                                             */
/*****************************************************************************/
void nvram_test(){
    uint8_t nvram_data;

    // Try to write a value into address 0
    nvram.write(0, 0xFF);
    // And read it back
    nvram_data = nvram.read(0);

    if(nvram_data == 0xFF){
        // Success
        Serial1.write(nvram.length);
        if(DEBUG){ Serial.print("NVRAM length: "); Serial.println(nvram.length); }
    }else{
        // Failure
        Serial1.write(0xFF);
        if(DEBUG){ Serial.println("NVRAM not responding."); }
    }

    
}

/*****************************************************************************/
/* Sets all addresses to zero                                                */
/*****************************************************************************/
void nvram_clear(){
    if(DEBUG){ Serial.println("Clearing NVRAM..."); }

    for(uint8_t address = 0; address<nvram.length; address++){
        nvram.write(address, 0x00);
    }

    if(DEBUG){ Serial.println("   Cleared"); }
}