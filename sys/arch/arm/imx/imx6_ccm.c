/*	$NetBSD: imx6_ccm.c,v 1.7.2.2 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2010-2012, 2014  Genetec Corporation.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Clock Controller Module (CCM) for i.MX6
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx6_ccm.c,v 1.7.2.2 2017/12/03 11:35:53 jdolecek Exp $");

#include "opt_imx.h"
#include "opt_imx6clk.h"
#include "opt_cputypes.h"

#include "locators.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/sysctl.h>
#include <sys/cpufreq.h>
#include <sys/malloc.h>
#include <sys/param.h>

#include <machine/cpu.h>
#ifdef CPU_CORTEXA9
#include <arm/cortex/a9tmr_var.h>
#endif

#include <arm/imx/imx6_ccmvar.h>
#include <arm/imx/imx6_ccmreg.h>

#include <arm/imx/imx6var.h>
#include <arm/imx/imx6_reg.h>

struct imxccm_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_ioh_analog;

	/* for sysctl */
	struct sysctllog *sc_log;
	int sc_sysctlnode_pll1_arm;
	int sc_sysctlnode_pll2_sys;
	int sc_sysctlnode_pll3_usb1;
	int sc_sysctlnode_pll7_usb2;
	int sc_sysctlnode_pll4_audio;
	int sc_sysctlnode_pll5_video;
	int sc_sysctlnode_pll6_enet;
/*	int sc_sysctlnode_pll8_mlb;	*/
	int sc_sysctlnode_arm;
	int sc_sysctlnode_periph;
	int sc_sysctlnode_ahb;
	int sc_sysctlnode_ipg;
	int sc_sysctlnode_axi;
};

struct imxccm_softc *ccm_softc;

static int imxccm_match(device_t, cfdata_t, void *);
static void imxccm_attach(device_t, device_t, void *);

static int imxccm_sysctl_freq_helper(SYSCTLFN_PROTO);
static int imxccm_sysctl_setup(struct imxccm_softc *);

CFATTACH_DECL_NEW(imxccm, sizeof(struct imxccm_softc),
    imxccm_match, imxccm_attach, NULL, NULL);

static int
imxccm_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct axi_attach_args *aa = aux;

	if (ccm_softc != NULL)
		return 0;

	if (aa->aa_addr == IMX6_AIPS1_BASE + AIPS1_CCM_BASE)
		return 1;

	return 0;
}

static void
imxccm_attach(device_t parent, device_t self, void *aux)
{
	struct imxccm_softc * const sc = device_private(self);
	struct axi_attach_args *aa = aux;
	bus_space_tag_t iot = aa->aa_iot;

	ccm_softc = sc;
	sc->sc_dev = self;
	sc->sc_iot = iot;
	sc->sc_log = NULL;

	if (bus_space_map(iot, aa->aa_addr, AIPS1_CCM_SIZE, 0, &sc->sc_ioh)) {
		aprint_error(": can't map CCM registers\n");
		return;
	}

	if (bus_space_map(iot, IMX6_AIPS1_BASE + AIPS1_CCM_ANALOG_BASE,
	    AIPS1_CCM_ANALOG_SIZE, 0, &sc->sc_ioh_analog)) {
		aprint_error(": can't map CCM_ANALOG registers\n");
		bus_space_unmap(iot, sc->sc_ioh, AIPS1_CCM_SIZE);
		return;
	}

	aprint_normal(": Clock Control Module\n");
	aprint_naive("\n");

	imxccm_sysctl_setup(sc);

	aprint_verbose_dev(self, "PLL_ARM clock=%d\n",
	    imx6_get_clock(IMX6CLK_PLL1));
	aprint_verbose_dev(self, "PLL_SYS clock=%d\n",
	    imx6_get_clock(IMX6CLK_PLL2));
	aprint_verbose_dev(self, "PLL_USB1 clock=%d\n",
	    imx6_get_clock(IMX6CLK_PLL3));
	aprint_verbose_dev(self, "PLL_USB2 clock=%d\n",
	    imx6_get_clock(IMX6CLK_PLL7));
	aprint_verbose_dev(self, "PLL_AUDIO clock=%d\n",
	    imx6_get_clock(IMX6CLK_PLL4));
	aprint_verbose_dev(self, "PLL_VIDEO clock=%d\n",
	    imx6_get_clock(IMX6CLK_PLL5));
	aprint_verbose_dev(self, "PLL_ENET clock=%d\n",
	    imx6_get_clock(IMX6CLK_PLL6));
	aprint_verbose_dev(self, "PLL_MLB clock=%d\n",
	    imx6_get_clock(IMX6CLK_PLL7));

	aprint_verbose_dev(self, "IMX6CLK_PLL2_PFD0=%d\n",
	    imx6_get_clock(IMX6CLK_PLL2_PFD0));
	aprint_verbose_dev(self, "IMX6CLK_PLL2_PFD1=%d\n",
	    imx6_get_clock(IMX6CLK_PLL2_PFD1));
	aprint_verbose_dev(self, "IMX6CLK_PLL2_PFD2=%d\n",
	    imx6_get_clock(IMX6CLK_PLL2_PFD2));
	aprint_verbose_dev(self, "IMX6CLK_PLL3_PFD0=%d\n",
	    imx6_get_clock(IMX6CLK_PLL3_PFD0));
	aprint_verbose_dev(self, "IMX6CLK_PLL3_PFD1=%d\n",
	    imx6_get_clock(IMX6CLK_PLL3_PFD1));
	aprint_verbose_dev(self, "IMX6CLK_PLL3_PFD2=%d\n",
	    imx6_get_clock(IMX6CLK_PLL3_PFD2));
	aprint_verbose_dev(self, "IMX6CLK_PLL3_PFD3=%d\n",
	    imx6_get_clock(IMX6CLK_PLL3_PFD3));
	aprint_verbose_dev(self, "IMX6CLK_ARM_ROOT=%d\n",
	    imx6_get_clock(IMX6CLK_ARM_ROOT));
	aprint_verbose_dev(self, "IMX6CLK_PERIPH=%d\n",
	    imx6_get_clock(IMX6CLK_PERIPH));
	aprint_verbose_dev(self, "IMX6CLK_AHB=%d\n",
	    imx6_get_clock(IMX6CLK_AHB));
	aprint_verbose_dev(self, "IMX6CLK_IPG=%d\n",
	    imx6_get_clock(IMX6CLK_IPG));
	aprint_verbose_dev(self, "IMX6CLK_AXI=%d\n",
	    imx6_get_clock(IMX6CLK_AXI));

	aprint_verbose_dev(self, "IMX6CLK_USDHC1=%d\n",
	    imx6_get_clock(IMX6CLK_USDHC1));
	aprint_verbose_dev(self, "IMX6CLK_USDHC2=%d\n",
	    imx6_get_clock(IMX6CLK_USDHC2));
	aprint_verbose_dev(self, "IMX6CLK_USDHC3=%d\n",
	    imx6_get_clock(IMX6CLK_USDHC3));
	aprint_verbose_dev(self, "IMX6CLK_USDHC4=%d\n",
	    imx6_get_clock(IMX6CLK_USDHC4));
}

static int
imxccm_sysctl_setup(struct imxccm_softc *sc)
{
	const struct sysctlnode *node, *imxnode, *freqnode, *pllnode;
	int rv;

	rv = sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE,
	    "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);
	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &node, &imxnode,
	    0, CTLTYPE_NODE,
	    "imx6", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &imxnode, &freqnode,
	    0, CTLTYPE_NODE,
	    "frequency", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &pllnode,
	    0, CTLTYPE_NODE,
	    "pll", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "arm", SYSCTL_DESCR("frequency of ARM clock (PLL1)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_pll1_arm= node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT,
	    "system", SYSCTL_DESCR("frequency of system clock (PLL2)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_pll2_sys = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT,
	    "usb1", SYSCTL_DESCR("frequency of USB1 clock (PLL3)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_pll3_usb1 = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT,
	    "usb2", SYSCTL_DESCR("frequency of USB2 clock (PLL7)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_pll7_usb2 = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT,
	    "audio", SYSCTL_DESCR("frequency of AUDIO clock (PLL4)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_pll4_audio = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT,
	    "video", SYSCTL_DESCR("frequency of VIDEO clock (PLL5)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_pll5_video = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT,
	    "enet", SYSCTL_DESCR("frequency of ENET clock (PLL6)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_pll6_enet = node->sysctl_num;

#if 0
	rv = sysctl_createv(&sc->sc_log, 0, &pllnode, &node,
	    0, CTLTYPE_INT,
	    "mlb", SYSCTL_DESCR("frequency of MediaLinkBus clock (PLL8)"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_pll8_mlb = node->sysctl_num;
#endif

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT,
	    "arm", SYSCTL_DESCR("frequency of ARM Root clock"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_arm = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    0, CTLTYPE_INT,
	    "peripheral", SYSCTL_DESCR("current frequency of Peripheral clock"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_periph = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    0, CTLTYPE_INT,
	    "ahb", SYSCTL_DESCR("current frequency of AHB clock"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_ahb = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    0, CTLTYPE_INT,
	    "ipg", SYSCTL_DESCR("current frequency of IPG clock"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_ipg = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &freqnode, &node,
	    0, CTLTYPE_INT,
	    "axi", SYSCTL_DESCR("current frequency of AXI clock"),
	    imxccm_sysctl_freq_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);
	if (rv != 0)
		goto fail;
	sc->sc_sysctlnode_axi = node->sysctl_num;

	return 0;

 fail:
	aprint_error_dev(sc->sc_dev, "cannot initialize sysctl (err=%d)\n", rv);

	sysctl_teardown(&sc->sc_log);
	sc->sc_log = NULL;

	return -1;
}

static int
imxccm_sysctl_freq_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct imxccm_softc *sc;
	int value, ovalue, err;

	node = *rnode;
	sc = node.sysctl_data;

	/* for sysctl read */
	if (rnode->sysctl_num == sc->sc_sysctlnode_pll1_arm)
		value = imx6_get_clock(IMX6CLK_PLL1);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_pll2_sys)
		value = imx6_get_clock(IMX6CLK_PLL2);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_pll3_usb1)
		value = imx6_get_clock(IMX6CLK_PLL3);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_pll7_usb2)
		value = imx6_get_clock(IMX6CLK_PLL7);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_pll4_audio)
		value = imx6_get_clock(IMX6CLK_PLL4);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_pll5_video)
		value = imx6_get_clock(IMX6CLK_PLL5);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_pll6_enet)
		value = imx6_get_clock(IMX6CLK_PLL6);
#if 0
	else if (rnode->sysctl_num == sc->sc_sysctlnode_pll8_mlb)
		value = imx6_get_clock(IMX6CLK_PLL8);
#endif
	else if (rnode->sysctl_num == sc->sc_sysctlnode_arm)
		value = imx6_get_clock(IMX6CLK_ARM_ROOT);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_periph)
		value = imx6_get_clock(IMX6CLK_PERIPH);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_ipg)
		value = imx6_get_clock(IMX6CLK_IPG);
	else if (rnode->sysctl_num == sc->sc_sysctlnode_axi)
		value = imx6_get_clock(IMX6CLK_AXI);
	else
		return EOPNOTSUPP;

#ifdef SYSCTL_BY_MHZ
	value /= 1000 * 1000;	/* Hz -> MHz */
#endif
	ovalue = value;

	node.sysctl_data = &value;
	err = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (err != 0 || newp == NULL)
		return err;

	/* for sysctl write */
	if (value == ovalue)
		return 0;

#ifdef SYSCTL_BY_MHZ
	value *= 1000 * 1000;	/* MHz -> Hz */
#endif

	if (rnode->sysctl_num == sc->sc_sysctlnode_arm)
		return imx6_set_clock(IMX6CLK_ARM_ROOT, value);

	return 0;
}


uint32_t
imx6_ccm_read(uint32_t reg)
{
	if (ccm_softc == NULL)
		return 0;

	return bus_space_read_4(ccm_softc->sc_iot, ccm_softc->sc_ioh, reg);
}

void
imx6_ccm_write(uint32_t reg, uint32_t val)
{
	if (ccm_softc == NULL)
		return;

	bus_space_write_4(ccm_softc->sc_iot, ccm_softc->sc_ioh, reg, val);
}


uint32_t
imx6_ccm_analog_read(uint32_t reg)
{
	if (ccm_softc == NULL)
		return 0;

	return bus_space_read_4(ccm_softc->sc_iot, ccm_softc->sc_ioh_analog,
	    reg);
}

void
imx6_ccm_analog_write(uint32_t reg, uint32_t val)
{
	if (ccm_softc == NULL)
		return;

	bus_space_write_4(ccm_softc->sc_iot, ccm_softc->sc_ioh_analog, reg,
	    val);
}

int
imx6_set_clock(enum imx6_clock_id clk, uint32_t freq)
{
	uint32_t v;

	if (ccm_softc == NULL)
		return 0;

	switch (clk) {
	case IMX6CLK_ARM_ROOT:
		{
			uint32_t pll;
			int cacrr;

			for (cacrr = 7; cacrr >= 0; cacrr--) {
				pll = (uint64_t)freq * (cacrr + 1) * 2 / IMX6_OSC_FREQ;
				if (pll >= 54 && pll <= 108) {

					v = imx6_ccm_read(CCM_CACRR);
					v &= ~CCM_CACRR_ARM_PODF;
					imx6_ccm_write(CCM_CACRR,
					    v | __SHIFTIN(cacrr, CCM_CACRR_ARM_PODF));

					v = imx6_ccm_analog_read(CCM_ANALOG_PLL_ARM);
					v &= ~CCM_ANALOG_PLL_ARM_DIV_SELECT;
					imx6_ccm_analog_write(CCM_ANALOG_PLL_ARM,
					    v | __SHIFTIN(pll, CCM_ANALOG_PLL_ARM_DIV_SELECT));

					v = imx6_get_clock(IMX6CLK_ARM_ROOT);
					cpufreq_set_all(v);
#ifdef CPU_CORTEXA9
					a9tmr_update_freq(v / IMX6_PERIPHCLK_N);
#endif
					return 0;
				}
			}
			return EINVAL;
		}
		break;

	default:
		aprint_error_dev(ccm_softc->sc_dev,
		    "clock %d: not supported yet\n", clk);
		return EINVAL;
	}

	return 0;
}

uint32_t
imx6_get_clock(enum imx6_clock_id clk)
{
	uint32_t d, denom, num, sel, v;
	uint64_t freq;

	if (ccm_softc == NULL)
		return 0;

	switch (clk) {
	/* CLOCK SWITCHER */
	case IMX6CLK_PLL1:
		v = imx6_ccm_analog_read(CCM_ANALOG_PLL_ARM);
		freq = IMX6_OSC_FREQ * (v & CCM_ANALOG_PLL_ARM_DIV_SELECT) / 2;
		break;
	case IMX6CLK_PLL2:
		v = imx6_ccm_analog_read(CCM_ANALOG_PLL_SYS);
		freq = IMX6_OSC_FREQ * ((v & CCM_ANALOG_PLL_SYS_DIV_SELECT) ? 22 : 20);
		break;
	case IMX6CLK_PLL3:
		v = imx6_ccm_analog_read(CCM_ANALOG_PLL_USB1);
		freq = IMX6_OSC_FREQ * ((v & CCM_ANALOG_PLL_USB1_DIV_SELECT) ? 22 : 20);
		break;

	case IMX6CLK_PLL4:
		v = imx6_ccm_analog_read(CCM_ANALOG_PLL_AUDIO);
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_AUDIO_DIV_SELECT);
		num = imx6_ccm_analog_read(CCM_ANALOG_PLL_AUDIO_NUM);
		denom = imx6_ccm_analog_read(CCM_ANALOG_PLL_AUDIO_DENOM);
		freq = (uint64_t)IMX6_OSC_FREQ * (d + num / denom);
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_AUDIO_POST_DIV_SELECT);
		freq = freq >> (2 - d);
		break;

	case IMX6CLK_PLL5:
		v = imx6_ccm_analog_read(CCM_ANALOG_PLL_VIDEO);
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_VIDEO_DIV_SELECT);
		num = imx6_ccm_analog_read(CCM_ANALOG_PLL_VIDEO_NUM);
		denom = imx6_ccm_analog_read(CCM_ANALOG_PLL_VIDEO_DENOM);
		freq = (uint64_t)IMX6_OSC_FREQ * (d + num / denom);
		d = __SHIFTOUT(v, CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT);
		freq = freq >> (2 - d);
		break;

	case IMX6CLK_PLL6:
		/* XXX: iMX6UL has 2 div. which? */
		v = imx6_ccm_analog_read(CCM_ANALOG_PLL_ENET);
		switch (v & CCM_ANALOG_PLL_ENET_DIV_SELECT_MASK) {
		case 0:
			freq = 25 * 1000 * 1000;
			break;
		case 1:
			freq = 50 * 1000 * 1000;
			break;
		case 2:
			freq = 100 * 1000 * 1000;
			break;
		case 3:
			freq = 125 * 1000 * 1000;
			break;
		}
		break;
	case IMX6CLK_PLL7:
		v = imx6_ccm_analog_read(CCM_ANALOG_PLL_USB2);
		freq = IMX6_OSC_FREQ * ((v & CCM_ANALOG_PLL_USBn_DIV_SELECT) ? 22 : 20);
		break;

#if 0
	case IMX6CLK_PLL8:
		/* XXX notyet */
		break;
#endif

	case IMX6CLK_PLL2_PFD0:
		freq = imx6_get_clock(IMX6CLK_PLL2);
		v = imx6_ccm_analog_read(CCM_ANALOG_PFD_528);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_528_PFD0_FRAC);
		break;
	case IMX6CLK_PLL2_PFD1:
		freq = imx6_get_clock(IMX6CLK_PLL2);
		v = imx6_ccm_analog_read(CCM_ANALOG_PFD_528);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_528_PFD1_FRAC);
		break;
	case IMX6CLK_PLL2_PFD2:
		freq = imx6_get_clock(IMX6CLK_PLL2);
		v = imx6_ccm_analog_read(CCM_ANALOG_PFD_528);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_528_PFD2_FRAC);
		break;
	case IMX6CLK_PLL3_PFD3:
		freq = imx6_get_clock(IMX6CLK_PLL3);
		v = imx6_ccm_analog_read(CCM_ANALOG_PFD_480);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480_PFD3_FRAC);
		break;
	case IMX6CLK_PLL3_PFD2:
		freq = imx6_get_clock(IMX6CLK_PLL3);
		v = imx6_ccm_analog_read(CCM_ANALOG_PFD_480);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480_PFD2_FRAC);
		break;
	case IMX6CLK_PLL3_PFD1:
		freq = imx6_get_clock(IMX6CLK_PLL3);
		v = imx6_ccm_analog_read(CCM_ANALOG_PFD_480);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480_PFD1_FRAC);
		break;
	case IMX6CLK_PLL3_PFD0:
		freq = imx6_get_clock(IMX6CLK_PLL3);
		v = imx6_ccm_analog_read(CCM_ANALOG_PFD_480);
		freq = freq * 18 / __SHIFTOUT(v, CCM_ANALOG_PFD_480_PFD0_FRAC);
		break;

	/* CLOCK ROOT GEN */
	case IMX6CLK_ARM_ROOT:
		freq = imx6_get_clock(IMX6CLK_PLL1);
		v = __SHIFTOUT(imx6_ccm_read(CCM_CACRR), CCM_CACRR_ARM_PODF);
		freq = freq / (v + 1);
		break;

	case IMX6CLK_PERIPH:
		v = imx6_ccm_read(CCM_CBCDR);
		if (v & CCM_CBCDR_PERIPH_CLK_SEL) {
			v = imx6_ccm_read(CCM_CBCMR);
			sel = __SHIFTOUT(v, CCM_CBCMR_PERIPH_CLK2_SEL);
			switch (sel) {
			case 0:
				freq = imx6_get_clock(IMX6CLK_PLL3);
				break;
			case 1:
			case 2:
				freq = IMX6_OSC_FREQ;
				break;
			case 3:
				freq = 0;
				aprint_error_dev(ccm_softc->sc_dev,
				    "IMX6CLK_PERIPH: CCM_CBCMR:CCM_CBCMR_PERIPH_CLK2_SEL is set reserved value\n");
				break;
			}
		} else {
			v = imx6_ccm_read(CCM_CBCMR);
			sel = __SHIFTOUT(v, CCM_CBCMR_PRE_PERIPH_CLK_SEL);
			switch (sel) {
			case 0:
				freq = imx6_get_clock(IMX6CLK_PLL2);
				break;
			case 1:
				freq = imx6_get_clock(IMX6CLK_PLL2_PFD2);
				break;
			case 2:
				freq = imx6_get_clock(IMX6CLK_PLL2_PFD0);
				break;
			case 3:
				freq = imx6_get_clock(IMX6CLK_PLL2_PFD2) / 2;
				break;
			}
		}
		break;
	case IMX6CLK_AHB:
		freq = imx6_get_clock(IMX6CLK_PERIPH);
		v = imx6_ccm_read(CCM_CBCDR);
		freq = freq / (__SHIFTOUT(v, CCM_CBCDR_AHB_PODF) + 1);
		break;
	case IMX6CLK_IPG:
		freq = imx6_get_clock(IMX6CLK_AHB);
		v = imx6_ccm_read(CCM_CBCDR);
		freq = freq / (__SHIFTOUT(v, CCM_CBCDR_IPG_PODF) + 1);
		break;
	case IMX6CLK_AXI:
		v = imx6_ccm_read(CCM_CBCDR);
		if (v & CCM_CBCDR_AXI_SEL) {
			if (v & CCM_CBCDR_AXI_ALT_SEL) {
				freq = imx6_get_clock(IMX6CLK_PLL2_PFD2);
			} else {
				freq = imx6_get_clock(IMX6CLK_PLL3_PFD1);
			}
		} else {
			freq = imx6_get_clock(IMX6CLK_PERIPH);
			freq = freq / (__SHIFTOUT(v, CCM_CBCDR_AXI_PODF) + 1);
		}
		break;

	case IMX6CLK_USDHC1:
		v = imx6_ccm_read(CCM_CSCMR1);
		freq = imx6_get_clock((v & CCM_CSCMR1_USDHC1_CLK_SEL) ?
		    IMX6CLK_PLL2_PFD0 : IMX6CLK_PLL2_PFD2);
		v = imx6_ccm_read(CCM_CSCDR1);
		freq = freq / (__SHIFTOUT(v, CCM_CSCDR1_USDHC1_PODF) + 1);
		break;
	case IMX6CLK_USDHC2:
		v = imx6_ccm_read(CCM_CSCMR1);
		freq = imx6_get_clock((v & CCM_CSCMR1_USDHC2_CLK_SEL) ?
		    IMX6CLK_PLL2_PFD0 : IMX6CLK_PLL2_PFD2);
		v = imx6_ccm_read(CCM_CSCDR1);
		freq = freq / (__SHIFTOUT(v, CCM_CSCDR1_USDHC2_PODF) + 1);
		break;
	case IMX6CLK_USDHC3:
		v = imx6_ccm_read(CCM_CSCMR1);
		freq = imx6_get_clock((v & CCM_CSCMR1_USDHC3_CLK_SEL) ?
		    IMX6CLK_PLL2_PFD0 : IMX6CLK_PLL2_PFD2);
		v = imx6_ccm_read(CCM_CSCDR1);
		freq = freq / (__SHIFTOUT(v, CCM_CSCDR1_USDHC3_PODF) + 1);
		break;
	case IMX6CLK_USDHC4:
		v = imx6_ccm_read(CCM_CSCMR1);
		freq = imx6_get_clock((v & CCM_CSCMR1_USDHC4_CLK_SEL) ?
		    IMX6CLK_PLL2_PFD0 : IMX6CLK_PLL2_PFD2);
		v = imx6_ccm_read(CCM_CSCDR1);
		freq = freq / (__SHIFTOUT(v, CCM_CSCDR1_USDHC4_PODF) + 1);
		break;

	case IMX6CLK_PERCLK:
		freq = imx6_get_clock(IMX6CLK_IPG);
		v = imx6_ccm_read(CCM_CSCMR1);
		freq = freq / (__SHIFTOUT(v, CCM_CSCMR1_PERCLK_PODF) + 1);
		break;

	case IMX6CLK_MMDC_CH1_CLK_ROOT:
		freq = imx6_get_clock(IMX6CLK_MMDC_CH1);
		v = __SHIFTOUT(imx6_ccm_read(CCM_CBCDR), CCM_CBCDR_MMDC_CH1_AXI_PODF);
		freq = freq / (v + 1);
		break;

	case IMX6CLK_MMDC_CH0_CLK_ROOT:
		freq = imx6_get_clock(IMX6CLK_MMDC_CH0);
		v = __SHIFTOUT(imx6_ccm_read(CCM_CBCDR), CCM_CBCDR_MMDC_CH0_AXI_PODF);
		freq = freq / (v + 1);
		break;

	case IMX6CLK_MMDC_CH0:
	case IMX6CLK_MMDC_CH1:
		v = imx6_ccm_read(CCM_CBCMR);
		sel = (clk == IMX6CLK_MMDC_CH0) ?
		    __SHIFTOUT(v, CCM_CBCMR_PRE_PERIPH_CLK_SEL) :
		    __SHIFTOUT(v, CCM_CBCMR_PRE_PERIPH2_CLK_SEL);
		switch (sel) {
		case 0:
			freq = imx6_get_clock(IMX6CLK_PLL2);
			break;
		case 1:
			freq = imx6_get_clock(IMX6CLK_PLL2_PFD2);
			break;
		case 2:
			freq = imx6_get_clock(IMX6CLK_PLL2_PFD0);
			break;
		case 3:
			freq = imx6_get_clock(IMX6CLK_PLL2_PFD2) / 2;
			break;
		}
		break;

	case IMX6CLK_IPU1_HSP_CLK_ROOT:
		v = imx6_ccm_read(CCM_CSCDR3);
		switch (__SHIFTOUT(v, CCM_CSCDR3_IPU1_HSP_CLK_SEL)) {
		case 0:
			freq = imx6_get_clock(IMX6CLK_MMDC_CH0);
			break;
		case 1:
			freq = imx6_get_clock(IMX6CLK_PLL2_PFD2);
			break;
		case 2:
			freq = imx6_get_clock(IMX6CLK_PLL3) / 4;
			break;
		case 3:
			freq = imx6_get_clock(IMX6CLK_PLL3_PFD1);
			break;
		}
		v = __SHIFTOUT(v, CCM_CSCDR3_IPU1_HSP_CLK_SEL);
		freq = freq / (v + 1);
		break;
	case IMX6CLK_IPU2_HSP_CLK_ROOT:
		v = imx6_ccm_read(CCM_CSCDR3);
		switch (__SHIFTOUT(v, CCM_CSCDR3_IPU2_HSP_CLK_SEL)) {
		case 0:
			freq = imx6_get_clock(IMX6CLK_MMDC_CH0);
			break;
		case 1:
			freq = imx6_get_clock(IMX6CLK_PLL2_PFD2);
			break;
		case 2:
			freq = imx6_get_clock(IMX6CLK_PLL3) / 4;
			break;
		case 3:
			freq = imx6_get_clock(IMX6CLK_PLL3_PFD1);
			break;
		}
		v = __SHIFTOUT(v, CCM_CSCDR3_IPU2_HSP_CLK_SEL);
		freq = freq / (v + 1);
		break;

	case IMX6CLK_IPU1_DI0_CLK_ROOT:
	case IMX6CLK_IPU1_DI1_CLK_ROOT:
		v = imx6_ccm_read(CCM_CHSCCDR);
		sel = (clk == IMX6CLK_IPU1_DI0_CLK_ROOT) ?
		    __SHIFTOUT(v, CCM_CHSCCDR_IPU1_DI0_CLK_SEL) :
		    __SHIFTOUT(v, CCM_CHSCCDR_IPU1_DI1_CLK_SEL);
		switch (sel) {
		case 0:
			sel = (clk == IMX6CLK_IPU1_DI0_CLK_ROOT) ?
			    __SHIFTOUT(v, CCM_CHSCCDR_IPU1_DI0_PRE_CLK_SEL) :
			    __SHIFTOUT(v, CCM_CHSCCDR_IPU1_DI1_PRE_CLK_SEL);
			switch (sel) {
			case 0:
				freq = imx6_get_clock(IMX6CLK_MMDC_CH0);
				break;
			case 1:
				freq = imx6_get_clock(IMX6CLK_PLL3);
				break;
			case 2:
				freq = imx6_get_clock(IMX6CLK_PLL5);
				break;
			case 3:
				freq = imx6_get_clock(IMX6CLK_PLL2_PFD0);
				break;
			case 4:
				freq = imx6_get_clock(IMX6CLK_PLL2_PFD2);
				break;
			case 5:
				freq = imx6_get_clock(IMX6CLK_PLL3_PFD1);
				break;
			default:
				/* reserved */
				freq = 0;
			}
			if (clk == IMX6CLK_IPU1_DI0_CLK_ROOT)
				freq = freq / (__SHIFTOUT(v, CCM_CHSCCDR_IPU1_DI0_PODF) + 1);
			else
				freq = freq / (__SHIFTOUT(v, CCM_CHSCCDR_IPU1_DI1_PODF) + 1);
			break;
		case 1:
		case 2:
			/* IPP_DI[01]_CLK is an external clock */
			freq = 0;
			break;
		case 3:
			freq = imx6_get_clock(IMX6CLK_LDB_DI0_IPU);
			break;
		case 4:
			freq = imx6_get_clock(IMX6CLK_LDB_DI1_IPU);
			break;
		default:
			/* reserved */
			freq = 0;
		}
		break;

	case IMX6CLK_LDB_DI0_SERIAL_CLK_ROOT:
	case IMX6CLK_LDB_DI1_SERIAL_CLK_ROOT:
		v = imx6_ccm_read(CCM_CS2CDR);
		sel = (clk == IMX6CLK_LDB_DI0_SERIAL_CLK_ROOT) ?
		    __SHIFTOUT(v, CCM_CS2CDR_LDB_DI0_CLK_SEL) :
		    __SHIFTOUT(v, CCM_CS2CDR_LDB_DI1_CLK_SEL);
		switch (sel) {
		case 0:
			freq = imx6_get_clock(IMX6CLK_PLL5);
			break;
		case 1:
			freq = imx6_get_clock(IMX6CLK_PLL2_PFD0);
			break;
		case 2:
			freq = imx6_get_clock(IMX6CLK_PLL2_PFD2);
			break;
		case 3:
			freq = imx6_get_clock(IMX6CLK_MMDC_CH1);
			break;
		case 4:
			freq = imx6_get_clock(IMX6CLK_PLL3);
			break;
		case 5:
		default:
			/* reserved */
			freq = 0;
		}
		break;

	case IMX6CLK_LDB_DI0_IPU:
		freq = imx6_get_clock(IMX6CLK_LDB_DI0_SERIAL_CLK_ROOT);
		v = imx6_ccm_read(CCM_CSCMR2);
		if (!ISSET(v, CCM_CSCMR2_LDB_DI0_IPU_DIV))
			freq *= 2;
		freq /= 7;
		break;
	case IMX6CLK_LDB_DI1_IPU:
		freq = imx6_get_clock(IMX6CLK_LDB_DI1_SERIAL_CLK_ROOT);
		v = imx6_ccm_read(CCM_CSCMR2);
		if (!ISSET(v, CCM_CSCMR2_LDB_DI1_IPU_DIV))
			freq *= 2;
		freq /= 7;
		break;

	default:
		aprint_error_dev(ccm_softc->sc_dev,
		    "clock %d: not supported yet\n", clk);
		return 0;
	}

	return freq;
}

int
imx6_pll_power(uint32_t pllreg, int on, uint32_t en)
{
	uint32_t v;
	int timeout;

	switch (pllreg) {
	case CCM_ANALOG_PLL_USB1:
	case CCM_ANALOG_PLL_USB2:
		v = imx6_ccm_analog_read(pllreg);
		if (on) {
			v |= en;
			v &= ~CCM_ANALOG_PLL_USBn_BYPASS;
		} else {
			v &= ~en;
		}
		imx6_ccm_analog_write(pllreg, v);
		return 0;

	case CCM_ANALOG_PLL_ENET:
		v = imx6_ccm_analog_read(pllreg);
		if (on)
			v &= ~CCM_ANALOG_PLL_ENET_POWERDOWN;
		else
			v |= CCM_ANALOG_PLL_ENET_POWERDOWN;
		imx6_ccm_analog_write(pllreg, v);

		for (timeout = 100000; timeout > 0; timeout--) {
			if (imx6_ccm_analog_read(pllreg) &
			    CCM_ANALOG_PLL_ENET_LOCK)
				break;
		}
		if (timeout <= 0)
			break;

		v |= CCM_ANALOG_PLL_ENET_ENABLE;
		if (on) {
			v &= ~CCM_ANALOG_PLL_ENET_BYPASS;
			imx6_ccm_analog_write(pllreg, v);
			v |= en;
		} else {
			v &= ~en;
		}
		imx6_ccm_analog_write(pllreg, v);
		return 0;

	case CCM_ANALOG_PLL_ARM:
	case CCM_ANALOG_PLL_SYS:
	case CCM_ANALOG_PLL_AUDIO:
	case CCM_ANALOG_PLL_VIDEO:
	case CCM_ANALOG_PLL_MLB:
		/* notyet */
	default:
		break;
	}

	return -1;
}
