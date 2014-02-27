/*	$NetBSD: imx51_tzicreg.h,v 1.2 2014/02/27 03:09:57 hkenken Exp $	*/
/*
 * Copyright (c) 2010  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
#ifndef _ARM_IMX_IMX51_TZICREG_H_
#define	_ARM_IMX_IMX51_TZICREG_H_

#include <sys/cdefs.h>

#define	TZIC_SIZE		0x4000
#define	TZIC_INTCNTL		0x0000
#define	 INTCNTL_NSEN_MASK	__BIT(31)
#define	 INTCNTL_NSEN		__BIT(16)
#define	 INTCNTL_EN		__BIT(0)
#define	TZIC_INTTYPE		0x0004
#define	TZIC_PRIOMASK		0x000c
#define	TZIC_SYNCCTRL		0x0010
#define	TZIC_DSMINT		0x0014
#define	TZIC_INTSEC(n)		(0x0080 + 0x04 * (n))
#define	TZIC_ENSET(n)		(0x0100 + 0x04 * (n))
#define	TZIC_ENCLEAR(n)		(0x0180 + 0x04 * (n))
#define	TZIC_SRCSET(n)		(0x0200 + 0x04 * (n))
#define	TZIC_SRCCLAR(n)		(0x0280 + 0x04 * (n))
#define	TZIC_PRIORITY(n)	(0x0400 + 0x04 * (n))
#define	TZIC_PND(n)		(0x0d00 + 0x04 * (n))
#define	TZIC_HIPND(n)		(0x0d80 + 0x04 * (n))
#define	TZIC_WAKEUP(n)		(0x0e00 + 0x04 * (n))
#define	TZIC_SWINT		0x0f00

#define	TZIC_INTNUM		128
#endif /* _ARM_IMX_IMX51_TZICREG_H_ */
