#include <lua.h>
#include <lauxlib.h>
#include <glib.h>

#include <string.h>

#include <fish-util.h>
#include <fish-utils.h>

#include "flua_config.h"

/* Simple way to load values from a lua table into C data structure.
 *
 * Not thread-safe.
 *
 * String indices of the hashes are all static,
 * i.e., don't free.
 */

static struct {
    /* Override verbose and quiet of conf_t objects it they are set.
     */
    bool verbose;
    bool quiet;
} g;

static bool required_key_set(struct flua_config_conf_t *conf, gpointer key);
static void got_required_key(struct flua_config_conf_t *conf, gpointer key);
static GList *get_required_keys(struct flua_config_conf_t *conf);
static void flua_config_insert(struct flua_config_conf_t *conf, gpointer key, gpointer val);
static void flua_config_add_required_key(struct flua_config_conf_t *conf, gpointer key);

//static void conf_value_destroy(gpointer value);
static void required_keys_value_destroy(gpointer value);

void flua_config_destroy(struct flua_config_conf_t *conf) {
    g_hash_table_destroy(conf->conf);
    g_hash_table_destroy(conf->required_keys);
    free(conf);
}

struct flua_config_conf_t *flua_config_new(lua_State *l) {
    return flua_config_new_f(l, 0);
}

struct flua_config_conf_t *flua_config_new_f(lua_State *l, int flags) {
    struct flua_config_conf_t *conf = f_mallocv(*conf);
    memset(conf, '\0', sizeof *conf);

    conf->conf = g_hash_table_new_full(
        g_str_hash,
        g_str_equal,
        // keys are static strings: don't destroy.
        NULL,
        // vals are static structs: don't destroy.
        NULL 
    );
    conf->required_keys = g_hash_table_new_full(
        g_str_hash,
        g_str_equal,
        // keys are static strings: don't destroy.
        NULL,
        // vals are malloc'd integers-as-pointers: destroy.
        required_keys_value_destroy
    );

    conf->quiet = flags & FLUA_CONFIG_QUIET;
    conf->verbose = flags & FLUA_CONFIG_VERBOSE;

    conf->L = l;

    return conf;
}

/* namespace is optional (gives nicer verbose messages).
 */
void flua_config_set_namespace(struct flua_config_conf_t *conf, char *ns) {
    conf->namespace = ns;
}

/* Throws on lua errors, returns false on others.
 */

bool flua_config_load_config(struct flua_config_conf_t *conf, struct flua_config_conf_item_t confary[], int num_rules) {
    for (int i = 0; i < num_rules; i++) {
        struct flua_config_conf_item_t *confitem = &confary[i];
        flua_config_insert(conf, (gpointer) confitem->key, (gpointer) confitem); 
        if (confitem->required)
            flua_config_add_required_key(conf, confitem->key);
    }

    if (conf->namespace && (g.verbose || conf->verbose)) {
        _();
        M(conf->namespace);
        info("flua_config: config namespace « %s »", _s);
    }

    bool first = true;
    //luaH_checktable(conf->L, -1); XX
    lua_pushnil(conf->L); // first key
    while (
        first ? (first = false) : lua_pop(conf->L, 1), // pop val, keep key for iter
        lua_next(conf->L, -2) != 0) 
    { // puts key -> -2, val -> -1
        const char *key; 
        const char *luatype = lua_typename(conf->L, lua_type(conf->L, -2));
        if (strcmp(luatype, "string")) {
            size_t s;
            char *as_str = "";
            const char *try = lua_tolstring(conf->L, -2, &s); // NULL unless number or string
            if (try) {
                _();
                BR(try);
                spr(" (%s)", _s);
                as_str = _t;
            }

            if (! conf->quiet && ! g.quiet) 
                warn("Non-string key val passed in config%s, ignoring.", as_str);
            continue;
        }
        key = luaL_checkstring(conf->L, -2);

        gpointer lookup_ptr = g_hash_table_lookup(conf->conf, (gpointer) key);
        if (! lookup_ptr) {
            if (! conf->quiet && ! g.quiet) {
                _(); 
                BR(key); 
                warn("Unknown config key: %s, ignoring.", _s); 
            }
            continue;
        }

        struct flua_config_conf_item_t *lookup = (struct flua_config_conf_item_t *) lookup_ptr;

        char *type = lookup->type;

        if (g.verbose || conf->verbose) {
            _();
            Y(key); // _s
        }

        if (!strcmp(type, "string")) {
            const char *val = luaL_checkstring(conf->L, -1);
            if (!val) 
                pieprf;
            char *dup = (char*) g_strdup(val); // -O- lua string
            lookup->value.string = dup;
            if (g.verbose || conf->verbose) {
                spr("%s", dup); // _t
                G(_t); // _u
                M("string"); // _v
            }
        }
        else if (!strcmp(type, "integer")) {
            lua_Number val = luaL_checknumber(conf->L, -1);
            lookup->value.integer = (int) val;
            if (g.verbose || conf->verbose) {
                spr("%d", (int) val); // _t
                G(_t); // _u
                M("integer"); // _v
            }
        }
        else if (!strcmp(type, "real")) {
            lua_Number val = luaL_checknumber(conf->L, -1);
            lookup->value.real = val;
            if (g.verbose || conf->verbose) {
                spr("%f", val); // _t
                G(_t); // _u
                M("real"); // _v
            }
        }
        else if (!strcmp(type, "boolean")) {

            // doesn't throw
            bool val = lua_toboolean(conf->L, -1);
            lookup->value.boolean = val;
            if (g.verbose || conf->verbose) {
                G(val ? "true" : "false"); // _t
                spr("«%s»", _t); // _u
                M("boolean"); // _v
            }
        }
        else 
            piepc;

        if (lookup->required)  
            got_required_key(conf, (gpointer) key);

        if (g.verbose || conf->verbose) 
            info("flua_config: setting %s = %s (%s)", _s, _u, _v); 
    }

    bool ok = true;

    GList *req = get_required_keys(conf);
    while (req) {
        gpointer req_key = req->data;
        if (! required_key_set(conf, req_key)) {
            ok = false;
            if (! conf->quiet && ! g.quiet) {
                _();
                BR((char *) req_key);
                warn("Missing required key: %s.", _s);
            }
        }

        req = req->next;
    }
    g_list_free(req);

    return ok;
}

static void flua_config_insert(struct flua_config_conf_t *conf, gpointer key, gpointer val) {
    g_hash_table_insert(conf->conf, key, val);
}

static void flua_config_add_required_key(struct flua_config_conf_t *conf, gpointer key) {
    g_hash_table_insert(conf->required_keys, key, NULL);
}

static void got_required_key(struct flua_config_conf_t *conf, gpointer key) {
    g_hash_table_replace(conf->required_keys, key, GINT_TO_POINTER(1));
}

static bool required_key_set(struct flua_config_conf_t *conf, gpointer key) {
    return g_hash_table_lookup(conf->required_keys, key);
}

static GList *get_required_keys(struct flua_config_conf_t *conf) {
    return g_hash_table_get_keys(conf->required_keys);
}

static struct flua_config_conf_item_t *lookup_key(struct flua_config_conf_t *conf, gpointer key) {
    if (! conf) {
        if (! g.quiet) 
            iwarn("Conf object is null");
        return NULL;
    }
    if (! conf->conf) {
        if (! conf->quiet && ! g.quiet) 
            iwarn("Conf->conf object is null (namespace %s)", conf->namespace);
        return NULL;
    }
    gpointer ptr = g_hash_table_lookup(conf->conf, key);
    if (!ptr) {
        if (! conf->quiet && ! g.quiet) {
            _();
            BR(key);
            if (conf->namespace) {
                M(conf->namespace);
                spr(" «namespace %s»", _t);
            }
            iwarn("Couldn't get config value for key %s%s", _s, _u);
        }
        return NULL;
    }
    return (struct flua_config_conf_item_t *) ptr;
}

char *flua_config_get_string(struct flua_config_conf_t *conf, gpointer key) {
    struct flua_config_conf_item_t *item;
    if ((item = lookup_key(conf, key)))
        return item->value.string;
    return NULL;
}

bool flua_config_get_boolean(struct flua_config_conf_t *conf, gpointer key) {
    struct flua_config_conf_item_t *item;
    if ((item = lookup_key(conf, key)))
        return item->value.boolean;
    return false;
}

int flua_config_get_integer(struct flua_config_conf_t *conf, gpointer key) {
    struct flua_config_conf_item_t *item;
    if ((item = lookup_key(conf, key)))
        return item->value.integer;
    return 0;
}

double flua_config_get_real(struct flua_config_conf_t *conf, gpointer key) {
    struct flua_config_conf_item_t *item;
    if ((item = lookup_key(conf, key)))
        return item->value.real;
    return 0;
}

static void required_keys_value_destroy(gpointer value) {
    /* val is a malloc'd integer-as-pointer.
     */
    free(value);
}

void flua_config_set_verbose(bool v) {
    g.verbose = v;
}
void flua_config_set_quiet(bool q) {
    g.quiet = q;
}


