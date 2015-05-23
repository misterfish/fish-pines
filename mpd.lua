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
    sayf ("Setting random to %s", col (hoe))
end

-- our listener will also harmlessly trigger if we set random via a button, with a
-- delay which depends on the update/ticks settings for mpd_update. 
local function listen_random (rand)
    local r = RAND[rand]
    local col, hoe = r.col, r.hoe
    sayf ("Got mpd event: random set to %s", col (hoe))
    led[hoe] 'random'
end

return {
    random = random,
    toggle_random = toggle_random,
    listen_random = listen_random,
}
