/* $NetBSD: ascreg.h,v 1.2 2025/01/12 09:28:26 nat Exp $ */

/*-
 * Copyright (c) 2017, 2023 Nathanial Sloss <nathanialsloss@yahoo.com.au>
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_ARCH_MAC68K_OBIO_ASCREG_H
#define _SYS_ARCH_MAC68K_OBIO_ASCREG_H

#define ASCIRQ		5

#define FIFO_A		0
#define FIFO_B		0x400
#define FIFO_A_ALT	0x1000
#define FIFO_B_ALT	0x1800
#define FIFO_LEN	0x400

#define VERLOC		0x800

#define ASCMODE		0x801		
#define MODESTOP	0
#define MODEFIFO	1
#define MODEWAVE	2 		/* not in easc */

#define ASCTRL		0x802
#define UNDERRUN	__BIT(7)
#define STEREO		__BIT(1)
#define ANAPWM		__BIT(0)

#define FIFOPARAM	0x803
#define CLEARFIFO	__BIT(7)
#define NONCOMP		__BIT(1)
#define ROMCOMP		__BIT(0)

#define FIFOSTATUS	0x804
#define A_HALF		__BIT(0)
#define A_FULL		__BIT(1)
#define B_HALF		__BIT(2)
#define B_FULL		__BIT(3)

#define INTVOL		0x806		/* b2-b4 Int volume. b5-b7 ext. */

#define ASCRATE		0x807
#define MACFREQ		0 		/* 22254 Hz */
#define F22KHZ		2		/* 22050 Hz */
#define F44KHZ		3		/* 44100 Hz */	

#define APLAYREC	0x80a
#define RECORDA		__BIT(0)
#define REC22KHZ	__BIT(1)

#define ASCTEST		0x80f

#define	A_WRITEPTRHI	0xf00
#define	A_WRITEPTRLO	0xf01
#define	A_READPTRHI	0xf02
#define	A_READPTRLO	0xf03
#define	B_WRITEPTRHI	0xf20
#define	B_WRITEPTRLO	0xf21
#define	B_READPTRHI	0xf22
#define	B_READPTRLO	0xf23

#define MACOS_HIGH_VOL	0x36  /* Should NOT exceed this value */

#define A_LEFT_VOL	0xf06
#define A_RIGHT_VOL	0xf07
#define B_LEFT_VOL	0xf26
#define B_RIGHT_VOL	0xf27

#define FIFOCTRLA	0xf08
#define FIFOCTRLB	0xf28
#define CDQUALITY	0x81
#define MACDEFAULTS	0x80

#define IRQA		0xf09
#define IRQB		0xf29
#define DISABLEHALFIRQ	__BIT(0)

#endif /* !_SYS_ARCH_MAC68K_OBIO_ASCREG_H */
