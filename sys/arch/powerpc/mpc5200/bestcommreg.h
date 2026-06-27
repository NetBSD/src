/*	$NetBSD: bestcommreg.h,v 1.1 2026/06/27 13:28:34 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#ifndef _POWERPC_MPC5200_BESTCOMMREG_H_
#define _POWERPC_MPC5200_BESTCOMMREG_H_

/*
 * BestComm SDMA controller registers (MBAR+0x1200 block). 
 */

#define	SDMA_TASKBAR	0x00	/* task table base (physical SRAM addr)	*/
#define	SDMA_CUR_PTR	0x04	/* current descriptor pointer		*/
#define	SDMA_END_PTR	0x08	/* end descriptor pointer		*/
#define	SDMA_VAR_PTR	0x0c	/* variable table pointer		*/
#define	SDMA_INT_VECT1	0x10	/* interrupt vector 1 (8-bit)		*/
#define	SDMA_INT_VECT2	0x11	/* interrupt vector 2 (8-bit)		*/
#define	SDMA_PTD_CNTRL	0x12	/* processor task dispatch control (16-bit) */
#define	SDMA_INT_PEND	0x14	/* interrupt pending (write 1 to clear)	*/
#define	SDMA_INT_MASK	0x18	/* interrupt mask (1 = masked)		*/
#define	SDMA_TCR	0x1c	/* task control regs: 16 x 16-bit	*/
#define	SDMA_IPR	0x3c	/* initiator priority regs: 32 x 8-bit	*/
#define	SDMA_CREQ_SEL	0x5c	/* current request select		*/
#define	SDMA_TASK_SIZE0	0x60	/* task size 0				*/
#define	SDMA_TASK_SIZE1	0x64	/* task size 1				*/
#define	SDMA_MDE_DEBUG	0x68	/* MDE debug				*/
#define	SDMA_ADS_DEBUG	0x6c	/* ADS debug				*/
#define	SDMA_VALUE1	0x70	/* debug value 1			*/
#define	SDMA_VALUE2	0x74	/* debug value 2			*/
#define	SDMA_DBG_CONTROL 0x78	/* debug control			*/
#define	SDMA_DBG_STATUS	0x7c	/* debug status				*/
#define	SDMA_PTD_DEBUG	0x80	/* PTD debug				*/

#define	SDMA_REG_SIZE	0x100	/* register window if OF omits a size	*/

/*
 * Per-task control register (TCR).
 */
#define	SDMA_TCR_TASK(n)	(SDMA_TCR + (n) * 2)
#define	SDMA_TCR_ENABLE		0x8000	/* enable/start this task (bit 15)	*/
#define	SDMA_TCR_INIT_SHIFT	8	/* initiator number field (bits 8-12)	*/
#define	SDMA_TCR_INIT_MASK	0x1f00
#define	SDMA_TCR_HOLD		0x0020	/* hold initiator (bit 5)		*/
#define	SDMA_TCR_AUTOSTART	0x0080	/* restart after completion (bit 7)	*/
#define	SDMA_TCR_AUTOTASK_MASK	0x000f	/* task to auto-start (bits 0-3)		*/

#define	SDMA_NTASKS		16	/* number of SDMA tasks		*/

/*
 * Per-task transfer-size register
 */
#define	SDMA_SIZE_BYTE(task)	(SDMA_TASK_SIZE0 + (task) / 2)
#define	SDMA_SIZE_CODE(sz)	((sz) & 0x3)
#define	SDMA_SIZE_FIELD(src, dst) \
	((SDMA_SIZE_CODE(src) << 2) | SDMA_SIZE_CODE(dst))

/*
 * Data Routing Descriptor (DRD)
 */
#define	SDMA_DRD_INIT_SHIFT	21
#define	SDMA_DRD_INIT_MASK	(0x1fu << SDMA_DRD_INIT_SHIFT)	/* 0x03e00000 */
#define	SDMA_DRD_EXT		0x40000000
#define	SDMA_INITIATOR_ALWAYS	0

/* Per-initiator priority registers: byte SDMA_IPR + initiator */
#define	SDMA_IPR_INIT(init)	(SDMA_IPR + (init))

/* PtdCntrl (SDMA_PTD_CNTRL) bits. */
#define	SDMA_PTD_CNTRL_TI	0x8000	/* task initiator disable	*/
#define	SDMA_PTD_CNTRL_TEA	0x4000	/* bus-error (TEA) disable	*/
#define	SDMA_PTD_CNTRL_HE	0x2000	/* halt on error		*/
#define	SDMA_PTD_CNTRL_PE	0x0001	/* parity enable		*/

/*
 * Interrupt pending / mask register layout.
 */
#define	SDMA_INT_TASK_MASK	0x0000ffff	/* the 16 task events	*/
#define	SDMA_INT_TASK(n)	(1u << (n))
#define	SDMA_INT_TEA		__BIT(28)	/* XLB bus error	*/
#define	SDMA_INT_TEA_TASK_MASK	0x0f000000	/* faulting task (bits 24-27) */
#define	SDMA_INT_TEA_TASK_SHIFT	24
#define	SDMA_INT_DBG		__BIT(31)	/* debug event		*/
#define	SDMA_INT_IMPL		0x9000ffff	/* implemented bits	*/

#endif /* _POWERPC_MPC5200_BESTCOMMREG_H_ */
