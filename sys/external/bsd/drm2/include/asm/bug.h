/*	$NetBSD: bug.h,v 1.3 2018/08/27 06:18:30 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _ASM_BUG_H_
#define _ASM_BUG_H_

#include <sys/cdefs.h>
#include <sys/systm.h>

#define	BUG()			panic("%s:%d: BUG!", __FILE__, __LINE__)
#define	BUG_ON(CONDITION)	KASSERT(!(CONDITION))

#define	BUILD_BUG()		do {} while (0)
#define	BUILD_BUG_ON(CONDITION)	CTASSERT(!(CONDITION))

/* XXX Rate limit?  */
#define WARN(CONDITION, FMT, ...)					\
	linux_warning((CONDITION)?					\
	    (printf("warning: %s:%d: " FMT, __FILE__, __LINE__,		\
		##__VA_ARGS__), 1)					\
	    : 0)

#define	WARN_ONCE(CONDITION, FMT, ...)					\
	WARN(CONDITION, FMT, ##__VA_ARGS__) /* XXX */

#define	WARN_ON(CONDITION)	WARN(CONDITION, "%s\n", #CONDITION)
#define	WARN_ON_SMP(CONDITION)	WARN_ON(CONDITION) /* XXX */
#define	WARN_ON_ONCE(CONDITION)	WARN_ON(CONDITION) /* XXX */

/* XXX Kludge to avoid GCC warning about statements without effect.  */
static inline int
linux_warning(int x)
{
	return x;
}

#endif  /* _ASM_BUG_H_ */
