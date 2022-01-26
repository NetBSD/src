/*	$NetBSD: octeon_uart.c,v 1.10 2022/01/26 18:57:55 martin Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_uart.c,v 1.10 2022/01/26 18:57:55 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <sys/bus.h>
#include <sys/cpu.h>
#include <machine/intr.h>

#include <dev/cons.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <mips/cavium/include/iobusvar.h>
#include <mips/cavium/dev/octeon_uartreg.h>
#include <mips/cavium/dev/octeon_uartvar.h>
#include <mips/cavium/dev/octeon_ciureg.h>

struct octuart_iobus_softc {
	struct com_softc sc_com;
	int sc_irq;
	void *sc_ih;
};

static int	octuart_iobus_match(device_t, struct cfdata *, void *);
static void	octuart_iobus_attach(device_t, device_t, void *);
static int	octuart_com_enable(struct com_softc *);
static void	octuart_com_disable(struct com_softc *);

/* octputc() is not declared static so it can be used for debugging elsewhere */
void		octputc(dev_t, int);

/* XXX */
const bus_addr_t octuart_com_bases[] = {
	MIO_UART0_BASE,
	MIO_UART1_BASE
};
const struct com_regs octuart_com_regs = {
	.cr_map = {
		[COM_REG_RXDATA] =	MIO_UART_RBR_OFFSET,
		[COM_REG_TXDATA] =	MIO_UART_THR_OFFSET,
		[COM_REG_DLBL] =	MIO_UART_DLL_OFFSET,
		[COM_REG_DLBH] =	MIO_UART_DLH_OFFSET,
		[COM_REG_IER] =		MIO_UART_IER_OFFSET,
		[COM_REG_IIR] =		MIO_UART_IIR_OFFSET,
		[COM_REG_FIFO] =	MIO_UART_FCR_OFFSET,
		[COM_REG_EFR] =		0,
		[COM_REG_LCR] =		MIO_UART_LCR_OFFSET,
		[COM_REG_MCR] =		MIO_UART_MCR_OFFSET,
		[COM_REG_LSR] =		MIO_UART_LSR_OFFSET,
		[COM_REG_MSR] =		MIO_UART_MSR_OFFSET,
#if 0 /* XXX COM_TYPE_16750_NOERS */
		[COM_REG_USR] =		MIO_UART_USR_OFFSET,
		[COM_REG_SRR] =		MIO_UART_SRR_OFFSET
#endif
	}
};

CFATTACH_DECL_NEW(com_iobus, sizeof(struct octuart_iobus_softc),
    octuart_iobus_match, octuart_iobus_attach, NULL, NULL);

static int
octuart_iobus_match(device_t parent, struct cfdata *cf, void *aux)
{
	struct iobus_attach_args *aa = aux;
	int result = 0;

	if (strcmp(cf->cf_name, aa->aa_name) != 0)
		goto out;
	if (cf->cf_unit != aa->aa_unitno)
		goto out;
	result = 1;

out:
	return result;
}

static void
octuart_iobus_attach(device_t parent, device_t self, void *aux)
{
	struct octuart_iobus_softc *sc = device_private(self);
	struct com_softc *sc_com = &sc->sc_com;
	struct iobus_attach_args *aa = aux;
	int status;

	sc_com->sc_dev = self;
	com_init_regs(&sc_com->sc_regs, aa->aa_bust, 0, aa->aa_unit->addr);
	memcpy(sc_com->sc_regs.cr_map, octuart_com_regs.cr_map,
	    sizeof(octuart_com_regs.cr_map));

	sc->sc_irq = aa->aa_unit->irq;

	status = bus_space_map(
		aa->aa_bust,
		aa->aa_unit->addr,
		COM_NPORTS,
		0,
		&sc_com->sc_regs.cr_ioh);
	if (status != 0) {
		aprint_error(": can't map i/o space\n");
		return;
	}

	sc_com->sc_type = COM_TYPE_16550_NOERS;
	sc_com->sc_frequency = octeon_ioclock_speed();
	sc_com->enable = octuart_com_enable;
	sc_com->disable = octuart_com_disable;

	octuart_com_enable(sc_com);
	sc_com->enabled = 1;

	com_attach_subr(sc_com);

	sc->sc_ih = octeon_intr_establish(CIU_INT_UART_0 + device_unit(self),
	    IPL_SERIAL, comintr, sc_com);
	if (sc->sc_ih == NULL)
		panic("%s: can't establish interrupt\n",
		    device_xname(self));

	/* XXX disable if kgdb? */
}

static int
octuart_com_enable(struct com_softc *sc_com)
{
	struct com_regs *regsp = &sc_com->sc_regs;

	/* XXX Clear old busy detect interrupts */
	bus_space_read_1(regsp->cr_iot, regsp->cr_ioh,
	    MIO_UART_USR_OFFSET);

	return 0;
}

static void
octuart_com_disable(struct com_softc *sc_com)
{
	/*
	 * XXX chip specific procedure
	 */
}


#ifndef CONMODE
#define	CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif

int
octuart_com_cnattach(bus_space_tag_t bust, int portno, int speed)
{
	struct com_regs regs;

	com_init_regs(&regs, bust, 0, octuart_com_bases[portno]);
	memcpy(regs.cr_map, octuart_com_regs.cr_map,
	    sizeof(octuart_com_regs.cr_map));

	return comcnattach1(
		&regs,
		speed,
		octeon_ioclock_speed(),
		COM_TYPE_16550_NOERS,
		CONMODE);
}


/*
 * A very simple output-only console so early printf() can work.
 */
struct consdev early_console = {
	.cn_putc = octputc,
	.cn_pollc = nullcnpollc,
	.cn_dev = makedev(0, 0),
	.cn_pri = CN_DEAD
};
static int early_comcnrate;

void
octputc(dev_t dev, int c)
{

	octeon_xkphys_write_8(MIO_UART0_RBR, (uint8_t)c);
	delay(1000000 / (early_comcnrate / 10)); /* wait for char to drain */
}

void
octuart_early_cnattach(int rate)
{

	early_comcnrate = rate;
	cn_tab = &early_console;
}
