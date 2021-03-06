needs ({ me = 'vol' }, '__imported_util', 'capi', 'configlua')

local _, k, v

local function _alsa_do (args)
    args = args or {}
    local amount = args.amount or error3 'need amount'
    if args.dir == 'down' then
        amount = -1 * amount
    end
    local card = args.card or 'all'
    local elem = args.elem or 'all'
    local chan = args.chan or 'all'
    capi.vol.alsa_set_rel (card, elem, chan, amount)
end

local function alsa_up (args)
    args = args or {}
    if not args.amount then args.amount = configlua.vol.upamount end
    args.dir = 'up'
    _alsa_do (args)
end

local function alsa_down (args)
    args = args or {}
    if not args.amount then args.amount = configlua.vol.downamount end
    args.dir = 'down'
    _alsa_do (args)
end

--[[
-- vol up on all left channels
local function up_left (args)
    args = args or {}
    for _, v in pairs { 'front left', 'rear left', 'side left' } do
        args.chan = v
        up (args)
    end
end

-- vol down on all left channels
local function down_left (args)
    args = args or {}
    for _, v in pairs { 'front left', 'rear left', 'side left' } do
        args.chan = v
        down (args)
    end
end

-- vol up on all right channels
local function up_right (args)
    args = args or {}
    for _, v in pairs { 'front right', 'rear right', 'side right' } do
        args.chan = v
        up (args)
    end
end

-- vol down on all right channels
local function down_right (args)
    args = args or {}
    for _, v in pairs { 'front right', 'rear right', 'side right' } do
        args.chan = v
        down (args)
    end
end
]]

return {
    alsa_up = configlua.anton_mode and alsa_down or alsa_up,
    alsa_down = configlua.anton_mode and alsa_up or alsa_down,
    alsa_up = configlua.anton_mode and alsa_down or alsa_up,
    alsa_down = configlua.anton_mode and alsa_up or alsa_down,
}
