/*	$NetBSD: softint.h,v 1.1.16.1 2014/08/20 00:03:19 tls Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#ifndef _POWERPC_SOFTINT_H_
#define _POWERPC_SOFTINT_H_

#ifdef __HAVE_FAST_SOFTINTS

#ifdef __INTR_PRIVATE
#ifdef __HAVE_PREEMPTION
#define	IPL_PREEMPT_SOFTMASK	(1 << IPL_NONE)
#else
#define	IPL_PREEMPT_SOFTMASK	0
#endif

#define	IPL_SOFTMASK \
	    ((1 << IPL_SOFTSERIAL) | (1 << IPL_SOFTNET   )	\
	    |(1 << IPL_SOFTBIO   ) | (1 << IPL_SOFTCLOCK )	\
	    |IPL_PREEMPT_SOFTMASK)

#define SOFTINT2IPL_MAP \
	    ((IPL_SOFTSERIAL << (4*SOFTINT_SERIAL))	\
	    |(IPL_SOFTNET    << (4*SOFTINT_NET   ))	\
	    |(IPL_SOFTBIO    << (4*SOFTINT_BIO   ))	\
	    |(IPL_SOFTCLOCK  << (4*SOFTINT_CLOCK )))
#define	SOFTINT2IPL(si_level)	((SOFTINT2IPL_MAP >> (4 * (si_level))) & 0x0f)
#define IPL2SOFTINT_MAP \
	    ((SOFTINT_SERIAL << (4*IPL_SOFTSERIAL))	\
	    |(SOFTINT_NET    << (4*IPL_SOFTNET   ))	\
	    |(SOFTINT_BIO    << (4*IPL_SOFTBIO   ))	\
	    |(SOFTINT_CLOCK  << (4*IPL_SOFTCLOCK )))
#define	IPL2SOFTINT(ipl)	((IPL2SOFTINT_MAP >> (4 * (ipl))) & 0x0f)

#endif /* __INTR_PRIVATE */

#ifdef _KERNEL
struct cpu_info;
void	powerpc_softint(struct cpu_info *, int, vaddr_t);
void	powerpc_softint_init_md(lwp_t *, u_int, uintptr_t *);
void	powerpc_softint_trigger(uintptr_t);
#endif

#endif /* __HAVE_FAST_SOFTINTS */
#endif /* !_POWERPC_SOFTINT_H_ */
