/* $NetBSD: latchreg.h,v 1.2.6.2 2002/06/23 17:33:46 jdolecek Exp $ */
/*-
 * Copyright (c) 1997, 1998, 2001 Ben Harris
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
/*
 * latchreg.h - Archimedes internal latches
 */

#ifndef _LATCHREG_H
#define _LATCHREG_H

/* Register offsets for latches (bus_space units) */

#define LATCH_A		0x10
#define LATCH_B		0x06

/* Latch A controls some of the floppy disc system (all active-low). */
#define LATCHA_NSEL0	0x01	/* Floppy drive 0 select */
#define LATCHA_NSEL1	0x02	/* Floppy drive 1 select */
#define LATCHA_NSEL2	0x04	/* Floppy drive 2 select */
#define LATCHA_NSEL3	0x08	/* Floppy drive 3 select */
#define LATCHA_NSIDE1	0x10	/* Floppy side select. */
#define LATCHA_NMOTORON	0x20	/* Motor on/off */
#define LATCHA_NINUSE	0x40	/* 'In Use' */
/* Bit 7 not used */

/* Latch B does all sorts of random stuff (mostly active-low). */
/* Bit 0 reserved */
#define LATCHB_NDDEN	0x02	/* Floppy disc density setting */
/* Bit 2 reserved */
#define LATCHB_NFDCR	0x08	/* Floppy controller reset */
#define LATCHB_NPSTB	0x10	/* Printer strobe line */
#define LATCHB_AUX1	0x20	/* Spare (VIDC Enhancer?) */
#define LATCHB_AUX2	0x40	/* Spare */
/* Bit 7 not used */

#endif /* !_LATCHREG_H */
