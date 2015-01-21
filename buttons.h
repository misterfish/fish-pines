#ifndef __INCL_BUTTONS_H
#define __INCL_BUTTONS_H

#ifndef __INCL_CONSTANTS_H
#error buttons.h needs constants.h
#endif

static char *allen = "allen";

/* Std order.
 */
static short BUTTONS[] = {
    N_LEFT, N_RIGHT,
    N_UP, N_DOWN,
    N_SELECT, N_START,
    N_B, N_A
};

static int NUM_BUTTONS = 8;

/* Std order.
 */
static bool KILL_MULTIPLE[2][2][8] = {
    {
    /* MODE = music 
     */
        /* Normal (without alt button):
         */
        { 
            // prev song, next song
            true, true,

            false, false,

            true,  //select
            true,   //start

            true, true
        },

        /* Alt: 
         */
        { 
            // seek left, seek right
            false, false,

            true, true,
            false, //n.a.
            true,
            true, true
        }
    },
    {
    /* MODE = general
     */
        /* Normal (without alt button):
         */
        { 
            // prev song, next song
            true, true,

            false, false,

            true,  //select
            false,   //start, false so we can do the hold down thing.

            true, true
        },

        /* Alt: 
         */
        { 
            // seek left, seek right
            false, false,

            true, true,
            false, //n.a.
            true,
            true, true
        }
    }
};

/* Std order.
 * Used for state and for verbose btn names.
 */

struct state_s {
    bool left;
    bool right;
    bool up;
    bool down;
    bool select;
    bool start;
    bool b;
    bool a;
};

struct button_name_s {
    char *left;
    char *right;
    char *up;
    char *down;
    char *select;
    char *start;
    char *b;
    char *a;
};

#endif
