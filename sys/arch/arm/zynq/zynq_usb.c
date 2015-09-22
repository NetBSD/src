/*	$NetBSD: zynq_usb.c,v 1.1.2.3 2015/09/22 12:05:38 skrll Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zynq_usb.c,v 1.1.2.3 2015/09/22 12:05:38 skrll Exp $");

#include "opt_zynq.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ehcireg.h>
#include <dev/usb/ehcivar.h>
#include <dev/usb/ulpireg.h>

#include <arm/zynq/zynq_usbreg.h>
#include <arm/zynq/zynq_usbvar.h>

#include "locators.h"

static uint8_t ulpi_read(struct zynqehci_softc *sc, int addr);
static void ulpi_write(struct zynqehci_softc *sc, int addr, uint8_t data);
static void ulpi_reset(struct zynqehci_softc *sc);

static void zynqusb_select_interface(struct zynqehci_softc *, enum zynq_usb_if);
static void zynqusb_init(struct ehci_softc *);

/* attach structures */
CFATTACH_DECL_NEW(zynqusb, sizeof(struct zynqehci_softc),
    zynqusb_match, zynqusb_attach, NULL, NULL);

void
zynqusb_attach_common(device_t parent, device_t self, bus_space_tag_t iot,
    bus_dma_tag_t dmat, paddr_t iobase, size_t size, int intr, int flags,
    enum zynq_usb_if type, enum zynq_usb_role role)
{
	struct zynqehci_softc *sc = device_private(self);
	ehci_softc_t *hsc = &sc->sc_hsc;
	uint16_t hcirev;
	uint32_t id, hwhost, hwdevice;
	const char *comma;

	sc->sc_hsc.sc_dev = self;
	sc->sc_iot = sc->sc_hsc.iot = iot;
	sc->sc_iftype = type;
	sc->sc_role = role;

	hsc->sc_bus.ub_hcpriv = sc;
	hsc->sc_bus.ub_revision = USBREV_2_0;
	hsc->sc_flags |= EHCIF_ETTF;
	hsc->sc_vendor_init = zynqusb_init;

	aprint_normal("\n");

	if (bus_space_map(iot, iobase, size, 0, &sc->sc_ioh)) {

		aprint_error_dev(self, "unable to map device\n");
		return;
	}

	if (bus_space_subregion(iot, sc->sc_ioh,
		ZYNQUSB_EHCIREGS, ZYNQUSB_EHCI_SIZE - ZYNQUSB_EHCIREGS,
		&sc->sc_hsc.ioh)) {

		aprint_error_dev(self, "unable to map subregion\n");
		return;
	}

	id = bus_space_read_4(iot, sc->sc_ioh, ZYNQUSB_ID);
	hcirev = bus_space_read_2(iot, sc->sc_hsc.ioh, EHCI_HCIVERSION);

	aprint_normal_dev(self,
	    "Zynq USB Controller id=%d revision=%d version=%d\n",
	    (int)__SHIFTOUT(id, ZYNQUSB_ID_ID),
	    (int)__SHIFTOUT(id, ZYNQUSB_ID_REVISION),
	    (int)__SHIFTOUT(id, ZYNQUSB_ID_VERSION));
	aprint_normal_dev(self, "HCI revision=0x%x\n", hcirev);

	hwhost = bus_space_read_4(iot, sc->sc_ioh, ZYNQUSB_HWHOST);
	hwdevice = bus_space_read_4(iot, sc->sc_ioh, ZYNQUSB_HWDEVICE);

	aprint_normal_dev(self, "");

	comma = "";
	if (hwhost & HWHOST_HC) {
		int n_ports = 1 + __SHIFTOUT(hwhost, HWHOST_NPORT);
		aprint_normal("%d host port%s",
		    n_ports, n_ports > 1 ? "s" : "");
		comma = ", ";
	}

	if (hwdevice & HWDEVICE_DC) {
		int n_endpoints = __SHIFTOUT(hwdevice, HWDEVICE_DEVEP);
		aprint_normal("%sdevice capable, %d endpoint%s",
		    comma,
		    n_endpoints, n_endpoints > 1 ? "s" : "");
	}
	aprint_normal("\n");

	sc->sc_hsc.sc_bus.ub_dmatag = dmat;

	sc->sc_hsc.sc_offs = bus_space_read_1(iot, sc->sc_hsc.ioh,
	    EHCI_CAPLENGTH);

	zynqusb_reset(sc);
	zynqusb_select_interface(sc, sc->sc_iftype);

	if (sc->sc_iftype == ZYNQUSBC_IF_ULPI) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, ZYNQUSB_ULPIVIEW, 0);

		aprint_normal_dev(hsc->sc_dev,
		    "ULPI phy VID 0x%04x PID 0x%04x\n",
		    (ulpi_read(sc, ULPI_VENDOR_ID_LOW) |
			ulpi_read(sc, ULPI_VENDOR_ID_HIGH) << 8),
		    (ulpi_read(sc, ULPI_PRODUCT_ID_LOW) |
			ulpi_read(sc, ULPI_PRODUCT_ID_HIGH) << 8));

		ulpi_reset(sc);
	}

	if (sc->sc_iftype == ZYNQUSBC_IF_ULPI) {
		if (hsc->sc_bus.ub_revision == USBREV_2_0) {
			ulpi_write(sc, ULPI_FUNCTION_CONTROL + ULPI_REG_CLEAR,
			    FUNCTION_CONTROL_XCVRSELECT);
			ulpi_write(sc, ULPI_FUNCTION_CONTROL + ULPI_REG_SET,
			    FUNCTION_CONTROL_TERMSELECT);
		} else {
			ulpi_write(sc, ULPI_FUNCTION_CONTROL + ULPI_REG_SET,
			    XCVRSELECT_FSLS);
			ulpi_write(sc, ULPI_FUNCTION_CONTROL + ULPI_REG_CLEAR,
			    FUNCTION_CONTROL_TERMSELECT);
		}

		ulpi_write(sc, ULPI_OTG_CONTROL + ULPI_REG_SET,
		    OTG_CONTROL_USEEXTVBUSIND |
		    OTG_CONTROL_DRVVBUSEXT |
		    OTG_CONTROL_DRVVBUS |
		    OTG_CONTROL_CHRGVBUS);
	}

	/* Disable interrupts, so we don't get any spurious ones. */
	EOWRITE4(hsc, EHCI_USBINTR, 0);

	intr_establish(intr, IPL_USB, IST_LEVEL, ehci_intr, hsc);

	/* Figure out vendor for root hub descriptor. */
	strlcpy(hsc->sc_vendor, "Xilinx", sizeof(hsc->sc_vendor));

	int err = ehci_init(hsc);
	if (err) {
		aprint_error_dev(self, "init failed, error = %d\n", err);
		return;
	}

	/* Attach usb device. */
	hsc->sc_child = config_found(self, &hsc->sc_bus, usbctlprint);
}

static void
zynqusb_select_interface(struct zynqehci_softc *sc, enum zynq_usb_if interface)
{
	uint32_t reg;
	struct ehci_softc *hsc = &sc->sc_hsc;

	reg = EOREAD4(hsc, EHCI_PORTSC(1));
	reg &= ~(PORTSC_PTS | PORTSC_PTW);
	switch (interface) {
	case ZYNQUSBC_IF_UTMI_WIDE:
		reg |= PORTSC_PTW_16;
	case ZYNQUSBC_IF_UTMI:
		reg |= PORTSC_PTS_UTMI;
		break;
	case ZYNQUSBC_IF_PHILIPS:
		reg |= PORTSC_PTS_PHILIPS;
		break;
	case ZYNQUSBC_IF_ULPI:
		reg |= PORTSC_PTS_ULPI;
		break;
	case ZYNQUSBC_IF_SERIAL:
		reg |= PORTSC_PTS_SERIAL;
		break;
	}
	EOWRITE4(hsc, EHCI_PORTSC(1), reg);
}

static uint32_t
ulpi_wakeup(struct zynqehci_softc *sc, int tout)
{
	uint32_t ulpi_view;
	int i = 0;
	ulpi_view = bus_space_read_4(sc->sc_iot, sc->sc_ioh, ZYNQUSB_ULPIVIEW);

	if ( !(ulpi_view & ULPI_SS) ) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    ZYNQUSB_ULPIVIEW, ULPI_WU);
		for (i = 0; (tout < 0) || (i < tout); i++) {
			ulpi_view = bus_space_read_4(sc->sc_iot,
			    sc->sc_ioh, ZYNQUSB_ULPIVIEW);
			if ( !(ulpi_view & ULPI_WU) )
				break;
			delay(1);
		};
	}

	if ((tout > 0) && (i >= tout)) {
		aprint_error_dev(sc->sc_hsc.sc_dev, "%s: timeout\n", __func__);
	}

	return ulpi_view;
}

static uint32_t
ulpi_wait(struct zynqehci_softc *sc, int tout)
{
	uint32_t ulpi_view;
	int i;
	ulpi_view = bus_space_read_4(sc->sc_iot, sc->sc_ioh, ZYNQUSB_ULPIVIEW);

	for (i = 0; (tout < 0) | (i < tout); i++) {
		ulpi_view = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    ZYNQUSB_ULPIVIEW);
		if (!(ulpi_view & ULPI_RUN))
			break;
		delay(1);
	}

	if ((tout > 0) && (i >= tout)) {
		aprint_error_dev(sc->sc_hsc.sc_dev, "%s: timeout\n", __func__);
	}

	return ulpi_view;
}

#define	TIMEOUT	100000

static uint8_t
ulpi_read(struct zynqehci_softc *sc, int addr)
{
	uint32_t data;

	ulpi_wakeup(sc, TIMEOUT);

	data = ULPI_RUN | __SHIFTIN(addr, ULPI_ADDR);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ZYNQUSB_ULPIVIEW, data);

	data = ulpi_wait(sc, TIMEOUT);

	return __SHIFTOUT(data, ULPI_DATRD);
}

static void
ulpi_write(struct zynqehci_softc *sc, int addr, uint8_t data)
{
	uint32_t reg;

	ulpi_wakeup(sc, TIMEOUT);

	reg = ULPI_RUN | ULPI_RW | __SHIFTIN(addr, ULPI_ADDR) | __SHIFTIN(data, ULPI_DATWR);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ZYNQUSB_ULPIVIEW, reg);

	ulpi_wait(sc, TIMEOUT);

	return;
}

static void
ulpi_reset(struct zynqehci_softc *sc)
{
	uint8_t data;
	int timo = 1000 * 1000;	/* XXXX: 1sec */

	ulpi_write(sc, ULPI_FUNCTION_CONTROL + ULPI_REG_SET,
	    FUNCTION_CONTROL_RESET /*0x20*/);
	do {
		data = ulpi_read(sc, ULPI_FUNCTION_CONTROL);
		if (!(data & FUNCTION_CONTROL_RESET))
			break;
		delay(100);
		timo -= 100;
	} while (timo > 0);
	if (timo <= 0) {
		aprint_error_dev(sc->sc_hsc.sc_dev, "%s: reset failed!!\n",
		    __func__);
		return;
	}

	return;
}

void
zynqusb_reset(struct zynqehci_softc *sc)
{
	uint32_t reg;
	int i;
	struct ehci_softc *hsc = &sc->sc_hsc;
#define	RESET_TIMEOUT 100
	reg = EOREAD4(hsc, EHCI_USBCMD);
	reg &= ~EHCI_CMD_RS;
	EOWRITE4(hsc, EHCI_USBCMD, reg);

	for (i=0; i < RESET_TIMEOUT; ++i) {
		reg = EOREAD4(hsc, EHCI_USBCMD);
		if ((reg & EHCI_CMD_RS) == 0)
			break;
		usb_delay_ms(&hsc->sc_bus, 1);
	}

	EOWRITE4(hsc, EHCI_USBCMD, reg | EHCI_CMD_HCRESET);
	for (i = 0; i < RESET_TIMEOUT; i++) {
		reg = EOREAD4(hsc, EHCI_USBCMD);
		if ((reg &  EHCI_CMD_HCRESET) == 0)
			break;
		usb_delay_ms(&hsc->sc_bus, 1);
	}
	if (i >= RESET_TIMEOUT) {
		aprint_error_dev(hsc->sc_dev, "reset timeout (%x)\n", reg);
	}

	usb_delay_ms(&hsc->sc_bus, 100);
}

static void
zynqusb_init(struct ehci_softc *hsc)
{
	struct zynqehci_softc *sc = device_private(hsc->sc_dev);
	uint32_t reg;

	reg = EOREAD4(hsc, EHCI_PORTSC(1));
	reg &= ~(EHCI_PS_CSC | EHCI_PS_PEC | EHCI_PS_OCC);
	reg |= EHCI_PS_PP | EHCI_PS_PE;
	EOWRITE4(hsc, EHCI_PORTSC(1), reg);

	reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, ZYNQUSB_OTGSC);
	reg |= OTGSC_IDPU;
	reg |= OTGSC_DPIE | OTGSC_IDIE;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ZYNQUSB_OTGSC, reg);

	reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, ZYNQUSB_USBMODE);
	reg &= ~USBMODE_CM;
	reg |= USBMODE_CM_HOST;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, ZYNQUSB_USBMODE, reg);
}
