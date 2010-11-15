/*	$NetBSD: imxkppreg.h,v 1.1.2.2 2010/11/15 14:38:22 uebayasi Exp $	*/

/*
 * Copyright (c) 2010  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Register definitions for Keypad Port (KPP)
 */

#ifndef _ARM_IMX_IMXKPPREG_H
#define	_ARM_IMX_IMXKPPREG_H

/* Registers are byte- or halfword-addressable */

#define	KPP_KPCR	0x0000	/* control register */
#define	 KPCR_KCO	0xff00	/* Column strobe */
#define	 KPCR_KRE	0x00ff	/* Row Enable */
#define	KPP_KPSR	0x0002	/* status register */
#define	 KPSR_KRIE	__BIT(9)
#define	 KPSR_KDIE	__BIT(8)
#define	 KPSR_KRSS	__BIT(3)
#define	 KPSR_KDSC	__BIT(2)
#define	 KPSR_KPKR	__BIT(1)
#define	 KPSR_KPKD	__BIT(0)

#define	KPP_KDDR	0x0004	/* data direction register */

#define	KPP_KPDR	0x0006	/* data register */

#endif	/* _ARM_IMX_IMXKPPREG_H */
