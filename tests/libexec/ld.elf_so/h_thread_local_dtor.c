/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <dlfcn.h>
#include <err.h>
#include <pthread.h>
#include <stdio.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

static void *
thread_helper(void *arg)
{
	void (*testfunc)(void) = arg;
	testfunc();

	pthread_mutex_lock(&mutex);
	pthread_cond_broadcast(&cond1);
	pthread_mutex_unlock(&mutex);

	pthread_mutex_lock(&mutex);
	pthread_cond_wait(&cond2, &mutex);
	pthread_mutex_unlock(&mutex);

	return NULL;
}

int
main(void)
{
	void *dso;
	void (*testfunc)(void);
	pthread_t thread;

	dso = dlopen("libh_helper_dso3.so", RTLD_LAZY);
	if (dso == NULL)
		errx(1, "%s", dlerror());
	testfunc = dlsym(dso, "testfunc");
	if (testfunc == NULL)
		errx(1, "%s", dlerror());

	pthread_mutex_lock(&mutex);

	if (pthread_create(&thread, NULL, thread_helper, testfunc))
		err(1, "pthread_create");

	pthread_cond_wait(&cond1, &mutex);
	pthread_mutex_unlock(&mutex);

	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&cond2);
	pthread_mutex_unlock(&mutex);

	printf("before dlclose\n");
	dlclose(dso);
	printf("after dlclose\n");
	dso = dlopen("libh_helper_dso3.so", RTLD_LAZY);
	if (dso == NULL)
		errx(1, "%s", dlerror());
	dlclose(dso);
	if (pthread_join(thread, NULL))
		err(1, "pthread_join");
	return 0;
}
