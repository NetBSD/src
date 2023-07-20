#! /usr/bin/lua
-- $NetBSD: check-msgs.lua,v 1.19 2023/07/10 11:46:14 rillig Exp $

--[[

usage: lua ./check-msgs.lua *.c *.y

Check that the message text in the comments of the C source code matches the
actual user-visible message text in err.c.

]]


local function load_messages()
  local msgs = {} ---@type table<string>string

  local f = assert(io.open("err.c"))
  for line in f:lines() do
    local msg, id = line:match("%s*\"(.+)\",%s*/%*%s*(Q?%d+)%s*%*/$")
    if msg ~= nil then
      msgs[id] = msg
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
    print_error("%s:%d: id=%s not found", fname, lineno, id)
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

  print_error("%s:%d:   id=%-3s   msg=%-40s   comment=%s",
    fname, lineno, id, msg, comment)
end

local message_prefix = {
  error = "",
  error_at = "",
  warning = "",
  warning_at = "",
  query_message = "Q",
  c99ism = "",
  c11ism = "",
  c23ism = "",
  gnuism = "",
}

local function check_file(fname, msgs)
  local f = assert(io.open(fname, "r"))
  local lineno = 0
  local prev = ""
  for line in f:lines() do
    lineno = lineno + 1

    local func, id = line:match("^%s+([%w_]+)%((%d+)[),]")
    local prefix = message_prefix[func]
    if prefix then
      id = prefix .. id
      local comment = prev:match("^%s+/%* (.+) %*/$")
      if comment ~= nil then
        check_message(fname, lineno, id, comment, msgs)
      else
        print_error("%s:%d: missing comment for %s: /* %s */",
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
    local msgid = filename:match("^msg_(%d%d%d)")
    if msgs[msgid] then
      local unescaped_msg = msgs[msgid]:gsub("\\(.)", "%1")
      local expected_text = ("%s [%s]"):format(unescaped_msg, msgid)
      local fullname = ("%s/%s"):format(testdir, filename)
      if not file_contains(fullname, expected_text) then
        print_error("%s must contain: %s", fullname, expected_text)
      end
    end
  end
  filenames:close()
end

local function check_yacc_file(filename)
  local decl = {}
  local f = assert(io.open(filename, "r"))
  local lineno = 0
  for line in f:lines() do
    lineno = lineno + 1
    local type = line:match("^%%type%s+<[%w_]+>%s+(%S+)$") or
      line:match("^/%* No type for ([%w_]+)%. %*/$")
    if type then
      decl[type] = lineno
    end
    local rule = line:match("^([%w_]+):")
    if rule then
      if decl[rule] then
        decl[rule] = nil
      else
        print_error("%s:%d: missing type declaration for rule %q",
          filename, lineno, rule)
      end
    end
  end
  for rule, decl_lineno in pairs(decl) do
    print_error("%s:%d: missing rule %q", filename, decl_lineno, rule)
  end
  f:close()
end

local function main(arg)
  local msgs = load_messages()
  for _, fname in ipairs(arg) do
    check_file(fname, msgs)
    if fname:match("%.y$") then
      check_yacc_file(fname)
    end
  end
  check_test_files(msgs)
end

main(arg)
os.exit(not had_errors)
