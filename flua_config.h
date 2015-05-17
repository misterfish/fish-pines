#ifndef __INCL_FLUA_CONFIG_H
#define __INCL_FLUA_CONFIG_H

#include <stdbool.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <glib.h>

/* Example usage. XX
 *
 * The vars must be called 'key' and 'val' and have the types below.
 * Everything is passed unquoted.
 *
 *
bool f_mpd_config(const char *key, void *val) {
    flua_config_start
    flua_config_line(conf, host, string)
    flua_config_line(conf, port, int)
    flua_config_line(conf, timeout_ms, int)
    // returns false if unknown key
    flua_config_unknown 

    return true;
}
*/

/* Quiet means don't complain on stderr about e.g. missine keys.
 * Verbose refers to stdout.
 */
#define FLUA_CONFIG_QUIET   0x01
#define FLUA_CONFIG_VERBOSE 0x02

#define flua_conf_default(key_name, type_name, dflt) do { \
    struct conf_t *item = malloc(sizeof(*item)); /* XX */ \
    memset(item, '\0', sizeof(*item)); \
    item->key = #key_name; \
    item->type = #type_name; \
        type_name *storeme = malloc(sizeof(*storeme)); /* XX */ \
    *storeme = dflt; \
    item->value = (gpointer) storeme; \
    info("stored %s %d %s", #key_name, *storeme, item->type); \
    flua_config_insert((gpointer) #key_name, (gpointer) item); \
} while (0);

#define flua_conf_optional(key_name, type_name) do { \
    struct conf_t *item = malloc(sizeof(*item)); /* XX */ \
    memset(item, '\0', sizeof(*item)); \
    item->key = #key_name; \
    item->type = #type_name; \
        type_name *storeme = malloc(sizeof(*storeme)); /* XX */ \
    item->value = (gpointer) storeme; \
    flua_config_insert((gpointer) #key_name, (gpointer) item); \
} while (0);

#define flua_conf_required(key_name, type_name) do { \
    struct conf_t *item = malloc(sizeof(*item)); /* XX */ \
    memset(item, '\0', sizeof(*item)); \
    item->key = #key_name; \
    item->type = #type_name; \
    item->required = true; \
        type_name *storeme = malloc(sizeof(*storeme)); /* XX */ \
    item->value = (gpointer) storeme; \
    flua_config_insert((gpointer) #key_name, (gpointer) item); \
} while (0);

#define gflua_conf_default(key, type, dflt) do { \
    struct conf_##type_t *item = malloc(sizeof(*item)); /* XX */ \
    memset(item, '\0', sizeof(struct *item)); \
    item->key = #key; \
    item->dflt = dflt; \
    g_hash_table_insert(conf, (gpointer) #key, (gpointer) item); \
} while (0);

#define gflua_conf_optional(key, type, dflt) do { \
    struct conf_##type_t *item = malloc(sizeof(*item)); /* XX */ \
    memset(item, '\0', sizeof(struct *item)); \
    item->key = #key; \
    item->required = false; \
    g_hash_table_insert(conf, (gpointer) #key, (gpointer) item); \
} while (0);

#define gflua_conf_required(key, type, dflt) do { \
    struct conf_##type_t *item = malloc(sizeof(*item)); /* XX */ \
    memset(item, '\0', sizeof(struct *item)); \
    item->key = #key; \
    item->required = true; \
    g_hash_table_insert(conf, (gpointer) #key, (gpointer) item); \
} while (0);

/*
struct conf_string_t {
    char *key;
    bool required;
    char *dflt;

    char *value;
};

struct conf_bool_t {
    char *key;
    bool required;
    bool dflt;

    bool value;
};

struct conf_int_t {
    char *key;
    bool required;
    int dflt;

    int value;
};

struct conf_double_t {
    char *key;
    bool required;
    double dflt;

    double value;
};
*/

struct conf_t {
    char *key;
    char *type;
    bool required;
    gpointer value;
};

#define malloc_it(type) f_malloc(sizeof(type))

#define flua_config_get_conf(type, key_name) *(type *) (flua_config_get((gpointer) #key_name))

#if 0
#define flua_config_set_string(conf_struct, key_name) do { \
    char *v = (char*) (*(char**) val); \
    /* XX */ \
    conf_struct.key_name = strndup(v, strlen(v)); /* -O- lua string */ \
    if (flua_config_Verbose) { \
        flua_config_inform(conf_struct, key_name, string, "%s"); \
    } \
} while (0);

#define flua_config_set_int(conf_struct, key_name) do { \
    conf_struct.key_name = (int) *(double*) val; \
    if (flua_config_Verbose) { \
        flua_config_inform(conf_struct, key_name, int, "%d"); \
    } \
} while (0);

#define flua_config_set_double(conf_struct, key_name) do { \
    conf_struct.key_name = *(double*) val; \
    if (flua_config_Verbose) { \
        flua_config_inform(conf_struct, key_name, double, "%f"); \
    } \
} while (0);

#define flua_config_set_boolean(conf_struct, key_name) do { \
    conf_struct.key_name = *(bool*) val; \
    if (flua_config_Verbose) { \
        flua_config_inform(conf_struct, key_name, boolean, "<bool>"); \
    } \
} while (0);

#define flua_config_inform(conf_struct, key_name, type, format) do { \
    _(); \
    Y(#conf_struct); \
    Y(#key_name); \
    if (! strcmp(format, "<bool>")) { \
        G(conf_struct.key_name ? "true" : "false"); \
        spr("«%s»", _u); \
    } \
    else { \
        spr(format, conf_struct.key_name); \
        G(_u); \
    } \
    M(#type); \
    info("flua_config: setting %s.%s = %s (%s)", _s, _t, _v, _w); \
} while (0);

#define flua_config_unknown \
    else { \
        _(); \
        BR(key); \
        warn("Unknown config key: %s.", _s); \
        return false; \
    }

#define flua_config_start if (false) {} 

/* Allowed types: string, int, double, bool.
 */

#define flua_config_line(conf_struct, key_name, type) \
    else if (!strcmp(key, #key_name)) { \
        flua_config_set_##type(conf, key_name);  \
    } 
#endif

void flua_config_new(lua_State *L);
void flua_config_insert(gpointer key, gpointer val);
gpointer flua_config_get(gpointer key);

/* Throws on lua errors, returns false on others. */
bool flua_config_load_config();
bool flua_config_load_config_f(int flags);

#endif
