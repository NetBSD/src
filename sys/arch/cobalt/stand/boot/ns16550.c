/*	$NetBSD: ns16550.c,v 1.4.4.1 2008/05/16 02:22:09 yamt Exp $	*/

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

#ifdef CONS_SERIAL

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

void *
com_init(int addr, int speed)
{
	uint8_t *com_port;

	com_port = (void *)MIPS_PHYS_TO_KSEG1(COM_BASE + addr);

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
