/*	$NetBSD: imx51_ipuv3.c,v 1.1.16.1 2014/08/10 06:53:51 tls Exp $	*/

/*
 * Copyright (c) 2011, 2012  Genetec Corporation.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: imx51_ipuv3.c,v 1.1.16.1 2014/08/10 06:53:51 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/kernel.h>			/* for cold */
#include <sys/pmf.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wscons_callbacks.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <sys/bus.h>
#include <machine/cpu.h>
#include <arm/cpufunc.h>

#include <arm/imx/imx51var.h>
#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51_ipuv3var.h>
#include <arm/imx/imx51_ipuv3reg.h>
#include <arm/imx/imx51_ccmvar.h>
#include <arm/imx/imx51_ccmreg.h>

#include "imxccm.h"	/* if CCM driver is configured into the kernel */
#include "wsdisplay.h"
#include "opt_imx51_ipuv3.h"

/*
 * Console variables. These are necessary since console is setup very early,
 * before devices get attached.
 */
struct {
	int is_console;
	struct imx51_wsscreen_descr *descr;
	struct wsdisplay_accessops *accessops;
	const struct lcd_panel_geometry *geom;
} imx51_ipuv3_console;

#define	IPUV3_READ(ipuv3, module, reg)					      \
	bus_space_read_4((ipuv3)->iot, (ipuv3)->module##_ioh, (reg))
#define	IPUV3_WRITE(ipuv3, module, reg, val)				      \
	bus_space_write_4((ipuv3)->iot, (ipuv3)->module##_ioh, (reg), (val))

#ifdef IPUV3_DEBUG
int ipuv3_debug = IPUV3_DEBUG;
#define	DPRINTFN(n,x)   if (ipuv3_debug>(n)) printf x; else
#else
#define	DPRINTFN(n,x)
#endif

int ipuv3intr(void *);

static void imx51_ipuv3_initialize(struct imx51_ipuv3_softc *,
    const struct lcd_panel_geometry *);
#if NWSDISPLAY > 0
static void imx51_ipuv3_setup_rasops(struct imx51_ipuv3_softc *,
    struct rasops_info *, struct imx51_wsscreen_descr *,
    const struct lcd_panel_geometry *);
#endif
static void imx51_ipuv3_set_idma_param(uint32_t *, uint32_t, uint32_t);

static bool imx51_ipuv3_resume(device_t, const pmf_qual_t *);
static bool imx51_ipuv3_suspend(device_t, const pmf_qual_t *);

#ifdef IPUV3_DEBUG
static void
imx51_ipuv3_dump(struct imx51_ipuv3_softc *sc)
{
	int i;

	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

#define	__DUMP(grp, reg)						\
	DPRINTFN(4, ("%-16s = 0x%08X\n", #reg, IPUV3_READ(sc, grp, IPU_##reg)))

	__DUMP(cm, CM_CONF);
	__DUMP(cm, CM_DISP_GEN);
	__DUMP(idmac, IDMAC_CONF);
	__DUMP(idmac, IDMAC_CH_EN_1);
	__DUMP(idmac, IDMAC_CH_EN_2);
	__DUMP(idmac, IDMAC_CH_PRI_1);
	__DUMP(idmac, IDMAC_CH_PRI_2);
	__DUMP(idmac, IDMAC_BNDM_EN_1);
	__DUMP(idmac, IDMAC_BNDM_EN_2);
	__DUMP(cm, CM_CH_DB_MODE_SEL_0);
	__DUMP(cm, CM_CH_DB_MODE_SEL_1);
	__DUMP(dmfc, DMFC_WR_CHAN);
	__DUMP(dmfc, DMFC_WR_CHAN_DEF);
	__DUMP(dmfc, DMFC_DP_CHAN);
	__DUMP(dmfc, DMFC_DP_CHAN_DEF);
	__DUMP(dmfc, DMFC_IC_CTRL);
	__DUMP(cm, CM_FS_PROC_FLOW1);
	__DUMP(cm, CM_FS_PROC_FLOW2);
	__DUMP(cm, CM_FS_PROC_FLOW3);
	__DUMP(cm, CM_FS_DISP_FLOW1);
	__DUMP(dc, DC_DISP_CONF1_0);
	__DUMP(dc, DC_DISP_CONF2_0);
	__DUMP(dc, DC_WR_CH_CONF_5);

	printf("*** IPU ***\n");
	for (i = 0; i <= 0x17c; i += 4)
		DPRINTFN(6, ("0x%08X = 0x%08X\n", i, IPUV3_READ(sc, cm, i)));
	printf("*** IDMAC ***\n");
	for (i = 0; i <= 0x104; i += 4)
		DPRINTFN(6, ("0x%08X = 0x%08X\n", i, IPUV3_READ(sc, idmac, i)));
	printf("*** CPMEM ***\n");
	for (i = 0x5c0; i <= 0x600; i += 4)
		DPRINTFN(6, ("0x%08X = 0x%08X\n", i, IPUV3_READ(sc, cpmem, i)));

#undef __DUMP

}
#endif

static void
imx51_ipuv3_enable_display(struct imx51_ipuv3_softc *sc)
{
	uint32_t reg = 0;

	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	/* enable sub modules */
	reg = IPUV3_READ(sc, cm, IPU_CM_CONF);
	reg |= CM_CONF_DP_EN |
	    CM_CONF_DC_EN |
	    CM_CONF_DMFC_EN |
	    CM_CONF_DI0_EN;
	IPUV3_WRITE(sc, cm, IPU_CM_CONF, reg);
}

static void
imx51_ipuv3_dmfc_init(struct imx51_ipuv3_softc *sc)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	/* IC channel is disabled */
	IPUV3_WRITE(sc, dmfc, IPU_DMFC_IC_CTRL,
	    IC_IN_PORT_DISABLE);

	IPUV3_WRITE(sc, dmfc, IPU_DMFC_WR_CHAN, 0x00000000);
	IPUV3_WRITE(sc, dmfc, IPU_DMFC_WR_CHAN_DEF, 0x20202020);
	IPUV3_WRITE(sc, dmfc, IPU_DMFC_DP_CHAN, 0x00000094);
	IPUV3_WRITE(sc, dmfc, IPU_DMFC_DP_CHAN_DEF, 0x202020F6);

	IPUV3_WRITE(sc, dmfc, IPU_DMFC_GENERAL1,
	    DCDP_SYNC_PR_ROUNDROBIN);

#ifdef IPUV3_DEBUG
	int i;
	printf("*** DMFC ***\n");
	for (i = 0; i <= 0x34; i += 4)
		printf("0x%08X = 0x%08X\n", i, IPUV3_READ(sc, dmfc, i));

	printf("%s: DMFC_IC_CTRL         0x%08X\n", __func__,
	    IPUV3_READ(sc, dmfc, IPU_DMFC_IC_CTRL));
	printf("%s: IPU_DMFC_WR_CHAN     0x%08X\n", __func__,
	    IPUV3_READ(sc, dmfc, IPU_DMFC_WR_CHAN));
	printf("%s: IPU_DMFC_WR_CHAN_DEF 0x%08X\n", __func__,
	    IPUV3_READ(sc, dmfc, IPU_DMFC_WR_CHAN_DEF));
	printf("%s: IPU_DMFC_GENERAL1    0x%08X\n", __func__,
	    IPUV3_READ(sc, dmfc, IPU_DMFC_GENERAL1));
#endif
}

static void
imx51_ipuv3_dc_map_clear(struct imx51_ipuv3_softc *sc, int map)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	uint32_t reg;
	uint32_t addr;

	addr = IPU_DC_MAP_CONF_PNTR(map / 2);
	reg = IPUV3_READ(sc, dc, addr);
	reg &= ~(0xFFFF << (16 * (map & 0x1)));
	IPUV3_WRITE(sc, dc, addr, reg);
}

static void
imx51_ipuv3_dc_map_conf(struct imx51_ipuv3_softc *sc,
    int map, int byte, int offset, uint8_t mask)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	uint32_t reg;
	uint32_t addr;

	addr = IPU_DC_MAP_CONF_MASK((map * 3 + byte) / 2);
	reg = IPUV3_READ(sc, dc, addr);
	reg &= ~(0xFFFF << (16 * ((map * 3 + byte) & 0x1)));
	reg |= ((offset << 8) | mask) << (16 * ((map * 3 + byte) & 0x1));
	IPUV3_WRITE(sc, dc, addr, reg);
#ifdef IPUV3_DEBUG
	printf("%s: addr 0x%08X reg 0x%08X\n", __func__, addr, reg);
#endif

	addr = IPU_DC_MAP_CONF_PNTR(map / 2);
	reg = IPUV3_READ(sc, dc, addr);
	reg &= ~(0x1F << ((16 * (map & 0x1)) + (5 * byte)));
	reg |= ((map * 3) + byte) << ((16 * (map & 0x1)) + (5 * byte));
	IPUV3_WRITE(sc, dc, addr, reg);
#ifdef IPUV3_DEBUG
	printf("%s: addr 0x%08X reg 0x%08X\n", __func__, addr, reg);
#endif
}

static void
imx51_ipuv3_dc_template_command(struct imx51_ipuv3_softc *sc,
    int index, int sync, int gluelogic, int waveform, int mapping,
    int operand, int opecode, int stop)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	uint32_t reg;

	reg = (sync << 0) |
	    (gluelogic << 4) |
	    (waveform << 11) |
	    (mapping << 15) |
	    (operand << 20);
	IPUV3_WRITE(sc, dctmpl, index * 8, reg);
#ifdef IPUV3_DEBUG
	printf("%s: addr 0x%08X reg 0x%08X\n", __func__, index * 8, reg);
#endif
	reg = (opecode << 0) |
	    (stop << 9);
	IPUV3_WRITE(sc, dctmpl, index * 8 + 4, reg);
#ifdef IPUV3_DEBUG
	printf("%s: addr 0x%08X reg 0x%08X\n", __func__, index * 8 + 4, reg);
#endif
}

static void
imx51_ipuv3_set_routine_link(struct imx51_ipuv3_softc *sc,
    int base, int evt, int addr, int pri)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	uint32_t reg;

	reg = IPUV3_READ(sc, dc, IPU_DC_RL(base, evt));
	reg &= ~(0xFFFF << (16 * (evt & 0x1)));
	reg |= ((addr << 8) | pri) << (16 * (evt & 0x1));
	IPUV3_WRITE(sc, dc, IPU_DC_RL(base, evt), reg);
#ifdef IPUV3_DEBUG
	printf("%s: event %d addr %d priority %d\n", __func__,
	    evt, addr, pri);
	printf("%s: %p = 0x%08X\n", __func__,
	    (void *)IPU_DC_RL(base, evt), reg);
#endif
}

static void
imx51_ipuv3_dc_init(struct imx51_ipuv3_softc *sc)
{
	uint32_t reg;

	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	imx51_ipuv3_dc_map_clear(sc, 0);
	imx51_ipuv3_dc_map_conf(sc, 0, 0,  7, 0xff);
	imx51_ipuv3_dc_map_conf(sc, 0, 1, 15, 0xff);
	imx51_ipuv3_dc_map_conf(sc, 0, 2, 23, 0xff);
	imx51_ipuv3_dc_map_clear(sc, 1);
	imx51_ipuv3_dc_map_conf(sc, 1, 0,  5, 0xfc);
	imx51_ipuv3_dc_map_conf(sc, 1, 1, 11, 0xfc);
	imx51_ipuv3_dc_map_conf(sc, 1, 2, 17, 0xfc);
	imx51_ipuv3_dc_map_clear(sc, 2);
	imx51_ipuv3_dc_map_conf(sc, 2, 0, 15, 0xff);
	imx51_ipuv3_dc_map_conf(sc, 2, 1, 23, 0xff);
	imx51_ipuv3_dc_map_conf(sc, 2, 2,  7, 0xff);
	imx51_ipuv3_dc_map_clear(sc, 3);
	imx51_ipuv3_dc_map_conf(sc, 3, 0,  4, 0xf8);
	imx51_ipuv3_dc_map_conf(sc, 3, 1, 10, 0xfc);
	imx51_ipuv3_dc_map_conf(sc, 3, 2, 15, 0xf8);
	imx51_ipuv3_dc_map_clear(sc, 4);
	imx51_ipuv3_dc_map_conf(sc, 4, 0,  5, 0xfc);
	imx51_ipuv3_dc_map_conf(sc, 4, 1, 13, 0xfc);
	imx51_ipuv3_dc_map_conf(sc, 4, 2, 21, 0xfc);

	/* microcode */
	imx51_ipuv3_dc_template_command(sc,
	    5, 5, 8, 1, 5, 0, 0x180, 1);
	imx51_ipuv3_dc_template_command(sc,
	    6, 5, 4, 1, 5, 0, 0x180, 1);
	imx51_ipuv3_dc_template_command(sc,
	    7, 5, 0, 1, 5, 0, 0x180, 1);

	reg = (4 << 5) | 0x2;
	IPUV3_WRITE(sc, dc, IPU_DC_WR_CH_CONF_5, reg);
	IPUV3_WRITE(sc, dc, IPU_DC_WR_CH_CONF_1, 0x4);
	IPUV3_WRITE(sc, dc, IPU_DC_WR_CH_ADDR_5, 0x0);
	IPUV3_WRITE(sc, dc, IPU_DC_GEN, 0x44);

	imx51_ipuv3_set_routine_link(sc, DC_RL_CH_5, DC_RL_EVT_NL, 5, 3);
	imx51_ipuv3_set_routine_link(sc, DC_RL_CH_5, DC_RL_EVT_EOL, 6, 2);
	imx51_ipuv3_set_routine_link(sc, DC_RL_CH_5, DC_RL_EVT_NEW_DATA, 7, 1);
	imx51_ipuv3_set_routine_link(sc, DC_RL_CH_5, DC_RL_EVT_NF, 0, 0);
	imx51_ipuv3_set_routine_link(sc, DC_RL_CH_5, DC_RL_EVT_NFIELD, 0, 0);
	imx51_ipuv3_set_routine_link(sc, DC_RL_CH_5, DC_RL_EVT_EOF, 0, 0);
	imx51_ipuv3_set_routine_link(sc, DC_RL_CH_5, DC_RL_EVT_EOFIELD, 0, 0);
	imx51_ipuv3_set_routine_link(sc, DC_RL_CH_5, DC_RL_EVT_NEW_CHAN, 0, 0);
	imx51_ipuv3_set_routine_link(sc, DC_RL_CH_5, DC_RL_EVT_NEW_ADDR, 0, 0);

#ifdef IPUV3_DEBUG
	int i;
	printf("*** DC ***\n");
	for (i = 0; i <= 0x1C8; i += 4)
		printf("0x%08X = 0x%08X\n", i, IPUV3_READ(sc, dc, i));
	printf("*** DCTEMPL ***\n");
	for (i = 0; i <= 0x100; i += 4)
		printf("0x%08X = 0x%08X\n", i, IPUV3_READ(sc, dctmpl, i));
#endif
}

static void
imx51_ipuv3_dc_display_config(struct imx51_ipuv3_softc *sc, int width)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	IPUV3_WRITE(sc, dc, IPU_DC_DISP_CONF2_0, width);
}

static void
imx51_ipuv3_di_sync_conf(struct imx51_ipuv3_softc *sc, int no,
    uint32_t reg_gen0, uint32_t reg_gen1, uint32_t repeat)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	uint32_t reg;

	IPUV3_WRITE(sc, di0, IPU_DI_SW_GEN0(no), reg_gen0);
	IPUV3_WRITE(sc, di0, IPU_DI_SW_GEN1(no), reg_gen1);
	reg = IPUV3_READ(sc, di0, IPU_DI_STP_REP(no));
	reg &= ~DI_STP_REP_MASK(no);
	reg |= repeat << DI_STP_REP_SHIFT(no);
	IPUV3_WRITE(sc, di0, IPU_DI_STP_REP(no), reg);

#ifdef IPUV3_DEBUG
	printf("%s: no %d\n", __func__, no);
	printf("%s: addr 0x%08X reg_gen0   0x%08X\n", __func__,
	    IPU_DI_SW_GEN0(no), reg_gen0);
	printf("%s: addr 0x%08X reg_gen1   0x%08X\n", __func__,
	    IPU_DI_SW_GEN1(no), reg_gen1);
	printf("%s: addr 0x%08X DI_STP_REP 0x%08X\n", __func__,
	    IPU_DI_STP_REP(no), reg);
#endif
}

static void
imx51_ipuv3_di_init(struct imx51_ipuv3_softc *sc)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	uint32_t reg;
	uint32_t div;
	u_int ipuclk;
	const struct lcd_panel_geometry *geom = sc->geometry;

#if NIMXCCM > 0
	ipuclk = imx51_get_clock(IMX51CLK_IPU_HSP_CLK_ROOT);
#elif !defined(IMX51_IPU_HSP_CLOCK)
#error	IMX51_CPU_HSP_CLOCK need to be defined.
#else
	ipuclk = IMX51_IPU_HSP_CLOCK;
#endif
	DPRINTFN(3, ("IPU HSP clock = %d\n", ipuclk));
	div = (ipuclk * 16) / geom->pixel_clk;
	div = div < 16 ? 16 : div & 0xff8;

	/* DI counter */
	reg = IPUV3_READ(sc, cm, IPU_CM_DISP_GEN);
	reg &= ~(CM_DISP_GEN_DI1_COUNTER_RELEASE |
	    CM_DISP_GEN_DI0_COUNTER_RELEASE);
	IPUV3_WRITE(sc, cm, IPU_CM_DISP_GEN, reg);

	IPUV3_WRITE(sc, di0, IPU_DI_BS_CLKGEN0, div);
	IPUV3_WRITE(sc, di0, IPU_DI_BS_CLKGEN1,
	    (div / 16) << DI_BS_CLKGEN1_DOWN_SHIFT);
#ifdef IPUV3_DEBUG
	printf("%s: IPU_DI_BS_CLKGEN0 = 0x%08X\n", __func__,
	    IPUV3_READ(sc, di0, IPU_DI_BS_CLKGEN0));
	printf("%s: IPU_DI_BS_CLKGEN1 = 0x%08X\n", __func__,
	    IPUV3_READ(sc, di0, IPU_DI_BS_CLKGEN1));
#endif
	/* Display Time settings */
	reg = ((div / 16 - 1) << DI_DW_GEN_ACCESS_SIZE_SHIFT) |
	    ((div / 16 - 1) << DI_DW_GEN_COMPONNENT_SIZE_SHIFT) |
	    (3 << DI_DW_GEN_PIN_SHIFT(15));
	IPUV3_WRITE(sc, di0, IPU_DI_DW_GEN(0), reg);
#ifdef IPUV3_DEBUG
	printf("%s: div = %d\n", __func__, div);
	printf("%s: IPU_DI_DW_GEN(0) 0x%08X = 0x%08X\n", __func__,
	    IPU_DI_DW_GEN(0), reg);
#endif

	/* Up & Down Data Wave Set */
	reg = (div / 16 * 2) << DI_DW_SET_DOWN_SHIFT;
	IPUV3_WRITE(sc, di0, IPU_DI_DW_SET(0, 3), reg);
#ifdef IPUV3_DEBUG
	printf("%s: IPU_DI_DW_SET(0, 3) 0x%08X = 0x%08X\n", __func__,
	    IPU_DI_DW_SET(0, 3), reg);
#endif

	/* internal HSCYNC */
	imx51_ipuv3_di_sync_conf(sc, 1,
	    __DI_SW_GEN0(geom->panel_width + geom->hsync_width +
		geom->left + geom->right - 1, 1, 0, 0),
	    __DI_SW_GEN1(0, 1, 0, 0, 0, 0, 0),
	    0);

	/* HSYNC */
	imx51_ipuv3_di_sync_conf(sc, 2,
	    __DI_SW_GEN0(geom->panel_width + geom->hsync_width +
		geom->left + geom->right - 1, 1, 0, 1),
	    __DI_SW_GEN1(1, 1, 0, geom->hsync_width * 2, 1, 0, 0),
	    0);

	/* VSYNC */
	imx51_ipuv3_di_sync_conf(sc, 3,
	    __DI_SW_GEN0(geom->panel_height + geom->vsync_width +
		geom->upper + geom->lower - 1, 2, 0, 0),
	    __DI_SW_GEN1(1, 1, 0, geom->vsync_width * 2, 2, 0, 0),
	    0);

	IPUV3_WRITE(sc, di0, IPU_DI_SCR_CONF,
	    geom->panel_height + geom->vsync_width + geom->upper +
	    geom->lower - 1);

	/* Active Lines Start */
	imx51_ipuv3_di_sync_conf(sc, 4,
	    __DI_SW_GEN0(0, 3, geom->vsync_width + geom->upper, 3),
	    __DI_SW_GEN1(0, 0, 4, 0, 0, 0, 0),
	    geom->panel_height);

	/* Active Clock Start */
	imx51_ipuv3_di_sync_conf(sc, 5,
	    __DI_SW_GEN0(0, 1, geom->hsync_width + geom->left, 1),
	    __DI_SW_GEN1(0, 0, 5, 0, 0, 0, 0),
	    geom->panel_width);

	IPUV3_WRITE(sc, di0, IPU_DI_SW_GEN0(6), 0);
	IPUV3_WRITE(sc, di0, IPU_DI_SW_GEN1(6), 0);
	IPUV3_WRITE(sc, di0, IPU_DI_SW_GEN0(7), 0);
	IPUV3_WRITE(sc, di0, IPU_DI_SW_GEN1(7), 0);
	IPUV3_WRITE(sc, di0, IPU_DI_SW_GEN0(8), 0);
	IPUV3_WRITE(sc, di0, IPU_DI_SW_GEN1(8), 0);
	IPUV3_WRITE(sc, di0, IPU_DI_SW_GEN0(9), 0);
	IPUV3_WRITE(sc, di0, IPU_DI_SW_GEN1(9), 0);

	reg = IPUV3_READ(sc, di0, IPU_DI_STP_REP(6));
	reg &= ~DI_STP_REP_MASK(6);
	IPUV3_WRITE(sc, di0, IPU_DI_STP_REP(6), reg);
	IPUV3_WRITE(sc, di0, IPU_DI_STP_REP(7), 0);
	IPUV3_WRITE(sc, di0, IPU_DI_STP_REP(9), 0);

	IPUV3_WRITE(sc, di0, IPU_DI_GENERAL, 0);
	reg = ((3 - 1) << DI_SYNC_AS_GEN_VSYNC_SEL_SHIFT) | 0x2;
	IPUV3_WRITE(sc, di0, IPU_DI_SYNC_AS_GEN, reg);
	IPUV3_WRITE(sc, di0, IPU_DI_POL, DI_POL_DRDY_POLARITY_15);

	/* release DI counter */
	reg = IPUV3_READ(sc, cm, IPU_CM_DISP_GEN);
	reg |= CM_DISP_GEN_DI0_COUNTER_RELEASE;
	IPUV3_WRITE(sc, cm, IPU_CM_DISP_GEN, reg);

#ifdef IPUV3_DEBUG
	int i;
	printf("*** DI0 ***\n");
	for (i = 0; i <= 0x174; i += 4)
		printf("0x%08X = 0x%08X\n", i, IPUV3_READ(sc, di0, i));

	printf("%s: IPU_CM_DISP_GEN    : 0x%08X\n", __func__,
	    IPUV3_READ(sc, cm, IPU_CM_DISP_GEN));
	printf("%s: IPU_DI_SYNC_AS_GEN : 0x%08X\n", __func__,
	    IPUV3_READ(sc, di0, IPU_DI_SYNC_AS_GEN));
	printf("%s: IPU_DI_GENERAL     : 0x%08X\n", __func__,
	    IPUV3_READ(sc, di0, IPU_DI_GENERAL));
	printf("%s: IPU_DI_POL         : 0x%08X\n", __func__,
	    IPUV3_READ(sc, di0, IPU_DI_POL));
#endif
}


void
imx51_ipuv3_geometry(struct imx51_ipuv3_softc *sc,
    const struct lcd_panel_geometry *geom)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	sc->geometry = geom;

#ifdef IPUV3_DEBUG
	printf("%s: screen height = %d\n",__func__ , geom->panel_height);
	printf("%s:        width  = %d\n",__func__ , geom->panel_width);
	printf("%s: IPU Clock = %d\n", __func__,
	    imx51_get_clock(IMX51CLK_IPU_HSP_CLK_ROOT));
	printf("%s: Pixel Clock = %d\n", __func__, geom->pixel_clk);
#endif

	imx51_ipuv3_di_init(sc);

#ifdef IPUV3_DEBUG
	printf("%s: IPU_CM_DISP_GEN    : 0x%08X\n", __func__,
	    IPUV3_READ(sc, cm, IPU_CM_DISP_GEN));
#endif

	imx51_ipuv3_dc_display_config(sc, geom->panel_width);

	return;
}

/*
 * Initialize the IPUV3 controller.
 */
static void
imx51_ipuv3_initialize(struct imx51_ipuv3_softc *sc,
    const struct lcd_panel_geometry *geom)
{
	uint32_t reg;

	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	IPUV3_WRITE(sc, cm, IPU_CM_CONF, 0);

	/* reset */
	IPUV3_WRITE(sc, cm, IPU_CM_MEM_RST, CM_MEM_START | CM_MEM_EN);
	while (IPUV3_READ(sc, cm, IPU_CM_MEM_RST) & CM_MEM_START)
		; /* wait */

	imx51_ipuv3_dmfc_init(sc);
	imx51_ipuv3_dc_init(sc);

	imx51_ipuv3_geometry(sc, geom);

	/* set global alpha */
	IPUV3_WRITE(sc, dp, IPU_DP_GRAPH_WIND_CTRL_SYNC, 0x80 << 24);
	IPUV3_WRITE(sc, dp, IPU_DP_COM_CONF_SYNC, DP_DP_GWAM_SYNC);

	reg = IPUV3_READ(sc, cm, IPU_CM_DISP_GEN);
	reg |= CM_DISP_GEN_MCU_MAX_BURST_STOP |
	    CM_DISP_GEN_MCU_T(0x8);
	IPUV3_WRITE(sc, cm, IPU_CM_DISP_GEN, reg);
}

static void
imx51_ipuv3_init_screen(void *cookie, struct vcons_screen *scr,
		    int existing, long *defattr)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	struct imx51_ipuv3_softc *sc = cookie;
	struct rasops_info *ri = &scr->scr_ri;
	struct imx51_wsscreen_descr *descr = imx51_ipuv3_console.descr;

	if ((scr == &sc->console) && (sc->vd.active != NULL))
		return;

	ri->ri_bits = sc->active->buf_va;

	scr->scr_flags |= VCONS_DONT_READ;
	if (existing)
		ri->ri_flg |= RI_CLEAR;

	imx51_ipuv3_setup_rasops(sc, ri, descr, sc->geometry);

	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri,
	    ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
}

/*
 * Common driver attachment code.
 */
void
imx51_ipuv3_attach_sub(struct imx51_ipuv3_softc *sc,
    struct axi_attach_args *axia, const struct lcd_panel_geometry *geom)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	bus_space_tag_t iot = axia->aa_iot;
	bus_space_handle_t ioh;
	int error;

	aprint_normal(": i.MX51 IPUV3 controller\n");

	sc->n_screens = 0;
	LIST_INIT(&sc->screens);

	sc->iot = iot;
	sc->dma_tag = &imx_bus_dma_tag;

	/* map controller registers */
	error = bus_space_map(iot, IPU_CM_BASE, IPU_CM_SIZE, 0, &ioh);
	if (error)
		goto fail_retarn_cm;
	sc->cm_ioh = ioh;

	/* map Display Multi FIFO Controller registers */
	error = bus_space_map(iot, IPU_DMFC_BASE, IPU_DMFC_SIZE, 0, &ioh);
	if (error)
		goto fail_retarn_dmfc;
	sc->dmfc_ioh = ioh;

	/* map Display Interface registers */
	error = bus_space_map(iot, IPU_DI0_BASE, IPU_DI0_SIZE, 0, &ioh);
	if (error)
		goto fail_retarn_di0;
	sc->di0_ioh = ioh;

	/* map Display Processor registers */
	error = bus_space_map(iot, IPU_DP_BASE, IPU_DP_SIZE, 0, &ioh);
	if (error)
		goto fail_retarn_dp;
	sc->dp_ioh = ioh;

	/* map Display Controller registers */
	error = bus_space_map(iot, IPU_DC_BASE, IPU_DC_SIZE, 0, &ioh);
	if (error)
		goto fail_retarn_dc;
	sc->dc_ioh = ioh;

	/* map Image DMA Controller registers */
	error = bus_space_map(iot, IPU_IDMAC_BASE, IPU_IDMAC_SIZE, 0, &ioh);
	if (error)
		goto fail_retarn_idmac;
	sc->idmac_ioh = ioh;

	/* map CPMEM registers */
	error = bus_space_map(iot, IPU_CPMEM_BASE, IPU_CPMEM_SIZE, 0, &ioh);
	if (error)
		goto fail_retarn_cpmem;
	sc->cpmem_ioh = ioh;

	/* map DCTEMPL registers */
	error = bus_space_map(iot, IPU_DCTMPL_BASE, IPU_DCTMPL_SIZE, 0, &ioh);
	if (error)
		goto fail_retarn_dctmpl;
	sc->dctmpl_ioh = ioh;

#ifdef notyet
	sc->ih = imx51_ipuv3_intr_establish(IMX51_INT_IPUV3, IPL_BIO,
	    ipuv3intr, sc);
	if (sc->ih == NULL) {
		aprint_error_dev(sc->dev,
		    "unable to establish interrupt at irq %d\n",
		    IMX51_INT_IPUV3);
		return;
	}
#endif

	imx51_ipuv3_initialize(sc, geom);

#if NWSDISPLAY > 0
	struct imx51_wsscreen_descr *descr = imx51_ipuv3_console.descr;
	struct imx51_ipuv3_screen *scr;

	sc->mode = WSDISPLAYIO_MODE_EMUL;
	error = imx51_ipuv3_new_screen(sc, descr->depth, &scr);
	if (error) {
		aprint_error_dev(sc->dev,
		    "unable to create new screen (errno=%d)", error);
		return;
	}
	sc->active = scr;

	vcons_init(&sc->vd, sc, &descr->c,
	    imx51_ipuv3_console.accessops);
	sc->vd.init_screen = imx51_ipuv3_init_screen;

#ifdef IPUV3_DEBUG
	printf("%s: IPUV3 console ? %d\n", __func__, imx51_ipuv3_console.is_console);
#endif

	struct rasops_info *ri;
	long defattr;
	ri = &sc->console.scr_ri;

	vcons_init_screen(&sc->vd, &sc->console, 1,
	    &defattr);
	sc->console.scr_flags |= VCONS_SCREEN_IS_STATIC;

	descr->c.nrows = ri->ri_rows;
	descr->c.ncols = ri->ri_cols;
	descr->c.textops = &ri->ri_ops;
	descr->c.capabilities = ri->ri_caps;

	if (imx51_ipuv3_console.is_console) {
		wsdisplay_cnattach(&descr->c, ri, 0, 0, defattr);
		aprint_normal_dev(sc->dev, "console\n");
	}

	vcons_replay_msgbuf(&sc->console);

	imx51_ipuv3_start_dma(sc, scr);
#endif

	if (!pmf_device_register(sc->dev, imx51_ipuv3_suspend,
		imx51_ipuv3_resume)) {
		aprint_error_dev(sc->dev, "can't establish power hook\n");
	}

	return;

fail_retarn_dctmpl:
	bus_space_unmap(sc->iot, sc->cpmem_ioh, IPU_CPMEM_SIZE);
fail_retarn_cpmem:
	bus_space_unmap(sc->iot, sc->idmac_ioh, IPU_IDMAC_SIZE);
fail_retarn_idmac:
	bus_space_unmap(sc->iot, sc->dc_ioh, IPU_DC_SIZE);
fail_retarn_dp:
	bus_space_unmap(sc->iot, sc->dp_ioh, IPU_DP_SIZE);
fail_retarn_dc:
	bus_space_unmap(sc->iot, sc->di0_ioh, IPU_DI0_SIZE);
fail_retarn_di0:
	bus_space_unmap(sc->iot, sc->dmfc_ioh, IPU_DMFC_SIZE);
fail_retarn_dmfc:
	bus_space_unmap(sc->iot, sc->dc_ioh, IPU_CM_SIZE);
fail_retarn_cm:
	aprint_error_dev(sc->dev,
	    "failed to map registers (errno=%d)\n", error);
	return;
}

int
imx51_ipuv3_cnattach(bool isconsole, struct imx51_wsscreen_descr *descr,
    struct wsdisplay_accessops *accessops,
    const struct lcd_panel_geometry *geom)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));
	imx51_ipuv3_console.descr = descr;
	imx51_ipuv3_console.geom = geom;
	imx51_ipuv3_console.accessops = accessops;
	imx51_ipuv3_console.is_console = isconsole;
	return 0;
}

#ifdef notyet
/*
 * Interrupt handler.
 */
int
ipuv3intr(void *arg)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	struct imx51_ipuv3_softc *sc = (struct imx51_ipuv3_softc *)arg;
	bus_space_tag_t iot = sc->iot;
	bus_space_handle_t ioh = sc->dc_ioh;
	uint32_t status;

	status = IPUV3_READ(ioh, V3CR);
	/* Clear stickey status bits */
	IPUV3_WRITE(ioh, V3CR, status);

	return 1;
}
#endif

static void
imx51_ipuv3_write_dmaparam(struct imx51_ipuv3_softc *sc,
    int ch, uint32_t *value, int size)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	int i;
	uint32_t addr = ch * 0x40;

	for (i = 0; i < size; i++) {
		IPUV3_WRITE(sc, cpmem, addr + ((i % 5) * 0x4) +
		    ((i / 5) * 0x20), value[i]);
#ifdef IPUV3_DEBUG
		printf("%s: addr = 0x%08X, val = 0x%08X\n", __func__,
		    addr + ((i % 5) * 0x4) + ((i / 5) * 0x20),
		    IPUV3_READ(sc, cpmem, addr + ((i % 5) * 0x4) +
			((i / 5) * 0x20)));
#endif
	}
}

static void
imx51_ipuv3_build_param(struct imx51_ipuv3_softc *sc,
    struct imx51_ipuv3_screen *scr,
    uint32_t *params)
{
	const struct lcd_panel_geometry *geom = sc->geometry;

	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_FW,
	    (geom->panel_width - 1));
	imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_FH,
	    (geom->panel_height - 1));
	imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_EBA0,
	    scr->segs[0].ds_addr >> 3);
	imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_EBA1,
	    scr->segs[0].ds_addr >> 3);
	imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_SL,
	    (scr->stride - 1));

	imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_PFS, 0x7);
	imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_NPB, 32 - 1);

	switch (scr->depth) {
	case 32:
		/* ARBG888 */
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_BPP, 0x0);

		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS0, 0);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS1, 8);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS2, 16);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS3, 24);

		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID0, 8 - 1);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID1, 8 - 1);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID2, 8 - 1);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID3, 8 - 1);
		break;
	case 24:
		/* RBG888 */
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_BPP, 0x1);

		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS0, 0);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS1, 8);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS2, 16);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS3, 24);

		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID0, 8 - 1);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID1, 8 - 1);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID2, 8 - 1);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID3, 0);
		break;
	case 16:
		/* RBG565 */
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_BPP, 0x3);

		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS0, 0);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS1, 5);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS2, 11);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_OFS3, 16);

		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID0, 5 - 1);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID1, 6 - 1);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID2, 5 - 1);
		imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_WID3, 0);
		break;
	default:
		panic("%s: unsupported depth %d\n", __func__, scr->depth);
		break;
	}

	imx51_ipuv3_set_idma_param(params, IDMAC_Ch_PARAM_ID, 1);
}

static void
imx51_ipuv3_set_idma_param(uint32_t *params, uint32_t name, uint32_t val)
{
	int word = (name >> 16) & 0xff;
	int shift = (name >> 8) & 0xff;
	int width = name & 0xff;
	int index;

	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	index = word * 5;
	index += shift / 32;
	shift = shift % 32;

	params[index] |= val << shift;
	shift = 32 - shift;

	if (width > shift)
		params[index+1] |= val >> shift;
}

/*
 * Enable DMA to cause the display to be refreshed periodically.
 * This brings the screen to life...
 */
void
imx51_ipuv3_start_dma(struct imx51_ipuv3_softc *sc,
    struct imx51_ipuv3_screen *scr)
{
	int save;
	uint32_t params[10];
	uint32_t reg;

	DPRINTFN(3, ("%s: Pixel depth = %d\n", __func__, scr->depth));

	reg = IPUV3_READ(sc, idmac, IPU_IDMAC_CH_EN_1);
	reg &= ~__BIT(CH_PANNEL_BG);
	IPUV3_WRITE(sc, idmac, IPU_IDMAC_CH_EN_1, reg);

	memset(params, 0, sizeof(params));
	imx51_ipuv3_build_param(sc, scr, params);

	save = disable_interrupts(I32_bit);

	/* IDMAC configuration */
	imx51_ipuv3_write_dmaparam(sc, CH_PANNEL_BG, params,
	    sizeof(params) / sizeof(params[0]));

	IPUV3_WRITE(sc, idmac, IPU_IDMAC_CH_PRI_1, __BIT(CH_PANNEL_BG));

	/* double buffer */
	reg = IPUV3_READ(sc, cm, IPU_CM_CH_DB_MODE_SEL_0);
	reg |= __BIT(CH_PANNEL_BG);
	IPUV3_WRITE(sc, cm, IPU_CM_CH_DB_MODE_SEL_0, reg);

	reg = IPUV3_READ(sc, idmac, IPU_IDMAC_CH_EN_1);
	reg |= __BIT(CH_PANNEL_BG);
	IPUV3_WRITE(sc, idmac, IPU_IDMAC_CH_EN_1, reg);

	reg = IPUV3_READ(sc, cm, IPU_CM_GPR);
	reg &= ~CM_GPR_IPU_CH_BUF0_RDY0_CLR;
	IPUV3_WRITE(sc, cm, IPU_CM_GPR, reg);

	IPUV3_WRITE(sc, cm, IPU_CM_CUR_BUF_0, __BIT(CH_PANNEL_BG));

	restore_interrupts(save);

	imx51_ipuv3_enable_display(sc);

#ifdef IPUV3_DEBUG
	imx51_ipuv3_dump(sc);
#endif
}

/*
 * Disable screen refresh.
 */
static void
imx51_ipuv3_stop_dma(struct imx51_ipuv3_softc *sc)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	return;
}

/*
 * Create and initialize a new screen buffer.
 */
int
imx51_ipuv3_new_screen(struct imx51_ipuv3_softc *sc, int depth,
    struct imx51_ipuv3_screen **scrpp)
{
	const struct lcd_panel_geometry *geometry;
	struct imx51_ipuv3_screen *scr = NULL;
	int width, height;
	bus_size_t size;
	int error;
	int busdma_flag = (cold ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK);

	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	geometry = sc->geometry;

	width = geometry->panel_width;
	height = geometry->panel_height;

	scr = malloc(sizeof(*scr), M_DEVBUF, M_NOWAIT);
	if (scr == NULL)
		return ENOMEM;

	memset(scr, 0, sizeof(*scr));

	scr->nsegs = 0;
	scr->depth = depth;
	scr->stride = width * depth / 8;
	scr->buf_size = size = scr->stride * height;
	scr->buf_va = NULL;

	error = bus_dmamem_alloc(sc->dma_tag, size, 16, 0, scr->segs, 1,
	    &scr->nsegs, busdma_flag);

	if (error || scr->nsegs != 1) {
		/* XXX:
		 * Actually we can handle nsegs>1 case by means
		 * of multiple DMA descriptors for a panel.  It
		 * will make code here a bit hairly.
		 */
		if (error == 0)
			error = E2BIG;
		goto bad;
	}

	error = bus_dmamem_map(sc->dma_tag, scr->segs, scr->nsegs, size,
	    (void **)&scr->buf_va, busdma_flag | BUS_DMA_COHERENT);
	if (error)
		goto bad;

	memset(scr->buf_va, 0, scr->buf_size);

	/* map memory for DMA */
	error = bus_dmamap_create(sc->dma_tag, 1024*1024*2, 1, 1024*1024*2, 0,
	    busdma_flag, &scr->dma);
	if (error)
		goto bad;

	error = bus_dmamap_load(sc->dma_tag, scr->dma, scr->buf_va, size,
	    NULL, busdma_flag);
	if (error)
		goto bad;

	LIST_INSERT_HEAD(&sc->screens, scr, link);
	sc->n_screens++;

#ifdef IPUV3_DEBUG
	printf("%s: screen buffer width  %d\n", __func__, width);
	printf("%s: screen buffer height %d\n", __func__, height);
	printf("%s: screen buffer depth  %d\n", __func__, depth);
	printf("%s: screen buffer stride %d\n", __func__, scr->stride);
	printf("%s: screen buffer size   0x%08X\n", __func__,
	    (uint32_t)scr->buf_size);
	printf("%s: screen buffer addr virtual  %p\n", __func__, scr->buf_va);
	printf("%s: screen buffer addr physical %p\n", __func__,
	    (void *)scr->segs[0].ds_addr);
#endif

	scr->map_size = size;		/* used when unmap this. */

	*scrpp = scr;

	return 0;

bad:
#ifdef IPUV3_DEBUG
	printf("%s: error = 0x%08X\n", __func__, error);
#endif
	if (scr) {
		if (scr->buf_va)
			bus_dmamem_unmap(sc->dma_tag, scr->buf_va, size);
		if (scr->nsegs)
			bus_dmamem_free(sc->dma_tag, scr->segs, scr->nsegs);
		free(scr, M_DEVBUF);
	}
	*scrpp = NULL;
	return error;
}

#if NWSDISPLAY > 0
/*
 * Initialize rasops for a screen, as well as struct wsscreen_descr if this
 * is the first screen creation.
 */
static void
imx51_ipuv3_setup_rasops(struct imx51_ipuv3_softc *sc, struct rasops_info *rinfo,
    struct imx51_wsscreen_descr *descr,
    const struct lcd_panel_geometry *geom)
{

	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	rinfo->ri_flg = descr->flags;
	rinfo->ri_depth = descr->depth;
	rinfo->ri_width = geom->panel_width;
	rinfo->ri_height = geom->panel_height;
	rinfo->ri_stride = rinfo->ri_width * rinfo->ri_depth / 8;

	/* swap B and R */
	if (descr->depth == 16) {
		rinfo->ri_rnum = 5;
		rinfo->ri_rpos = 11;
		rinfo->ri_gnum = 6;
		rinfo->ri_gpos = 5;
		rinfo->ri_bnum = 5;
		rinfo->ri_bpos = 0;
	}

	if (descr->c.nrows == 0) {
		/* get rasops to compute screen size the first time */
		rasops_init(rinfo, 100, 100);
	} else {
		rasops_init(rinfo, descr->c.nrows, descr->c.ncols);
	}

	descr->c.nrows = rinfo->ri_rows;
	descr->c.ncols = rinfo->ri_cols;
	descr->c.capabilities = rinfo->ri_caps;
	descr->c.textops = &rinfo->ri_ops;
}
#endif
/*
 * Power management
 */
static bool
imx51_ipuv3_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct imx51_ipuv3_softc *sc = device_private(dv);
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));
	if (sc->active)
		imx51_ipuv3_stop_dma(sc);
	return true;
}

static bool
imx51_ipuv3_resume(device_t dv, const pmf_qual_t *qual)
{
	struct imx51_ipuv3_softc *sc = device_private(dv);
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));
	if (sc->active) {
		imx51_ipuv3_initialize(sc, sc->geometry);
		imx51_ipuv3_start_dma(sc, sc->active);
	}
	return true;
}

#if NWSDISPLAY > 0
int
imx51_ipuv3_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{
	struct vcons_data *vd = v;
	struct imx51_ipuv3_softc *sc = vd->cookie;
	struct imx51_ipuv3_screen *scr = cookie, *old;
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	old = sc->active;
	if (old == scr)
		return 0;
	if (old)
		imx51_ipuv3_stop_dma(sc);
	imx51_ipuv3_start_dma(sc, scr);
	sc->active = scr;
	return 0;
}

int
imx51_ipuv3_alloc_screen(void *v, const struct wsscreen_descr *_type,
    void **cookiep, int *curxp, int *curyp, long *attrp)
{
	struct vcons_data *vd = v;
	struct imx51_ipuv3_softc *sc = vd->cookie;
	struct imx51_ipuv3_screen *scr;
	const struct imx51_wsscreen_descr *type =
		(const struct imx51_wsscreen_descr *)_type;
	int error;
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	error = imx51_ipuv3_new_screen(sc, type->depth, &scr);
	if (error)
		return error;

	/*
	 * initialize raster operation for this screen.
	 */
	scr->rinfo.ri_flg = 0;
	scr->rinfo.ri_depth = type->depth;
	scr->rinfo.ri_bits = scr->buf_va;
	scr->rinfo.ri_width = sc->geometry->panel_width;
	scr->rinfo.ri_height = sc->geometry->panel_height;
	scr->rinfo.ri_stride = scr->rinfo.ri_width * scr->rinfo.ri_depth / 8;
#ifdef CPU_XSCALE_PXA270
	if (scr->rinfo.ri_depth > 16)
		scr->rinfo.ri_stride = scr->rinfo.ri_width * 4;
#endif
	scr->rinfo.ri_wsfcookie = -1;	/* XXX */

	rasops_init(&scr->rinfo, type->c.nrows, type->c.ncols);

	(*scr->rinfo.ri_ops.allocattr)(&scr->rinfo, 0, 0, 0, attrp);

	*cookiep = scr;
	*curxp = 0;
	*curyp = 0;

	return 0;
}

void
imx51_ipuv3_free_screen(void *v, void *cookie)
{
	struct vcons_data *vd = v;
	struct imx51_ipuv3_softc *sc = vd->cookie;
	struct imx51_ipuv3_screen *scr = cookie;
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	LIST_REMOVE(scr, link);
	sc->n_screens--;
	if (scr == sc->active) {
		/* at first, we need to stop LCD DMA */
		sc->active = NULL;

		printf("lcd_free on active screen\n");

		imx51_ipuv3_stop_dma(sc);
	}

	if (scr->buf_va)
		bus_dmamem_unmap(sc->dma_tag, scr->buf_va, scr->map_size);
	if (scr->nsegs > 0)
		bus_dmamem_free(sc->dma_tag, scr->segs, scr->nsegs);
	free(scr, M_DEVBUF);
}

int
imx51_ipuv3_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct vcons_data *vd = v;
	struct imx51_ipuv3_softc *sc = vd->cookie;
	struct wsdisplay_fbinfo *wsdisp_info;
	struct vcons_screen *ms = vd->active;

	DPRINTFN(5, ("%s : cmd 0x%X (%d)\n", __func__, (u_int)cmd, (int)IOCGROUP(cmd)));

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_IMXIPU;
		return 0;

	case WSDISPLAYIO_GINFO:
		wsdisp_info = (struct wsdisplay_fbinfo *)data;
		wsdisp_info->height = ms->scr_ri.ri_height;
		wsdisp_info->width = ms->scr_ri.ri_width;
		wsdisp_info->depth = ms->scr_ri.ri_depth;
		wsdisp_info->cmsize = 0;
		return 0;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = ms->scr_ri.ri_stride;
		return 0;

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
		return EPASSTHROUGH;	/* XXX Colormap */

	case WSDISPLAYIO_SVIDEO:
		if (*(int *)data == WSDISPLAYIO_VIDEO_ON) {
			/* turn it on */
		}
		else {
			/* start IPUV3 shutdown */
			/* sleep until interrupt */
		}
		return 0;

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data =  WSDISPLAYIO_VIDEO_ON;
		return 0;

	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		return EPASSTHROUGH;	/* XXX */
	case WSDISPLAYIO_SMODE:
		{
			int new_mode = *(int*)data;

			/* notify the bus backend */
			if (new_mode != sc->mode) {
				sc->mode = new_mode;
				if(new_mode == WSDISPLAYIO_MODE_EMUL)
					vcons_redraw_screen(ms);
			}
		}
		return 0;
	}

	return EPASSTHROUGH;
}

paddr_t
imx51_ipuv3_mmap(void *v, void *vs, off_t offset, int prot)
{
	DPRINTFN(5, ("%s : %d\n", __func__, __LINE__));

	struct vcons_data *vd = v;
	struct imx51_ipuv3_softc *sc = vd->cookie;
	struct imx51_ipuv3_screen *scr = sc->active;

	return bus_dmamem_mmap(sc->dma_tag, scr->segs, scr->nsegs,
	    offset, prot,
	    BUS_DMA_WAITOK | BUS_DMA_COHERENT);
}
#endif /* NWSDISPLAY > 0 */

