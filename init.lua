posix           = require 'posix'
util            = require 'util'
coro            = require 'coro'

-- get everything from util (via metatable)
util.import (_G, util)

config          = require 'config'
configlua       = require 'configlua'

shutdown        = require 'shutdown'
vol             = require 'vol'
led             = require 'led'
custom          = require 'custom'
mode            = require 'mode'
rules           = require 'rules'

local capi = capi

local no_nes = os.getenv ("NO_NES") == '1'

local conf = {'mpd', 'mode'}
if not no_nes then push (conf, 'nes') end

for _,v in pairs (conf) do
    capi[v].config (config[v])
end

-- called by C after init.
function buttons_config () 
    for modename,t in pairs (rules) do
        local mode_idx = mode.idx_for_name (modename) 
        if not mode_idx then
            warn ("ABC") -- XX
            break
        end
        for event,u in pairs (t) do
            for _,rule in ipairs (u) do
                rule.mode = mode_idx - 1
                rule.event = event
                capi.buttons.add_rule (rule)
            end
        end
    end
end
