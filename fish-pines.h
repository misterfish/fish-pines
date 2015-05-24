#ifndef __INCL_FISH_PINES_H
#define __INCL_FISH_PINES_H

#include <glib.h>
#include <lua.h>

int main_config_l(lua_State *L);
void main_set_mode(int mode);
void main_register_loop_event(char *desc, int count, bool (*cb)(void *data));
bool main_fire_event(char *event, gpointer data);

#endif
