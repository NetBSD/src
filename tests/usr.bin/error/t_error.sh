# $NetBSD: t_error.sh,v 1.1 2023/08/26 10:06:16 rillig Exp $
#
# Copyright (c) 2023 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Roland Illig.
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

create_file() {
	local fn="$1"; shift
	printf '%s\n' "$@" > "$fn"
}

atf_test_case cc
cc_body() {
	create_file code.c \
	    'goto error'
	create_file err \
	    'code.c:1: error: syntax error'
	create_file expected \
	    '/*###1 [cc] error: syntax error%%%*/' \
	    'goto error'

	atf_check -o ignore \
	    error err
	atf_check -o 'file:expected' cat code.c
}

atf_test_case f77
f77_body() {
	create_file code.f \
		'doi=1,1,1'
	create_file err \
		'Compiler error line 1 of code.f: syntax error'
	create_file expected \
	    'C###1 [f77] Compiler error line 1 of code.f syntax error%%%' \
	    'doi=1,1,1'

	atf_check -o ignore \
	    error err
	atf_check -o 'file:expected' cat code.f
}

atf_test_case lint
lint_body() {
	create_file code.c \
	    'goto error'
	create_file err \
	    'code.c(1): syntax error'
	create_file expected \
	    '/*###1 [lint] syntax error%%%*/' \
	    'goto error'

	atf_check -o ignore \
	    error err
	atf_check -o 'file:expected' cat code.c
}

atf_test_case mod2
mod2_body() {
	create_file code.m2 \
	    'END.'
	create_file err \
	    'File code.m2, line 1: missing BEGIN'
	create_file expected \
	    '(*###1 [mod2] missing BEGIN%%%*)' \
	    'END.'

	atf_check -o ignore \
	    error err
	atf_check -o 'file:expected' cat code.m2
}

atf_init_test_cases() {
	atf_add_test_case cc
	atf_add_test_case f77
	atf_add_test_case lint
	atf_add_test_case mod2
}
