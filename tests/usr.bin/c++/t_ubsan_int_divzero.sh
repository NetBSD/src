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

atf_test_case int_divzero
int_divzero_head() {
	atf_set "descr" "Test Undefined Behavior for int division with zero"
	atf_set "require.progs" "c++"
}

atf_test_case int_divzero_profile
int_divzero_profile_head() {
	atf_set "descr" "Test Undefined Behavior for int division with zero with profiling option"
	atf_set "require.progs" "c++"
}
atf_test_case int_divzero_pic
int_divzero_pic_head() {
	atf_set "descr" "Test Undefined Behavior for int division with zero with position independent code (PIC) flag"
	atf_set "require.progs" "c++"
}
atf_test_case int_divzero_pie
int_divzero_pie_head() {
	atf_set "descr" "Test Undefined Behavior for int division with zero with position independent execution (PIE) flag"
	atf_set "require.progs" "c++"
}
atf_test_case int_divzero32
int_divzero32_head() {
	atf_set "descr" "Test Undefined Behavior for int division with zero in NetBSD_32 emulation"
	atf_set "require.progs" "c++ file diff cat"
}


int_divzero_body(){
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {volatile int l = argc; volatile int k = 0; l/= k; return l;}
EOF

	c++ -fsanitize=integer-divide-by-zero -o test test.c 
	atf_check -s signal:8 -e match:"division by zero" ./test
}

int_divzero_profile_body(){
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {volatile int l = argc; volatile int k = 0; l/= k; return l;}
EOF

	c++ -fsanitize=integer-divide-by-zero -o test -pg test.c 
	atf_check -s signal:8 -e match:"division by zero" ./test
}

int_divzero_pic_body(){
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int help(int);
int main(int argc, char **argv) {return help(argc);}
EOF

	cat > pic.c << EOF
#include <stdlib.h>
#include <stdio.h>
int help(int count) {volatile int l = count; volatile int k = 0; return l/k;}
EOF

	c++ -fsanitize=integer-divide-by-zero -fPIC -shared -o libtest.so pic.c
	c++ -o test test.c -fsanitize=integer-divide-by-zero -L. -ltest

	export LD_LIBRARY_PATH=.
	atf_check -s signal:8 -e match:"division by zero" ./test
}
int_divzero_pie_body(){
	
	#check whether -pie flag is supported on this architecture
	if ! c++ -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then 
		atf_set_skip "c++ -pie not supported on this architecture"
	fi
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {volatile int l = argc; int k = 0; l/= k; return l;}
EOF

	c++ -fsanitize=integer-divide-by-zero -o test -fpie -pie test.c 
	atf_check -s signal:8 -e match:"division by zero" ./test
}


int_divzero32_body(){
	
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
int main(int argc, char **argv) {int l = argc; int k = 0; l/= k; return l;}
EOF

	c++ -fsanitize=integer-divide-by-zero -o md32 -m32 test.c
	c++ -fsanitize=integer-divide-by-zero -o md64 test.c
	file -b ./md32 > ./ftype32
	file -b ./md64 > ./ftype64
	if diff ./ftype32 ./ftype64 >/dev/null; then
		atf_fail "Generated 32bit binaries do not differ from 64bit ones"
	fi
	echo "32bit binaries on this platform are:"
	cat ./ftype32
	echo "64bit binaries are on the other hand:"
	cat ./ftype64
	atf_check -s signal:8 -e match:"division by zero" ./md32

	# Another test with profile 32bit binaries, just to make sure everything has been thoroughly done
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {int l = argc; int k = 0; l /= k; return l;}
EOF

	c++ -fsanitize=integer-divide-by-zero -pg -m32 -o test test.c
	atf_check -s signal:8  -e match:"division by zero" ./test
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
	atf_add_test_case int_divzero
#	atf_add_test_case int_divzero_profile
	atf_add_test_case int_divzero_pie
	atf_add_test_case int_divzero_pic
#	atf_add_test_case int_divzero32
}
