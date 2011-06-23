/*	$NetBSD: marvell_intr.h,v 1.17.6.1 2011/06/23 14:19:32 cherry Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#ifndef _POWERPC_MARVELL_INTR_H_
#define _POWERPC_MARVELL_INTR_H_


#ifndef _LOCORE

#define	__IMASK_T	uint64_t

#if 0
#define	PIC_VIRQ_MS_PENDING(p)	(((p) != (uint32_t)(p))			\
				    ? __builtin_clz((uint32_t((p) >> 32)) \
				    : 32 + __builtin_clz((uint32_t)(p)))
#endif


#define ICU_LEN		(64 + 32)	/* Main Interrupt(64) + GPIO(32) */

extern struct pic_ops *discovery_pic;
extern struct pic_ops *discovery_gpp_pic[4];
struct pic_ops *setup_discovery_pic(void);
struct pic_ops *setup_discovery_gpp_pic(void *, int);
void genppc_cpu_configure(void);

#endif /* !_LOCORE */

#include <powerpc/intr.h>

#endif /* _POWERPC_MARVELL_INTR_H_ */
