/*	$NetBSD: rmixl_intr.h,v 1.1.2.11 2011/12/31 08:20:43 matt Exp $	*/
/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors.
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

#ifndef _MIPS_RMI_RMIXL_INTR_H_
#define _MIPS_RMI_RMIXL_INTR_H_

#ifdef _KERNEL_OPT
#include "opt_multiprocessor.h"
#endif

/*
 * A 'vector' is bit number in EIRR/EIMR
 * - IRT or non-IRT interrupts use vectors 8..63
 * - vector or dynamically assigned (except for IPI and FMN)
 */
#define	NINTRVECS	64	/* bit width of the EIRR */
#define	RMIXLP_NIRTS	160	/* #entries in Interrupt Redirection Table */
#define	RMIXLR_NIRTS	32	/* #entries in Interrupt Redirection Table */
#define	RMIXLS_NIRTS	32	/* #entries in Interrupt Redirection Table */

/*
 * vectors (0 <= vec < 8)  are CAUSE[8..15] (including softintrs and count/compare)
 */
#define RMIXL_INTRVEC_IPI	8
#define RMIXL_INTRVEC_FMN	(RMIXL_INTRVEC_IPI + NIPIS)

/*
 * iv_list and ref count manage sharing of each vector
 */
typedef struct rmixl_intrhand {
	LIST_ENTRY(rmixl_intrhand) ih_link;
        int (*ih_func)(void *);
        void *ih_arg; 
        bool ih_mpsafe; 		/* true if does not need kernel lock */
        uint8_t ih_vec;			/* vector is bit number in EIRR/EIMR */
} rmixl_intrhand_t;

typedef struct rmixl_intrvec {
	LIST_HEAD(, rmixl_intrhand) iv_hands;
	TAILQ_ENTRY(rmixl_intrvec) iv_lruq_link;
	rmixl_intrhand_t iv_intrhand;
        uint8_t iv_ipl; 		/* interrupt priority */
} rmixl_intrvec_t;

typedef TAILQ_HEAD(rmixl_intrvecq, rmixl_intrvec) rmixl_intrvecq_t;

/*
 * stuff exported from rmixl_spl.S
 */
extern const struct splsw rmixl_splsw;
extern uint64_t ipl_eimr_map[];

void *	rmixl_intr_establish(size_t /* irt */, int /* ipl */, int /* ist */,
	    int (*)(void *), void *, bool);
void	rmixl_intr_disestablish(void *);
void *	rmixl_vec_establish(size_t /* vec */, rmixl_intrhand_t *, int /* ipl */,
	    int (*)(void *), void *, bool);
void	rmixl_vec_disestablish(void *);
const char *
	rmixl_intr_string(size_t);
const char *
	rmixl_irt_string(size_t);
void	rmixl_intr_init_cpu(struct cpu_info *);
void	rmixl_intr_init_clk(void);
#ifdef MULTIPROCESSOR
void	rmixl_intr_init_ipi(void);
#endif

void *	gpio_intr_establish(size_t /* pin */, int /* ipl */, int /* ist */,
	    int (*)(void *), void *, bool);

void	gpio_intr_disestablish(void *);
#endif	/* _MIPS_RMI_RMIXL_INTR_H_ */
