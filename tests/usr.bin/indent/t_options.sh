#! /bin/sh
# $NetBSD: t_options.sh,v 1.1 2021/10/16 03:20:13 rillig Exp $
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

# Tests for indent that focus on comparing the effects of the various command
# line options.
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
#		Finishes an 'input' or 'run' section.
#
# All text between these directives is not passed to indent.

srcdir=$(atf_get_srcdir)
indent=$(atf_config_get usr.bin.indent.test_indent /usr/bin/indent)

# shellcheck disable=SC2016
check_awk='
function die(msg)
{
	print msg > "/dev/stderr"
	exit(1)
}

# Skip comments starting with dollar; they are used for marking bugs and
# adding other remarks directly in the input or output sections.
/^[[:space:]]*\/[*][[:space:]]*[$].*[*]\/$/ ||
    /^[[:space:]]*\/\/[[:space:]]*[$]/ {
	next
}

/^#/ && $1 == "#indent" {
	print $0
	if ($2 == "input") {
		if (unused != 0)
			die(FILENAME ":" unused ": input is not used")
		mode = "input"
		in_lines_len = 0
		unused = NR
	} else if ($2 == "run") {
		mode = "run"
		cmd = ENVIRON["INDENT"]
		for (i = 3; i <= NF; i++)
			cmd = cmd " " $i
		for (i = 1; i <= in_lines_len; i++)
			print in_lines[i] | cmd
		close(cmd)
		unused = 0
	} else if ($2 == "run-identity") {
		cmd = ENVIRON["INDENT"]
		for (i = 3; i <= NF; i++)
			cmd = cmd " " $i
		for (i = 1; i <= in_lines_len; i++) {
			print in_lines[i] | cmd
			print in_lines[i] > "expected.out"
		}
		close(cmd)
		unused = 0
	} else if ($2 == "end") {
		mode = ""
	} else {
		die(FILENAME ":" NR ": error: invalid line \"" $0 "\"")
	}
	print $0 > "expected.out"
	next
}

mode == "input" {
	in_lines[++in_lines_len] = $0
}

mode == "run" {
	print $0 > "expected.out"
}

END {
	if (mode != "")
		die(FILENAME ":" NR ": still in mode \"" mode "\"")
	if (unused != 0)
		die(FILENAME ":" unused ": input is not used")
}
'

check()
{
	printf '%s\n' "$check_awk" > check.awk

	atf_check -o "file:expected.out" \
	    env INDENT="$indent" awk -f check.awk "$srcdir/$1.c"
}

atf_init_test_cases()
{
	for fname in "$srcdir"/opt_*.c; do
		test_name=${fname##*/}
		test_name=${test_name%.c}

		atf_test_case "$test_name"
		eval "${test_name}_body() { check '$test_name'; }"
		atf_add_test_case "$test_name"
	done
}
