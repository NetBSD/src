#! /usr/bin/lua
-- $NetBSD: lint.lua,v 1.6 2021/06/13 19:50:18 rillig Exp $

--[[

usage: lua ./lint.lua

Check that the argument handling code does not contain unintended
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

local had_errors = false
local function print_error(fmt, ...)
  print(fmt:format(...))
  had_errors = true
end


local function num(s)
  if s == nil then return nil end
  return tonumber(s)
end


-- After each macro ARGC, there must be the corresponding macros for ARG.
local function check_args()
  local fname = "curses_commands.c"
  local lines = load_lines(fname)
  local curr_argc, curr_arg ---@type number|nil, number|nil

  for lineno, line in ipairs(lines) do

    local line_argc = num(line:match("^\tARGC%((%d+)"))
    if line_argc and line_argc > 0 then
      curr_argc, curr_arg = line_argc, 0
      goto next
    end

    local line_arg = line:match("^\tARG_[%w_]+%(")
    if line_arg and curr_arg then
      curr_arg = curr_arg + 1
      if curr_arg == curr_argc then
        curr_argc, curr_arg = nil, nil
      end
    elseif line_arg then
      print_error("%s:%d: ARG without preceding ARGC", fname, lineno)
    elseif curr_arg then
      print_error("%s:%d: expecting ARG %d, got %s",
        fname, lineno, curr_arg, line)
      curr_argc, curr_arg = nil, nil
    end

    ::next::
  end
end

check_args()
os.exit(not had_errors)
