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
	atf_set "require.arch" "x86_64"
}

atf_test_case vptr_race
vptr_race_head() {
	atf_set "descr" "Test thread sanitizer for vptr race condition"
	atf_set "require.progs" "c++ paxctl"
	test_target
}

atf_test_case vptr_race_profile
vptr_race_profile_head() {
	atf_set "descr" "Test thread sanitizer for vptr race with profiling option"
	atf_set "require.progs" "c++ paxctl"
	test_target
}
atf_test_case vptr_race_pic
vptr_race_pic_head() {
	atf_set "descr" "Test thread sanitizer for vptr race with position independent code (PIC) flag"
	atf_set "require.progs" "c++ paxctl"
	test_target
}
atf_test_case vptr_race_pie
vptr_race_pie_head() {
	atf_set "descr" "Test thread sanitizer for vptr race with position independent execution (PIE) flag"
	atf_set "require.progs" "c++ paxctl"
	test_target
}

vptr_race_body(){
	cat > test.cc << EOF
#include <pthread.h>
pthread_barrier_t barrier;
struct A {
  volatile bool done;
  A(): done(false) { }
  virtual void Done() { done = true; }
  virtual ~A() { while (!done) ; }
};
struct B: A {};
A *obj = new B;
void *Thread1(void *x) {
  pthread_barrier_wait(&barrier);
  obj->Done();
  return NULL;
}
int main() {
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_t t;
  pthread_create(&t, NULL, Thread1, NULL);
  pthread_barrier_wait(&barrier);
  delete obj;
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -o test test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race on vptr \(ctor/dtor vs virtual call\)" ./test
}

vptr_race_profile_body(){
	atf_expect_fail "PR toolchain/55760"
	cat > test.cc << EOF
#include <pthread.h>
pthread_barrier_t barrier;
struct A {
  volatile bool done;
  A(): done(false) { }
  virtual void Done() { done = true; }
  virtual ~A() { while (!done) ; }
};
struct B: A {};
A *obj = new B;
void *Thread1(void *x) {
  pthread_barrier_wait(&barrier);
  obj->Done();
  return NULL;
}
int main() {
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_t t;
  pthread_create(&t, NULL, Thread1, NULL);
  pthread_barrier_wait(&barrier);
  delete obj;
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -o test -pg test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race on vptr \(ctor/dtor vs virtual call\)" ./test
}

vptr_race_pic_body(){
	cat > test.cc << EOF
#include <stdio.h>
#include <stdlib.h>
int help(int);
int main(int argc, char **argv) {return help(argc);}
EOF

	cat > pic.cc << EOF
#include <pthread.h>
pthread_barrier_t barrier;
struct A {
  volatile bool done;
  A(): done(false) { }
  virtual void Done() { done = true; }
  virtual ~A() { while (!done) ; }
};
struct B: A {};
A *obj = new B;
void *Thread1(void *x) {
  pthread_barrier_wait(&barrier);
  obj->Done();
  return NULL;
}
int help(int argc) {
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_t t;
  pthread_create(&t, NULL, Thread1, NULL);
  pthread_barrier_wait(&barrier);
  delete obj;
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -fPIC -shared -o libtest.so pic.cc
	c++ -o test test.cc -fsanitize=thread -L. -ltest
	paxctl +a test

	export LD_LIBRARY_PATH=.
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race on vptr \(ctor/dtor vs virtual call\)" ./test
}
vptr_race_pie_body(){

	#check whether -pie flag is supported on this architecture
	if ! c++ -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then
		atf_set_skip "c++ -pie not supported on this architecture"
	fi
	cat > test.cc << EOF
#include <pthread.h>
pthread_barrier_t barrier;
struct A {
  volatile bool done;
  A(): done(false) { }
  virtual void Done() { done = true; }
  virtual ~A() { while (!done) ; }
};
struct B: A {};
A *obj = new B;
void *Thread1(void *x) {
  pthread_barrier_wait(&barrier);
  obj->Done();
  return NULL;
}
int main() {
  pthread_barrier_init(&barrier, NULL, 2);
  pthread_t t;
  pthread_create(&t, NULL, Thread1, NULL);
  pthread_barrier_wait(&barrier);
  delete obj;
  pthread_join(t, NULL);
  return 0;
}
EOF

	c++ -fsanitize=thread -o test -fpie -pie test.cc
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: data race on vptr \(ctor/dtor vs virtual call\)" ./test
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
	atf_add_test_case vptr_race
	atf_add_test_case vptr_race_profile
	atf_add_test_case vptr_race_pie
	atf_add_test_case vptr_race_pic
}
