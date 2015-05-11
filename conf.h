#ifndef __INCL_CONF_H
#define __INCL_CONF_H

#define LED_ORANGE 12
#define LED_BLUE 10

#define LED_MODE LED_ORANGE
#define LED_RANDOM LED_BLUE
#define LED_REMAKE_PLAYLIST LED_BLUE

//#define SHUTDOWN_HOLD_SECS 2
#define SHUTDOWN_HOLD_SECS 0

/* Switch down and up keys for volume control.
 * Useful for bugging the bartender.
 */
#define ANTON_MODE true

/* Need generic config XX
 */
static const char *PL_PATH = "mpd-playlists";
static const char *MPD_PLAYLIST_DIR = "mpd-playlists";
static const char *LEDD_SOCK = "/tmp/.ledd-socket";
static const char *VOLD_SOCK = "/tmp/.vold-simple-socket";
static const char *FISH_VOL_SOCK = "%s/.local/share/fish-vol/socket";

#endif
