/*	$NetBSD: rumpuser_int.h,v 1.2.4.2 2008/10/19 22:18:07 haad Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* XXX */
void _kernel_lock(int);
void _kernel_unlock(int, int *);

#define KLOCK_WRAP(a)							\
do {									\
	int nlocks;							\
	_kernel_unlock(0, &nlocks);					\
	a;								\
	if (nlocks)							\
		_kernel_lock(nlocks);					\
} while (/*CONSTCOND*/0)

#define DOCALL(rvtype, call)						\
	rvtype rv;							\
	rv = call;							\
	if (rv == -1)							\
		*error = errno;						\
	else								\
		*error = 0;						\
	return rv;

#define DOCALL_KLOCK(rvtype, call)					\
	rvtype rv;							\
	int nlocks;							\
	_kernel_unlock(0, &nlocks);					\
	rv = call;							\
	if (nlocks)							\
		_kernel_lock(nlocks);					\
	if (rv == -1)							\
		*error = errno;						\
	else								\
		*error = 0;						\
	return rv;
