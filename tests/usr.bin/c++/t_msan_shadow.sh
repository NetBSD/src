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

atf_test_case shadow
shadow_head() {
	atf_set "descr" "Test memory sanitizer for shadow interface"
	atf_set "require.progs" "c++ paxctl"
}

atf_test_case shadow_profile
shadow_profile_head() {
	atf_set "descr" "Test memory sanitizer for shadow with profiling option"
	atf_set "require.progs" "c++ paxctl"
}
atf_test_case shadow_pic
shadow_pic_head() {
	atf_set "descr" "Test memory sanitizer for shadow with position independent code (PIC) flag"
	atf_set "require.progs" "c++ paxctl"
}
atf_test_case shadow_pie
shadow_pie_head() {
	atf_set "descr" "Test memory sanitizer for shadow with position independent execution (PIE) flag"
	atf_set "require.progs" "c++ paxctl"
}

shadow_body(){
	cat > test.cc << EOF
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/msan_interface.h>

int main(int argc, char **argv) {
  char str[] = "abc";
  char str2[] = "cdefghi";
  __msan_poison(str + 2, 1);
  __msan_copy_shadow(str2 + 2, str, 4);
  printf("%ld\n", __msan_test_shadow(str, 4));
  __msan_print_shadow(str2, 8);
  return 0;
}
EOF

	c++ -fsanitize=memory -o test test.cc
	paxctl +a test
	atf_check -s ignore -o match:"2" -e match:"00000000 ff000000" ./test
}

shadow_profile_body(){
	cat > test.cc << EOF
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/msan_interface.h>

int main(int argc, char **argv) {
  char str[] = "abc";
  char str2[] = "cdefghi";
  __msan_poison(str + 2, 1);
  __msan_copy_shadow(str2 + 2, str, 4);
  printf("%ld\n", __msan_test_shadow(str, 4));
  __msan_print_shadow(str2, 8);
  return 0;
}
EOF

	c++ -fsanitize=memory -static -o test -pg test.cc
	paxctl +a test
	atf_check -s ignore -o match:"2" -e match:"00000000 ff000000" ./test
}

shadow_pic_body(){
	cat > test.cc << EOF
#include <stdio.h>
#include <stdlib.h>
int help(int);
int main(int argc, char **argv) {return help(argc);}
EOF

	cat > pic.cc << EOF
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/msan_interface.h>

int help(int argc) {
  char str[] = "abc";
  char str2[] = "cdefghi";
  __msan_poison(str + 2, 1);
  __msan_copy_shadow(str2 + 2, str, 4);
  printf("%ld\n", __msan_test_shadow(str, 4));
  __msan_print_shadow(str2, 8);
  return 0;
}
EOF

	c++ -fsanitize=memory -fPIC -shared -o libtest.so pic.cc
	c++ -o test test.cc -fsanitize=memory -L. -ltest
	paxctl +a test

	export LD_LIBRARY_PATH=.
	atf_check -s ignore -o match:"2" -e match:"00000000 ff000000" ./test
}
shadow_pie_body(){

	#check whether -pie flag is supported on this architecture
	if ! c++ -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_set_skip "c++ -pie not supported on this architecture"
	fi
	cat > test.cc << EOF
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/msan_interface.h>

int main(int argc, char **argv) {
  char str[] = "abc";
  char str2[] = "cdefghi";
  __msan_poison(str + 2, 1);
  __msan_copy_shadow(str2 + 2, str, 4);
  printf("%ld\n", __msan_test_shadow(str, 4));
  __msan_print_shadow(str2, 8);
  return 0;
}
EOF

	c++ -fsanitize=memory -o test -fpie -pie test.cc
	paxctl +a test
	atf_check -s ignore -o match:"2" -e match:"00000000 ff000000" ./test
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
	atf_add_test_case shadow
	atf_add_test_case shadow_profile
	atf_add_test_case shadow_pie
	atf_add_test_case shadow_pic
}
