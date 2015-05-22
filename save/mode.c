#include <fish-util.h>

#include "led.h"
#include "mode.h"

#define MODE_MUSIC      0x00
#define MODE_GENERAL    0x01

static bool update_mode_led();

int Mode = -1;

bool mode_music() {
    return Mode == MODE_MUSIC;
}

bool mode_general() {
    return Mode == MODE_GENERAL;
}

bool mode_set_music() {
    Mode = MODE_MUSIC;
    if (!update_mode_led()) 
        pieprf;
    return true;
}

bool mode_set_general() {
    Mode = MODE_GENERAL;
    if (!update_mode_led()) 
        pieprf;
    return true;
}

bool mode_toggle() {
    if (mode_music()) 
        return mode_set_general();
    else if (mode_general()) 
        return mode_set_music();
    else  
        pieprf;
}

static bool update_mode_led() {
#ifndef NO_NES
    if (!led_update_mode()) {
        warn("Couldn't update led for mode");
        return false;
    }
#endif
    return true;
}
