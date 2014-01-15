## TokyoTime

An arduino based alarm-clock. But much more than that. It has a Two-Wire interface, serial interface, RTC, temperature sensor and a buzzer.

These features make it ideal for all projects where temperature and time are critical like:

  * Waking up in the morning,
  * Cooking,
  * Brewing,
  * Making yogurt,
  * etc.

In addition it can be powered from a CR2032 battery, or from a 5V wall-wart.

This project builds on the [BigTime](https://github.com/nseidle/BigTime) project by @nseidle, but the whole board was redesigned from scratch.

<img src="https://dl.dropbox.com/u/78009186/Photos/2013-01-19%2015.16.09.jpg">

## Assembly

Checkout the assembly guide [here](https://dl.dropbox.com/u/78009186/Documents/TokyoTime_2.1_assembly.pdf).

## Bootloader

The bootloader needed is the "Atmega328 on a breadboard" bootloader available
(with instructions) from the Arduino
[website](http://arduino.cc/en/Tutorial/ArduinoToBreadboard).

The bootloader add-on files for Arduino IDE are located in `firmware/hardware`.
To install them, simply copy the `firmware/hardware` to the local Arduino
folder.

The TokyoTime doesn't have an ISP header to upload the firmware, so another
board must be used to burn the bootloader to the chip. A standard Arduino board
can be used.

The fuse should be set automatically when burning the bootloader using Arduino IDE.
If not, the fuse should be set to use the internal 8MHz oscillator and disable
the brown-out threshold. The fuse can be burnt with avrdude using the following
command

    -U lfuse:w:0xe2:m -U hfuse:w:0xda:m -U efuse:w:0x07:m

## Firmware

The TokyoTime board features a handy serial header that can be used to upload
new code to the chip (provided it has a bootloader) with a 3.3V FTDI-to-USB converter
such as this [one](https://www.sparkfun.com/products/9873).

There are so far three different firmware for the board.

* **TokyoTime** `firmware/TokyoTime` -- A standard clock with settable alarm
  and temperature reading. 
* **Kuishimbo** `firmware/Kuishimbo` -- A controller using a PID loop to
  control the temperature of a toaster oven. There is a blog
  [post](http://robinscheibler.org/hacks/2012/08/21/fixing-an-electric-oven/)
  describing the whole project.  This firmware requires the Arduino PID library
  located in `firmware/libraries`. It can be copied to the local Arduino.
* **TokyoIncubator** `firmware/TokyoIncubator` -- A controller using a PID loop
  to control the temperature of an incubator for bacteria, yoghurt or natto
  culture. This was later spun into a
  [project](https://github.com/BioDesignRealWorld/Fermento) of its
  [own](http://biodesign.cc/2013/12/25/diy-incubator/). This firmware requires the
  Arduino PID library located in `firmware/libraries`. It can be copied to the
  local Arduino.

## Bill Of Material

All the parts can be bought at [Akizuki Denshi](http://www.akizukidenshi.com/)
in Akihabara, Tokyo. Equivalent parts can be found on
[Mouser](http://www.mouser.com), but please check the datasheets to make sure
the footprints correspond.

* 1x Green 7 segment (common cathode) [OSL40562-LG](http://akizukidenshi.com/catalog/g/gI-03945/)
* 1x 3.3V Regulator [TA48M033F](http://akizukidenshi.com/catalog/g/gI-00538/)
* 1x [Atmega328p](http://akizukidenshi.com/catalog/g/gI-03142/)
* 1x [28-pin socket]() (optional, allows to change chip easily)
* 1x [Buzzer](http://akizukidenshi.com/catalog/g/gP-04118/) (13mm)
* 1x [Tactil switch](http://akizukidenshi.com/catalog/g/gP-03651/)
* 1x Temperature sensor [LM61CIZ](http://akizukidenshi.com/catalog/g/gI-02726/)
* 1x [32.768 Crystal](http://akizukidenshi.com/catalog/g/gP-04005/)
* 1x [2.1mm barrel jack](http://akizukidenshi.com/catalog/g/gC-00077/)
* 1x [2032 coin cell holder](http://akizukidenshi.com/catalog/g/gP-00706/)
* 4x [0.1uF cap](http://akizukidenshi.com/catalog/g/gP-00090/) (C1, C2, C3, C5) (one 0.1uF cap is included with regulator from Akizuki)
* 1x 47uF cap (C4) (Also included with regulator from Akizuki, value may vary)
* 2x 10K resistors (R1, R3)
* 1K resistor (R2)
* 1x6 pin header
* 1x3 pin header
* 3x2 pin header

## License

(c) 2011-2014, Robin Scheibler

This work is realeased under the [CC-BY-SA 3.0](http://creativecommons.org/licenses/by-sa/3.0/us/) License.
