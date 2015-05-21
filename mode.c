#define _GNU_SOURCE

#include <lua.h>
#include <lauxlib.h>

#include <fish-util.h>
#include <fish-utils.h>

#include "const.h"
#include "global.h"
#include "util.h"
#include "flua_config.h"

#include "mode.h"

#define CONF_NAMESPACE "mode"

static struct flua_config_conf_item_t CONF[] = {
    flua_conf_required(modes, stringlist)
    flua_conf_optional(fun, integerlist)
    flua_conf_optional(funt, booleanlist)
    flua_conf_optional(funr, reallist)
    flua_conf_last
};

static struct {
    bool lua_initted;
    struct flua_config_conf_t *conf;

    vec *modes;
    short num_modes;
    short cur_mode_idx;
    struct mode_t *cur_mode;
} g;

bool mode_init_config() {
    g.conf = flua_config_new(global.L);
    if (!g.conf)
        pieprf;
    flua_config_set_namespace(g.conf, CONF_NAMESPACE);
    return true;
}

int mode_configl() {
    int num_rules = (sizeof CONF) / (sizeof CONF[0]) - 1;

    /* Throws. 
     */
    if (! flua_config_load_config(g.conf, CONF, num_rules)) {
        _();
        BR("Couldn't load lua config.");
        lua_pushstring(global.L, _s);
        lua_error(global.L);
    }
    g.lua_initted = true;

    return 0;
}

bool mode_init() {
    if (! g.lua_initted) {
        warn("%s: forgot lua init?", CONF_NAMESPACE);
        return false;
    }

    short num_modes;
    vec *mode_names = conf_sl(modes);
    bool ok = false;
    if (mode_names) {
        num_modes = (short) vec_size(mode_names);
        if (num_modes) 
            ok = true;
    }

    if (!ok) {
        warn("No modes.");
        return false;
    }

    g.modes = vec_new();
    g.num_modes = num_modes;
    for (int i = 0; i < num_modes; i++) {
        char *name = (char *) vec_get(mode_names, i);
        struct mode_t *mode = f_mallocv(*mode);
        memset(mode, '\0', sizeof *mode);
        // No dup. Assume conf doesn't get cleaned up.
        mode->name = name;
        vec_add(g.modes, mode);
    }

    return mode_set_mode(0);
}

short mode_get_num_modes() {
    return g.num_modes;
}

char *mode_get_mode_name() {
    struct mode_t *cur_mode = (struct mode_t *) vec_get(g.modes, g.cur_mode_idx);
    return cur_mode->name;
}

short mode_get_mode() {
    return g.cur_mode_idx;
}

bool mode_set_mode(short s) {
    if (s < 0 || s >= g.num_modes) {
        iwarn("Bad mode");
        return false;
    }
    g.cur_mode_idx = s;
    g.cur_mode = (struct mode_t *) vec_get(g.modes, s);
    return true;
}

bool mode_next_mode() {
    short s = g.cur_mode_idx;
    s = ++s % g.num_modes;
    return mode_set_mode(s);
}

int mode_next_model() {
    if (!mode_next_mode()) {
        lua_pushstring(global.L, "Couldn't switch to next mode.");
        lua_error(global.L);
    }
    return 0;
}

int mode_get_mode_namel() {
    char *mode = mode_get_mode_name();
    lua_pushstring(global.L, mode);
    return 1;
}

// XX
bool mode_cleanup() {
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
