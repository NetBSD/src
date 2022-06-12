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

atf_test_case locked_mutex_destroy
locked_mutex_destroy_head() {
	atf_set "descr" "Test thread sanitizer for destroying locked mutex condition"
	atf_set "require.progs" "c++ paxctl"
	tsan_available_archs
}

atf_test_case locked_mutex_destroy_profile
locked_mutex_destroy_profile_head() {
	atf_set "descr" "Test thread sanitizer for destroying locked mutex with profiling option"
	atf_set "require.progs" "c++ paxctl"
	tsan_available_archs
}
atf_test_case locked_mutex_destroy_pic
locked_mutex_destroy_pic_head() {
	atf_set "descr" "Test thread sanitizer for destroying locked mutex with position independent code (PIC) flag"
	atf_set "require.progs" "c++ paxctl"
	tsan_available_archs
}
atf_test_case locked_mutex_destroy_pie
locked_mutex_destroy_pie_head() {
	atf_set "descr" "Test thread sanitizer for destroying locked mutex with position independent execution (PIE) flag"
	atf_set "require.progs" "c++ paxctl"
	tsan_available_archs
}

locked_mutex_destroy_body(){
	cat > test.cc << EOF
#include <pthread.h>
#include <stdlib.h>

pthread_mutex_t mutex;
pthread_barrier_t barrier;
void *Thread(void *a) {
  pthread_mutex_lock(&mutex);
  pthread_barrier_wait(&barrier);
  return 0;
}

int main() {
  pthread_t t;
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_mutex_init(&mutex, NULL);
  pthread_create(&t, NULL, Thread, NULL);
  pthread_barrier_wait(&barrier);
  pthread_mutex_destroy(&mutex);
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -o test test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: destroy of a locked mutex" ./test
}

locked_mutex_destroy_profile_body(){
	atf_expect_fail "PR toolchain/55760"
	cat > test.cc << EOF
#include <pthread.h>
#include <stdlib.h>

pthread_mutex_t mutex;
pthread_barrier_t barrier;
void *Thread(void *a) {
  pthread_mutex_lock(&mutex);
  pthread_barrier_wait(&barrier);
  return 0;
}

int main() {
  pthread_t t;
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_mutex_init(&mutex, NULL);
  pthread_create(&t, NULL, Thread, NULL);
  pthread_barrier_wait(&barrier);
  pthread_mutex_destroy(&mutex);
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -static -o test -pg test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: destroy of a locked mutex" ./test
}

locked_mutex_destroy_pic_body(){
	cat > test.cc << EOF
#include <stdio.h>
#include <stdlib.h>
int help(int);
int main(int argc, char **argv) {return help(argc);}
EOF

	cat > pic.cc << EOF
#include <pthread.h>
#include <stdlib.h>

pthread_mutex_t mutex;
pthread_barrier_t barrier;
void *Thread(void *a) {
  pthread_mutex_lock(&mutex);
  pthread_barrier_wait(&barrier);
  return 0;
}

int help(int argc) {
  pthread_t t;
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_mutex_init(&mutex, NULL);
  pthread_create(&t, NULL, Thread, NULL);
  pthread_barrier_wait(&barrier);
  pthread_mutex_destroy(&mutex);
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -fPIC -shared -o libtest.so pic.cc
	c++ -o test test.cc -fsanitize=thread -L. -ltest
	paxctl +a test

	export LD_LIBRARY_PATH=.
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: destroy of a locked mutex" ./test
}
locked_mutex_destroy_pie_body(){

	#check whether -pie flag is supported on this architecture
	if ! c++ -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_set_skip "c++ -pie not supported on this architecture"
	fi
	cat > test.cc << EOF
#include <pthread.h>
#include <stdlib.h>

pthread_mutex_t mutex;
pthread_barrier_t barrier;
void *Thread(void *a) {
  pthread_mutex_lock(&mutex);
  pthread_barrier_wait(&barrier);
  return 0;
}

int main() {
  pthread_t t;
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_mutex_init(&mutex, NULL);
  pthread_create(&t, NULL, Thread, NULL);
  pthread_barrier_wait(&barrier);
  pthread_mutex_destroy(&mutex);
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -o test -fpie -pie test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: destroy of a locked mutex" ./test
}


atf_init_test_cases()
{
	atf_add_test_case locked_mutex_destroy
	atf_add_test_case locked_mutex_destroy_profile
	atf_add_test_case locked_mutex_destroy_pie
	atf_add_test_case locked_mutex_destroy_pic
}
