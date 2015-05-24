needs ({ me = 'led' }, 'configlua', 'capi', '__imported_util', 'mpd')

local on, off

local function init ()
    if mpd.random() then
        on 'random'
    else
        off 'random'
    end
    off 'mode'
end

on = function (which) 
    local pin = configlua.leds[which]
    if not pin then return warnf ("Can't find led for %s", BR (which)) end
    capi.gpio.on (pin)
end

off = function (which) 
    local pin = configlua.leds[which]
    if not pin then return warnf ("Can't find led for %s", BR (which)) end
    capi.gpio.off (pin)
end

local function get (which)
    local pin = configlua.leds[which]
    if not pin then return warnf ("Can't find led for %s", BR (which)) end
    return capi.gpio.read (pin)
end

local function flashco (which)
    local conf = configlua.leds.flash
    local flashco = (function () 
        local slept = -1
        local state = nil
        local function update_state () 
            if state then
                led.off (which)
                state = nil
            else
                led.on (which)
                state = 1
            end
        end

        local flashco = coroutine.create (function () 
            while true do 
                slept = (slept + 1) % conf.ntimes
                if slept == 0 then
                    update_state ()
                end
                posix.nanosleep (conf.sleep.secs, conf.sleep.msecs * 1e6)
                coroutine.yield ()
            end
        end)

        return flashco
    end) ()

    return flashco
end

return {
    init = init,
    on = on,
    off = off,
    get = get,
    flashco = flashco,
}
