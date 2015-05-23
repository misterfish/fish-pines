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
#define conf_sl(x) \
    flua_config_get_stringlist(g.conf, #x)
#define conf_bl(x) \
    flua_config_get_booleanlist(g.conf, #x)
#define conf_rl(x) \
    flua_config_get_reallist(g.conf, #x)
#define conf_il(x) \
    flua_config_get_integerlist(g.conf, #x)

/* Mode is just there for extra checking. Can be 0 to turn it off.
 */
bool f_terminal_raw_input(int mode, int bytes, int poll_tenths_of_a_second);
bool f_terminal_normal();
void check_lua_err(int rc, char *format, ...);

bool util_get_clock(time_t *secs, long *nanosecs);

int util_get_clock_l(lua_State *L);
int util_socket_unix_message_l(lua_State *L);
int util_close_fd_l(lua_State *L);
int util_write_fd_to_dev_null_l(lua_State *L /* char mode */);



#endif
