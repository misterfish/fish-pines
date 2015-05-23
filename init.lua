local _, k, v

-- extern --

posix           = require 'posix'

-- intern --

needs           = require 'needs'

util            = require 'util'

-- get everything from util (via metatable)
util.import (_G, util)

coro            = require 'coro'

config          = require 'config'
configlua       = require 'configlua'

shutdown        = require 'shutdown'
vol             = require 'vol'
mpd             = require 'mpd'
led             = require 'led'
custom          = require 'custom'
mode            = require 'mode'
rules           = require 'rules'

local capi = capi

local no_nes = os.getenv ("NO_NES") == '1'

local conf = {'mpd', 'mode'}
if not no_nes then push (conf, 'nes') end

for _, v in pairs (conf) do
    capi[v].config (config[v])
end

capi.main.add_listener('random', mpd.listen_random)
capi.main.add_listener('playlists-changed', mpd.listen_playlists)
capi.main.add_listener('update-started-or-finished', mpd.listen_update)
capi.main.add_listener('database-updated', mpd.listen_database_updated)
capi.main.add_listener('player-state-changed', mpd.listen_player_state)
capi.main.add_listener('volume-altered', mpd.listen_volume)
capi.main.add_listener('device-state-changed', mpd.listen_device)
capi.main.add_listener('sticker-modified', mpd.listen_sticker)
capi.main.add_listener('client-channel-subscription-altered', mpd.listen_subscription)
capi.main.add_listener('subscribed-channel-message-received', mpd.listen_message)

-- called by C after init.
function start () 
    led.init()
    for modename, t in pairs (rules) do
        local mode_idx = mode.idx_for_name (modename) 
        if not mode_idx then
            warn "XX" -- XX
            break
        end
        for event, u in pairs (t) do
            for _, rule in ipairs (u) do
                rule.mode = mode_idx - 1
                rule.event = event
                capi.buttons.add_rule (rule)
            end
        end
    end
end
