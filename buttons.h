#ifndef __INCL_BUTTONS_H
#define __INCL_BUTTONS_H

#include <stdbool.h>

#include <fish-utils.h>

#include "global.h"

// default is don't kill
//#define BUTTONS_KILL_MULTIPLE_DEFAULT false

/* Rules for button press and button release. 
 * kill_multiple only applies to press.
 */
struct button_rule_t {
    short buttons;
    bool once;
    bool chain; // when false, stop after first matching rule.
    short event;
    
    /* Allow a rule to have no handler, for now.
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

int buttons_add_rulel();

/*
struct button_rule_t *buttons_get_rule_press(short mode, short read);
struct button_rule_t *buttons_get_rule_release(short mode, short read);
*/

bool buttons_get_rules_press(short mode, short read, vec *rules_ret);
bool buttons_get_rules_release(short mode, short read, vec *rules_ret);

#endif
