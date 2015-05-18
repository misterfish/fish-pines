#ifndef __INCL_CONST_H
#define __INCL_CONST_H

/* This is the mapping for DPIN, CPIN, and LPIN to the Broadcom (revision 2) pin
 * numbers. The values come from pinToGpioR2 in wiringPi/wiringPi.c.

  0 -> 17
  1 -> 18
  2 -> 27
  3 -> 22
  4 -> 23
  5 -> 24
  6 -> 25
  7 -> 4
  8 -> 2
  9 -> 3
  10 -> 8
  11 -> 7
  12 -> 10
  13 -> 9
  14 -> 11
  15 -> 14
  16 -> 15
  17 -> 28
  18 -> 29
  19 -> 30
  20 -> 31

This is the mapping from physical pins to BCM pins using my physical counting:

     1   2 [+5]
   2 3   4 [+5]
   3 5   6 [GND]
   4 7   8 14
     9  10 15
  17 11 12 18
  27 13 14 -1
  22 15 16 23
     17 18 24
  10 19 20
   9 21 22 25
  11 23 24 8
     25 26 7

*/

/* From BCM numbering to wiringPi numbering.
 * (Reverse of pinToGpioR2)
 */
inline short BCM2WIRINGPI(short b) {
    if (b == 17)
        return 0;
    else if (b == 18)
        return 1;
    else if (b == 27)
        return 2;
    else if (b == 22)
        return 3;
    else if (b == 23)
        return 4;
    else if (b == 24)
        return 5;
    else if (b == 25)
        return 6;
    else if (b == 4)
        return 7;
    else if (b == 2)
        return 8;
    else if (b == 3)
        return 9;
    else if (b == 8)
        return 10;
    else if (b == 7)
        return 11;
    else if (b == 10)
        return 12;
    else if (b == 9)
        return 13;
    else if (b == 11)
        return 14;
    else if (b == 14)
        return 15;
    else if (b == 15)
        return 16;
    else if (b == 28)
        return 17;
    else if (b == 29)
        return 18;
    else if (b == 30)
        return 19;
    else if (b == 31)
        return 20;
    else
        return 0;
}

/* From wiringPi.
 */
#define	N_RIGHT	        0x01
#define	N_LEFT	        0x02
#define	N_DOWN	        0x04
#define	N_UP		0x08
#define	N_START	        0x10
#define	N_SELECT	0x20
#define	N_B		0x40
#define	N_A		0x80

/* Our std order.
 */

#define F_LEFT      0
#define F_RIGHT     1
#define F_UP        2
#define F_DOWN      3
#define F_SELECT    4
#define F_START     5
#define F_B         6
#define F_A         7

#ifdef NO_NES
#define POLL_TENTHS_OF_A_SECOND 1
#else
#define POLL_MS     40
#endif

#define LED_ORANGE 12
#define LED_BLUE 10

#define LED_MODE LED_ORANGE
#define LED_RANDOM LED_BLUE
#define LED_REMAKE_PLAYLIST LED_BLUE

//#define SHUTDOWN_HOLD_SECS 2
#define SHUTDOWN_HOLD_SECS 0

/* Switch down and up keys for volume control.
 * Useful for bugging the bartender.
 */
#define ANTON_MODE true

#define LEDD_SOCK           "/tmp/.ledd-socket"
#define VOLD_SOCK           "/tmp/.vold-simple-socket"
#define FISH_VOL_SOCK       "%s/.local/share/fish-vol/socket"

/* Mapping from our standard order to their N_ order.
 */
inline short BUTTONS(short b) {
    if (b == 0) 
        return N_LEFT;
    else if (b == 1) 
        return N_RIGHT;
    else if (b == 2) 
        return N_UP;
    else if (b == 3) 
        return N_DOWN;
    else if (b == 4) 
        return N_SELECT;
    else if (b == 5) 
        return N_START;
    else if (b == 6) 
        return N_B;
    else if (b == 7) 
        return N_A;
    else 
        return 0;
}

#endif


