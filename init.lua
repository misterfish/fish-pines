util          = require 'util'

-- export everything from util
setmetatable(_G, {__index = function(tbl, varname, val)
    if val == nil then
        return util[varname]
    end
end})

config          = require 'config'
configlua       = require 'configlua'

shutdown        = require 'shutdown'
led             = require 'led'
custom          = require 'custom'
mode            = require 'mode'

local capi = capi

local no_nes = os.getenv("NO_NES") == '1'

local conf = {'mpd', 'mode'}
if not no_nes then push(conf, 'nes') end

for _,v in pairs(conf) do
    capi[v].config(config[v])
end

function toggle_random() 
    local rand = capi.mpd.toggle_random() 
    if rand then led.on('random') else led.off('random') end
    printf("Random set to %s", CY(rand))
end

--[[ 

Rules passed to capi.buttons.add_rule() look like this:

{ 'b', 'right', mode = , event = , once = , handler = , chain = , exact = }

Button names: <required> 'b', 'a', 'select', 'up', etc.

mode                        <required>.           
                            Integer starting at 0, corresponding to the
                            order modes were given in the lua config.

event                       <required>
                            "press" or "release"

handler                     <optional>         
                            The lua function to call. It's actually
                            optional. You could have a rule with no handler,
                            whose only purpose is to block subsequent rules.
                            If the handler is nil, it will silently fail.

once:                       <optional, false>            
                            Debounce press events, so a long hold only
                            counts as a single press. 
                            Only applies to press events.
                            Also it only cancels the rule in which it
                            appears; other combinations will still trigger
                            on hold events unless they each have once set to
                            true.

chain:                      <optional, false>    
                            Keep looking for more rules after this one matched.

exact:                      <optional, true>
                            Only trigger if it matches this exactly, e.g.,
                            'b' + 'a' + 'select' won't trigger a 'b' + 'a'
                            event (except probably briefly, while the
                            buttons are being pressed)

]]

local rules = {
    -- mode = music
    music = {
        press = {
            {      'right', handler = function() capi.mpd.next_song() end },
            {      'left',  handler = function() capi.mpd.prev_song() end },
            { 'b', 'right', handler = function() capi.mpd.seek(configlua.mpd.seek) end },
            { 'b', 'left',  handler = function() capi.mpd.seek(configlua.mpd.seek * -1) end },
            { 'b', 'up',    handler = function() capi.mpd.next_playlist() end },
            { 'b', 'down',  handler = function() capi.mpd.prev_playlist() end },
            { 'a',          once = true, handler = toggle_random },
            { 'select',     once = true, handler = mode.next_mode  },
            { 'start',      once = true, handler = function() capi.mpd.toggle_play() end },
        },
        release = {
        }
    },
    -- mode = general
    general = {
        press = {
            { 'select',     once = true, handler = mode.next_mode  },
            { 'start',      once = true, handler = shutdown.start_pressed  },
            { 'b',          once = true, handler = custom.switch_to_wired },
            { 'a',          once = true, handler = custom.switch_to_wireless },
        },
        release = {
            { 'start',      handler = shutdown.start_released  },
        }
    }
}

-- called by C after init.
function buttons_config() 
    for modename,t in pairs(rules) do
        local mode_idx = mode.idx_for_name(modename) 
        if not mode_idx then
            warn("ABC") -- XX
            break
        end
        for event,u in pairs(t) do
            for _,rule in ipairs(u) do
                rule.mode = mode_idx - 1
                rule.event = event
                capi.buttons.add_rule(rule)
            end
        end
    end
end
