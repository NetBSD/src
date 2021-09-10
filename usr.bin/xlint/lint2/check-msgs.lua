#! /usr/bin/lua
-- $NetBSD: check-msgs.lua,v 1.2 2021/09/10 21:05:08 rillig Exp $

--[[

usage: lua ./check-msgs.lua *.c

Check that the message text in the comments of the C source code matches the
actual user-visible message text in msg.c.

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
  msg = msg:gsub("\\t", "\\\\t")  -- for lint2
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


local function check_file(fname, msgs)
  local f = assert(io.open(fname, "r"))
  local lineno = 0
  local prev = ""
  for line in f:lines() do
    lineno = lineno + 1

    local func, id = line:match("^%s+(%w+)%((%d+)[),]")
    id = tonumber(id)
    if func == "msg" then
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


local function main(arg)
  local msgs = load_messages("msg.c")
  for _, fname in ipairs(arg) do
    check_file(fname, msgs)
  end
end

main(arg)
os.exit(not had_errors)
