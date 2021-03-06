-- Ani = require "paint/ani" 
-- Object = require "paint/object" 
-- Menu = require "paint/menu" 
-- -- Ui = 

--- render
local module = {

    NewSprite = function (index, layer, ...)
        local fileidx
        if layer == RenderLayer.Active then
            fileidx = Ani 
        elseif layer == RenderLayer.Object then
            fileidx = Object
        elseif layer == RenderLayer.Ui then
            fileidx = Ui 
        elseif layer == RenderLayer.Menu then
            fileidx = Menu
        else 
            -- unreachable code 
        end
        --print("look 1", layer, type(fileidx), tostring(fileidx), type(index), tostring(index))
        local task = coroutine.create(fileidx[index])
        local result, why = coroutine.resume(task, ...)
        if result == false  then
            print("lua: Create New Sprite Failed", " why: ", tostring(why))
        end

        return function (...)   -- x, y, frame, option<table> as arg 
            coroutine.resume(task, ...)
        end
    end

    -- --- fn for sketchers 
    -- StepRender = function ()
    --     if #TaskQue > 0 then 
    --         for k, v in pairs(TaskQue) do
    --             if type(v) == "thread" then
    --                 coroutine.resume(v)
    --                 if coroutine.status(v) == "dead" then 
    --                     TaskQue[k] = nil
    --                 end
    --             end
    --         end
    --     end
    -- end,
    
    -- ClearRender = function ()
    --     for k, _ in pairs(TaskQue) do
    --         TaskQue[k] = nil 
    --     end
    -- end,

    -- ShowLog = function ()
    --     local LogArgs = {
    --         Hight   = 16,
    --         Width   = 9,
    --         Font    = "courier",
    --         GetLog  = function (id)
    --             if id == ThreadId.U then
    --                 local info = LogTable[id] or ""
    --                 local len = string.len(info)
    --                 if len > 10 then
    --                     info = string.sub(info, len - 10, len)
    --                 end
    --                 return info
    --             else
    --                 return LogTable[id] or "(no log)"  
    --             end
    --         end
    --     }
        
        
    --     Set.SetTextStyle(LogArgs.Hight, LogArgs.Width, LogArgs.Font)
    --     Act.Xyout(0,        0,                      "FPS:"..LogArgs.GetLog(ThreadId.C))
    --     Act.Xyout(1400,     0,                      "Key:"..LogArgs.GetLog(ThreadId.U))
    --     Act.Xyout(0,        900 - LogArgs.Hight,    "Net:"..LogArgs.GetLog(ThreadId.N))
    -- end,

    -- ClearTask = function ()
    --     TaskQue = {} 
    --     print("lua: ClearTask called")
    -- end,

    -- ClearObject = function ()
    --     print("lua: ClearObject called")
    -- end,

    -- UploadLog = function (logs)
    --     for _, v in pairs(ThreadId) do 
    --         if logs[v] ~= nil then 
    --             LogTable[v] = logs[v]
    --         end
    --     end
    -- end,

    -- UpdateTask = function (queue)
    --     for k, v in ipairs(queue) do
    --         table.insert(TaskQue, v)
    --         -- clear taskque in cache by cpp
    --     end
    --     -- print("lua: UpdateTask called")
    --     -- print("Cache's TaskQue len: "..#queue)
    -- end,

    -- --- fn for cache
    -- AddRenderTask = function (index, ...)
    --     -- print("lua: index: "..index)
    --     local task = coroutine.create(Ani[index])
    --     local result, why = coroutine.resume(task, ...)
    --     -- print("lua: AddRenderTask Called result: "..tostring(result).." why: "..tostring(why))
    --     table.insert(TaskCache, task)
    -- end,

    -- AddRenderLog = function (id, logstr)
    --     if id == ThreadId.N then
    --         local current = tostring(os.date("%M:%S"))
    --         logstr = current..": "..logstr
    --     end
    --     LogCache[id] = logstr
    -- end
}

return module