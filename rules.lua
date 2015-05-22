local _, i, k, v

function test () 

    local function update_cmdbc (onw8, onsucc, onfail) 
        util.fork_wait('find /usr >/dev/null', { 
            onsuccess = function () 
                info 'success!'
                onsucc()
            end,
            onwait = function ()
                info 'waiting'
                onw8()
            end,
            onfailure = function ()
                warn 'disaster!'
                onfail()
            end,
            verbose_ok = true,
        })
    end

    local function update_cmd ()
        local isok = false
        util.fork_wait('find /usr/lib >/dev/null', { 
            onsuccess = function () 
                -- we're done, don't yield, just return from the function.
                isok = true
            end,
            onwait = function ()
                coroutine.yield {}
            end,
            onfailure = function ()
                -- same here.
                isok = false -- not nil
            end,
            --verbose_ok = true,
        })
        return { ok = isok }
    end

    led.flash ('update', {
        verbose = true,
        tasks = {
            update_cmd,
            update_cmd,
            update_cmd,
            update_cmd,
        }
    })

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


