/*	$NetBSD: lock.h,v 1.5 2000/07/01 06:41:06 matt Exp $	*/

/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _VAX_LOCK_H_
#define _VAX_LOCK_H_

typedef	__volatile int		__cpu_simple_lock_t;

#define	__SIMPLELOCK_LOCKED	1
#define	__SIMPLELOCK_UNLOCKED	0

static __inline void
__cpu_simple_lock_init(__cpu_simple_lock_t *alp)
{
	*alp = __SIMPLELOCK_UNLOCKED;
}

static __inline void
__cpu_simple_lock(__cpu_simple_lock_t *alp)
{
	__asm__ __volatile ("1:;bbssi $0, %0, 1b"
		: /* No output */
		: "m"(*alp));
}

static __inline void
__cpu_simple_unlock(__cpu_simple_lock_t *alp)
{
	*alp = __SIMPLELOCK_UNLOCKED;
}

static __inline int
__cpu_simple_lock_try(__cpu_simple_lock_t *alp)
{
	int ret;

	__asm__ __volatile ("movl $0,%0;bbssi $0,%1,1f;incl %0;1:"
		: "=&r"(ret)
		: "m"(*alp));

	return ret;
}

#endif /* _VAX_LOCK_H_ */
