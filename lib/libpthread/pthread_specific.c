/*	$NetBSD: pthread_specific.c,v 1.21.22.3 2013/04/29 01:50:18 riz Exp $	*/

/*-
 * Copyright (c) 2001, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: pthread_specific.c,v 1.21.22.3 2013/04/29 01:50:18 riz Exp $");

/* Functions and structures dealing with thread-specific data */

#include "pthread.h"
#include "pthread_int.h"
#include "reentrant.h"

#include <sys/lwpctl.h>

__strong_alias(__libc_thr_setspecific,pthread_setspecific)
__strong_alias(__libc_thr_getspecific,pthread_getspecific)
__strong_alias(__libc_thr_curcpu,pthread_curcpu_np)

int
pthread_setspecific(pthread_key_t key, const void *value)
{
	pthread_t self;

	if (__predict_false(__uselibcstub))
		return __libc_thr_setspecific_stub(key, value);

	self = pthread__self();
	/*
	 * We can't win here on constness. Having been given a 
	 * "const void *", we can only assign it to other const void *,
	 * and return it from functions that are const void *, without
	 * generating a warning. 
	 */
	/*LINTED const cast*/
	self->pt_specific[key] = (void *) value;
	self->pt_havespecific = 1;

	return 0;
}

void *
pthread_getspecific(pthread_key_t key)
{
	if (__predict_false(__uselibcstub))
		return __libc_thr_getspecific_stub(key);

	return pthread__self()->pt_specific[key];
}

unsigned int
pthread_curcpu_np(void)
{
	if (__predict_false(__uselibcstub))
		return __libc_thr_curcpu_stub();

	return pthread__self()->pt_lwpctl->lc_curcpu;
}
