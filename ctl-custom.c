/* This is where callbacks for combinations are defined. Most custom stuff
 * should go here.
 */

#include <fish-util.h>
#include "mode.h"
#include "mpd.h"
#include "conf.h"
#include "vol.h"

#include "ctl-custom.h"

bool ctl_custom_init() {
    return true;
}

bool ctl_custom_b_left() {
    if (mode_music()) 
        if (!f_mpd_seek(-5)) 
            pieprf;
    return true;
}

bool ctl_custom_b_right() {
    if (mode_music()) 
        if (!f_mpd_seek(5))
            pieprf;
    return true;
}

bool ctl_custom_b_up() {
    if (mode_music()) 
        if (!f_mpd_prev_playlist())
            pieprf;
    return true;
}

bool ctl_custom_b_down() {
    if (mode_music()) 
        if (!f_mpd_next_playlist())
            pieprf;
    return true;
}

bool ctl_custom_left() {
    if (mode_music()) 
        if (!f_mpd_prev()) 
            pieprf;
    return true;
}

bool ctl_custom_right() {
    if (mode_music()) 
        if (!f_mpd_next()) 
            pieprf;
    return true;
}

bool ctl_custom_up() {
    if (mode_music()) {
        if (ANTON_MODE) {
            if (! vol_down()) 
                pieprf;
        }
        else {
            if (! vol_up()) 
                pieprf;
        }
    }
    return true;
}


bool ctl_custom_down() {
    if (mode_music()) {
        if (ANTON_MODE) {
            if (! vol_up()) 
                pieprf;
        }
        else {
            if (! vol_down()) 
                pieprf;
        }
    }
    return true;
}

bool ctl_custom_a() {
    if (mode_music()) 
        if (!f_mpd_toggle_random()) 
            pieprf;
    return true;
}

bool ctl_custom_start() {
    if (mode_music()) 
        if (!f_mpd_toggle_play())
            pieprf;
    return true;
}

bool ctl_custom_select() {
    if (!mode_toggle()) 
        pieprf;
    return true;
}
