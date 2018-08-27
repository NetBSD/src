/*	$NetBSD: err.h,v 1.2 2018/08/27 07:20:25 riastradh Exp $	*/

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

#ifndef _LINUX_ERR_H_
#define _LINUX_ERR_H_

/* XXX Linux uses long and int inconsistently here.  Hope this works out.  */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/systm.h>

#define	MAX_ERRNO	ELAST

static inline bool
IS_ERR_VALUE(uintptr_t n)
{
	return (n >= (uintptr_t)-MAX_ERRNO);
}

static inline void *
ERR_PTR(long error)
{
	KASSERT(error < 0);
	return (void *)(intptr_t)error;
}

static inline long
PTR_ERR(const void *ptr)
{
	KASSERT(ptr == (void *)(intptr_t)(long)(intptr_t)ptr); /* XXX Hurk!  */
	return (long)(intptr_t)ptr;
}

static inline bool
IS_ERR(const void *ptr)
{
	return IS_ERR_VALUE((uintptr_t)ptr);
}

static inline bool
IS_ERR_OR_NULL(const void *ptr)
{
	return ((ptr == NULL) || IS_ERR(ptr));
}

static inline void *
ERR_CAST(void *ptr)		/* XXX Linux declares with const.  */
{
	return ptr;
}

static inline long
PTR_ERR_OR_ZERO(const void *ptr)
{
	return (IS_ERR(ptr)? PTR_ERR(ptr) : 0);
}

#define	PTR_RET	PTR_ERR_OR_ZERO

#endif  /* _LINUX_ERR_H_ */
