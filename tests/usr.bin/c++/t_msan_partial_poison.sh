# Copyright (c) 2018 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by Yang Zheng.
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
	if uname -m | grep -q "amd64" && command -v c++ >/dev/null 2>&1 && \
		   ! echo __clang__ | c++ -E - | grep -q __clang__; then
		# only clang with major version newer than 7 is supported
		CLANG_MAJOR=`echo __clang_major__ | c++ -E - | grep -o '^[[:digit:]]'`
		if [ "$CLANG_MAJOR" -ge "7" ]; then
			SUPPORT='y'
		fi
	fi
}

atf_test_case partial_poison
partial_poison_head() {
	atf_set "descr" "Test memory sanitizer for __msan_partial_poison interface"
	atf_set "require.progs" "c++ paxctl"
}

atf_test_case partial_poison_profile
partial_poison_profile_head() {
	atf_set "descr" "Test memory sanitizer for __msan_partial_poison with profiling option"
	atf_set "require.progs" "c++ paxctl"
}
atf_test_case partial_poison_pic
partial_poison_pic_head() {
	atf_set "descr" "Test memory sanitizer for __msan_partial_poison with position independent code (PIC) flag"
	atf_set "require.progs" "c++ paxctl"
}
atf_test_case partial_poison_pie
partial_poison_pie_head() {
	atf_set "descr" "Test memory sanitizer for __msan_partial_poison with position independent execution (PIE) flag"
	atf_set "require.progs" "c++ paxctl"
}

partial_poison_body(){
	cat > test.cc << EOF
#include <stdint.h>
#include <sanitizer/msan_interface.h>

int main(void) {
  char x[4];
  char x_s[4] = {0x77, 0x65, 0x43, 0x21};
  __msan_partial_poison(&x, &x_s, sizeof(x_s));
  __msan_print_shadow(&x, sizeof(x_s));
  return 0;
}
EOF

	c++ -fsanitize=memory -o test test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:": 77654321" ./test
}

partial_poison_profile_body(){
	cat > test.cc << EOF
#include <stdint.h>
#include <sanitizer/msan_interface.h>

int main(void) {
  char x[4];
  char x_s[4] = {0x77, 0x65, 0x43, 0x21};
  __msan_partial_poison(&x, &x_s, sizeof(x_s));
  __msan_print_shadow(&x, sizeof(x_s));
  return 0;
}
EOF

	c++ -fsanitize=memory -o test -pg test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:": 77654321" ./test
}

partial_poison_pic_body(){
	cat > test.cc << EOF
#include <stdio.h>
#include <stdlib.h>
int help(int);
int main(int argc, char **argv) {return help(argc);}
EOF

	cat > pic.cc << EOF
#include <stdint.h>
#include <sanitizer/msan_interface.h>

int help(int argc) {
  char x[4];
  char x_s[4] = {0x77, 0x65, 0x43, 0x21};
  __msan_partial_poison(&x, &x_s, sizeof(x_s));
  __msan_print_shadow(&x, sizeof(x_s));
  return 0;
}
EOF

	c++ -fsanitize=memory -fPIC -shared -o libtest.so pic.cc
	c++ -o test test.cc -fsanitize=memory -L. -ltest
	paxctl +a test

	export LD_LIBRARY_PATH=.
	atf_check -s ignore -o ignore -e match:": 77654321" ./test
}
partial_poison_pie_body(){
	
	#check whether -pie flag is supported on this architecture
	if ! c++ -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then 
		atf_set_skip "c++ -pie not supported on this architecture"
	fi
	cat > test.cc << EOF
#include <stdint.h>
#include <sanitizer/msan_interface.h>

int main(void) {
  char x[4];
  char x_s[4] = {0x77, 0x65, 0x43, 0x21};
  __msan_partial_poison(&x, &x_s, sizeof(x_s));
  __msan_print_shadow(&x, sizeof(x_s));
  return 0;
}
EOF

	c++ -fsanitize=memory -o test -fpie -pie test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:": 77654321" ./test
}

atf_test_case target_not_supported
target_not_supported_head()
{
	atf_set "descr" "Test forced skip"
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
	atf_add_test_case partial_poison
	atf_add_test_case partial_poison_profile
	atf_add_test_case partial_poison_pie
	atf_add_test_case partial_poison_pic
}
