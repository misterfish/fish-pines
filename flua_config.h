#ifndef __INCL_FLUA_CONFIG_H
#define __INCL_FLUA_CONFIG_H

#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>

/* Example usage.
 *
 * The vars must be called 'key' and 'val' and have the types below.
 *
 *
bool f_mpd_config(const char *key, void *val) {
    flua_config_start
    flua_config_line(conf, host, string)
    flua_config_line(conf, port, int)
    flua_config_line(conf, timeout_ms, int)

    return true;
}
*/

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
    conf_struct.key_name = (double) *(double*) val; \
    if (flua_config_Verbose) { \
        flua_config_inform(conf_struct, key_name, double, "%f"); \
    } \
} while (0);

#define flua_config_inform(conf_struct, key_name, type, format) do { \
    _(); \
    Y(#conf_struct); \
    Y(#key_name); \
    spr(format, conf_struct.key_name); \
    G(_u); \
    M(#type); \
    info("flua_config: setting %s.%s = %s (%s)", _s, _t, _v, _w); \
} while (0);

#define flua_config_start if (false) {} 

#define flua_stringify(x) #x

/* Allowed types: string, int, double, bool.
 * Without quotes!
 */

#define flua_config_line(conf_struct, key_name, type) \
    else if (!strcmp(key, #key_name)) { \
        flua_config_set_##type(conf, key_name);  \
    } 

bool flua_config(lua_State *L, bool (*config_cb)(const char *key, void *val));

bool flua_config_Verbose;
void flua_config_set_verbose(bool v);

#endif
