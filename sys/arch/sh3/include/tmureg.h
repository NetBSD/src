/* $NetBSD: tmureg.h,v 1.4 2000/03/20 20:36:58 msaitoh Exp $ */

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

#ifndef _SH3_TMUREG_H_
#define _SH3_TMUREG_H_

/*
 * Timer Unit
 */

#if !defined(SH4)

/* SH3 definition */

/* common */
#define SHREG_TOCR	(*(volatile unsigned char *)	0xfffffe90)
#define SHREG_TSTR	(*(volatile unsigned char *)	0xfffffe92)

/* ch. 0 */
#define SHREG_TCOR0	(*(volatile unsigned int *)	0xfffffe94)
#define SHREG_TCNT0	(*(volatile unsigned int *)	0xfffffe98)
#define SHREG_TCR0	(*(volatile unsigned short *)	0xfffffe9c)

/* ch. 1 */
#define SHREG_TCOR1	(*(volatile unsigned int *)	0xfffffea0)
#define SHREG_TCNT1	(*(volatile unsigned int *)	0xfffffea4)
#define SHREG_TCR1	(*(volatile unsigned short *)	0xfffffea8)

/* ch. 2 */
#define SHREG_TCOR2	(*(volatile unsigned int *)	0xfffffeac)
#define SHREG_TCNT2	(*(volatile unsigned int *)	0xfffffeb0)
#define SHREG_TCR2	(*(volatile unsigned short *)	0xfffffeb4)
#define SHREG_TCPR2	(*(volatile unsigned int *)	0xfffffeb8)

#else

/* SH4 address definition */

/* common */
#define SHREG_TOCR	(*(volatile unsigned char *)	0xffd80000)
#define SHREG_TSTR	(*(volatile unsigned char *)	0xffd80004)

/* ch. 0 */
#define SHREG_TCOR0	(*(volatile unsigned int *)	0xffd80008)
#define SHREG_TCNT0	(*(volatile unsigned int *)	0xffd8000c)
#define SHREG_TCR0	(*(volatile unsigned short *)	0xffd80010)

/* ch. 1 */
#define SHREG_TCOR1	(*(volatile unsigned int *)	0xffd80014)
#define SHREG_TCNT1	(*(volatile unsigned int *)	0xffd80018)
#define SHREG_TCR1	(*(volatile unsigned short *)	0xffd8001c)

/* ch. 2 */
#define SHREG_TCOR2	(*(volatile unsigned int *)	0xffd80020)
#define SHREG_TCNT2	(*(volatile unsigned int *)	0xffd80024)
#define SHREG_TCR2	(*(volatile unsigned short *)	0xffd80028)
#define SHREG_TCPR2	(*(volatile unsigned int *)	0xffd8002c)

#endif

#define TOCR_TCOE	0x01

#define TSTR_STR2	0x04
#define TSTR_STR1	0x02
#define TSTR_STR0	0x01

#define TCR_ICPF	0x0200
#define TCR_UNF		0x0100
#define TCR_ICPE1	0x0080
#define TCR_ICPE0	0x0040
#define TCR_UNIE	0x0020
#define TCR_CKEG1	0x0010
#define TCR_CKEG0	0x0008
#define TCR_TPSC2	0x0004
#define TCR_TPSC1	0x0002
#define TCR_TPSC0	0x0001

#define TCR_TPSC_P4	0x0000
#define TCR_TPSC_P16	0x0001
#define TCR_TPSC_P64	0x0002
#define TCR_TPSC_P256	0x0003
#if !defined(SH4)
#define TCR_TPSC_RTC	0x0004
#define TCR_TPSC_TCLK	0x0005
#else
#define TCR_TPSC_P512	0x0004
#define TCR_TPSC_RTC	0x0006
#define TCR_TPSC_TCLK	0x0007
#endif

#endif	/* !_SH3_TMUREG_H_ */
