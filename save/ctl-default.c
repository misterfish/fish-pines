#define _GNU_SOURCE

/* This is for hooking callbacks when a button is pressed or released (e.g.
 * start), regardless of the combination. Note that this is not the same as
 * defining the combination N_START in the custom callbacks.
 *
 * First the _pre in this module is called, then the custom callback, then
 * the _post.
 */

#include <string.h>

/*
#include <fish-util.h>

#include "global.h"
#include "buttons.h"
#include "mpd.h"
#include "mode.h"

#include "ctl-custom.h"
*/

#include "ctl-default.h"

//#include "fish-pines.h"

bool ctl_default_init() {
    return true;
}

bool ctl_default_pre_up() {
    return true;
}

bool ctl_default_post_up() {
    return true;
}

bool ctl_default_pre_down() {
    return true;
}

bool ctl_default_post_down() {
    return true;
}

bool ctl_default_pre_left() {
    return true;
}

bool ctl_default_post_left() {
    return true;
}

bool ctl_default_pre_right() {
    return true;
}

bool ctl_default_post_right() {
    return true;
}

bool ctl_default_pre_center_x() {
    return true;
}

bool ctl_default_post_center_x() {
    return true;
}

bool ctl_default_pre_center_y() {
    return true;
}

bool ctl_default_post_center_y() {
    return true;
}

bool ctl_default_pre_select_down() {
    return true;
}

bool ctl_default_post_select_down() {
    return true;
}

bool ctl_default_pre_select_up() {
    return true;
}

bool ctl_default_post_select_up() {
    return true;
}

bool ctl_default_pre_start_down() {
    return true;
}

bool ctl_default_post_start_down() {
    return true;
}

bool ctl_default_pre_start_up() {
    /*
    if (mode_general()) 
        ctl_custom_start_released();
        */

    return true;
}

bool ctl_default_post_start_up() {
    return true;
}

bool ctl_default_pre_a_down() {
    return true;
}

bool ctl_default_post_a_down() {
    return true;
}

bool ctl_default_pre_a_up() {
    return true;
}

bool ctl_default_post_a_up() {
    return true;
}

bool ctl_default_pre_b_down() {
    return true;
}

bool ctl_default_post_b_down() {
    return true;
}

bool ctl_default_pre_b_up() {
    return true;
}

bool ctl_default_post_b_up() {
    return true;
}

