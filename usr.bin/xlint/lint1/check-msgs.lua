#! /usr/bin/lua
-- $NetBSD: check-msgs.lua,v 1.15 2022/07/03 20:05:46 rillig Exp $

--[[

usage: lua ./check-msgs.lua *.c *.y

Check that the message text in the comments of the C source code matches the
actual user-visible message text in err.c.

]]


local function load_messages(fname)
  local msgs = {} ---@type table<number>string

  local f = assert(io.open(fname, "r"))
  for line in f:lines() do
    local msg, id = line:match("%s*\"(.+)\",%s*/%*%s*(%d+)%s*%*/$")
    if msg ~= nil then
      msgs[tonumber(id)] = msg
    end
  end

  f:close()

  return msgs
end


local had_errors = false
---@param fmt string
function print_error(fmt, ...)
  print(fmt:format(...))
  had_errors = true
end


local function check_message(fname, lineno, id, comment, msgs)
  local msg = msgs[id]

  if msg == nil then
    print_error("%s:%d: id=%d not found", fname, lineno, id)
    return
  end

  msg = msg:gsub("/%*", "**")
  msg = msg:gsub("%*/", "**")
  msg = msg:gsub("\\(.)", "%1")

  if comment == msg then
    return
  end

  local prefix = comment:match("^(.-)%s*%.%.%.$")
  if prefix ~= nil and msg:find(prefix, 1, 1) == 1 then
    return
  end

  print_error("%s:%d:   id=%-3d   msg=%-40s   comment=%s",
    fname, lineno, id, msg, comment)
end

local is_message_function = {
  error = true,
  error_at = true,
  warning = true,
  warning_at = true,
  c99ism = true,
  c11ism = true,
  gnuism = true,
}

local function check_file(fname, msgs)
  local f = assert(io.open(fname, "r"))
  local lineno = 0
  local prev = ""
  for line in f:lines() do
    lineno = lineno + 1

    local func, id = line:match("^%s+([%w_]+)%((%d+)[),]")
    if is_message_function[func] then
      id = tonumber(id)
      local comment = prev:match("^%s+/%* (.+) %*/$")
      if comment ~= nil then
        check_message(fname, lineno, id, comment, msgs)
      else
        print_error("%s:%d: missing comment for %d: /* %s */",
          fname, lineno, id, msgs[id])
      end
    end

    prev = line
  end

  f:close()
end


local function file_contains(filename, text)
  local f = assert(io.open(filename, "r"))
  local found = f:read("a"):find(text, 1, true)
  f:close()
  return found
end


-- Ensure that each test file for a particular message mentions the full text
-- of that message and the message ID.
local function check_test_files(msgs)
  local testdir = "../../../tests/usr.bin/xlint/lint1"
  local cmd = ("cd '%s' && printf '%%s\\n' msg_[0-9][0-9][0-9]*.c"):format(testdir)
  local filenames = assert(io.popen(cmd))
  for filename in filenames:lines() do
    local msgid = tonumber(filename:match("^msg_(%d%d%d)"))
    if msgs[msgid] then
      local unescaped_msg = msgs[msgid]:gsub("\\(.)", "%1")
      local expected_text = ("%s [%d]"):format(unescaped_msg, msgid)
      local fullname = ("%s/%s"):format(testdir, filename)
      if not file_contains(fullname, expected_text) then
        print_error("%s must contain: %s", fullname, expected_text)
      end
    end
  end
  filenames:close()
end

local function main(arg)
  local msgs = load_messages("err.c")
  for _, fname in ipairs(arg) do
    check_file(fname, msgs)
  end
  check_test_files(msgs)
end

main(arg)
os.exit(not had_errors)
