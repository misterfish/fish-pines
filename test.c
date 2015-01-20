#define _GNU_SOURCE

#undef DEBUG

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include <fish-util.h>

#include "nes.h"
#include "ctl.h"

int UPOLL = 40000;

enum {
    L = 1, R,
    U, D,
    C, R,
    B, A,
};

bool KILL_MULTIPLE[8] = {
    true, true,
    false, false,
    true, true,
    false, true,
};

short BUTTONS[8] = {
    N_UP, N_DOWN,
    N_LEFT, N_RIGHT,
    N_SELECT, N_START,
    N_A, N_B,
};

char* STR[8] = {
    "  UP  ", " DOWN ",
    " LEFT ", "RIGHT ",
    "SELECT", "START ",
    "  A   ", "  B   ",
};

void* FN[8] = {
    ctl_do_up, ctl_do_down,
    ctl_do_left, ctl_do_right,
            ctl_do_center_y();
                ctl_do_select_down();
            ctl_do_select_up();
                ctl_do_start_down();
            ctl_do_start_up();
                ctl_do_a_down();
            ctl_do_a_up();
                ctl_do_b_down();
            ctl_do_b_up();

// can alt
// up down left right
            ctl_do_center_x();
struct {
    bool cur[8];
} g;

int main (int argc, char** argv) {
    autoflush();
    info("setting up wiringPi");

    nes_init_wiring();

    info("setting up nes");

    int joystick = nes_setup();

    info("setting up ctl");

    ctl_init();

    int first = 1;

    unsigned int buttons;

    while (1) {
        first = first ? 0 : 0 * usleep (SLEEP);

        buttons = nes_read(joystick);

        // dir
        for (int i = L; i <= D; i++) {
            bool do_it = false;
            if (buttons & BUTTONS[i]) {
                if (!g.cur[i]) {
                    do_it = true;
                    g.cur[i] = true;
                }
                else {
                    do_it = ! KILL_MULTIPLE[i];
                }

                if (do_it) {
                    // xx
                    ctl_do_up(cur.b);
#ifdef DEBUG
                    printf("%s", STR[i]);
#endif
                }
            }
            else {
                g.cur[i] = false;
                FN_CENTER[i];
            }
        }

        if ((buttons & N_SELECT) != 0) {
            if (!cur.select) {
                do_it = true;
                cur.select = true;
            }
            else {
                do_it = ! KILL_MULTIPLE_SELECT;
            }

            if (do_it) {
#ifdef DEBUG
                printf ("SELECT" ) ;
#endif
            }
        }
        else {
            cur.select = false;
        }
        if ((buttons & N_START)  != 0) {
            if (!cur.start) {
                do_it = true;
                cur.start = true;
            }
            else {
                do_it = ! KILL_MULTIPLE_START;
            }

            if (do_it) {
#ifdef DEBUG
                printf ("START " ) ;
#endif
            }
        }
        else {
            cur.start = false;
        }
        if ((buttons & N_A)      != 0) {
            if (!cur.a) {
                do_it = true;
                cur.a = true;
            }
            else {
                do_it = ! KILL_MULTIPLE_A;
            }

            if (do_it) {
#ifdef DEBUG
                printf ("  A   " ) ;
#endif
            }
        }
        else {
            cur.a = false;
        }
        if ((buttons & N_B)      != 0) {
            if (!cur.b) {
                do_it = true;
                cur.b = true;
            }
            else {
                do_it = ! KILL_MULTIPLE_B;
            }

            if (do_it) {
#ifdef DEBUG
                printf ("  B   " ) ;
#endif
            }
        }
        else {
            cur.b = false;
        }
    }
}

