/* $Id: rtcreg.h,v 1.2.2.2 2001/04/23 09:42:02 bouyer Exp $ */
/* $NetBSD: rtcreg.h,v 1.2.2.2 2001/04/23 09:42:02 bouyer Exp $ */

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

#ifndef _SH3_RTCREG_H__
#define _SH3_RTCREG_H__

/*
 * Real Time Clock
 */

#if !defined(SH4)

/* SH3 definitions */

#define SHREG_R64CNT	(*(volatile unsigned char *)	0xFFFFFEC0)
#define SHREG_RSECCNT	(*(volatile unsigned char *)	0xFFFFFEC2)
#define SHREG_RMINCNT	(*(volatile unsigned char *)	0xFFFFFEC4)
#define SHREG_RHRCNT	(*(volatile unsigned char *)	0xFFFFFEC6)
#define SHREG_RWKCNT	(*(volatile unsigned char *)	0xFFFFFEC8)
#define SHREG_RDAYCNT	(*(volatile unsigned char *)	0xFFFFFECA)
#define SHREG_RMONCNT	(*(volatile unsigned char *)	0xFFFFFECC)
#define SHREG_RYRCNT	(*(volatile unsigned char *)	0xFFFFFECE)
#define SHREG_RSECAR	(*(volatile unsigned char *)	0xFFFFFED0)
#define SHREG_RMINAR	(*(volatile unsigned char *)	0xFFFFFED2)
#define SHREG_RHRAR	(*(volatile unsigned char *)	0xFFFFFED4)
#define SHREG_RWKAR	(*(volatile unsigned char *)	0xFFFFFED6)
#define SHREG_RDAYAR	(*(volatile unsigned char *)	0xFFFFFED8)
#define SHREG_RMONAR	(*(volatile unsigned char *)	0xFFFFFEDA)
#define SHREG_RCR1	(*(volatile unsigned char *)	0xFFFFFEDC)
#define SHREG_RCR2	(*(volatile unsigned char *)	0xFFFFFEDE)

#else

/* SH4 definitions */

#define SHREG_R64CNT	(*(volatile unsigned char *)	0xffc80000)
#define SHREG_RSECCNT	(*(volatile unsigned char *)	0xffc80004)
#define SHREG_RMINCNT	(*(volatile unsigned char *)	0xffc80008)
#define SHREG_RHRCNT	(*(volatile unsigned char *)	0xffc8000c)
#define SHREG_RWKCNT	(*(volatile unsigned char *)	0xffc80010)
#define SHREG_RDAYCNT	(*(volatile unsigned char *)	0xffc80014)
#define SHREG_RMONCNT	(*(volatile unsigned char *)	0xffc80018)
#define SHREG_RYRCNT	(*(volatile unsigned short *)	0xffc8001c)
#define SHREG_RSECAR	(*(volatile unsigned char *)	0xffc80020)
#define SHREG_RMINAR	(*(volatile unsigned char *)	0xffc80024)
#define SHREG_RHRAR	(*(volatile unsigned char *)	0xffc80028)
#define SHREG_RWKAR	(*(volatile unsigned char *)	0xffc8002c)
#define SHREG_RDAYAR	(*(volatile unsigned char *)	0xffc80030)
#define SHREG_RMONAR	(*(volatile unsigned char *)	0xffc80034)
#define SHREG_RCR1	(*(volatile unsigned char *)	0xffc80038)
#define SHREG_RCR2	(*(volatile unsigned char *)	0xffc8003c)

#endif

#define SHREG_RCR1_CF		0x80
#define SHREG_RCR1_CIE		0x10
#define SHREG_RCR1_AIE		0x08
#define SHREG_RCR1_AF		0x01

#define SHREG_RCR2_PEF		0x80
#define SHREG_RCR2_PES2		0x40
#define SHREG_RCR2_PES1		0x20
#define SHREG_RCR2_PES0		0x10
#define SHREG_RCR2_ENABLE	0x08
#define SHREG_RCR2_ADJ		0x04
#define SHREG_RCR2_RESET	0x02
#define SHREG_RCR2_START	0x01

#define SHREG_RCR2_P64		(SHREG_RCR2_PES1)

#endif /* !_SH3_RTCREG_H__ */
