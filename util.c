#define _BSD_SOURCE // cfmakeraw
#include <unistd.h> // _exit

#include <lua.h>
//#include <lauxlib.h>

#include <fish-util.h>

#include "util.h"

struct termios save_attr_cooked;
bool saved;

bool f_terminal_raw_input(int mode, int bytes, int poll_tenths_of_a_second) {
    // gcc complains about missing initializer like this XX
    struct termios term = {0};
    int fd = 0;
    if (tcgetattr(fd, &term)) {
        warn_perr("Couldn't get terminal attributes.");
        return false;
    }

    memcpy(&save_attr_cooked, &term, sizeof(term));
    saved = true;

    cfmakeraw(&term);
    term.c_lflag |= ISIG; // so Ctl-c works

    /* man termios.
     */

    /* Keep output normal.  */
    term.c_oflag |= OPOST;

    if (mode == 0) {
        // no check
    }
    else if (mode == F_UTIL_TERMINAL_MODE_POLLING) {
        if (bytes) 
            pieprf;
        if (poll_tenths_of_a_second)
            pieprf;
    }
    else if (mode == F_UTIL_TERMINAL_MODE_BLOCKING){
        if (!bytes)
            pieprf;
        if (poll_tenths_of_a_second)
            pieprf;
    }
    else if (mode == F_UTIL_TERMINAL_MODE_READ_WITH_TIMEOUT) {
        if (bytes)
            pieprf;
        if (!poll_tenths_of_a_second)
            pieprf;
    }
    else if (mode == F_UTIL_TERMINAL_MODE_READ_WITH_INTERBYTE_TIMEOUT) {
        if (!bytes) 
            pieprf;
        if (!poll_tenths_of_a_second)
            pieprf;
    }
    term.c_cc[VMIN] = bytes;
    term.c_cc[VTIME] = poll_tenths_of_a_second;

    /* Also means partial success, not really ideal.
     */
    int failure = tcsetattr(fd, 0, &term);
    return !(bool)failure;

}

/* Reverse raw.
 */
bool f_terminal_normal() {
    /* Was never made raw.
     */
    if (!saved)
        return true;

    /*
     * man tcsetattr:
     *
     * raw means 
     * termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON); 
     * termios_p->c_oflag &= ~OPOST; 
     * termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN); 
     * termios_p->c_cflag &= ~(CSIZE | PARENB); 
     * termios_p->c_cflag |= CS8;
     *
     * But it's easier to just restore the saved state.
     */

    int fd = 0;

    /* Returns success if ANY of the changes could be carried out. Not
     * really ideal.
     */
    int failure = tcsetattr(fd, TCSANOW, &save_attr_cooked);
    if (failure) {
        warn_perr("Failure setting fd %d to normal", fd);
    }
    return !(bool)failure;
}

/* Error handler function (last arg to pcall) has already been called, if
 * there is one.
 *
 * Dies immediately on OOM.
 */

void check_lua_err(int rc, char *format, ...) {
    int SIZE = 100;
    char buf[SIZE];
    va_list arglist;
    va_start(arglist, format);
    if (SIZE == vsnprintf(buf, SIZE, format, arglist)) 
        fprintf(stderr, "(warning truncated)");
    va_end(arglist);

    if (rc == LUA_ERRMEM) {
        fprintf(stderr, "Out of memory!");
        _exit(1);
    }
    else if (rc == LUA_ERRERR) {
        warn(buf);
        warn("Also, error running lua error handler.");
    }
    // lua runtime error
    else if (rc == LUA_ERRRUN) {
        warn(buf);
    }
    else {
        piep;
    }
}


