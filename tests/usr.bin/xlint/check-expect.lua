#!  /usr/bin/lua
-- $NetBSD: check-expect.lua,v 1.2 2021/01/16 15:02:11 rillig Exp $

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

local function load_expect_comments_from_c(fname)

  local lines = load_lines(fname)
  if lines == nil then return nil end

  local comments_by_line = {}
  local seen_comment = false
  for lineno, line in ipairs(lines) do
    local comments_in_line = {}
    for comments in line:gmatch("/%* expect: (.-) %*/$") do
      for comment in comments:gmatch("[^,]+") do
	table.insert(comments_in_line, comment:match("^%s*(.-)%s*$"))
        seen_comment = true
      end
    end
    comments_by_line[lineno] = comments_in_line
  end

  if seen_comment then return comments_by_line else return nil end
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
  local comments = load_expect_comments_from_c(c_fname)
  if comments == nil or #comments == 0 then return end
  local messages = load_actual_messages_from_exp(exp_fname)
  if messages == nil then return end

  local remaining = 0
  for lineno, exps in ipairs(comments) do
    for _, msg in ipairs(exps) do
      -- print("comment", lineno, msg)
      remaining = remaining + 1
    end
  end

  for _, act in ipairs(messages) do
    -- print("messages", act.exp_lineno, act.c_lineno, act.msg)

    local exp = comments[act.c_lineno] or {}

    local found = false
    for i, msg in ipairs(exp) do
      if msg ~= "" and act.msg:find(msg, 1, true) then
        exp[i] = ""
        found = true
        remaining = remaining - 1
        -- print("found", act.c_lineno, act.msg, msg, remaining)
        break
      end
    end

    if not found then
      errors:add("error: %s:%d: must expect \"%s\"",
        c_fname, act.c_lineno, act.msg)
    end
  end

  for lineno, exps in ipairs(comments) do
    for _, msg in ipairs(exps) do
      if msg ~= "" then
        errors:add("error: %s:%d: declared message \"%s\" is not in the actual output",
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
