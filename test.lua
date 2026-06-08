local heap = require("minheap")

local errors = 0
local function check(cond, msg)
    if not cond then
        print("FAIL: " .. msg)
        errors = errors + 1
    else
        print("OK:   " .. msg)
    end
end

print("── basic push/pop ──")
local h = heap.new()
h:push("a", 3)
h:push("b", 1)
h:push("c", 2)
check(h:size() == 3, "size == 3")
check(not h:empty(), "not empty")
check(h:contains("a"), "contains a")
check(h:contains("b"), "contains b")
check(h:contains("c"), "contains c")
check(not h:contains("d"), "not contains d")

local v = h:pop()
check(v.value == "b" and v.priority == 1, "pop b/1")
v = h:pop()
check(v.value == "c" and v.priority == 2, "pop c/2")
v = h:pop()
check(v.value == "a" and v.priority == 3, "pop a/3")
check(h:empty(), "empty after pops")
check(h:pop() == nil, "pop empty returns nil")

print("── remove ──")
h:push("x", 1); h:push("y", 2); h:push("z", 3)
h:remove("y")
check(h:size() == 2, "size after remove")
check(not h:contains("y"), "y removed")
check(h:contains("x") and h:contains("z"), "x and z remain")
h:remove("no_such_value")
check(h:size() == 2, "remove nonexistent silent")

print("── peek ──")
v = h:peek()
check(v.value == "x" and v.priority == 1, "peek x/1")
check(h:size() == 2, "peek doesn't change size")

print("── update ──")
h:update("x", 10)
v = h:peek()
check(v.value == "z" and v.priority == 3, "update x: z now top")
h:update("no_such_value", 5)
check(h:size() == 2, "update nonexistent silent")
h:update("z", 3)  -- same value
v = h:peek()
check(v.value == "z" and v.priority == 3, "update same value")

print("── clear ──")
h:clear()
check(h:empty(), "empty after clear")
check(h:size() == 0, "size 0 after clear")
check(not h:contains("x"), "x gone after clear")
check(h:pop() == nil, "pop nil after clear")

print("── integer values ──")
local h2 = heap.new()
h2:push(100, 10)
h2:push(200, 5)
check(h2:contains(200), "contains int 200")
v = h2:pop()
check(v.value == 200 and v.priority == 5, "pop int 200/5")
v = h2:pop()
check(v.value == 100 and v.priority == 10, "pop int 100/10")
check(not h2:contains(100), "int 100 gone")

print("── Dijkstra-style push + update ──")
local h3 = heap.new()
h3:push("A", math.huge)
h3:push("B", math.huge)
h3:push("C", math.huge)
h3:update("A", 0)
v = h3:pop()
check(v.value == "A" and v.priority == 0, "Dijkstra: A/0")
h3:update("B", 5)
v = h3:peek()
check(v.value == "B" and v.priority == 5, "Dijkstra: B/5 now top")

print("── string vs integer key distinction ──")
local h4 = heap.new()
h4:push("42", 1)   -- string "42"
h4:push(42, 2)     -- integer 42
check(h4:size() == 2, "string 42 and int 42 are distinct")
check(h4:contains("42"), "contains string 42")
check(h4:contains(42), "contains int 42")
v = h4:pop()
check(v.value == "42" and v.priority == 1, "pop string 42")
v = h4:pop()
check(v.value == 42 and v.priority == 2, "pop int 42")

print("")
if errors == 0 then
    print("All tests passed.")
else
    print(errors .. " test(s) FAILED.")
end
os.exit(errors == 0 and 0 or 1)
