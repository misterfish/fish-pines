#define _GNU_SOURCE

#include <lua.h>
#include <lauxlib.h>

#include <fish-util.h>
#include <fish-utils.h>

#include "const.h"
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
    int num_modes;
    int cur_mode_idx;
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
    g.modes = vec_new();
    g.cur_mode_idx = -1;

    return true;
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
