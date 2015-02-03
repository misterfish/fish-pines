#include <fish-util.h>
#include "conf.h"
#include "vol.h"

bool vol_up() {
    if (!socket_unix_message(VOLD_SOCK, "up")) 
        pieprf;
    return true;
}

bool vol_down() {
    if (!socket_unix_message(VOLD_SOCK, "down")) 
        pieprf;
    return true;
}
