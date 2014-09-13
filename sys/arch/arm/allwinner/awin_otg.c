/* $NetBSD: awin_otg.c,v 1.2 2014/09/13 18:37:16 jmcneill Exp $ */

/*-
 * Copyright (c) 2014 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: awin_otg.c,v 1.2 2014/09/13 18:37:16 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/pool.h>
#include <sys/kmem.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_otgreg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/motgvar.h>

struct awin_otg_softc {
	struct motg_softc sc_motg;
	void *sc_ih;
	struct awin_gpio_pindata sc_drv_pin;
};

#define OTG_READ1(sc, reg) \
    bus_space_read_1((sc)->sc_motg.sc_iot, (sc)->sc_motg.sc_ioh, (reg))
#define OTG_WRITE1(sc, reg, val) \
    bus_space_write_1((sc)->sc_motg.sc_iot, (sc)->sc_motg.sc_ioh, (reg), (val))
#define OTG_READ2(sc, reg) \
    bus_space_read_2((sc)->sc_motg.sc_iot, (sc)->sc_motg.sc_ioh, (reg))
#define OTG_WRITE2(sc, reg, val) \
    bus_space_write_2((sc)->sc_motg.sc_iot, (sc)->sc_motg.sc_ioh, (reg), (val))
#define OTG_READ4(sc, reg) \
    bus_space_read_4((sc)->sc_motg.sc_iot, (sc)->sc_motg.sc_ioh, (reg))
#define OTG_WRITE4(sc, reg, val) \
    bus_space_write_4((sc)->sc_motg.sc_iot, (sc)->sc_motg.sc_ioh, (reg), (val))

static int	awin_otg_match(device_t, cfdata_t, void *);
static void	awin_otg_attach(device_t, device_t, void *);
static void	awin_otg_init(struct awin_otg_softc *);
static void	awin_otg_phy_write(struct awin_otg_softc *, u_int,
				   u_int, u_int);

static int	awin_otg_intr(void *);
static void	awin_otg_poll(void *);

CFATTACH_DECL_NEW(awin_otg, sizeof(struct awin_otg_softc),
	awin_otg_match, awin_otg_attach, NULL, NULL);

static int
awin_otg_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_otg_attach(device_t parent, device_t self, void *aux)
{
	struct awin_otg_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	aprint_naive("\n");
	aprint_normal(": OTG\n");

	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_AHB_GATING0_REG, AWIN_AHB_GATING0_USB0, 0);
	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_USB_CLK_REG,
	    AWIN_USB_CLK_USBPHY_ENABLE|AWIN_USB_CLK_PHY0_ENABLE, 0);

	awin_reg_set_clear(aio->aio_core_bst, aio->aio_core_bsh,
	    AWIN_DRAM_OFFSET + AWIN_SRAM_CTL1_REG,
	    __SHIFTOUT(AWIN_SRAM_CTL1_SRAMD_MAP_USB0,
		       AWIN_SRAM_CTL1_SRAMD_MAP),
	    0);

	sc->sc_motg.sc_dev = self;
	sc->sc_motg.sc_bus.dmatag = aio->aio_dmat;
	sc->sc_motg.sc_iot = aio->aio_core_bst;
	bus_space_subregion(sc->sc_motg.sc_iot, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_motg.sc_ioh);
	sc->sc_motg.sc_size = loc->loc_size;
	sc->sc_motg.sc_intr_poll = awin_otg_poll;
	sc->sc_motg.sc_intr_poll_arg = sc;

	sc->sc_motg.sc_mode = MOTG_MODE_HOST;
	sc->sc_motg.sc_ep_max = 5;
	sc->sc_motg.sc_ep_fifosize = 512;

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SCHED, IST_LEVEL,
	    awin_otg_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	device_printf(self, "interrupting at irq %d\n", loc->loc_intr);

	awin_otg_init(sc);

	motg_init(&sc->sc_motg);
}

static void
awin_otg_init(struct awin_otg_softc *sc)
{
	uint32_t val;

	/* initialize the USB phy */
	awin_otg_phy_write(sc, 0x0c, 0x01, 1);
	awin_otg_phy_write(sc, 0x20, 0x14, 5);
	awin_otg_phy_write(sc, 0x2a, 0x03, 2);

	if (awin_gpio_pin_reserve("usb0drv", &sc->sc_drv_pin)) {
		awin_gpio_pindata_write(&sc->sc_drv_pin, 1);
	} else {
		aprint_error_dev(sc->sc_motg.sc_dev, "no power gpio found\n");
	}

	val = OTG_READ4(sc, AWIN_USB0_PHY_CSR_REG);
	val &= ~AWIN_USB0_PHY_CSR_VBUS_CHANGE_DET;
	val &= ~AWIN_USB0_PHY_CSR_ID_CHANGE_DET;
	val &= ~AWIN_USB0_PHY_CSR_DPDM_CHANGE_DET;
	val |= AWIN_USB0_PHY_CSR_DPDM_PULLUP_EN;
	val |= AWIN_USB0_PHY_CSR_ID_PULLUP_EN;
	val &= ~AWIN_USB0_PHY_CSR_FORCE_ID;
	val |= __SHIFTIN(AWIN_USB0_PHY_CSR_FORCE_ID_LOW,
			 AWIN_USB0_PHY_CSR_FORCE_ID);
	val &= ~AWIN_USB0_PHY_CSR_FORCE_VBUS_VALID;
	val |= __SHIFTIN(AWIN_USB0_PHY_CSR_FORCE_VBUS_VALID_HIGH,
			 AWIN_USB0_PHY_CSR_FORCE_VBUS_VALID);
	OTG_WRITE4(sc, AWIN_USB0_PHY_CSR_REG, val);

	OTG_WRITE1(sc, MUSB2_REG_AWIN_VEND0, 0);
}

static int
awin_otg_intr(void *priv)
{
	struct awin_otg_softc *sc = priv;
	uint8_t intusb;
	uint16_t inttx, intrx;

	mutex_enter(&sc->sc_motg.sc_intr_lock);

	intusb = OTG_READ1(sc, MUSB2_REG_INTUSB);
	inttx = OTG_READ2(sc, MUSB2_REG_INTTX);
	intrx = OTG_READ2(sc, MUSB2_REG_INTRX);
	if (!intusb && !inttx && !intrx) {
		mutex_exit(&sc->sc_motg.sc_intr_lock);
		return 0;
	}

#if 0
	device_printf(sc->sc_motg.sc_dev, "ctrl %02x tx %04x rx %04x\n",
	    intusb, inttx, intrx);
#endif

	if (intusb)
		OTG_WRITE1(sc, MUSB2_REG_INTUSB, intusb);
	if (inttx)
		OTG_WRITE2(sc, MUSB2_REG_INTTX, inttx);
	if (intrx)
		OTG_WRITE2(sc, MUSB2_REG_INTRX, intrx);

	motg_intr(&sc->sc_motg, intrx, inttx, intusb);

	mutex_exit(&sc->sc_motg.sc_intr_lock);

	return 1;
}

static void
awin_otg_poll(void *priv)
{
	awin_otg_intr(priv);
}

static void
awin_otg_phy_write(struct awin_otg_softc *sc, u_int bit_addr, u_int bits,
    u_int len)
{
	bus_space_tag_t bst = sc->sc_motg.sc_iot;
	bus_space_handle_t bsh = sc->sc_motg.sc_ioh;
	uint32_t clk = AWIN_USB0_PHY_CTL_CLK0;
	bus_size_t reg = AWIN_USB0_PHY_CTL_REG;

	uint32_t v = bus_space_read_4(bst, bsh, reg);

	KASSERT((v & AWIN_USB0_PHY_CTL_CLK0) == 0);
	KASSERT((v & AWIN_USB0_PHY_CTL_CLK1) == 0);
	KASSERT((v & AWIN_USB0_PHY_CTL_CLK2) == 0);

	v &= ~AWIN_USB0_PHY_CTL_ADDR;
	v &= ~AWIN_USB0_PHY_CTL_DAT;

	v |= __SHIFTIN(bit_addr, AWIN_USB0_PHY_CTL_ADDR);

	/*
	 * Bitbang the data to the phy, bit by bit, incrementing bit address
	 * as we go.
	 */
	for (; len > 0; bit_addr++, bits >>= 1, len--) {
		v |= __SHIFTIN(bits & 1, AWIN_USB0_PHY_CTL_DAT);
		bus_space_write_4(bst, bsh, reg, v);
		delay(1);
		bus_space_write_4(bst, bsh, reg, v | clk);
		delay(1);
		bus_space_write_4(bst, bsh, reg, v);
		delay(1);
		v += __LOWEST_SET_BIT(AWIN_USB0_PHY_CTL_ADDR);
		v &= ~AWIN_USB0_PHY_CTL_DAT;
	}
}
