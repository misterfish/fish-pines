#define _GNU_SOURCE

#ifndef __UTIL_H
#define __UTIL_H

#include <stdbool.h>
#include <termios.h> // libc6-dev

#define F_UTIL_TERMINAL_MODE_POLLING     0x1
#define F_UTIL_TERMINAL_MODE_BLOCKING    0x2
#define F_UTIL_TERMINAL_MODE_READ_WITH_TIMEOUT   0x3
#define F_UTIL_TERMINAL_MODE_READ_WITH_INTERBYTE_TIMEOUT   0x4

/* Mode is just there for extra checking. Can be 0 to turn it off.
 */
bool f_terminal_raw_input(int mode, int bytes, int poll_tenths_of_a_second);
bool f_terminal_normal();


#endif
