local function on (which, opts) 
    local pin = configlua.leds[which]
    opts = opts or {}
    if not pin then return warnf("Can't find led for %s", BR(which)) end
    local fn = opts.coroutine and capi.gpio.on_co or capi.gpio.on
    fn(pin)
end

local function off (which, opts) 
    local pin = configlua.leds[which]
    opts = opts or {}
    if not pin then return warnf("Can't find led for %s", BR(which)) end
    local fn = opts.coroutine and capi.gpio.off_co or capi.gpio.off
    fn(pin)
end

local function get (which)
    local pin = configlua.leds[which]
    if not pin then return warnf("Can't find led for %s", BR(which)) end
    return capi.gpio.read(pin)
end

return {
    on = on,
    off = off,
    get = get
}
