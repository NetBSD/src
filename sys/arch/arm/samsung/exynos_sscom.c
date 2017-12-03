/*	$NetBSD: exynos_sscom.c,v 1.5.10.3 2017/12/03 11:35:56 jdolecek Exp $ */

/*
 * Copyright (c) 2014 Reinoud Zandijk
 * Copyright (c) 2002, 2003 Fujitsu Component Limited
 * Copyright (c) 2002, 2003 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: exynos_sscom.c,v 1.5.10.3 2017/12/03 11:35:56 jdolecek Exp $");

#include "opt_sscom.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/timepps.h>
#include <sys/vnode.h>

#include <machine/intr.h>
#include <sys/bus.h>

#include <arm/samsung/sscom_reg.h>
#include <arm/samsung/sscom_var.h>
#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_var.h>
#include <sys/termios.h>

#include <dev/fdt/fdtvar.h>

#include <evbarm/exynos/platform.h>

extern int num_exynos_uarts_entries;
extern int exynos_uarts[];

static int sscom_match(device_t, cfdata_t, void *);
static void sscom_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(exynos_sscom, sizeof(struct sscom_softc), sscom_match,
    sscom_attach, NULL, NULL);

static const char * const compatible[] = {
	"samsung,exynos4210-uart",
	NULL
};

static int
sscom_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
exynos_unmask_interrupts(struct sscom_softc *sc, int intbits)
{
	int psw = disable_interrupts(IF32_bits);
	uint32_t val;

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSCOM_UINTM);
	val &= ~intbits;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSCOM_UINTM, val);

	restore_interrupts(psw);
}

static void
exynos_mask_interrupts(struct sscom_softc *sc, int intbits)
{
	int psw = disable_interrupts(IF32_bits);
	uint32_t val;

	val = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SSCOM_UINTM);
	val |= intbits;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSCOM_UINTM, val);

	restore_interrupts(psw);
}

static void
exynos_change_txrx_interrupts(struct sscom_softc *sc, bool unmask_p,
    u_int flags)
{
	int intbits = 0;
	if (flags & SSCOM_HW_RXINT)
		intbits |= UINT_RXD;
	if (flags & SSCOM_HW_TXINT)
		intbits |= UINT_TXD;
	if (unmask_p) {
		exynos_unmask_interrupts(sc, intbits);
	} else {
		exynos_mask_interrupts(sc, intbits);
	}
}

static void
exynos_clear_interrupts(struct sscom_softc *sc, u_int flags)
{
	uint32_t val = 0;

	if (flags & SSCOM_HW_RXINT)
		val |= UINT_RXD;
	if (flags & SSCOM_HW_TXINT)
		val |= UINT_TXD;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SSCOM_UINTP, val);
}

static void
sscom_attach(device_t parent, device_t self, void *aux)
{
	struct sscom_softc *sc = device_private(self);
	struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	struct clk *clk_uart, *clk_uart_baud0;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(bst, addr, size, 0, &bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	clk_uart = fdtbus_clock_get(phandle, "uart");
	if (clk_uart != NULL) {
		if (clk_enable(clk_uart) != 0) {
			aprint_error(": couldn't enable uart clock\n");
			return;
		}
	}
	clk_uart_baud0 = fdtbus_clock_get(phandle, "clk_uart_baud0");
	if (clk_uart_baud0 == NULL) {
		aprint_error(": couldn't get baud clock\n");
		return;
	}
	if (clk_enable(clk_uart_baud0) != 0) {
		aprint_error(": couldn't enable baud clock\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_iot = bst = faa->faa_bst;
	sc->sc_ioh = bsh;
	sc->sc_unit = phandle;
	sc->sc_frequency = clk_get_rate(clk_uart_baud0);

	sc->sc_change_txrx_interrupts = exynos_change_txrx_interrupts;
	sc->sc_clear_interrupts = exynos_clear_interrupts;

	/* not used here, but do initialise */
	sc->sc_rx_irqno = 0;
	sc->sc_tx_irqno = 0;

	if (sscom_is_console(sc->sc_iot, phandle, &sc->sc_ioh))
		aprint_normal(" (console)");

	aprint_normal("\n");

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		return;
	}

	void *ih = fdtbus_intr_establish(phandle, 0, IPL_SERIAL,
	    FDT_INTR_MPSAFE, sscomintr, sc);
	if (ih == NULL)
		aprint_error_dev(self, "failed to establish interrupt\n");

	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sscom_attach_subr(sc);

}


#if 0
int
exynos_sscom_cnattach(bus_space_tag_t iot, int unit, int rate,
    int frequency, tcflag_t cflag)
{
	return sscom_cnattach(iot, exynos_uart_config + unit,
	    rate, frequency, cflag);
}

#ifdef KGDB
int
exynos_sscom_kgdb_attach(bus_space_tag_t iot, int unit, int rate,
    int frequency, tcflag_t cflag)
{
	return sscom_kgdb_attach(iot, exynos_uart_config + unit,
	    rate, frequency, cflag);
}
#endif /* KGDB */
#endif


/*
 * Console support
 */

static int
exynos_sscom_console_match(int phandle)
{
	return of_match_compatible(phandle, compatible);
}

static void
exynos_sscom_console_consinit(struct fdt_attach_args *faa, u_int uart_freq)
{
	const struct sscom_uart_info info = {
		.iobase = 0,	/* Offset from bsh */
		.unit = faa->faa_phandle
	};
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	tcflag_t flags;
	int speed;

	fdtbus_get_reg(phandle, 0, &addr, &size);
	speed = fdtbus_get_stdout_speed();
	if (speed < 0)
		speed = 115200;	/* default */
	flags = fdtbus_get_stdout_flags();

	if (bus_space_map(bst, addr, size, 0, &bsh) != 0)
		panic("cannot map console UART");

	/* Calculate UART frequency from bootloader (XXX) */
	uint32_t freq = speed
	    * (16 * (bus_space_read_4(bst, bsh, SSCOM_UBRDIV) + 1)
		  + bus_space_read_4(bst, bsh, SSCOM_UFRACVAL));
	freq = (freq + speed / 2) / 1000;
	freq *= 1000;

	if (sscom_cnattach(bst, bsh, &info, speed, freq, flags) != 0)
		panic("cannot attach console UART");
}

static const struct fdt_console exynos_sscom_console = {
	.match = exynos_sscom_console_match,
	.consinit = exynos_sscom_console_consinit,
};

FDT_CONSOLE(exynos_sscom, &exynos_sscom_console);
