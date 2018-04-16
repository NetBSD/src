#	$NetBSD: t_asan_poison.sh,v 1.1.2.2 2018/04/16 02:00:10 pgoyette Exp $
#
# Copyright (c) 2018 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Siddharth Muralee.
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

SUPPORT='n'
test_target() {
	if uname -m | grep -q "amd64"; then
		SUPPORT='y'
	fi

	if uname -m | grep -q "i386"; then
		SUPPORT='y'
	fi
}

atf_test_case poison
poison_head() {
	atf_set "descr" "compile and run \"Use after Poison example\""
	atf_set "require.progs" "cc paxctl"
}

atf_test_case poison_profile
poison_profile_head() {
	atf_set "descr" "compile and run \"Use after Poison example\" with profiling option"
	atf_set "require.progs" "cc paxctl"
}

atf_test_case poison_pic
poison_pic_head() {
	atf_set "descr" "compile and run PIC \"Use after Poison example\""
	atf_set "require.progs" "cc paxctl"
}

atf_test_case poison_pie
poison_pie_head() {
	atf_set "descr" "compile and run position independent (PIE) \"Use after Poison example\""
	atf_set "require.progs" "cc paxctl"
}

atf_test_case poison32
poison32_head() {
	atf_set "descr" "compile and run \"Use after Poison example\" for/in netbsd32 emulation"
	atf_set "require.progs" "cc paxctl file diff cat"
}

atf_test_case target_not_supported
target_not_supported_head()
{
	atf_set "descr" "Test forced skip"
}

poison_body() {
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>
int foo() {
	int p = 2;
	int *a;
	ASAN_POISON_MEMORY_REGION(&p, sizeof(int));
	a=&p;
	printf("%d", *a);
}

int main() {
	foo();
	printf("CHECK\n");
	exit(0);
}
EOF
	cc -fsanitize=address -o test test.c
	paxctl +a test
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"use-after-poison" ./test
}

poison_profile_body() {
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>
int foo() {
	int p = 2;
	int *a;
	ASAN_POISON_MEMORY_REGION(&p, sizeof(int));
	a=&p;
	printf("%d", *a);
}

int main() {
	foo();
	printf("CHECK\n");
	exit(0);
}
EOF
	cc -fsanitize=address -o test -pg test.c
	paxctl +a test
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"use-after-poison" ./test
}

poison_pic_body() {
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>
int foo();
int main() {
	foo();
	printf("CHECK\n");
	exit(0);
}
EOF
	cat > pic.c << EOF
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>
int foo() {
	int p = 2;
	int *a;
	ASAN_POISON_MEMORY_REGION(&p, sizeof(int));
	a=&p;
	printf("%d", *a);
}
EOF

	cc -fPIC -fsanitize=address -shared -o libtest.so pic.c
	cc -o test test.c -fsanitize=address -L. -ltest
	paxctl +a test

	export LD_LIBRARY_PATH=.
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"use-after-poison" ./test
}

poison_pie_body() {
	# check whether this arch supports -pice
	if ! cc -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_set_skip "cc -pie not supported on this architecture"
	fi
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>
int foo() {
	int p = 2;
	int *a;
	ASAN_POISON_MEMORY_REGION(&p, sizeof(int));
	 a=&p;
	printf("%d", *a);
}

int main() {
	foo();
	printf("CHECK\n");
	exit(0);
}
EOF
	cc -fsanitize=address -fpie -pie -o test test.c
	paxctl +a test
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"use-after-poison" ./test
}

poison32_body() {
	# check whether this arch is 64bit
	if ! cc -dM -E - < /dev/null | fgrep -q _LP64; then
		atf_skip "this is not a 64 bit architecture"
	fi
	if ! cc -m32 -dM -E - < /dev/null 2>/dev/null > ./def32; then
		atf_skip "cc -m32 not supported on this architecture"
	else
		if fgrep -q _LP64 ./def32; then
		atf_fail "cc -m32 does not generate netbsd32 binaries"
	fi
fi

	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>
int foo() {
	int p = 2;
	int *a;
	ASAN_POISON_MEMORY_REGION(&p, sizeof(int));
	a=&p;
	printf("%d", *a);
}

int main() {
	foo();
	printf("CHECK\n");
	exit(0);
}
EOF
	cc -fsanitize=address -o psn32 -m32 test.c
	cc -fsanitize=address -o psn64 test.c
	file -b ./psn32 > ./ftype32
	file -b ./psn64 > ./ftype64
	if diff ./ftype32 ./ftype64 >/dev/null; then
		atf_fail "generated binaries do not differ"
	fi
	echo "32bit binaries on this platform are:"
	cat ./ftype32
	echo "While native (64bit) binaries are:"
	cat ./ftype64
	paxctl +a psn32
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"use-after-poison" ./psn32

# and another test with profile 32bit binaries
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>
int foo() {
	int p = 2;
	int *a;
	ASAN_POISON_MEMORY_REGION(&p, sizeof(int));
	a=&p;
	printf("%d", *a);
}

int main() {
	foo();
	printf("CHECK\n");
	exit(0);
}
EOF
	cc -o test -m32 -fsanitize=address -pg test.c
	paxctl +a test
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"use-after-poison" ./test
}

target_not_supported_body()
{
	atf_skip "Target is not supported"
}

atf_init_test_cases()
{
	test_target
	test $SUPPORT = 'n' && {
		atf_add_test_case target_not_supported
		return 0
	}

	atf_add_test_case poison
	atf_add_test_case poison_profile
	atf_add_test_case poison_pic
	atf_add_test_case poison_pie
	atf_add_test_case poison32
	# static option not supported
	# -static and -fsanitize=address can't be used together for compilation
	# (gcc version  5.4.0 and clang 7.1) tested on April 2nd 2018.
}
