/*	$NetBSD: fpu_status.h,v 1.2 1997/04/01 16:35:12 matthias Exp $	*/

/* 
 * IEEE floating point support for NS32081 and NS32381 fpus.
 * Copyright (c) 1995 Ian Dall
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * IAN DALL ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.
 * IAN DALL DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
/* 
 *	File:	fpu_status.h
 *	Author:	Ian Dall
 *	Date:	November 1995
 *
 *	FPU status register definitions
 *
 * HISTORY
 * 14-Dec-95  Ian Dall (Ian.Dall@dsto.defence.gov.au)
 *	First release.
 *
 */

#ifndef _FPU_STATUS_H_
#define _FPU_STATUS_H_
/*
 * Control register
 */
#define FPC_RMB		0x00010000	/* register modify bit */
#define FPC_SWF		0x0000fe00	/* reserved for software */
#define FPC_RM		0x00000180	/* rounding mode */
#define FPC_RM_NEAREST	0x00000000	/* round to nearest */
#define FPC_RM_TOZERO	0x00000080	/* round towards zero */
#define FPC_RM_TOPOS	0x00000100	/* round towards +infinity */
#define FPC_RM_TONEG	0x00000180	/* round towards -infinity */
#define FPC_IF		0x00000040	/* inexact result flag */
#define FPC_IEN		0x00000020	/* inexact result trap enable */
#define FPC_UF		0x00000010	/* underflow flag (else 0) */
#define FPC_UEN		0x00000008	/* underflow trap enable */
#define FPC_TT		0x00000007	/* trap type mask */
#define FPC_TT_NONE	0x00000000	/* no exceptional condition */
#define FPC_TT_UNDFL	0x00000001	/* underflow */
#define FPC_TT_OVFL	0x00000002	/* overflow */
#define FPC_TT_DIV0	0x00000003	/* divide by zero */
#define FPC_TT_ILL	0x00000004	/* illegal instruction */
#define FPC_TT_INVOP	0x00000005	/* invalid operation */
#define FPC_TT_INEXACT	0x00000006	/* inexact result */
#define FPC_TT_UNKNOWN  0x00000007      /* Not a real trap type */

/* Bits in the SWF field used for software emulation */
#define FPC_OVE  0x200		/* Overflow enable */
#define FPC_OVF  0x400		/* Overflow flag */
#define FPC_IVE  0x800		/* Invalid enable */
#define FPC_IVF  0x1000		/* Invalid flag */
#define FPC_DZE  0x2000		/* Divide by zero enable */
#define FPC_DZF  0x4000		/* Divide by zero flag */
#define FPC_UNDE 0x8000		/* Soft Underflow enable, requires FPC_UEN */

#define GET_SET_FSR(val) ({int _tmp; asm volatile("sfsr %0; lfsr %1" : "&=g" (_tmp): "g" (val)); _tmp;})
#define GET_FSR() ({int _tmp; asm volatile("sfsr %0" : "=g" (_tmp)); _tmp;})
#define SET_FSR(val) ({asm volatile("lfsr %0" :: "g" (val));})

#endif /* _FPU_STATUS_H_ */
