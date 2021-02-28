#!  /usr/bin/lua
-- $NetBSD: check-expect.lua,v 1.6 2021/02/28 01:20:54 rillig Exp $

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

  local comment_linenos = {}
  local comments_by_lineno = {}
  local function add_expectation(lineno, msg)
    if comments_by_lineno[lineno] == nil then
      table.insert(comment_linenos, lineno)
      comments_by_lineno[lineno] = {}
    end
    local trimmed_msg = msg:match("^%s*(.-)%s*$")
    table.insert(comments_by_lineno[lineno], trimmed_msg)
  end

  for lineno, line in ipairs(lines) do

    for offset, comments in line:gmatch("/%* expect([+%-]%d+): (.-) %*/") do
      for comment in comments:gmatch("[^,]+") do
        add_expectation(lineno + tonumber(offset), comment)
      end
    end

    for comments in line:gmatch("/%* expect: (.-) %*/") do
      for comment in comments:gmatch("[^,]+") do
	add_expectation(lineno, comment)
      end
    end

    local pp_lineno, pp_fname = line:match("^#%s*(%d+)%s+\"([^\"]+)\"")
    if pp_lineno ~= nil then
      if pp_fname == fname and tonumber(pp_lineno) ~= lineno + 1 then
        errors:add("error: %s:%d: preprocessor line number must be %d",
          fname, lineno, lineno + 1)
      end
    end
  end

  return comment_linenos, comments_by_lineno
end


local function load_actual_messages_from_exp(fname)

  local lines = load_lines(fname)
  if lines == nil then return nil end

  local messages = {}
  for lineno, line in ipairs(lines) do
    for c_lineno, message in line:gmatch("%S+%((%d+)%): (.+)$") do
      table.insert(messages, {
        exp_lineno = lineno,
        c_lineno = tonumber(c_lineno),
        msg = message
      })
    end
  end

  return messages
end


local function check_test(c_fname, errors)
  local exp_fname = c_fname:gsub("%.c$", ".exp")

  local comment_linenos, comments_by_lineno =
    load_expect_comments_from_c(c_fname, errors)
  if comment_linenos == nil then return end

  local messages = load_actual_messages_from_exp(exp_fname)
  if messages == nil then return end

  for _, act in ipairs(messages) do
    local exp = comments_by_lineno[act.c_lineno] or {}

    local found = false
    for i, msg in ipairs(exp) do
      if msg ~= "" and act.msg:find(msg, 1, true) then
        exp[i] = ""
        found = true
        break
      end
    end

    if not found then
      errors:add("error: %s:%d: must expect \"%s\"",
        c_fname, act.c_lineno, act.msg)
    end
  end

  for _, lineno in ipairs(comment_linenos) do
    for _, msg in ipairs(comments_by_lineno[lineno]) do
      if msg ~= "" then
        errors:add(
          "error: %s:%d: declared message \"%s\" is not in the actual output",
          c_fname, lineno, msg)
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
