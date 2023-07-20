#!  /usr/bin/lua
-- $NetBSD: check-expect.lua,v 1.7 2023/07/08 11:03:00 rillig Exp $

--[[

usage: lua ./check-expect.lua *.c

Check that the /* expect+-n: ... */ comments in the .c source files match the
actual messages found in the corresponding .exp files.  The .exp files are
expected in the current working directory.

The .exp files are generated on the fly during the ATF tests, see
t_integration.sh.  During development, they can be generated using
lint1/accept.sh.
]]


local function test(func)
  func()
end

local function assert_equals(got, expected)
  if got ~= expected then
    assert(false, string.format("got %q, expected %q", got, expected))
  end
end


local had_errors = false
---@param fmt string
function print_error(fmt, ...)
  print(fmt:format(...))
  had_errors = true
end


local function load_lines(fname)
  local lines = {}

  local f, err, errno = io.open(fname, "r")
  if f == nil then return nil, err, errno end

  for line in f:lines() do
    table.insert(lines, line)
  end
  f:close()

  return lines
end


local function save_lines(fname, lines)
  local f = io.open(fname, "w")
  for _, line in ipairs(lines) do
    f:write(line .. "\n")
  end
  f:close()
end


-- Load the 'expect:' comments from a C source file.
--
-- example return values:
--   {
--     ["file.c(18)"] = {"invalid argument 'a'", "invalid argument 'b'"},
--     ["file.c(23)"] = {"not a constant expression [123]"},
--   },
--   { "file.c(18)", "file.c(23)" }
local function load_c(fname)
  local basename = fname:match("([^/]+)$")
  local lines = load_lines(fname)
  if lines == nil then return nil, nil end

  local pp_fname = fname
  local pp_lineno = 0
  local comment_locations = {}
  local comments_by_location = {}

  local function add_expectation(offset, message)
    local location = ("%s(%d)"):format(pp_fname, pp_lineno + offset)
    if comments_by_location[location] == nil then
      table.insert(comment_locations, location)
      comments_by_location[location] = {}
    end
    local trimmed_msg = message:match("^%s*(.-)%s*$")
    table.insert(comments_by_location[location], trimmed_msg)
  end

  for phys_lineno, line in ipairs(lines) do

    for offset, comment in line:gmatch("/%* expect([+%-]%d+): (.-) %*/") do
      add_expectation(tonumber(offset), comment)
    end

    pp_lineno = pp_lineno + 1

    local ppl_lineno, ppl_fname = line:match("^#%s*(%d+)%s+\"([^\"]+)\"")
    if ppl_lineno ~= nil then
      if ppl_fname == basename and tonumber(ppl_lineno) ~= phys_lineno + 1 then
        print_error("error: %s:%d: preprocessor line number must be %d",
          fname, phys_lineno, phys_lineno + 1)
      end
      if ppl_fname:match("%.c$") and ppl_fname ~= basename then
        print_error("error: %s:%d: preprocessor filename must be '%s'",
          fname, phys_lineno, basename)
      end
      pp_fname = ppl_fname
      pp_lineno = ppl_lineno
    end
  end

  return comment_locations, comments_by_location
end


-- Load the expected raw lint output from a .exp file.
--
-- example return value: {
--   {
--     exp_lineno = 18,
--     location = "file.c(18)",
--     message = "not a constant expression [123]",
--   }
-- }
local function load_exp(exp_fname)

  local lines = load_lines(exp_fname)
  if lines == nil then
    print_error("check-expect.lua: error: file " .. exp_fname .. " not found")
    return
  end

  local messages = {}
  for exp_lineno, line in ipairs(lines) do
    for location, message in line:gmatch("(%S+%(%d+%)): (.+)$") do
      table.insert(messages, {
        exp_lineno = exp_lineno,
        location = location,
        message = message
      })
    end
  end

  return messages
end


local function matches(comment, pattern)
  if comment == "" then return false end

  local any_prefix = pattern:sub(1, 3) == "..."
  if any_prefix then pattern = pattern:sub(4) end
  local any_suffix = pattern:sub(-3) == "..."
  if any_suffix then pattern = pattern:sub(1, -4) end

  if any_prefix and any_suffix then
    return comment:find(pattern, 1, true) ~= nil
  elseif any_prefix then
    return pattern ~= "" and comment:sub(-#pattern) == pattern
  elseif any_suffix then
    return comment:sub(1, #pattern) == pattern
  else
    return comment == pattern
  end
end

test(function()
  assert_equals(matches("a", "a"), true)
  assert_equals(matches("a", "b"), false)
  assert_equals(matches("a", "aaa"), false)

  assert_equals(matches("abc", "a..."), true)
  assert_equals(matches("abc", "c..."), false)

  assert_equals(matches("abc", "...c"), true)
  assert_equals(matches("abc", "...a"), false)

  assert_equals(matches("abc123xyz", "...a..."), true)
  assert_equals(matches("abc123xyz", "...b..."), true)
  assert_equals(matches("abc123xyz", "...c..."), true)
  assert_equals(matches("abc123xyz", "...1..."), true)
  assert_equals(matches("abc123xyz", "...2..."), true)
  assert_equals(matches("abc123xyz", "...3..."), true)
  assert_equals(matches("abc123xyz", "...x..."), true)
  assert_equals(matches("abc123xyz", "...y..."), true)
  assert_equals(matches("abc123xyz", "...z..."), true)
  assert_equals(matches("pattern", "...pattern..."), true)
end)


-- Inserts the '/* expect */' lines to the .c file, so that the .c file matches
-- the .exp file.  Multiple 'expect' comments for a single line of code are not
-- handled correctly, but it's still better than doing the same work manually.
local function insert_missing(missing)
  for fname, items in pairs(missing) do
    table.sort(items, function(a, b) return a.lineno > b.lineno end)
    local lines = assert(load_lines(fname))
    for _, item in ipairs(items) do
      local lineno, message = item.lineno, item.message
      local indent = (lines[lineno] or ""):match("^([ \t]*)")
      local line = ("%s/* expect+1: %s */"):format(indent, message)
      table.insert(lines, lineno, line)
    end
    save_lines(fname, lines)
  end
end


local function check_test(c_fname, update)
  local exp_fname = c_fname:gsub("%.c$", ".exp"):gsub(".+/", "")

  local c_comment_locations, c_comments_by_location = load_c(c_fname)
  if c_comment_locations == nil then return end

  local exp_messages = load_exp(exp_fname) or {}
  local missing = {}

  for _, exp_message in ipairs(exp_messages) do
    local c_comments = c_comments_by_location[exp_message.location] or {}
    local expected_message =
      exp_message.message:gsub("/%*", "**"):gsub("%*/", "**")

    local found = false
    for i, c_comment in ipairs(c_comments) do
      if c_comment ~= "" and matches(expected_message, c_comment) then
        c_comments[i] = ""
        found = true
        break
      end
    end

    if not found then
      print_error("error: %s: missing /* expect+1: %s */",
        exp_message.location, expected_message)

      if update then
        local fname = exp_message.location:match("^([^(]+)")
        local lineno = tonumber(exp_message.location:match("%((%d+)%)$"))
        if not missing[fname] then missing[fname] = {} end
        table.insert(missing[fname], {
          lineno = lineno,
          message = expected_message,
        })
      end
    end
  end

  for _, c_comment_location in ipairs(c_comment_locations) do
    for _, c_comment in ipairs(c_comments_by_location[c_comment_location]) do
      if c_comment ~= "" then
        print_error(
          "error: %s: declared message \"%s\" is not in the actual output",
          c_comment_location, c_comment)
      end
    end
  end

  if missing then
    insert_missing(missing)
  end
end


local function main(args)
  local update = args[1] == "-u"
  if update then
    table.remove(args, 1)
  end

  for _, name in ipairs(args) do
    check_test(name, update)
  end
end


main(arg)
os.exit(not had_errors)
