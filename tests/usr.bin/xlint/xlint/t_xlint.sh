# $NetBSD: t_xlint.sh,v 1.1 2023/01/15 23:18:05 rillig Exp $
#
# Copyright (c) 2023 The NetBSD Foundation, Inc.
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

: ${lint:='/usr/bin/lint'}

run_lint1_error_head()
{
	:
}

run_lint1_error_body()
{
	cat <<-EOF >input.c || atf_fail 'prepare input.c'
		#include <stdbool.h>

		bool
		return_true(void)
		{
			return 1;
		}
	EOF

	atf_check \
	    -s 'exit:1' \
	    -o "inline:input.c(6): error: function has return type '_Bool' but returns 'int' [211]\n" \
	    "$lint" -aabceghiprSTxz input.c

	# In case of an error, no output file is written.
	atf_check \
	    -s 'exit:1' \
	    test -f input.ln
}


run_lint1_warning_head()
{
	:
}

run_lint1_warning_body()
{
	cat <<-EOF >input.c || atf_fail 'prepare input.c'
		static int number;

		const void *
		function(int a, const char *s)
		{
			return s + a;
		}
	EOF
	cat <<-EOF >input.exp || atf_fail 'prepare input.exp'
		0sinput.c
		Sinput.c
		1s<built-in>
		2s<command-line>
		4d0.4dr8functionF2IPcCPcV
	EOF

	atf_check \
	    -o "inline:input.c(1): warning: static variable 'number' unused [226]\n" \
	    "$lint" -aabceghiprSTxz input.c
	atf_check \
	    -o 'file:input.exp' \
	    cat input.ln
}

run_lint2_head()
{
	:
}

run_lint2_body()
{
	cat <<-EOF >input.ln || atf_fail 'prepare input.ln'
		0sinput.c
		Sinput.c
		1s<built-in>
		2s<command-line>
		4d0.4dr8functionF2IPcCPcV
	EOF

	# Most of the command line options are not relevant for lint2,
	# so they are effectively ignored.  The option '-i' is absent.
	atf_check \
	    -o 'inline:function defined( input.c(4) ), but never used\n' \
	    -e 'inline:lint: cannot find llib-lc.ln\n' \
	    "$lint" -aabceghprSTxz input.ln
}


atf_init_test_cases()
{
	atf_add_test_case 'run_lint1_error'
	atf_add_test_case 'run_lint1_warning'
	atf_add_test_case 'run_lint2'
}
