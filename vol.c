#define _GNU_SOURCE

#include <stdlib.h>
#include <fish-util.h>

#include "const.h"
#include "global.h"
#include "vol.h"

static struct {
    char *fish_vol_sock;
} g;

bool vol_init() {
    /* This will end up sending a message to a random socket if someone
     * messes with our env. 
     * But if they can do we have bigger problems.
     */
    char *home = /*secure_*/getenv("HOME");
    int s1 = strnlen(home, 50);
    int s2 = strnlen(FISH_VOL_SOCK, 200);
    if (s2 == 200) {
        warn("FISH_VOL_SOCK too long");
        return false;
    }
    if (s1 == 50) {
        warn("ENV{HOME} too long");
        return false;
    }
    g.fish_vol_sock = f_malloc(sizeof(char) * (s1 + s2));
    sprintf(g.fish_vol_sock, FISH_VOL_SOCK, home);
    return true;
}

bool vol_up() {
    if (!f_socket_unix_message(g.fish_vol_sock, "rel all all +5")) 
        if (!f_socket_unix_message(VOLD_SOCK, "up")) 
            pieprf;
    return true;
}

bool vol_down() {
    if (!f_socket_unix_message(g.fish_vol_sock, "rel all all -5")) 
        if (!f_socket_unix_message(VOLD_SOCK, "down")) 
            pieprf;
    return true;
}
