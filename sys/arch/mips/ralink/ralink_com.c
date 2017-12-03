/*	$NetBSD: ralink_com.c,v 1.3.6.2 2017/12/03 11:36:28 jdolecek Exp $	*/
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

/*-
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
 *    This product includes software developed by Garrett D'Amore.
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

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
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
 *    @(#)com.c    7.5 (Berkeley) 5/16/91
 */

/* ralink_com.c -- Ralink 3052 uart console driver */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ralink_com.c,v 1.3.6.2 2017/12/03 11:36:28 jdolecek Exp $");

#include "locators.h"
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/termios.h>
#include <sys/ttydefaults.h>

#include <dev/cons.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <mips/cpuregs.h>

#include <mips/ralink/ralink_reg.h>
#include <mips/ralink/ralink_var.h>

#include "opt_com.h"

struct ralink_com_softc {
	struct com_softc sc_com;
	void *sc_ih;
	bus_addr_t sc_addr;
	int sc_irq;
};

static int ralink_com_match(device_t, cfdata_t , void *);
static void ralink_com_attach(device_t, device_t, void *);
static void ralink_com_initmap(struct com_regs *regsp);

CFATTACH_DECL_NEW(ralink_com, sizeof(struct ralink_com_softc),
	ralink_com_match, ralink_com_attach, NULL, NULL);

#define CONMODE	\
	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

#ifndef COM_REGMAP
#error  COM_REGMAP not defined!
#endif

#ifndef RALINK_CONADDR
#define RALINK_CONADDR	RA_UART_LITE_BASE	/* default console is UART_LITE */
#endif

/* address/irq/rst/gpiomode mappings */
static struct {
	bus_addr_t addr;
	int irq;
	uint32_t rst;
	uint32_t gpiomode;
} ralink_uart_maps[] = {
#ifdef MT7628
	{ RA_UART_LITE_BASE, RA_IRQ_UARTL, RST_UART0_7628, GPIO1MODE_UART0 },
	{ RA_UART1_BASE,     RA_IRQ_UART1, RST_UART1_7628, GPIO1MODE_UART1 },
	{ RA_UART2_BASE,     RA_IRQ_UART2, RST_UART2_7628, GPIO1MODE_UART2 },
#else
	{ RA_UART_BASE,      RA_IRQ_UARTF, RST_UART,  GPIOMODE_UARTF0 },
	{ RA_UART_LITE_BASE, RA_IRQ_UARTL, RST_UARTL, GPIOMODE_UARTL }
#endif
};

static inline int
ra_uart2irq(bus_addr_t addr)
{
	int i;
	for (i = 0; __arraycount(ralink_uart_maps); i++)
		if (ralink_uart_maps[i].addr == addr)
			return ralink_uart_maps[i].irq;
	return -1;
}

static inline uint32_t
ra_uart2rst(bus_addr_t addr)
{
	int i;
	for (i = 0; __arraycount(ralink_uart_maps); i++)
		if (ralink_uart_maps[i].addr == addr)
			return ralink_uart_maps[i].rst;
	return 0;
}

static inline uint32_t
ra_uart2gpiomode(bus_addr_t addr)
{
	int i;
	for (i = 0; __arraycount(ralink_uart_maps); i++)
		if (ralink_uart_maps[i].addr == addr)
			return ralink_uart_maps[i].gpiomode;
	return 0;
}

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

#ifdef RALINK_CONSOLE_EARLY
static int
ralink_cngetc(dev_t dv)
{
	if ((uart_read(RA_UART_LSR) & LSR_RXRDY) == 0)
		return -1;

	return uart_read(RA_UART_RBR) & 0xff;
}

static void
ralink_cnputc(dev_t dv, int c)
{
	int timo = 150000;

	while ((uart_read(RA_UART_LSR) & LSR_TXRDY) == 0 && --timo > 0)
		;

	uart_write(RA_UART_TBR, c);
	__asm __volatile("sync");

	timo = 150000;
	while ((uart_read(RA_UART_LSR) & LSR_TSRE) == 0 && --timo > 0)
		;
}

static struct consdev ralink_earlycons = {
	.cn_putc = ralink_cnputc,
	.cn_getc = ralink_cngetc,
	.cn_pollc = nullcnpollc,
};

void
ralink_console_early(void)
{
	cn_tab = &ralink_earlycons;
}
#endif /* RALINK_CONSOLE_EARLY */


int
ralink_com_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct mainbus_attach_args *ma;
	bus_addr_t addr;

	ma = aux;
	addr = ma->ma_addr;
	if (addr == MAINBUSCF_ADDR_DEFAULT)
		addr = RA_UART_LITE_BASE;

	if (ra_uart2irq(addr) < 0)
		return 0;

	if (cn_tab == NULL || cn_tab->cn_pri < CN_NORMAL) {
		printf("NULL console set, don't install ourselves "
		    "(of course this shouldn't print)");
		return 0;
	}

	/*
	 * If we got this far, assume we want to install it as the console.
	 * No need to probe. Future possibilities include checking to see if it
	 * is console or KGDB but now it is our only console method if we aren't
	 * forcing a null console
	 */
	return 1;
}

void
ralink_com_attach(device_t parent, device_t self, void *aux)
{
	const struct mainbus_attach_args *ma = aux;
	struct ralink_com_softc * const rtsc = device_private(self);
	struct com_softc * const sc = &rtsc->sc_com;
	bus_space_handle_t ioh;
	int error;

	/* opt addr and irq */
	rtsc->sc_addr = ma->ma_addr;
	if (rtsc->sc_addr == MAINBUSCF_ADDR_DEFAULT)
		rtsc->sc_addr = RA_UART_LITE_BASE;
	rtsc->sc_irq = ra_uart2irq(rtsc->sc_addr);

	if ((error = bus_space_map(ma->ma_memt, rtsc->sc_addr,
	    RA_UART_SIZE, 0, &ioh)) != 0) {
		aprint_error(": can't map registers, error=%d\n", error);
		return;
	}

	sc->sc_dev = self;
	sc->sc_frequency = RA_UART_FREQ;
	sc->sc_regs.cr_nports = 32;
#if defined(MT7628)
	sc->sc_type = COM_TYPE_NORMAL;
#else
	sc->sc_type = COM_TYPE_AU1x00;
#endif
	sc->enabled = 1;

	/* reset hardware if not a console */
	if (rtsc->sc_addr != RALINK_CONADDR) {
		uint32_t r;

		/* reset */
		r = sysctl_read(RA_SYSCTL_RST);
		r |= ra_uart2rst(rtsc->sc_addr);
		sysctl_write(RA_SYSCTL_RST, r);
		r ^= ra_uart2rst(rtsc->sc_addr);
		sysctl_write(RA_SYSCTL_RST, r);

		/* make sure we are in UART mode */
		r = sysctl_read(RA_SYSCTL_GPIOMODE);
		r &= ra_uart2gpiomode(rtsc->sc_addr);
		r |= __SHIFTIN(0, ra_uart2gpiomode(rtsc->sc_addr));
		sysctl_write(RA_SYSCTL_GPIOMODE, r);
	}

	COM_INIT_REGS(sc->sc_regs, ma->ma_memt, ioh, rtsc->sc_addr);
	ralink_com_initmap(&sc->sc_regs);

	rtsc->sc_ih = ra_intr_establish(rtsc->sc_irq, comintr, sc, 1);
	com_attach_subr(sc);
}

static void
ralink_com_initmap(struct com_regs *regsp)
{
	regsp->cr_map[COM_REG_RXDATA] = RA_UART_RBR;
	regsp->cr_map[COM_REG_TXDATA] = RA_UART_TBR;
	regsp->cr_map[COM_REG_DLBL]   = RA_UART_DLL;
#if defined(MT7628)
	regsp->cr_map[COM_REG_DLBH]   = RA_UART_DLM;
#endif
	regsp->cr_map[COM_REG_IER]    = RA_UART_IER;
	regsp->cr_map[COM_REG_IIR]    = RA_UART_IIR;
	regsp->cr_map[COM_REG_FIFO]   = RA_UART_FCR;
	regsp->cr_map[COM_REG_LCR]    = RA_UART_LCR;
	regsp->cr_map[COM_REG_MCR]    = RA_UART_MCR;
	regsp->cr_map[COM_REG_LSR]    = RA_UART_LSR;
	regsp->cr_map[COM_REG_MSR]    = RA_UART_MSR;
}

void
ralink_com_early(int silent)
{
	struct com_regs regs;
	uint32_t r;
	int error;

	/* reset */
	r = sysctl_read(RA_SYSCTL_RST);
	r |= ra_uart2rst(RALINK_CONADDR);
	sysctl_write(RA_SYSCTL_RST, r);
	r ^= ra_uart2rst(RALINK_CONADDR);
	sysctl_write(RA_SYSCTL_RST, r);

	if (silent) {
		/*
		 * put us in PIO mode,
		 * effectively tri-stating the UARTL block
		 */
		r = sysctl_read(RA_SYSCTL_GPIOMODE);
		r &= ra_uart2gpiomode(RALINK_CONADDR);
		r |= __SHIFTIN(1, ra_uart2gpiomode(RALINK_CONADDR));
		sysctl_write(RA_SYSCTL_GPIOMODE, r);
	} else {
		/* make sure we are in UART mode */
		r = sysctl_read(RA_SYSCTL_GPIOMODE);
		r &= ra_uart2gpiomode(RALINK_CONADDR);
		r |= __SHIFTIN(0, ra_uart2gpiomode(RALINK_CONADDR));
		sysctl_write(RA_SYSCTL_GPIOMODE, r);
	}

	uart_write(RA_UART_IER, 0);		/* disable interrupts */
	uart_write(RA_UART_FCR, 0);		/* disable fifos */

	/* set baud rate */
	uart_write(RA_UART_LCR,
	    UART_LCR_WLS0 | UART_LCR_WLS1 | UART_LCR_DLAB);
	uart_write(RA_UART_DLL,
	    (RA_UART_FREQ / RA_SERIAL_CLKDIV / RA_BAUDRATE) & 0xffff);
#if defined(MT7628)
	uart_write(RA_UART_DLM,
	    ((RA_UART_FREQ / RA_SERIAL_CLKDIV / RA_BAUDRATE) & 0xffff) >> 8);
#endif
	uart_write(RA_UART_LCR, UART_LCR_WLS0 | UART_LCR_WLS1);

	regs.cr_iot = &ra_bus_memt;
	regs.cr_iobase = RALINK_CONADDR;
	regs.cr_nports = 32;
	ralink_com_initmap(&regs);

	if ((error = bus_space_map(regs.cr_iot, regs.cr_iobase, regs.cr_nports,
	    0, &regs.cr_ioh)) != 0) {
		return;
	}

#if defined(MT7628)
	comcnattach1(&regs, RA_BAUDRATE, RA_UART_FREQ,
	    COM_TYPE_NORMAL, CONMODE);
#else
	/* Ralink UART has a 16-bit rate latch (like the AU1x00) */
	comcnattach1(&regs, RA_BAUDRATE, RA_UART_FREQ,
	    COM_TYPE_AU1x00, CONMODE);
#endif
}
