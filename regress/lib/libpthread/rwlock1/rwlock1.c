/*	$NetBSD: rwlock1.c,v 1.1 2004/08/03 11:36:23 yamt Exp $	*/

/*-
 * Copyright (c)2004 YAMAMOTO Takashi,
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

pthread_rwlock_t lk;

int main(void);
void *do_nothing(void *);

struct timespec to = {1, 0};

/* ARGSUSED */
void *
do_nothing(void *dummy)
{

	return NULL;
}

int
main()
{
	int error;
	pthread_t t;

	error = pthread_create(&t, NULL, do_nothing, NULL);
	if (error)
		exit(7);

	error = pthread_rwlock_init(&lk, NULL);
	if (error)
		exit(1);

	error = pthread_rwlock_rdlock(&lk);
	if (error)
		exit(2);

	error = pthread_rwlock_rdlock(&lk);
	if (error)
		exit(3);

	error = pthread_rwlock_unlock(&lk);
	if (error)
		exit(4);

	error = pthread_rwlock_trywrlock(&lk);
	if (error != EBUSY)
		exit(5);

	error = pthread_rwlock_timedwrlock(&lk, &to);
	if (error != ETIMEDOUT && error != EDEADLK)
		exit(6);

	exit(0);

	/* NOTREACHED */
}
