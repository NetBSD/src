/*	$NetBSD: smdk2800_io_init.c,v 1.1 2003/07/30 18:54:22 bsh Exp $	*/

/*
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <arm/s3c2xx0/s3c2800reg.h>

#define EXTINTR_INIT	((EXTINTR_HIGH|EXTINTR_FALLING)<<28) | \
			((EXTINTR_HIGH|EXTINTR_FALLING)<<24) | \
			((EXTINTR_HIGH|EXTINTR_FALLING)<<20) | \
			((EXTINTR_HIGH|EXTINTR_FALLING)<<16) | \
			((EXTINTR_HIGH|EXTINTR_FALLING)<<12) | \
			((EXTINTR_HIGH|EXTINTR_FALLING)<<8) | \
			((EXTINTR_HIGH|EXTINTR_FALLING)<<4) | \
			((EXTINTR_HIGH|EXTINTR_FALLING))
#define FCLK		200000000
#define F_1MHZ		1000000

#define IOW(a, d)	(*(volatile unsigned int *)(a) = (d))
#define IOR(a)		(*(volatile unsigned int *)(a))
#define SETLED(d)	IOW(S3C2800_GPIO_BASE+GPIO_PDATC,(d))

void smdk2800_io_init(void);

void
smdk2800_io_init(void)
{
	unsigned int hclk;
	unsigned int pclk;
	unsigned int tmdat;

#define	O	PCON_OUTPUT
#define	I	PCON_INPUT
#define	A	PCON_ALTFUN
#define	_	0       
#define	_C(b7,b6,b5,b4,b3,b2,b1,b0) \
	((b7<<14)|(b6<<12)|(b5<<10)|(b4<<8)|(b3<<6)|(b2<<4)|(b1<<2)|(b0<<0))

	/* GPIO port */
	IOW(S3C2800_GPIO_BASE+GPIO_PCONA, _C(O,O,A,A,A,A,A,A));
	IOW(S3C2800_GPIO_BASE+GPIO_PUPA,  0xff);
	IOW(S3C2800_GPIO_BASE+GPIO_PCONB, _C(I,O,A,A,A,A,A,A));
	IOW(S3C2800_GPIO_BASE+GPIO_PCONC, _C(_,_,_,_,O,A,A,A));
	IOW(S3C2800_GPIO_BASE+GPIO_PUPC,  0xff);
	IOW(S3C2800_GPIO_BASE+GPIO_PCOND, _C(A,A,A,A,A,A,A,A));
	IOW(S3C2800_GPIO_BASE+GPIO_PUPD,  0xff);
	IOW(S3C2800_GPIO_BASE+GPIO_PCONE, _C(O,O,O,O,A,A,A,A));
	IOW(S3C2800_GPIO_BASE+GPIO_PUPE,  0xff);
	IOW(S3C2800_GPIO_BASE+GPIO_PCONF, _C(A,A,A,A,A,A,A,A));
	IOW(S3C2800_GPIO_BASE+GPIO_PUPF,  0xff);
	IOW(S3C2800_GPIO_BASE+GPIO_EXTINTR, EXTINTR_INIT);

#undef	O
#undef	I
#undef	A
#undef	_
#undef 	_C

	/* Get clock value */
	if(IOR(S3C2800_CLKMAN_BASE+CLKMAN_CLKCON) & CLKCON_HCLK)
		hclk = FCLK / 2;
	else
		hclk = FCLK;

	if(IOR(S3C2800_CLKMAN_BASE+CLKMAN_CLKCON) & CLKCON_PCLK)
		pclk = hclk / 2;
	else
		pclk = hclk;

	/* Timer */
	if((pclk/F_1MHZ) < 1)
		tmdat = 1<<16;
	else
		tmdat = (pclk/F_1MHZ)<<16;

#define TMDAT_INIT		0xf424

	IOW(S3C2800_TIMER0_BASE+TIMER_TMDAT, (tmdat | TMDAT_INIT));
	IOW(S3C2800_TIMER1_BASE+TIMER_TMDAT, (tmdat | TMDAT_INIT));
	IOW(S3C2800_TIMER2_BASE+TIMER_TMDAT, (tmdat | TMDAT_INIT));

	IOW(S3C2800_TIMER0_BASE+TIMER_TMCON, TMCON_MUX_DIV32 | TMCON_INTENA | TMCON_ENABLE);
	IOW(S3C2800_TIMER1_BASE+TIMER_TMCON, TMCON_MUX_DIV16 | TMCON_INTENA | TMCON_ENABLE);
	IOW(S3C2800_TIMER2_BASE+TIMER_TMCON, TMCON_MUX_DIV8 | TMCON_INTENA | TMCON_ENABLE);

	/* Interrupt controller */
	IOW(S3C2800_INTCTL_BASE+INTCTL_INTMOD, 0);
	IOW(S3C2800_INTCTL_BASE+INTCTL_INTMSK, 0);

	/* Initial complete */
	SETLED(0x0);	/* All LEDs on (o o o) */
}
