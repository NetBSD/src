/*	$NetBSD: sbusreg.h,v 1.5 2003/08/07 16:29:36 agc Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)sbusreg.h	8.1 (Berkeley) 6/11/93
 */

/*
 * Sun-4c S-bus definitions.  (Should be made generic!)
 *
 * Sbus slot 0 is not a separate slot; it talks to the onboard I/O devices.
 * It is, however, addressed just like any `real' Sbus.
 *
 * Sbus device addresses are obtained from the FORTH PROMs.  They come
 * in `absolute' and `relative' address flavors, so we have to handle both.
 * Relative addresses do *not* include the slot number.
 */
#define	SBUS_BASE		0xf8000000
#define	SBUS_ADDR(slot, off)	(SBUS_BASE + ((slot) << 25) + (off))
#define	SBUS_ABS(a)		((unsigned)(a) >= SBUS_BASE)
#define	SBUS_ABS_TO_SLOT(a)	(((a) - SBUS_BASE) >> 25)
#define	SBUS_ABS_TO_OFFSET(a)	(((a) - SBUS_BASE) & 0x1ffffff)

#if _sbus_for_your_eyes_only_
struct sbusreg {
	u_int32_t	sbus_afsr;	/* M-to-S Asynchronous Fault Status */
	u_int32_t	sbus_afar;	/* M-to-S Asynchronous Fault Address */
	u_int32_t	sbus_arbiter;	/* Arbiter Enable  */
	u_int32_t	sbus_reserved1;

#define NSBUSCFG	20
	/* Actual number dependent on machine model */
	u_int32_t	sbus_sbuscfg[NSBUSCFG];	/* Sbus configuration control */
};
#endif

/* Register offsets */
#define SBUS_AFSR_REG	0
#define SBUS_AFAR_REG	4
#define SBUS_ARB_REG	8
#define SBUS_CFG_REG(n)	(16 + 4*(n))
#define SBUS_MFSR_REG	32		/* MS1 only: memory fault status */
#define SBUS_MFAR_REG	34		/* MS1 only: memory fault address */

/* M-to-S Asynchronous Fault Status register */
#define SBUS_AFSR_PAH	0x0000000f	/* PA<35:32> of fault address */
#define SBUS_AFSR_WM	0x00000100	/* SBus wide mode access */
#define SBUS_AFSR_SSIZ	0x00000e00	/* Size of error transaction */
#define SBUS_AFSR_SA	0x0001f000	/* bits <4:0> of fault address */
#define SBUS_AFSR_FAV	0x00020000	/* Fault address valid (MS only) */
#define SBUS_AFSR_RD	0x00040000	/* Read transaction */
#define SBUS_AFSR_ME	0x00080000	/* Multiple error */
#define SBUS_AFSR_MID	0x00f00000	/* Module ID */
#define SBUS_AFSR_S	0x01000000	/* Supervisor mode */
#define SBUS_AFSR_SIZ	0x0e000000	/* Requested transaction size */
#define SBUS_AFSR_BERR	0x10000000	/* Bus error (Sbus) or error ACK (VME)*/
#define SBUS_AFSR_TO	0x20000000	/* Bus Timeout */
#define SBUS_AFSR_LE	0x40000000	/* SBus late error */
#define SBUS_AFSR_ERR	0x80000000	/* Summary bit: one of LE,TO,BERR */
#define SBUS_AFSR_BITS	"\177\020"					\
			"f\0\4PAH\0b\10WM\0f\11\3SSIZ\0f\14\5SA\0"	\
			"b\11FAV\0b\12RD\0b\13ME\0f\14\4MID\0b\30S\0"	\
			"f\31\3SIZ\0b\34BERR\0b\35TO\0b\36LE\0b\37ERR\0"

/* Arbiter Enable register */
#define SBUS_ARB_P1	0x00000002	/* Enable MBus master 9 */
#define SBUS_ARB_P2	0x00000004	/* Enable MBus master 10 */
#define SBUS_ARB_P3	0x00000008	/* Enable MBus master 11 */
#define SBUS_ARB_B0	0x00010000	/* Enable SBus Slot 0 */
#define SBUS_ARB_B1	0x00020000	/* Enable SBus Slot 1 */
#define SBUS_ARB_B2	0x00040000	/* Enable SBus Slot 2 */
#define SBUS_ARB_B3	0x00080000	/* Enable SBus Slot 3 */
#define SBUS_ARB_BF	0x00100000	/* Enable on-board SBus devices */
#define SBUS_ARB_SBW	0x80000000	/* Enable S-to-M synchronous writes */
#define SBUS_ARB_BITS	"\177\020"			\
			"f\0\4CPUs Enabled\0"		\
			"f\20\5SBus Slots Enabled\0"	\
			"b\37S-to-M synchronous\0"

/* SBus Slot Configuration register */
#define SBUS_CFG_BY	0x00000001	/* Bypass Enabled */
#define SBUS_CFG_BA8	0x00000002	/* Slave supports 8-byte bursts */
#define SBUS_CFG_BA16	0x00000004	/* Slave supports 16-byte bursts */
#define SBUS_CFG_BA32	0x00000008	/* Slave supports 32-byte bursts */
#define SBUS_CFG_BA64	0x00000010	/* Slave supports 64-byte bursts */
#define SBUS_CFG_WMA	0x00004000	/* Enable wide-mode access */
#define SBUS_CFG_CP	0x00008000	/* Cacheable bit */
#define SBUS_CFG_SEGA	0x003f0000	/* PA<35:30> in by-pass mode */
#define SBUS_CFG_BITS	"\177\020"					\
			"b\0BY\0b\1BA8\0b\2BA16\0b\3BA32\0b\4BA64\0"	\
			"b\16WMA\0b\17CP\0f\20\6SEGA\0"
