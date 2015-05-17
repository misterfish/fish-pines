#define _GNU_SOURCE

#include <string.h> //tmp
#include <stdio.h>
#include <stdlib.h>

/* libmpdclient */
#include <mpd/client.h>

#include <unistd.h> // usleep

#include <fish-util.h>
#include <fish-utils.h>

//#include <fish-pigpio.h> // works also on no_nes but why XX
#include "const.h"
#include "flua_config.h"
#include "global.h"

#include "mpd.h"

static int get_state();
static int get_queue_pos();
static int get_elapsed_time();
static bool load_playlist(int idx);
static bool reload_playlists();

struct {
    struct mpd_connection *connection;
    bool init;
    vec *playlist_vec;
    int playlist_idx;
    int playlist_n;
} g;

struct {
    char *host;
    short port;
    short timeout_ms;
    short timeout_playlist_ms;
    bool play_on_load_playlist;
    
    double my_friend;
} conf;

bool f_mpd_config(const char *key, void *val) {
    flua_config_start
    flua_config_line(conf, host, string)
    flua_config_line(conf, port, int)
    flua_config_line(conf, timeout_ms, int)
    flua_config_line(conf, timeout_playlist_ms, int)
    flua_config_line(conf, my_friend, double)
    flua_config_line(conf, play_on_load_playlist, boolean)
    flua_config_unknown

    return true;
}

// throws
void f_mpd_configl() {
    /* Dies on lua errors, returns false on others.
     */
    flua_config(global.L, f_mpd_config);
}

bool f_error = false;

/* this was in f_mpd_error, and causing an infinite loop. 
        if (!f_mpd_init()) \
            piep; \
            */

#define f_mpd_error(s) do { \
    if (g.connection) { \
        _();                    \
        if (mpd_connection_get_error(g.connection) == MPD_ERROR_SUCCESS) { \
            piep; \
            warn("f_mpd_error called (%s) but mpd says success", s); \
        } \
        else { \
            BR(mpd_connection_get_error_message(g.connection));   \
            warn("Error on %s: %s.", s, _s);      \
        } \
    } \
} while (0)

#define f_check_exit f_check_msg_exit()

#define f_check_msg_exit(s) do { \
    if (!f_mpd_ok()) { \
        f_mpd_error(s); \
        exit(1); \
    } \
} while (0)

#define f_try(expr, msg) do { \
    f_error = false; \
    bool rc = (expr); \
    info("got rc %d", rc); \
    if (!rc || !f_mpd_ok()) { \
        f_error = true; \
        f_mpd_error(msg); \
    } \
} while (0)
 
#define f_try_rf(expr, msg) do { \
    bool rc = (expr); \
    if (!rc || !f_mpd_ok()) { \
        f_mpd_error(msg); \
        return false; \
    } \
} while (0)

#define f_try_rv(expr, msg) do { \
    bool rc = expr; \
    if (!rc || !f_mpd_ok()) { \
        f_mpd_error(msg); \
        return; \
    } \
} while (0)

/* No while wrapper, because it's meant to be in a while. Not sure if it
 * works.
 */
#define f_try_break(s, msg) { \
    bool rc = s; \
    if (!rc || !f_mpd_ok()) { \
        f_mpd_error(msg); \
        break; \
    } \
}; 

/* Use xxx_run_yyy to send and recv.
 * Go into idle to receive updates (e.g. random).
 */

#if 0
#ifdef NO_NES
bool _led_random_on() { 
    info("random led on, teehee"); 
    return true;
}
bool _led_random_off() { 
    info("random led off, teehee"); 
    return true;
}
bool _led_update_random(bool random) {
    return random ? _led_random_on() : _led_random_off();
}
#else

bool _led_random_on() { 
    return led_update_random(true);
}
bool _led_random_off() { 
    return led_update_random(false);
}
bool _led_update_random(bool random) { 
    return led_update_random(random);
}
#endif
#endif

bool f_mpd_ok() {
    if (g.connection) {
        enum mpd_error e = mpd_connection_get_error(g.connection);
        if (e == MPD_ERROR_SUCCESS) 
            return true;
        else 
            return false;
    }
    else {
        warn("No connection.");
        return false;
    }
} 

#define CONF_DEFAULT_HOST "localhost"
#define CONF_DEFAULT_PORT 6601
#define CONF_DEFAULT_TIMEOUT_MS 3000
#define CONF_DEFAULT_TIMEOUT_PLAYLIST_MS 3000

/* Everything has a default value.
 */
static bool check_conf() {
    if (! conf.host) 
        conf.host = CONF_DEFAULT_HOST;
    if (! conf.port) 
        conf.port = CONF_DEFAULT_PORT;
    if (! conf.timeout_ms) 
        conf.timeout_ms = CONF_DEFAULT_TIMEOUT_MS;
    if (! conf.timeout_playlist_ms) 
        conf.timeout_playlist_ms = CONF_DEFAULT_TIMEOUT_PLAYLIST_MS;

    return true;
}

bool f_mpd_init() {
info("using port %d", conf.port);
info("using host %s", conf.host);
if (g.init) 
    mpd_connection_free(g.connection);

    f_try_rf(
        g.connection = mpd_connection_new (conf.host, conf.port, conf.timeout_ms),
        "opening connection"
    );

    //g.connection = mpd_connection_new (conf.host, conf.port, conf.timeout_ms);
    //fprintf(stderr, "connection = %p\n", g.connection);
    //return false;
        //warn("GOT HERE");

if (!g.init) {
    if (! check_conf()) 
        return false;

    g.playlist_vec = vec_new();
    g.playlist_idx = -1;
    g.playlist_n = 0;

    info("MPD connection opened successfully.");

    if (!reload_playlists()) 
        pieprf;

}
    g.init = true;
    return true;
}

bool f_mpd_toggle_play() {
    if (!g.init) return false;
f_mpd_init();

    int status = get_state();
    switch(status) {
        case MPD_STATE_STOP:
            f_try_rf( mpd_run_play(g.connection), "play" );
            break;
        case MPD_STATE_PLAY:
        case MPD_STATE_PAUSE:
            f_try_rf( mpd_run_toggle_pause(g.connection), "toggle pause" );
            break;
        case MPD_STATE_UNKNOWN: 
            iwarn("unknown status, skipping.");
            return false;
            break;
    }
    return true;
}

/* no extra paranoid init? XX */
bool f_mpd_seek(int secs) {
    if (!g.init) 
        return false;

    int pos = get_queue_pos();

    /* Not playing.
     */
    if (pos == -1) 
        return true;

    int time = get_elapsed_time();
    if (pos == -1) 
        pieprf;
    int seek_to = time + secs;

    if (seek_to < 0) 
        return true;
    f_try_rf( mpd_run_seek_pos(g.connection, pos, seek_to), "seek position" ); // ok if overshoot

    return true;
}

bool f_mpd_prev() {
    if (!g.init) return false;
f_mpd_init();

    f_try_rf( mpd_run_previous(g.connection), "run previous song" );
    return true;
}

bool f_mpd_next() {
    if (!g.init) return false;
f_mpd_init();

    f_try_rf( mpd_run_next(g.connection), "run next song" );
    return true;
}

bool f_mpd_cleanup() {
    if (!g.init) 
        return false;

    /* void.
     */
    mpd_connection_free(g.connection);

    if (!vec_destroy_flags(g.playlist_vec, VEC_DESTROY_DEEP))
        pieprf;

    return true;
}

/* - - - */

static struct mpd_status* get_status() {
f_mpd_init();
    struct mpd_status* s = mpd_run_status(g.connection);
    if (s == NULL) {
        _();
        R(mpd_connection_get_error_message(g.connection));
        warn("Couldn't get status: %s", _s);
    }
    return s;
}

static void free_status(struct mpd_status* s) {
    if (s) mpd_status_free(s);
}

static int get_state() {
    struct mpd_status* s = get_status();
    if (s == NULL) return 0;
    enum mpd_state st = mpd_status_get_state(s);
    free_status(s);
    return st;
}

static int get_queue_pos() {
    struct mpd_status* s = get_status();
    if (s == NULL) return -1;
    int ret = mpd_status_get_song_pos(s);
    free_status(s);
    return ret;
}

static int get_elapsed_time() {
    struct mpd_status* s = get_status();
    if (s == NULL) return -1;
    int ret = mpd_status_get_elapsed_time(s);
    free_status(s);
    return ret;
}

// cache states? XX
int get_random() {
    struct mpd_status* s = get_status();
    if (s == NULL) return -1;
    bool ret = mpd_status_get_random(s);
    free_status(s);
    return ret;
}

bool f_mpd_toggle_random() {
    bool random = (bool) get_random();
    return random ? f_mpd_random_off() : f_mpd_random_on();
}

/* Leds actually lit twice, once here, and once in response to event, which
 * lags a bit.
 */
bool f_mpd_random_off() {
    f_try_rf(mpd_run_random(g.connection, false), "random off");
    //return _led_random_off();
    return true;
}

bool f_mpd_random_on() {
    f_try_rf(mpd_run_random(g.connection, true), "random on");
    //return _led_random_on();
    return true;
}

bool f_mpd_update() {
    bool disable_timeout = true; // blocks forever on recv

    /* Enter idle momentarily.
     */
    f_try_rf(mpd_send_idle(g.connection), "enter idle");
    f_try_rf(mpd_send_noidle(g.connection), "enter no idle");
    /* If events occur during idle, this receives them. (But it's too
     * brief).
     * If events occur during no idle, first you enter idle again, then
     * this receives them, too.
     */
    enum mpd_idle res = mpd_recv_idle(g.connection, disable_timeout); 
    if (res) {
        bool reload = false;
        if (res & MPD_IDLE_OPTIONS) {
            bool random = (bool) get_random();
            info("Random changed to %s", random ? "on" : "off");
            //_led_update_random(random);
        }
        if (res & MPD_IDLE_STORED_PLAYLIST) {
            info("Stored playlists have been altered / created, reloading.");
            reload = true;
        }
        if (res & MPD_IDLE_UPDATE) {
            info("Update started or finished, reloading playlists.");
            reload = true;
        }

        if (reload) {
            // warn but don't return false
            if (!reload_playlists()) 
                piep;
        }

        res &= ~(MPD_IDLE_OPTIONS | MPD_IDLE_STORED_PLAYLIST | MPD_IDLE_UPDATE);

        char *str = NULL;
        _();
        if (res & MPD_IDLE_DATABASE) 
            str = "song database has been updated";
        else if (res & MPD_IDLE_PLAYER) 
            str = "the player state has changed: play, stop, pause, seek, ...";
        else if (res & MPD_IDLE_MIXER) 
            str = "the volume has been modified";
        else if (res & MPD_IDLE_OUTPUT) 
            str = "an audio output device has been enabled or disabled";
        /* old version on pi XX
        else if (res & MPD_IDLE_STICKER) 
            str = "a sticker has been modified.";
        else if (res & MPD_IDLE_SUBSCRIPTION) 
            str = "a client has subscribed to or unsubscribed from a channel";
        else if (res & MPD_IDLE_MESSAGE) 
            str = "a message on a subscribed channel was received";
            */
        if (str) {
            BB(str);
            say("");
            info("Mpd event: [%s] (ignoring)", _s);
        }
    }
    return true;
}

bool f_mpd_next_playlist() {
    if (!g.playlist_n) 
        return true;
    if (++g.playlist_idx > g.playlist_n - 1) 
        g.playlist_idx = 0;
    
    return load_playlist(g.playlist_idx);
}

bool f_mpd_prev_playlist() {
    if (!g.playlist_n) 
        return true;
    if (--g.playlist_idx < 0) 
        g.playlist_idx = g.playlist_n - 1; 
    
    return load_playlist(g.playlist_idx);
}

static bool load_playlist(int idx) {
    struct pl *pl = (struct pl *) vec_get(g.playlist_vec, idx);
    if (!pl)
        pieprf;

    f_try_rf(mpd_run_clear(g.connection), "clear playlist (queue)");
    if (!f_mpd_random_off())
        piep; // don't return

    char *name = pl->name;
    char *path = pl->path;
    _();
    G(name);
    Y(path);
    info("Loading playlist [%s -> %s]", _s, _t);

    // void
    mpd_connection_set_timeout(g.connection, conf.timeout_playlist_ms);
    f_try_rf(mpd_run_load(g.connection, path), "load playlist");
    mpd_connection_set_timeout(g.connection, conf.timeout_ms);

    if (conf.play_on_load_playlist) 
        f_try_rf(mpd_run_play(g.connection), "play");


    return true;
}

static bool reload_playlists() {
    vec *v = g.playlist_vec;
    {
        int s = vec_size(v);
        if (s == -1) {
            pieprf;
        }
        else if (s) {
            vec_clear(v);
        }
    }
    g.playlist_idx = -1;
    g.playlist_n = 0;

    f_try_rf(mpd_send_list_meta(g.connection, PL_PATH), "send list meta");

    /* Can receive either pair or entity from mpd. 
     * Entity can give a playlist object, but you can't do too much with it
     * that's interesting. Just get a pair object and gets its ->value.
     */

    char **matches = calloc(2, sizeof(char*));
    if (!matches) 
        ierr_perr("");

    while (1) {
        bool _break = false;
        struct mpd_pair *p = mpd_recv_pair_named(g.connection, "playlist");

        if (!p) {
            // no more pairs
            _break = true;
        }
        else if (!f_mpd_ok()) {
            f_mpd_error("Couldn't receive named pair for playlist");
            _break = true;
        }
        else {
            char *_path = (char*) p->value; // discard const

            char *path = str((strlen(_path) + 1) * sizeof(char));
            f_track_heap(path);
            strcpy(path, _path);

            char *regex_spr = "^ %s / (.+) \\.m3u $";
            char *regex = str(1 + strlen(regex_spr) - 2 + strlen(MPD_PLAYLIST_DIR));
            sprintf(regex, regex_spr, MPD_PLAYLIST_DIR);

            if (!match_matches(path, regex, matches)) {
                iwarn("Got unexpected path: %s", path);
                _break = true;
            }
            free(regex);

            _();
            G(matches[1]);
            info("Got playlist [%s]", _s);

            struct pl *_pl = malloc(sizeof(struct pl));
            if (! _pl)
                ierr_perr("");

            char *name = str(strlen(matches[1]) + 1);
            f_track_heap(name);
            strcpy(name, matches[1]);

            _pl->name = name;
            _pl->path = path;

            if (!vec_add(v, (void*) _pl)) {
                piep;
                _break = true;
            }
        }

        if (p) mpd_return_pair(g.connection, p);

        if (_break) break;
    }

    {
        int s = vec_size(v);
        if (s == -1) 
            pieprf;

        g.playlist_n = s;
    }
    free(matches);
    return true;
}
