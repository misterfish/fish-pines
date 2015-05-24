#ifndef __INCL_VOL
#define __INCL_VOL

#include <lua.h>

bool vol_init_config();
bool vol_init();
bool vol_rel(int card, int ctl, int chan, int delta_perc);

int vol_config_l(lua_State *L);
int vol_rel_l(lua_State *L);

#endif
