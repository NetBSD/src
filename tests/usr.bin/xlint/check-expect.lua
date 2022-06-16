#!  /usr/bin/lua
-- $NetBSD: check-expect.lua,v 1.16 2022/06/16 16:58:36 rillig Exp $

--[[

usage: lua ./check-expect.lua *.c

Check that the /* expect: ... */ comments in the .c source files match the
actual messages found in the corresponding .exp files.

]]


local had_errors = false
---@param fmt string
function print_error(fmt, ...)
  print(fmt:format(...))
  had_errors = true
end


local function load_lines(fname)
  local lines = {}

  local f = io.open(fname, "r")
  if f == nil then return nil end

  for line in f:lines() do
    table.insert(lines, line)
  end
  f:close()

  return lines
end


local function load_expect_comments_from_c(fname)

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

    -- TODO: Remove these comments for all tests, as they often contain
    -- only the raw message ID, without the actual message text,
    -- which makes them harder to understand without looking up more context.
    for comment in line:gmatch("/%* expect: (.-) %*/") do
      if not fname:match("^msg_[01]") then
        add_expectation(0, comment)
      end
    end

    pp_lineno = pp_lineno + 1

    local ppl_lineno, ppl_fname = line:match("^#%s*(%d+)%s+\"([^\"]+)\"")
    if ppl_lineno ~= nil then
      if ppl_fname == fname and tonumber(ppl_lineno) ~= phys_lineno + 1 then
        print_error("error: %s:%d: preprocessor line number must be %d",
          fname, phys_lineno, phys_lineno + 1)
      end
      pp_fname = ppl_fname
      pp_lineno = ppl_lineno
    end
  end

  return comment_locations, comments_by_location
end


local function load_actual_messages_from_exp(exp_fname)

  local lines = load_lines(exp_fname)
  if lines == nil then return {} end

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


local function check_test(c_fname)
  local exp_fname = c_fname:gsub("%.c$", ".exp")

  local comment_locations, comments_by_location =
    load_expect_comments_from_c(c_fname)
  if comment_locations == nil then return end

  local messages = load_actual_messages_from_exp(exp_fname)
  if messages == nil then return end

  for _, act in ipairs(messages) do
    local exp = comments_by_location[act.location] or {}
    local exp_comment = act.message:gsub("/%*", "**"):gsub("%*/", "**")

    local found = false
    for i, message in ipairs(exp) do
      if message ~= "" and exp_comment:find(message, 1, true) then
        exp[i] = ""
        found = true
        break
      end
    end

    if not found then
      print_error("error: %s: missing /* expect+1: %s */",
        act.location, exp_comment)
    end
  end

  for _, location in ipairs(comment_locations) do
    for _, message in ipairs(comments_by_location[location]) do
      if message ~= "" then
        print_error(
          "error: %s: declared message \"%s\" is not in the actual output",
          location, message)
      end
    end
  end
end


local function main(args)
  for _, name in ipairs(args) do
    check_test(name)
  end
end


main(arg)
os.exit(not had_errors)
