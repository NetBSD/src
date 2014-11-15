#	$NetBSD: t_script.sh,v 1.2 2014/11/15 03:10:01 uebayasi Exp $
#
# Copyright (c) 2014 The NetBSD Foundation, Inc.
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

atf_test_case multisec
multisec_head() {
	atf_set "descr" "check if multiple SECTIONS commands work"
	atf_set "require.progs" "cc" "ld" "readelf" "nm" "sed" "grep"
}

multisec_body() {
	cat > test.c << EOF
#include <sys/cdefs.h>
char a __section(".data.a") = 'a';
char b __section(".data.b") = 'b';
char c __section(".data.c") = 'c';
EOF
	atf_check -s exit:0 -o ignore -e ignore cc -c test.c

	cat > test.x << EOF
SECTIONS {
	.data : {
		*(.data)
		*(.data.a)
	}
}
SECTIONS {
	.data : {
		*(.data)
		*(.data.b)
	}
}
EOF
	atf_check -s exit:0 -o ignore -e ignore \
	    ld -r -T test.x -Map test.map -o test.ro test.o
	extract_section_names test.ro >test.secs
	extract_symbol_names test.ro >test.syms
	assert_nosec '\.data\.a'
	assert_nosec '\.data\.b'
	assert_sec '\.data\.c'
}

extract_section_names() {
	readelf -S "$1" |
	sed -ne '/\] \./ { s/^.*\] //; s/ .*$//; p }'
}

extract_symbol_names() {
	nm -n "$1" |
	sed -e 's/^.* //'
}

assert_sec() {
	atf_check -s exit:0 -o ignore -e ignore \
	    grep "^$1\$" test.secs
}

assert_nosec() {
	atf_check -s exit:1 -o ignore -e ignore \
	    grep "^$1\$" test.secs
}

atf_init_test_cases()
{

	atf_add_test_case multisec
}
