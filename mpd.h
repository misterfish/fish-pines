#ifndef __INCL_MPD_H
#define __INCL_MPD_H

/* For the playlist stuff to work, change mpd.conf:
 * set the absolute path setting in playlists to yes
 * keep it on user mpd, and chgrp audio and chmd g+w on the playlist dir.
 */

#define F_MPD_FORCE_INIT    0x1

bool f_mpd_init_config();
bool f_mpd_init();
bool f_mpd_init_f(short flags);

bool f_mpd_toggle_play();
bool f_mpd_seek(int);
bool f_mpd_cleanup();
bool f_mpd_prev();
bool f_mpd_next();
bool f_mpd_update_wrapper(void *p);
bool f_mpd_update();
bool f_mpd_toggle_random(bool *r);
bool f_mpd_get_random(bool *r);
bool f_mpd_random_off();
bool f_mpd_random_on();

bool f_mpd_prev_playlist();
bool f_mpd_next_playlist();
bool f_mpd_load_playlist_by_name();

/* Lua functions. 
 * These 'throw'.
 */
int f_mpd_config_l(lua_State *L);
int f_mpd_toggle_play_l(lua_State *L);
int f_mpd_prev_l(lua_State *L);
int f_mpd_next_l(lua_State *L);
int f_mpd_toggle_random_l(lua_State *L);
int f_mpd_get_random_l(lua_State *L);
int f_mpd_random_off_l(lua_State *L);
int f_mpd_random_on_l(lua_State *L);
int f_mpd_database_update_l(lua_State *L);
int f_mpd_next_playlist_l(lua_State *L);
int f_mpd_prev_playlist_l(lua_State *L);
int f_mpd_load_playlist_by_name_l(lua_State *L);
int f_mpd_seek_l(lua_State *L /* int secs */);
int f_mpd_is_updating_l(lua_State *L);
int f_mpd_vol_set_rel_l(lua_State *L);

#endif
