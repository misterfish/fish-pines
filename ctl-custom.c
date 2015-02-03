/* This is where callbacks for combinations are defined. Most custom stuff
 * should go here.
 */

#include <fish-util.h>
#include "mode.h"
#include "mpd.h"

#include "ctl-custom.h"

// XX
static char *SOCKET_VOLD = "/tmp/.vold-simple-socket";

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
    if (mode_music()) 
        if (!socket_unix_message(SOCKET_VOLD, "up")) 
            pieprf;
    return true;
}

bool ctl_custom_down() {
    if (mode_music()) 
        if (!socket_unix_message(SOCKET_VOLD, "down")) 
            pieprf;
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
