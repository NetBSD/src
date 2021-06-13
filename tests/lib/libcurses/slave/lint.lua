#! /usr/bin/lua
-- $NetBSD: lint.lua,v 1.3 2021/06/13 18:11:44 rillig Exp $

--[[

usage: lua ./lint.lua

Check that the boilerplate code does not contain unintended
inconsistencies.

]]

---@return string[]
local function load_lines(fname)
  local lines = {}

  local f = assert(io.open(fname, "r"))
  for line in f:lines() do
    table.insert(lines, line)
  end

  f:close()

  return lines
end


local function new_errors()
  local errors = {}
  errors.add = function(self, fmt, ...)
    table.insert(self, string.format(fmt, ...))
  end
  errors.print = function(self)
    for _, msg in ipairs(self) do
      print(msg)
    end
  end
  return errors
end


local function num(s)
  if s == nil then return nil end
  return tonumber(s)
end


-- After each macro ARGC, there must be the corresponding macros for ARG.
local function check_args(errors)
  local fname = "curses_commands.c"
  local lines = load_lines(fname)
  local argi, argc

  for lineno, line in ipairs(lines) do

    local c = num(line:match("^\tARGC%((%d)"))
    if c and c > 0 then
      argc, argi = c, 0
    end

    local arg = line:match("^\tARG_[%w_]+%(")
    if arg and not argi then
      errors:add("%s:%d: ARG without preceding ARGC", fname, lineno)
    end

    if not arg and argi and not c then
      errors:add("%s:%d: expecting ARG %d, got %s", fname, lineno, argi, line)
      argc, argi = nil, nil
    end

    if arg and argi then
      argi = argi + 1
      if argi == argc then
        argc, argi = nil, nil
      end
    end
  end
end


local function main(arg)
  local errors = new_errors()
  check_args(errors)
  errors:print()
  return #errors == 0
end


os.exit(main(arg))
