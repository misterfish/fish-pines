local _, i, k, v

function test () 

    -- if e.g. this fails in C, a lua error is displayed, and execution of
    -- this stack stops. 
    -- so we don't need to check for errors at every step.
    --capi.mpd.update()

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

    --local 
    flashco = coroutine.create (function() 
        while true do 
            slept = (slept + 1) % 5
            if slept == 0 then
                update_state()
            end
            posix.nanosleep(0, 200e6)
            coroutine.yield()
        end
    end)

    --opts XX
    local verbose_ok = true

    local pid, errmsg = posix.fork ()
    if not pid then
        error 'hop'
    elseif pid == 0 then
        -- kind
        cmd = 'find /etc >/dev/null'
        local ok, str, int = os.execute(cmd)
        local okstr
        if not ok then
            okstr = BR('not ok') 
        else 
            okstr = G('ok')
        end

        local msg
        if str == 'signal' then
            msg = spr("killed by signal «%s»", BR(int))
        elseif str == 'exit' then
            if int ~= 0 then
                msg = spr("exited with status «%s»", BR(int))
            else
                msg = nil
            end
        else
            msg = "[lua unexpected str]" --XX
        end
        if ok and verbose_ok then
            infof("Cmd done, %s.", G('ok'))
        elseif not ok then
            infof("Error with cmd «%s»: %s.", BR(cmd), msg)
        end
        posix._exit(int)
    else
        -- papa
        while true do
            local cpid, str, int = posix.wait(pid, posix.WNOHANG)
            if not cpid then
                local errmsg, errnum = str, int
                warn("Reap: " .. errmsg)
                break
            elseif cpid == 0 then
                info 'waiting'
            else
                local how, status = str, int
                local msg
                if str == 'killed' then
                    msg = spr("killed by signal «%s»", BR(int))
                elseif str == 'stopped' then
                    msg = spr("stopped by signal «%s»", BR(int))
                elseif str == 'exited' then
                    if int ~= 0 then
                        msg = spr("exited with status «%s»", BR(int))
                    else
                        msg = nil
                    end
                else
                    msg = "[lua unexpected str]"
                end

                local prnt
                local ok
                if int ~= 0 then
                    prnt = true
                    ok = BR('not ok')
                elseif verbose_ok then
                    prnt = true
                    ok = G('ok')
                end
                if prnt then infof("Child %s done, %s.", Y(cpid), join(': ', ok, msg)) end
                break
            end
            coroutine.resume (flashco)
        end

    end
end

--[[ 

Rules passed to capi.buttons.add_rule() look like this:

{ 'b', 'right', mode = , event = , once = , handler = , chain = , exact = }

Button names: <required> 'b', 'a', 'select', 'up', etc.

mode                        <required>.           
                            Integer starting at 0, corresponding to the
                            order modes were given in the lua config.

event                       <required>
                            "press" or "release"

handler                     <optional>         
                            The lua function to call. It's actually
                            optional. You could have a rule with no handler,
                            whose only purpose is to block subsequent rules.
                            If the handler is nil, it will silently fail.

once:                       <optional, false>            
                            Debounce press events, so a long hold only
                            counts as a single press. 
                            Only applies to press events.
                            Also it only cancels the rule in which it
                            appears; other combinations will still trigger
                            on hold events unless they each have once set to
                            true.

chain:                      <optional, false>    
                            Keep looking for more rules after this one matched.

exact:                      <optional, true>
                            Only trigger if it matches this exactly, e.g.,
                            'b' + 'a' + 'select' won't trigger a 'b' + 'a'
                            event (except probably briefly, while the
                            buttons are being pressed)

]]

function toggle_random() 
    local rand = capi.mpd.toggle_random() 
    if rand then led.on 'random' else led.off 'random' end
    sayf("Random set to %s", CY(rand))
end

return {
    -- mode = music
    music = {
        press = {
            {      'right', once = true, handler = function() capi.mpd.next_song() end },
            {      'left',  once = true, handler = function() capi.mpd.prev_song() end },
            { 'b', 'right', handler = function() capi.mpd.seek(configlua.mpd.seek) end },
            { 'b', 'left',  handler = function() capi.mpd.seek(configlua.mpd.seek * -1) end },
            { 'b', 'up',    once = true, handler = function() capi.mpd.next_playlist() end },
            { 'b', 'down',  once = true, handler = function() capi.mpd.prev_playlist() end },
            {      'down',  handler = function() vol.down() end },
            { 'up',         handler = function() vol.up() end },
            { 'a',          once = true, handler = toggle_random },
            { 'select',     once = true, handler = mode.next_mode  },
            { 'start',      once = true, handler = function() capi.mpd.toggle_play() end },
            { 'b', 'a',     once = true, handler = function() test() end },
        },
        release = {
        }
    },
    -- mode = general
    general = {
        press = {
            { 'select',     once = true, handler = mode.next_mode  },
            { 'start',      handler = shutdown.start_pressed  },
            { 'b',          once = true, handler = custom.switch_to_wired },
            { 'a',          once = true, handler = custom.switch_to_wireless },
        },
        release = {
            { 'start',      handler = shutdown.start_released  },
        }
    }
}


