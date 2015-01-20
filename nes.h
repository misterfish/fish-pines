#include <wiringPi.h>
#include <piNes.h>

// yellow = GPIO 25  = #11
static int DPIN = 6;
// red = GPIO 24 = #9
static int CPIN = 5;
// orange = GPIO 23 = #8
static int LPIN = 4;

bool nes_init_wiring();
int nes_setup();
int nes_read(int joystick);

