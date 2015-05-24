needs ({ me = 'shutdown' }, '__imported_util', 'capi', 'configlua')

local clock
clock = (function ()
    local function recalc (self, base)
        -- secs from the clock call is about 30 bits. 
        -- this calculation won't overflow double.
        if base == 'sus' then
            self.combined = self.secs + self.usecs * 1e-6
        elseif base == 'comb' then
            self.secs = math.floor (self.combined)
            self.usecs = (self.combined - self.secs) * 1e6
        else 
            warn 'abc' --XX
        end
    end
    local function set_secs (self, s) 
        self.secs = s
        recalc (self, 'sus')
    end
    local function set_usecs (self, us) 
        if us >= 1e6 then
            warn 'us too big'
            us = 1e6
        end
        self.usecs = us
        recalc (self, 'sus')
    end
    local function set_combined (self, com) 
        self.combined = com
        recalc (self, 'comb')
    end
    local function new (args) 
        args = args or {}
        local self = {
            secs = nil,
            usecs = nil,
            combined = nil,
        }
        setmetatable(self, {
            __index = function (tbl, varname, val)
                if val == nil then
                    return clock[varname]
                end
            end
        })
        if args.now then
            self.secs, self.usecs = capi.util.get_clock ()
            recalc (self, 'sus')
        end
        return self
    end

    return {
        new = new,
        set_secs = set_secs,
        set_usecs = set_usecs,
        set_combined = set_combined,
    }
end)()

local toen -- time pressed
local doel -- time for shutdown
local chars_printed = 0

local disabled = false

local function reset ()
    toen = nil
    doel = nil
    io.write '\r'
    io.write (spr ('%' .. chars_printed .. 's', ''))
    io.write '\r'
    chars_printed = 0
end

local function go () 
    info "Shutting down!"
    if sys (configlua.shutdown.cmd) then
        disabled = true
    end
    reset()
end

local function start_pressed () 
    if disabled then 
        return 
    end

    if not toen then
        toen = clock.new { now = true }
        doel = clock.new ()
        doel:set_combined(toen.combined + configlua.shutdown.secs)
        print ''
        print ''
        return
    end

    local now = clock.new { now = true }
    --local time_left = {}
    --time_left.secs = doel.secs - now.secs
    --time_left.usecs = doel.usecs - now.usecs
    local time_left = doel.combined - now.combined
    local str = spr ("\r« %.3f -> %s » ", math.max (time_left, 0), time_left > 0 and BR 'shutdown' or G 'shutdown' )
    chars_printed = math.max (chars_printed, #str)
    io.write (str)

    if now.combined > doel.combined then
        go ()
    end
end

local function start_released ()
    reset()
end

return {
    start_pressed = start_pressed,
    start_released = start_released
}
