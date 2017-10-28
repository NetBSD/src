/* $NetBSD: tiotg.c,v 1.7 2017/10/28 00:37:12 pgoyette Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: tiotg.c,v 1.7 2017/10/28 00:37:12 pgoyette Exp $");

#include "opt_omap.h"
#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <arm/mainbus/mainbus.h>
#include <arm/omap/omap_var.h>

#include <arm/omap/omap2_reg.h>
#include <arm/omap/tiotgreg.h>

#ifdef TI_AM335X
#  include <arm/omap/am335x_prcm.h>
#  include <arm/omap/omap2_prcm.h>
#  include <arm/omap/sitara_cm.h>
#  include <arm/omap/sitara_cmreg.h>
#endif

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/motgreg.h>
#include <dev/usb/motgvar.h>
#include <dev/usb/usbhist.h>

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

struct tiotg_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	int			sc_intrbase;
	void *			sc_ih;
	device_t		sc_motgdev[TI_OTG_NPORTS];
};

static int	tiotg_match(device_t, cfdata_t, void *);
static void	tiotg_attach(device_t, device_t, void *);
static int	tiotg_rescan(device_t, const char *, const int *);
static void	tiotg_childdet(device_t, device_t);

static int	tiotg_intr(void *);

CFATTACH_DECL2_NEW(tiotg, sizeof(struct tiotg_softc),
    tiotg_match, tiotg_attach, NULL, NULL,
    tiotg_rescan, tiotg_childdet);

/*
 * motg device attachement and driver,
 * for the per-port part of the controller: TI-specific part, phy and
 * MI Mentor OTG.
 */

struct motg_attach_args {
	int			aa_port;
	int			aa_intr;
	bus_space_tag_t		aa_iot;
	bus_space_handle_t	aa_ioh;
	bus_dma_tag_t		aa_dmat;
};

struct ti_motg_softc {
	struct motg_softc	sc_motg;
	bus_space_tag_t		sc_ctrliot;
	bus_space_handle_t	sc_ctrlioh;
	void *			sc_ctrlih;
	int			sc_ctrlport;
};
static int	ti_motg_match(device_t, cfdata_t, void *);
static void	ti_motg_attach(device_t, device_t, void *);
static int	ti_motg_rescan(device_t, const char *, const int *);
static void	ti_motg_childdet(device_t, device_t);
static int	ti_motg_print(void *, const char *);
static int	ti_motg_intr(void *);
static void	ti_motg_poll(void *);

CFATTACH_DECL2_NEW(motg, sizeof(struct ti_motg_softc),
    ti_motg_match, ti_motg_attach, NULL, NULL,
    ti_motg_rescan, ti_motg_childdet);

static int
tiotg_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *mb = aux;

	if (mb->mb_iobase == MAINBUSCF_BASE_DEFAULT ||
	    mb->mb_iosize == MAINBUSCF_SIZE_DEFAULT ||
	    mb->mb_intrbase == MAINBUSCF_INTRBASE_DEFAULT)
		return 0;
	return 1;
}

static void
tiotg_attach(device_t parent, device_t self, void *aux)
{
	struct tiotg_softc *sc = device_private(self);
	struct mainbus_attach_args *mb = aux;
	uint32_t val;

	sc->sc_iot = &omap_bs_tag;
	sc->sc_dmat = &omap_bus_dma_tag;
	sc->sc_dev = self;

	if (bus_space_map(sc->sc_iot, mb->mb_iobase, mb->mb_iosize, 0,
	    &sc->sc_ioh)) {
		aprint_error(": couldn't map register space\n");
		return;
	}

	sc->sc_intrbase = mb->mb_intrbase;
	sc->sc_ih = intr_establish(mb->mb_intrbase, IPL_USB, IST_LEVEL,
	    tiotg_intr, sc);
	KASSERT(sc->sc_ih != NULL);
	aprint_normal(": TI dual-port USB controller");
	/* XXX this looks wrong */
	prcm_write_4(AM335X_PRCM_CM_WKUP, CM_WKUP_CM_CLKDCOLDO_DPLL_PER,
	    CM_WKUP_CM_CLKDCOLDO_DPLL_PER_CLKDCOLDO_GATE_CTRL|
	    CM_WKUP_CM_CLKDCOLDO_DPLL_PER_CLKDCOLDO_ST);

	prcm_write_4(AM335X_PRCM_CM_PER, CM_PER_USB0_CLKCTRL, 2);
	while ((prcm_read_4(AM335X_PRCM_CM_PER, CM_PER_USB0_CLKCTRL) & 0x3) != 2)
		delay(10);

	while (prcm_read_4(AM335X_PRCM_CM_PER, CM_PER_USB0_CLKCTRL) & (3<<16))
		delay(10);

	/* reset module */
	TIOTG_USBSS_WRITE4(sc, USBSS_SYSCONFIG, USBSS_SYSCONFIG_SRESET);
	while (TIOTG_USBSS_READ4(sc, USBSS_SYSCONFIG) & USBSS_SYSCONFIG_SRESET)
		delay(10);
	val = TIOTG_USBSS_READ4(sc, USBSS_REVREG);
	aprint_normal(": version v%d.%d.%d.%d",
	    (val >> 11) & 15, (val >> 8) & 7, (val >> 6) & 3, val & 63);
	aprint_normal("\n");

	/* enable clock */

	tiotg_rescan(self, NULL, NULL);

}

static int
tiotg_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct tiotg_softc *sc = device_private(self);
	struct motg_attach_args	aa;
	int i;

	for (i = 0; i < TI_OTG_NPORTS; i++) {
		if (sc->sc_motgdev[i] != NULL)
			continue;
		if (!ifattr_match(ifattr, "tiotg_port"))
			continue;
		if (bus_space_subregion(sc->sc_iot, sc->sc_ioh,
		    USB_CTRL_OFFSET(i), USB_PORT_SIZE, &aa.aa_ioh) < 0) {
			aprint_error_dev(self,
			    "can't subregion for port %d\n", i);
			continue;
		}
		aa.aa_iot = sc->sc_iot;
		aa.aa_dmat = sc->sc_dmat;
		aa.aa_port = i;
		aa.aa_intr = sc->sc_intrbase + 1 + i;
		sc->sc_motgdev[i] =
		    config_found_ia(self, "tiotg_port", &aa, ti_motg_print);
	}
	return 0;
}

static void
tiotg_childdet(device_t self, device_t child)
{
	struct tiotg_softc *sc = device_private(self);
	int i;

	for (i = 0; TI_OTG_NPORTS; i++) {
		if (child == sc->sc_motgdev[i]) {
			sc->sc_motgdev[i] = NULL;
			break;
		}
	}
}

static int
tiotg_intr(void *v)
{
	panic("tiotg_intr");
}

#ifdef TI_AM335X
struct tiotg_padconf {
	const char *padname;
	const char *padmode;
};
const struct tiotg_padconf tiotg_padconf_usb0[] = {
	{"USB0_DM", "USB0_DM"},
	{"USB0_DP", "USB0_DP"},
	{"USB0_CE", "USB0_CE"},
	{"USB0_ID", "USB0_ID"},
	{"USB0_VBUS", "USB0_VBUS"},
	{"USB0_DRVVBUS", "USB0_DRVVBUS"},
	{NULL, NULL},
};
const struct tiotg_padconf tiotg_padconf_usb1[] = {
	{"USB1_DM", "USB1_DM"},
	{"USB1_DP", "USB1_DP"},
	{"USB1_CE", "USB1_CE"},
	{"USB1_ID", "USB1_ID"},
	{"USB1_VBUS", "USB1_VBUS"},
	{"USB1_DRVVBUS", "USB1_DRVVBUS"},
	{NULL, NULL},
};
struct tiotg_port_control {
	uint32_t	scm_reg;
	const struct tiotg_padconf *padconf;
};
const struct tiotg_port_control tiotg_port_control[TI_OTG_NPORTS] = {
	{
	     .scm_reg = OMAP2SCM_USB_CTL0,
	     .padconf = tiotg_padconf_usb0,
	},
	{
	     .scm_reg = OMAP2SCM_USB_CTL1,
	     .padconf = tiotg_padconf_usb1,
	},
};
#endif

static int
ti_motg_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
ti_motg_attach(device_t parent, device_t self, void *aux)
{
	struct ti_motg_softc *sc = device_private(self);
	struct motg_attach_args *aa = aux;
	uint32_t val;
#ifdef TI_AM335X
	int i;
	const char *mode;
	u_int state;
#endif
	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	sc->sc_motg.sc_dev = self;
	sc->sc_ctrliot = aa->aa_iot;
	sc->sc_ctrlioh = aa->aa_ioh;
	sc->sc_ctrlih = intr_establish(aa->aa_intr, IPL_USB, IST_LEVEL,
	    ti_motg_intr, sc);
	sc->sc_ctrlport = aa->aa_port;
	sc->sc_motg.sc_bus.ub_dmatag = aa->aa_dmat;

	val = TIOTG_USBC_READ4(sc, USBCTRL_REV);
	aprint_normal(": 0x%x version v%d.%d.%d", val,
	    (val >> 8) & 7, (val >> 6) & 3, val & 63);

#ifdef TI_AM335X
	/* configure pads */
	for (i = 0;
	     tiotg_port_control[sc->sc_ctrlport].padconf[i].padname != NULL;
	     i++) {
		const char *padname =
		    tiotg_port_control[sc->sc_ctrlport].padconf[i].padname;
		const char *padmode =
		    tiotg_port_control[sc->sc_ctrlport].padconf[i].padmode;
		if (sitara_cm_padconf_get(padname, &mode, &state) == 0) {
			aprint_debug(": %s mode %s state %d ",
			    padname, mode, state);
		}
		if (sitara_cm_padconf_set(padname, padmode, 0) != 0) {
			aprint_error(": can't switch %s pad from %s to %s\n",
			    padname, mode, padmode);
			return;
		}
	}
	/* turn clock on */
	sitara_cm_reg_read_4(tiotg_port_control[sc->sc_ctrlport].scm_reg, &val);
	DPRINTF("power val 0x%jx", val, 0, 0, 0);
	/* Enable power */
	val &= ~(OMAP2SCM_USB_CTLx_OTGPHY_PWD | OMAP2SCM_USB_CTLx_CMPHY_PWD);
	/* enable vbus detect and session end */
	val |= (OMAP2SCM_USB_CTLx_VBUSDET | OMAP2SCM_USB_CTLx_SESSIONEND);
	sitara_cm_reg_write_4(tiotg_port_control[sc->sc_ctrlport].scm_reg, val);
	sitara_cm_reg_read_4(tiotg_port_control[sc->sc_ctrlport].scm_reg, &val);
	DPRINTF("now val 0x%jx", val, 0, 0, 0);
#endif
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
	if (bus_space_subregion(sc->sc_ctrliot, sc->sc_ctrlioh,
	    USB_CORE_OFFSET, USB_CORE_SIZE, &sc->sc_motg.sc_ioh) < 0) {
		aprint_error_dev(self, "can't subregion for core\n");
		return;
	}
	sc->sc_motg.sc_iot = sc->sc_ctrliot;
	sc->sc_motg.sc_size = USB_CORE_SIZE;
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
ti_motg_print(void *aux, const char *pnp)
{
	struct motg_attach_args *aa = aux;
	if (pnp != NULL)
		aprint_normal("motg at %s", pnp);
	printf(" port %d", aa->aa_port);
	return UNCONF;
}

static int
ti_motg_intr(void *v)
{
	struct ti_motg_softc *sc = v;
	uint32_t stat, stat0, stat1;
	int rv = 0;
	int i;

	MOTGHIST_FUNC(); MOTGHIST_CALLED();

	mutex_spin_enter(&sc->sc_motg.sc_intr_lock);
	stat = TIOTG_USBC_READ4(sc, USBCTRL_STAT);
	stat0 = TIOTG_USBC_READ4(sc, USBCTRL_IRQ_STAT0);
	stat1 = TIOTG_USBC_READ4(sc, USBCTRL_IRQ_STAT1);
	DPRINTF("USB %jd 0x%jx 0x%jx stat %jd",
	    sc->sc_ctrlport, stat0, stat1, stat);
	/* try to deal with vbus errors */
	if (stat1 & MUSB2_MASK_IVBUSERR ) {
		stat1 &= ~MUSB2_MASK_IVBUSERR;
		for (i = 0; i < 1000; i++) {
			TIOTG_USBC_WRITE4(sc, USBCTRL_IRQ_STAT1,
			    MUSB2_MASK_IVBUSERR);
			motg_intr_vbus(&sc->sc_motg, stat & 0x1);
			delay(1000);
			stat = TIOTG_USBC_READ4(sc, USBCTRL_STAT);
			if (stat & 0x1)
				break;
		}
	}
	if (stat0) {
		TIOTG_USBC_WRITE4(sc, USBCTRL_IRQ_STAT0, stat0);
	}
	if (stat1) {
		TIOTG_USBC_WRITE4(sc, USBCTRL_IRQ_STAT1, stat1);
	}
	if ((stat & 0x1) == 0) {
		mutex_spin_exit(&sc->sc_motg.sc_intr_lock);
		aprint_error_dev(sc->sc_motg.sc_dev, ": vbus error\n");
		return 1;
	}
	if (stat0 != 0 || stat1 != 0) {
		rv = motg_intr(&sc->sc_motg, ((stat0 >> 16) & 0xffff),
			    stat0 & 0xffff, stat1 & 0xff);
	}
	mutex_spin_exit(&sc->sc_motg.sc_intr_lock);
	return rv;
}

static void
ti_motg_poll(void *v)
{
	ti_motg_intr(v);
}

static int
ti_motg_rescan(device_t self, const char *ifattr, const int *locs)
{
	return 0;
}

static void
ti_motg_childdet(device_t self, device_t child)
{
	return;
}
