return {
    main = {
        -- when this is 40 responsivity is great and the main binary uses
        -- about 2% of the pi's cpu.
        poll_ms = 40,
        verbose = false,
    },
    nes = {
        -- Using Broadcom gpio numbering.
        dpin = 22, --yellow, old = 25
        cpin = 27, --red, old = 24
        lpin = 23, --orange, old = 23
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
        -- update mpd every n ms.
        update_ms = 600,
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
