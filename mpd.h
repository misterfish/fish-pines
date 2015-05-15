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
bool f_mpd_cleanup();
bool f_mpd_prev();
bool f_mpd_next();
bool f_mpd_update();
bool f_mpd_toggle_random();
bool f_mpd_random_off();
bool f_mpd_random_on();

bool f_mpd_prev_playlist();
bool f_mpd_next_playlist();

bool f_mpd_configl();
