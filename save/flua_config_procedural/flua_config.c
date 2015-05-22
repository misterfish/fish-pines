#include <lua.h>
#include <lauxlib.h>
#include <glib.h>

#include <string.h>

#include <fish-util.h>
#include <fish-utils.h>

#include "flua_config.h"

/* Simple way to load values from a lua table into C data structure.
 *
 * Uses global state, so no threading / re-entrancy.
 *
 * String indices of the hashes are all static,
 * i.e., don't free.
 */

static struct {
    GHashTable *conf;
    GHashTable *required_keys;
    char *namespace;
    lua_State *L;
} g;

static bool required_key_set(gpointer key);
static void got_required_key(gpointer key);
static GList *get_required_keys();

static void conf_value_destroy(gpointer value);
static void required_keys_value_destroy(gpointer value);

void flua_config_init(lua_State *l) {
    if (g.conf) 
        g_hash_table_destroy(g.conf);
    if (g.required_keys) 
        g_hash_table_destroy(g.required_keys);

    g.conf = g_hash_table_new_full(
        g_str_hash,
        g_str_equal,
        NULL, // key destroy
        conf_value_destroy
    );
    g.required_keys = g_hash_table_new_full(
        g_str_hash,
        g_str_equal,
        NULL, // key destroy
        required_keys_value_destroy
    );

    g.L = l;
}

/* namespace is optional (gives nicer verbose messages).
 */
void flua_config_set_namespace(char *ns) {
    g.namespace = ns;
}

bool flua_config_load_config_f(int flags) {
    bool quiet = flags & FLUA_CONFIG_QUIET;
    bool verbose = flags & FLUA_CONFIG_VERBOSE;

    bool first = true;
    //luaH_checktable(g.L, -1); XX
    lua_pushnil(g.L); // first key
    while (
        first ? (first = false) : lua_pop(g.L, 1), // pop val, keep key for iter
        lua_next(g.L, -2) != 0) 
    { // puts key -> -2, val -> -1
        const char *key; 
        const char *luatype = lua_typename(g.L, lua_type(g.L, -2));
        if (strcmp(luatype, "string")) {
            size_t s;
            char *as_str = "";
            const char *try = lua_tolstring(g.L, -2, &s); // NULL unless number or string
            if (try) {
                _();
                BR(try);
                spr(" (%s)", _s);
                as_str = _t;
            }

            warn("Non-string key val passed in config%s, ignoring.", as_str);
            continue;
        }
        key = luaL_checkstring(g.L, -2);

        gpointer lookup_ptr = g_hash_table_lookup(g.conf, (gpointer) key);
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
            const char *val = luaL_checkstring(g.L, -1);
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
            lua_Number val = luaL_checknumber(g.L, -1);
            *(int *) lookup->value = (int) val;
            if (verbose) {
                spr("%d", (int) val); // _t
                G(_t); // _u
                M("int"); // _v
            }
        }
        else if (!strcmp(type, "double")) {
            lua_Number val = luaL_checknumber(g.L, -1);
            *(double *) lookup->value = val;
            if (verbose) {
                spr("%f", val); // _t
                G(_t); // _u
                M("double"); // _v
            }
        }
        else if (!strcmp(type, "bool")) {

            // doesn't throw
            bool val = lua_toboolean(g.L, -1);
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

        if (verbose) {
            if (g.namespace) {
                M(g.namespace);
                spr("| %s » ", _w);
            }
            else {
                spr("");
                spr("");
            }

            info("flua_config: setting %s%s = %s (%s)", _x, _s, _u, _v); 
        }
    }

    bool ok = true;

    GList *req = get_required_keys();
    while (req) {
        gpointer req_key = req->data;
        if (! required_key_set(req_key)) {
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
    g_hash_table_insert(g.conf, key, val);
}

void flua_config_add_required_key(gpointer key) {
    g_hash_table_insert(g.required_keys, key, NULL);
}

static void got_required_key(gpointer key) {
    g_hash_table_replace(g.required_keys, key, GINT_TO_POINTER(1));
}

static bool required_key_set(gpointer key) {
    return g_hash_table_lookup(g.required_keys, key);
}

static GList *get_required_keys() {
    return g_hash_table_get_keys(g.required_keys);
}

gpointer flua_config_get(gpointer key) {
    return ((struct conf_t *) g_hash_table_lookup(g.conf, key))->value;
}

static void conf_value_destroy(gpointer value) {
    /* val is a malloc'd struct conf_t *
     */
    info("destroying conf value");
    free(value);
}

static void required_keys_value_destroy(gpointer value) {
    /* val is a malloc'd integer-as-pointer.
     */
info("destroying req key value");
    free(value);
}



