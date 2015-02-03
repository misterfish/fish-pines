#define _GNU_SOURCE

/* This is for hooking callbacks when a button is pressed or released (e.g.
 * start), regardless of the combination. Note that this is not the same as
 * defining the combination N_START in ctl-custom.
 * You probably want to put your callbacks in ctl-custom.
 *
 * This is also where the release of start is watched for shutdown, since we
 * don't currently have a general way of defining callbacks for combination
 * release.
 */

#include <unistd.h>
#include <string.h>

#include <fish-util.h>

#include "constants.h"
#include "conf.h"
#include "buttons.h"
#include "mpd.h"
#include "uinput.h"
#include "mode.h"

#ifndef NO_NES
#include "led.h"
#endif

#include "ctl-default.h"

#include "fish-pines.h"

//#define SHUTDOWN_HOLD_SECS 2
#define SHUTDOWN_HOLD_SECS 0

#define myinfo(x, ...) do { \
    printf("\n"); \
    info(x, ##__VA_ARGS__); \
} while (0) ;

/*
bool shell_cmd_with_cb(int which, bool alt, void*(cb)());
bool alt = false;
*/

/*
static char *SHELL_CMDS_MUSIC[8][3] = {
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" }, 
    { "", "", "" },
    { "", "", "" }
};

static char *SHELL_CMDS_MUSIC_ALT[8][3] = {
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    // playlist will be owned by root but world-readable.
    { "make-playlist-all", "", "" } // A
};

static char *SHELL_CMDS_GENERAL[8][3] = {
    { "echo >&2 test general left", "", "" },
    { "echo >&2 test general right", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "poweroff-with-led", "", "" }, // start, needs hold down
    //{ "echo THATS ENOUGH", "", "" }, // start, needs hold down
    { "switch-to-internet-wired &", "", "" },
    { "switch-to-internet-wireless &", "", "" }
};

static char *SHELL_CMDS_GENERAL_ALT[8][3] = {
    { "echo >&2 test general alt left", "", "" },
    { "echo >&2 test general alt right", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" },
    { "", "", "" }
};
*/

static struct {
    bool do_uinput;
    double time_start_down;
} g;

/*
bool shell_go(char **cmds) {
    bool err = false;
    for (int i = 0 ;; i++) {
        char *cmd = *(cmds+i);
        if (!strcmp(cmd, "")) 
            break; // empty, ok

        myinfo("cmd: %s", cmd);

        // Fork can fail XX
        if (fork()) {
        }
        else {
            if (sysx(cmd))
                err = true; // useless XX
            exit(0);
        }
    }
    return ! err;
}
*/

bool ctl_default_init(bool do_uinput) {
    //verbose_cmds(false);

#ifndef NO_NES
    if (!led_init()) 
        pieprf;
#endif
    //g.statep = statep;
    g.do_uinput = do_uinput;
    g.time_start_down = -1;

    //sys_die(false);

    if (g.do_uinput) uinput_init();

    mode_set_music();

    if (!f_mpd_init()) 
        pieprf;

    return true;
}

/*
bool shell_cmd(int which, bool alt) {
    return shell_cmd_with_cb(which, alt, NULL);
}

bool shell_cmd_with_cb(int which, bool alt, void*(cb)()) {
    char **ptr;
    if (g.mode == MODE_MUSIC) {
        if (alt) 
            ptr = SHELL_CMDS_MUSIC_ALT[which];
        else 
            ptr = SHELL_CMDS_MUSIC[which];
    }
    else if (g.mode == MODE_GENERAL) {
        if (alt) 
            ptr = SHELL_CMDS_GENERAL_ALT[which];
        else 
            ptr = SHELL_CMDS_GENERAL[which];
    }
    else
        pieprf;

    if (!shell_go(ptr)) 
        pieprf;

    bool ok = true;
    if (cb) ok = (*cb)();

    return ok;
}
*/

bool ctl_do_down() {

    /*
    if (!shell_cmd(F_UP, alt)) 
        pieprf;
        */
    if (g.do_uinput) uinput_up();
    return true;
}

bool ctl_default_up() {
    /*
    if (!shell_cmd(F_DOWN, alt)) 
        pieprf;
        */
    if (g.do_uinput) uinput_down();
    return true;
}

bool ctl_default_left() {
    /*
    if (!shell_cmd(F_LEFT, alt)) 
        pieprf;
        */
    if (g.do_uinput) uinput_left();
    return true;
}

bool ctl_default_right() {
    /*
    if (!shell_cmd(F_RIGHT, alt)) 
        pieprf;
        */
    if (g.do_uinput) uinput_right();
    return true;
}

bool ctl_default_down() {
    if (g.do_uinput) uinput_right();
    return true;
}

bool ctl_default_center_x() {
    if (g.do_uinput) uinput_center_x();
    return true;
}

bool ctl_default_center_y() {
    if (g.do_uinput) uinput_center_y();
    return true;
}

bool ctl_default_select_down() {
    if (g.do_uinput) uinput_btn_select();

    return true;
}

bool ctl_default_select_up() {
    return true;
}

// require a few seconds -- shutdown
bool ctl_default_start_down() {
    if (mode_general()) {
        double now = time_hires();

        if (g.time_start_down == -1) {
            g.time_start_down = now;
        }
        else {
            /* This only works if the button is allowed to repeat (kill
             * multiple has to be configured right).
             */
            if (now - g.time_start_down > SHUTDOWN_HOLD_SECS) {
                /*
                if (!shell_cmd(F_START, false))
                    pieprf;
                    */
                g.time_start_down = -1;
            }
        }
    }

    if (g.do_uinput) uinput_btn_start();

    return true;
}

bool ctl_default_start_up() {
    if (mode_general()) 
        g.time_start_down = -1;

    return true;
}

bool ctl_default_a_down() {
        /*
        if (!shell_cmd(F_A, false)) 
            pieprf;
            */
        /*
    }
    */

    if (g.do_uinput) uinput_btn_a();
    return true;
}

bool ctl_default_a_up() {
    return true;
}

bool ctl_default_b_down() {
    /*
        if (!shell_cmd(F_B, false)) 
            pieprf;
            */
        /*
    }
    */
    if (g.do_uinput) uinput_btn_b_down();
    return true;
}

bool ctl_default_b_up() {
    if (g.do_uinput) uinput_btn_b_up();
    return true;
}

/*
bool _ctl_default_a_and_b_down() {
    shell_cmd_with_cb(F_A, true, NULL);
    return true;
}
*/

