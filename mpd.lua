needs ({ me = 'mpd' }, 'capi', '__imported_util')
-- needs_late 'led' XX

local RAND = {[true] = {col = Y, hoe = "on"}, [false] = {col = CY, hoe = "off"}}

local function random ()
    return capi.mpd.get_random () 
end

local function toggle_random () 
    local rand = capi.mpd.toggle_random () 
    local r = RAND[rand]
    local col, hoe = r.col, r.hoe
    led[hoe] 'random'
    infof ("Setting random to %s", col (hoe))
end

local function listen_xxx (msg, col)
    infof(' %s | %s', CY('mpd listener'), col(msg))
end

-- our listener will also harmlessly trigger if we set random via a button, with a
-- delay which depends on the update/ticks settings for mpd_update. 
local function listen_random (rand)
    local r = RAND[rand]
    local col, hoe = r.col, r.hoe
    listen_xxx (spr ("random set to %s", col (hoe)), Y)
    led[hoe] 'random'
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

return {
    random = random,
    toggle_random = toggle_random,

    listen_random = listen_random,
    listen_playlists = listen_playlists,  
    listen_update = listen_update,  
    listen_database_updated = listen_database_updated,  
    listen_player_state = listen_player_state,  
    listen_volume = listen_volume,  
    listen_device = listen_device,  
    listen_sticker = listen_sticker,  
    listen_subscription = listen_subscription,  
    listen_message = listen_message,  
}
