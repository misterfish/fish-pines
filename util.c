#define _BSD_SOURCE // cfmakeraw
//#define _POSIX_C_SOURCE 199309L // time stuff

#include <unistd.h> // _exit
//#include <time.h>
#include <sys/time.h>

#include <lua.h>
#include <lauxlib.h>

#include <fish-util.h>

#include "global.h"
#include "util.h"

#define SOCKET_LENGTH_DEFAULT 100

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

bool util_get_clock(time_t *secs, suseconds_t *usecs) {

#if 0 // requires extra feature macros, and -lrt.
bool util_get_clock(time_t *secs, long *nanosecs) {
    /*
     *    struct timespec {
     *        time_t   tv_sec;        // seconds
     *        long     tv_nsec;       // nanoseconds
     *    };
     */
    struct timespec ts = {0};

    if (clock_gettime(CLOCK_REALTIME, &ts)) {
        warn_perr("Can't get realtime clock");
        return false;
    }
    *secs = ts.tv_sec;
    *nanosecs = ts.tv_nsec;
#endif

    /*
    struct timeval {
        time_t      tv_sec;     // seconds
        suseconds_t tv_usec;    // microseconds
    };
    */

    struct timeval tv = {0};
    struct timezone *tz = NULL;

    if (gettimeofday(&tv, tz)) {
        warn_perr("Can't call gettimeofday()");
        return false;
    }
    *secs = tv.tv_sec;
    *usecs = tv.tv_usec;

    return true;
}

int util_get_clock_l() {
    time_t secs = {0};
    //long nanosecs;
    suseconds_t usecs = {0};
    if (! util_get_clock(&secs, &usecs)) {
        lua_pushstring(global.L, "Can't get clock.");
        lua_error(global.L);
        return 0;
    }
    lua_pushnumber(global.L, secs);
    lua_pushnumber(global.L, usecs);

    return 2;
}

int util_socket_unix_message() {
    const char *filename = luaL_checkstring(global.L, -2);
    const char *msg = luaL_checkstring(global.L, -1);
    char response[SOCKET_LENGTH_DEFAULT];
    if (! f_socket_unix_message_f(filename, msg, response, SOCKET_LENGTH_DEFAULT)) {
        warn("Couldn't send message to socket.");
        return 0;
    }
    lua_pushstring(global.L, f_strdup(response));
    return 1;
}
