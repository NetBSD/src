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

atf_test_case oom
oom_head() {
	atf_set "descr" "Test thread sanitizer for out-of-memory condition"
	atf_set "require.progs" "cc paxctl"
}

atf_test_case oom_profile
oom_profile_head() {
	atf_set "descr" "Test thread sanitizer for out-of-memory with profiling option"
	atf_set "require.progs" "cc paxctl"
}
atf_test_case oom_pic
oom_pic_head() {
	atf_set "descr" "Test thread sanitizer for out-of-memory with position independent code (PIC) flag"
	atf_set "require.progs" "cc paxctl"
}
atf_test_case oom_pie
oom_pie_head() {
	atf_set "descr" "Test thread sanitizer for out-of-memory with position independent execution (PIE) flag"
	atf_set "require.progs" "cc paxctl"
}

oom_body(){
	cat > test.c << EOF
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size > 0 && data[0] == 'b') while (1) malloc(16*1024*1024);
  return 0;
}
EOF

	cc -fsanitize=fuzzer -o test test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"ERROR: libFuzzer: out-of-memory" ./test -rss_limit_mb=30
}

oom_profile_body(){
	cat > test.c << EOF
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size > 0 && data[0] == 'b') while (1) malloc(16*1024*1024);
  return 0;
}
EOF

	cc -fsanitize=fuzzer -o test -pg test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"ERROR: libFuzzer: out-of-memory" ./test -rss_limit_mb=30
}

oom_pic_body(){
	cat > test.c << EOF
#include <stddef.h>
#include <stdint.h>
int help(const uint8_t *data, size_t size);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    return help(data, size);
}
EOF

	cat > pic.c << EOF
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int help(const uint8_t *data, size_t size) {
  if (size > 0 && data[0] == 'b') while (1) malloc(16*1024*1024);
  return 0;
}
EOF

	cc -fsanitize=fuzzer -fPIC -shared -o libtest.so pic.c
	cc -o test test.c -fsanitize=fuzzer -L. -ltest
	paxctl +a test

	export LD_LIBRARY_PATH=.
	atf_check -s ignore -o ignore -e match:"ERROR: libFuzzer: out-of-memory" ./test -rss_limit_mb=30
}
oom_pie_body(){

	#check whether -pie flag is supported on this architecture
	if ! cc -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_set_skip "cc -pie not supported on this architecture"
	fi
	cat > test.c << EOF
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size > 0 && data[0] == 'b') while (1) malloc(16*1024*1024);
  return 0;
}
EOF

	cc -fsanitize=fuzzer -o test -fpie -pie test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"ERROR: libFuzzer: out-of-memory" ./test -rss_limit_mb=30
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
	atf_add_test_case oom
	atf_add_test_case oom_profile
	atf_add_test_case oom_pie
	atf_add_test_case oom_pic
}
