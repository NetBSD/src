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

tsan_available_archs()
{
	atf_set "require.arch" "x86_64"
}

atf_test_case data_race
data_race_head() {
	atf_set "descr" "Test thread sanitizer for data race condition"
	atf_set "require.progs" "cc paxctl"
	tsan_available_archs
}

atf_test_case data_race_profile
data_race_profile_head() {
	atf_set "descr" "Test thread sanitizer for data race with profiling option"
	atf_set "require.progs" "cc paxctl"
	tsan_available_archs
}
atf_test_case data_race_pic
data_race_pic_head() {
	atf_set "descr" "Test thread sanitizer for data race with position independent code (PIC) flag"
	atf_set "require.progs" "cc paxctl"
	tsan_available_archs
}
atf_test_case data_race_pie
data_race_pie_head() {
	atf_set "descr" "Test thread sanitizer for data race with position independent execution (PIE) flag"
	atf_set "require.progs" "cc paxctl"
	tsan_available_archs
}

data_race_body(){
	cat > test.c << EOF
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

	cc -fsanitize=thread -o test test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race " ./test
}

data_race_profile_body(){
	atf_expect_fail "PR toolchain/55760"
	cat > test.c << EOF
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

	cc -fsanitize=thread -static -o test -pg test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race " ./test
}

data_race_pic_body(){
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int help(int);
int main(int argc, char **argv) {return help(argc);}
EOF

	cat > pic.c << EOF
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

	cc -fsanitize=thread -fPIC -shared -o libtest.so pic.c
	cc -o test test.c -fsanitize=thread -L. -ltest
	paxctl +a test

	export LD_LIBRARY_PATH=.
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race " ./test
}
data_race_pie_body(){

	#check whether -pie flag is supported on this architecture
	if ! cc -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_set_skip "cc -pie not supported on this architecture"
	fi
	cat > test.c << EOF
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

	cc -fsanitize=thread -o test -fpie -pie test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race " ./test
}

atf_init_test_cases()
{
	atf_add_test_case data_race
	atf_add_test_case data_race_profile
	atf_add_test_case data_race_pie
	atf_add_test_case data_race_pic
}
