#include <lua.h>
#include <lauxlib.h>
#include <glib.h>

#include <string.h>

#include <fish-util.h>
#include <fish-utils.h>

#include "flua_config.h"

/* String indices of hash are all static,
 * i.e., don't free.
 */
static GHashTable *conf;
static GHashTable *required_keys;

static lua_State *L;

static bool required_key_set(gpointer key);
static void got_required_key(gpointer key);
static GList *get_required_keys();

void flua_config_new(lua_State *l) {
    if (conf) 
        g_hash_table_destroy(conf);
    if (required_keys) 
        g_hash_table_destroy(required_keys);

    conf = g_hash_table_new/*_full*/(
        g_str_hash,
        g_str_equal
    );
    required_keys = g_hash_table_new/*_full*/(
        g_str_hash,
        g_str_equal
    );

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
        lua_next(L, -2) != 0) 
    { // puts key -> -2, val -> -1
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

        if (lookup->required)  {
fprintf(stderr, "yes it was.\n");
            got_required_key((gpointer) key);
        }

        if (verbose) 
            info("flua_config: setting (MPDXX) %s = %s (%s)", _s, _u, _v); 
    }

    bool ok = true;

    GList *req = get_required_keys();
    while (req) {
        gpointer req_key = req->data;
        if (! g_hash_table_lookup(required_keys, req_key)) {
            ok = false;
            _();
            BR((char *) req_key);
            warn("Missing required key: %s.", _s);
        }

        req = req->next;
    }
    g_list_free(req);

    return ok;
}

bool flua_config_load_config() {
    return flua_config_load_config_f(0);
}

void flua_config_insert(gpointer key, gpointer val) {
    g_hash_table_insert(conf, key, val);
}

void flua_config_add_required_key(gpointer key) {
    g_hash_table_insert(required_keys, key, NULL);
}

static void got_required_key(gpointer key) {
    g_hash_table_replace(required_keys, key, GINT_TO_POINTER(1));
}

static bool required_key_set(gpointer key) {
    return ! g_hash_table_lookup(required_keys, key);
}

static GList *get_required_keys() {
    return g_hash_table_get_keys(required_keys);
}

gpointer flua_config_get(gpointer key) {
    return ((struct conf_t *) g_hash_table_lookup(conf, key))->value;
}



