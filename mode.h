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

bool mode_init();
bool mode_cleanup();

short mode_get_mode();
bool mode_set_mode(short s);
bool mode_next_mode();
char *mode_get_mode_name();

int mode_configl();
int mode_next_model();
int mode_get_mode_namel();

#endif
