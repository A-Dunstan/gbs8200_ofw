This repository aims to produce replacement firmware for the MTV230 chip found on GBS8200/8220 YPbPr/RGBS/RGBHV scaler boards. CURRENTLY THIS REPLACEMENT FIRMWARE IS HIGHLY EXPERIMENTAL AND NOT FIT FOR GENERAL USE.

This firmware lives side-by-side with the original firmware; holding the UP button at power-on will set the original firmware as the default, while holding DOWN will set OFW as the default.

OFW is written to the unused part of the MTV230's flash memory. The original firmware only occupies a little bit more than half of the flash so there is plenty of free space.

An Arduino-compatible microprocessor with I2C is required to upload the firmware. This is done by the gbs8200_ofw.ino sketch which is currently targetted at an Arduino Nano. There are some additional "one-time" patches that need to be applied to the original firmware to allow the replacement firmware to run, these are intended to be "safe" in the sense that they do not erase any pages / modify any flash page that would prevent the MTV230 from booting directly into ICSP mode.


TL,DR:  
1- get an arduino nano  
2- connect pin A3 to the left-side pin of the P8 jumper  
3- connect GND to the right-side pin of the P8 jumper (or any other pin labelled GND)  
4- connect pin A4 to the "TX" pin  
5- connect pin A5 to the "RX" pin  
6- load the sketch and send "$" to the arduino which will make it write all the patches. Make sure it says "All patches done."  
7- restart the GBS2000, make sure the language is set to english then choose the "SWITCH FIRMWARE" option in the language menu.  
