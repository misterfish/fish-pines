#include <lua.h>
#include <lauxlib.h>
#include <glib.h>

#include <string.h>

#include <fish-util.h>
#include <fish-utils.h>

#include "flua_config.h"

/* String indices of hash and string elements of vector are all static,
 * i.e., don't free.
 */
static GHashTable *conf;
static vec *unknown_keys;
static lua_State *L;

void flua_config_new(lua_State *l) {
    if (conf) 
        g_hash_table_destroy(conf);
    if (unknown_keys) 
        vec_destroy(unknown_keys);

    conf = g_hash_table_new/*_full*/(
        g_str_hash,
        g_str_equal
    );
    unknown_keys = vec_new();

    L = l;
}

bool flua_config_load_config_f(int flags) {
    bool quiet = flags & FLUA_CONFIG_QUIET;
    bool verbose = flags & FLUA_CONFIG_VERBOSE;

    bool first = true;
    //luaH_checktable(L, -1); XX
    lua_pushnil(L); // first key
    while (
        first ? (first = false) : lua_pop(L, 1), // pop val, keep key for iter
        lua_next(L, -2) != 0) { // puts key -> -2, val -> -1
        const char *key; 
        const char *luatype = lua_typename(L, lua_type(L, -2));
        if (strcmp(luatype, "string")) {
            size_t s;
            char *as_str = "";
            const char *try = lua_tolstring(L, -2, &s); // NULL unless number or string
            if (try) {
                _();
                BR(try);
                spr(" (%s)", _s);
                as_str = _t;
            }

            warn("Non-string key val passed in config%s, ignoring.", as_str);
            continue;
        }
        key = luaL_checkstring(L, -2);

        gpointer lookup_ptr = g_hash_table_lookup(conf, (gpointer) key);
        if (! lookup_ptr) {
            _(); \
            BR(key); \
            warn("Unknown config key: %s, ignoring.", _s); 
            continue;
        }

        struct conf_t *lookup = (struct conf_t *) lookup_ptr;

        char *type = lookup->type;

        if (verbose) {
            _();
            Y(key); // _s
        }

        //const char *valtype = lua_typename(L, lua_type(L, -1));
        if (!strcmp(type, "char*")) {
            const char *val = luaL_checkstring(L, -1);
            if (!val) 
                pieprf;
            char *dup = (char*) g_strdup(val); // -O- lua string
            *(char **) lookup->value = dup;
            if (verbose) {
                spr("%s", dup); // _t
                G(_t); // _u
                M("string"); // _v
            }
        }
        else if (!strcmp(type, "int")) {
            lua_Number val = luaL_checknumber(L, -1);
            *(int *) lookup->value = (int) val;
            if (verbose) {
                spr("%d", (int) val); // _t
                G(_t); // _u
                M("int"); // _v
            }
        }
        else if (!strcmp(type, "double")) {
            lua_Number val = luaL_checknumber(L, -1);
            *(double *) lookup->value = val;
            if (verbose) {
                spr("%f", val); // _t
                G(_t); // _u
                M("double"); // _v
            }
        }
        else if (!strcmp(type, "bool")) {

            // doesn't throw
            bool val = lua_toboolean(L, -1);
            *(bool *) lookup->value = val;
            if (verbose) {
                G(val ? "true" : "false"); // _t
                spr("«%s»", _t); // _u
                M("boolean"); // _v
            }
        }
        else 
            piepc;

        if (verbose) {
            info("flua_config: setting (MPDXX) %s = %s (%s)", _s, _u, _v); \
        }
    }

    return true;
}

bool flua_config_load_config() {
    return flua_config_load_config_f(0);
}

/*
void flua_config_set_verbose(bool v) {
    flua_config_Verbose = v;
}
*/
 
void flua_config_insert(gpointer key, gpointer val) {
    g_hash_table_insert(conf, key, val);
}

gpointer flua_config_get(gpointer key) {
    return ((struct conf_t *) g_hash_table_lookup(conf, key))->value;
}



            /* Throwing version.
            char *err = "Non-string key val passed in config%s";
            int errstr_len = as_str_len + strlen(err) - 2 + 1;
            char *errstr = str(errstr_len);
            if (snprintf(errstr, errstr_len, err, as_str) == errstr_len) {
                iwarn("overflow");
                return false;
            }
            lua_pushstring(L, errstr);
            lua_error(L);
            */

