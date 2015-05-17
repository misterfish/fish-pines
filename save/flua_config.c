#include <lua.h>
#include <lauxlib.h>

#include <fish-util.h>

#include "flua_config.h"

bool flua_config(lua_State *L, bool (*config_cb)(const char *, void *)) {
    //luaH_checktable(L, -1); XX
    lua_pushnil(L); // first key
    while (lua_next(L, -2) != 0) { // puts key -> -2, val -> -1
        const char *key; 
        if (strcmp(lua_typename(L, lua_type(L, -2)), "string")) {
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
            return false;
        }
        key = luaL_checkstring(L, -2);
        const char *valtype = lua_typename(L, lua_type(L, -1));
        bool type_ok = false;
        bool ok = false;
        if (!strcmp(valtype, "string")) {
            const char *val = lua_tolstring(L, -1, NULL);
            if (!val) 
                pieprf;
            type_ok = true;
            ok = (*config_cb)(key, &val);
        }
        else if (!strcmp(valtype, "number")) {
            double val = lua_tonumber(L, -1);
            type_ok = true;
            ok = (*config_cb)(key, &val);
        }
        else if (!strcmp(valtype, "boolean")) {
            bool val = lua_toboolean(L, -1);
            type_ok = true;
            ok = (*config_cb)(key, &val);
        }
        else {
            _();
            BR(valtype);
            warn("Can't deal with config val type %s, ignoring.", _s);
        }

        if (type_ok && !ok) {
            _();
            BR(key);
            warn("Couldn't set config val for key %s, ignoring.", _s);
        }
        lua_pop(L, 1); // pop val, keep key for next iter
    }
}

void flua_config_set_verbose(bool v) {
    flua_config_Verbose = v;
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

