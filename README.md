# Arduino Serial Multi-Device Controller for dastaZ80's dzOS

1. [Why this project?](#why-this-project)
1. [How it works?](#how-it-works)
1. [Advantages/Disadvantages](#advantagesdisadvantages)
1. [Commands](#commands)
1. [Tools](#tools)

---

An Arduino board acting as a _man-in-the-middle_ to allow my homebrew computer
[dastaZ80](https://github.com/dasta400/dzOS) to communicate with different
devices like Real-Time Clock, NVRAM, SD card, Floppy Disc Drive, and more.

Technically it is possible (but not tested) to use this project with any other
kind of computer or microcontroller that has a serial communication port.

---

## Why this project?

As I wanted to add some more peripherals to my homebrew computer
[dastaZ80](https://github.com/dasta400/dzOS) (a Real-Time Clock (RTC), an SD
card, an NVRAM and a Floppy Disc Drive) I made some calculations of the cost.
Between components and PCBs, I came up with the  total cost of around 210 Euros.
As I've already spent a couple of hundreds so far, this hobby is starting to be
too expensive for my pocket.

On the other hand, I have already several Arduino boards (Uno, Mega2560,
Leonardo) and several devices I got on a developer's kit, which happened to
contain a DS3231 RTC (with NVRAM included) and a MicroSD Card Adapter.

So I thought, why not make an Arduino to communicate with the RTC and SD card
modules, plus an FDD if possible, and then make the Arduino to communicate with
the dastaZ80 like if it was an external controller?

As all Arduinos include serial ports (TX / RX pins) and I have a spare serial port
on the dastaZ80 (SIO/2 Channel B), it should be relatively easy to make them to
talk to each other.

---

## How it works?

The dastaZ80's SIO/2 Channel B is connected to an Arduino Mega2560's serial port.
The dastaZ80 sends commands (identified by a unique single byte) to the Arduino,
which interprets the received command and communicates with the corresponding
attached device (e.g. get time from the RTC), and send information back to the
dastaZ80.

---

## Advantages/Disadvantages

* **Advantages**:
  * Cost next to zero, as I already have all the necessary hardware.
  * The SD card is formatted with FAT16, so it's readable by modern PCs.
  * The SD card holds disk images, which makes easy to swap between different
  images, or to have multiple disks.
  * Expandable. For example to add a PseudoRandom Number Generator (PRNG).
* **Disadvanges**:
  * Slower disk (SD card) access? I haven't benchmarked it.
  * If I would like to run DRI CP/M, I would need to modify CP/M's code.

---

## Commands

The Arduino responds to commands received that are identified by an unique
single byte:

| Command | Description                        | Input Size | Input  | Output Size | Output      |
| ------- | ---------------------------------- | ---------- | ------ | ----------- | ----------- |
| A0      | FDD status                         | 0          | -      | 1           | _(see below)_ |
| A1      | FDD      Busy                      | 0          | -      | 1           | 0x00=Not busy                                                           0x01=Busy |
| A2      | Read sector from Floppy Disk       | 2          | sector_num_lsb sector_num_msb | 512 | Sector contents |
| A3      | Write sector into Floppy Disk      | 512        | Sector contents | 0 | - |
| A4      | Checks if a disk is in the drive   | 0          | -      | 1           | 0x00=Disk / 0xFF=NoDisk |
| A5      | Checks if disk is Write Protected  | 0          | -      | 1           | 0x00=Protect / 0xFF=Unprotected |
| A6      | Set drive as Double-density        | 0          | -      | 1           | Return code (0=success) |
| A7      | Set drive as High-density          | 0          | -      | 0           | - |
| A8      | Low-level format (no file system)  | 0          | -      | 1           | Return code (0=success) |
| AA      | Turns the FDD motor ON            | 0          | -      | 0           | - |
| AB      | Turns the FDD motor OFF           | 0          | -      | 0           | - |
| B0      | SD Card status                     | 0          | -      | 1           | _(see below)_ |
| B1      | SD Card Busy                       | 0          | -      | 1           |0x00=Not busy 0x01=Busy |
| B2      | Read sector from Image in SD Card  | 2          | sector_num_lsb sector_num_msb | 512 | Sector contents |
| B3      | Write sector into Image in SD Card | 512        | Sector contents | 0 | - |
| B4      | Close Image File                   | 0          | -      | -           | - |
| B5      | Open Image File                    | 0          | -      | -           | - |
| C0      | Get RTC info                       | 0          | -      | 1           | _(see below)_ |
| C1      | Check Battery Health               | 0          | -      | 1           | 0xA0=Healthy 0x00=Dead |
| C2      | Get current Date (in Hexadecimal)  | 0          | -      | 9           | CCYYMMDDW   |
| C3      | Get current Time (in Hexadecimal)  | 0          | -      | 6           | HHMMSS      |
| C4      | Set Date                           | 6          | YYMMDD | 0           | -           |
| C5      | Set Time                           | 6          | HHMMSS | 0           | -           |
| C6      | Get RTC Temperature                | 0          | -      | 1           | bits0-6=Celsius bit7=sign(1=negative)
| D0      | Test  NVRAM can be written         | 0          | -      | 1           | NVRAM capacity (in bytes) or 0xFF if failure |
| D1      | Clear (Set all to zeros) NVRAM     | 0          | -      | 0           | - |

### A0 (FDD status)

Tells the status of the FDD.

This command should be called after each command on the FDD to check if the
command was successful or not. Any value other than 0x00 indicates and error.

* **Lower Nibble** (0x00 if all OK)
  * **bit 0** = not used
  * **bit 1** = not used
  * **bit 2** = set if last command resulted in error
  * **bit 3** = not used
* **Upper Nibble** (error code)

### B0 (SD Card status)

Tells the status of the SD Card reader.

This command should be called after each command on the SD card to check if the
command was successful or not. Any value other than 0x00 indicates and error.

* **Lower Nibble** (0x00 if all OK)
  * **bit 0** = set if SD card was not found
  * **bit 1** = set if image file was not found
  * **bit 2** = set if last command resulted in error
  * **bit 3** = not used
* **Upper Nibble** (number of disk image files found)

### C5 (Get RTC info)

Tells information of the configuration of the RTC module.

Useful, for example if the time is not kept accurately, to check if the clock is
running or not.

* **Byte 1**
  * **bit 0** = set if clock is running.
  * **bit 1** = set if clock is in 24 hours mode. Otherwise, clock is in 12 hours mode
  * **bit 2** = set if alarm 1 is ON
  * **bit 3** = set if alarm 2 is ON

---

## Tools

### imgmnr (Image Manager)

Allows to add, rename, delete and change attributes of files inside a DZFS image
file. Plus create new image file, display the image file's catalogue and display
the Superblock information.

This tool is for use with [dzOS](https://github.com/dasta400/dzOS). Otherwise,
it does not have any purpose.

It's written with [FreeBASIC](https://www.freebasic.net/). To compile use _fbc -static imgmngr.bas_

---

## Dependencies

* Real-Time Clock
  * [I2C_RTC.h](https://github.com/cvmanjoo/RTC)
  * [Wire.h](https://github.com/cvmanjoo/RTC)
* SD Card
  * [SPI.h](https://www.arduino.cc/reference/en/libraries/sd)
  * [SD.h](https://www.arduino.cc/reference/en/libraries/sd)
* Floppy Disk Drive
  * [ArduinoFDC.h](https://github.com/dhansel/ArduinoFDC)
