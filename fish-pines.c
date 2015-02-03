#define _GNU_SOURCE

#define DEBUG
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
#include "ctl.h"
#include "mpd.h"
#include "util.h"

#ifndef NO_NES
# include "nes.h"
#endif

#include "fish-pines.h"

/* Each button has its own off. 
 * Each full state string has an on.
 */

/* Std order.
 */
static bool (*FN_ON[8])() = {
    ctl_do_left,
    ctl_do_right,
    ctl_do_up,
    ctl_do_down,
    ctl_do_select_down,
    ctl_do_start_down,
    ctl_do_b_down,
    ctl_do_a_down,
};
static bool (*FN_OFF[8])() = {
    ctl_do_center_x,
    ctl_do_center_x,
    ctl_do_center_y,
    ctl_do_center_y,
    ctl_do_select_up,
    ctl_do_start_up,
    ctl_do_b_up,
    ctl_do_a_up,
};

/* Is initted by compiler.
 * Static is important to avoid silently clobbering other g's.
 */

static struct {
    bool *alt; // linked to b button. also in ctl.c.
    int mode; // music or general
    /* Std order.
     */
    struct button_name_s button_names;
    struct state_s state;

    bool *state_iter[8];
    char **button_name_iter[8];

    //int max_button_print_size;

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

#ifndef NO_NES
    info("setting up wiringPi");

    if (!nes_init_wiring()) 
        die();

    info("setting up nes");

    int joystick = nes_setup();
    if (joystick == -1) 
        die();
#else
    info("setting terminal raw");
    // not interbyte timeout, because 'nothing' should also be a valid
    // response.
    if (!f_terminal_raw_input(F_UTIL_TERMINAL_MODE_READ_WITH_TIMEOUT, 0, POLL_TENTHS_OF_A_SECOND)) 
        err("Couldn't set terminal raw.");
#endif

    info("setting up ctl + mpd");

    if (!ctl_init(&g.state, DO_UINPUT)) 
        errp("Couldn't init ctl.");

    int first = 1;
    unsigned int cur_read;

    int i_mpd_update = 0;

#ifdef DEBUG
    g.debug_read_s = debug_read_init();
#endif

    char *button_print = malloc(get_max_button_print_size() * sizeof(char));
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

        if (cur_read) 
            do_read(cur_read, button_print);

        /* Check for mpd events.
         */
        if (do_mpd_update) 
            f_mpd_update();
    }

    cleanup();
    exit(0);
}

/* - - - */

void main_set_mode(int mode) {
    g.mode = mode;
}

int getRand01() {
    return (int) (1.0 * rand() / RAND_MAX * 2);
}

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

static int getRandomButtons() {
    return 
        (N_LEFT * getRand01()) | 
        (N_RIGHT * getRand01()) |
        (N_UP * getRand01()) |
        (N_DOWN * getRand01()) |
        (N_SELECT * getRand01()) |
        (N_START * getRand01()) |
        (N_B * getRand01()) |
        (N_A * getRand01());
}

static bool do_read(unsigned int cur_read, char *button_print) {
#ifdef DEBUG
    /* our bit order
     */
    unsigned int cur_read_canonical = make_canonical(cur_read);
    debug_read(cur_read_canonical, g.debug_read_s);
    info(g.debug_read_s);
#endif

    bool (*fn_on)();
    bool (*fn_off)();

    bool *state_ptr;
    char **button_name_ptr;

    /* In testing mode, polling doesn't happen fast enough to get b
     * (alt) before the others. So do an explicit check for b first.
     * But means [B] will never be printed. XX
     */
#ifdef NO_NES
    if (cur_read & N_B) 
        g.state.b = true;
#endif
    bool first = true;
    *button_print = '\0';
    for (int i = 0; i < 8; i++) {
        state_ptr = g.state_iter[i];
        button_name_ptr = g.button_name_iter[i];
        fn_on = FN_ON[i];
        fn_off = FN_OFF[i];
        bool on = cur_read & BUTTONS[i];
        if (on) {
            if (first) 
                first = false;
            else 
                strcat(button_print, " + ");
            strcat(button_print, *button_name_ptr);
        }

    }
printf("[%s] ", button_print);
    if (!process_read(cur_read, button_print)) 
        pieprf;

    return true;
}

/* If any one button is a kill multiple, consider the whole thing killed.
 */
static bool process_read(unsigned int read, char *button_print) {
    bool kill_multiple = get_kill_multiple(read);

    bool ok = true;
    bool do_it = false;

    /* Button on.
     */
    if (on) {
        /* Button was not already on.
         */
        if (! *state_ptr) {
            do_it = true;
            *state_ptr = true;
        }

        /* Button was already on -- kill multiples if applicable.
         */
        else {
            do_it = ! kill_multiple;
        }

        /* Go.
         */
        if (do_it) {
            ok = (*fn_on)();
#ifdef VERBOSE
            printf("[%s] ", button_print);
#endif
        }
    }

    /* Button off.
     */
    else {
        /* Button was already on.
         */
        if (*state_ptr) {
            ok = (*fn_off)();
            *state_ptr = false;
        }

        /* Button was not already on.
         */
        else {}

    }
    return ok;
}

static void cleanup() {
    info("setting terminal normal.");
    if (!f_terminal_normal()) 
        piep;
    fish_util_cleanup();
    fish_utils_cleanup();
    f_mpd_cleanup();

    char **button_name_ptr = (char**) (&g.button_names);
    for (int i = 0; i < 8; i++) {
        free(*button_name_ptr);
        button_name_ptr++;
    }

#ifdef DEBUG
    free(g.debug_read_s);
#endif
}

static void init_state() {
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

    g.state_iter[0] = &g.state.left;
    g.state_iter[1] = &g.state.right;
    g.state_iter[2] = &g.state.up;
    g.state_iter[3] = &g.state.down;
    g.state_iter[4] = &g.state.select;
    g.state_iter[5] = &g.state.start;
    g.state_iter[6] = &g.state.b;
    g.state_iter[7] = &g.state.a;

    g.button_name_iter[0] = &g.button_names.left;
    g.button_name_iter[1] = &g.button_names.right;
    g.button_name_iter[2] = &g.button_names.up;
    g.button_name_iter[3] = &g.button_names.down;
    g.button_name_iter[4] = &g.button_names.select;
    g.button_name_iter[5] = &g.button_names.start;
    g.button_name_iter[6] = &g.button_names.b;
    g.button_name_iter[7] = &g.button_names.a;

    g.alt = &g.state.b;
    g.mode = -1;
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

static bool get_kill_multiple(unsigned int read) {
    // music or general
    if (g.mode == -1) 
        pieprf;

    unsigned int *ary;
    int cnt;

    if (g.mode == MODE_MUSIC) {
        ary = KILL_NEW_MUSIC;
        cnt = NUM_KILL_MULTIPLE_RULES_MUSIC;
    }
    else if (g.mode == MODE_GENERAL) {
        ary = KILL_NEW_GENERAL;
        cnt = NUM_KILL_MULTIPLE_RULES_GENERAL;
    }

    /* Stop on first matching rule.
     */
    for (int i = 0; i < cnt*2; i += 2) {
        unsigned int mask = ary[i];
        bool kill = (bool) ary[i+1];
        //info("read is %d, mask is %d, read&mask is %d", read, mask, read&mask);
        if ((read & mask) == mask) {
#ifdef DEBUG
            info("kill multiple: %d", kill);
#endif
            return kill;
        }
    }
    // XX DEFAULT
    return false;
}
