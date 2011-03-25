#	$NetBSD: t_hello.sh,v 1.2 2011/03/25 19:19:45 njoly Exp $
#
# Copyright (c) 2011 The NetBSD Foundation, Inc.
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

atf_test_case hello
hello_head() {
	atf_set "descr" "compile and run \"hello world\""
	atf_set "require.progs" "cc"
}

atf_test_case hello_pic
hello_pic_head() {
	atf_set "descr" "compile and run PIC \"hello world\""
	atf_set "require.progs" "cc"
}

hello_body() {
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int main(void) {printf("hello world\n");exit(0);}
EOF
	atf_check -s exit:0 -o ignore -e ignore cc -o hello test.c
	atf_check -s exit:0 -o inline:"hello world\n" ./hello
}

hello_pic_body() {
	cat > test.c << EOF
#include <stdlib.h>
int main(void) {callpic();exit(0);}
EOF
	cat > pic.c << EOF
#include <stdio.h>
int callpic(void) {printf("hello world\n");}
EOF

	atf_check -s exit:0 -o ignore -e ignore \
	    cc -fPIC -dPIC -shared -o libtest.so pic.c
	atf_check -s exit:0 -o ignore -e ignore \
	    cc -o hello test.c -L. -ltest

	export LD_LIBRARY_PATH=.
	atf_check -s exit:0 -o inline:"hello world\n" ./hello
}

atf_init_test_cases()
{

	atf_add_test_case hello
	atf_add_test_case hello_pic
}
