needs ({ me = 'custom' }, '__imported_util')

local function switch_to_wireless () 
    info 'Switching to internet wired'
    sys (configlua.cmds.switch_to_wireless)
end

local function switch_to_wired () 
    info 'Switching to internet wired'
    sys (configlua.cmds.switch_to_wired)
end

local function kill_us ()
    info 'Killing us'
--sys 's /etc/init.d/mpd restart'
    sys 's ka fish-pines'
end

local function sndi ()
    info 'sndi'
    sys 'sndi'
    kill_us ()
end

local function sndx ()
    info 'sndx'
    sys 'sndx'
    kill_us ()
end

local rules_replace = nil

local rules_enhance = {
    music = {
        press = {
            { 'start',         once = true, handler = function ()
                sys 'killerr ka mplayer'
            end },
        },
        release = {
        },
    },
    general = {
        press = {
            { 'b',          once = true, handler = switch_to_wired },
            { 'a',          once = true, handler = switch_to_wireless },
            { 'down',       once = true, handler = kill_us },
            { 'left',       once = true, handler = sndi },
            { 'right',      once = true, handler = sndx },
        },
        release = {
        },
    },
}

return {
    rules_replace = rules_replace,
    rules_enhance = rules_enhance,
}
