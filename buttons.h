#ifndef __INCL_BUTTONS_H
#define __INCL_BUTTONS_H

#include <stdbool.h>

#include <fish-utils.h>

#include "constants.h"

/* Mapping from our standard order to their N_ order.
 */
static short BUTTONS[] = {
    N_LEFT, N_RIGHT,
    N_UP, N_DOWN,
    N_SELECT, N_START,
    N_B, N_A
};

// default is don't kill
#define BUTTONS_KILL_MULTIPLE_DEFAULT false

#define NUM_KILL_MULTIPLE_RULES_MUSIC 9
#define NUM_KILL_MULTIPLE_RULES_GENERAL 1

#define NUM_RULES_MUSIC 9
#define NUM_RULES_GENERAL 1

/* Rules for button combinations.
 * Note, there is no release event for a combination. 
 * Could be in the future though.
 */
struct button_rule {
    unsigned int buttons;
    bool kill_multiple;
    bool (*press_event)();
};

static vec *rules_music; // members: struct button_rule *
static vec *rules_general; // members: struct button_rule *

/* Std order.
 * Used for state and for verbose btn names.
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
bool buttons_cleanup();

struct button_rule *buttons_get_rule(unsigned int read);

#endif
