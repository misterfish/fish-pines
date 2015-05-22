#ifndef __INCL_MODE_H
#define __INCL_MODE_H

#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>

#include <fish-utils.h>

#include "global.h"

struct mode_t {
    char *name;
};

bool mode_init_config();

bool mode_init();
bool mode_cleanup();

short mode_get_mode();
bool mode_set_mode(short s);
bool mode_next_mode();
char *mode_get_mode_name();

int mode_config_l();
int mode_next_mode_l();
int mode_get_mode_name_l();

short mode_get_num_modes();

#endif
