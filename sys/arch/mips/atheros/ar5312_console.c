/* $NetBSD: ar5312_console.c,v 1.1 2006/09/26 06:37:32 gdamore Exp $ */

/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * Portions of this code were written by Garrett D'Amore for the
 * Champaign-Urbana Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file includes a implementation specific console for AR5312.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ar5312_console.c,v 1.1 2006/09/26 06:37:32 gdamore Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include "opt_memsize.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>

#include <dev/cons.h>

#include <mips/cache.h>
#include <mips/locore.h>
#include <mips/cpuregs.h>

#include <mips/atheros/include/ar5312reg.h>
#include <mips/atheros/include/ar531xvar.h>
#include <mips/atheros/include/arbusvar.h>
#include "com.h"

void
ar531x_consinit(void)
{
	/*
	 * Everything related to console initialization is done
	 * in mach_init().
	 */
#if NCOM > 0
	/* Setup polled serial for early console I/O */
	/* XXX: pass in CONSPEED? */
	com_arbus_cnattach(AR5312_UART0_BASE, ar531x_bus_freq());
#else
	panic("Not configured to use serial console!\n");
	/* not going to see that message now, are we? */
#endif
}

/*
 * Early console support.
 */
#define	STRIDE(x)	((x*4) + MIPS_PHYS_TO_KSEG1(AR5312_UART0_BASE + 3))
#define	THR		STRIDE(0)
#define	RHR		STRIDE(0)
#define	DLBL		STRIDE(0)
#define	DLBH		STRIDE(1)
#define	IER		STRIDE(1)
#define	FCR		STRIDE(2)
#define	LCR		STRIDE(3)
#define	MCR		STRIDE(4)
#define	LSR		STRIDE(5)
#define	MSR		STRIDE(6)
#define	SCRATCH		STRIDE(7)
#define	INB(x)		(*(volatile char *)(x))
#define	OUTB(x, v)	(*(volatile char *)(x) = (v))

/* line status register */
#define	LSR_RCV_FIFO	0x80
#define	LSR_TSRE	0x40	/* Transmitter empty: byte sent */
#define	LSR_TXRDY	0x20	/* Transmitter buffer empty */
#define	LSR_BI		0x10	/* Break detected */
#define	LSR_FE		0x08	/* Framing error: bad stop bit */
#define	LSR_PE		0x04	/* Parity error */
#define	LSR_OE		0x02	/* Overrun, lost incoming byte */
#define	LSR_RXRDY	0x01	/* Byte ready in Receive Buffer */
#define	LSR_RCV_MASK	0x1f	/* Mask for incoming data or error */

static void
ar5312_putc(dev_t dev, int c)
{
	unsigned char	lsr = 0;

	while (!(lsr & LSR_TXRDY)) {
		lsr = INB(LSR);
	}
	OUTB(THR, (char)c);
}

static int
ar5312_getc(dev_t dev)
{
	unsigned char	lsr = 0, dat;

	while (!(lsr & LSR_RXRDY)) {
		lsr = INB(LSR);
	}
	dat = INB(RHR);
	return (dat);
}

static void
ar5312_flush(dev_t dev)
{
	unsigned char	lsr = 0;
	while (!(lsr & LSR_TSRE)) {
		lsr = INB(LSR);
	}
}


void
ar531x_early_console(void)
{
	static struct consdev promcn = {
		.cn_probe = NULL,
		.cn_init = NULL,
		.cn_getc = ar5312_getc,
		.cn_putc = ar5312_putc,
		.cn_pollc = nullcnpollc,
		.cn_bell = NULL,
		.cn_halt = NULL,
		.cn_flush = ar5312_flush,
		.cn_dev = makedev(0, 0),
		.cn_pri = CN_DEAD,
	};

	cn_tab = &promcn;
}

