/* $NetBSD: wdtreg.h,v 1.3 2000/01/17 21:41:14 msaitoh Exp $ */

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

#ifndef _SH3_WDTREG_H_
#define _SH3_WDTREG_H_

/* WDT registers */

#if !defined(SH4)

/* SH3 definitions */

#define	SHREG_WTCNT_R	(*(volatile unsigned char  *)0xffffff84)
#define	SHREG_WTCNT_W	(*(volatile unsigned short *)0xffffff84)
#define	SHREG_WTCSR_R	(*(volatile unsigned char  *)0xffffff86)
#define	SHREG_WTCSR_W	(*(volatile unsigned short *)0xffffff86)

#else

/* SH4 definitions */

#define	SHREG_WTCNT_R	(*(volatile unsigned char  *)0xffc00008)
#define	SHREG_WTCNT_W	(*(volatile unsigned short *)0xffc00008)
#define	SHREG_WTCSR_R	(*(volatile unsigned char  *)0xffc0000c)
#define	SHREG_WTCSR_W	(*(volatile unsigned short *)0xffc0000c)

#endif

#define WTCNT_W_M	0x5A00
#define WTCSR_W_M	0xA500

#define WTCSR_TME	0x80
#define WTCSR_WT	0x40
#define WTCSR_RSTS	0x20
#define WTCSR_WOVF	0x10
#define WTCSR_IOVF	0x08
#define WTCSR_CKS2	0x04
#define WTCSR_CKS1	0x02
#define WTCSR_CKS0	0x01

#define WTCSR_CKS	0x07
#define WTCSR_CKS_1	0x00
#define WTCSR_CKS_4	0x01
#define WTCSR_CKS_16	0x02
#define WTCSR_CKS_32	0x03
#define WTCSR_CKS_64	0x04
#define WTCSR_CKS_256	0x05
#define WTCSR_CKS_1024	0x06
#define WTCSR_CKS_4096	0x07

#endif	/* !_SH3_WDTREG_H_ */
