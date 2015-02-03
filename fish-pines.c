#define _GNU_SOURCE

#define VERBOSE

#define DO_UINPUT false

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

/* rand
 */
#include <stdlib.h>

#include <signal.h>

#include <fish-util.h>
#include <fish-utils.h>

#include "constants.h"
#include "buttons.h"
#include "ctl-default.h"
#include "ctl-custom.h"
#include "mpd.h"
#include "mode.h"
#include "util.h"

#ifndef NO_NES
# include "nes.h"
#endif

#include "fish-pines.h"

/* 
 * Std order.
 *
 * On each read, look up the first matching rule for the combination. 
 * If kill_multiple is true in the rule, and the read is the same as the
 * last one, don't do anything else.
 * Otherwise, cycle through the buttons and fire the press events for
 * each currently pressed button (meaning, tap or hold) and the release event for each button released since the last read.
 *
 * Then fire the specific event for the combination (from the looked up
 * rule) if applicable.
 */

static bool (*EVENT_PRESS[8])() = {
    ctl_default_left,
    ctl_default_right,
    ctl_default_up,
    ctl_default_down,
    ctl_default_select_down,
    ctl_default_start_down,
    ctl_default_b_down,
    ctl_default_a_down,
};
static bool (*EVENT_RELEASE[8])() = {
    ctl_default_center_x,
    ctl_default_center_x,
    ctl_default_center_y,
    ctl_default_center_y,
    ctl_default_select_up,
    ctl_default_start_up,
    ctl_default_b_up,
    ctl_default_a_up,
};

/* Static is important to avoid silently clobbering other g's.
 */

static struct {
    /* Structs have named fields 'a', 'start', 'down', etc.
     */
    struct button_name_s button_names;

    /* Tracks the state of each button. 
     * The binary number representing a read is not stored globally.
     */
    //struct state_s state;

    /* Std order.
     */
    //bool *state_iter[8];
    char **button_name_iter[8];

    char *button_print;

#ifdef DEBUG
    char *debug_read_s;
#endif
} g;

bool _break;

bool sighandler_term() {
    info("Ctl c");
    _break = true;
}

int main (int argc, char** argv) {

    fish_utils_init();

    autoflush();

    f_sig(SIGTERM, sighandler_term);
    f_sig(SIGINT, sighandler_term);

    init_state();

    if (!buttons_init()) 
        ierr;

#ifndef NO_NES
    info("setting up wiringPi");

    if (!nes_init_wiring()) 
        ierr;

    info("setting up nes");

    int joystick = nes_setup();
    if (joystick == -1) 
        ierr;
#else
    info("setting terminal raw");
    // not interbyte timeout, because 'nothing' should also be a valid
    // response.
    if (!f_terminal_raw_input(F_UTIL_TERMINAL_MODE_READ_WITH_TIMEOUT, 0, POLL_TENTHS_OF_A_SECOND)) 
        err("Couldn't set terminal raw.");
#endif

    info("setting up ctl + mpd");

    if (!ctl_default_init(DO_UINPUT)) 
        ierr_msg("Couldn't init ctl-default.");

    if (!ctl_custom_init()) 
        ierr_msg("Couldn't init ctl-custom.");

    int first = 1;
    unsigned int cur_read;

    int i_mpd_update = 0;

#ifdef DEBUG
    g.debug_read_s = debug_read_init();
#endif

    g.button_print = malloc(get_max_button_print_size() * sizeof(char));

    /* Main loop.
     */
    while (1) {
        if (_break) break; // ctl c
        bool do_mpd_update = false;

        if (++i_mpd_update % MPD_UPDATE == 0) {
            i_mpd_update = 0;
            do_mpd_update = true;
        }

#ifdef NO_NES
        /* timeout happens in readkey
         */
        cur_read = read_buttons_testing();
#else
        /* sleep here
         */
        first = first ? 0 : 0 * usleep (1000 * POLL_MS);
        cur_read = nes_read(joystick);
#endif

        /* Do it also if cur_read is 0.
         */
        do_read(cur_read);

        /* Check for mpd events.
         */
        if (do_mpd_update) 
            f_mpd_update();
    }

    cleanup();
    exit(0);
}

/* - - - */

bool is_button_dir(int i) {
    //return i <= 
}

static unsigned int read_buttons_testing() {
    ssize_t s;
    char *buf = calloc(2, sizeof(char));
    int fd = 0; // stdin
    s = read(fd, buf, 1);

    int ret;
    if (s == -1) {
        ret = 0;
    }
    else {
        ret = 
            *buf == 'a' ? N_A :
            *buf == 'b' ? N_B :
            *buf == 'h' ? N_LEFT :
            *buf == 'l' ? N_RIGHT :
            *buf == 'j' ? N_DOWN :
            *buf == 'k' ? N_UP :
            *buf == 'H' ? N_B | N_LEFT :
            *buf == 'L' ? N_B | N_RIGHT :
            *buf == 'J' ? N_B | N_DOWN :
            *buf == 'K' ? N_B | N_UP :
            *buf == ',' ? N_SELECT :
            *buf == '.' ? N_START :
            0;
    }
    free(buf);
    return ret;
}

static bool do_read(unsigned int cur_read) {
#ifdef DEBUG
    if (cur_read) {
        /* our bit order
         */
        unsigned int cur_read_canonical = make_canonical(cur_read);
        debug_read(cur_read_canonical, g.debug_read_s);
        info(g.debug_read_s);
    }
#endif

    //bool *state_ptr;
    char **button_name_ptr;

    char *button_print = g.button_print;

    /* In testing mode, polling doesn't happen fast enough to get b
     * (alt) before the others. So do an explicit check for b first.
     * But means [B] will never be printed. XX
     */
#ifdef NO_NES
    if (cur_read & N_B)  {
    }
        //g.state.b = true;
#endif
    bool first = true;
    *button_print = '\0';
    for (int i = 0; i < 8; i++) {
        //state_ptr = g.state_iter[i];
        button_name_ptr = g.button_name_iter[i];
        bool on = cur_read & BUTTONS[i];
        if (on) {
            if (first) 
                first = false;
            else 
                strcat(button_print, " + ");
            strcat(button_print, *button_name_ptr);
        }

    }
//printf("[%s] ", button_print);

    if (!process_read(cur_read, button_print)) 
        pieprf;

    return true;
}

static bool process_read(unsigned int read, char *button_print) {
    static unsigned int prev_read = -1;

    struct button_rule *rule = buttons_get_rule(read);

    bool kill_multiple = BUTTONS_KILL_MULTIPLE_DEFAULT;
    bool (*press_cb)() = NULL;
    if (rule) {
        kill_multiple = rule->kill_multiple;
        press_cb = rule->press_event;
    }

    if ((prev_read == read) && kill_multiple) {
#ifdef DEBUG
        info("Ignoring read %d (kill multiple is true)", read);
#endif
        return true;
    }

    /* Do the press and release events for each individual button. 
     * Then do the event matching the combination.
     */
    bool ok = true;
    for (int i = 0; i < 8; i++) {
        int button_flag = BUTTONS[i]; // wiringPi system
        bool (*event_press)() = EVENT_PRESS[i];
        bool (*event_release)() = EVENT_RELEASE[i];

        bool this_ok;
        /* Button down (press or hold).
         */
        if (read & button_flag) {
            this_ok = (*event_press)();
            if (!this_ok) 
                ok = false;
        }

        /* Button not down.
         */
        else {
            /* Released.
             */
            if (prev_read & button_flag) {
                this_ok = (*event_release)();
                if (!this_ok) 
                    ok = false;
            }

            /* Was up and stayed up, do nothing.
             */
            else {
            }
        }
    }

    if (press_cb) {
        if (! (*press_cb)())
            pieprf;
    }
    prev_read = read;
    return ok;
}

static void cleanup() {
#ifdef NO_NES
    info("setting terminal normal.");
    if (!f_terminal_normal()) 
        piep;
#endif
    fish_util_cleanup();
    fish_utils_cleanup();

    if (!f_mpd_cleanup())
        piep;
    if (!buttons_cleanup()) 
        piep;

    char **button_name_ptr = (char**) (&g.button_names);
    for (int i = 0; i < 8; i++) {
        free(*button_name_ptr);
        button_name_ptr++;
    }

    free(g.button_print);

#ifdef DEBUG
    free(g.debug_read_s);
#endif
}

static void init_state() {
    /*
    ghash_table ht = new;
    ht_add(ht, 
        (N_B | N_LEFT), 
        button_spec_new(
            false, 
            */

    /* malloc
     */
    g.button_names.left = G_("left");
    g.button_names.right = G_("right");
    g.button_names.up = G_("up");
    g.button_names.down = G_("down");
    g.button_names.select = Y_("select");
    g.button_names.start = Y_("start");
    g.button_names.b = CY_("B");
    g.button_names.a = CY_("A");

    /*
    g.state_iter[0] = &g.state.left;
    g.state_iter[1] = &g.state.right;
    g.state_iter[2] = &g.state.up;
    g.state_iter[3] = &g.state.down;
    g.state_iter[4] = &g.state.select;
    g.state_iter[5] = &g.state.start;
    g.state_iter[6] = &g.state.b;
    g.state_iter[7] = &g.state.a;
    */

    g.button_name_iter[0] = &g.button_names.left;
    g.button_name_iter[1] = &g.button_names.right;
    g.button_name_iter[2] = &g.button_names.up;
    g.button_name_iter[3] = &g.button_names.down;
    g.button_name_iter[4] = &g.button_names.select;
    g.button_name_iter[5] = &g.button_names.start;
    g.button_name_iter[6] = &g.button_names.b;
    g.button_name_iter[7] = &g.button_names.a;
}

static int make_canonical(unsigned int read) {
    return 
        ((read & N_LEFT)    ? 1<<7 : 0) |
        ((read & N_RIGHT)   ? 1<<6 : 0) |
        ((read & N_UP)      ? 1<<5 : 0) |
        ((read & N_DOWN)    ? 1<<4 : 0) |
        ((read & N_SELECT)  ? 1<<3 : 0) |
        ((read & N_START)   ? 1<<2 : 0) |
        ((read & N_B)       ? 1<<1 : 0) |
        ((read & N_A)       ? 1<<0 : 0);
}

static char *debug_read_init() {
    char *ret = malloc(
        9
        * sizeof(char)
    );
    ret[8] = '\0';
    return ret;
}

static void debug_read(unsigned int read_canonical, char *ret) {
    char *ptr = ret;
    for (int shift = 7; shift >= 0; shift--) {
        *ptr++ = (read_canonical & (1<<shift)) ? '1' : '0';
    }
}

static int get_max_button_print_size() {
    int n=0;
    for (int i = 0; i < 8; i++) {
        n += strlen(*g.button_name_iter[i]) 
            + 3; // ' + '
    }
    return n+1;
}


