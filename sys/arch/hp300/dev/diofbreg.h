/*	$NetBSD: diofbreg.h,v 1.1.2.3 2011/02/17 11:59:38 bouyer Exp $	*/
/*	$OpenBSD: diofbreg.h,v 1.3 2007/01/07 15:13:52 miod Exp $	*/

/*
 * Copyright (c) 1991 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: grfreg.h 1.6 92/01/31$
 *
 *	@(#)grfreg.h	8.1 (Berkeley) 6/10/93
 */

/* 300 bitmapped display hardware primary id */
#define GRFHWID		0x39

/* 300 internal bitmapped display address */
#define GRFIADDR	0x560000

/* 300 hardware secondary ids */
#define GID_GATORBOX	1
#define	GID_TOPCAT	2
#define GID_RENAISSANCE	4
#define GID_LRCATSEYE	5
#define GID_HRCCATSEYE	6
#define GID_HRMCATSEYE	7
#define GID_DAVINCI	8
#define GID_XXXCATSEYE	9
#define GID_XGENESIS   11
#define GID_TIGER      12
#define GID_YGENESIS   13
#define GID_HYPERION   14
#define GID_A1474MID   16
#define GID_A147xVGA   17

#ifndef	_LOCORE
struct	diofbreg {
	uint8_t		:8;
	uint8_t		id;		/* id and reset register	0x01 */
	uint8_t 	sec_interrupt;	/* secondary interrupt register	0x02 */
	uint8_t		interrupt;	/* interrupt register		0x03 */
	uint8_t		:8;
	uint8_t		fbwmsb;		/* frame buffer width MSB	0x05 */
	uint8_t		:8;
	uint8_t		fbwlsb;		/* frame buffer height LSB	0x07 */
	uint8_t		:8;
	uint8_t		fbhmsb;		/* frame buffer height MSB	0x09 */
	uint8_t		:8;
	uint8_t		fbhlsb;		/* frame buffer height LSB	0x0b */
	uint8_t		:8;
	uint8_t		dwmsb;		/* display width MSB		0x0d */
	uint8_t		:8;
	uint8_t		dwlsb;		/* display width LSB		0x0f */
	uint8_t		:8;
	uint8_t		dhmsb;		/* display height MSB		0x11 */
	uint8_t		:8;
	uint8_t		dhlsb;		/* display height LSB		0x13 */
	uint8_t		:8;
	uint8_t		fbid;		/* frame buffer id		0x15 */
	uint8_t		pad2[0x45];
	uint8_t		num_planes;	/* number of color planes	0x5b */
	uint8_t		:8;
	uint8_t		fbomsb;		/* frame buffer offset MSB	0x5d */
	uint8_t		:8;
	uint8_t		fbolsb;		/* frame buffer offset LSB	0x5f */
};
#endif
