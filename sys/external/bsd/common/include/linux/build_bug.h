/*	$NetBSD: build_bug.h,v 1.4 2021/12/19 11:14:34 riastradh Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

#ifndef	_LINUX_BUILD_BUG_H_
#define	_LINUX_BUILD_BUG_H_

#include <lib/libkern/libkern.h>

/* Required to be false _or_ nonconstant.  */
#define	BUILD_BUG_ON(EXPR)						      \
	CTASSERT(__builtin_choose_expr(__builtin_constant_p(EXPR), !(EXPR), 1))

/* Required to be constant _and_ true.  XXX Should take optional message.  */
#define	static_assert(EXPR)		CTASSERT(EXPR)

#define	BUILD_BUG()			do {} while (0)
#define	BUILD_BUG_ON_MSG(EXPR,MSG)	BUILD_BUG_ON(EXPR)
#define	BUILD_BUG_ON_INVALID(EXPR)	((void)sizeof((long)(EXPR)))
#define	BUILD_BUG_ON_ZERO(EXPR)		((int)(0*sizeof(int[(EXPR) ? -1 : 1])))

#endif	/* _LINUX_BUILD_BUG_H_ */
