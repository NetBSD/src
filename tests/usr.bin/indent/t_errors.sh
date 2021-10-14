#! /bin/sh
# $NetBSD: t_errors.sh,v 1.2 2021/10/14 17:42:13 rillig Exp $
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

# Tests for error handling in indent.

indent=$(atf_config_get usr.bin.indent.test_indent /usr/bin/indent)
nl='
'

expect_error()
{
	local msg

	msg="$1"
	shift

	atf_check -s 'exit:1' \
	    -e "inline:$msg$nl" \
	    "$indent" "$@"
}

atf_test_case 'option_unknown'
option_unknown_body()
{
	expect_error \
	    'indent: Command line: unknown option "-Z-unknown"' \
	    -Z-unknown
}

atf_test_case 'option_bool_trailing_garbage'
option_bool_trailing_garbage_body()
{
	expect_error \
	    'indent: Command line: unknown option "-bacchus"' \
	    -bacchus
}

atf_test_case 'option_int_missing_parameter'
option_int_missing_parameter_body()
{
	expect_error \
	    'indent: Command line: option "-ts" requires an integer parameter' \
	    -tsx
}

atf_test_case 'option_profile_not_found'
option_profile_not_found_body()
{
	expect_error \
	    'indent: profile ./nonexistent: No such file or directory' \
	    -P./nonexistent
}

atf_test_case 'option_typedefs_not_found'
option_typedefs_not_found_body()
{
	expect_error \
	    'indent: cannot open file ./nonexistent' \
	    -U./nonexistent
}

atf_test_case 'option_buffer_overflow'
option_buffer_overflow_body()
{
	opt='12345678123456781234567812345678'	# 32
	opt="$opt$opt$opt$opt$opt$opt$opt$opt"	# 256
	opt="$opt$opt$opt$opt$opt$opt$opt$opt"	# 2048
	opt="$opt$opt$opt$opt$opt$opt$opt$opt"	# 16384
	printf '%s\n' "-$opt" > indent.pro

	# TODO: The call to 'diag' should be replaced with 'errx'.
	expect_error \
	    'Error@1: buffer overflow in indent.pro, starting with '\''-123456781'\''' \
	    -Pindent.pro
}

atf_test_case 'option_special_missing_param'
option_special_missing_param_body()
{
	# TODO: Write '-cli' instead of only 'cli'.
	expect_error \
	    'indent: Command line: ``cli'\'\'' requires a parameter' \
	    -cli

	expect_error \
	    'indent: Command line: ``T'\'\'' requires a parameter' \
	    -T

	expect_error \
	    'indent: Command line: ``U'\'\'' requires a parameter' \
	    -U
}

atf_test_case 'unterminated_comment'
unterminated_comment_body()
{
	echo '/*' > comment.c

	atf_check -s 'exit:1' \
	    -o 'inline:/*'"$nl"' *'"$nl" \
	    -e 'inline:/**INDENT** Error@2: Unterminated comment */'"$nl" \
	    "$indent" -st < comment.c
}

atf_init_test_cases()
{
	atf_add_test_case 'option_unknown'
	atf_add_test_case 'option_bool_trailing_garbage'
	atf_add_test_case 'option_int_missing_parameter'
	atf_add_test_case 'option_profile_not_found'
	atf_add_test_case 'option_buffer_overflow'
	atf_add_test_case 'option_typedefs_not_found'
	atf_add_test_case 'option_special_missing_param'
	atf_add_test_case 'unterminated_comment'
}
