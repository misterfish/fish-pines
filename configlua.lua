return {
    mpd = {
        seek = 5, -- secs
    },
    shutdown = {
        secs = 1.3,
        --cmd = 'sudo shutdown',
        cmd = 'sudo ls',
    },
    cmds = {
        switch_to_wireless = 'switch-to-internet-wireless',
        switch_to_wired    = 'switch-to-internet-wired',
    },
    leds = {
        -- BCM numbering.
        random = 15,
        update = 15,
        mode = 18,
        flash = {
            -- yield every <sleep> secs, flash every n times
            sleep = { secs = 0, msecs = 200 },
            ntimes = 3,
        },
    },
    vol = {
        upamount = 4, 
        downamount = 4, 
        sock = os.getenv ('HOME') .. '/.local/share/fish-vol/socket'
    },
    verbose = {
        sockets = false,
    },
    -- switch down and up for changing the volume.
    -- useful for bugging the bartender.
    anton_mode = true,
    default_playlist = 'all',
    update_playlist_verbose_tasks = false,
}
