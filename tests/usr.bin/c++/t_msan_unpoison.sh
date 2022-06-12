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

atf_test_case unpoison
unpoison_head() {
	atf_set "descr" "Test memory sanitizer for __msan_unpoison interface"
	atf_set "require.progs" "c++ paxctl"
}

atf_test_case unpoison_profile
unpoison_profile_head() {
	atf_set "descr" "Test memory sanitizer for __msan_unpoison with profiling option"
	atf_set "require.progs" "c++ paxctl"
}
atf_test_case unpoison_pic
unpoison_pic_head() {
	atf_set "descr" "Test memory sanitizer for __msan_unpoison with position independent code (PIC) flag"
	atf_set "require.progs" "c++ paxctl"
}
atf_test_case unpoison_pie
unpoison_pie_head() {
	atf_set "descr" "Test memory sanitizer for __msan_unpoison with position independent execution (PIE) flag"
	atf_set "require.progs" "c++ paxctl"
}

unpoison_body(){
	cat > test.cc << EOF
#include <sanitizer/msan_interface.h>

int main(void) {
  char p[32] = {};
  char q[32] = {};
  __msan_poison(p + 10, 2);
  __msan_poison(q, 32);
  __msan_unpoison(p + 10, 2);
  __msan_unpoison_string(q);
  __msan_check_mem_is_initialized(p, 32);
  __msan_check_mem_is_initialized(p, 32);
  return 0;
}
EOF

	c++ -fsanitize=memory -o test test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e not-match:"WARNING: MemorySanitizer: use-of-uninitialized-value" ./test
}

unpoison_profile_body(){
	cat > test.cc << EOF
#include <sanitizer/msan_interface.h>

int main(void) {
  char p[32] = {};
  char q[32] = {};
  __msan_poison(p + 10, 2);
  __msan_poison(q, 32);
  __msan_unpoison(p + 10, 2);
  __msan_unpoison_string(q);
  __msan_check_mem_is_initialized(p, 32);
  __msan_check_mem_is_initialized(p, 32);
  return 0;
}
EOF

	c++ -fsanitize=memory -static -o test -pg test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e not-match:"WARNING: MemorySanitizer: use-of-uninitialized-value" ./test
}

unpoison_pic_body(){
	cat > test.cc << EOF
#include <stdio.h>
#include <stdlib.h>
int help(int);
int main(int argc, char **argv) {return help(argc);}
EOF

	cat > pic.cc << EOF
#include <sanitizer/msan_interface.h>

int help(int argc) {
  char p[32] = {};
  char q[32] = {};
  __msan_poison(p + 10, 2);
  __msan_poison(q, 32);
  __msan_unpoison(p + 10, 2);
  __msan_unpoison_string(q);
  __msan_check_mem_is_initialized(p, 32);
  __msan_check_mem_is_initialized(p, 32);
  return 0;
}
EOF

	c++ -fsanitize=memory -fPIC -shared -o libtest.so pic.cc
	c++ -o test test.cc -fsanitize=memory -L. -ltest
	paxctl +a test

	export LD_LIBRARY_PATH=.
	atf_check -s ignore -o ignore -e not-match:"WARNING: MemorySanitizer: use-of-uninitialized-value" ./test
}
unpoison_pie_body(){

	#check whether -pie flag is supported on this architecture
	if ! c++ -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_set_skip "c++ -pie not supported on this architecture"
	fi
	cat > test.cc << EOF
#include <sanitizer/msan_interface.h>

int main(void) {
  char p[32] = {};
  char q[32] = {};
  __msan_poison(p + 10, 2);
  __msan_poison(q, 32);
  __msan_unpoison(p + 10, 2);
  __msan_unpoison_string(q);
  __msan_check_mem_is_initialized(p, 32);
  __msan_check_mem_is_initialized(p, 32);
  return 0;
}
EOF

	c++ -fsanitize=memory -o test -fpie -pie test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e not-match:"WARNING: MemorySanitizer: use-of-uninitialized-value" ./test
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
	atf_add_test_case unpoison
	atf_add_test_case unpoison_profile
	atf_add_test_case unpoison_pie
	atf_add_test_case unpoison_pic
}
