#define _GNU_SOURCE

/* This is for hooking callbacks when a button is pressed or released (e.g.
 * start), regardless of the combination. Note that this is not the same as
 * defining the combination N_START in ctl-custom.
 * You probably want to put your callbacks in ctl-custom.
 */

#include <string.h>

#include <fish-util.h>

#include "constants.h"
#include "conf.h"
#include "buttons.h"
#include "mpd.h"
#include "uinput.h"
#include "mode.h"

#ifndef NO_NES
# include "led.h"
#endif

#include "ctl-custom.h"
#include "ctl-default.h"

#include "fish-pines.h"

static struct {
    bool do_uinput;
} g;

bool ctl_default_init(bool do_uinput) {
#ifndef NO_NES
    if (!led_init()) 
        pieprf;
#endif
    g.do_uinput = do_uinput;

    if (g.do_uinput) 
        if (!uinput_init())
            pieprf;

    mode_set_music();

    if (!f_mpd_init()) 
        pieprf;

    return true;
}

bool ctl_do_down() {
    if (g.do_uinput) 
        return uinput_up();
    return true;
}

bool ctl_default_up() {
    if (g.do_uinput) 
        return uinput_down();
    return true;
}

bool ctl_default_left() {
    if (g.do_uinput) 
        return uinput_left();
    return true;
}

bool ctl_default_right() {
    if (g.do_uinput) 
        return uinput_right();
    return true;
}

bool ctl_default_down() {
    if (g.do_uinput) 
        return uinput_right();
    return true;
}

bool ctl_default_center_x() {
    if (g.do_uinput) 
        return uinput_center_x();
    return true;
}

bool ctl_default_center_y() {
    if (g.do_uinput) 
        return uinput_center_y();
    return true;
}

bool ctl_default_select_down() {
    if (g.do_uinput) 
        return uinput_btn_select();

    return true;
}

bool ctl_default_select_up() {
    return true;
}

bool ctl_default_start_down() {
    if (g.do_uinput) 
        return uinput_btn_start();

    return true;
}

bool ctl_default_start_up() {
    if (mode_general()) 
        ctl_custom_start_released();

    return true;
}

bool ctl_default_a_down() {
    if (g.do_uinput) 
        return uinput_btn_a();
    return true;
}

bool ctl_default_a_up() {
    return true;
}

bool ctl_default_b_down() {
    if (g.do_uinput) 
        return uinput_btn_b_down();

    return true;
}

bool ctl_default_b_up() {
    if (g.do_uinput) 
        return uinput_btn_b_up();
    return true;
}

