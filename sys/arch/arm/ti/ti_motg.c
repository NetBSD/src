/* $NetBSD: ti_motg.c,v 1.4 2021/01/27 03:10:20 thorpej Exp $ */
/*
 * Copyright (c) 2013 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ti_motg.c,v 1.4 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#include <arm/ti/ti_prcm.h>
#include <arm/ti/ti_otgreg.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/motgreg.h>
#include <dev/usb/motgvar.h>
#include <dev/usb/usbhist.h>

#include <dev/fdt/fdtvar.h>

#ifdef USB_DEBUG
#ifndef MOTG_DEBUG
#define motgdebug 0
#else
extern int motgdebug;
#endif /* MOTG_DEBUG */
#endif /* USB_DEBUG */

#define	DPRINTF(FMT,A,B,C,D)	USBHIST_LOGN(motgdebug,1,FMT,A,B,C,D)
#define	MOTGHIST_FUNC()		USBHIST_FUNC()
#define	MOTGHIST_CALLED(name)	USBHIST_CALLED(motgdebug)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,musb-am33xx" },
	DEVICE_COMPAT_EOL
};

/*
 * motg device attachement and driver,
 * for the per-port part of the controller: TI-specific part, phy and
 * MI Mentor OTG.
 */

struct ti_motg_softc {
	struct motg_softc	sc_motg;
	bus_space_tag_t		sc_ctrliot;
	bus_space_handle_t	sc_ctrlioh;
	void *			sc_ctrlih;
	int			sc_ctrlport;
};

static int	ti_motg_match(device_t, cfdata_t, void *);
static void	ti_motg_attach(device_t, device_t, void *);
static int	ti_motg_intr(void *);
static void	ti_motg_poll(void *);

CFATTACH_DECL_NEW(ti_motg, sizeof(struct ti_motg_softc),
    ti_motg_match, ti_motg_attach, NULL, NULL);

static int
ti_motg_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ti_motg_attach(device_t parent, device_t self, void *aux)
{
	struct ti_motg_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	char intrstr[128];
	bus_addr_t addr[2];
	bus_size_t size[2];
	uint32_t val;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	if (fdtbus_get_reg_byname(phandle, "mc", &addr[0], &size[0]) != 0 ||
	    fdtbus_get_reg_byname(phandle, "control", &addr[1], &size[1])) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": couldn't decode interrupt\n");
		return;
	}

	sc->sc_motg.sc_dev = self;
	sc->sc_ctrliot = faa->faa_bst;
	if (bus_space_map(sc->sc_ctrliot, addr[1], size[1], 0, &sc->sc_ctrlioh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_ctrlih = fdtbus_intr_establish_xname(phandle, 0, IPL_USB, 0,
	    ti_motg_intr, sc, device_xname(self));
	sc->sc_motg.sc_bus.ub_dmatag = faa->faa_dmat;

	val = TIOTG_USBC_READ4(sc, USBCTRL_REV);
	aprint_normal(": 0x%x version v%d.%d.%d", val,
	    (val >> 8) & 7, (val >> 6) & 3, val & 63);

	/* XXX configure mode */
#if 0
	if (sc->sc_ctrlport == 0)
		sc->sc_motg.sc_mode = MOTG_MODE_DEVICE;
	else
		sc->sc_motg.sc_mode = MOTG_MODE_HOST;
#else
	/* XXXXX
	 * Both ports always the host mode only.
	 * And motg(4) doesn't supports device and OTG modes.
	 */
	sc->sc_motg.sc_mode = MOTG_MODE_HOST;
#endif
	if (sc->sc_motg.sc_mode == MOTG_MODE_HOST) {
		val = TIOTG_USBC_READ4(sc, USBCTRL_MODE);
		val |= USBCTRL_MODE_IDDIGMUX;
		val &= ~USBCTRL_MODE_IDDIG;
		TIOTG_USBC_WRITE4(sc, USBCTRL_MODE, val);
		TIOTG_USBC_WRITE4(sc, USBCTRL_UTMI, USBCTRL_UTMI_FSDATAEXT);
	} else {
		val = TIOTG_USBC_READ4(sc, USBCTRL_MODE);
		val |= USBCTRL_MODE_IDDIGMUX;
		val |= USBCTRL_MODE_IDDIG;
		TIOTG_USBC_WRITE4(sc, USBCTRL_MODE, val);
	}

	aprint_normal("\n");
	aprint_naive("\n");

	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	sc->sc_motg.sc_iot = faa->faa_bst;
	if (bus_space_map(sc->sc_motg.sc_iot, addr[0], size[0], 0,
	    &sc->sc_motg.sc_ioh) != 0) {
		aprint_error_dev(self, "couldn't map mc registers\n");
		return;
	}
	sc->sc_motg.sc_size = size[0];
	sc->sc_motg.sc_intr_poll = ti_motg_poll;
	sc->sc_motg.sc_intr_poll_arg = sc;
	delay(10);
	motg_init(&sc->sc_motg);
	/* enable interrupts */
	TIOTG_USBC_WRITE4(sc, USBCTRL_INTEN_SET0, 0xffffffff);
	TIOTG_USBC_WRITE4(sc, USBCTRL_INTEN_SET1,
	    USBCTRL_INTEN_USB_ALL & ~USBCTRL_INTEN_USB_SOF);
}

static int
ti_motg_intr(void *v)
{
	struct ti_motg_softc *sc = v;
	uint32_t stat, stat0, stat1;
	int rv;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	mutex_spin_enter(&sc->sc_motg.sc_intr_lock);
	stat = TIOTG_USBC_READ4(sc, USBCTRL_STAT);
	stat0 = TIOTG_USBC_READ4(sc, USBCTRL_IRQ_STAT0);
	stat1 = TIOTG_USBC_READ4(sc, USBCTRL_IRQ_STAT1);
	DPRINTF("USB %jd 0x%jx 0x%jx stat %jd",
	    sc->sc_ctrlport, stat0, stat1, stat);

	if (stat0) {
		TIOTG_USBC_WRITE4(sc, USBCTRL_IRQ_STAT0, stat0);
	}
	if (stat1) {
		TIOTG_USBC_WRITE4(sc, USBCTRL_IRQ_STAT1, stat1);
	}
	if (stat1 & USBCTRL_IRQ_STAT1_DRVVBUS) {
		motg_intr_vbus(&sc->sc_motg, stat & 0x1);
	}
	rv = motg_intr(&sc->sc_motg, ((stat0 >> 16) & 0xffff),
		    stat0 & 0xffff, stat1 & 0xff);
	mutex_spin_exit(&sc->sc_motg.sc_intr_lock);
	return rv;
}

static void
ti_motg_poll(void *v)
{
	ti_motg_intr(v);
}
