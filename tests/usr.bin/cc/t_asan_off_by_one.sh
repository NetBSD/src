#	$NetBSD: t_asan_off_by_one.sh,v 1.1.2.3 2018/07/28 04:38:13 pgoyette Exp $
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

atf_test_case off_by_one
off_by_one_head() {
	atf_set "descr" "compile and run \"Off by one example\""
	atf_set "require.progs" "cc paxctl"
}

atf_test_case off_by_one_profile
off_by_one_profile_head() {
	atf_set "descr" "compile and run \"Off by one example\" with profiling option"
	atf_set "require.progs" "cc paxctl"
}

atf_test_case off_by_one_pic
off_by_one_pic_head() {
	atf_set "descr" "compile and run PIC \"Off by one example\""
	atf_set "require.progs" "cc paxctl"
}

atf_test_case off_by_one_pie
off_by_one_pie_head() {
	atf_set "descr" "compile and run position independent (PIE) \"Off by one example\""
	atf_set "require.progs" "cc paxctl"
}

atf_test_case off_by_one32
off_by_one32_head() {
	atf_set "descr" "compile and run \"Off by one example\" for/in netbsd32 emulation"
	atf_set "require.progs" "cc paxctl file diff cat"
}

atf_test_case target_not_supported
target_not_supported_head()
{
	atf_set "descr" "Test forced skip"
}

off_by_one_body() {
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
void foo() {
	int arr[5];
	for (int i = 0; i <= 5 ; i++) {
		arr[i] = 0;
	}
}
void main() {foo(); printf("CHECK\n"); exit(0);}
EOF
	cc -fsanitize=address -o test test.c
	paxctl +a test
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"stack-buffer-overflow" ./test
}

off_by_one_profile_body() {
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
void foo() {
	int arr[5];
	for (int i = 0; i <= 5 ; i++) {
			arr[i] = 0;
	}
}
void main() {foo(); printf("CHECK\n"); exit(0);}
EOF
	cc -fsanitize=address -o test -pg test.c
	paxctl +a test
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"stack-buffer-overflow" ./test
}

off_by_one_pic_body() {
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
void foo();
void main() {foo(); printf("CHECK\n"); exit(0);}
EOF
	cat > pic.c << EOF
#include <stdio.h>
#include <stdlib.h>
void foo() {
	int arr[5];
	for (int i = 0; i <= 5 ; i++) {
		 arr[i] = 0;
	}
}
EOF
	cc -fPIC -fsanitize=address -shared -o libtest.so pic.c
	cc -o test test.c -fsanitize=address -L. -ltest

	export LD_LIBRARY_PATH=.
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"stack-buffer-overflow" ./test
}

off_by_one_pie_body() {
	# check whether this arch supports -pice
	if ! cc -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_set_skip "cc -pie not supported on this architecture"
	fi
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
void foo() {
	int arr[5];
	for (int i = 0; i <= 5 ; i++) {
		 arr[i] = 0;
	}
}
void main() {foo(); printf("CHECK\n"); exit(0);}
EOF
	cc -fsanitize=address -o test -fpie -pie test.c
	paxctl +a test
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"stack-buffer-overflow" ./test
}

off_by_one32_body() {
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
void foo() {
	int arr[5];
	for (int i = 0; i <= 5 ; i++) {
		arr[i] = 0;
	}
}
void main() {foo(); printf("CHECK\n"); exit(0);}
EOF
	cc -fsanitize=address -o obo32 -m32 test.c
	cc -fsanitize=address -o obo64 test.c
	file -b ./obo32 > ./ftype32
	file -b ./obo64 > ./ftype64
	if diff ./ftype32 ./ftype64 >/dev/null; then
		atf_fail "generated binaries do not differ"
	fi
	echo "32bit binaries on this platform are:"
	cat ./ftype32
	echo "While native (64bit) binaries are:"
	cat ./ftype64
	paxctl +a obo32
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"stack-buffer-overflow" ./obo32

# and another test with profile 32bit binaries
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
void foo() {
	int arr[5];
	for (int i = 0; i <= 5 ; i++) {
		arr[i] = 0;
	}
}
void main() {foo(); printf("CHECK\n"); exit(0);}
EOF
	cc -fsanitize=address -o test -pg test.c
	paxctl +a test
	atf_check -s not-exit:0 -o not-match:"CHECK\n" -e match:"stack-buffer-overflow" ./test
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

	atf_add_test_case off_by_one
#	atf_add_test_case off_by_one_profile
	atf_add_test_case off_by_one_pic
	atf_add_test_case off_by_one_pie
#	atf_add_test_case off_by_one32
	# static option not supported
	# -static and -fsanitize=address can't be used together for compilation
	# (gcc version  5.4.0 and clang 7.1) tested on April 2nd 2018.
}
