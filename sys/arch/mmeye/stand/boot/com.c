/*	$NetBSD: com.c,v 1.1 2011/03/03 05:59:37 kiyohara Exp $	*/

/*-
 * Copyright (c) 2008 Izumi Tsutsui.  All rights reserved.
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
/*-
 * Copyright (c) 1993, 1994, 1995, 1996, 1997
 *	Charles M. Hannum.  All rights reserved.
 *
 * Interrupt processing and hardware flow control partly based on code from
 * Onno van der Linden and Gordon Ross.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
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
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

#ifdef CONS_COM

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <dev/ic/comreg.h>

#include <machine/cpu.h>

#include "boot.h"
#include "ns16550.h"

#define CSR_READ(base, reg)		(*(volatile uint8_t *)((base) + (reg)))
#define CSR_WRITE(base, reg, val)					\
	do {								\
		*(volatile uint8_t *)((base) + (reg)) = (val);		\
	} while (/* CONSTCOND */ 0)

#define	COM_FREQ	1843200	/* 16-bit baud rate divisor */
#define	COM_TOLERANCE	30	/* baud rate tolerance, in 0.1% units */

static int comspeed(long);

static int
comspeed(long speed)
{
#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	int x, err;
	long frequency = COM_FREQ * 10;

	if (speed <= 0)
		return -1;
	x = divrnd((frequency / 16), speed);
	if (x <= 0)
		return -1;
	err = divrnd(((quad_t)frequency) * 1000 / 16, speed * x) - 1000;
	if (err < 0)
		err = -err;
	if (err > COM_TOLERANCE)
		return -1;
	return x;

#undef	divrnd
}

void *
com_init(int addr, int speed)
{
	uint8_t *com_port;

	com_port = (void *)addr;

	CSR_WRITE(com_port, com_lctl, LCR_DLAB);
	speed = comspeed(speed);
	CSR_WRITE(com_port, com_dlbl, speed);
	CSR_WRITE(com_port, com_dlbh, speed >> 8);

	CSR_WRITE(com_port, com_lctl, LCR_PNONE | LCR_8BITS);
	CSR_WRITE(com_port, com_mcr, MCR_RTS | MCR_DTR);
	CSR_WRITE(com_port, com_fifo,
	    FIFO_XMT_RST | FIFO_RCV_RST | FIFO_ENABLE);
	CSR_WRITE(com_port, com_ier, 0);

	return com_port;
}

void
com_putc(void *dev, int c)
{
	volatile uint8_t *com_port = dev;

	while ((CSR_READ(com_port, com_lsr) & LSR_TXRDY) == 0)
		;

	CSR_WRITE(com_port, com_data, c);
}

int
com_getc(void *dev)
{
	volatile uint8_t *com_port = dev;

	while ((CSR_READ(com_port, com_lsr) & LSR_RXRDY) == 0)
		;

	return CSR_READ(com_port, com_data);
}

int
com_scankbd(void *dev)
{
	volatile uint8_t *com_port = dev;

	if ((CSR_READ(com_port, com_lsr) & LSR_RXRDY) == 0)
		return -1;

	return CSR_READ(com_port, com_data);
}
#endif /* CONS_SERIAL */
