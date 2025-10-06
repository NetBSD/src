#	$NetBSD: t_pthread_abuse.sh,v 1.1 2025/10/06 13:11:56 riastradh Exp $
#
# Copyright (c) 2025 The NetBSD Foundation, Inc.
# All rights reserved.
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

pthread_abuse_head()
{

	atf_set "descr" \
	    "Test that pthread_create calls without -lpthread fail to link"
	atf_set "require.progs" "cc"
}
pthread_abuse_body()
{

	cat >test.c <<'EOF'
#include <err.h>
#include <pthread.h>
#include <stdlib.h>

static void *
start(void *cookie)
{
	return cookie;
}

int
main(void)
{
	int cookie = 123;
	pthread_t t;
	void *result;
	int error;

	error = pthread_create(&t, NULL, &start, &cookie);
	if (error)
		errc(EXIT_FAILURE, error, "pthread_create");
	error = pthread_join(t, &result);
	if (error)
		errc(EXIT_FAILURE, error, "pthread_join");
	return (result == &cookie ? 0 : EXIT_FAILURE);
}
EOF
	atf_check -s not-exit:0 \
	    -e match:'undefined reference to.*pthread_create' \
	    cc -o test test.c
	atf_check cc -o test test.c -lpthread
	atf_check ./test
	atf_check cc -o test test.c -pthread
	atf_check ./test
}

atf_init_test_cases()
{

	atf_add_test_case pthread_abuse
}
