# Copyright (c) 2018 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Harry Pantazis.
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

test_target()
{
	SUPPORT='n'
	if ! echo __GNUC__ | c++ -E - | grep -q __GNUC__; then
		SUPPORT='y'
	fi

	if ! echo __clang__ | c++ -E - | grep -q __clang__; then
		SUPPORT='y'
	fi
}

atf_test_case vla_out_of_bounds
vla_out_of_bounds_head() {
	atf_set "descr" "Test Undefined Behavior for vla (Variable Length Array) out of bounds"
	atf_set "require.progs" "c++"
}

atf_test_case vla_out_of_bounds_profile
vla_out_of_bounds_profile_head() {
	atf_set "descr" "Test Undefined Behavior for vla (Variable Length Array) out of bounds with profiling option"
	atf_set "require.progs" "c++"
}
atf_test_case vla_out_of_bounds_pic
vla_out_of_bounds_pic_head() {
	atf_set "descr" "Test Undefined Behavior for vla (Variable Length Array) out of bounds with position independent code (PIC) flag"
	atf_set "require.progs" "c++"
}
atf_test_case vla_out_of_bounds_pie
vla_out_of_bounds_pie_head() {
	atf_set "descr" "Test Undefined Behavior for vla (Variable Length Array) out of bounds with position independent execution (PIE) flag"
	atf_set "require.progs" "c++"
}
atf_test_case vla_out_of_bounds32
vla_out_of_bounds32_head() {
	atf_set "descr" "Test Undefined Behavior for vla (Variable Length Array) out of bounds in NetBSD_32 emulation"
	atf_set "require.progs" "c++ file diff cat"
}


vla_out_of_bounds_body(){
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {volatile int val1 = argc, val2 = argc+1; volatile int arr[val1]; arr[val2] = argc; exit(0);}
EOF

	c++ -fsanitize=undefined -o test test.c
	atf_check -e match:"out of bounds" ./test
}

vla_out_of_bounds_profile_body(){
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {volatile int val1 = argc, val2 = argc+1; volatile int arr[val1]; arr[val2] = argc; exit(0);}
EOF

	c++ -fsanitize=undefined -o test -pg test.c
	atf_check -e match:"out of bounds" ./test
}

vla_out_of_bounds_pic_body(){
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
void help(int);
int main(int argc, char **argv) {help(argc); exit(0);}
EOF

	cat > pic.c << EOF
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
void help(int count) {volatile int val1 = count, val2 = count+1; volatile int arr[val1]; arr[val2] = count; }
EOF

	c++ -fsanitize=undefined -fPIC -shared -o libtest.so pic.c
	c++ -o test test.c -fsanitize=undefined -L. -ltest

	export LD_LIBRARY_PATH=.
	atf_check -e match:"out of bounds" ./test
}

vla_out_of_bounds_pie_body(){

	#check whether -pie flag is supported on this architecture
	if ! c++ -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_set_skip "c++ -pie not supported on this architecture"
	fi
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {volatile int val1 = argc, val2 = argc+1; volatile int arr[val1]; arr[val2] = argc; exit(0);}
EOF

	c++ -fsanitize=undefined -o test -fpie -pie test.c
	atf_check -e match:"out of bounds" ./test
}


vla_out_of_bounds32_body(){

	# check what this architecture is, after all
	if ! c++ -dM -E - < /dev/null | grep -F -q _LP64; then
		atf_skip "This is not a 64 bit architecture"
	fi
	if ! c++ -m32 -dM -E - < /dev/null 2>/dev/null > ./def32; then
		atf_skip "c++ -m32 Not supported on this architecture"
	else
		if grep -F -q _LP64 ./def32; then
		atf_fail "c++ -m32 Does not generate NetBSD32 binaries"
		fi
	fi

	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {volatile int val1 = argc, val2 = argc+1; volatile int arr[val1]; arr[val2] = argc; exit(0);}
EOF

	c++ -fsanitize=undefined -o md32 -m32 test.c
	c++ -fsanitize=undefined -o md64 test.c
	file -b ./md32 > ./ftype32
	file -b ./md64 > ./ftype64
	if diff ./ftype32 ./ftype64 >/dev/null; then
		atf_fail "Generated 32bit binaries do not differ from 64bit ones"
	fi
	echo "32bit binaries on this platform are:"
	cat ./ftype32
	echo "64bit binaries are on the other hand:"
	cat ./ftype64
	atf_check -e match:"out of bounds" ./md32

	# Another test with profile 32bit binaries, just to make sure everything has been thoroughly done
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) { volatile int val1 = argc, val2 = argc+1; volatile int arr[val1]; arr[val2] = argc; exit(0);}
EOF

	c++ -fsanitize=undefined -pg -m32 -o test test.c
	atf_check -e match:"out of bounds" ./test
}

atf_test_case target_not_supported
target_not_supported_head()
{
	atf_set "descr" "Test forced skip"
}

atf_init_test_cases()
{
	test_target
	test $SUPPORT = 'n' && {
		atf_add_test_case target_not_supported
		return 0
	}
	atf_add_test_case vla_out_of_bounds
#	atf_add_test_case vla_out_of_bounds_profile
	atf_add_test_case vla_out_of_bounds_pie
	atf_add_test_case vla_out_of_bounds_pic
#	atf_add_test_case vla_out_of_bounds32
}
