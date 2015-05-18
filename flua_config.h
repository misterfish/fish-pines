#ifndef __INCL_FLUA_CONFIG_H
#define __INCL_FLUA_CONFIG_H

#include <stdbool.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <glib.h>

/* Example usage.
 *
    flua_config_new(global.L, "mpd");

    flua_conf_default(host, char*, CONF_DEFAULT_HOST)
    flua_conf_default(port, int, CONF_DEFAULT_PORT)
    flua_conf_default(play_on_load_playlist, bool, false)
    flua_conf_optional(playlist_path, char*)
    flua_conf_required(my_friend, double)

    // Throws on lua errors, returns false on others (and then we throw).
    if (! flua_config_load_config_f(FLUA_CONFIG_VERBOSE)) {
        _();
        BR("Couldn't load lua config.");
        lua_pushstring(global.L, _s);
        lua_error(global.L);
    }
*/

/* Quiet means don't complain on stderr about e.g. missing keys.
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
fprintf(stderr, "yes it was %s %d.\n", #key_name, item->required); \
    flua_config_add_required_key(#key_name); \
        type_name *storeme = malloc(sizeof(*storeme)); /* XX */ \
    item->value = (gpointer) storeme; \
    flua_config_insert((gpointer) #key_name, (gpointer) item); \
} while (0);

struct conf_t {
    char *key;
    char *type;
    bool required;
    gpointer value;
};

#define flua_config_get_conf(type, key_name) *(type *) (flua_config_get((gpointer) #key_name))

void flua_config_new(lua_State *L, char *nspace);
void flua_config_insert(gpointer key, gpointer val);
gpointer flua_config_get(gpointer key);

/* Throws on lua errors, returns false on others. */
bool flua_config_load_config();
bool flua_config_load_config_f(int flags);
void flua_config_add_required_key(gpointer key);
vec *flua_config_get_required_keys();

#endif
