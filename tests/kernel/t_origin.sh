#	$NetBSD: t_origin.sh,v 1.1 2019/06/07 21:18:16 christos Exp $
#
# Copyright (c) 2019 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Christos Zoulas.
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

atf_test_case origin_simple
origin_simple_head() {
	atf_set "descr" 'test native $ORIGIN support'
	atf_set "require.progs" "cc"
}

atf_test_case origin_simple_32
origin_simple_32_head() {
	atf_set "descr" 'test $ORIGIN support in 32 bit mode'
	atf_set "require.progs" "cc"
}

make_code() {
	cat > origin$1.c << _EOF
#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <dlfcn.h>

static const char lib[] = "libfoo$1.so";
int
main(void)
{
	void *h = dlopen(lib, RTLD_NOW);
	if (h == NULL)
		errx(EXIT_FAILURE, "dlopen: %s", dlerror());
	dlclose(h);
	return EXIT_SUCCESS;
}
_EOF

cat > libfoo$1.c << EOF
void foo(void);
void foo(void)
{
}
EOF
}

test_code() {
	local m32
	if [ -z "$1" ]; then
		m32=
	else
		m32=-m32
	fi
	atf_check -s exit:0 -o empty -e empty \
	    cc -fPIC $m32 -o origin$1 origin$1.c -Wl,-R'$ORIGIN'
	atf_check -s exit:0 -o empty -e empty \
	    cc -shared -fPIC $m32 -o libfoo$1.so libfoo$1.c 
	atf_check -s exit:0 -o empty -e empty ./origin$1
}

check32() {
	# check whether this arch is 64bit
	if ! cc -dM -E - < /dev/null | fgrep -q _LP64; then
		atf_skip "this is not a 64 bit architecture"
		return 1
	fi
	if ! cc -m32 -dM -E - < /dev/null 2>/dev/null > ./def32; then
		atf_skip "c++ -m32 not supported on this architecture"
		return 1
	else
		if fgrep -q _LP64 ./def32; then
			atf_fail "c++ -m32 does not generate netbsd32 binaries"
			return 1
		fi
		return 0
	fi
}

origin_simple_body() {
	make_code ""
	test_code ""

}

origin_simple_32_body() {
	if ! check32; then
		return
	fi
	make_code "32"
	test_code "32"
}


atf_init_test_cases()
{

	atf_add_test_case origin_simple
	atf_add_test_case origin_simple_32
}
