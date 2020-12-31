#! /usr/bin/lua
-- $NetBSD: check-msgs.lua,v 1.1 2020/12/31 22:48:33 rillig Exp $

--[[

Check that the message text in the C source matches the actual user-visible
message text.

usage: ./check-diagnostics *.c

]]


local function load_messages(fname)
  local msgs = {}

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


local function check_message(fname, lineno, id, comment, msgs, errors)
  local msg = msgs[id]

  if msg == nil then
    errors:add("%s:%d: id=%d not found", fname, lineno, id)
    return
  end

  if comment == msg then
    return
  end

  local prefix = comment:match("(.*) %.%.%.$")
  if prefix ~= nil and msg:find(prefix) == 1 then
    return
  end

  errors:add("%s:%d:   id=%-3d   msg=%-40s   comment=%s",
    fname, lineno, id, msg, comment)
end


local function collect_errors(fname, msgs)
  local errors = {}
  errors.add = function(self, fmt, ...)
    table.insert(self, string.format(fmt, ...))
  end
  local f = assert(io.open(fname, "r"))
  local lineno = 0
  local prev = ""
  for line in f:lines() do
    lineno = lineno + 1

    local id = line:match("^%s+warning%((%d+)[),]")
    if id == nil then
      id = line:match("^%s+error%((%d+)[),]")
    end
    if id ~= nil then
      local comment = prev:match("^%s+/%*%s+(.+)%s+%*/$")
      if comment ~= nil then
        check_message(fname, lineno, tonumber(id), comment, msgs, errors)
      end
    end

    prev = line
  end

  f:close()

  return errors
end


local function check_file(fname, msgs)
  local errors = collect_errors(fname, msgs)
  for _, err in ipairs(errors) do
    print(err)
  end
  return #errors == 0
end


local function main(arg)
  local msgs = load_messages("err.c")
  local ok = true
  for _, fname in ipairs(arg) do
    ok = check_file(fname, msgs) and ok
  end
  return ok
end

os.exit(main(arg))
