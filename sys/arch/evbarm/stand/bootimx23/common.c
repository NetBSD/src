/* $Id: common.c,v 1.3.4.2 2013/02/25 00:28:38 tls Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cdefs.h>

#include <arm/imx/imx23_digctlreg.h>
#include <arm/imx/imx23_uartdbgreg.h>

#include "common.h"

/*
 * Delay us microseconds.
 */
void
delay(unsigned int us)
{
	uint32_t start;
	uint32_t now;
	uint32_t elapsed;
	uint32_t total;
	uint32_t last;

	total = 0;
	last = 0;
	start = REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS);

	do {
		now = REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS);

		if (start <= now)
			elapsed = now - start;
		else	/* Take care of overflow. */
			elapsed = (UINT32_MAX - start) + 1 + now;

		total += elapsed - last;
		last = elapsed;

	} while (total < us);

	return;
}

/*
 * Write character to debug UART.
 */
void
putchar(int ch)
{

	/* Wait until transmit FIFO has space for the new character. */
	while (REG_RD_HW(HW_UARTDBG_BASE + HW_UARTDBGFR) & HW_UARTDBGFR_TXFF);

	REG_WR_BYTE(HW_UARTDBG_BASE + HW_UARTDBGDR,
	    __SHIFTIN(ch, HW_UARTDBGDR_DATA));
#ifdef DEBUG
	/* Flush: Wait until transmit FIFO contents are written to UART. */
	while (!(REG_RD_HW(HW_UARTDBG_BASE + HW_UARTDBGFR) &
	    HW_UARTDBGFR_TXFE));
#endif

	return;
}

/*
 * Read character from debug UART.
 */
int
getchar(void)
{

	/* Wait until receive FIFO has character(s) */

	while (REG_RD_HW(HW_UARTDBG_BASE + HW_UARTDBGFR) & HW_UARTDBGFR_RXFE);

	return REG_RD_HW(HW_UARTDBG_BASE + HW_UARTDBGDR) & 0xFF;
}
