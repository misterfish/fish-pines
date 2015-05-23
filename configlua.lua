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
    },
    vol = {
        upamount = 5,
        downamount = 5,
        sock = os.getenv('HOME') .. '/.local/share/fish-vol/socket'
    },
    verbose = {
        sockets = false,
    },
    -- switch down and up for changing the volume.
    -- useful for bugging the bartender.
    anton_mode = true,
    default_playlist = 'all',
}
