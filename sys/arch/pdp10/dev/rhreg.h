/*	$NetBSD: rhreg.h,v 1.1 2003/08/19 10:51:57 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * RH20 register definitions.
 */

/*
 * Different device codes.
 */
#define	RH_DT_RP04	0x10
#define RH_DT_RP05	0x11
#define RH_DT_RP06	0x12
#define RH_DT_RP07	0x22
#define RH_DT_RM02	0x15
#define RH_DT_RM03	0x14
#define RH_DT_RM05	0x17
#define RH_DT_RM80	0x16
#define	RH_DT_TU45	052
#define	RH_DT_DRQ	0x800		/* Dual ported */
#define	RH_DT_MOH	0x2000		/* Moving head device */

/*
 * Some common registers used in the rh device driver.
 */
#define RH_DS		1		/* unit status */
#define RH_AS		4		/* attention summary */
#define RH_DT		6		/* drive type */
#define	RH_SBAR		070		/* Bus address register */
#define	RH_STCR		071		/* Transfer control register */
#define	RH_IVIR		074		/* Interrupt vector register */

#define RH_DS_DPR	0x100		/* Unit present */
/*
 * Internal register bits.
 */
#define	RH_BAR_ADRMSK	0177777		/* Disk/tape address mask */
#define	RH_TCR_RCLP	0002000000000	/* Reset channel when starting */
#define	RH_TCR_SES	0000200000000	/* Store status after transfer */
#define	RH_TCR_DTES	0000000200000	/* Don't stop if errors */
#define	RH_TCR_NBC	0000000177700	/* Negative block count */
#define RH_TCR_NBCSH	6		/* Block count shift */

/*
 * RH20 register definitions.
 */
#define	RH20_CONI_BPE	0400000		/* Data bus parity error */
#define	RH20_CONI_EXC	0200000		/* Exception */
#define	RH20_CONI_LWE	0100000		/* Long word count error */
#define	RH20_CONI_SWE	0040000		/* Short word count error */
#define	RH20_CONI_MBE	0020000		/* MBOX error */
#define	RH20_CONI_DRE	0010000		/* Drive response error */
#define	RH20_CONI_RAE	0004000		/* Register access error */
#define	RH20_CONI_MBH	0002000		/* MBOX halted */
#define	RH20_CONI_OVR	0001000		/* Data overrun */
#define	RH20_CONI_MEN	0000400		/* Massbus enable */
#define	RH20_CONI_ATN	0000200		/* Drive attention */
#define	RH20_CONI_2RF	0000100		/* Secondary command register full */
#define	RH20_CONI_ATE	0000040		/* Attention enable */
#define	RH20_CONI_1RF	0000020		/* Primary command register full */
#define	RH20_CONI_DON	0000010		/* Channel done */

#define	RH20_CONO_RAE	0004000		/* Clear RAE */
#define	RH20_CONO_RST	0002000		/* Controller reset */
#define	RH20_CONO_XFR	0001000		/* Clear transfer error */
#define	RH20_CONO_MEN	0000400		/* Massbus enable */
#define	RH20_CONO_RCL	0000200		/* Reset MBOX command list pointer */
#define	RH20_CONO_DEL	0000100		/* Delete secondary command */
#define	RH20_CONO_ATE	0000040		/* Enable attention interrupts */
#define	RH20_CONO_STP	0000020		/* Stop current command */
#define	RH20_CONO_DON	0000010		/* Clear done */
#define	RH20_CONO_IMSK	0000007		/* Mask for interrupt level */

#define	RH20_DATAI_CBPE	0001000000000	/* Control bus parity error */
#define	RH20_DATAI_TRA	0000200000000	/* Transfer received */
#define	RH20_DATAI_CPA	0000000200000	/* Massbus parity bit */
#define	RH20_DATAO_RS	0770000000000	/* Register select */
#define	RH20_DATAO_LR	0004000000000	/* Load register */
#define	RH20_DATAO_RAES	0000400000000	/* RAE Suppress */

/*
 * Data channel defines.
 */
#define	DCH_CCL_OFF	0	/* Channel Command List opcode */
#define	DCH_CST_OFF	1	/* Channel Status Word */
#define	DCH_CCW_OFF	2	/* Current Channel Command Word */
#define	DCH_CIV_OFF	3	/* Unused, free for software to use */

#define DCH_CCW_HLT	0000000000000	/* HALT opcode */
#define DCH_CCW_JMP	0200000000000	/* JUMP opcode */
#define DCH_CCW_XFR	0400000000000	/* XFER opcode, plus next 2 bits: */
#define DCH_CCW_XHLT	0200000000000	/* Xfer cmd: "Halt after xfer" */
#define DCH_CCW_XREV	0100000000000	/* Xfer cmd: "Reverse xfer" */
#define DCH_CCW_CNTMSK	0077760000000	/* Word Count (positive) */
#define DCH_CCW_CNTSH	22		/* Word Count shift */
#define DCH_CCW_ADRMSK	0000017777777	/* 22-bit address */

/* Channel Status Word
*/
#define DCH_CSW_SET	0400000000000	/* Always set 1 */
#define DCH_CSW_MPAR	0200000000000	/* Mem Par Err during CCW fetch */
#define DCH_CSW_NOAPE	0100000000000	/* Set if NO mem addr par err */
#define DCH_CSW_NOWC0	0040000000000	/* CCW count NOT 0 when CSW set */
#define DCH_CSW_NXM	0020000000000	/* Chan referenced NXM */
#define DCH_CSW_LXFE	0000400000000	/* Last Transfer Error */
#define DCH_CSW_RH20E	0000200000000	/* RH20 tried to start not-ready chn */
#define DCH_CSW_LWCE	0000100000000	/* Long Word Count Error */
#define DCH_CSW_SWCE	0000040000000	/* Short Word Count Error */
#define DCH_CSW_OVN	0000020000000	/* Overrun */
#define	DCH_CSW_ADRMSK	0000017777777	/* 22-bit CCW addr+1 */
