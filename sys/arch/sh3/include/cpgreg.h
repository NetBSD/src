/* $Id: cpgreg.h,v 1.1 1999/09/13 10:31:16 itojun Exp $ */
/* $NetBSD: cpgreg.h,v 1.1 1999/09/13 10:31:16 itojun Exp $ */

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

#ifndef _SH3_CPGREG_H__
#define _SH3_CPGREG_H__

/*
 * Clock Pulse Generator
 */

#if !defined(SH4)

/* SH3 definition */
#define SHREG_FRQCR	(*(volatile unsigned short *)	0xFFFFFF80)

#else

/* SH4 definition */
#define SHREG_FRQCR	(*(volatile unsigned short *)	0xffc00000)

#endif

/*
 * Standby Control
 */

#if !defined(SH4)

/* SH3 definitions */
#define SHREG_STBCR	(*(volatile unsigned char *)	0xFFFFFF82)
#if defined(SH7709) || defined(SH7709A)
#define SHREG_STBCR2	(*(volatile unsigned char *)	0xffffff88)
#endif

#else

/* SH4 definitions */
#define SHREG_STBCR	(*(volatile unsigned char *)	0xffc00004)
#define SHREG_STBCR2	(*(volatile unsigned char *)	0xffc00010)

#endif

#endif /* !_SH3_CPGREG_H__ */
