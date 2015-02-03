#ifndef __INCL_BUTTONS_H
#define __INCL_BUTTONS_H

#ifndef __INCL_CONSTANTS_H
#error buttons.h needs constants.h
#endif

static char *allen = "allen";

/* Mapping from our standard order to their N_ order.
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

#define NUM_KILL_MULTIPLE_RULES_MUSIC 8
#define NUM_KILL_MULTIPLE_RULES_GENERAL 1

/* Put rules in order with more buttons first.
 * if the or equals itself.
 * def true
 */

static unsigned int KILL_NEW_MUSIC[2 * NUM_KILL_MULTIPLE_RULES_MUSIC] = {
    (N_B | N_LEFT)  , 0, // seek left
    (N_B | N_RIGHT) , 0, // seek right
    (N_B | N_UP)    , 1, // playlist up
    (N_B | N_DOWN)  , 1, // playlist down
    (      N_LEFT)  , 1, // prev song
    (      N_RIGHT) , 1, // next song
    (      N_UP)    , 1, // vol up
    (      N_DOWN)  , 1, // vol down
};

static unsigned int KILL_NEW_GENERAL[2 * NUM_KILL_MULTIPLE_RULES_GENERAL] = {
    (N_START)       , 0, // so we can hold down start for power off
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
