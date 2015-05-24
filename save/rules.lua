local function update_playlists_coro_version () 
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


