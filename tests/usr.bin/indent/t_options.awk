# $NetBSD: t_options.awk,v 1.2 2021/10/23 20:35:18 rillig Exp $
#
# Copyright (c) 2021 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# $FreeBSD$

# Test driver for indent that focuses on comparing the effects of the various
# command line options.
#
# The test files contain the input to be formatted, the formatting options
# and the output, all as close together as possible. The test files use the
# following directives:
#
#	#indent input [description]
#		Specifies the input to be formatted.
#	#indent run [options]
#		Runs indent on the input, using the given options.
#	#indent end [description]
#		Finishes an '#indent input' or '#indent run' section.
#	#indent run-equals-input [options]
#		Runs indent on the input, expecting unmodified output.
#	#indent run-equals-prev-output [options]
#		Runs indent on the input, expecting the same output as from
#		the previous run.
#
# All text outside these directives is not passed to indent.

# Read the test specification from stdin, output the actual test output on
# stdout, write the expected test output to 'expected.out'.

function die(lineno, msg)
{
	if (!died) {
		died = 1	# suppress further actions in END block
		print FILENAME ":" lineno ": error: " msg > "/dev/stderr"
		exit(1)
	}
}

function warn(lineno, msg)
{
	print FILENAME ":" lineno ": warning: " msg > "/dev/stderr"
	warned = 1
}

function quote(s)
{
	return "'" s "'"
}

BEGIN {
	warned = 0
	died = 0

	section = ""		# "", "input" or "run"
	section_excl_comm = ""	# without dollar comments
	section_incl_comm = ""	# with dollar comments

	input_excl_comm = ""	# stdin for indent
	input_incl_comm = ""	# used for duplicate checks
	unused_input_lineno = 0

	output_excl_comm = ""	# expected output
	output_incl_comm = ""	# used for duplicate checks
	output_lineno = 0
}

# Hide comments starting with dollar from indent; they are used for marking
# bugs and adding other remarks directly in the input or output sections.
/^[[:space:]]*\/[*][[:space:]]*[$].*[*]\/$/ ||
    /^[[:space:]]*\/\/[[:space:]]*[$]/ {
	if (section != "")
		section_incl_comm = section_incl_comm $0 "\n"
	next
}

function check_unused_input()
{
	if (unused_input_lineno != 0)
		warn(unused_input_lineno, "input is not used")
}

function run_indent(inp,   i, cmd)
{
	cmd = ENVIRON["INDENT"]
	if (cmd == "")
		cmd = "indent"
	for (i = 3; i <= NF; i++)
		cmd = cmd " " $i
	printf("%s", inp) | cmd
	close(cmd)
}

/^#/ && $1 == "#indent" {
	print $0
	print $0 > "expected.out"

	if ($2 == "input") {
		check_unused_input()
		section = "input"
		section_excl_comm = ""
		section_incl_comm = ""
		unused_input_lineno = NR

	} else if ($2 == "run") {
		if (section != "")
			warn(NR, "unfinished section " quote(section))
		section = "run"
		output_lineno = NR
		section_excl_comm = ""
		section_incl_comm = ""

		run_indent(input_excl_comm)
		unused_input_lineno = 0

	} else if ($2 == "run-equals-input") {
		run_indent(input_excl_comm)
		printf("%s", input_excl_comm) > "expected.out"
		unused_input_lineno = 0

	} else if ($2 == "run-equals-prev-output") {
		run_indent(input_excl_comm)
		printf("%s", output_excl_comm) > "expected.out"

	} else if ($2 == "end" && section == "input") {
		if (section_incl_comm == input_incl_comm)
			warn(NR, "duplicate input; remove this section")

		input_excl_comm = section_excl_comm
		input_incl_comm = section_incl_comm
		section = ""

	} else if ($2 == "end" && section == "run") {
		if (section_incl_comm == input_incl_comm)
			warn(output_lineno,
			    "output == input; use run-equals-input")
		if (section_incl_comm == output_incl_comm)
			warn(output_lineno,
			    "duplicate output; use run-equals-prev-output")

		output_excl_comm = section_excl_comm
		output_incl_comm = section_incl_comm
		section = ""

	} else if ($2 == "end") {
		warn(NR, "misplaced " quote("#indent end"))

	} else {
		die(NR, "invalid line " quote($0))
	}

	next
}

section == "input" || section == "run" {
	section_excl_comm = section_excl_comm $0 "\n"
	section_incl_comm = section_incl_comm $0 "\n"
}

section == "run" {
	print $0 > "expected.out"
}

section == "" && !/^$/ && !/^#/ && !/^\// && !/^ \*/ {
	warn(NR, "non-comment line outside 'input' or 'run' section")
}

END {
	if (section != "")
		die(NR, "still in section " quote(section))
	check_unused_input()
	if (warned)
		exit(1)
}
