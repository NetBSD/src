/*	$NetBSD: vrc4173cmureg.h,v 1.1 2001/10/21 09:38:11 takemura Exp $	*/

/*-
 * Copyright (c) 2001 TAKEMURA Shin
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
 */

#define	VRC4173CMU_IOBASE	0x040
#define	VRC4173CMU_IOSIZE	0x002

#define	VRC4173CMU_CLKMSK	0x00
#define	VRC4173CMU_CLKMSK_48MOSC	(1<<12)
#define	VRC4173CMU_CLKMSK_48MPIN	(1<<11)
#define	VRC4173CMU_CLKMSK_48MUSB	(1<<10)
#define	VRC4173CMU_CLKMSK_AC97		(1<<8)
#define	VRC4173CMU_CLKMSK_CARD2		(1<<7)
#define	VRC4173CMU_CLKMSK_CARD1		(1<<6)
#define	VRC4173CMU_CLKMSK_USB		(1<<5)
#define	VRC4173CMU_CLKMSK_PS2CH2	(1<<4)
#define	VRC4173CMU_CLKMSK_PS2CH1	(1<<3)
#define	VRC4173CMU_CLKMSK_AIU		(1<<2)
#define	VRC4173CMU_CLKMSK_KIU		(1<<1)
#define	VRC4173CMU_CLKMSK_PIU		(1<<0)

#define	VRC4173CMU_SRST		0x02
#define	VRC4173CMU_SRST_AC97	(1<<3)
#define	VRC4173CMU_SRST_CARD2	(1<<2)
#define	VRC4173CMU_SRST_CARD1	(1<<1)
#define	VRC4173CMU_SRST_USB	(1<<0)
