/* $NetBSD: ubcreg.h,v 1.1 1999/09/13 10:31:24 itojun Exp $ */

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
#define _SH3_UBCREG_H_

/*
 * User Break Controller
 */

#if !defined(SH4)

/* SH3 definition */

/* ch-A */
#define SHREG_BARA	(*(volatile unsigned int *)	0xFFFFFFB0)
#define SHREG_BASRA	(*(volatile unsigned char *)	0xFFFFFFE4)
#define SHREG_BAMRA	(*(volatile unsigned char *)	0xFFFFFFB4)
#define SHREG_BBRA	(*(volatile unsigned short *)	0xFFFFFFB8)
/* ch-B */
#define SHREG_BARB	(*(volatile unsigned int *)	0xFFFFFFA0)
#define SHREG_BAMRB	(*(volatile unsigned char *)	0xFFFFFFa4)
#define SHREG_BASRB	(*(volatile unsigned char *)	0xFFFFFFe8)
#define SHREG_BBRB	(*(volatile unsigned short *)	0xFFFFFFa8)
#define SHREG_BDRB	(*(volatile unsigned int *)	0xFFFFFF90)
#define SHREG_BDMRB	(*(volatile unsigned int *)	0xFFFFFF94)
/* common */
#define SHREG_BRCR	(*(volatile unsigned short *)	0xFFFFFF98)

#else

/* SH4 definitions */

/* ch-A */
#define SHREG_BARA	(*(volatile unsigned int *)	0xff200000)
#define SHREG_BAMRA	(*(volatile unsigned char *)	0xff200004)
#define SHREG_BBRA	(*(volatile unsigned short *)	0xff200008)
#define SHREG_BASRA	(*(volatile unsigned char *)	0xff000014)
/* ch-B */
#define SHREG_BARB	(*(volatile unsigned int *)	0xff20000c)
#define SHREG_BAMRB	(*(volatile unsigned char *)	0xff200010)
#define SHREG_BBRB	(*(volatile unsigned short *)	0xff200014)
#define SHREG_BASRB	(*(volatile unsigned char *)	0xff000018)
#define SHREG_BDRB	(*(volatile unsigned int *)	0xff200018)
#define SHREG_BDMRB	(*(volatile unsigned int *)	0xff20001c)
/* common */
#define SHREG_BRCR	(*(volatile unsigned short *)	0xff200020)

#endif

#endif	/* !_SH3_UBCREG_H_ */
