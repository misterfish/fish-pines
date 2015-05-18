#ifndef __INCL_FISH_PINES_H
#define __INCL_FISH_PINES_H
void main_set_mode(int mode);
void main_register_loop_event(char *desc, int count, bool (*cb)(void *data));

#endif
