#ifndef __INCL_MODE_H
#define __INCL_MODE_H

#include <stdbool.h>

#include <lua.h>
#include <lauxlib.h>

#include <fish-utils.h>

#include "global.h"

struct mode_t {
    char *name;
    // leds XX
};

bool mode_init_config();
int mode_configl();

bool mode_init();
bool mode_cleanup();


#endif
