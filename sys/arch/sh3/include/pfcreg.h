/*	$NetBSD: pfcreg.h,v 1.3.4.1 2002/03/16 15:59:38 jdolecek Exp $	*/

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

#ifndef _PFCREG_H_
#define _PFCREG_H_
#include <sh3/devreg.h>

/* address definitions for pin function controller (PFC)*/

#define SH3_PCTR		0xffffff76	/* 16bit */
#define SH3_PDTR		0xffffff78	/*  8bit */
#define SH3_SCSPTR		0xffffff7c	/*  8bit */

/* SH7709, SH7709A */
#define SH7709_PACR		0xa4000100	/* 16bit */
#define SH7709_PBCR		0xa4000102	/* 16bit */
#define SH7709_PCCR		0xa4000104	/* 16bit */
#define SH7709_PDCR		0xa4000106	/* 16bit */
#define SH7709_PECR		0xa4000108	/* 16bit */
#define SH7709_PFCR		0xa400010a	/* 16bit */
#define SH7709_PGCR		0xa400010c	/* 16bit */
#define SH7709_PHCR		0xa400010e	/* 16bit */
#define SH7709_PJCR		0xa4000110	/* 16bit */
#define SH7709_PKCR		0xa4000112	/* 16bit */
#define SH7709_PLCR		0xa4000114	/* 16bit */
#define SH7709_SCPCR		0xa4000116	/* 16bit */
#define SH7709_PADR		0xa4000120	/*  8bit */
#define SH7709_PBDR		0xa4000122	/*  8bit */
#define SH7709_PCDR		0xa4000124	/*  8bit */
#define SH7709_PDDR		0xa4000126	/*  8bit */
#define SH7709_PEDR		0xa4000128	/*  8bit */
#define SH7709_PFDR		0xa400012a	/*  8bit */
#define SH7709_PGDR		0xa400012c	/*  8bit */
#define SH7709_PHDR		0xa400012e	/*  8bit */
#define SH7709_PJDR		0xa4000130	/*  8bit */
#define SH7709_PKDR		0xa4000132	/*  8bit */
#define SH7709_PLDR		0xa4000134	/*  8bit */
#define SH7709_SCPDR		0xa4000136	/*  8bit */

#define SH4_PCTRA		0xff80002c	/* 32bit */
#define SH4_PDTRA		0xff800030	/* 16bit */
#define SH4_PCTRB		0xff800040	/* 32bit */
#define SH4_PDTRB		0xff800044	/* 16bit */
#define SH4_GPIOIC		0xff800048	/* 16bit */
#define SH4_SCSPTR1		0xffe0001c	/*  8bit */
#define SH4_SCSPTR2		0xffe80020	/* 16bit */

#endif /* _PFCREG_H_ */

