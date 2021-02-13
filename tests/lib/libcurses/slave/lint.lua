#! /usr/bin/lua
-- $NetBSD: lint.lua,v 1.2 2021/02/13 18:24:11 rillig Exp $

--[[

usage: lua ./lint.lua

Check that the boilerplate code does not contain unintended
inconsistencies.

]]


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
    if c ~= nil and c > 0 then
      argc, argi = c, 0
    end

    local i = num(line:match("^\tARG_[%w_]+%((%d+)"))
    if i ~= nil and argi == nil then
      errors:add("%s:%d: ARG without preceding ARGC", fname, lineno)
    end

    if i == nil and argi ~= nil and c == nil then
      errors:add("%s:%d: expecting ARG %d, got %s", fname, lineno, argi, line)
      argc, argi = nil, nil
    end

    if i ~= nil and argi ~= nil and i ~= argi then
      errors:add("%s:%d: expecting ARG %d, not %d", fname, lineno, argi, i)
      argi = i
    end

    if i ~= nil and argi ~= nil and i == argi then
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
