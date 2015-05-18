#ifndef __INCL_BUTTONS_H
#define __INCL_BUTTONS_H

#include <stdbool.h>

#include <fish-utils.h>

#include "global.h"

// default is don't kill
#define BUTTONS_KILL_MULTIPLE_DEFAULT false

/* Rules for button press and button release. 
 * kill_multiple only applies to press.
 */
struct button_rule_t {
    short buttons;
    bool kill_multiple;
    short event;
    // Lua function, indexed at this value in the registry.
    int handler;
    bool has_handler;
};

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

struct button_rule_t *buttons_get_rule(short read);

int buttons_add_rulel();

struct button_rule_t *buttons_get_rule_press(short mode, short read);
struct button_rule_t *buttons_get_rule_release(short mode, short read);

#endif
