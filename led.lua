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

-- need arg:
--  tasks = {
--   task, ...
--  }

local function flash (which, args)
    local pin = configlua.leds[which]
    if not pin then return warnf("Can't find led for %s", BR(which)) end

    local tasks = args.tasks or 
        error "flash: need tasks"
    local verbose = args.verbose or false

    local flashco = (function () 
        local slept = -1
        local state = nil
        local function update_state () 
            if state then
                led.off('random', { coroutine = true })
                state = nil
            else
                led.on('random', { coroutine = true })
                state = 1
            end
        end

        local flashco = coroutine.create (function() 
            while true do 
                slept = (slept + 1) % 5
                if slept == 0 then
                    update_state()
                end
                posix.nanosleep(0, 200e6)
                coroutine.yield()
            end
        end)

        return flashco
    end)()

    local cos, numtasks = (function ()
        local taskidx = 0
        local numtasks = 0
        local task
        local cos = {}

        for _, task in pairs (tasks) do
            taskidx = taskidx + 1
            numtasks = numtasks + 1

            local co = coroutine.create(task)

            cos[taskidx] = co
        end

        return cos, numtasks
    end)()

    local idx
    local done = {}
    local numdone = 0
    while true do
        for idx, co in ipairs (cos) do
            if done[idx] then goto nxt end

            if coroutine.status (co) ~= 'suspended' then
                done[idx] = true
                numdone = numdone + 1
                if numdone == numtasks then
                    goto done
                end
                goto nxt
            end
            local yield, how = coroutine.resume (co)
            if not yield then
                error "Error running coroutine."
            end

            if how.ok == nil then
                if verbose then 
                    infof ("Task %s still spinning.", 
                    ( ({ CY, Y }) [idx % 2 + 1] ) (idx)
                    )
                end
            elseif how.ok then
                if verbose then
                    infof ("Task %s went home.", G (idx))
                end
            else
                if verbose then
                    infof ("Task %s went crazy.", BR (idx))
                end
            end

            ::nxt::
        end
        coroutine.resume (flashco)
    end
    ::done::
end

return {
    on = on,
    off = off,
    get = get,
    flash = flash,
}
