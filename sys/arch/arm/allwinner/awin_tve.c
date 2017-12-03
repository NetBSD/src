/* $NetBSD: awin_tve.c,v 1.1.18.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Michael Lorenz
 *               2014 Jared D. McNeill <jmcneill@invisible.ca>
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

/* based on jmcneill's awin_hdmi.c */

#include "opt_ddb.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: awin_tve.c,v 1.1.18.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/kthread.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ddcvar.h>
#include <dev/i2c/ddcreg.h>
#include <dev/videomode/videomode.h>
#include <dev/videomode/edidvar.h>

#include "opt_allwinner.h"

#define AWIN_TVE_DEBUG

struct awin_tve_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	void *sc_ih;

	struct i2c_controller *sc_i2c;

	bool sc_connected;
	char sc_display_vendor[16];
	char sc_display_product[16];
	
	int   sc_tcon_unit;
	unsigned int sc_tcon_pll;

	uint32_t sc_ver;
	unsigned int sc_i2c_blklen;
};

#define TVE_READ(sc, reg)			\
    bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define TVE_WRITE(sc, reg, val)		\
    bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val));

static int	awin_tve_match(device_t, cfdata_t, void *);
static void	awin_tve_attach(device_t, device_t, void *);

static void	awin_tve_i2c_init(struct awin_tve_softc *);

static void	awin_tve_enable(struct awin_tve_softc *);
static void	awin_tve_read_edid(struct awin_tve_softc *);
static int	awin_tve_read_edid_block(struct awin_tve_softc *, uint8_t *,
					  uint8_t);
static void	awin_tve_video_enable(struct awin_tve_softc *, bool);
#if 0
static int	awin_tve_intr(void *);
#endif

#if defined(DDB)
void		awin_tve_dump_regs(void);
#endif

CFATTACH_DECL_NEW(awin_tve, sizeof(struct awin_tve_softc),
	awin_tve_match, awin_tve_attach, NULL, NULL);

static int
awin_tve_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	return 1;
}

static void
awin_tve_attach(device_t parent, device_t self, void *aux)
{
	struct awin_tve_softc *sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	prop_dictionary_t cfg = device_properties(self);
	int8_t	tcon_unit = -1;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	if (prop_dictionary_get_int8(cfg, "tcon_unit", &tcon_unit)) {
		sc->sc_tcon_unit = tcon_unit;
	} else {
		sc->sc_tcon_unit = 0; /* default value */
	}
	sc->sc_tcon_pll = awin_tcon_get_clk_pll(sc->sc_tcon_unit);
	switch (sc->sc_tcon_pll) {
	case 3:
		awin_pll3_enable();
		break;
	case 7:
		awin_pll7_enable();
		break;
	default:
		panic("awin_tve pll");
	}

	/* for now assume we're always at 0 */
	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_AHB_GATING1_REG, AWIN_AHB_GATING1_TVE0, 0);

	aprint_naive("\n");
	aprint_normal(": TV Encoder / VGA output\n");
	if (tcon_unit >= 0) {
		aprint_verbose_dev(self, ": using TCON%d, pll%d\n",
		    sc->sc_tcon_unit, sc->sc_tcon_pll);
	}

	sc->sc_i2c_blklen = 16;

#if 0
	sc->sc_ih = intr_establish(loc->loc_intr, IPL_SCHED, IST_LEVEL,
	    awin_tve_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);
#endif

	awin_tve_i2c_init(sc);

	awin_tve_enable(sc);
	awin_tve_read_edid(sc);
}

static void
awin_tve_i2c_init(struct awin_tve_softc *sc)
{
	device_t iic2;

	sc->sc_i2c = NULL;

	iic2 = device_find_by_driver_unit("awiniic", 2);

	if (iic2 == NULL)
		return;

	sc->sc_i2c = awin_twi_get_controller(iic2);
}

static void
awin_tve_enable(struct awin_tve_softc *sc)
{
	uint32_t reg;

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_TVE_CONFIG, 0x20000000);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_TVE_DAC_1, 0x403f1ac7);
	reg = bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_TVE_DAC_2);
	reg &= 0xff000000;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_TVE_DAC_2, reg);
	reg = bus_space_read_4(sc->sc_bst, sc->sc_bsh, AWIN_TVE_ENABLE);
	reg &= ~(0xfff << 4);
	reg |= (0x321 << 4);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AWIN_TVE_ENABLE, reg);
	delay(1000);
}


static int
awin_tve_read_edid_block(struct awin_tve_softc *sc, uint8_t *data,
    uint8_t block)
{
	i2c_tag_t tag = sc->sc_i2c;
	uint8_t wbuf[2];
	int error;

	if (tag == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't find awiniic2 - no DDC2 possible\n");
		return -1;
	}

	if ((error = iic_acquire_bus(tag, I2C_F_POLL)) != 0)
		return error;

	wbuf[0] = block;	/* start address */

	if ((error = iic_exec(tag, I2C_OP_READ_WITH_STOP, DDC_ADDR, wbuf, 1,
	    data, 128, I2C_F_POLL)) != 0) {
		iic_release_bus(tag, I2C_F_POLL);
		return error;
	}
	iic_release_bus(tag, I2C_F_POLL);

	return 0;
}

static void
awin_tve_read_edid(struct awin_tve_softc *sc)
{
	const struct videomode *mode = NULL;
	char edid[128];
	struct edid_info ei;
	int retry = 4;

	memset(edid, 0, sizeof(edid));
	memset(&ei, 0, sizeof(ei));

	while (--retry > 0) {
		if (!awin_tve_read_edid_block(sc, edid, 0))
			break;
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "failed to read EDID\n");
	} else {
		if (edid_parse(edid, &ei) != 0) {
			device_printf(sc->sc_dev, "failed to parse EDID\n");
		}
#ifdef AWIN_TVE_DEBUG
		else {
			edid_print(&ei);
		}
#endif
	}

	strlcpy(sc->sc_display_vendor, ei.edid_vendorname,
	    sizeof(sc->sc_display_vendor));
	strlcpy(sc->sc_display_product, ei.edid_productname,
	    sizeof(sc->sc_display_product));

	device_printf(sc->sc_dev, "VGA monitor found: [%s]\n",
	    sc->sc_display_product);

	mode = ei.edid_preferred_mode;

	if (mode == NULL)
		mode = pick_mode_by_ref(1024, 768, 60);

	if (mode != NULL) {
		awin_tve_video_enable(sc, false);
		awin_tcon1_enable(sc->sc_tcon_unit, false);
		delay(20000);

		awin_tcon1_set_videomode(sc->sc_tcon_unit, mode);
		awin_tcon1_enable(sc->sc_tcon_unit, true);
		delay(20000);
		awin_tve_video_enable(sc, true);
	}
}

static void
awin_tve_video_enable(struct awin_tve_softc *sc, bool enable)
{

}
