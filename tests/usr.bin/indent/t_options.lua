-- $NetBSD: t_options.lua,v 1.6 2023/06/16 23:19:01 rillig Exp $
--
-- Copyright (c) 2023 The NetBSD Foundation, Inc.
-- All rights reserved.
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
-- 1. Redistributions of source code must retain the above copyright
--    notice, this list of conditions and the following disclaimer.
-- 2. Redistributions in binary form must reproduce the above copyright
--    notice, this list of conditions and the following disclaimer in the
--    documentation and/or other materials provided with the distribution.
--
-- THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
-- ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
-- TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
-- PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
-- BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
-- POSSIBILITY OF SUCH DAMAGE.

-- usage: [INDENT=...] lua t_options.lua <file>...
--
-- Test driver for indent that runs indent on several inputs, checks the
-- output and can run indent with different command line options on the same
-- input.
--
-- The test files contain the input to be formatted, the formatting options
-- and the output, all as close together as possible. The test files use the
-- following directives:
--
--	//indent input
--		Specifies the input to be formatted.
--	//indent run [options]
--		Runs indent on the input, using the given options.
--	//indent end
--		Finishes an '//indent input' or '//indent run' section.
--	//indent run-equals-input [options]
--		Runs indent on the input, expecting unmodified output.
--	//indent run-equals-prev-output [options]
--		Runs indent on the input, expecting the same output as from
--		the previous run.
--
-- All text outside input..end or run..end directives is not passed to indent.
--
-- Inside the input..end or run..end sections, comments that start with '$'
-- are filtered out, they can be used for remarks near the affected code.
--
-- The actual output from running indent is written to stdout, the expected
-- test output is written to 'expected.out'.

local warned = false

local filename = ""
local lineno = 0

local prev_empty_lines = 0	-- the finished "block" of empty lines
local curr_empty_lines = 0	-- the ongoing "block" of empty lines
local max_empty_lines = 0	-- between sections
local seen_input_section = false-- the first input section is not checked

local section = ""		-- "", "input" or "run"
local section_excl_comm = ""	-- without dollar comments
local section_incl_comm = ""	-- with dollar comments

local input_excl_comm = ""	-- the input text for indent
local input_incl_comm = ""	-- used for duplicate checks
local unused_input_lineno = 0

local output_excl_comm = ""	-- expected output
local output_incl_comm = ""	-- used for duplicate checks
local output_lineno = 0

local expected_out = assert(io.open("expected.out", "w"))

local function die(ln, msg)
	io.stderr:write(("%s:%d: error: %s\n"):format(filename, ln, msg))
	os.exit(false)
end

local function warn(ln, msg)
	io.stderr:write(("%s:%d: warning: %s\n"):format(filename, ln, msg))
	warned = true
end

local function init_file(fname)
	filename = fname
	lineno = 0

	prev_empty_lines = 0
	curr_empty_lines = 0
	max_empty_lines = 0
	seen_input_section = false

	section = ""
	section_excl_comm = ""
	section_incl_comm = ""

	input_excl_comm = ""
	input_incl_comm = ""
	unused_input_lineno = 0

	output_excl_comm = ""
	output_incl_comm = ""
	output_lineno = 0
end

local function check_empty_lines_block(n)
	if max_empty_lines ~= n and seen_input_section then
		local lines = n ~= 1 and "lines" or "line"
		warn(lineno, ("expecting %d empty %s, got %d")
		    :format(n, lines, max_empty_lines))
	end
end

local function check_unused_input()
	if unused_input_lineno ~= 0 then
		warn(unused_input_lineno, "input is not used")
	end
end

local function run_indent(inp, args)
	local indent = os.getenv("INDENT") or "indent"
	local cmd = indent .. " " .. args .. " 2>t_options.err"

	local indent_in = assert(io.popen(cmd, "w"))
	indent_in:write(inp)
	local ok, kind, info = indent_in:close()
	if not ok then
		print("// " .. kind .. " " .. info)
	end
	for line in io.lines("t_options.err") do
		print("// " .. line)
	end
end

local function handle_empty_section(line)
	if line == "" then
		curr_empty_lines = curr_empty_lines + 1
	else
		if curr_empty_lines > max_empty_lines then
			max_empty_lines = curr_empty_lines
		end
		if curr_empty_lines > 0 then
			if prev_empty_lines > 1 then
				warn(lineno - curr_empty_lines - 1,
				    prev_empty_lines .. " empty lines a few "
				    .. "lines above, should be only 1")
			end
			prev_empty_lines = curr_empty_lines
		end
		curr_empty_lines = 0
	end
end

local function handle_indent_input()
	if prev_empty_lines ~= 2 and seen_input_section then
		warn(lineno, "input section needs 2 empty lines "
		    .. "above, not " .. prev_empty_lines)
	end
	check_empty_lines_block(2)
	check_unused_input()
	section = "input"
	section_excl_comm = ""
	section_incl_comm = ""
	unused_input_lineno = lineno
	seen_input_section = true
	output_excl_comm = ""
	output_incl_comm = ""
	output_lineno = 0
end

local function handle_indent_run(args)
	if section ~= "" then
		warn(lineno, "unfinished section '" .. section .. "'")
	end
	check_empty_lines_block(1)
	if prev_empty_lines ~= 1 then
		warn(lineno, "run section needs 1 empty line above, "
		    .. "not " .. prev_empty_lines)
	end
	section = "run"
	output_lineno = lineno
	section_excl_comm = ""
	section_incl_comm = ""

	run_indent(input_excl_comm, args)
	unused_input_lineno = 0
end

local function handle_indent_run_equals_input(args)
	check_empty_lines_block(1)
	run_indent(input_excl_comm, args)
	expected_out:write(input_excl_comm)
	unused_input_lineno = 0
	max_empty_lines = 0
end

local function handle_indent_run_equals_prev_output(args)
	check_empty_lines_block(1)
	run_indent(input_excl_comm, args)
	expected_out:write(output_excl_comm)
	max_empty_lines = 0
end

local function handle_indent_end_input()
	if section_incl_comm == input_incl_comm then
		warn(lineno, "duplicate input; remove this section")
	end

	input_excl_comm = section_excl_comm
	input_incl_comm = section_incl_comm
	section = ""
	max_empty_lines = 0
end

local function handle_indent_end_run()
	if section_incl_comm == input_incl_comm then
		warn(output_lineno,
		    "output == input; use run-equals-input")
	end
	if section_incl_comm == output_incl_comm then
		warn(output_lineno,
		    "duplicate output; use run-equals-prev-output")
	end

	output_excl_comm = section_excl_comm
	output_incl_comm = section_incl_comm
	section = ""
	max_empty_lines = 0
end

local function handle_indent_directive(line, command, args)
	print(line)
	expected_out:write(line .. "\n")

	if command == "input" and args ~= "" then
		warn(lineno, "'//indent input' does not take arguments")
	elseif command == "input" then
		handle_indent_input()
	elseif command == "run" then
		handle_indent_run(args)
	elseif command == "run-equals-input" then
		handle_indent_run_equals_input(args)
	elseif command == "run-equals-prev-output" then
		handle_indent_run_equals_prev_output(args)
	elseif command == "end" and args ~= "" then
		warn(lineno, "'//indent end' does not take arguments")
	elseif command == "end" and section == "input" then
		handle_indent_end_input()
	elseif command == "end" and section == "run" then
		handle_indent_end_run()
	elseif command == "end" then
		warn(lineno, "misplaced '//indent end'")
	else
		die(lineno, "invalid line '" .. line .. "'")
	end

	prev_empty_lines = 0
	curr_empty_lines = 0
end

local function handle_line(line)
	if section == "" then
		handle_empty_section(line)
	end

	-- Hide comments starting with dollar from indent; they are used for
	-- marking bugs and adding other remarks directly in the input or
	-- output sections.
	if line:match("^%s*/[*]%s*[$].*[*]/$")
	    or line:match("^%s*//%s*[$]") then
		if section ~= "" then
			section_incl_comm = section_incl_comm .. line .. "\n"
		end
		return
	end

	local cmd, args = line:match("^//indent%s+([^%s]+)%s*(.*)$")
	if cmd then
		handle_indent_directive(line, cmd, args)
		return
	end

	if section == "input" or section == "run" then
		section_excl_comm = section_excl_comm .. line .. "\n"
		section_incl_comm = section_incl_comm .. line .. "\n"
	end

	if section == "run" then
		expected_out:write(line .. "\n")
	end

	if section == ""
	    and line ~= ""
	    and line:sub(1, 1) ~= "/"
	    and line:sub(1, 2) ~= " *" then
		warn(lineno, "non-comment line outside 'input' or 'run' "
		    .. "section")
	end
end

local function handle_file(fname)
	init_file(fname)
	local f = assert(io.open(fname))
	for line in f:lines() do
		lineno = lineno + 1
		handle_line(line)
	end
	f:close()
end

local function main()
	for _, arg in ipairs(arg) do
		handle_file(arg)
	end
	if section ~= "" then
		die(lineno, "still in section '" .. section .. "'")
	end
	check_unused_input()
	expected_out:close()
	os.exit(not warned)
end

main()
