#include <fish-util.h>

#include "util.h"

bool f_terminal_raw_input(int mode, int bytes, int poll_tenths_of_a_second) {
    struct termios term = {0};
    int fd = 0;
    tcgetattr(fd, &term);
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

    // also means partial success
    int failure = tcsetattr(fd, 0, &term);
    return !(bool)failure;

}

// Reverse raw. Some guesswork, seems to work.
bool f_terminal_normal() {
/*
    termios_p->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    termios_p->c_oflag &= ~OPOST;
    termios_p->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    termios_p->c_cflag &= ~(CSIZE | PARENB);
    termios_p->c_cflag |= CS8;
*/

    int failure = 0;
    for (int fd = 0; fd < 1; fd++) { // only stdin actually
        struct termios term = {0};
        tcgetattr(fd, &term);
        term.c_iflag |= (IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | ICRNL | IXON); // dump IGNCR
        // OPOST was left untouched.
        term.c_lflag |= (ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        term.c_cflag |= (CSIZE | PARENB);
        //term.c_cflag &= ~CS8;
        // no failure also means partial success
        int _failure = tcsetattr(fd, TCSANOW, &term);
        if (_failure) {
            warnp("Failure setting fd %d to normal", fd);
            failure = 1;
        }
    }
    return !(bool)failure;
}
