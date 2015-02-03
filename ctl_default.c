#define _GNU_SOURCE

#include <unistd.h>
#include <string.h>

#include <fish-util.h>

#include "constants.h"
#include "conf.h"
#include "buttons.h"
#include "mpd.h"
#include "uinput.h"

#ifndef NO_NES
#include "led.h"
#endif

#include "ctl.h"

#include "fish-pines.h"

//#define SHUTDOWN_HOLD_SECS 2
#define SHUTDOWN_HOLD_SECS 0

#define myinfo(x, ...) do { \
    printf("\n"); \
    info(x, ##__VA_ARGS__); \
} while (0) ;

bool _ctl_do_a_and_b_down();
bool shell_cmd_with_cb(int which, bool alt, void*(cb)());

// XX
bool alt = false;

static char *SOCKET_VOLD = "/tmp/.vold-simple-socket";

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

static struct {
    bool do_uinput;

    double time_start_down;

    int mode;

    // passed from fish-pines.c during init
    //struct state_s *statep;
} g;

bool mode_music();
bool mode_general(); 

bool shell_go(char **cmds) {
    bool err = false;
    for (int i = 0 ;; i++) {
        char *cmd = *(cmds+i);
        if (!strcmp(cmd, "")) 
            break; // empty, ok

        myinfo("cmd: %s", cmd);

        /* Fork can fail XX
         */
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

//bool ctl_init(struct state_s *statep, bool do_uinput) {
bool ctl_init(bool do_uinput) {
    verbose_cmds(false);

#ifndef NO_NES
    if (!led_init()) 
        pieprf;
#endif
    //g.statep = statep;
    g.do_uinput = do_uinput;
    g.time_start_down = -1;

    sys_die(false);

    if (g.do_uinput) uinput_init();

    set_mode(MODE_MUSIC);
    if (!update_mode_led()) 
        pieprf;

    if (!f_mpd_init()) 
        pieprf;

    return true;
}

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

bool ctl_do_down() {
    //bool alt = g.statep->b;
    if (alt) {
        if (!f_mpd_next_playlist())
            pieprf;
    }
    else {
        if (mode_music()) 
            if (!socket_unix_message(SOCKET_VOLD, "up")) 
                pieprf;
    }

    if (!shell_cmd(F_UP, alt)) 
        pieprf;
    if (g.do_uinput) uinput_up();
    return true;
}

bool ctl_do_up() {
    //bool alt = g.statep->b;
    if (alt) {
        if (!f_mpd_prev_playlist())
            pieprf;
    }
    else {
        if (mode_music()) 
            if (!socket_unix_message(SOCKET_VOLD, "down")) 
                pieprf;
    } 
    if (!shell_cmd(F_DOWN, alt)) 
        pieprf;
    if (g.do_uinput) uinput_down();
    return true;
}

bool ctl_do_left() {
    //bool alt = g.statep->b;
    if (mode_music()) {
        if (alt) {
            if (!f_mpd_seek(-5)) 
                pieprf;
        }
        else {
            if (!f_mpd_prev()) 
                pieprf;
        }
    }

    if (!shell_cmd(F_LEFT, alt)) 
        pieprf;
    if (g.do_uinput) uinput_left();
    return true;
}

bool ctl_do_right() {
    //bool alt = g.statep->b;
    if (mode_music()) {
        if (alt) {
            if (!f_mpd_seek(5))
                pieprf;
        }
        else {
            if (!f_mpd_next()) 
                pieprf;
        }
    }

    if (!shell_cmd(F_RIGHT, alt)) 
        pieprf;
    if (g.do_uinput) uinput_right();
    return true;
}

bool ctl_do_center_x() {
    if (g.do_uinput) uinput_center_x();
    return true;
}

bool ctl_do_center_y() {
    if (g.do_uinput) uinput_center_y();
    return true;
}

bool ctl_do_select_down() {
    if (g.do_uinput) uinput_btn_select();

    set_mode(!g.mode);
    _();
    BR(g.mode == MODE_MUSIC ? "music" : "general");
    myinfo("set mode to %s", _s);
    if (!update_mode_led())
        pieprf;
    return true;
}

bool ctl_do_select_up() {
    return true;
}

// require a few seconds -- shutdown
bool ctl_do_start_down() {
    if (mode_general()) {
        double now = time_hires();

        if (g.time_start_down == -1) {
            g.time_start_down = now;
        }
        else {
            /* This only works if the button is allowed to repeat. But start
             * isn't.
             */
            if (now - g.time_start_down > SHUTDOWN_HOLD_SECS) {
                if (!shell_cmd(F_START, false))
                    pieprf;
                g.time_start_down = -1;
            }
        }
    }

    if (g.do_uinput) uinput_btn_start();

    return true;
}

bool ctl_do_start_up() {

    if (mode_music()) {
        if (!f_mpd_toggle_play()) 
            pieprf;
    }
    else 
        g.time_start_down = -1;

    return true;
}

bool ctl_do_a_down() {
    /*
    if (g.statep->b) {
        if (!_ctl_do_a_and_b_down()) 
            pieprf;
    }
    else {
    */
        if (mode_music()) {
            if (!f_mpd_toggle_random()) 
                pieprf;
        }
        if (!shell_cmd(F_A, false)) 
            pieprf;
        /*
    }
    */

    if (g.do_uinput) uinput_btn_a();
    return true;
}

bool ctl_do_a_up() {
    return true;
}

bool ctl_do_b_down() {
    /*
    if (g.statep->a) {
        if (!_ctl_do_a_and_b_down()) 
            pieprf;
    }
    else {
    */
        if (!shell_cmd(F_B, false)) 
            pieprf;
        /*
    }
    */
    if (g.do_uinput) uinput_btn_b_down();
    return true;
}

bool ctl_do_b_up() {
    if (g.do_uinput) uinput_btn_b_up();
    return true;
}

bool _ctl_do_a_and_b_down() {
    shell_cmd_with_cb(F_A, true, NULL);
    return true;
}

bool mode_music() {
    return g.mode == MODE_MUSIC;
}

bool mode_general() {
    return g.mode == MODE_GENERAL;
}

static void set_mode(int mode) {
    g.mode = mode;
    main_set_mode(mode);
}

static bool update_mode_led() {
#ifndef NO_NES
    if (!led_update_mode(g.mode)) {
        warn("Couldn't update led for mode");
        return false;
    }
#endif
    return true;
}
