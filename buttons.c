#define _GNU_SOURCE

#include <lua.h>
#include <lauxlib.h> // luaL_ functions..h>

#include "const.h"
#include "mode.h"

#include "buttons.h"

#define BUTTONS_PRESS 0
#define BUTTONS_RELEASE 1

/* vector of vector of vectors.
 * [ mode => [ {_PRESS|_RELEASE} => [ button_rule_t *rule, ...] ] ]
 */
static struct {
    vec *rules;
} g;

/* Return pointer to our global vector-in-a-vector. */
static vec *get_rules_for_event(short mode, short event);

/* Fill in the given vector. */
static bool get_rules_for_read(short mode, short event, short read, vec *rules_ret);

// add checks for combos / single logic.

/* Throws.
 */
int buttons_add_rule_l() {
    lua_State *L = global.L;
    lua_pushnil(L); // init iter
    short buttons = 0;
    short mode = -1;
    short event = -1;

    struct button_rule_t *rule = f_mallocv(*rule);
    memset(rule, '\0', sizeof *rule);
    while (lua_next(L, -2)) {
        const char *luatype = lua_typename(L, lua_type(L, -2));
        // plain table entry, i.e. button name.
        if (! strcmp(luatype, "number")) {
            const char *value = luaL_checkstring(L, -1);
            if (! strcmp(value, "a")) 
                buttons |= N_A;
            else if (! strcmp(value, "b"))
                buttons |= N_B;
            else if (! strcmp(value, "select"))
                buttons |= N_SELECT;
            else if (! strcmp(value, "start"))
                buttons |= N_START;
            else if (! strcmp(value, "up"))
                buttons |= N_UP;
            else if (! strcmp(value, "down"))
                buttons |= N_DOWN;
            else if (! strcmp(value, "left"))
                buttons |= N_LEFT;
            else if (! strcmp(value, "right"))
                buttons |= N_RIGHT;
        }
        else {
            const char *key = luaL_checkstring(L, -2);
            if (! strcmp(key, "once")) {
                const bool value = lua_toboolean(L, -1);
                rule->once = value;
            }
            if (! strcmp(key, "chain")) {
                const bool value = lua_toboolean(L, -1);
                rule->chain = value;
            }
            else if (! strcmp(key, "mode")) {
                lua_Number val = luaL_checknumber(L, -1);
                mode = (short) val;
            }
            else if (! strcmp(key, "event")) {
                const char *value = luaL_checkstring(L, -1);
                if (! strcmp(value, "press"))
                    event = BUTTONS_PRESS;
                else if (! strcmp(value, "release"))
                    event = BUTTONS_RELEASE;
                else {
                    lua_pushstring(global.L, ("Unknown event type %s."));
                    lua_error(global.L);
                }
            }
            /* Note that a nil handler will never even show up here (can't
             * have nil as value in a table).
             */
            else if (! strcmp(key, "handler")) {
                if (strcmp(lua_typename(L, lua_type(L, -1)), "function"))
                    piep;
                else {
                    int reg_index = luaL_ref(L, LUA_REGISTRYINDEX); // also pops
                    rule->has_handler = true;
                    rule->handler = reg_index;
                    continue;
                }
            }
        }
        lua_pop(L, 1);
    }

    if (mode == -1) {
        lua_pushstring(global.L, "Need mode for rule.");
        lua_error(global.L);
    }

    if (event == -1) {
        lua_pushstring(global.L, "Need event for rule.");
        lua_error(global.L);
    }

    if (! buttons) {
        lua_pushstring(global.L, "Need buttons for rule.");
        lua_error(global.L);
    }

    rule->buttons = buttons;
    rule->event = event;

    vec *rules = get_rules_for_event(mode, event);

    if (! rules) 
        piepr0;
    else 
        vec_add(rules, rule);

    return 0;
}

static vec *get_rules_for_event(short mode, short event) {
    vec *rules_for_mode = vec_get(g.rules, mode);
    if (!rules_for_mode) 
        pieprnull;
    vec *rules_for_event = vec_get(rules_for_mode, event);
    if (!rules_for_event) 
        pieprnull;
    return rules_for_event;
}

static bool get_rules_for_read(short mode, short event, short read, vec *rules_ret) {
    vec *rules_for_event = get_rules_for_event(mode, event);
    if (!rules_for_event)
        pieprf;
    int i, l;
    for (i = 0, l = vec_size(rules_for_event); i < l; i++) {
        // ok to cast NULL
        struct button_rule_t *rule = (struct button_rule_t *) vec_get(rules_for_event, i);
        if (! rule) 
            pieprf;

        short rule_buttons = rule->buttons;
        if ((read & rule_buttons) == rule_buttons) {
            // got it.
            if ( !vec_add(rules_ret, rule)) 
                pieprf;
        }
    }
    return true;
}

bool buttons_init() {
    g.rules = vec_new();

    short modes = mode_get_num_modes();

    for (int i = 0; i < modes; i++) {
        vec *rules_for_mode = vec_new();
        vec_add(g.rules, rules_for_mode);

        for (int event = 0; event < 2; event++) {
            vec *rules_for_event = vec_new();
            vec_add(rules_for_mode, rules_for_event);
        }
    }

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

bool buttons_get_rules_press(short mode, short read, vec *rules_ret) {
    return get_rules_for_read(mode, BUTTONS_PRESS, read, rules_ret);
}

bool buttons_get_rules_release(short mode, short read, vec *rules_ret) {
    return get_rules_for_read(mode, BUTTONS_RELEASE, read, rules_ret);
}

bool buttons_cleanup() {
    // XX
    bool ok = true;
    return ok;
}
