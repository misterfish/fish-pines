#ifndef __INCL_BUTTONS_H
#define __INCL_BUTTONS_H

#include <stdbool.h>

#include <fish-utils.h>

#include "global.h"

/* Rules for button press and button release. 
 * 'once' only applies to press events.
 * See defines in *.c for defaults.
 */
struct button_rule_t {
    short buttons;
    short event;

    bool once; // don't trigger repeatedly on hold (only applies to press of course) (default false)
    bool chain; // when false, stop after first matching rule. (only applies to press; we could easily allow it to apply to release as well but that doesn't seem useful) (default false)
    bool exact; // require exact match to trigger (only applies to press) (default true)
    
    /* A rule is allowed to have no handler.
     * The only use I can think of where this might be handy is for example
     * a rule whose only purpose is to cancel further rules.
     */
    int handler; // lua function, indexed at this value in the registry.
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

int buttons_add_rule_l(lua_State *L);

bool buttons_get_rules_press(short mode, short read, vec *rules_ret);
bool buttons_get_rules_release(short mode, short read, vec *rules_ret);

#endif
