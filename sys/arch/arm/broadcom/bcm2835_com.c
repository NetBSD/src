/* $NetBSD: bcm2835_com.c,v 1.4 2017/12/10 21:38:26 skrll Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_com.c,v 1.4 2017/12/10 21:38:26 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835var.h>
#include <arm/broadcom/bcm2835_intr.h>

#include <dev/fdt/fdtvar.h>

#include <dev/ic/comvar.h>

static int	bcm_com_match(device_t, cfdata_t, void *);
static void	bcm_com_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcmcom, sizeof(struct com_softc),
	bcm_com_match, bcm_com_attach, NULL, NULL);

static const char * const compatible[] = {
	"brcm,bcm2835-aux-uart",
	NULL
};

static int
bcm_com_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
bcm_com_attach(device_t parent, device_t self, void *aux)
{
	struct com_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	bus_space_tag_t bst = faa->faa_a4x_bst;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	void *ih;

	sc->sc_dev = self;
	sc->sc_type = COM_TYPE_BCMAUXUART;

	int error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	if (com_is_console(bst, addr, &bsh) == 0 &&
	    bus_space_map(bst, addr, size, 0, &bsh) != 0) {
		aprint_error(": can't map device\n");
		return;
	}

	/* Enable clocks */
	struct clk *clk;
	for (int i = 0; (clk = fdtbus_clock_get_index(phandle, i)); i++) {
		if (clk_enable(clk) != 0) {
			aprint_error(": failed to enable clock #%d\n", i);
			return;
		}
		/* First clock is UARTCLK */
		if (i == 0)
			sc->sc_frequency = clk_get_rate(clk);
	}

	sc->sc_frequency *= 2;

	COM_INIT_REGS(sc->sc_regs, bst, bsh, addr);

	com_attach_subr(sc);
	aprint_naive("\n");

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	ih = fdtbus_intr_establish(phandle, 0, IPL_SERIAL, FDT_INTR_MPSAFE,
	    comintr, sc);
	if (ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);
}

static int
bcmaux_com_console_match(int phandle)
{

	return of_match_compatible(phandle, compatible);
}

static void
bcmaux_com_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_a4x_bst;
	bus_addr_t addr;
	tcflag_t flags;
	int speed;

	fdtbus_get_reg(phandle, 0, &addr, NULL);
	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200;	/* default */
	flags = fdtbus_get_stdout_flags();

	if (comcnattach(bst, addr, speed, uart_freq, COM_TYPE_BCMAUXUART,
	    flags))
		panic("Cannot initialize bcm com console");

	cn_set_magic("+++++");
}

static const struct fdt_console bcmaux_com_console = {
	.match = bcmaux_com_console_match,
	.consinit = bcmaux_com_console_consinit,
};

FDT_CONSOLE(bcmcom, &bcmaux_com_console);
