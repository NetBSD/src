/*	$NetBSD: cpgreg.h,v 1.2.16.2 2002/06/23 17:40:38 jdolecek Exp $	*/

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

#ifndef _SH3_CPGREG_H_
#define	_SH3_CPGREG_H_

/*
 * Clock Pulse Generator
 */
#define	SH3_FRQCR		0xffffff80	/* 16bit */
#define	SH4_FRQCR		0xffc00000	/* 16bit */

/*
 * Standby Control
 */
#define	SH3_STBCR		0xffffff82	/* 8bit */
#define	SH7709_STBCR2		0xffffff88	/* 8bit */

#define	SH4_STBCR		0xffc00004	/* 8bit */
#define	SH4_STBCR2		0xffc00010	/* 8bit */

#endif /* !_SH3_CPGREG_H_ */
