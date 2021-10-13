#! /bin/sh
# $NetBSD: t_errors.sh,v 1.1 2021/10/13 23:33:52 rillig Exp $
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

indent=$(atf_config_get usr.bin.indent.test_indent /usr/bin/indent)
nl='
'

atf_test_case 'option_unknown'
option_unknown_body()
{
	atf_check -s 'exit:1' \
	    -e 'inline:indent: Command line: unknown option "-Z-unknown"'"$nl" \
	    "$indent" -Z-unknown
}

atf_test_case 'option_bool_trailing_garbage'
option_bool_trailing_garbage_body()
{
	atf_check -s 'exit:1' \
	    -e 'inline:indent: Command line: unknown option "-bacchus"'"$nl" \
	    "$indent" -bacchus
}

atf_test_case 'option_int_missing_parameter'
option_int_missing_parameter_body()
{
	atf_check -s 'exit:1' \
	    -e 'inline:indent: Command line: option "-ts" requires an integer parameter'"$nl" \
	    "$indent" -tsx
}

atf_init_test_cases()
{
	atf_add_test_case 'option_unknown'
	atf_add_test_case 'option_bool_trailing_garbage'
	atf_add_test_case 'option_int_missing_parameter'
}
