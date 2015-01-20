/* For the playlist stuff to work, change mpd.conf:
 * set the absolute path setting in playlists to yes
 * keep it on user mpd, and chgrp audio and chmd g+w on the playlist dir.
 */

// static not necessary -- it's just a namespace.
struct pl {
    char *name;
    char *path;
};

bool f_mpd_init();
bool f_mpd_toggle_play();
bool f_mpd_seek(int);
void f_mpd_cleanup();
bool f_mpd_prev();
bool f_mpd_next();
bool f_mpd_update();
bool f_mpd_toggle_random();
bool f_mpd_random_off();
bool f_mpd_random_on();

bool f_mpd_prev_playlist();
bool f_mpd_next_playlist();

static bool reload_playlists();
static int get_state();
static int get_queue_pos();
static int get_elapsed_time();
static int get_random();
//static bool playlist();

static bool
load_playlist(int idx);

