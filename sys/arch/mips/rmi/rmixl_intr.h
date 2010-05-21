/*	$NetBSD: rmixl_intr.h,v 1.1.2.4 2010/05/21 23:35:42 cliff Exp $	*/
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

/*
 * A 'vector' is bit number in EIRR/EIMR
 * - non-IRT-based interrupts use vectors 0..31
 * - IRT-based interrupts use vectors 32..63 
 * - RMIXL_VECTOR_IRT(vec) is used to index into the IRT
 * - IRT entry n always routes to vector RMIXL_IRT_VECTOR(n)
 * - only 1 intrhand_t per vector
 */
#define	NINTRVECS	64	/* bit width of the EIRR */
#define	NIRTS		32	/* #entries in the Interrupt Redirection Table */

/*
 * mapping between IRT index and vector number
 */
#define RMIXL_VECTOR_IS_IRT(vec)	((vec) >= 32)
#define RMIXL_IRT_VECTOR(irt)		((irt) + 32)
#define RMIXL_VECTOR_IRT(vec)		((vec) - 32)

/*
 * vectors (0 <= vec < 8)  are CAUSE[8..15] (including softintrs and count/compare)
 * vectors (8 <= vec < 31) are for other non-IRT based interrupts
 */
#define RMIXL_INTRVEC_IPI	8
#define RMIXL_INTRVEC_FMN	9

typedef enum {
	RMIXL_TRIG_NONE=0,
	RMIXL_TRIG_EDGE,
	RMIXL_TRIG_LEVEL,
} rmixl_intr_trigger_t;

typedef enum {
	RMIXL_POLR_NONE=0,
	RMIXL_POLR_RISING,
	RMIXL_POLR_HIGH,
	RMIXL_POLR_FALLING,
	RMIXL_POLR_LOW,
} rmixl_intr_polarity_t;


/*
 * iv_list and ref count manage sharing of each vector
 */
typedef struct rmixl_intrhand {
        int (*ih_func)(void *);
        void *ih_arg; 
        int ih_mpsafe; 			/* true if does not need kernel lock */
        int ih_vec;			/* vector is bit number in EIRR/EIMR */
        int ih_ipl; 			/* interrupt priority */
        int ih_cpumask; 		/* CPUs which may handle this irpt */
} rmixl_intrhand_t;

/*
 * stuff exported from rmixl_spl.S
 */
extern const struct splsw rmixl_splsw;
extern uint64_t ipl_eimr_map[];

extern void *rmixl_intr_establish(int, int, int,
	rmixl_intr_trigger_t, rmixl_intr_polarity_t,
	int (*)(void *), void *, bool);
extern void  rmixl_intr_disestablish(void *);
extern void *rmixl_vec_establish(int, int, int,
	int (*)(void *), void *, bool);
extern void  rmixl_vec_disestablish(void *);
extern const char *rmixl_intr_string(int);
extern void rmixl_intr_init_cpu(struct cpu_info *);
extern void *rmixl_intr_init_clk(void);
#ifdef MULTIPROCESSOR
extern void *rmixl_intr_init_ipi(void);
#endif

#endif	/* _MIPS_RMI_RMIXL_INTR_H_ */
