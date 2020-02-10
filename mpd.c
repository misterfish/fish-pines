#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
/* libmpdclient */
#include <mpd/client.h>

#include <unistd.h> // usleep

#include <fish-util.h>
#include <fish-utils.h>

#include "fish-pines.h"
#include "global.h"
#include "const.h"
#include "flua_config.h"
#include "util.h"
#include "vec-sort.h"

#include "mpd.h"

#define CONF_NAMESPACE "mpd"

#define CONF_DEFAULT_HOST "localhost"
#define CONF_DEFAULT_PORT 6600
#define CONF_DEFAULT_TIMEOUT_MS 3000
#define CONF_DEFAULT_TIMEOUT_PLAYLIST_MS 3000

#define NUM_FAILURES_REINIT 5
#define NUM_SECS_RETRY_RETRY 5

#define ERR_INVALID_ERR     0x01
#define ERR_CONNECTION      0x02
#define ERR_OTHER           0x03

#define TEST_FORCE_REINIT false

static struct flua_config_conf_item_t CONF[] = {
    flua_conf_default(timeout_ms, integer, CONF_DEFAULT_TIMEOUT_MS)
    flua_conf_default(host, string, CONF_DEFAULT_HOST)

    flua_conf_default(verbose, boolean, false)
    flua_conf_default(verbose_events, boolean, false)
    flua_conf_default(port, integer, CONF_DEFAULT_PORT)
    flua_conf_default(timeout_playlist_ms, integer, CONF_DEFAULT_TIMEOUT_PLAYLIST_MS)
    flua_conf_default(play_on_load_playlist, boolean, false)
    flua_conf_optional(playlist_path, string)
    flua_conf_optional(update_ms, integer)

    flua_conf_last
};

static int get_state();
static int get_queue_pos();
static int get_elapsed_time();
static bool load_playlist(int idx);
static bool have_playlists();
static bool reload_playlists();
static struct mpd_status *get_status();
static void free_status(struct mpd_status* s);
static void playlist_by_name_destroy_key(gpointer ptr);

static bool is_even(int i) {
    return ! (i % 2);
}

struct pl {
    char *name;
    char *path;
};

// --- sort pl entries by name.
static int cmp_pl(const void *a, const void *b) {
    return strcmp(
        (* (struct pl **) a)->name,
        (* (struct pl **) b)->name
    );
}

static struct {
    bool verbose;
    bool verbose_events;

    struct flua_config_conf_t *conf;
    bool lua_initted;

    struct mpd_connection *connection;
    bool init;
    vec *playlist_vec;
    GHashTable *playlist_by_name;
    int playlist_idx;
    int playlist_n;

    short failures;
    bool force_reinit;

    bool reinit_timeout_running;
} g;

static int f_mpd_error(char *msg) {
    if (g.connection) {
        enum mpd_error err = mpd_connection_get_error(g.connection);
        if (err == MPD_ERROR_SUCCESS) {
            piep;
            warn("f_mpd_error called (%s) but mpd says success", msg);
            return ERR_INVALID_ERR;
        }
        /* err codes which mean, reinit g.connection */
        else if (
                err == MPD_ERROR_TIMEOUT ||
                err == MPD_ERROR_SYSTEM ||
                err == MPD_ERROR_CLOSED ||
                err == MPD_ERROR_SERVER
                ) {
            return ERR_CONNECTION;
        }
        else
            return ERR_OTHER;
    }
    else {
        iwarn("No connection.");
        return ERR_INVALID_ERR;
    }
}

#define f_try(expr, msg, retn) do { \
    bool rc = (expr); \
    if (! rc || ! f_mpd_ok()) { \
        if (f_mpd_error(msg) == ERR_CONNECTION) { \
            if (! reinit(true, true)) \
                piep; \
        } \
        else if (++g.failures > NUM_FAILURES_REINIT) { \
            if (! reinit(true, false)) \
                piep; \
        } \
        else { \
            _(); \
            BR(mpd_connection_get_error_message(g.connection)); \
            warn("Error on %s: %s.", msg, _s); \
        } \
        return retn; \
    } \
} while (0)

#define f_try_rf(expr, msg) do { \
    f_try(expr, msg, false); \
} while (0)

#define f_try_rnull(expr, msg) do { \
    f_try(expr, msg, NULL); \
} while (0)

/* Glib timeout version. */
static bool reinit_gt(gpointer data) {
    int num_secs = GPOINTER_TO_INT(data);
    if (! f_mpd_init_f(F_MPD_FORCE_INIT)) {
        warn("Still couldn't reinit. Scheduling retry in %d secs", num_secs);
        return true;
    }
    else {
        g.failures = 0;
        g.reinit_timeout_running = false;
        return false;
    }
}

static bool reinit(bool force, bool conn_error) {
    if (force || g.force_reinit) {
        char s[ERROR_BUF_SIZE];
        if (conn_error)
            snprintf(s, ERROR_BUF_SIZE, "Error with connection");
        else {
            _();
            spr("%d", NUM_FAILURES_REINIT);
            BR(_s);
            snprintf(s, ERROR_BUF_SIZE, "Too many mpd failures ( >= %s )", _t);
        }
        warn("%s, reestablishing connection in %d seconds.", s, NUM_SECS_RETRY_RETRY);
        if (! g.reinit_timeout_running) {
            main_add_timeout(NUM_SECS_RETRY_RETRY * 1000, reinit_gt, GINT_TO_POINTER(NUM_SECS_RETRY_RETRY));
            g.reinit_timeout_running = true;
        }
    }
    return true;
}

static gboolean wrap_f_mpd_update() {
    return f_mpd_update();
}

/* Use xxx_run_yyy to send and recv.
 * Go into idle to receive updates (e.g. random).
 */

bool f_mpd_ok() {
    if (g.connection) {
        enum mpd_error e = mpd_connection_get_error(g.connection);
        if (e == MPD_ERROR_SUCCESS)
            return true;
        else if (e == MPD_ERROR_OOM) {
            fprintf(stderr, "Out of memory!");
            _exit(1);
        }
        else
            return false;
    }
    else {
        warn("f_mpd_ok: no connection.");
        return false;
    }
}

bool f_mpd_init_config() {
    g.conf = flua_config_new(global.L);
    if (!g.conf)
        pieprf;
    flua_config_set_namespace(g.conf, CONF_NAMESPACE);
    return true;
}

int f_mpd_config_l(lua_State *L) {
    int num_rules = (sizeof CONF) / (sizeof CONF[0]) - 1;

    /* Throws.
     */
    if (! flua_config_load_config(g.conf, CONF, num_rules)) {
        _();
        BR("Couldn't load lua config.");
        lua_pushstring(L, _s);
        lua_error(L);
    }
    g.lua_initted = true;

    g.verbose = conf_b(verbose);
    g.verbose_events = conf_b(verbose_events);

    return 0;
}

bool f_mpd_init() {
    return f_mpd_init_f(0);
}

bool f_mpd_init_f(short flags) {
    bool force = flags & F_MPD_FORCE_INIT;
    if (g.init) {
        if (force) {
            g.init = false;
            mpd_connection_free(g.connection);
        }
        else {
            warn("f_mpd_init: already initted.");
            return false;
        }
    }
    if (! g.lua_initted) {
        warn("%s: forgot lua init?", CONF_NAMESPACE);
        return false;
    }

    f_try_rf(
        g.connection = mpd_connection_new (conf_s(host), conf_i(port), conf_i(timeout_ms)),
        "opening connection"
    );

    {
        int i = conf_i(update_ms);
        if (i)
            /* This used to be update_wrapper; maybe htat's the cause of
             * the crash when mpd is restarted? XX
             */
            main_register_loop_event("mpd update", i, wrap_f_mpd_update);
    }

    g.playlist_by_name = g_hash_table_new_full(
        g_str_hash,
        g_str_equal,
        /* keys are dup'ed string -- destroy. */
        playlist_by_name_destroy_key,
        /* vals are ints-as-pointers -- do not destroy. */
        NULL
    );
    g.playlist_idx = -1;
    g.playlist_n = 0;

    if (g.verbose)
        info("MPD connection opened successfully.");

    if (have_playlists() && !reload_playlists())
        pieprf;

    g.init = true;

    return true;
}

bool f_mpd_get_random(bool *r) {
    if (!g.init) {
        warn("f_mpd_get_random: mpd not initted.");
        return false;
    }

    struct mpd_status *s = get_status();
    if (!s)
        return false; // let get_status do the whining
    /* can't fail */
    *r = mpd_status_get_random(s);
    free_status(s);
    return true;
}

/* r can be NULL. */
bool f_mpd_toggle_random(bool *r) {
    if (!g.init) {
        warn("f_mpd_toggle_random: mpd not initted.");
        return false;
    }

    bool random;
    if (! f_mpd_get_random(&random))
        return false; // let them do the whining
    bool ok = random ? f_mpd_random_off() : f_mpd_random_on();
    if (ok && r)
        *r = !random;
    return ok;
}

bool f_mpd_random_off() {
    if (!g.init) {
        warn("f_mpd_random_off: mpd not initted.");
        return false;
    }

    f_try_rf(mpd_run_random(g.connection, false), "random off");
    return true;
}

bool f_mpd_random_on() {
    if (!g.init) {
        warn("f_mpd_random_on: mpd not initted.");
        return false;
    }

    f_try_rf(mpd_run_random(g.connection, true), "random on");
    return true;
}

bool f_mpd_toggle_play() {
    if (!g.init) {
        warn("f_mpd_toggle_play: mpd not initted.");
        return false;
    }
if (TEST_FORCE_REINIT) g.force_reinit = true;

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

bool f_mpd_seek(int secs) {
    if (!g.init) {
        warn("f_mpd_seek: mpd not initted.");
        return false;
    }


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
    if (!g.init) {
        warn("f_mpd_prev: mpd not initted.");
        return false;
    }
if (TEST_FORCE_REINIT) g.force_reinit = true;

    f_try_rf( mpd_run_previous(g.connection), "run previous song" );
    return true;
}

bool f_mpd_next() {
    if (!g.init) {
        warn("f_mpd_next: mpd not initted.");
        return false;
    }
if (TEST_FORCE_REINIT) g.force_reinit = true;

    f_try_rf( mpd_run_next(g.connection), "run next song" );
    return true;
}

/* This is *not* the update metadata function of mpd, but our own function
 * for updating the status of mpd.
 *
 * We can die with memory corruption if mpd is restarted. My hunch is that
 * it's here. @todo
 */
bool f_mpd_update() {
    if (!g.init) {
        warn("f_mpd_update: mpd not initted.");
        return false;
    }

    bool disable_timeout = true; // blocks forever on recv

    static bool first = true;
    static bool prev_random;

    /* Enter idle momentarily.
     */
    f_try_rf(mpd_send_idle(g.connection), "enter idle");
    f_try_rf(mpd_send_noidle(g.connection), "enter no idle");

    /* If events occur during idle, this receives them. (But it's too
     * brief).
     * If events occur during no idle, first you enter idle again, then
     * this receives them, too.
     */

    /* Don't use f_try_rf -- this is allowed to be 0. */
    enum mpd_idle res = mpd_recv_idle(g.connection, disable_timeout);

    if (res) {

        vec *strvec;
        if (g.verbose_events)
            strvec = vec_new();

        /* Event names are hard-coded here.
         * Should match EVENTS in main.
         */
        char *reload = NULL;
        if (res & MPD_IDLE_OPTIONS) {
            /* We swallow successive 'random' events if the final state is
             * the same as it was at the last update.
             */
            bool random;
            bool fire = false;
            if (! f_mpd_get_random(&random))
                return false; // let them do the whining
            if (first) {
                first = false;
                fire = true;
            }
            else {
                if (random != prev_random)
                    fire = true;
            }
            prev_random = random;
            if (fire && ! main_fire_event("random", GINT_TO_POINTER(random)))
                pieprf;
            if (g.verbose_events)
                vec_add(strvec, Y_("random changed"));
        }
        if (res & MPD_IDLE_STORED_PLAYLIST) {
            if (g.verbose_events)
                vec_add(strvec, Y_("stored playlists have been altered / created"));
            reload = "playlist";
            if (! main_fire_event("playlists-changed", NULL))
                pieprf;
        }
        if (res & MPD_IDLE_UPDATE) {
            if (g.verbose_events)
                vec_add(strvec, G_("update started or finished"));
            reload = "update";
            if (! main_fire_event("update-started-or-finished", NULL))
                pieprf;
        }

        if (reload) {
            if (g.verbose)
                info("mpd_update: reloading playlists in response to %s event.", reload);

            /* Warn but don't return false. */
            if (have_playlists() && ! reload_playlists())
                piep;
        }

        /* Can cancel bits we don't need (but not doing it).
        res &= ~(MPD_IDLE_OPTIONS | MPD_IDLE_STORED_PLAYLIST | MPD_IDLE_UPDATE);
        */

        if (res & MPD_IDLE_DATABASE) {
            if (g.verbose_events)
                vec_add(strvec, BB_("song database has been updated"));
            if (! main_fire_event("database-updated", NULL))
                pieprf;
        }
        if (res & MPD_IDLE_PLAYER) {
            if (g.verbose_events)
                vec_add(strvec, M_("the player state has changed: play, stop, pause, seek, ..."));
            if (! main_fire_event("player-state-changed", NULL))
                pieprf;
        }
        if (res & MPD_IDLE_MIXER) {
            if (! main_fire_event("volume-altered", NULL))
                pieprf;
            if (g.verbose_events)
                vec_add(strvec, CY_("the volume has been altered"));
        }
        if (res & MPD_IDLE_OUTPUT) {
            if (g.verbose_events)
                vec_add(strvec, CY_("an audio output device has been enabled or disabled"));
            if (! main_fire_event("device-state-changed", NULL))
                pieprf;
        }
        /* Only in the newer version.
        else if (res & MPD_IDLE_STICKER) {
            if (g.verbose_events)
                vec_add(strvec, CY_("a sticker has been modified."));
            if (! main_fire_event("sticker-modified", NULL))
                pieprf;
        }
        else if (res & MPD_IDLE_SUBSCRIPTION) {
            if (g.verbose_events)
                vec_add(strvec, CY_("a client has subscribed to or unsubscribed from a channel"));
            if (! main_fire_event("client-channel-subscription-altered", NULL))
                pieprf;
        }
        else if (res & MPD_IDLE_MESSAGE) {
            if (g.verbose_events)
                vec_add(strvec, CY_("a message on a subscribed channel was received"));
            if (! main_fire_event("subscribed-channel-message-received", NULL))
                pieprf;
        }
        */
        if (g.verbose_events) {
            for (int i = 0, l = vec_size(strvec); i < l; i++) {
                char *str = (char *) vec_get(strvec, i);
                say("");
                info("mpd update: got event %s.", str);
            }
            if (! vec_destroy_deep(strvec))
                pieprf;
        }
    }
    return true;
}

bool f_mpd_database_update() {
    if (!g.init) {
        warn("f_mpd_database_update: mpd not initted.");
        return false;
    }
    const char *path = NULL;
    f_try_rf(mpd_run_update(g.connection, path), "send database update");
    return true;
}

bool f_mpd_is_updating(bool *u) {
    if (!g.init) {
        warn("f_mpd_is_updating: mpd not initted.");
        return false;
    }

    /*
        MPD_IDLE_UPDATE
            a database update has started or finished.

        So it will also return true when it's just finished.
    */

    struct mpd_status *st = get_status();
    if (!st)
        return false; // let get_status do the whining
    if (u)
        *u = mpd_status_get_update_id(st);

    return true;
}

bool f_mpd_next_playlist() {
    if (!g.init) {
        warn("f_mpd_next_playlist: mpd not initted.");
        return false;
    }

    if (!g.playlist_n)
        return true;
    if (++g.playlist_idx > g.playlist_n - 1)
        g.playlist_idx = 0;

    return load_playlist(g.playlist_idx);
}

bool f_mpd_prev_playlist() {
    if (!g.init) {
        warn("f_mpd_prev_playlist: mpd not initted.");
        return false;
    }

    if (!g.playlist_n)
        return true;
    if (--g.playlist_idx < 0)
        g.playlist_idx = g.playlist_n - 1;

    return load_playlist(g.playlist_idx);
}

bool f_mpd_load_playlist_by_name(char *name) {
    if (!g.init) {
        warn("f_mpd_load_playlist: mpd not initted.");
        return false;
    }

    gpointer ptr = g_hash_table_lookup(g.playlist_by_name, (gpointer) name);
    if (!ptr) {
        _();
        BR(name);
        warn("No playlist found with name %s", _s);
        return false; // not overkill, good to trigger lua error.
    }
    return load_playlist(GPOINTER_TO_INT(ptr));
}

static void playlist_by_name_destroy_key(gpointer ptr) {
    free(ptr);
}

bool f_mpd_cleanup() {
    if (!g.init) {
        warn("f_mpd_cleanup: mpd not initted.");
        return false;
    }

    /* void */
    mpd_connection_free(g.connection);

    vec *v = g.playlist_vec;
    if (v) {
        for (int i = 0, l = vec_size(v); i < l; i++) {
            struct pl *p = (struct pl *) vec_get(v, i);
            if (p->name) free(p->name);
            if (p->path) free(p->path);
            free(p);
        }
        vec_destroy(g.playlist_vec);
    }

    g_hash_table_destroy(g.playlist_by_name);

    return true;
}

/* Private functions.
 */

static struct mpd_status *get_status() {
if (TEST_FORCE_REINIT) g.force_reinit = true;
    struct mpd_status* s;
    f_try_rnull(s = mpd_run_status(g.connection), "get status");
    return s;
}

static void free_status(struct mpd_status* s) {
    if (s)
        /* void */
        mpd_status_free(s);
}

static int get_state() {
    struct mpd_status *s = get_status();
    if (!s)
        return 0; // let get_status do the whining
    enum mpd_state st = mpd_status_get_state(s);
    free_status(s);
    return st;
}

static int get_queue_pos() {
    struct mpd_status *s = get_status();
    if (!s)
        return -1; // let get_status do the whining
    int ret = mpd_status_get_song_pos(s);
    free_status(s);
    return ret;
}

/* -1 on failure */
static int get_elapsed_time() {
    struct mpd_status *s = get_status();
    if (!s)
        return -1; // let get_status do the whining
    int ret = mpd_status_get_elapsed_time(s);
    free_status(s);
    return ret;
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
    if (g.verbose)
        info("Loading playlist [%s -> %s]", _s, _t);

    // void
    mpd_connection_set_timeout(g.connection, conf_i(timeout_playlist_ms));
    f_try_rf(mpd_run_load(g.connection, path), "load playlist");
    mpd_connection_set_timeout(g.connection, conf_i(timeout_ms));

    if (conf_b(play_on_load_playlist))
        f_try_rf(mpd_run_play(g.connection), "play");


    return true;
}

static void print_playlists() {
    vec *v = g.playlist_vec;
    if (!v)
        return;
    size_t n = vec_size(v);
    static short line = 0;

    say("");
    info("Got playlists:");

    for (size_t idx = 0; idx < n; idx++) {
        struct pl *_pl = (struct pl *) vec_get(v, idx);
        _();
        is_even(line) ? Y(_pl->name) : CY(_pl->name);
        if (is_even(idx)) {
            BB(get_bullet());
            printf("%s    %s", _t, _s);
        }
        else {
            say(", %s", _s);
            line++;
        }
    }
}

static bool clear_playlist_vec(vec *v, bool deep) {
    int s = vec_size(v);
    short flags = deep ? VEC_CLEAR_DEEP : 0;
    if (s && ! vec_clear_f(v, flags))
        pieprf;
    return true;
}

static bool have_playlists() {
    return conf_s(playlist_path) ? true : false;
}

/* Check have_playlists() before calling this.
 */
static bool reload_playlists() {
    vec *curplvec = g.playlist_vec;
    if (curplvec && !clear_playlist_vec(curplvec, true))
        pieprf;

    vec *plvec = vec_new();
    g.playlist_idx = -1;
    g.playlist_n = 0;

    char *playlist_path = conf_s(playlist_path);

    f_try_rf(mpd_send_list_meta(g.connection, playlist_path), "send list meta");

    /* Can receive either pair or entity from mpd.
     * Entity can give a playlist object, but you can't do too much with it
     * that's interesting. Just get a pair object and gets its ->value.
     */

    char **matches = calloc(2, sizeof(char*));
    if (!matches)
        ierr_perr("");

    int idx = -1;
    while (1) {
        bool _break = false;
        struct mpd_pair *p = mpd_recv_pair_named(g.connection, "playlist");

        /* no more pairs. */
        if (!p)
            _break = true;
        else if (!f_mpd_ok()) {
            f_mpd_error("Couldn't receive named pair for playlist");
            _break = true;
        }
        else {
            bool _err = false;

            char *_path = (char*) p->value; // discard const

            char *path = str((strlen(_path) + 1) * sizeof(char));
            strcpy(path, _path);

            char *regex_spr = "^ %s / (.+) \\.m3u $";
            char *regex = str(1 + strlen(regex_spr) - 2 + strlen(playlist_path));
            sprintf(regex, regex_spr, playlist_path);

            if (! match_matches(path, regex, matches)) {
                iwarn("Got unexpected path: %s", path);
                _err = true;
            }
            free(regex);

            if (_err) {
                free(path);
                _break = true;
                goto ERR;
            }

            struct pl *_pl = f_mallocv(*_pl);
            memset(_pl, 0, sizeof(*_pl));

            char *name = str(strlen(matches[1]) + 1);
            strcpy(name, matches[1]);

            /* Both malloc'd and deep destroyed with vector on cleanup. */
            _pl->name = name;
            _pl->path = path;

            vec_add(plvec, _pl);
            idx++;

            /* Note, this will clobber.*/

            /* If the key exists, _replace will destroy the key and value,
             * while _insert will only destroy the value, i believe .*/
            g_hash_table_replace(g.playlist_by_name, g_strdup(name), GINT_TO_POINTER(idx));

            ERR:
            _err = _err;

        }

        if (p) mpd_return_pair(g.connection, p);

        if (_break) break;
    }

    g.playlist_vec = vec_sort(plvec, struct pl *, cmp_pl);

    if (g.verbose) print_playlists();

    /* No playlists added. */
    int s = vec_size(plvec);
    if (s == -1)
        pieprf;

    clear_playlist_vec(plvec, false);

    g.playlist_n = s;

    if (g.verbose) {
        say("");
        say("");
    }

    free(matches);
    return true;
}

/* Lua versions.
 * These all 'throw'.
 */
int f_mpd_toggle_play_l(lua_State *L) {
    if (! f_mpd_toggle_play()) {
        lua_pushstring(L, "Couldn't toggle play.");
        lua_error(L);
    }
    return 0;
}
int f_mpd_prev_l(lua_State *L) {
    if (! f_mpd_prev()) {
        lua_pushstring(L, "Couldn't go to prev song.");
        lua_error(L);
    }
    return 0;
}
int f_mpd_next_l(lua_State *L) {
    if (! f_mpd_next()) {
        lua_pushstring(L, "Couldn't go to next song.");
        lua_error(L);
    }
    return 0;
}
int f_mpd_get_random_l(lua_State *L) {
    bool r;
    if (! f_mpd_get_random(&r)) {
        lua_pushstring(L, "Couldn't get random.");
        lua_error(L);
    }
    lua_pushboolean(L, r);
    return 1;
}
int f_mpd_toggle_random_l(lua_State *L) {
    bool r;
    if (! f_mpd_toggle_random(&r)) {
        lua_pushstring(L, "Couldn't toggle random.");
        lua_error(L);
    }
    lua_pushboolean(L, r);
    return 1;
}
int f_mpd_random_off_l(lua_State *L) {
    if (! f_mpd_random_off()) {
        lua_pushstring(L, "Couldn't turn random off.");
        lua_error(L);
    }
    return 0;
}
int f_mpd_random_on_l(lua_State *L) {
    if (! f_mpd_random_on()) {
        lua_pushstring(L, "Couldn't turn random on.");
        lua_error(L);
    }
    return 0;
}

int f_mpd_database_update_l(lua_State *L) {
    if (! f_mpd_database_update()) {
        lua_pushstring(L, "Couldn't update mpd.");
        lua_error(L);
    }
    return 0;
}

int f_mpd_is_updating_l(lua_State *L) {
    bool u;
    if (! f_mpd_is_updating(&u)) {
        lua_pushstring(L, "Couldn't get is_updating status.");
        lua_error(L);
    }
    lua_pushboolean(L, u);
    return 1;
}

int f_mpd_next_playlist_l(lua_State *L) {
    if (! f_mpd_next_playlist()) {
        lua_pushstring(L, "Couldn't go to next playlist.");
        lua_error(L);
    }
    return 0;
}
int f_mpd_prev_playlist_l(lua_State *L) {
    if (! f_mpd_prev_playlist()) {
        lua_pushstring(L, "Couldn't go to prev playlist.");
        lua_error(L);
    }
    return 0;
}
int f_mpd_load_playlist_by_name_l(lua_State *L) {
    const char *name = luaL_checkstring(L, -1);
    if (! f_mpd_load_playlist_by_name((char *) name)) {
        lua_pushstring(L, "Couldn't load playlist by name.");
        lua_error(L);
    }
    return 0;
}
int f_mpd_seek_l(lua_State *L /* int secs */) {
    lua_Number seek = luaL_checknumber(L, -1);
    if (! f_mpd_seek((int) seek)) {
        lua_pushstring(L, "Couldn't seek.");
        lua_error(L);
    }
    return 0;
}
