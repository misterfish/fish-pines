local capi = capi

require 'config'
require 'util'

local u = util

-- export everything from util
setmetatable(_G, {__index = function(tbl, varname, val)
    if (val == nil) then
        return util[varname]
    end
end})

local no_nes = os.getenv("NO_NES") == '1'

local conf = {'mpd', 'mode'}
if not no_nes then push(conf, 'nes') end

for _,v in pairs(conf) do
    capi[v].config(config[v])
end

function mode_next() 
    capi.mode.next_mode()
    local m = capi.mode.get_mode_name()
    local col
    col = m == config.mode.modes[1] and Y or CY

    sayf("Switched to mode %s", col(m))
end

function toggle_random() 
    local rand = capi.mpd.toggle_random() 
    printf("Random set to %s", CY(rand))
end

local shutdown = (function () 
    local time_down = {
        secs = nil,
        usecs = nil,
    }
    function start_pressed () 
        time_down.secs, time_down.usecs = capi.util.get_clock()
    end
    function start_released ()
        if time_down.secs == -1 or time_down.usecs == -1 then
            return warn "start_released called before start_pressed."
        end
        -- assume we're not talking about more than 10 secs here.
        local toen = {}
        local now = {}
        now.secs, now.usecs = capi.util.get_clock()
        toen.secs, toen.usecs = time_down.secs, time_down.usecs

        for _,v in pairs {now, toen} do
            local s = string.sub(v.secs, -2) -- ok on numbers, too
            v.combined = s + v.usecs * 1e-6
        end

        if now.combined - toen.combined > configlocal.shutdown_secs then
            doit()
        end
    end
    function doit () 
        say "Shutting down!"
        local cmd = "sudo shutdown"
        local code = os.execute(cmd)
        if code ~= 0 then
            warnf("Couldn't execute cmd «%s», code was «%s»", BR(cmd), Y(code))
        end
    end

    return {
        start_pressed = start_pressed,
        start_released = start_released
    }
end )()

function test ()
end

-- rules can also have a 'chain' attribute (default false)
local rules = {
    -- mode = music
    music = {
        press = {
            { 'b', 'right', handler = function() capi.mpd.seek(configlocal.mpd.seek) end },
            { 'b', 'left',  handler = function() capi.mpd.seek(configlocal.mpd.seek * -1) end },
            { 'a',          once = true, handler = toggle_random },
            { 'select',     once = true, handler = mode_next  },
            { 'start',      once = true, handler = function() capi.mpd.toggle_play() end },
        },
        release = {
        }
    },
    -- mode = general
    general = {
        press = {
            { 'select',     once = true, handler = mode_next  },
            { 'start',      once = true, handler = shutdown.start_pressed  },
        },
        release = {
            { 'start',      handler = shutdown.start_released  },
        }
    }
}

-- called by C after init.
function buttons_config() 
    local mode_names = imap(function (v, i) 
        return v, i
    end, config.mode.modes)

    for mode,t in pairs(rules) do
        for event,u in pairs(t) do
            for _,rule in ipairs(u) do
                rule.mode = mode_names[mode] - 1
                rule.event = event
                capi.buttons.add_rule(rule)
            end
        end
    end
end
