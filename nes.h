#include <wiringPi.h>
#include <piNes.h>

/* The wiringPi numbering system (for revision 2) is:
 *
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

  cf pinToGpioR2 in wiringPi/wiringPi.c
*/

/* Yellow.
 */
static int DPIN = 6;
/* Red.
 */
static int CPIN = 5;
/* Orange.
 */
static int LPIN = 4;

bool nes_init_wiring();
int nes_setup();
int nes_read(int joystick);

