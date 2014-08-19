/*	$NetBSD: sscom.c,v 1.5.122.1 2014/08/20 00:02:56 tls Exp $ */


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
/* derived from ns16550.c */
/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/* 
 * This file provides the cons_init() function and console I/O routines
 * for boards that use built-in UART of Samsung's S3C2xx0 CPUs.
 */

#include <sys/types.h>
#include <arch/arm/s3c2xx0/s3c2xx0reg.h>
#ifdef	CPU_S3C2410
#include <arch/arm/s3c2xx0/s3c2410reg.h>
#endif
#ifdef	CPU_S3C2800
#include <arch/arm/s3c2xx0/s3c2800reg.h>
#endif
#include <lib/libsa/stand.h>

#include "board.h"

#ifndef SSCOM_TOLERANCE
#define	SSCOM_TOLERANCE	30	/* XXX: baud rate tolerance, in 0.1% units */
#endif

#define	INB(x)		*((volatile uint8_t *) ((CONADDR) + (x)))
#define	INW(x)		*((volatile uint32_t *) ((CONADDR) + (x)))
#define	OUTB(x, v)	(*((volatile uint8_t *) ((CONADDR) + (x))) = (v))
#define	OUTW(x, v)	(*((volatile uint32_t *) ((CONADDR) + (x))) = (v))

#define	ISSET(t,f)	((t) & (f))

static long get_com_freq(void);

static int
sscomspeed(long speed)
{
#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	int x, err;
	long pclk = get_com_freq();

	if (speed <= 0)
		return -1;
	x = divrnd(pclk / 16, speed);
	if (x <= 0)
		return -1;
	err = divrnd(((quad_t)pclk) * 1000 / 16, speed * x) - 1000;
	if (err < 0)
		err = -err;
	if (err > SSCOM_TOLERANCE)
		return -1;
	return x-1;

#undef	divrnd
}

void
cons_init(void)
{
	int rate;

	OUTW(SSCOM_UCON, 0);
	OUTB(SSCOM_UFCON, UFCON_TXTRIGGER_8 | UFCON_RXTRIGGER_8 |
	    UFCON_TXFIFO_RESET | UFCON_RXFIFO_RESET |
	    UFCON_FIFO_ENABLE);

	rate = sscomspeed(CONSPEED);
	OUTW(SSCOM_UBRDIV, rate);
	OUTW(SSCOM_ULCON, ULCON_PARITY_NONE|ULCON_LENGTH_8);

	/* enable UART */
	OUTW(SSCOM_UCON, UCON_TXMODE_INT|UCON_RXMODE_INT);
	OUTW(SSCOM_UMCON, UMCON_RTS);
}

#define sscom_rxrdy() (INB(SSCOM_UTRSTAT) & UTRSTAT_RXREADY)

int
getchar(void)
{
	uint8_t stat __unused;
	int c;

	while (!sscom_rxrdy())
		/* spin */ ;
	c = INB(SSCOM_URXH);
	stat = INB(SSCOM_UERSTAT);	/* XXX */

	return c;
}

static void
iputchar(int c)
{
	uint32_t stat;
	int timo;

	/* Wait for any pending transmission to finish. */
	timo = 50000;   
	while (ISSET(stat = INW(SSCOM_UFSTAT), UFSTAT_TXFULL) && --timo)
		/* spin */ ;

	OUTB(SSCOM_UTXH, c);

#if 0
	/* Wait for this transmission to complete. */
	timo = 1500000;
	while (!ISSET(stat = INW(SSCOM_UFSTAT), UFSTAT_TXFULL) && --timo)
		/* spin */ ;
#endif
}

void
putchar(int c)
{

	if (c == '\n')
		iputchar('\r');
	iputchar(c);
}


#define read_reg(addr)	(*(volatile uint32_t *)(addr))

static long
get_com_freq(void)
{
	long clk;
#ifdef	CPU_S3C2800
	uint32_t pllcon = read_reg(S3C2800_CLKMAN_BASE+CLKMAN_PLLCON);
	uint32_t div = read_reg(S3C2800_CLKMAN_BASE+CLKMAN_CLKCON);
#define	HDIV	CLKCON_HCLK
#define PDIV	CLKCON_PCLK
#endif
#ifdef	CPU_S3C2410
	uint32_t pllcon = read_reg(S3C2410_CLKMAN_BASE+CLKMAN_MPLLCON);
	uint32_t div = read_reg(S3C2410_CLKMAN_BASE+CLKMAN_CLKDIVN);
#define	HDIV	CLKDIVN_HDIVN
#define PDIV	CLKDIVN_PDIVN
#endif

	int mdiv = (pllcon & PLLCON_MDIV_MASK) >> PLLCON_MDIV_SHIFT;
	int pdiv = (pllcon & PLLCON_PDIV_MASK) >> PLLCON_PDIV_SHIFT;
	int sdiv = (pllcon & PLLCON_SDIV_MASK) >> PLLCON_SDIV_SHIFT;

#if XTAL_CLK < 1000   /* in MHz */
	clk = (XTAL_CLK * 1000000 * (8 + mdiv)) / ((pdiv + 2) << sdiv);
#else /* in Hz */
	clk = (XTAL_CLK * (8 + mdiv)) / ((pdiv + 2) << sdiv);
#endif

	/*printf( "M=%d P=%d S=%d\n", mdiv, pdiv, sdiv);*/

	if (div & HDIV)
		clk /= 2;
	if (div & PDIV)
		clk /= 2;

	return clk;
}
