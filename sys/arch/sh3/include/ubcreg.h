/*	$NetBSD: ubcreg.h,v 1.4.74.1 2008/06/23 04:30:39 wrstuden Exp $	*/

/*-
 * Copyright (C) 1999 SAITOH Masanobu.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_UBCREG_H_
#define	_SH3_UBCREG_H_
#include <sh3/devreg.h>

/*
 * User Break Controller
 */

/* Channel A */
#define	SH3_BARA		0xffffffb0 /* 32: address */
#define	SH3_BAMRA		0xffffffb4 /* 32: address mask */
#define	SH3_BASRA		0xffffffe4 /* 16: ASID */
#define	SH3_BBRA		0xffffffb8 /* 16: bus cycle */
/* Channel B */
#define	SH3_BARB		0xffffffa0 /* 32: address */
#define	SH3_BAMRB		0xffffffa4 /* 32: address mask */
#define	SH3_BDRB		0xffffff90 /* 32: data */
#define	SH3_BDMRB		0xffffff94 /* 32: data mask */
#define	SH3_BASRB		0xffffffe8 /* 16: asid */
#define	SH3_BBRB		0xffffffa8 /* 16: bus cycle */
/* Common */
#define	SH3_BRCR		0xffffff98 /* 32: control */


/* Channel A */
#define	SH4_BARA		0xff200000 /* 32: address */
#define	SH4_BAMRA		0xff200004 /*  8: address/asid mask */
#define	SH4_BASRA		0xff000014 /*  8: ASID */
#define	SH4_BBRA		0xff200008 /* 16: bus cycle */

/* Channel B */
#define	SH4_BARB		0xff20000c /* 32: address */
#define	SH4_BAMRB		0xff200010 /*  8: address/asid mask */
#define	SH4_BASRB		0xff000018 /*  8: ASID */
#define	SH4_BDRB		0xff200018 /* 32: data */
#define	SH4_BDMRB		0xff20001c /* 32: data mask */
#define	SH4_BBRB		0xff200014 /* 16: bus cycle */
/* common */
#define	SH4_BRCR		0xff200020 /* 16: control */


/* SH4_BAMRx bits (sh3 uses plain 32-bit address mask) */
#define SH4_UBC_MASK_ASID	0x04 		/* ignore BASRx */
#define SH4_UBC_MASK_MASK	0x0b		/* mask BARx: */
#define SH4_UBC_MASK_NONE		0x00	/* - compare all bits */
#define SH4_UBC_MASK_10			0x01	/* - mask lower 10 bits */
#define SH4_UBC_MASK_12			0x02	/* - mask lower 12 bits */
#define SH4_UBC_MASK_ALL		0x03	/* - mask all bits */
#define SH4_UBC_MASK_16			0x08	/* - mask lower 16 bits */
#define SH4_UBC_MASK_20			0x09	/* - mask lower 20 bits */

/* BBRx bits */
#define SH3_UBC_CYCLE_SZ_MASK	0x03 /* exclusive */
#define SH4_UBC_CYCLE_SZ_MASK	0x43
#define UBC_CYCLE_8			0x01
#define UBC_CYCLE_16			0x02
#define UBC_CYCLE_32			0x03
#define SH4_UBC_CYCLE_64		0x40
#define UBC_CYCLE_RW_MASK	0x0c /* can be combined */
#define UBC_CYCLE_READ			0x04
#define UBC_CYCLE_WRITE			0x08
#define UBC_CYCLE_ID_MASK	0x30 /* can be combined */
#define UBC_CYCLE_INSN			0x10
#define UBC_CYCLE_DATA			0x20
#define SH3_UBC_CYCLE_CD_MASK	0xc0 /* exclusive */
#define SH3_UBC_CYCLE_CPU		0x40
#define SH3_UBC_CYCLE_DMAC		0x80

/* BRCR bits */
#define UBC_CTL_SEQ			0x0008 /* A||B vs A&&B */
#define UBC_CTL_B_AFTER_INSN		0x0040 /* B: before/after execution */
#define UBC_CTL_B_DATA			0x0080 /* B: match BDRB/BDMRB */
#define UBC_CTL_A_AFTER_INSN		0x0400 /* A: before/after execution */
#define UBC_CTL_B_MATCH			0x4000 /* B matched (sh3: cpu) */
#define UBC_CTL_A_MATCH			0x8000 /* A matched (sh3: cpu) */
#define SH3_UBC_CTL_B_MASK_ASID	    0x00100000 /* ignore BASRB */
#define SH3_UBC_CTL_A_MASK_ASID	    0x00200000 /* ignore BASRA */


#ifndef _LOCORE
#if defined(SH3) && defined(SH4)
extern uint32_t __sh_BARA;
extern uint32_t __sh_BAMRA;
extern uint32_t __sh_BASRA;
extern uint32_t __sh_BBRA;
extern uint32_t __sh_BARB;
extern uint32_t __sh_BAMRB;
extern uint32_t __sh_BASRB;
extern uint32_t __sh_BBRB;
extern uint32_t __sh_BDRB;
extern uint32_t __sh_BDMRB;
extern uint32_t __sh_BRCR;
#endif /* SH3 && SH4 */
#endif /* !_LOCORE */

#endif	/* !_SH3_UBCREG_H_ */
