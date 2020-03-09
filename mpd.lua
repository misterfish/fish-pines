needs ({ me = 'mpd' }, 'capi', '__imported_util')
-- needs_late 'led' XX

local RAND = {[true] = {col = G, hoe = "on"}, [false] = {col = CY, hoe = "off"}}
local _, k, v

local function random ()
    return capi.mpd.get_random ()
end

local function toggle_random ()
    local rand = capi.mpd.toggle_random ()
    local r = RAND [rand]
    local col, hoe = r.col, r.hoe
    led [hoe] 'random'
    infof ("Setting random to %s", col (hoe))
end

local function _vol_do (dir, amount)
    local col
    if dir == 'down' then
        amount = -1 * amount
        col = BR
    else
        col = G
    end
    capi.mpd.vol_set_rel (amount)
    infof ("Adjust mpd volume by %s", col (amount))
end

local function vol_up ()
    local amount = configlua.mpd.volupamount
    _vol_do ('up', amount)
end

local function vol_down ()
    local amount = configlua.mpd.voldownamount
    _vol_do ('down', amount)
end

local function listen_xxx (msg, col)
    infof (' %s | %s', CY ('mpd listener'), col (msg))
end

-- our listener will also harmlessly trigger if we set random via a button, with a
-- delay which depends on the update/ticks settings for mpd_update.
local function listen_random (rand)
    local r = RAND [rand]
    local col, hoe = r.col, r.hoe
    listen_xxx (spr ("random set to %s", col (hoe)), Y)
    led [hoe] 'random'
end

local function listen_playlists ()
    listen_xxx ('playlists-changed', Y)
end
local function listen_update ()
    listen_xxx ('update-started-or-finished', G)
end
local function listen_database_updated ()
    listen_xxx ('database-updated', BB)
end
local function listen_player_state ()
    listen_xxx ('player-state-changed', M)
end
local function listen_volume ()
    listen_xxx ('volume-altered', CY)
end
local function listen_device ()
    listen_xxx ('device-state-changed', CY)
end
local function listen_sticker ()
    listen_xxx ('sticker-modified', CY)
end
local function listen_subscription ()
    listen_xxx ('client-channel-subscription-altered', CY)
end
local function listen_message ()
    listen_xxx ('subscribed-channel-message-received', CY)
end

local function init ()
    for k, v in pairs {
        ['random'] = listen_random,
        ['playlists-changed'] = listen_playlists,
        ['update-started-or-finished'] = listen_update,
        ['database-updated'] = listen_database_updated,
        ['player-state-changed'] = listen_player_state,
        ['volume-altered'] = listen_volume,
        ['device-state-changed'] = listen_device,
        ['sticker-modified'] = listen_sticker,
        ['client-channel-subscription-altered'] = listen_subscription,
        ['subscribed-channel-message-received'] = listen_message,
    }
    do
        capi.main.add_listener(k, v)
    end
end

return {
    init = init,
    random = random,
    toggle_random = toggle_random,
    vol_up = vol_up,
    vol_down = vol_down,
}
