#define _GNU_SOURCE

#include <lua.h>
#include <lauxlib.h> // luaL_ functions..h>

#include "const.h"
#include "buttons.h"

#define BUTTONS_PRESS 0
#define BUTTONS_RELEASE 1

//#include "mode.h"

/* vector of vector of vectors.
 * outer index is mode. 
 * next vector is indexed by 0/1 (press/release)
 * finally, vector of button_rule_t.
 */
static struct {
    vec *rules;
} g;

static vec *get_rules(short mode, short event);
static struct button_rule_t *get_rule(short mode, short event, short read);

// add checks for combos / single logic.
// and comments. XX

/* Throws.
 */
int buttons_add_rulel() {
    lua_State *L = global.L;
    lua_pushnil(L); // init iter
    short buttons = 0;
    short event = -1;

    struct button_rule_t *rule = f_mallocv(*rule);
    memset(rule, '\0', sizeof *rule);
    while (lua_next(L, -2)) {
        const char *luatype = lua_typename(L, lua_type(L, -2));
        //info("got key type %s", luatype);
        // plain table entry, i.e. button name.
        if (! strcmp(luatype, "number")) {
            const char *value = luaL_checkstring(L, -1);
            if (! strcmp(value, "a")) 
                buttons |= N_A;
            else if (! strcmp(value, "b"))
                buttons += N_B;
            else if (! strcmp(value, "select"))
                buttons += N_SELECT;
            else if (! strcmp(value, "start"))
                buttons += N_START;
            else if (! strcmp(value, "up"))
                buttons += N_UP;
            else if (! strcmp(value, "down"))
                buttons += N_DOWN;
            else if (! strcmp(value, "left"))
                buttons += N_LEFT;
            else if (! strcmp(value, "right"))
                buttons += N_RIGHT;
        }
        else {
            const char *key = luaL_checkstring(L, -2);
            if (! strcmp(key, "kill_multiple")) {
                const bool value = lua_toboolean(L, -1);
                rule->kill_multiple = value;
info("kill: %d", value);
            }
            else if (! strcmp(key, "mode")) {
                const char *value = luaL_checkstring(L, -1);
                info("mode: %s", value);
            }
            else if (! strcmp(key, "event")) {
                const char *value = luaL_checkstring(L, -1);
info("event: %s", value);
                if (! strcmp(value, "press"))
                    event = BUTTONS_PRESS;
                else if (! strcmp(value, "release"))
                    event = BUTTONS_RELEASE;
                else {
                    lua_pushstring(global.L, ("Unknown event type %s."));
                    lua_error(global.L);
                }
            }
            else if (! strcmp(key, "handler")) {
                if (strcmp(lua_typename(L, lua_type(L, -1)), "function")) 
                    piep;
                else {
                    int reg_index = luaL_ref(L, LUA_REGISTRYINDEX); // also pops
                    info("got index %d", reg_index);
                    rule->has_handler = true;
                    rule->handler = reg_index;
                    continue;
                }
            }
        }
        lua_pop(L, 1);
    }

    // XX
short mode = 0;

    if (event == -1) {
        lua_pushstring(global.L, "Need event for rule.");
        lua_error(global.L);
    }

    if (! buttons) {
        lua_pushstring(global.L, "Need buttons for rule.");
        lua_error(global.L);
    }

info("storing buttons %d, event %d", buttons, event);
    rule->buttons = buttons;
    rule->event = event;

    vec *rules = get_rules(mode, event);

    if (! rules) 
        piepr0;
    else 
        vec_add(rules, rule);

    return 0;
}

static vec *get_rules(short mode, short event) {
    vec *rules_for_mode = vec_get(g.rules, mode);
    if (!rules_for_mode) 
        pieprnull;
    vec *rules_for_event = vec_get(rules_for_mode, event);
    if (!rules_for_event) 
        pieprnull;
    return rules_for_event;
}

// XX
#define NUMMODES 2

bool buttons_init() {
    g.rules = vec_new();

    for (int i = 0; i < NUMMODES; i++) {
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

struct button_rule_t *buttons_get_rule_press(short mode, short read) {
    return get_rule(mode, BUTTONS_PRESS, read);
}
struct button_rule_t *buttons_get_rule_release(short mode, short read) {
    return get_rule(mode, BUTTONS_RELEASE, read);
}

static struct button_rule_t *get_rule(short mode, short event, short read) {
    vec *rules = get_rules(mode, event);
    if (!rules)
        pieprnull;
info("getting rule for %d, event %d", read, event);

    int cnt = vec_size(rules);

    /* Stop on first matching rule.
     */
    for (int i = 0; i < cnt; i++) {
        // ok to cast NULL
        struct button_rule_t *rule = (struct button_rule_t *) vec_get(rules, i);
        if (! rule) 
            piepc;

info("rule->event %d", rule->event);

        short rule_buttons = rule->buttons;
info("checking against %d", rule_buttons);
        if ((read & rule_buttons) == rule_buttons) {
            // got it.
            return rule;
        }
    }

    return NULL;
}

bool buttons_cleanup() {
    bool ok = true;
    /*
    if (!vec_destroy_flags(rules_music, VEC_DESTROY_DEEP)) {
        piep;
        ok = false;
    }
    if (!vec_destroy_flags(rules_general, VEC_DESTROY_DEEP)) {
        piep;
        ok = false;
    }
    */
    return ok;
}
