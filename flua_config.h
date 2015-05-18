#ifndef __INCL_FLUA_CONFIG_H
#define __INCL_FLUA_CONFIG_H

#include <stdbool.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <glib.h>

/* Example usage. XX
 *
*/

/* Quiet means don't complain on stderr about e.g. missing keys.
 * Verbose refers to stdout.
 */
#define FLUA_CONFIG_QUIET   0x01
#define FLUA_CONFIG_VERBOSE 0x02

#define flua_conf_last \
{ \
    .key = NULL, \
    .type = NULL, \
    .required = false, \
    .value.real = 0 \
}

#define flua_conf_default(key_name, type_name, dflt)  \
{  \
    .key = #key_name, \
    .type = #type_name, \
    .required = false, \
    .value.type_name = dflt \
},

#define flua_conf_optional(key_name, type_name) \
{  \
    .key = #key_name, \
    .type = #type_name, \
    .required = false, \
    .value.type_name = 0 \
},

#define flua_conf_required(key_name, type_name) \
{  \
    .key = #key_name, \
    .type = #type_name, \
    .required = true, \
    .value.type_name = 0 \
},

struct flua_config_conf_t {
    GHashTable *conf;
    GHashTable *required_keys;
    char *namespace;
    bool verbose; // info on stdout
    bool quiet; // warnings on stderr
    lua_State *L;
};

struct flua_config_conf_item_t {
    char *key;
    char *type;
    bool required;
    //gpointer value;
    union {
        char *string;
        bool boolean;
        int integer;
        double real;
    } value;
};

struct flua_config_conf_t *flua_config_new(lua_State *l);
struct flua_config_conf_t *flua_config_new_f(lua_State *l, int flags);
void flua_config_destroy(struct flua_config_conf_t *conf);

void flua_config_set_namespace(struct flua_config_conf_t *conf, char *ns);
//gpointer flua_config_get(gpointer key);
char *flua_config_get_string(struct flua_config_conf_t *conf, gpointer key);
bool flua_config_get_boolean(struct flua_config_conf_t *conf, gpointer key);
int flua_config_get_integer(struct flua_config_conf_t *conf, gpointer key);
double flua_config_get_real(struct flua_config_conf_t *conf, gpointer key);

/* Throws on lua errors, returns false on others. */
bool flua_config_load_config(struct flua_config_conf_t *conf, struct flua_config_conf_item_t confary[], int num_rules);

void flua_config_set_verbose(bool v);
void flua_config_set_quiet(bool q);

#endif

