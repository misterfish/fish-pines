#define _GNU_SOURCE

#include <math.h>

#include <glib.h>

#include <lua.h>
#include <lauxlib.h> // luaL_ functions..h>

#include "const.h"
#include "mode.h"

#include "buttons.h"

#define BUTTONS_PRESS 0
#define BUTTONS_RELEASE 1

#define DEFAULT_EXACT   true
#define DEFAULT_ONCE    false
#define DEFAULT_CHAIN   true
#define DEFAULT_HOLD_INDICATOR   true

#define btn_name_to_bit(btn_name, bitmask) { \
    if (! strcmp(btn_name, "a")) \
        bitmask |= N_A; \
    else if (! strcmp(btn_name, "b")) \
        bitmask |= N_B; \
    else if (! strcmp(btn_name, "select")) \
        bitmask |= N_SELECT; \
    else if (! strcmp(btn_name, "start")) \
        bitmask |= N_START; \
    else if (! strcmp(btn_name, "up")) \
        bitmask |= N_UP; \
    else if (! strcmp(btn_name, "down")) \
        bitmask |= N_DOWN; \
    else if (! strcmp(btn_name, "left")) \
        bitmask |= N_LEFT; \
    else if (! strcmp(btn_name, "right")) \
        bitmask |= N_RIGHT; \
}

/* vector of vector of vectors.
 * [ mode => [ {_PRESS|_RELEASE} => [ button_rule_t *rule, ...] ] ]
 */
static struct {
    vec *rules;
    // simple implementation: one exact combinatino gets blocked.
    short blocked_btns;
} g;

/* Return pointer to our global vector-in-a-vector. */
static vec *get_rules_for_event(short mode, short event);

/* Fill in the given vector. */
static bool get_rules_for_read(short mode, short event, short read, vec *rules_ret);

/* Check for stack consistency -- pop after check ? XX */

/* Throws.
 */
int buttons_add_rule_l(lua_State *L) {
    lua_pushnil(L); // init iter
    short buttons = 0;
    short mode = -1;
    short event = -1;

    struct button_rule_t *rule = f_mallocv(*rule);
    memset(rule, '\0', sizeof *rule);
    rule->exact = DEFAULT_EXACT;
    rule->once = DEFAULT_ONCE;
    rule->chain = DEFAULT_CHAIN;
    rule->hold_indicator = DEFAULT_HOLD_INDICATOR;

    while (lua_next(L, -2)) {
        const char *luatype = lua_typename(L, lua_type(L, -2));
        // plain table entry, i.e. button name.
        if (! strcmp(luatype, "number")) {
            const char *value = luaL_checkstring(L, -1);
            btn_name_to_bit(value, buttons);

        }
        else {
            const char *key = luaL_checkstring(L, -2);
            if (! strcmp(key, "once")) {
                const bool value = lua_toboolean(L, -1);
                rule->once = value;
            }
            else if (! strcmp(key, "chain")) {
                const bool value = lua_toboolean(L, -1);
                rule->chain = value;
            }
            else if (! strcmp(key, "exact")) {
                const bool value = lua_toboolean(L, -1);
                rule->exact = value;
            }
            else if (! strcmp(key, "hold_indicator")) {
                const bool value = lua_toboolean(L, -1);
                rule->hold_indicator = value;
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
                    lua_pushstring(L, "Unknown event type");
                    lua_error(L);
                }
            }
            else if (! strcmp(key, "time_block")) {
                rule->has_time_block = true;
                lua_pushnil(L); // init iter
                while (lua_next(L, -2)) {
                    const char *value = luaL_checkstring(L, -2);
                    if (! strcmp(value, "target")) {
                        lua_pushnil(L); // init iter
                        while (lua_next(L, -2)) {
                            const char *btn = luaL_checkstring(L, -1);
                            btn_name_to_bit(btn, rule->time_block_target);
                            lua_pop(L, 1);
                        }
                    }
                    else if (! strcmp(value, "timeout")) {
                        lua_Number numval = luaL_checknumber(L, -1);
                        rule->time_block_timeout = (double) numval;
                    }
                    lua_pop(L, 1);
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
        /* Disallow combinations for release events. We do it by checking
         * that the button flag is a pure power of 2 (probably a faster
         * way).
         *
         * Also disallow setting of 'once', 'exact', 'hold_indicator', or 'chain'
         * (specifically, disallow setting it to a value other than the
         * default).
         */
        if (event == BUTTONS_RELEASE) {
            float power = log(buttons) / log(2);
            if (power != (int) power) {
                lua_pushstring(L, "Release events can not be applied to combinations.");
                lua_error(L);
            }
            if (rule->once != DEFAULT_ONCE) {
                lua_pushstring(L, "Attribute 'once' not writeable for release events.");
                lua_error(L);
            }
            if (rule->exact != DEFAULT_EXACT) {
                lua_pushstring(L, "Attribute 'exact' not writeable for release events.");
                lua_error(L);
            }
            if (rule->chain != DEFAULT_CHAIN) {
                lua_pushstring(L, "Attribute 'chain' not writeable for release events.");
                lua_error(L);
            }
            if (rule->hold_indicator != DEFAULT_HOLD_INDICATOR) {
                lua_pushstring(L, "Attribute 'hold_indicator' not writeable for release events.");
                lua_error(L);
            }
        }
        lua_pop(L, 1);
    }

    if (mode == -1) {
        lua_pushstring(L, "Need mode for rule.");
        lua_error(L);
    }

    if (event == -1) {
        lua_pushstring(L, "Need event for rule.");
        lua_error(L);
    }

    if (! buttons) {
        lua_pushstring(L, "Need buttons for rule.");
        lua_error(L);
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

        bool exact = rule->exact;
        short rule_buttons = rule->buttons;
        bool trigger = false;
        if (exact)
            /* Has to be exactly this. */
            trigger = read == rule_buttons;
        else
            /* Can be a combination containing us. */
            trigger = (read & rule_buttons) == rule_buttons;
        if (trigger) {
            /* Got it. */
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

    return true;
}

bool buttons_get_rules_press(short mode, short read, vec *rules_ret) {
    return get_rules_for_read(mode, BUTTONS_PRESS, read, rules_ret);
}

bool buttons_get_rules_release(short mode, short read, vec *rules_ret) {
    return get_rules_for_read(mode, BUTTONS_RELEASE, read, rules_ret);
}

void buttons_set_block(short btns) {
    g.blocked_btns = btns;
}

void buttons_remove_block() {
    g.blocked_btns = 0;
}

bool buttons_remove_block_timeout(gpointer data) {
    buttons_remove_block();
    return false;
}

bool buttons_is_blocked(short btns) {
    return g.blocked_btns == btns;
}

bool buttons_cleanup() {
    // XX
    bool ok = true;
    return ok;
}
