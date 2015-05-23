#define _BSD_SOURCE // cfmakeraw
//#define _POSIX_C_SOURCE 199309L // time stuff

#include <unistd.h> // _exit
//#include <time.h>
#include <sys/time.h>
#include <error.h>
#include <errno.h>

/* open */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <lua.h>
#include <lauxlib.h>

#include <fish-util.h>

#include "util.h"

#define SOCKET_LENGTH_DEFAULT 100
#define ERROR_BUF_SIZE 500

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
    char buf[ERROR_BUF_SIZE];
    va_list arglist;
    va_start(arglist, format);
    if (ERROR_BUF_SIZE <= vsnprintf(buf, ERROR_BUF_SIZE, format, arglist)) 
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

int util_get_clock_l(lua_State *L) {
    time_t secs = {0};
    //long nanosecs;
    suseconds_t usecs = {0};
    if (! util_get_clock(&secs, &usecs)) {
        lua_pushstring(L, "Can't get clock.");
        lua_error(L);
        return 0;
    }
    lua_pushnumber(L, secs);
    lua_pushnumber(L, usecs);

    return 2;
}

int util_socket_unix_message_l(lua_State *L) {
    const char *filename = luaL_checkstring(L, -2);
    const char *msg = luaL_checkstring(L, -1);
    char response[SOCKET_LENGTH_DEFAULT];
    if (! f_socket_unix_message_f(filename, msg, response, SOCKET_LENGTH_DEFAULT)) {
        warn("Couldn't send message to socket.");
        return 0;
    }
    lua_pushstring(L, f_strdup(response));
    return 1;
}

int util_close_fd_l(lua_State *L) {
    int fd = (int) luaL_checknumber(L, -1);
    if (close(fd)) {
        const char *err = strerror(errno);
        lua_pushstring(L, err);
        lua_error(L);
    }
    return 0;
}

int util_write_fd_to_dev_null_l(lua_State *L) {
    int oldfd = (int) luaL_checknumber(L, -1);
    char mode = 'a'; // doesn't seem to work if it's 'w'
    const char *path = "/dev/null";
    char *errs = "";
    int newfd = open(path, mode);
    int errn = 0;
    if (newfd == -1) {
        errn = errno;
        errs = "open";
        fprintf(stderr, "err1");
        goto ERR;
    }
    if (-1 == dup2(newfd, oldfd)) {
        errn = errno;
        errs = "dup2";
        fprintf(stderr, "err2");
        goto ERR;
    }
    ERR:
    if (errn) {
        char buf[ERROR_BUF_SIZE];
        const char *err = strerror(errn);
        snprintf(buf, ERROR_BUF_SIZE, "Error on %s: %s", errs, err);
        lua_pushstring(L, buf);
        lua_error(L);
    }
    return 0;
}
