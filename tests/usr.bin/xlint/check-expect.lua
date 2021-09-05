#!  /usr/bin/lua
-- $NetBSD: check-expect.lua,v 1.13 2021/09/05 19:16:37 rillig Exp $

--[[

usage: lua ./check-expect.lua *.c

Check that the /* expect: ... */ comments in the .c source files match the
actual messages found in the corresponding .exp files.

]]


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


local function load_expect_comments_from_c(fname, errors)

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

    for comment in line:gmatch("/%* expect: (.-) %*/") do
      add_expectation(0, comment)
    end

    pp_lineno = pp_lineno + 1

    local ppl_lineno, ppl_fname = line:match("^#%s*(%d+)%s+\"([^\"]+)\"")
    if ppl_lineno ~= nil then
      if ppl_fname == fname and tonumber(ppl_lineno) ~= phys_lineno + 1 then
        errors:add("error: %s:%d: preprocessor line number must be %d",
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


local function check_test(c_fname, errors)
  local exp_fname = c_fname:gsub("%.c$", ".exp")

  local comment_locations, comments_by_location =
    load_expect_comments_from_c(c_fname, errors)
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
      errors:add("error: %s: missing /* expect+1: %s */", act.location, exp_comment)
    end
  end

  for _, location in ipairs(comment_locations) do
    for _, message in ipairs(comments_by_location[location]) do
      if message ~= "" then
        errors:add(
          "error: %s: declared message \"%s\" is not in the actual output",
          location, message)
      end
    end
  end
end


local function main(args)
  local errors = {}
  errors.add = function(self, fmt, ...)
    table.insert(self, string.format(fmt, ...))
  end

  for _, name in ipairs(args) do
    check_test(name, errors)
  end

  for _, error in ipairs(errors) do
    print(error)
  end

  return #errors == 0
end

os.exit(main(arg))
