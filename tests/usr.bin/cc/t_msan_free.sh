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
	if uname -m | grep -q "amd64" && command -v cc >/dev/null 2>&1 && \
		   ! echo __clang__ | cc -E - | grep -q __clang__; then
		# only clang with major version newer than 7 is supported
		CLANG_MAJOR=`echo __clang_major__ | cc -E - | grep -o '^[[:digit:]]'`
		if [ "$CLANG_MAJOR" -ge "7" ]; then
			SUPPORT='y'
		fi
	fi
}

atf_test_case free
free_head() {
	atf_set "descr" "Test memory sanitizer for use-after-free case"
	atf_set "require.progs" "cc paxctl"
}

atf_test_case free_profile
free_profile_head() {
	atf_set "descr" "Test memory sanitizer for use-after-free with profiling option"
	atf_set "require.progs" "cc paxctl"
}
atf_test_case free_pic
free_pic_head() {
	atf_set "descr" "Test memory sanitizer for use-after-free with position independent code (PIC) flag"
	atf_set "require.progs" "cc paxctl"
}
atf_test_case free_pie
free_pie_head() {
	atf_set "descr" "Test memory sanitizer for use-after-free with position independent execution (PIE) flag"
	atf_set "require.progs" "cc paxctl"
}

free_body(){
	cat > test.c << EOF
#include <stdlib.h>
int main() {
  int *a = (int *)malloc(sizeof(int));
  *a = 9;
  free(a);
  return *a;
}
EOF

	cc -fsanitize=memory -o test test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: MemorySanitizer: use-of-uninitialized-value" ./test
}

free_profile_body(){
	cat > test.c << EOF
#include <stdlib.h>
int main() {
  int *a = (int *)malloc(sizeof(int));
  *a = 9;
  free(a);
  return *a;
}
EOF

	cc -fsanitize=memory -o test -pg test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: MemorySanitizer: use-of-uninitialized-value" ./test
}

free_pic_body(){
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int help(int);
int main(int argc, char **argv) {return help(argc);}
EOF

	cat > pic.c << EOF
#include <stdlib.h>
int help(int argc) {
  int *a = (int *)malloc(sizeof(int));
  *a = 9;
  free(a);
  return *a;
}
EOF

	cc -fsanitize=memory -fPIC -shared -o libtest.so pic.c
	cc -o test test.c -fsanitize=memory -L. -ltest
	paxctl +a test

	export LD_LIBRARY_PATH=.
	atf_check -s ignore -o ignore -e match:"WARNING: MemorySanitizer: use-of-uninitialized-value" ./test
}
free_pie_body(){

	#check whether -pie flag is supported on this architecture
	if ! cc -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_set_skip "cc -pie not supported on this architecture"
	fi
	cat > test.c << EOF
#include <stdlib.h>
int main() {
  int *a = (int *)malloc(sizeof(int));
  *a = 9;
  free(a);
  return *a;
}
EOF

	cc -fsanitize=memory -o test -fpie -pie test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: MemorySanitizer: use-of-uninitialized-value" ./test
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
	atf_add_test_case free
	atf_add_test_case free_profile
	atf_add_test_case free_pie
	atf_add_test_case free_pic
}
