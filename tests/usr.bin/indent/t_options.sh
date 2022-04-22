#! /bin/sh
# $NetBSD: t_options.sh,v 1.9 2022/04/22 21:21:20 rillig Exp $
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
#		Finishes an '#indent input' or '#indent run' section.
#	#indent run-equals-input [options]
#		Runs indent on the input, expecting unmodified output.
#	#indent run-equals-prev-output [options]
#		Runs indent on the input, expecting the same output as from
#		the previous run.
#
# All text outside these directives is not passed to indent.

srcdir=$(atf_get_srcdir)
indent=$(atf_config_get usr.bin.indent.test_indent /usr/bin/indent)

check()
{
	atf_check -o "file:expected.out" \
	    env INDENT="$indent" awk -f "$srcdir/t_options.awk" "$srcdir/$1.c"
}

atf_init_test_cases()
{
	for fname in "$srcdir"/*.c; do
		test_name=${fname##*/}
		test_name=${test_name%.c}

		atf_test_case "$test_name"
		eval "${test_name}_body() { check '$test_name'; }"
		atf_add_test_case "$test_name"
	done
}
