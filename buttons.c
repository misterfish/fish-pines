#define _GNU_SOURCE

#include "buttons.h"
//#include "mode.h"

#define new_rule(vec, buttons, kill_multiple, cb) { \
    bool rc = rule(vec, buttons, kill_multiple, cb); \
    if (!rc) \
        pieprf; \
};

/* Mapping from our standard order to their N_ order.
 */

static vec *rules_music; // members: struct button_rule *
static vec *rules_general; // members: struct button_rule *

static bool rule(vec *vec, unsigned int buttons, bool kill_multiple, bool (*cb)()) {
    struct button_rule *rule = f_malloc(sizeof(struct button_rule));

    rule->buttons = buttons;
    rule->kill_multiple = kill_multiple;
    rule->press_event = cb;

    if (!vec_add(vec, rule))
        pieprf;

    return true;
}


bool buttons_init() {
#if 0
    /* Put rules in order -- first matching rule wins.
 */

    rules_music = vec_new();
    rules_general = vec_new();

    new_rule(rules_music, 
        (N_A | N_B   ), true, ctl_custom_b_a       // make playlist all
    )
    new_rule(rules_music, 
        (N_B | N_LEFT), false, ctl_custom_b_left    // seek left
    )
    new_rule(rules_music,
        (N_B | N_RIGHT), false, ctl_custom_b_right  // seek right
    )
    new_rule(rules_music,
        (N_B | N_UP), true, ctl_custom_b_up         // playlist up
    )
    new_rule(rules_music,
        (N_B | N_DOWN), true, ctl_custom_b_down     // playlist down
    )
    new_rule(rules_music,
        (      N_LEFT), true, ctl_custom_left       // prev song
    )
    new_rule(rules_music,
        (      N_RIGHT), true, ctl_custom_right     // next song
    )
    new_rule(rules_music,
        (      N_UP), false, ctl_custom_up           // vol up
    )
    new_rule(rules_music,
        (      N_DOWN), false, ctl_custom_down       // vol down
    )
    new_rule(rules_music,
        (      N_A), true, ctl_custom_a             // random
    )
    new_rule(rules_music,
        (      N_START), true, ctl_custom_start     // play/pause
    )

    new_rule(rules_music,
        (      N_SELECT), true, ctl_custom_select     // mode toggle
    )

    new_rule(rules_general,
        (N_B         ), true, ctl_custom_b            // internet wired
    )

    new_rule(rules_general,
        (N_A         ), true, ctl_custom_a            // internet wireless
    )

    new_rule(rules_general,
        (      N_SELECT), true, ctl_custom_select                // mode toggle
    )

    new_rule(rules_general,
        (      N_START), false, ctl_custom_start                // poweroff, with hold down
    )

#endif
    return true;
}

struct button_rule *buttons_get_rule(unsigned int read) {
return NULL;
    vec *vec;
    int cnt;

    /*
    if (mode_music()) {
        vec = rules_music;
    }
    else if (mode_general()) {
        vec = rules_general;
    }
    else 
        pieprf;
        */

    vec = rules_music;

    cnt = vec_size(vec);

    /* Stop on first matching rule.
     */
    for (int i = 0; i < cnt; i++) {
        // ok to cast NULL
        struct button_rule *rule = (struct button_rule *) vec_get(vec, i);
        if (rule == NULL) {
            piep;
            continue;
        }

        unsigned int mask = rule->buttons;
        if ((read & mask) == mask) {
#ifdef DEBUG
            info("kill multiple: %d", rule->kill_multiple);
#endif
            return rule;
        }
    }

    return NULL;
}

bool buttons_cleanup() {
    bool ok = true;
    if (!vec_destroy_flags(rules_music, VEC_DESTROY_DEEP)) {
        piep;
        ok = false;
    }
    if (!vec_destroy_flags(rules_general, VEC_DESTROY_DEEP)) {
        piep;
        ok = false;
    }
    return ok;
}
