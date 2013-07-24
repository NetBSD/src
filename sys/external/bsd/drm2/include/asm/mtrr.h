/*	$NetBSD: mtrr.h,v 1.1.2.2 2013/07/24 02:03:46 riastradh Exp $	*/

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

#ifndef _ASM_MTRR_H_
#define _ASM_MTRR_H_

#ifdef _KERNEL_OPT
#include "opt_mtrr.h"
#endif

#include <machine/mtrr.h>

#define	MTRR_TYPE_WRCOMB	MTRR_TYPE_WC

static inline int
mtrr_add(unsigned long base, unsigned long size, int type,
    bool increment __unused)
{
#ifdef MTRR
	struct mtrr mtrr;
	int n = 1;

	mtrr.base = base;
	mtrr.len = size;
	mtrr.type = type;
	mtrr.flags = MTRR_VALID;

	/* XXX errno NetBSD->Linux */
	return -mtrr_set(&mtrr, &n, NULL, MTRR_GETSET_KERNEL);
#else
	return 0;
#endif
}

static inline int
mtrr_del(int handle __unused, unsigned long base, unsigned long size)
{
#ifdef MTRR
	struct mtrr mtrr;
	int n = 1;

	mtrr.base = base;
	mtrr.len = size;
	mtrr.type = 0;
	mtrr.flags = 0;		/* not MTRR_VALID */

	/* XXX errno NetBSD->Linux */
	return -mtrr_set(&mtrr, &n, NULL, MTRR_GETSET_KERNEL);
#else
	return 0;
#endif
}

#endif  /* _ASM_MTRR_H_ */
