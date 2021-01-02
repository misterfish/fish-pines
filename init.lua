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

main            = require 'main'
shutdown        = require 'shutdown'
vol             = require 'vol'
mpd             = require 'mpd'
led             = require 'led'
mode            = require 'mode'
rules           = require 'rules'
custom          = require 'custom'

local capi = capi

local no_nes = os.getenv ("NO_NES") == '1'

local conf = { 'main', 'mpd', 'mode', 'vol' }
if not no_nes then push (conf, 'nes') end

local rules_main, rules_enhance

function load_rules ()
    for modename, t in pairs (rules_main) do
        local mode_idx = mode.idx_for_name (modename)
        if not mode_idx then
            warn "XX" -- XX
            break
        end
        for event, u in pairs (t) do
            rules_enhance[modename] = rules_enhance[modename] or {}
            local enhance = rules_enhance[modename][event]
            if enhance then
                table_concat (u, enhance)
            end
            for _, rule in ipairs (u) do
                rule.mode = mode_idx - 1
                rule.event = event
                capi.buttons.add_rule (rule)
            end
        end
    end
end

local function init_capi (conf)
    for _, v in pairs (conf) do
        capi[v].config (config[v])
    end
end

-- called by C after init.
function start ()
    mpd.init ()
    led.init ()
    if custom.init ~= nil then custom.init () end
    load_rules ()
end

init_capi (conf)
rules_main = custom.rules_replace or rules.rules
rules_enhance = custom.rules_enhance or {}
