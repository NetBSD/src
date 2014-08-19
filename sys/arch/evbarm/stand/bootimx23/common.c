/* $Id: common.c,v 1.3.4.3 2014/08/20 00:02:56 tls Exp $ */

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

#include <lib/libsa/stand.h>

#include <arm/imx/imx23_digctlreg.h>
#include <arm/imx/imx23_uartdbgreg.h>

#include "common.h"

/*
 * Delay us microseconds.
 */
void
delay(unsigned int us)
{
        volatile uint32_t *us_r;

	us_r = (uint32_t *)(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS);

        *us_r = 0;
        while (*us_r < us)
                ;

        return;
}

/*
 * Write character c to debug UART.
 */
void
putchar(int c)
{
	volatile uint8_t *fr_r, *dr_r;

	fr_r = (uint8_t *)(HW_UARTDBG_BASE + HW_UARTDBGFR);
	dr_r = (uint8_t *)(HW_UARTDBG_BASE + HW_UARTDBGDR);

        /* Wait until transmit FIFO has space for the new character. */
        while (*fr_r & HW_UARTDBGFR_TXFF)
                ;

	*dr_r = c;
#ifdef DIAGNOSTIC

	/* Flush: Wait until transmit FIFO contents are written to UART. */
	while (!(*fr_r & HW_UARTDBGFR_TXFE))
		;
#endif

	return;
}

/*
 * Read character from debug UART.
 */
int
getchar(void)
{
	volatile uint8_t *fr_r, *dr_r;

	fr_r = (uint8_t *)(HW_UARTDBG_BASE + HW_UARTDBGFR);
	dr_r = (uint8_t *)(HW_UARTDBG_BASE + HW_UARTDBGDR);

	/* Wait until receive FIFO has character(s) */
	while (*fr_r & HW_UARTDBGFR_RXFE)
		;

	return *dr_r;
}

void
vpanic(const char *fmt, va_list ap)
{
	printf(fmt, ap);
	for(;;);

	/* NOTREACHED */
}
