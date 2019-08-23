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

atf_test_case data_race
data_race_head() {
	atf_set "descr" "Test thread sanitizer for data race condition"
	atf_set "require.progs" "c++ paxctl"
}

atf_test_case data_race_profile
data_race_profile_head() {
	atf_set "descr" "Test thread sanitizer for data race with profiling option"
	atf_set "require.progs" "c++ paxctl"
}
atf_test_case data_race_pic
data_race_pic_head() {
	atf_set "descr" "Test thread sanitizer for data race with position independent code (PIC) flag"
	atf_set "require.progs" "c++ paxctl"
}
atf_test_case data_race_pie
data_race_pie_head() {
	atf_set "descr" "Test thread sanitizer for data race with position independent execution (PIE) flag"
	atf_set "require.progs" "c++ paxctl"
}

data_race_body(){
	cat > test.cc << EOF
#include <pthread.h>
int GlobalData; pthread_barrier_t barrier;
void *Thread(void *a) { pthread_barrier_wait(&barrier); GlobalData = 42; return 0; }
int main() {
  pthread_t t;
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_create(&t, NULL, Thread, NULL);
  pthread_barrier_wait(&barrier);
  GlobalData = 43;
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -o test test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race " ./test
}

data_race_profile_body(){
	cat > test.cc << EOF
#include <pthread.h>
int GlobalData; pthread_barrier_t barrier;
void *Thread(void *a) { pthread_barrier_wait(&barrier); GlobalData = 42; return 0; }
int main() {
  pthread_t t;
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_create(&t, NULL, Thread, NULL);
  pthread_barrier_wait(&barrier);
  GlobalData = 43;
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -o test -pg test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race " ./test
}

data_race_pic_body(){
	cat > test.cc << EOF
#include <stdio.h>
#include <stdlib.h>
int help(int);
int main(int argc, char **argv) {return help(argc);}
EOF

	cat > pic.cc << EOF
#include <pthread.h>
int GlobalData; pthread_barrier_t barrier;
void *Thread(void *a) { pthread_barrier_wait(&barrier); GlobalData = 42; return 0; }
int help(int argc) {
  pthread_t t;
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_create(&t, NULL, Thread, NULL);
  pthread_barrier_wait(&barrier);
  GlobalData = 43;
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -fPIC -shared -o libtest.so pic.cc
	c++ -o test test.cc -fsanitize=thread -L. -ltest
	paxctl +a test

	export LD_LIBRARY_PATH=.
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race " ./test
}
data_race_pie_body(){
	
	#check whether -pie flag is supported on this architecture
	if ! c++ -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then 
		atf_set_skip "c++ -pie not supported on this architecture"
	fi
	cat > test.cc << EOF
#include <pthread.h>
int GlobalData; pthread_barrier_t barrier;
void *Thread(void *a) { pthread_barrier_wait(&barrier); GlobalData = 42; return 0; }
int main() {
  pthread_t t;
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_create(&t, NULL, Thread, NULL);
  pthread_barrier_wait(&barrier);
  GlobalData = 43;
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -o test -fpie -pie test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race " ./test
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
	atf_add_test_case data_race
	atf_add_test_case data_race_profile
	atf_add_test_case data_race_pie
	atf_add_test_case data_race_pic
}
