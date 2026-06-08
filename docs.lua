---@meta minheap

---@class minheap
---@field push fun(self: minheap, value: integer|string, priority: number)
---@field pop fun(self: minheap): {value: integer|string, priority: number}|nil
---@field peek fun(self: minheap): {value: integer|string, priority: number}|nil
---@field empty fun(self: minheap): boolean
---@field size fun(self: minheap): integer
---@field clear fun(self: minheap)
---@field remove fun(self: minheap, value: integer|string)
---@field contains fun(self: minheap, value: integer|string): boolean
---@field update fun(self: minheap, value: integer|string, new_priority: number)
local minheap = {}

---@return minheap
function minheap.new() end

return minheap
