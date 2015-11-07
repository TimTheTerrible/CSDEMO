# CSDEMO
An Arduino-based toy version of the Counter Strike demolition charge

## Overview
This project grew out of a desire to make a Counter Strike-style
game mode for real-world Arisoft play. It's basically a mockup
of the demolition charge from Counter Strike.

## How it Works
The device is based around the Teensy development platform.
It uses the Teensy Audio library to play the sounds from an
SD card on the shield.

## How to Use It
As built for this code, the device consists of a small box with
a 16x2 LCD display, an old-style telephone keypad, a key switch,
a Hall-effect sensor, and a speaker.

## Safe Mode
To play, turn on the device with the key switch. It will play an
introductory sound, enter Safe Mode and wait to be armed.

## Arming Mode
Press the # key to enter Arming Mode. Enter a seven-digit number
and press # again. The device is now armed, and begins counting
down from five minutes after playing an appropriate sound.

## Disarming Mode
To attempt to disarm the device, press #. If there are more than
thirty seconds left on the timer, the timer is reduced to thrity
seconds and continues counting down. If there are less, the timer
is untouched and continues to count down.

Enter the same disarming code entered above, then press # again
to disarm the device. If the correct number was entered, the
device will disarm after playing an appropriate sound. If an
incorrect number was entered, the device returns to Armed Mode
and the timer continues counting down.

## Time's Up
If the timer expires while the device is in Armed Mode or
Disarming Mode, the device enters Detonated Mode and plays an
appropriate sound.

If the correct disarm code was entered in time, the device enters
Disarmed Mode and plays an appropriate sound.

## Resetting the device
Once in either Disarmed Mode ot Detonated Mode, the device waits
for the user to press #, whereupon it will play an appropriate
sound and return to Safe Mode.

## Defuse Kit, and figuring out the defuse code.
The device supports a "defuse kit" play mode. The defuse kit is
simply a small fob containing a magnet. The fob is placed against
the device in the apporpriate place before entering Disarming Mode,
and the Hall-effect sensor detects the magent.

If the defuse kit is present, the disarm code is displayed clearly.
If the defuse kit is not present, the disarm code is displayed by
sequentially flashing the digits of the code from right to left.

## Scoreboard
While the device is in Safe Mode, pressing * switches to Scoreboard
Mode, and the current tally of wins and losses is displayed. Turing
the device off and back on resets the tally to zero each.
