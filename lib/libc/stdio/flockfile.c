/*	$NetBSD: flockfile.c,v 1.3 2003/01/21 23:26:03 nathanw Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: flockfile.c,v 1.3 2003/01/21 23:26:03 nathanw Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "reentrant.h"
#include "local.h"

#ifdef __weak_alias
__weak_alias(flockfile,_flockfile)
__weak_alias(ftrylockfile,_ftrylockfile)
__weak_alias(funlockfile,_funlockfile)
#endif

#ifdef _REENTRANT
/*
 * XXX This code makes the assumption that a thr_t (pthread_t) is a 
 * XXX pointer.
 */
void
__smutex_init(mutex_t *m)
{
	mutexattr_t attr;

	mutexattr_init(&attr);
	mutexattr_settype(&attr, MUTEX_TYPE_RECURSIVE);

	mutex_init(m, &attr);

	mutexattr_destroy(&attr);
}

extern int __isthreaded;

void
flockfile(FILE *fp)
{

	if (__isthreaded == 0)
		return;

	mutex_lock(&_LOCK(fp));
}

int
ftrylockfile(FILE *fp)
{

	if (__isthreaded == 0)
		return 0;

	return mutex_trylock(&_LOCK(fp));
}

void
funlockfile(FILE *fp)
{

	if (__isthreaded == 0)
		return;

	mutex_unlock(&_LOCK(fp));
}

#else /* _REENTRANT */

void
flockfile(FILE *fp)
{
	/* LINTED deliberate lack of effect */
	(void)fp;

	return;
}

int
ftrylockfile(FILE *fp)
{
	/* LINTED deliberate lack of effect */
	(void)fp;

	return (0);
}

void
funlockfile(FILE *fp)
{
	/* LINTED deliberate lack of effect */
	(void)fp;

	return;
}

#endif /* _REENTRANT */
