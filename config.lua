return {
    nes = {
        -- Using Broadcom gpio numbering.
        dpin = 25, --yellow
        cpin = 24, --red
        lpin = 23, --orange
    },
    mpd = {
        host = 'localhost',
        port = 6600,
        verbose = true,
        verbose_events = false,
        timeout_ms = 6000,
        timeout_playlist_ms = 15000, 
        play_on_load_playlist = true,
        playlist_path = "mpd-playlists", -- relative to root
        -- update mpd every n times through the main loop.
        -- each main loop is about 40ms.
        update_on_n_ticks = 15,
    },
    vol = {
        verbose = true,
        fasound_verbose = true,
    },
    mode = {
        modes = {
            'music',
            'general',
        },
    }
}
