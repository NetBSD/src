/* $NetBSD: latchreg.h,v 1.1 2000/05/09 21:56:02 bjh21 Exp $ */
/*-
 * Copyright (c) 1997, 1998 Ben Harris
 * All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * latchreg.h - Archimedes internal latches
 */

/* Under NetBSD, Latch A is latch0 and Latch B is latch1. */

#ifndef _LATCHREG_H
#define _LATCHREG_H

/* Latch A controls some of the floppy disc system */

#define LATCH0_DRIVESEL0	0x01	/* Floppy drive 0 select */
#define LATCH0_DRIVESEL1	0x02	/* Floppy drive 1 select */
#define LATCH0_DRIVESEL2	0x04	/* Floppy drive 2 select */
#define LATCH0_DRIVESEL3	0x08	/* Floppy drive 3 select */
#define LATCH0_SIDESEL		0x10	/* Floppy side select, inverted. */
#define LATCH0_SIDE0		0x10
#define LATCH0_SIDE1		0x00	/* Just to be friendly... */
#define LATCH0_MOTORONOFF	0x20	/* Motor on/off */
#define LATCH0_INUSE		0x40	/* 'In Use' */

/* Latch B does all sorts of random stuff */

#define LATCH1_DENSITY		0x02	/* Floppy disc density setting */
#define LATCH1_DOUBLEDENSITY	0x00	/* Double density */
#define LATCH1_SINGLEDENSITY	0x02	/* Single density */
#define LATCH1_NOTFDCRESET	0x08	/* Floppy controller reset (inverted) */
#define LATCH1_LPTSTROBE	0x10	/* Printer strobe line */
#define LATCH1_AUX1		0x20	/* Spare (VIDC Enhancer?) */
#define LATCH1_AUX2		0x40	/* Spare */

#endif /* !_LATCHREG_H */
