#define _GNU_SOURCE

#ifndef __INCL_UTIL_H
#define __INCL_UTIL_H

#include <stdbool.h>
#include <termios.h> // libc6-dev

#define F_UTIL_TERMINAL_MODE_POLLING     0x1
#define F_UTIL_TERMINAL_MODE_BLOCKING    0x2
#define F_UTIL_TERMINAL_MODE_READ_WITH_TIMEOUT   0x3
#define F_UTIL_TERMINAL_MODE_READ_WITH_INTERBYTE_TIMEOUT   0x4

#define conf_s(x) \
    flua_config_get_string(g.conf, #x)
#define conf_b(x) \
    flua_config_get_boolean(g.conf, #x)
#define conf_r(x) \
    flua_config_get_real(g.conf, #x)
#define conf_i(x) \
    flua_config_get_integer(g.conf, #x)

/* Mode is just there for extra checking. Can be 0 to turn it off.
 */
bool f_terminal_raw_input(int mode, int bytes, int poll_tenths_of_a_second);
bool f_terminal_normal();


#endif
