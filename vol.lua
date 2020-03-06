needs ({ me = 'vol' }, '__imported_util', 'capi', 'configlua')

local _, k, v

-- private
local function go (args)
    args = args or {}
    local amount = args.amount or error3 'need amount'
    if args.dir == 'down' then
        amount = -1 * amount
    end
    local card = args.card or 'all'
    local elem = args.elem or 'all'
    local chan = args.chan or 'all'
    capi.vol.set_rel (card, elem, chan, amount)
end

local function up (args)
    args = args or {}
    if not args.amount then args.amount = configlua.vol.upamount end
    args.dir = 'up'
    go (args)
end

local function down (args)
    args = args or {}
    if not args.amount then args.amount = configlua.vol.downamount end
    args.dir = 'down'
    go (args)
end

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

return {
    up = configlua.anton_mode and down or up,
    down = configlua.anton_mode and up or down,
    up_left = configlua.anton_mode and down_left or up_left,
    down_left = configlua.anton_mode and up_left or down_left,
    up_right = configlua.anton_mode and down_right or up_right,
    down_right = configlua.anton_mode and up_right or down_right,
}
