#include <fish-util.h>

#include "global.h"
#include "const.h"
#include "nes.h"

bool nes_init_wiring() {
    if (wiringPiSetup() == -1) {
        warn("Unable to call wiringPiSetup()");
        pieprf;
    }
    return true;
}

int nes_setup() {
    int joystick;
    if ((joystick = setupNesJoystick(DPIN, CPIN, LPIN)) == -1) {
        warn("Unable to set up joystick");
        pieprneg1;
    }

    return joystick;
}

int nes_read(int joystick) {
    return readNesJoystick(joystick);
}
