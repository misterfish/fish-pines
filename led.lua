needs ({ me = 'led' }, 'configlua', 'capi', '__imported_util', 'mpd')

local me

local timeout_ids = {}

local function init ()
    if mpd.random () then
        me.on 'random'
    else
        me.off 'random'
    end
    me.off 'mode'
end

-- private
local function getpin (which)
    local pin = configlua.leds [which]
    if not pin then 
        return warnf ("Can't find led for %s", BR (which)) 
    else 
        return pin
    end
end

local function on (which) 
    local pin = getpin (which) or error2 ''
    capi.gpio.on (pin)
end

local function off (which) 
    local pin = getpin (which) or error2 ''
    capi.gpio.off (pin)
end

local function get (which)
    local pin = getpin (which) or error2 ''
    return capi.gpio.read (pin)
end

-- private
local function flash_running (which)
    return timeout_ids [which] and true or false
end

local function flash_stop (which) 
    local id = timeout_ids [which] or error2f ('%s is not flashing', BR (which))
    main.remove_timeout { id = id } 
    timeout_ids [which] = nil
    off (which)
end

local function flash_start (which, ms)
    local inprogress = timeout_ids [which]
    if flash_running (which) then
        flash_stop (which)
    end
    local id = main.add_timeout {
        ms = ms,
        repeating = true,
        func = (function ()
            local state = false
            return function () 
                if not state then
                    on (which)
                else
                    off (which)
                end
                state = not state
            return true -- keep timeout alive
            end
        end)()
    }
    timeout_ids [which] = id
end

-- synonym for flash_start
local function flash (...)
    return flash_start (...)
end

-- use a coroutine to wait on some tasks and flash an led.
-- use this if the main loop should block while waiting on the tasks.
-- otherwise, use glib timeouts.
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

function test_on () 
    flash_start ('update', 1000)
end
function test_off () 
    flash_stop 'update'
end

me = {
    init = init,
    on = on,
    off = off,
    get = get,
    flashco = flashco,

    flash = flash,
    flash_start = flash_start,
    flash_stop = flash_stop,

    test_on = test_on,
    test_off = test_off,
}

return me
