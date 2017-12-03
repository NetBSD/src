/*	$NetBSD: console.c,v 1.2.12.1 2017/12/03 11:36:10 jdolecek Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
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
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: console.c,v 1.2.12.1 2017/12/03 11:36:10 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <dev/cons.h>

#include <mips/ralink/ralink_reg.h>
#include <mips/ralink/ralink_var.h>

#ifndef RALINK_CONADDR
#define RALINK_CONADDR	RA_UART_LITE_BASE	/* default console is UART_LITE */
#endif

static inline uint32_t
sysctl_read(const u_int offset)
{
	return *RA_IOREG_VADDR(RA_SYSCTL_BASE, offset);
}

static inline void
sysctl_write(const u_int offset, uint32_t val)
{
	*RA_IOREG_VADDR(RA_SYSCTL_BASE, offset) = val;
}

static inline uint32_t
uart_read(const u_int offset)
{
	return *RA_IOREG_VADDR(RALINK_CONADDR, offset);
}

static inline void
uart_write(const u_int offset, const uint32_t val)
{
	*RA_IOREG_VADDR(RALINK_CONADDR, offset) = val;
}

#ifdef RA_CONSOLE_EARLY 
static void
ra_console_putc(dev_t dev, int c)
{
	u_int timo;

	timo = 150000;
	do {
		if ((uart_read(RA_UART_LSR) & UART_LSR_TDRQ) != 0)
			break;
	} while(--timo != 0);

	uart_write(RA_UART_TBR, c);
	if (c == '\n')
		ra_console_putc (dev, '\r');
#if 1
	timo = 150000;
	do {
		if ((uart_read(RA_UART_LSR) & UART_LSR_TEMT) != 0)
			break;
	} while(--timo != 0); 
#endif
}

static int
ra_console_getc(dev_t dev)
{
	while ((uart_read(RA_UART_LSR) & UART_LSR_DR) == 0)
		;
	return (char)(uart_read(RA_UART_RBR) & 0xff);
}

static void
ra_console_flush(dev_t dev)
{
	while ((uart_read(RA_UART_LSR) & UART_LSR_TEMT) == 0)
		;
}

void
ra_console_early(void)
{
	uint32_t r;

	/* reset */
	r = sysctl_read(RA_SYSCTL_RST);
	r |= RST_UARTL;
	sysctl_write(RA_SYSCTL_RST, r);
	r ^= RST_UARTL;
	sysctl_write(RA_SYSCTL_RST, r);

	/* make sure we are in UART mode */
	r = sysctl_read(RA_SYSCTL_GPIOMODE);
	r &= ~GPIOMODE_UARTL;
	sysctl_write(RA_SYSCTL_GPIOMODE, r);

	uart_write(RA_UART_IER, 0);		/* disable interrupts */
	uart_write(RA_UART_FCR, 0);		/* disable fifos */

	/* set baud rate */
	uart_write(RA_UART_LCR,
		UART_LCR_WLS0 | UART_LCR_WLS1 | UART_LCR_DLAB);
	uart_write(RA_UART_DLL,
		(RA_UART_FREQ / RA_SERIAL_CLKDIV / RA_BAUDRATE)
			& 0xffff);
	uart_write(RA_UART_LCR,
		UART_LCR_WLS0 | UART_LCR_WLS1);

	static struct consdev lite_cn = {
	    .cn_probe = NULL,
	    .cn_init = NULL,
	    .cn_getc = ra_console_getc,
	    .cn_putc = ra_console_putc,
	    .cn_pollc = nullcnpollc,
	    .cn_bell = NULL,
	    .cn_halt = NULL,
	    .cn_flush = ra_console_flush,
	    .cn_dev = makedev(0, 0),
	    .cn_pri = CN_DEAD,
	};

	cn_tab = &lite_cn;

}
#endif	/* RA_CONSOLE_EARLY */

void
consinit(void)
{
	if (ra_check_memo_reg(SERIAL_CONSOLE)) {
		ralink_com_early(0);
		printf("Enabled early console\n");
	} else {
		/*
		 * this should be OK, but our shell configuration doesn't seem
		 * to be working, so create a silent console
		 */
		cn_tab = NULL;
		ralink_com_early(1);
		printf("Enabled silent console\n");
	}

#if 0
	/* update ddb escape sequence to '~~' to avoid the undocking issue */
	cn_set_magic("\x7E\x7E");
#endif
}
