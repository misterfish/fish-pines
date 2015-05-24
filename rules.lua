needs ({ me = 'rules' }, 'util', '__imported_util', 'capi', 'configlua')

local _, i, k, v

local FORK_WAIT_VERBOSE_OK = false

local function test () 
    util.fork_wait ('ls -l', {
        verbose_ok = true,
    })
end

local function update_playlists () 
    -- might be useful later in this factory form.
    -- for now, not so much.
    local function make_update_function (cmd)
        return function ()
            local isok = false
            util.fork_wait (cmd, {
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
                verbose_ok = FORK_WAIT_VERBOSE_OK,
            })
            return { ok = isok }
        end
    end

    local function mpd_update ()
        capi.mpd.database_update ()
        while true do
            if capi.mpd.is_updating () then
                coroutine.yield {}
            else 
                break
            end
        end
        return { ok = true }
    end

    -- fish-pines will notice on the next round of mpd_update that an update
    -- happened, and reload all the playlists.
    -- there are probably some race conditions in which the update finishes,
    -- but doesn't show all.m3u as a playlist.

    local function all_done ()
        info '… done!'
        -- coro version not necessary
        capi.mpd.load_playlist_by_name (configlua.default_playlist)
    end

    local flashco = led.flashco 'update'

    info 'Starting database update and playlist remake …'

    -- run tasks cooperatively and flash the led.
    coro.pool {
        verbose = configlua.update_playlist_verbose_tasks,
        done = all_done,
        tasks = {
            { master = true, flashco },

            { hashooks = true, coroutine.create (mpd_update) },
            { hashooks = true, coroutine.create (make_update_function ('make-playlist-all >/dev/null')) },
        }
    }

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

return {
    -- mode = music
    music = {
        press = {
            { 'up','right', once = true, handler = function () led.test_on () end },
            { 'up','left', once = true, handler = function () led.test_off () end },

            {      'left',  once = true, handler = function () capi.mpd.prev_song () end },
            { 'b', 'right', handler = function () capi.mpd.seek (configlua.mpd.seek) end },
            { 'b', 'left',  handler = function () capi.mpd.seek (configlua.mpd.seek * -1) end },
            { 'b', 'up',    once = true, handler = function () capi.mpd.next_playlist () end },
            { 'b', 'down',  once = true, handler = function () capi.mpd.prev_playlist () end },
            {      'down',  handler = function () vol.down () end },
            { 'up',         handler = function () vol.up () end },
            { 'a',          once = true, handler = mpd.toggle_random },
            { 'select',     once = true, handler = mode.next_mode  },
            { 'start',      once = true, handler = function () capi.mpd.toggle_play () end },

            { 'b', 'a',     once = true, handler = function () update_playlists () end },
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


