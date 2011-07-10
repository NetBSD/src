/* $NetBSD: ar_console.c,v 1.2 2011/07/10 06:26:02 matt Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

/*
 * This file implement an Atheros dependent early console.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ar_console.c,v 1.2 2011/07/10 06:26:02 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/termios.h>

#include <dev/cons.h>

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>

#include <mips/atheros/include/platform.h>
#include <mips/atheros/include/arbusvar.h>

#include <mips/locore.h>
#include "com.h"

#include <dev/ic/ns16450reg.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

void
atheros_consinit(void)
{
	/*
	 * Everything related to console initialization is done
	 * in mach_init().
	 */
#if NCOM > 0
	/* Setup polled serial for early console I/O */
	/* XXX: pass in CONSPEED? */
	com_arbus_cnattach(platformsw->apsw_uart0_base,
	    atheros_get_uart_freq());
#else
	panic("Not configured to use serial console!\n");
	/* not going to see that message now, are we? */
#endif
}

/*
 * Early console support.
 */
static void
earlycons_putc(dev_t dev, int c)
{
	volatile uint32_t * const uart =
	    (volatile uint32_t *)MIPS_PHYS_TO_KSEG1(platformsw->apsw_uart0_base);

	while (!(uart[com_lsr] & htobe32(LSR_TXRDY)))
		continue;

	uart[com_data] = htobe32(c);
}

static int
earlycons_getc(dev_t dev)
{
	volatile uint32_t * const uart =
	    (volatile uint32_t *)MIPS_PHYS_TO_KSEG1(platformsw->apsw_uart0_base);

	while (!(uart[com_lsr] & htobe32(LSR_RXRDY)))
		continue;

	return (uint8_t) be32toh(uart[com_data]);
}

static void
earlycons_flush(dev_t dev)
{
	volatile uint32_t * const uart =
	    (volatile uint32_t *)MIPS_PHYS_TO_KSEG1(platformsw->apsw_uart0_base);

	while (!(uart[com_lsr] & htobe32(LSR_TSRE)))
		continue;
}


void
atheros_early_consinit(void)
{
	static struct consdev earlycons_cn = {
		.cn_probe = NULL,
		.cn_init = NULL,
		.cn_getc = earlycons_getc,
		.cn_putc = earlycons_putc,
		.cn_pollc = nullcnpollc,
		.cn_bell = NULL,
		.cn_halt = NULL,
		.cn_flush = earlycons_flush,
		.cn_dev = makedev(0, 0),
		.cn_pri = CN_DEAD,
	};

	cn_tab = &earlycons_cn;
}
