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

atf_test_case signal_errno
signal_errno_head() {
	atf_set "descr" "Test thread sanitizer for errno modification in signal condition"
	atf_set "require.progs" "cc paxctl"
}

atf_test_case signal_errno_profile
signal_errno_profile_head() {
	atf_set "descr" "Test thread sanitizer for errno modification in signal with profiling option"
	atf_set "require.progs" "cc paxctl"
}
atf_test_case signal_errno_pic
signal_errno_pic_head() {
	atf_set "descr" "Test thread sanitizer for errno modification in signal with position independent code (PIC) flag"
	atf_set "require.progs" "cc paxctl"
}
atf_test_case signal_errno_pie
signal_errno_pie_head() {
	atf_set "descr" "Test thread sanitizer for errno modification in signal with position independent execution (PIE) flag"
	atf_set "require.progs" "cc paxctl"
}

signal_errno_body(){
	cat > test.c << EOF
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

pthread_t mainth;
static void MyHandler(int a, siginfo_t *s, void *c) { errno = 1; }
static void* sendsignal(void *p) { pthread_kill(mainth, SIGPROF); return NULL; }
int main() {
  mainth = pthread_self();
  struct sigaction act = {};
  act.sa_sigaction = &MyHandler;
  sigaction(SIGPROF, &act, 0);
  pthread_t th;
  pthread_create(&th, 0, sendsignal, 0);
  pthread_join(th, 0);
  return 0;
}
EOF

	cc -fsanitize=thread -o test test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: signal handler spoils errno" ./test
}

signal_errno_profile_body(){
	cat > test.c << EOF
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

pthread_t mainth;
static void MyHandler(int a, siginfo_t *s, void *c) { errno = 1; }
static void* sendsignal(void *p) { pthread_kill(mainth, SIGPROF); return NULL; }
int main() {
  mainth = pthread_self();
  struct sigaction act = {};
  act.sa_sigaction = &MyHandler;
  sigaction(SIGPROF, &act, 0);
  pthread_t th;
  pthread_create(&th, 0, sendsignal, 0);
  pthread_join(th, 0);
  return 0;
}
EOF

	cc -fsanitize=thread -o test -pg test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: signal handler spoils errno" ./test
}

signal_errno_pic_body(){
	cat > test.c << EOF
#include <stdio.h>
#include <stdlib.h>
int help(int);
int main(int argc, char **argv) {return help(argc);}
EOF

	cat > pic.c << EOF
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

pthread_t mainth;
static void MyHandler(int a, siginfo_t *s, void *c) { errno = 1; }
static void* sendsignal(void *p) { pthread_kill(mainth, SIGPROF); return NULL; }
int help(int argc) {
  mainth = pthread_self();
  struct sigaction act = {};
  act.sa_sigaction = &MyHandler;
  sigaction(SIGPROF, &act, 0);
  pthread_t th;
  pthread_create(&th, 0, sendsignal, 0);
  pthread_join(th, 0);
  return 0;
}
EOF

	cc -fsanitize=thread -fPIC -shared -o libtest.so pic.c
	cc -o test test.c -fsanitize=thread -L. -ltest
	paxctl +a test

	export LD_LIBRARY_PATH=.
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: signal handler spoils errno" ./test
}
signal_errno_pie_body(){
	
	#check whether -pie flag is supported on this architecture
	if ! cc -pie -dM -E - < /dev/null 2>/dev/null >/dev/null; then 
		atf_set_skip "cc -pie not supported on this architecture"
	fi
	cat > test.c << EOF
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

pthread_t mainth;
static void MyHandler(int a, siginfo_t *s, void *c) { errno = 1; }
static void* sendsignal(void *p) { pthread_kill(mainth, SIGPROF); return NULL; }
int main() {
  mainth = pthread_self();
  struct sigaction act = {};
  act.sa_sigaction = &MyHandler;
  sigaction(SIGPROF, &act, 0);
  pthread_t th;
  pthread_create(&th, 0, sendsignal, 0);
  pthread_join(th, 0);
  return 0;
}
EOF

	cc -fsanitize=thread -o test -fpie -pie test.c
	paxctl +a test
	atf_check -s ignore -o ignore -e match:"WARNING: ThreadSanitizer: signal handler spoils errno" ./test
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
	atf_add_test_case signal_errno
	atf_add_test_case signal_errno_profile
	atf_add_test_case signal_errno_pie
	atf_add_test_case signal_errno_pic
}
