/*	$NetBSD: vmparam.h,v 1.28 2001/04/29 22:44:38 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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

/*
 * We use 8K pages on both the sun3 and sun3x.  Override PAGE_*
 * to be compile-time constants.
 */
#define	PAGE_SHIFT	13
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

#ifdef	_SUN3_
#include <machine/vmparam3.h>
#endif	/* SUN3 */
#ifdef	_SUN3X_
#include <machine/vmparam3x.h>
#endif	/* SUN3X */
#ifdef	_LKM
#define	USRSTACK KERNBASE
/* Be conservative. If an LKM is gonna be built for sun3x, define this first */
#ifndef MAXDSIZ
#define MAXDSIZ         (32*1024*1024)          /* max data size */
#endif
extern	char KERNBASE[];
#define      VM_NFREELIST            1
#endif	/* _LKM */

/* This is needed by some LKMs. */
#define VM_PHYSSEG_MAX		4

#define	VM_MDPAGE_MEMBERS	/* XXX nothing yet */
#define	VM_MDPAGE_INIT(pg)	/* XXX nothing yet */

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define	MAXSLP 		20
