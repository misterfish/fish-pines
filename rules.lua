needs ({ me = 'rules' }, 'util', '__imported_util', 'capi', 'configlua') 

local me, _, i, k, v

local function test () 
    util.fork_wait ('ls -l', {
        verbose_ok = true,
    })
end

local function update_playlists () 
    local cmd = configlua.cmds.make_playlist_all

    led.flash_start ('update', 1000)

    local playlists_coro = util.fork_wait_coro {
        cmd = cmd,
        silence_out = configlua.cmds.silence_out,
        verbose_ok = configlua.coroutine_pools_verbose_ok,
    }

    local update_coro = coroutine.create (function ()
        capi.mpd.database_update ()
        while true do
            if capi.mpd.is_updating () then
                coroutine.yield {}
            else 
                break
            end
        end
        return { ok = true }
    end)

    local pool = coro.pool.new {
        tasks = {
            { hashooks = true, playlists_coro },
            { hashooks = true, update_coro },
        },
        verbose = configlua.update_playlist_verbose_tasks,
        done_cb = function ()
        end,
    }

    main.add_timeout {
        ms = 500,
        repeating = true,
        func = function ()
            pool:once ()
            if pool:isdone () then
                info 'Done with playlist update and database update.'
                led.flash_stop 'update'
                return false
            end

            return true
        end
    }
end

local function test1 () 
    update_playlists ()
end

--[[ 

Rules passed to capi.buttons.add_rule () look like this:

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

hold_indicator:             <optional, true>
                            Print a little graphic when a key is held.

]]

me = {
    -- mode = music
    music = {
        press = {
            { 'up', 'right',   handler = vol.up_right },
            { 'up', 'left',    handler = vol.up_left },
            { 'down', 'right', handler = vol.down_right },
            { 'down', 'left',  handler = vol.down_left },

            {      'left',     once = true, handler = function () capi.mpd.prev_song () end },
            { 'b', 'right',    handler = function () capi.mpd.seek (configlua.mpd.seek) end },
            { 'b', 'left',     handler = function () capi.mpd.seek (configlua.mpd.seek * -1) end },
            { 'b', 'up',       once = true, handler = function () capi.mpd.next_playlist () end },
            { 'b', 'down',     once = true, handler = function () capi.mpd.prev_playlist () end },
            {      'down',     handler = function () vol.down () end },
            { 'up',            handler = function () vol.up () end },
            { 'a',             once = true, handler = mpd.toggle_random },
            { 'select',        once = true, handler = mode.next_mode  },
            { 'start',         once = true, handler = function () capi.mpd.toggle_play () end },
 
            { 'b', 'a',        once = true, handler = function () update_playlists () end },
        },
        release = {
        }
    },
    -- mode = general
    general = {
        press = {
            { 'select',     once = true, handler = mode.next_mode  },
            { 'start',      hold_indicator = false, handler = shutdown.start_pressed  },
            { 'b',          once = true, handler = custom.switch_to_wired },
            { 'a',          once = true, handler = custom.switch_to_wireless },
        },
        release = {
            { 'start',      handler = shutdown.start_released  },
        }
    }
}

return me
