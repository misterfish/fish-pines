#ifndef __INCL_VOL
#define __INCL_VOL

#include <stdbool.h>

bool vol_init_config();
bool vol_init();
bool vol_up();
bool vol_down();

int vol_config_l(lua_State *L);

#endif
