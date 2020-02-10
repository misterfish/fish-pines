#ifndef __INCL_FISH_PINES_H
#define __INCL_FISH_PINES_H

#include <glib.h>
#include <lua.h>

void main_set_mode(int mode);
void main_register_loop_event(char *desc, int count, gboolean (*cb)(void *data));
bool main_fire_event(char *event, gpointer data);

void main_remove_timeout(guint id);
bool main_timeout_is_active(guint id);
guint main_add_timeout(int ms, gpointer timeout_func, gpointer data);
guint main_add_fd_watch(int fd, GIOCondition cond, gpointer func, gpointer data);

#endif
