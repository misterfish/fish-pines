#define _GNU_SOURCE

/* This is where callbacks for combinations are defined. Most custom stuff
 * should go here.
 * Should move to lua or something extensible.
 */

#include <unistd.h>
#include <string.h>

#include <fish-util.h>

#include "global.h"
#include "mode.h"
#include "mpd.h"
#include "vol.h"

#include "ctl-custom.h"

static bool shell_go(char *cmd);
static bool shell_cmd_with_cb(char *cmd, void*(cb)());
static bool shell_cmd(char *cmd);

static struct {
    double time_start_down;
    bool general_start_disabled;
    struct {
        char *shutdown;
        char *playlist_all;
        char *test;
        char *iwired;
        char *iwireless;
    } shell;
} g;

bool ctl_custom_init() {
    g.time_start_down = -1;
    g.shell.shutdown = "poweroff-with-led";
    //g.shell.shutdown = "echo SHUTDOWN, tee-hee";

    /* Playlist will be owned by root but world-readable.
     */
    g.shell.playlist_all = "make-playlist-all";
    g.shell.test = "echo >&2 shell test";
    g.shell.iwired = "switch-to-internet-wired";
    g.shell.iwireless = "switch-to-internet-wireless";
    return true;
}

bool ctl_custom_b_left() {
    if (mode_music()) 
        if (!f_mpd_seek(-5)) 
            pieprf;

    if (!shell_cmd(g.shell.test))
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

bool ctl_custom_b() {
    if (mode_general()) {
        if (!shell_cmd(g.shell.iwired))
            pieprf;
    }
    return true;
}

bool ctl_custom_b_a() {
    if (mode_music()) {
        if (!shell_cmd(g.shell.playlist_all))
            pieprf;
    }
    return true;
}

bool ctl_custom_left() {
    if (mode_music()) 
        if (!f_mpd_prev()) 
            pieprf;
    return true;
}

bool ctl_custom_right() {
    if (mode_music()) {
        if (!f_mpd_next()) 
            pieprf;
    }
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
    if (mode_music()) {
        if (!f_mpd_toggle_random()) 
            pieprf;
    }
    else if (mode_general()) {
        if (!shell_cmd(g.shell.iwireless))
            pieprf;
    }
    else 
        pieprf;
    return true;
}

bool ctl_custom_start() {
    if (mode_music()) {
        if (!f_mpd_toggle_play())
            pieprf;
    }

    /* Require a few seconds -- shutdown
     * Won't work unless kill_multiple is false.
     */
    else {
        if (g.general_start_disabled)
            return true;

        double now = f_time_hires();

        if (g.time_start_down == -1) {
            g.time_start_down = now;
        }
        else {
            if (now - g.time_start_down > SHUTDOWN_HOLD_SECS) {
                if (!shell_cmd(g.shell.shutdown))
                    pieprf;

                g.general_start_disabled = true;
            }
        }


    }

    return true;
}

bool ctl_custom_select() {
    if (!mode_toggle()) 
        pieprf;
    return true;
}

void ctl_custom_start_released() {
    g.time_start_down = -1;
}

static bool shell_go(char *cmd) {
    if (!cmd || !strcmp(cmd, "")) 
        pieprf;

    f_verbose_cmds(true);
    f_sys_die(false);

    pid_t pid = fork();
    if (pid == -1) {
        warn_perr("Can't fork");
        return false;
    }
    if (!pid) {
        if (sys(cmd)) {
            warn("Error running shell cmd.");
            _exit(1);
        }
        _exit(0);
    }
    return true;
}

static bool shell_cmd(char *cmd) {
    return shell_cmd_with_cb(cmd, NULL);
}

static bool shell_cmd_with_cb(char *cmd, void*(cb)()) {
    if (!shell_go(cmd)) 
        pieprf;

    bool ok = true;
    if (cb) ok = (*cb)();

    return ok;
}
