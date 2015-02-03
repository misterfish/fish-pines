#ifndef __INCL_BUTTONS_H
#define __INCL_BUTTONS_H

#include <stdbool.h>

#include "constants.h"

/*
#ifndef __INCL_CONSTANTS_H
#error buttons.h needs constants.h
#endif
*/

/* Mapping from our standard order to their N_ order.
 */
static short BUTTONS[] = {
    N_LEFT, N_RIGHT,
    N_UP, N_DOWN,
    N_SELECT, N_START,
    N_B, N_A
};

//static int NUM_BUTTONS = 8;

#define KILL_MULTIPLE_DEFAULT false
#define NUM_KILL_MULTIPLE_RULES_MUSIC 9
#define NUM_KILL_MULTIPLE_RULES_GENERAL 1

#define NUM_RULES_MUSIC 9
#define NUM_RULES_GENERAL 1

/* Rules for button combinations.
 * Note, there is no release event for a combination. 
 * Could be in the future though.
 */
struct button_rule {
    unsigned int mask;
    bool kill_multiple;
    void* (*press_event)();
};

/* Put rules in order with more buttons first.
 */

static struct button_rule *rules_music[NUM_RULES_MUSIC];
static struct button_rule *rules_general[NUM_RULES_GENERAL];

static unsigned int KILL_MULTIPLE_MUSIC[2 * NUM_KILL_MULTIPLE_RULES_MUSIC] = {
    (N_B | N_LEFT)  , 0, // seek left
    (N_B | N_RIGHT) , 0, // seek right
    (N_B | N_UP)    , 1, // playlist up
    (N_B | N_DOWN)  , 1, // playlist down
    (      N_LEFT)  , 1, // prev song
    (      N_RIGHT) , 1, // next song
    (      N_UP)    , 1, // vol up
    (      N_DOWN)  , 1, // vol down
    (N_A)           , 1, // random
};

static unsigned int KILL_MULTIPLE_GENERAL[2 * NUM_KILL_MULTIPLE_RULES_GENERAL] = {
    (N_START)       , 0, // so we can hold down start for power off
};

/* Std order.
 * Used for state and for verbose btn names.
 */

/*
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
*/

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

bool buttons_init();

#endif
