/******************************************************************************
 * ASMDC_defs.h
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
 * This code was written by David Asta
  * 
 * Version 1.0.0
 * Created on 30 Oct 2022
 * Last Modification 27 Nov 2023
 *******************************************************************************
 * CHANGELOG
 *   - 27 Nov 2023 - Removed all code for FDD and SD card
 *******************************************************************************
 */

/* ---------------------------LICENSE NOTICE--------------------------------
 *  MIT License
 *  
 *  Copyright (c) 2022-2023 David Asta
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

#define DEBUG               true        // set to true to get some debugging
                                        // values on the Arduino Serial Monitor

#define SERIAL_DBPS         115200      // This is both the speed for the 
                                        // Arduino Serial Monitor and the
                                        // speed of the device connecting
#define SERIAL_BUFFER       520         // Size of the receiving buffer

// Commands
#define RTC_CMD_GET_INFO    0xC0
#define RTC_CMD_GET_BATT    0xC1
#define RTC_CMD_GET_DATE    0xC2
#define RTC_CMD_GET_TIME    0xC3
#define RTC_CMD_SET_DATE    0xC4
#define RTC_CMD_SET_TIME    0xC5
#define RTC_CMD_GET_TEMP    0xC6

#define NVRAM_CMD_TEST      0xD0
#define NVRAM_CLEAR         0xD1

// Error codes
#define SD_ERR_NOSD         0x01
#define SD_ERR_NOIMG        0x02
