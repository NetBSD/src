/*	$NetBSD: lock.h,v 1.1 1999/05/30 18:57:27 eeh Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#ifndef _MACHINE_LOCK_H
#define _MACHINE_LOCK_H

/*
 * Machine dependent spin lock operations.
 */

#if defined(_KERNEL)

#include <sparc64/sparc64/asm.h>

static void	simple_lock_init __P((__volatile struct simplelock *));
static void	simple_lock __P((__volatile struct simplelock *));
static int	simple_lock_try __P((__volatile struct simplelock *));
static void	simple_unlock __P((__volatile struct simplelock *));

static __inline__ void
simple_lock_init (alp)
	__volatile struct simplelock *alp;
{

	alp->lock_data = 0;
}

static __inline__ void
simple_lock (alp)
	__volatile struct simplelock *alp;
{

	/*
	 * If someone else holds the lock use simple
	 * reads until it is released, then retry the
	 * atomic operation. This reduces memory bus contention
	 * becaused the cache-coherency logic does not have to
	 * broadcast invalidates on the lock while we spin on it.
	 */
	while (ldstub(&alp->lock_data) != 0) {
		while (alp->lock_data != 0)
			/*void*/;
	}
}

static __inline__ int
simple_lock_try (alp)
	__volatile struct simplelock *alp;
{

	return (ldstub(&alp->lock_data) == 0);
}

static __inline__ void
simple_unlock (alp)
	__volatile struct simplelock *alp;
{

	alp->lock_data = 0;
}

#if defined(LOCKDEBUG)
#define simple_lock_dump()
#define simple_lock_freecheck(start, end)
#endif /* LOCKDEBUG */

#endif /* _KERNEL */

#endif /* _MACHINE_LOCK_H */
