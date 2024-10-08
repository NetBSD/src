/*	$NetBSD: container_of.h,v 1.1 2024/10/08 22:53:20 christos Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

#ifndef _SYS_CONTAINER_OF_H_
#define _SYS_CONTAINER_OF_H_

#include <sys/stddef.h>

/*
 * Return the container of an embedded struct.  Given x = &c->f,
 * container_of(x, T, f) yields c, where T is the type of c.  Example:
 *
 *	struct foo { ... };
 *	struct bar {
 *		int b_x;
 *		struct foo b_foo;
 *		...
 *	};
 *
 *	struct bar b;
 *	struct foo *fp = &b.b_foo;
 *
 * Now we can get at b from fp by:
 *
 *	struct bar *bp = container_of(fp, struct bar, b_foo);
 *
 * The 0*sizeof((PTR) - ...) causes the compiler to warn if the type of
 * *fp does not match the type of struct bar::b_foo.
 * We skip the validation for coverity runs to avoid warnings.
 */
#if defined(__COVERITY__) || defined(__LGTM_BOT__)
#define __validate_container_of(PTR, TYPE, FIELD) 0
#define __validate_const_container_of(PTR, TYPE, FIELD) 0
#else
#define __validate_container_of(PTR, TYPE, FIELD)			\
    (0 * sizeof((PTR) - &((TYPE *)(((char *)(PTR)) -			\
    offsetof(TYPE, FIELD)))->FIELD))
#define __validate_const_container_of(PTR, TYPE, FIELD)			\
    (0 * sizeof((PTR) - &((const TYPE *)(((const char *)(PTR)) -	\
    offsetof(TYPE, FIELD)))->FIELD))
#endif

#define	container_of(PTR, TYPE, FIELD)					\
    ((TYPE *)(((char *)(PTR)) - offsetof(TYPE, FIELD))			\
	+ __validate_container_of(PTR, TYPE, FIELD))
#define	const_container_of(PTR, TYPE, FIELD)				\
    ((const TYPE *)(((const char *)(PTR)) - offsetof(TYPE, FIELD))	\
	+ __validate_const_container_of(PTR, TYPE, FIELD))

#endif /* !_SYS_CONTAINER_OF_H_ */
