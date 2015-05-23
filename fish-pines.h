#ifndef __INCL_FISH_PINES_H
#define __INCL_FISH_PINES_H

#include <glib.h>

void main_set_mode(int mode);
void main_register_loop_event(char *desc, int count, bool (*cb)(void *data));
bool main_fire_event(char *event, gpointer data);

#endif
