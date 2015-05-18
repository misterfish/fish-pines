config = {
    mpd = {
        --[3] = 4,
        host = 'localhost',
        port = 6600,
        timeout_ms = 3000,
        timeout_playlist_ms = 3000,
        play_on_load_playlist = true,
        playlist_path = "mpd-playlists", -- relative to root
        update_on_n_ticks = 10,
        my_friend = 3.4,
    }
}
