/*	$NetBSD: exception.h,v 1.5 2004/03/25 01:02:30 uwe Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _SH3_EXCEPTION_H_
#define	_SH3_EXCEPTION_H_
/*
 * SH3/SH4 Exception handling.
 */
#include <sh3/devreg.h>

#ifdef _KERNEL
#define	SH3_TRA			0xffffffd0	/* 32bit */
#define	SH3_EXPEVT		0xffffffd4	/* 32bit */
#define	SH3_INTEVT		0xffffffd8	/* 32bit */
#define	SH7709_INTEVT2		0xa4000000	/* 32bit */

#define	SH4_TRA			0xff000020	/* 32bit */
#define	SH4_EXPEVT		0xff000024	/* 32bit */
#define	SH4_INTEVT		0xff000028	/* 32bit */

/*
 * EXPEVT
 */
/* Reset exception */
#define	EXPEVT_RESET_POWER	0x000	/* Power-On reset */
#define	EXPEVT_RESET_MANUAL	0x020	/* Manual reset */
#define	EXPEVT_RESET_TLB_MULTI_HIT	0x140	/* SH4 only */

/* General exception */
#define	EXPEVT_TLB_MISS_LD	0x040	/* TLB miss (load) */
#define	EXPEVT_TLB_MISS_ST	0x060	/* TLB miss (store) */
#define	EXPEVT_TLB_MOD		0x080	/* Initial page write */
#define	EXPEVT_TLB_PROT_LD	0x0a0	/* Protection violation (load) */
#define	EXPEVT_TLB_PROT_ST	0x0c0	/* Protection violation (store)*/
#define	EXPEVT_ADDR_ERR_LD	0x0e0	/* Address error (load) */
#define	EXPEVT_ADDR_ERR_ST	0x100	/* Address error (store) */
#define	EXPEVT_FPU		0x120	/* FPU exception */
#define	EXPEVT_TRAPA		0x160	/* Unconditional trap (TRAPA) */
#define	EXPEVT_RES_INST		0x180	/* Illegal instruction */
#define	EXPEVT_SLOT_INST	0x1a0	/* Illegal slot instruction */
#define	EXPEVT_BREAK		0x1e0	/* User break */
#define	EXPEVT_FPU_DISABLE	0x800	/* FPU disabled */
#define	EXPEVT_FPU_SLOT_DISABLE	0x820	/* Slot FPU disabled */

/* Software bit */
#define	EXP_USER		0x001	/* exception from user-mode */

#define	_SH_TRA_BREAK		0xc3	/* magic number for debugger */

/*
 * INTEVT/INTEVT2
 */
/* External interrupt */
#define	SH_INTEVT_NMI		0x1c0

#define	SH_INTEVT_TMU0_TUNI0	0x400
#define	SH_INTEVT_TMU1_TUNI1	0x420
#define	SH_INTEVT_TMU2_TUNI2	0x440
#define	SH_INTEVT_TMU2_TICPI2	0x460

#define	SH_INTEVT_SCI_ERI	0x4e0
#define	SH_INTEVT_SCI_RXI	0x500
#define	SH_INTEVT_SCI_TXI	0x520
#define	SH_INTEVT_SCI_TEI	0x540

#define	SH_INTEVT_WDT_ITI	0x560

#define	SH_INTEVT_IRL9		0x320
#define	SH_INTEVT_IRL11		0x360
#define	SH_INTEVT_IRL13		0x3a0

#define	SH4_INTEVT_SCIF_ERI	0x700
#define	SH4_INTEVT_SCIF_RXI	0x720
#define	SH4_INTEVT_SCIF_BRI	0x740
#define	SH4_INTEVT_SCIF_TXI	0x760

#define	SH7709_INTEVT2_IRQ0	0x600
#define	SH7709_INTEVT2_IRQ1	0x620
#define	SH7709_INTEVT2_IRQ2	0x640
#define	SH7709_INTEVT2_IRQ3	0x660
#define	SH7709_INTEVT2_IRQ4	0x680
#define	SH7709_INTEVT2_IRQ5	0x6a0

#define	SH7709_INTEVT2_PINT07	0x700
#define	SH7709_INTEVT2_PINT8F	0x720

#define SH7709_INTEVT2_DEI0	0x800
#define SH7709_INTEVT2_DEI1	0x820
#define SH7709_INTEVT2_DEI2	0x840
#define SH7709_INTEVT2_DEI3	0x860

#define	SH7709_INTEVT2_IRDA_ERI	0x880
#define	SH7709_INTEVT2_IRDA_RXI	0x8a0
#define	SH7709_INTEVT2_IRDA_BRI	0x8c0
#define	SH7709_INTEVT2_IRDA_TXI	0x8e0

#define	SH7709_INTEVT2_SCIF_ERI	0x900
#define	SH7709_INTEVT2_SCIF_RXI	0x920
#define	SH7709_INTEVT2_SCIF_BRI	0x940
#define	SH7709_INTEVT2_SCIF_TXI	0x960

#define	SH7709_INTEVT2_ADC	0x980

#ifndef _LOCORE
#if defined(SH3) && defined(SH4)
extern u_int32_t __sh_TRA;
extern u_int32_t __sh_EXPEVT;
extern u_int32_t __sh_INTEVT;
#endif /* SH3 && SH4 */
#endif /* !_LOCORE */
#endif /* _KERNEL */
#endif /* !_SH3_EXCEPTION_H_ */
