/* $NetBSD: bscreg.h,v 1.2 1999/09/16 21:15:36 msaitoh Exp $ */

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

#ifndef _SH3_BSCREG_H_
#define _SH3_BSCREG_H_

/*
 * Bus State Controller
 */

#if !defined(SH4)

/* SH3 definitions */

#define SHREG_BCR1	(*(volatile unsigned short *)	0xffffff60)
#define SHREG_BCR2	(*(volatile unsigned short *)	0xffffff62)
#define SHREG_WCR1	(*(volatile unsigned short *)	0xffffff64)
#define SHREG_WCR2	(*(volatile unsigned short *)	0xffffff66)
#define SHREG_MCR	(*(volatile unsigned short *)	0xffffff68)
#define SHREG_DCR	(*(volatile unsigned short *)	0xffffff6a)
#define SHREG_PCR	(*(volatile unsigned short *)	0xffffff6c)
#define SHREG_RTCSR	(*(volatile unsigned short *)	0xffffff6e)
#define SHREG_RTCNT	(*(volatile unsigned short *)	0xffffff70)
#define SHREG_RTCOR	(*(volatile unsigned short *)	0xffffff72)
#define SHREG_RFCR	(*(volatile unsigned short *)	0xffffff74)
#define SHREG_BCR3	(*(volatile unsigned short *)	0xffffff7e)

#else

/* SH4 definitions */

#define SHREG_BCR1	(*(volatile unsigned int *)	0xff800000)
#define SHREG_BCR2	(*(volatile unsigned short *)	0xff800004)
#define SHREG_WCR1	(*(volatile unsigned int *)	0xff800008)
#define SHREG_WCR2	(*(volatile unsigned int *)	0xff80000c)
#define SHREG_WCR3	(*(volatile unsigned int *)	0xff800010)
#define SHREG_MCR	(*(volatile unsigned int *)	0xff800014)
#define SHREG_PCR	(*(volatile unsigned short *)	0xff800018)
#define SHREG_RTCSR	(*(volatile unsigned short *)	0xff80001c)
#define SHREG_RTCNT	(*(volatile unsigned short *)	0xff800020)
#define SHREG_RTCOR	(*(volatile unsigned short *)	0xff800024)
#define SHREG_RFCR	(*(volatile unsigned short *)	0xff800028)

#endif
#endif	/* !_SH3_BSCREG_H_ */
