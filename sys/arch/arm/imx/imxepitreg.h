/*	$NetBSD: imxepitreg.h,v 1.1.2.2 2010/11/15 14:38:22 uebayasi Exp $ */
/*
 * Copyright (c) 2009, 2010  Genetec corp.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec corp.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _ARM_IMX_IMXEPITREG_H
#define	_ARM_IMX_IMXEPITREG_H

#define	EPIT_EPITCR	0x0000
#define	 EPITCR_EN	__BIT(0)
#define	 EPITCR_ENMOD	__BIT(1)
#define	 EPITCR_OCIEN	__BIT(2)
#define	 EPITCR_RLD	__BIT(3)
#define	 EPITCR_PRESCALER	__BITS(4,15)
#define	 EPITCR_SWR	__BIT(16)
#define	 EPITCR_IOVW	__BIT(17)
#define	 EPITCR_DBGEN	__BIT(18)
#define	 EPITCR_WAITEN	__BIT(19)
#define	 EPITCR_DOZEN	__BIT(20)
#define	 EPITCR_STOPEN	__BIT(21)
#define	 EPITCR_OM	__BITS(22,23)
#define	 EPITCR_CLKSRC_MASK	__BITS(24,25)
#define	 EPITCR_CLKSRC_HIGH	(0x2 << 24)

#define	EPIT_EPITSR	0x0004
#define	EPIT_EPITLR	0x0008
#define	EPIT_EPITCMPR	0x000c
#define	EPIT_EPITCNT	0x0010

#endif	/* _ARM_IMX_IMXEPITREG_H */
