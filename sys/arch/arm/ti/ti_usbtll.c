/* $NetBSD: ti_usbtll.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: ti_usbtll.c,v 1.2 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/mutex.h>
#include <sys/bus.h>

#include <dev/fdt/fdtvar.h>

#include <arm/ti/ti_prcm.h>

#define	USBTLL_SYSCONFIG	0x10
#define	 USBTLL_SYSCONFIG_CLOCKACTIVITY	0x00000100
#define	 USBTLL_SYSCONFIG_SIDLEMODE	0x00000018
#define	 USBTLL_SYSCONFIG_ENAWAKEUP	0x00000004
#define	 USBTLL_SYSCONFIG_SOFTRESET	0x00000002
#define	 USBTLL_SYSCONFIG_AUTOIDLE	0x00000001

#define	USBTLL_SYSSTATUS	0x14
#define	 USBTLL_SYSSTATUS_RESETDONE	0x00000001

#define	USBTLL_SHARED_CONF	0x30
#define	 USBTLL_SHARED_CONF_USB_90D_DDR_EN	0x00000040
#define	 USBTLL_SHARED_CONF_USB_180D_SDR_EN	0x00000020
#define	 USBTLL_SHARED_CONF_USB_DIVRATIO	0x0000001c
#define	 USBTLL_SHARED_CONF_FCLK_REQ		0x00000002
#define	 USBTLL_SHARED_CONF_FCLK_IS_ON		0x00000001

#define	USBTLL_CHANNEL_CONF(i)	(0x40 + (0x04 * (i)))
#define	 USBTLL_CHANNEL_CONF_FSLSLINESTATE	0x30000000
#define	 USBTLL_CHANNEL_CONF_FSLSMODE		0x0f000000
#define	 USBTLL_CHANNEL_CONF_TESTTXSE0		0x00100000
#define	 USBTLL_CHANNEL_CONF_TESTTXDAT		0x00080000
#define	 USBTLL_CHANNEL_CONF_TESTTXEN		0x00040000
#define	 USBTLL_CHANNEL_CONF_TESTEN		0x00020000
#define	 USBTLL_CHANNEL_CONF_DRVVBUS		0x00010000
#define	 USBTLL_CHANNEL_CONF_CHRGVBUS		0x00008000
#define	 USBTLL_CHANNEL_CONF_ULPINOBITSTUFF	0x00000800
#define	 USBTLL_CHANNEL_CONF_ULPIAUTOIDLE	0x00000400
#define	 USBTLL_CHANNEL_CONF_UTMIAUTOIDLE	0x00000200
#define	 USBTLL_CHANNEL_CONF_ULPIDDRMODE	0x00000100
#define	 USBTLL_CHANNEL_CONF_ULPIOUTCLKMODE	0x00000080
#define	 USBTLL_CHANNEL_CONF_TLLFULLSPEED	0x00000040
#define	 USBTLL_CHANNEL_CONF_TLLCONNECT		0x00000020
#define	 USBTLL_CHANNEL_CONF_TLLATTACH		0x00000010
#define	 USBTLL_CHANNEL_CONF_UTMIISADEV		0x00000008
#define	 USBTLL_CHANNEL_CONF_CHANMODE		0x00000006
#define	 USBTLL_CHANNEL_CONF_CHANEN		0x00000001

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,usbhs-tll" },
	DEVICE_COMPAT_EOL
};

struct ti_usbtll_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
};

static int	ti_usbtll_match(device_t, cfdata_t, void *);
static void	ti_usbtll_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ti_usbtll, sizeof(struct ti_usbtll_softc),
    ti_usbtll_match, ti_usbtll_attach, NULL, NULL);

#define RD4(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static struct ti_usbtll_softc *ti_usbtll_sc = NULL;

void	tl_usbtll_enable_port(u_int);

void
tl_usbtll_enable_port(u_int port)
{
	struct ti_usbtll_softc *sc = ti_usbtll_sc;
	uint32_t val;

	if (sc == NULL) {
		printf("%s: driver not loaded\n", __func__);
		return;
	}

	val = RD4(sc, USBTLL_CHANNEL_CONF(port));
	val &= ~(USBTLL_CHANNEL_CONF_ULPINOBITSTUFF|
		 USBTLL_CHANNEL_CONF_ULPIAUTOIDLE|
		 USBTLL_CHANNEL_CONF_ULPIDDRMODE);
	val |= USBTLL_CHANNEL_CONF_CHANEN;
	WR4(sc, USBTLL_CHANNEL_CONF(port), val);
}

static void
ti_usbtll_reset(struct ti_usbtll_softc *sc)
{
	uint32_t val;
	int retry = 5000;

	WR4(sc, USBTLL_SYSCONFIG, USBTLL_SYSCONFIG_SOFTRESET);
	do {
		val = RD4(sc, USBTLL_SYSSTATUS);
		if (val & USBTLL_SYSSTATUS_RESETDONE)
			break;
		delay(10);
	} while (--retry > 0);
	if (retry == 0)
		aprint_error_dev(sc->sc_dev, "reset timeout\n");
}

static void
ti_usbtll_init(struct ti_usbtll_softc *sc)
{
	uint32_t val;

	ti_usbtll_reset(sc);

	val = USBTLL_SYSCONFIG_ENAWAKEUP |
	      USBTLL_SYSCONFIG_AUTOIDLE |
	      USBTLL_SYSCONFIG_SIDLEMODE |
	      USBTLL_SYSCONFIG_CLOCKACTIVITY;
	WR4(sc, USBTLL_SYSCONFIG, val);

	val = RD4(sc, USBTLL_SHARED_CONF);
	val |= (USBTLL_SHARED_CONF_FCLK_IS_ON | (1 << 2));
	val &= ~USBTLL_SHARED_CONF_USB_90D_DDR_EN;
	val &= ~USBTLL_SHARED_CONF_USB_180D_SDR_EN;
	WR4(sc, USBTLL_SHARED_CONF, val);
}

static int
ti_usbtll_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
ti_usbtll_attach(device_t parent, device_t self, void *aux)
{
	struct ti_usbtll_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (ti_prcm_enable_hwmod(phandle, 0) != 0) {
		aprint_error(": couldn't enable module\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": OMAP HS USB Host TLL\n");

	ti_usbtll_init(sc);

	if (ti_usbtll_sc == NULL)
		ti_usbtll_sc = sc;
}
