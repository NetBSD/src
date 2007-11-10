/*
 * Copyright (c) 2007 Danger Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Danger Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/hwclock.h>
#include <machine/bus.h>
#include <arm/omap/omap_tipb.h>
#include "opt_omap.h"

unsigned int omap_reset_reason;

/* Common */

/* Get frequency of clock that directly sources its parent. */

static int get_parent_freq(struct hwclock *hwc, unsigned long *freq)
{
	return hwc->parent->getfreq(hwc->parent, freq);
}


/* REF_CK */

static int ref_ck_getfreq(struct hwclock *hwc, unsigned long *freq)
{
	*freq = OMAP_CK_REF_SPEED;
	return 0;
}

static struct hwclock hwclock_ref_ck = {
	.name = "ref_ck",
	.getfreq = ref_ck_getfreq,
};


/* DPLL1 */

struct omapdpll1_softc {
        struct device           sc_dev;
        bus_space_tag_t         sc_iot;
        bus_space_handle_t      sc_ioh;
};

static struct omapdpll1_softc *dpll1_sc = NULL;

static int omapdpll_match(struct device *, struct cfdata *, void *);
static void omapdpll_attach(struct device *, struct device *, void *);

CFATTACH_DECL(omapdpll, sizeof(struct omapdpll1_softc),
    omapdpll_match, omapdpll_attach, NULL, NULL);

#define DPLL1_CTL_REG	0x0

#define PLL_MULT(dpll)		(((dpll) >> 7) & 0x1f)
#define PLL_DIV(dpll)		(((dpll) >> 5) & 0x02)
#define BYPASS_DIV(dpll)	(((dpll) >> 2) & 0x02)

static int dpll1_getfreq(struct hwclock *hwc, unsigned long *freq);
static int dpll1_setfreq(struct hwclock *hwc, unsigned long freq);

static struct hwclock hwclock_dpll1 = {
	.name = "dpll1",
	.parent = &hwclock_ref_ck,
	.getfreq = dpll1_getfreq,
	.setfreq = dpll1_setfreq,
};

static int dpll1_getfreq(struct hwclock *hwc, unsigned long *freq)
{
	unsigned long ref_ck_freq;
	unsigned dpll1_val;

	/* TODO?: bypass mode */

	dpll1_val = bus_space_read_2(dpll1_sc->sc_iot, dpll1_sc->sc_ioh,
				     DPLL1_CTL_REG);
	ref_ck_getfreq(&hwclock_ref_ck, &ref_ck_freq);
	*freq = ref_ck_freq * PLL_MULT(dpll1_val) / (PLL_DIV(dpll1_val)+1);
	return 0;
}

static int dpll1_setfreq(struct hwclock *hwc, unsigned long freq)
{
	// todo: DPLL reprogram sometimes not safe from SDRAM.
	return 0;
}

static int dpll1_freq_req(struct hwclock *hwc, unsigned long fcmin,
			  unsigned long fcmax, unsigned long *pfcmin,
			  unsigned long *pfcmax)
{
	// todo: determine what min/max frequencies can be generated from REF_CK.
	*pfcmin = fcmin;
	*pfcmax = fcmax;
	return 0;
}

static int
omapdpll_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct tipb_attach_args *tipb = aux;

	if (tipb->tipb_addr == -1)
		panic("omapdpll must have tipb bus addr specified in config.");

	if (tipb->tipb_size == 0)
		tipb->tipb_size = 256;

	return 1;
}

static void
omapdpll_attach(struct device *parent, struct device *self, void *aux)
{
	struct omapdpll1_softc *sc = (struct omapdpll1_softc*) self;
	struct tipb_attach_args *tipb = aux;
	unsigned long dpll1_freq;

	sc->sc_iot = tipb->tipb_iot;

	if (bus_space_map(sc->sc_iot, tipb->tipb_addr, tipb->tipb_size,
			  0, &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	dpll1_sc = sc;

	hwclock_register(&hwclock_dpll1);
	hwclock_getfreq("dpll1", &dpll1_freq);
	aprint_normal(": DPLL1 speed %luMHz\n", dpll1_freq/1000/1000);
}

/* MPU CLKM */

struct omapmpuclkm_softc {
        struct device           sc_dev;
        bus_space_tag_t         sc_iot;
        bus_space_handle_t      sc_ioh;
};

static struct omapmpuclkm_softc *mpuclkm_sc = NULL;

static int omapmpuclkm_match(struct device *, struct cfdata *, void *);
static void omapmpuclkm_attach(struct device *, struct device *, void *);

CFATTACH_DECL(omapmpuclkm, sizeof(struct omapmpuclkm_softc),
	      omapmpuclkm_match, omapmpuclkm_attach, NULL, NULL);

/* Register offsets from MPU CLKM. */

#define ARM_CKCTL		0x00
#define ARM_IDLECT1		0x04
#define ARM_IDLECT2		0x08
#define ARM_SYSST		0x18
#define ARM_IDLECT3		0x24

/* ARM_CKCTL fields */

#define ARM_TIMXO		12
#define TCDIV			8
#define ARMDIV			4
#define LCDDIV			2
#define ARM_PERDIV		0

/* ARM_IDLECT1 fields */

#define IDL_CLKOUT_ARM		12
#define IDLTIM_ARM		9
#define IDLAPI_ARM		8
#define IDLDPLL_ARM		7
#define IDLIF_ARM		6
#define IDLWDT_ARM		0

/* ARM_IDLECT2 fields */

#define EN_CKOUT_ARM		11
#define DMACK_REQ		8
#define EN_TIMCK		7
#define EN_APICK		6
#define EN_LCDCK		3
#define EN_PERCK		2
#define EN_XORPCK		1
#define EN_WDTCK		0

/* ARM_IDLECT3 fields */

#define IDLTC2_ARM		5
#define EN_TC2_CK		4
#define IDLTC1_ARM		3
#define EN_OCPI_CK		0

/* ARM_SYSST fields */

#define POR				5
#define EXT_RST			4
#define ARM_MCRST		3
#define ARM_WDRST		2
#define GLOB_SWRST		1

static int ckctl_getfreq(struct hwclock *hwc, unsigned long *freq);
static int ckctl_setfreq(struct hwclock *hwc, unsigned long freq);
static int idlectx_getenable(struct hwclock *hwc, unsigned long *value);
static int idlectx_setenable(struct hwclock *hwc, unsigned long value);
static int idlectx_setattr(struct hwclock *hwc, enum hwclock_arch_attr attr,
			   unsigned long value);
static void armtim_init(struct hwclock *hwc);

static struct hwclock hwclock_arm = {
	.name = "arm",
	.parent = &hwclock_dpll1,
	.getfreq = ckctl_getfreq,
	.setfreq = ckctl_setfreq,
	.machine = {
		.freq_field = ARMDIV,
	},
};

static struct hwclock hwclock_armper = {
	.name = "armper",
	.parent = &hwclock_dpll1,
	.getfreq = ckctl_getfreq,
	.setfreq = ckctl_setfreq,
	.getenable = idlectx_getenable,
	.setenable = idlectx_setenable,
	.parent_freq_req = dpll1_freq_req,
	.machine = {
		.freq_field = ARM_PERDIV,
		.enable_reg = ARM_IDLECT2,
		.enable_field = EN_PERCK,
	},
};

static struct hwclock hwclock_tc = {
	.name = "tc",
	.parent = &hwclock_dpll1,
	.getfreq = ckctl_getfreq,
	.setfreq = ckctl_setfreq,
	.machine = {
		.freq_field = TCDIV,
	},
};

static struct hwclock hwclock_lcd = {
	.name = "lcd",
	.parent = &hwclock_dpll1,
	.getfreq = ckctl_getfreq,
	.setfreq = ckctl_setfreq,
	.getenable = idlectx_getenable,
	.setenable = idlectx_setenable,
	.machine = {
		.freq_field = LCDDIV,
		.enable_reg = ARM_IDLECT2,
		.enable_field = EN_LCDCK,
	},
};

static struct hwclock hwclock_armtim = {
	.name = "armtim",
	.parent = &hwclock_ref_ck, /* .init chooses this parent */
	.getfreq = get_parent_freq,
	.getenable = idlectx_getenable,
	.setenable = idlectx_setenable,
	.setattr = idlectx_setattr,
	.init = armtim_init,
	.machine = {
		.enable_reg = ARM_IDLECT2,
		.enable_field = EN_TIMCK,
		.pm_reg = ARM_IDLECT1,
		.pm_field = IDLTIM_ARM,
	},
};

static struct hwclock hwclock_ocpi = {
	.name = "ocpi", /* aka "l3_ocpi" */
	.parent = &hwclock_tc,
	.getfreq = get_parent_freq,
	.getenable = idlectx_getenable,
	.setenable = idlectx_setenable,
	.machine = {
		.enable_reg = ARM_IDLECT3,
		.enable_field = EN_OCPI_CK,
	},
};

static struct hwclock hwclock_tc2 = {
	.name = "tc2",
	.parent = &hwclock_tc,
	.getfreq = get_parent_freq,
	.getenable = idlectx_getenable,
	.setenable = idlectx_setenable,
	.machine = {
		.enable_reg = ARM_IDLECT3,
		.enable_field = EN_TC2_CK,
	},
};

static int ckctl_getfreq(struct hwclock *hwc, unsigned long *freq)
{
	unsigned long dpll1_freq, ckctl_val;

	dpll1_getfreq(&hwclock_dpll1, &dpll1_freq);
	ckctl_val = bus_space_read_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
				     ARM_CKCTL);
	*freq = dpll1_freq / 
	    (1 << ((ckctl_val >> hwc->machine.freq_field) & 0x3));
	return 0;
}

static int ckctl_setfreq(struct hwclock *hwc, unsigned long freq)
{
	unsigned long dpll1_freq, newfreq;
	unsigned ckctl_val;
	int newdiv;
	int s;

	dpll1_getfreq(&hwclock_dpll1, &dpll1_freq);

	/* Requested speed greater than DPLL1 speed? */
	/* todo: Assume current DPLL1 frequency is a given for now. */

	if (freq > dpll1_freq)
		return EINVAL;

	for (newdiv = 3; newdiv >= 0; newdiv--) {
		newfreq = dpll1_freq >> newdiv;

		if (newfreq >= freq)
			break;
	}

	/* Sanity check, should have already ensured a dpll1 divider will
	   work. */

	if (newfreq < freq)
		return EINVAL;

	/* If new value violates a max constraint, just warn for now. */

	if (freq > hwc->freqcons_max)
		printf("Requested %s freq %lu, DPLL1 requires %lu,"
		       " violates max %lu\n",
		       hwc->name, freq, newfreq, hwc->freqcons_max);

	s = splhigh();
	ckctl_val = bus_space_read_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
				     ARM_CKCTL);
	ckctl_val &= ~(0x3 << hwc->machine.freq_field);
	ckctl_val |= newdiv << hwc->machine.freq_field;

	bus_space_write_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
			  ARM_CKCTL, ckctl_val);
	splx(s);
	return 0;
}

static int idlectx_getenable(struct hwclock *hwc, unsigned long *value)
{
	unsigned int idlectx_val;

	idlectx_val = bus_space_read_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
				       hwc->machine.enable_reg);
	*value = idlectx_val & (0x1 << hwc->machine.enable_field) ?
		HWCLOCK_ENABLED : HWCLOCK_DISABLED;
	return 0;
}

static int idlectx_setenable(struct hwclock *hwc, unsigned long value)
{
	unsigned int idlectx_val;
	int s;

	s = splhigh();
	idlectx_val = bus_space_read_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
				       hwc->machine.enable_reg);
	if (value == HWCLOCK_ENABLED)
		idlectx_val |= (0x1 << hwc->machine.enable_field);
	else
		idlectx_val &= ~(0x1 << hwc->machine.enable_field);

	bus_space_write_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
			  hwc->machine.enable_reg, idlectx_val);
	splx(s);
	return 0;
}

static int idlectx_setattr(struct hwclock *hwc, enum hwclock_arch_attr attr,
						   unsigned long value)
{
	unsigned int idlectx_val;
	int s;

	if (attr != AUTOIDLE)
		return EINVAL;

	s = splhigh();
	idlectx_val = bus_space_read_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
				       hwc->machine.pm_reg);
	if (value == HWCLOCK_ENABLED)
		idlectx_val |= (0x1 << hwc->machine.pm_field);
	else
		idlectx_val &= ~(0x1 << hwc->machine.pm_field);

	bus_space_write_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
			  hwc->machine.pm_reg, idlectx_val);
	splx(s);
	return 0;
}

static void armtim_init(struct hwclock *hwc)
{
	unsigned int ckctl_val;
	int s;

	s = splhigh();

	/*
	 * Set ARMTIM clock (MPU OS timers) to CK_REF source instead of
	 * DPLL1out clock source by clearing ARM_CKCTL[ARM_TIMXO].
	 */

	ckctl_val = bus_space_read_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
				     ARM_CKCTL);
	ckctl_val &= ~(1 << ARM_TIMXO);
	bus_space_write_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
			  ARM_CKCTL, ckctl_val);
	splx(s);
}


static int
omapmpuclkm_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct tipb_attach_args *tipb = aux;

	if (tipb->tipb_addr == -1)
		panic("omapmpuclkm must have tipb bus addr specified in config.");

	if (tipb->tipb_size == 0)
		tipb->tipb_size = 256;

	return 1;
}

static void
omapmpuclkm_attach(struct device *parent, struct device *self, void *aux)
{
	struct omapmpuclkm_softc *sc = (struct omapmpuclkm_softc*) self;
	struct tipb_attach_args *tipb = aux;
	unsigned long arm_freq, tc_freq;
	unsigned int val;
	int s;

	sc->sc_iot = tipb->tipb_iot;

	if (bus_space_map(sc->sc_iot, tipb->tipb_addr, tipb->tipb_size,
			  0, &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	mpuclkm_sc = sc;

	hwclock_register(&hwclock_arm);
	hwclock_register(&hwclock_armper);
	hwclock_register(&hwclock_tc);
	hwclock_register(&hwclock_lcd);
	hwclock_register(&hwclock_armtim);
	hwclock_register(&hwclock_ocpi);
	hwclock_register(&hwclock_tc2);

	s = splhigh();

	/*
	 * Save and clear ARM_SYSST reset reason bits.
	 */

	val = bus_space_read_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
			       ARM_SYSST);
	omap_reset_reason = val & (1 << POR | 1 << EXT_RST | 1 << ARM_MCRST |
				   1 << ARM_WDRST | 1 << GLOB_SWRST);
	val &= ~(1 << POR | 1 << EXT_RST | 1 << ARM_MCRST |
		 1 << ARM_WDRST | 1 << GLOB_SWRST);
	bus_space_write_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
			  ARM_SYSST, val);
	splx(s);

	hwclock_getfreq("arm", &arm_freq);
	hwclock_getfreq("tc", &tc_freq);
	aprint_normal(": ARM %luMHz TC %luMHz\n", arm_freq/1000/1000,
		      tc_freq/1000/1000);

	/* preset ARM_IDLECT registers for idle mode */

	bus_space_write_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
			  ARM_IDLECT1,
			  1 << IDL_CLKOUT_ARM |
			  1 << IDLDPLL_ARM |
			  1 << IDLIF_ARM |
			  1 << IDLAPI_ARM |
			  1 << IDLWDT_ARM);

	bus_space_write_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
			  ARM_IDLECT2,
			  bus_space_read_4(mpuclkm_sc->sc_iot,
					   mpuclkm_sc->sc_ioh,
					   ARM_IDLECT2)
			  | 1 << EN_CKOUT_ARM
			  | 1 << DMACK_REQ
			  | 1 << EN_TIMCK
			  | 1 << EN_APICK
			  | 1 << EN_LCDCK
			  | 1 << EN_PERCK
			  | 1 << EN_XORPCK
			  | 1 << EN_WDTCK);

	bus_space_write_4(mpuclkm_sc->sc_iot, mpuclkm_sc->sc_ioh,
			  ARM_IDLECT3,
			  bus_space_read_4(mpuclkm_sc->sc_iot,
					   mpuclkm_sc->sc_ioh,
					   ARM_IDLECT3)
			  | 1 << IDLTC2_ARM
			  | 1 << IDLTC1_ARM);
}

/* ULPD */

struct omapulpd_softc {
        struct device           sc_dev;
        bus_space_tag_t         sc_iot;
        bus_space_handle_t      sc_ioh;
};

static struct omapulpd_softc *ulpd_sc = NULL;

static int omapulpd_match(struct device *, struct cfdata *, void *);
static void omapulpd_attach(struct device *, struct device *, void *);

CFATTACH_DECL(omapulpd, sizeof(struct omapulpd_softc),
	      omapulpd_match, omapulpd_attach, NULL, NULL);


/* ULPD register offsets */

#define CLOCK_CTRL_REG			0x30
#define SOFT_REQ_REG			0x34
#define POWER_CTRL_REG			0x50
#define SLEEP_STATUS			0x58
#define SOFT_DISABLE_REQ_REG		0x68
#define COM_CLK_DIV_CTRL_SEL		0x78

/* CLOCK_CTRL_REG fields */

#define DIS_USB_PVCI_CLK		5
#define USB_MCLK_EN			4

/* SOFT_REQ_REG fields */

#define SOFT_MMC2_DPLL_REQ		13
#define SOFT_MMC_DPLL_REQ		12
#define SOFT_UART3_DPLL_REQ		11
#define SOFT_UART2_DPLL_REQ		10
#define SOFT_UART1_DPLL_REQ		9
#define SOFT_USB_OTG_DPLL_REQ		8
#define SOFT_COM_MCKO_REQ		6
#define USB_REQ_EN			4
#define SOFT_USB_REQ			3

/* POWER_CTRL_REG bits */

#define DVS_ENABLE			10
#define OSC_STOP_EN			9
#define LDO_CTRL_EN			7
#define DEEP_SLEEP_TRANSITION_EN	4
#define LOW_PWR_EN			0

/* SLEEP_STATUS fields */

#define BIG_SLEEP			1
#define DEEP_SLEEP			0

/* SOFT_DISABLE_REQ_REG bits */

#define DIS_MMC2_DPLL_REQ		11
#define DIS_MMC_DPLL_REQ		10
#define DIS_UART3_DPLL_REQ		9
#define DIS_UART2_DPLL_REQ		8
#define DIS_UART1_DPLL_REQ		7
#define DIS_USB_HOST_DPLL_REQ		6
#define DIS_COM_MCLK_REQ		0

/* COM_CLK_DIV_CTRL_SEL fields */

#define COM_ULPD_PLL_CLK_REQ	1
#define COM_SYSCLK_PLLCLK_SEL	0

static int ulpd_getenable(struct hwclock *hwc, unsigned long *value);
static int ulpd_setenable(struct hwclock *hwc, unsigned long value);
static int ulpd_get_swhw_enable(struct hwclock *hwc, unsigned long *value);
static int ulpd_set_swhw_enable(struct hwclock *hwc, unsigned long value);
static int ulpd_get_mclk_enable(struct hwclock *hwc, unsigned long *value);
static int ulpd_set_mclk_enable(struct hwclock *hwc, unsigned long value);
static int ulpd_setattr(struct hwclock *hwc, enum hwclock_arch_attr attr,
			unsigned long value);

static int config_set_mclk_src(struct hwclock *hwc,
			       enum hwclock_mclk_src src);
static int config_set_mmcsdio1_enable(struct hwclock *hwc,
				      unsigned long value);

static struct hwclock hwclock_mclk = {
	.name = "mclk",
	.getenable = ulpd_get_mclk_enable,
	.setenable = ulpd_set_mclk_enable,
	.setattr = ulpd_setattr,
	.machine = {
		.swreqenable_bit = SOFT_COM_MCKO_REQ,
		.hwreqdisable_bit = DIS_COM_MCLK_REQ,
	},
};

static struct hwclock hwclock_uart1 = {
	.name = "uart1",
	.getenable = ulpd_get_swhw_enable,
	.setenable = ulpd_set_swhw_enable,
	.setattr = ulpd_setattr,
	.machine = {
		.swreqenable_bit = SOFT_UART1_DPLL_REQ,
		.hwreqdisable_bit = DIS_UART1_DPLL_REQ,
	},
};

static struct hwclock hwclock_uart2 = {
	.name = "uart2",
	.getenable = ulpd_get_swhw_enable,
	.setenable = ulpd_set_swhw_enable,
	.setattr = ulpd_setattr,
	.machine = {
		.swreqenable_bit = SOFT_UART2_DPLL_REQ,
		.hwreqdisable_bit = DIS_UART2_DPLL_REQ,
	},
};

static struct hwclock hwclock_uart3 = {
	.name = "uart3",
	.getenable = ulpd_get_swhw_enable,
	.setenable = ulpd_set_swhw_enable,
	.setattr = ulpd_setattr,
	.machine = {
		.swreqenable_bit = SOFT_UART3_DPLL_REQ,
		.hwreqdisable_bit = DIS_UART3_DPLL_REQ,
	},
};

static struct hwclock hwclock_mmc = {
	.name = "mmc",
	.getenable = ulpd_get_swhw_enable,
	.setenable = ulpd_set_swhw_enable,
	.setattr = ulpd_setattr,
	.machine = {
		.swreqenable_bit = SOFT_MMC_DPLL_REQ,
		.hwreqdisable_bit = DIS_MMC_DPLL_REQ,
	},
};

static struct hwclock hwclock_mmc2 = {
	.name = "mmc2",
	.getenable = ulpd_get_swhw_enable,
	.setenable = ulpd_set_swhw_enable,
	.setattr = ulpd_setattr,
	.machine = {
		.swreqenable_bit = SOFT_MMC2_DPLL_REQ,
		.hwreqdisable_bit = DIS_MMC2_DPLL_REQ,
	},
};

static struct hwclock hwclock_usbh = {
	.name = "usbh",
	.getenable = ulpd_getenable,
	.setenable = ulpd_setenable,
	.machine = {
		.enable_reg = SOFT_REQ_REG,
		.enable_field = SOFT_USB_REQ,
	},
};

static struct hwclock hwclock_usbd = {
	.name = "usbd",
	.getenable = ulpd_getenable,
	.setenable = ulpd_setenable,
	.machine = {
		.enable_reg = SOFT_REQ_REG,
		.enable_field = USB_REQ_EN,
	},
};

static struct hwclock hwclock_usbotg = {
	.name = "usbotg",
	.getenable = ulpd_get_swhw_enable,
	.setenable = ulpd_set_swhw_enable,
	.setattr = ulpd_setattr,
	.machine = {
		.swreqenable_bit = SOFT_USB_OTG_DPLL_REQ,
		.hwreqdisable_bit = DIS_USB_HOST_DPLL_REQ,
	},
};

static struct hwclock hwclock_usb_mclk = {
	.name = "usb_mclk",
	.getenable = ulpd_getenable,
	.setenable = ulpd_setenable,
	.machine = {
		.enable_reg = CLOCK_CTRL_REG,
		.enable_field = USB_MCLK_EN,
	},
};

static struct hwclock hwclock_mmcsdio1 = {
	.name = "mmcsdio1",
	.setenable = config_set_mmcsdio1_enable,
};

static int ulpd_getenable(struct hwclock *hwc, unsigned long *value)
{
	unsigned int reg_val;

	reg_val = bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
				   hwc->machine.enable_reg);
	*value = reg_val & (0x1 << hwc->machine.enable_field) ?
		HWCLOCK_ENABLED : HWCLOCK_DISABLED;
	return 0;
}

static int ulpd_setenable(struct hwclock *hwc, unsigned long value)
{
	unsigned int reg_val;
	int s;

	s = splhigh();
	reg_val = bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
				   hwc->machine.enable_reg);
	if (value == HWCLOCK_ENABLED)
		reg_val |= (0x1 << hwc->machine.enable_field);
	else
		reg_val &= ~(0x1 << hwc->machine.enable_field);

	bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
			  hwc->machine.enable_reg, reg_val);
	splx(s);
	return 0;
}

static int ulpd_get_swhw_enable(struct hwclock *hwc, unsigned long *value)
{
	unsigned int reg_val;

	if (hwc->machine.flags & FLAG_HWREQ) {
		reg_val = bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
					   SOFT_DISABLE_REQ_REG);
		*value = reg_val & (0x1 << hwc->machine.hwreqdisable_bit) ?
			HWCLOCK_DISABLED : HWCLOCK_ENABLED;

	} else {
		reg_val = bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
					   SOFT_REQ_REG);
		*value = reg_val & (0x1 << hwc->machine.swreqenable_bit) ?
			HWCLOCK_ENABLED : HWCLOCK_DISABLED;
	}

	return 0;
}

static int ulpd_set_swhw_enable(struct hwclock *hwc, unsigned long value)
{
	unsigned int reg_val;
	int s;

	s = splhigh();

	if (value == HWCLOCK_ENABLED) {
		if (hwc->machine.flags & FLAG_HWREQ) {
			reg_val = bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
						   SOFT_DISABLE_REQ_REG);
			reg_val &= ~(1 << hwc->machine.hwreqdisable_bit);
			bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
					  SOFT_DISABLE_REQ_REG, reg_val);
		} else {
			reg_val = bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
						   SOFT_REQ_REG);
			reg_val |= (1 << hwc->machine.swreqenable_bit);
			bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
					  SOFT_REQ_REG, reg_val);
		}
	} else {
			reg_val = bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
						   SOFT_DISABLE_REQ_REG);
			reg_val |= (1 << hwc->machine.hwreqdisable_bit);
			bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
					  SOFT_DISABLE_REQ_REG, reg_val);
			reg_val = bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
						   SOFT_REQ_REG);
			reg_val &= ~(1 << hwc->machine.swreqenable_bit);
			bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
					  SOFT_REQ_REG, reg_val);
	}
	splx(s);
	return 0;
}

static int ulpd_get_mclk_enable(struct hwclock *hwc, unsigned long *value)
{
	if (hwc->machine.src != MCLK_SYSTEM_CLOCK)
		return EINVAL; 	// todo

	return ulpd_get_swhw_enable(hwc, value);
}

static int ulpd_set_mclk_enable(struct hwclock *hwc, unsigned long value)
{
	if (hwc->machine.src != MCLK_SYSTEM_CLOCK)
		return EINVAL; 	// todo

	return ulpd_set_swhw_enable(hwc, value);
}

static int ulpd_set_mclk_src(struct hwclock *hwc, enum hwclock_mclk_src src)
{
	unsigned int reg_val;
	int s;
	int ret = 0;

	s = splhigh();

	switch (src) {
	case MCLK_SYSTEM_CLOCK:
			reg_val = bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
						   COM_CLK_DIV_CTRL_SEL);
			/* system or undivided 48MHz on MCLK */
			reg_val |= (1 << COM_SYSCLK_PLLCLK_SEL);
			bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
					  COM_CLK_DIV_CTRL_SEL, reg_val);
			ret = 0;
			break;

	default:
		ret = EINVAL; // todo: APLL 48MHz sources
		break;
	}

	splx(s);

	return ret;
}

static int ulpd_setattr(struct hwclock *hwc, enum hwclock_arch_attr attr,
			unsigned long value)
{
	int ret = 0;

	switch (attr) {
	case HWSWREQ:
		if (value == HWREQ)
			hwc->machine.flags |= FLAG_HWREQ;
		else
			hwc->machine.flags &= ~FLAG_HWREQ;

		ret = 0;
		break;

	case SOURCE:
		if (hwc == &hwclock_mclk) {
			ret = ulpd_set_mclk_src(hwc, value) ||
				config_set_mclk_src(hwc, value);

			if (ret == 0)
				hwc->machine.src = value;

		} else {
			ret = EINVAL;
		}

		break;

	default:
		printf("ulpd_setattr: unknown attribute %d\n", (int) attr);
		ret = EINVAL;
	}

	return ret;
}


static int
omapulpd_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct tipb_attach_args *tipb = aux;


	if (tipb->tipb_addr == -1)
		panic("omapulpd must have tipb bus addr specified in config.");

	if (tipb->tipb_size == 0)
		tipb->tipb_size = 2048;

	return 1;
}

static void
omapulpd_attach(struct device *parent, struct device *self, void *aux)
{
	struct omapulpd_softc *sc = (struct omapulpd_softc*) self;
	struct tipb_attach_args *tipb = aux;

	sc->sc_iot = tipb->tipb_iot;

	if (bus_space_map(sc->sc_iot, tipb->tipb_addr, tipb->tipb_size,
			  0, &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	ulpd_sc = sc;

	hwclock_register(&hwclock_mclk);
	hwclock_register(&hwclock_uart1);
	hwclock_register(&hwclock_uart2);
	hwclock_register(&hwclock_uart3);
	hwclock_register(&hwclock_mmc);
	hwclock_register(&hwclock_mmc2);
	hwclock_register(&hwclock_usbh);
	hwclock_register(&hwclock_usbd);
	hwclock_register(&hwclock_usbotg);
	hwclock_register(&hwclock_usb_mclk);
	hwclock_register(&hwclock_mmcsdio1);

	aprint_normal(": Ultra Low Power Device\n");

	/* Enable Deep Sleep */

	bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
			  POWER_CTRL_REG,
			  (1 << DVS_ENABLE) |
			  (1 << OSC_STOP_EN) |
			  (1 << DEEP_SLEEP_TRANSITION_EN));

}

static unsigned int soft_req_save;
static unsigned int soft_disable_req_save;
static unsigned int clock_ctrl_save;
static unsigned int com_div_ctrl_save;

static void ulpd_suspend(void)
{

	/* All ULPD software requests inactive, disable any hardware
	   requests. */

	soft_req_save = bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
					 SOFT_REQ_REG);
	bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh, SOFT_REQ_REG,
			  0);
	soft_disable_req_save =
		bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
				 SOFT_DISABLE_REQ_REG);
	bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
			  SOFT_DISABLE_REQ_REG, 0xffff);

	/*
	 * Disable USB W2FC PVCI clock. (move to USB driver?)
	 */

	clock_ctrl_save =
		bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
				 CLOCK_CTRL_REG);
	bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh, CLOCK_CTRL_REG,
			  clock_ctrl_save | (1 << DIS_USB_PVCI_CLK));

	/*
	 * Inactivate the MCLK software request (move to aic28 driver?).
	 */

	com_div_ctrl_save =
		bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
						 COM_CLK_DIV_CTRL_SEL);
	bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
			  COM_CLK_DIV_CTRL_SEL,
			  com_div_ctrl_save & ~(1 << COM_ULPD_PLL_CLK_REQ));
}


static void ulpd_resume(void)
{
	unsigned int val;

	bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh, SOFT_REQ_REG,
			  soft_req_save);
	bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
			  SOFT_DISABLE_REQ_REG, soft_disable_req_save);
	bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh, CLOCK_CTRL_REG,
			  clock_ctrl_save);
	bus_space_write_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh,
			  COM_CLK_DIV_CTRL_SEL, com_div_ctrl_save);

	val = bus_space_read_2(ulpd_sc->sc_iot, ulpd_sc->sc_ioh, SLEEP_STATUS);

	printf("ULPD awake");

	if (val & BIG_SLEEP)
		printf(" from Big Sleep");

	if (val & DEEP_SLEEP)
		printf(" from Deep Sleep");


	printf("\n");
}


/* CONFIG */

struct omapconfig_softc {
        struct device           sc_dev;
        bus_space_tag_t         sc_iot;
        bus_space_handle_t      sc_ioh;
};

static struct omapconfig_softc *config_sc = NULL;

static int omapconfig_match(struct device *, struct cfdata *, void *);
static void omapconfig_attach(struct device *, struct device *, void *);

CFATTACH_DECL(omapconfig, sizeof(struct omapconfig_softc),
	      omapconfig_match, omapconfig_attach, NULL, NULL);


/* CONFIG register offsets */

#define MOD_CONF_CTRL_0		0x80
#define MOD_CONF_CTRL_1		0x110
#define RESET_CONTROL		0x140

/* MOD_CONF_CTRL_0 fields */

#define CONF_MOD_MMC_SD_CLK_REQ_R	23
#define CONF_MOD_COM_MCLK_12_48_SEL_R	12

/* MOD_CONF_CTRL_1 fields */

#define SSI_INTERCONNECT_GATE_EN_R	26
#define OCP_INTERCON_GATE_EN_R		25
#define CONF_MCBSP1_CLK_DIS		21

/* RESET_CONTROL bitmasks */

#define CONF_RNG_IDLE_MODE		0x40
#define CONF_CAMERAIF_RESET_R		0x20
#define CONF_UWIRE_RESET_R		0x10
#define CONF_OSTIMER_RESET_R		0x08
#define CONF_ARMIO_RESET_R		0x04
#define CONF_SSI_RESET_R		0x02
#define CONF_OCP_RESET_R		0x01

static int config_set_mod_conf(struct hwclock *hwc, unsigned long reg,
			       unsigned int bit, unsigned long value)
{
	unsigned int reg_val;
	int s;

	s = splhigh();
	reg_val = bus_space_read_4(config_sc->sc_iot, config_sc->sc_ioh,
				   reg);
	if (value == HWCLOCK_ENABLED)
		reg_val |= 0x1 << bit;
	else
		reg_val &= ~(0x1 << bit);

	bus_space_write_4(config_sc->sc_iot, config_sc->sc_ioh,
			  reg, reg_val);
	splx(s);
	return 0;
}

static int config_set_mclk_src(struct hwclock *hwc, enum hwclock_mclk_src src)
{
	int ret = 0;

	switch (src) {
	case MCLK_SYSTEM_CLOCK:
		ret = config_set_mod_conf(hwc, MOD_CONF_CTRL_0,
					  CONF_MOD_COM_MCLK_12_48_SEL_R,
					  HWCLOCK_DISABLED);
		break;

	default:
		ret = EINVAL; // todo: 48MHz sources
	}

	return ret;
}

static int config_set_mmcsdio1_enable(struct hwclock *hwc, unsigned long value)
{
	return config_set_mod_conf(hwc, MOD_CONF_CTRL_0,
				   CONF_MOD_MMC_SD_CLK_REQ_R, value);
}


static int
omapconfig_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct tipb_attach_args *tipb = aux;


	if (tipb->tipb_addr == -1)
		panic("omapconfig must have tipb bus addr specified in config.");

	if (tipb->tipb_size == 0)
		tipb->tipb_size = 2048;

	return 1;
}

static void
omapconfig_attach(struct device *parent, struct device *self, void *aux)
{
	struct omapconfig_softc *sc = (struct omapconfig_softc*) self;
	struct tipb_attach_args *tipb = aux;

	sc->sc_iot = tipb->tipb_iot;

	if (bus_space_map(sc->sc_iot, tipb->tipb_addr, tipb->tipb_size,
			  0, &sc->sc_ioh))
		panic("%s: Cannot map registers", self->dv_xname);

	config_sc = sc;
	aprint_normal(": Configuration module\n");

        /* enable autoidle on OCP and SSI interconnect */

	bus_space_write_4(config_sc->sc_iot, config_sc->sc_ioh,
			  MOD_CONF_CTRL_1,
			  bus_space_read_4(config_sc->sc_iot,
					   config_sc->sc_ioh,
					   MOD_CONF_CTRL_1)
			  | (1 << SSI_INTERCONNECT_GATE_EN_R)
			  | (1 << OCP_INTERCON_GATE_EN_R));
}

static unsigned int mod_conf0_save;
static unsigned int mod_conf1_save;
static unsigned int reset_control_save;

static void config_suspend(void)
{
	/*
	 * MOD_CONF_CTRL_*: Turn off various clocks.
	 */

	mod_conf0_save = bus_space_read_4(config_sc->sc_iot,
					  config_sc->sc_ioh, MOD_CONF_CTRL_0);
	bus_space_write_4(config_sc->sc_iot, config_sc->sc_ioh,
			  MOD_CONF_CTRL_0, 0);
	mod_conf1_save = bus_space_read_4(config_sc->sc_iot,
					  config_sc->sc_ioh, MOD_CONF_CTRL_1);
	bus_space_write_4(config_sc->sc_iot, config_sc->sc_ioh,
			  MOD_CONF_CTRL_1, mod_conf1_save |
			  (1 << CONF_MCBSP1_CLK_DIS));

	/*
	 * RESET_CONTROL: RNGidle disabled; camera IF, uWire, OS timer, MPUIO,
	 * SSI, and OCP functional (not in reset).
	 */

	reset_control_save = 
	    bus_space_read_4(config_sc->sc_iot, config_sc->sc_ioh,
			     RESET_CONTROL);

	bus_space_write_4(config_sc->sc_iot, config_sc->sc_ioh,
			  RESET_CONTROL,
			  CONF_CAMERAIF_RESET_R | CONF_UWIRE_RESET_R |
			  CONF_OSTIMER_RESET_R | CONF_ARMIO_RESET_R |
			  CONF_SSI_RESET_R | CONF_OCP_RESET_R);
}


static void config_resume(void)
{
	bus_space_write_4(config_sc->sc_iot, config_sc->sc_ioh,
			  RESET_CONTROL, reset_control_save);
	bus_space_write_4(config_sc->sc_iot, config_sc->sc_ioh,
			  MOD_CONF_CTRL_1, mod_conf1_save);
	bus_space_write_4(config_sc->sc_iot, config_sc->sc_ioh,
			  MOD_CONF_CTRL_0, mod_conf0_save);
}


/* Suspend/resume */

int hwclock_omap1_suspend(void)
{
	ulpd_suspend();
	config_suspend();
	return 0;
}


void hwclock_omap1_resume(void)
{
	ulpd_resume();
	config_resume();
}
