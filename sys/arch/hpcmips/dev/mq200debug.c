/*	$NetBSD: mq200debug.c,v 1.1.2.2 2001/03/27 15:30:54 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 TAKEMURA Shin
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifdef _KERNEL
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#else
#include <stdio.h>
#endif
#include <sys/types.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include "opt_mq200.h"
#include "mq200var.h"
#include "mq200reg.h"
#include "mq200priv.h"

#define ENABLE(b)	((b)?"enable":"disable")

#ifdef MQ200_DEBUG

char *mq200_clknames[] = { "BUS", "PLL1", "PLL2", "PLL3" };

void
mq200_dump_pll(struct mq200_softc *sc)
{
	int n, m;
	u_int32_t reg, pm00r;
	int clocks[4];
	int memclock, geclock;
	static char *clknames[] = { "BUS", "PLL1", "PLL2", "PLL3" };
	static char *fd_names[] = { "1", "1.5", "2.5", "3.5", "4.5", "5.5", "6.5" };
	static int fd_vals[] = { 10, 15, 25, 35, 45, 55, 65 };
#define FIXEDFLOAT1000(a)	(a)/1000, (a)%1000

	/* PM00R */
	pm00r = mq200_read(sc, MQ200_PMCR);
	geclock = (pm00r&MQ200_PMC_GE_CLK_MASK)>>MQ200_PMC_GE_CLK_SHIFT;

	/* MM01R */
	reg = mq200_read(sc, MQ200_MMR(1));
	memclock = (reg & MQ200_MM01_CLK_PLL2) ? 2 : 1;

	/* bus clock */
	clocks[0] = 0;

	/* PLL1 */
	reg = mq200_read(sc, MQ200_DCMISCR);
	m = ((reg & MQ200_PLL_M_MASK) >> MQ200_PLL_M_SHIFT) + 1;
	n = ((((reg & MQ200_PLL_N_MASK) >> MQ200_PLL_N_SHIFT) + 1) |
	    ((pm00r & MQ200_PMC_PLL1_N) << MQ200_PMC_PLL1_N_SHIFT));
	n <<= ((reg & MQ200_PLL_P_MASK) >> MQ200_PLL_P_SHIFT);
	printf("  PLL1:%3d.%03dMHz(0x%08x, %d.%03dMHzx%3d/%3d)\n",
	    FIXEDFLOAT1000(sc->sc_baseclock*m/n),
	    reg, FIXEDFLOAT1000(sc->sc_baseclock), m, n);
	clocks[1] = sc->sc_baseclock*m/n;

	/* PLL2 */
	if (pm00r & MQ200_PMC_PLL2_ENABLE) {
		reg = mq200_read(sc, MQ200_PLL2R);
		m = ((reg & MQ200_PLL_M_MASK) >> MQ200_PLL_M_SHIFT) + 1;
		n = ((((reg & MQ200_PLL_N_MASK) >> MQ200_PLL_N_SHIFT) +1) <<
		    ((reg & MQ200_PLL_P_MASK) >> MQ200_PLL_P_SHIFT));
		clocks[2] = sc->sc_baseclock*m/n;
		printf("  PLL2:%3d.%03dMHz(0x%08x, %d.%03dMHzx%3d/%3d)\n",
		    FIXEDFLOAT1000(sc->sc_baseclock*m/n),
		    reg, FIXEDFLOAT1000(sc->sc_baseclock), m, n);
	} else {
		printf("  PLL2: disable\n");
		clocks[2] = 0;
	}

	/* PLL3 */
	if (pm00r & MQ200_PMC_PLL3_ENABLE) {
		reg = mq200_read(sc, MQ200_PLL3R);
		m = (((reg & MQ200_PLL_M_MASK) >> MQ200_PLL_M_SHIFT) + 1);
		n = ((((reg & MQ200_PLL_N_MASK) >> MQ200_PLL_N_SHIFT) + 1) <<
		    ((reg & MQ200_PLL_P_MASK) >> MQ200_PLL_P_SHIFT));
		clocks[3] = sc->sc_baseclock*m/n;
		printf("  PLL3:%3d.%03dMHz(0x%08x, %d.%03dMHzx%3d/%3d)\n",
		    FIXEDFLOAT1000(sc->sc_baseclock*m/n),
		    reg, FIXEDFLOAT1000(sc->sc_baseclock), m, n);
	} else {
		printf("  PLL3: disable\n");
		clocks[3] = 0;
	}

	printf("   MEM:%3d.%03dMHz(%s)\n",
	    FIXEDFLOAT1000(clocks[memclock]),
	    clknames[memclock]);
	printf("    GE:%3d.%03dMHz(%s)\n",
	    FIXEDFLOAT1000(clocks[geclock]),
	    clknames[geclock]);

	/* GC1 */
	reg = mq200_read(sc, MQ200_GCCR(MQ200_GC1));
	if (reg & MQ200_GCC_ENABLE) {
		int fd, sd, rc;
		rc = (reg&MQ200_GCC_RCLK_MASK)>>MQ200_GCC_RCLK_SHIFT;
		fd = (reg&MQ200_GCC_MCLK_FD_MASK)>>MQ200_GCC_MCLK_FD_SHIFT;
		sd = (reg&MQ200_GCC_MCLK_SD_MASK)>>MQ200_GCC_MCLK_SD_SHIFT;
		printf("   GC1:%3d.%03dMHz(%s/%s/%d)",
		    FIXEDFLOAT1000(clocks[rc]*10/fd_vals[fd]/sd),
		    clknames[rc], fd_names[fd], sd);
		/* GC01R */
		reg = mq200_read(sc, MQ200_GC1CRTCR);
		if (reg&MQ200_GC1CRTC_DACEN)
			printf(", CRT");
		reg = mq200_read(sc, MQ200_FPCR);
		if ((reg & MQ200_FPC_ENABLE) && !(reg & MQ200_FPC_GC2))
			printf(", LCD");
		printf("\n");
	} else {
		printf("   GC1: disable\n");
	}

	/* GC2 */
	reg = mq200_read(sc, MQ200_GCCR(MQ200_GC2));
	if (reg & MQ200_GCC_ENABLE) {
		int fd, sd, rc;
		rc = (reg&MQ200_GCC_RCLK_MASK)>>MQ200_GCC_RCLK_SHIFT;
		fd = (reg&MQ200_GCC_MCLK_FD_MASK)>>MQ200_GCC_MCLK_FD_SHIFT;
		sd = (reg&MQ200_GCC_MCLK_SD_MASK)>>MQ200_GCC_MCLK_SD_SHIFT;
		printf("   GC2:%3d.%03dMHz(%s/%s/%d)",
		    FIXEDFLOAT1000(clocks[rc]*10/fd_vals[fd]/sd),
		    clknames[rc], fd_names[fd], sd);
		reg = mq200_read(sc, MQ200_FPCR);
		if ((reg & MQ200_FPC_ENABLE) && (reg & MQ200_FPC_GC2))
			printf(", FP");
		printf("\n");
	} else {
		printf("   GC2: disable\n");
	}
}

struct {
	char *name;
	u_int32_t base;
	int start, end;
} regs[] = {
	{ "GC",	MQ200_GCR(0),	0x00,	0x13 },
	{ "GC",	MQ200_GCR(0),	0x20,	0x33 },
	{ "FP",	MQ200_FP,	0x00,	0x0f },
	{ "CC",	MQ200_CC,	0x00,	0x01 },
	{ "PC",	MQ200_PC,	0x00,	0x05 },
	{ "MM",	MQ200_MM,	0x00,	0x04 },
	{ "DC",	MQ200_DC,	0x00,	0x03 },
	{ "PM",	MQ200_PM,	0x00,	0x03 },
	{ "PM",	MQ200_PM,	0x06,	0x07 },
	{ "IN",	MQ200_IN,	0x00,	0x03 },
};

char *
mq200_regname(struct mq200_softc *sc, int offset, char *buf, int bufsize)
{
	int i;

	for (i = 0; i < sizeof(regs)/sizeof(*regs); i++)
		if (regs[i].base + regs[i].start * 4 <= offset &&
		    offset <= regs[i].base + regs[i].end * 4) {
			sprintf(buf, "%s%02XR", regs[i].name,
			    (offset - regs[i].base) / 4);
			return (buf);
		}
	sprintf(buf, "OFFSET %02X", offset);
	return (buf);
}

void
mq200_dump_all(struct mq200_softc *sc)
{
	int i, j;

	for (i = 0; i < sizeof(regs)/sizeof(*regs); i++)
		for (j = regs[i].start; j <= regs[i].end; j++)
			printf("%s%02XR: %08x\n",
			    regs[i].name, j,
			    mq200_read(sc, regs[i].base + (j * 4)));
}

void
mq200_write(struct mq200_softc *sc, int offset, u_int32_t data)
{
	int i;
	char buf[32];

	for (i = 0; i < MQ200_I_MAX; i++) {
	    if (sc->sc_regctxs[i].offset == offset)
		printf("mq200_write: WARNING: raw access %s\n",
		    mq200_regname(sc, offset, buf, sizeof(buf)));
	}

	mq200_writex(sc, offset, data);
}

#if 0
void
mq200_dump_gc(struct mq200_softc *sc, int gc)
{
	u_int32_t reg;
	char *depth_names[] = {
		"1bpp with CLUT",
		"2bpp with CLUT",
		"4bpp with CLUT",
		"8bpp with CLUT",
		"16bpp with CLUT",
		"24bpp with CLUT",
		"32bpp(RGB) with CLUT",
		"32bpp(BGR) with CLUT",
		"1bpp w/o CLUT",
		"2bpp w/o CLUT",
		"4bpp w/o CLUT",
		"8bpp w/o CLUT",
		"16bpp w/o CLUT",
		"24bpp w/o CLUT",
		"32bpp(RGB) w/o CLUT",
		"32bpp(BGR) w/o CLUT",
	};
	char *rc_names[] = { "BUS", "PLL1", "PLL2", "PLL3" };
	char *fd_names[] = { "1", "1.5", "2.5", "3.5", "4.5", "5.5", "6.5" };

	/*
	 * GC00R	Graphics Controller Control
	 */
	reg = mq200_read(sc, MQ200_GCCR(gc));
	printf("GC00R=0x%08x: ", reg);
	printf("%s %s%s%s%s%s\n",
	    ENABLE(reg & MQ200_GCC_ENABLE),
	    (reg & MQ200_GCC_HCRESET)?"HC_reset ":"",
	    (reg & MQ200_GCC_VCRESET)?"VC_reset ":"",
	    (reg & MQ200_GCC_HCEN)?"cursor_enable ":"",
	    (reg & MQ200_GCC_TESTMODE0)?"test_mode0 ":"",
	    (reg & MQ200_GCC_TESTMODE1)?"test_mode1 ":"");
	printf("  window: %s %s\n",
	    ENABLE(reg & MQ200_GCC_WINEN),
	    depth_names[(reg&MQ200_GCC_DEPTH_MASK)>>MQ200_GCC_DEPTH_SHIFT]);
	printf("  altwin: %s %s\n",
	    ENABLE(reg & MQ200_GCC_ALTEN),
	    depth_names[(reg&MQ200_GCC_ALTDEPTH_MASK)>>MQ200_GCC_ALTDEPTH_SHIFT]);
	printf("   clock: root_clock/first_div/second_div = %s/%s/%d\n",
	    rc_names[(reg&MQ200_GCC_RCLK_MASK)>>MQ200_GCC_RCLK_SHIFT],
	    fd_names[(reg&MQ200_GCC_MCLK_FD_MASK)>>MQ200_GCC_MCLK_FD_SHIFT],
	    (reg&MQ200_GCC_MCLK_SD_MASK)>>MQ200_GCC_MCLK_SD_SHIFT);

	if (gc == 0) {
		/*
		 * GC01R	Graphics Controller CRT Control
		 */
		reg = mq200_read(sc, MQ200_GC1CRTCR);
		printf("GC01R=0x%08x:\n", reg);
		printf("  CRT DAC: %s\n",
		    ENABLE(reg&MQ200_GC1CRTC_DACEN));

		printf("  power down mode: H-sync=");
		switch (reg & MQ200_GC1CRTC_HSYNC_PMMASK) {
		case MQ200_GC1CRTC_HSYNC_PMNORMAL:
			if (reg & MQ200_GC1CRTC_HSYNC_PMCLK)
				printf("PMCLK");
			else
				printf("LOW");
			break;
		case MQ200_GC1CRTC_HSYNC_PMLOW:
			printf("LOW");
			break;
		case MQ200_GC1CRTC_HSYNC_PMHIGH:
			printf("HIGH");
			break;
		default:
			printf("???");
			break;
		}
	    
		printf("  V-sync=");
		switch (reg & MQ200_GC1CRTC_VSYNC_PMMASK) {
		case MQ200_GC1CRTC_VSYNC_PMNORMAL:
			if (reg & MQ200_GC1CRTC_VSYNC_PMCLK)
				printf("PMCLK");
			else
				printf("LOW");
			break;
		case MQ200_GC1CRTC_VSYNC_PMLOW:
			printf("LOW");
			break;
		case MQ200_GC1CRTC_VSYNC_PMHIGH:
			printf("HIGH");
			break;
		default:
			printf("???");
			break;
		}
		printf("\n");

		printf("  sync active: H=%s  V=%s\n",
		    (reg & MQ200_GC1CRTC_HSYNC_ACTVLOW)?"low":"high",
		    (reg & MQ200_GC1CRTC_VSYNC_ACTVLOW)?"low":"high");
		printf("  other: ");
		if (reg & MQ200_GC1CRTC_SYNC_PEDESTAL_EN)
			printf("Sync_pedestal ");
		if (reg & MQ200_GC1CRTC_BLANK_PEDESTAL_EN)
			printf("Blank_pedestal ");
		if (reg & MQ200_GC1CRTC_COMPOSITE_SYNC_EN)
			printf("Conposite_sync ");
		if (reg & MQ200_GC1CRTC_VREF_EXTR)
			printf("External_VREF ");
		if (reg & MQ200_GC1CRTC_MONITOR_SENCE_EN) {
			if (reg & MQ200_GC1CRTC_CONSTANT_OUTPUT_EN)
				printf("Monitor_sence=%s%s%s/- ",
				    (reg & MQ200_GC1CRTC_BLUE_NOTLOADED)?"":"B",
				    (reg & MQ200_GC1CRTC_RED_NOTLOADED)?"":"R",
				    (reg & MQ200_GC1CRTC_GREEN_NOTLOADED)?"":"G");
			else
				printf("Monitor_sence=%s%s%s/0x%02x ",
				    (reg & MQ200_GC1CRTC_BLUE_NOTLOADED)?"":"B",
				    (reg & MQ200_GC1CRTC_RED_NOTLOADED)?"":"R",
				    (reg & MQ200_GC1CRTC_GREEN_NOTLOADED)?"":"G",
				    (reg & MQ200_GC1CRTC_OUTPUT_LEVEL_MASK)>>MQ200_GC1CRTC_OUTPUT_LEVEL_SHIFT);
		}
		if (reg & MQ200_GC1CRTC_MONO)
			printf("Mono_monitor ");
		printf("\n");
	}

	/*
	 * GC02R	Horizontal Display Control
	 */
	reg = mq200_read(sc, MQ200_GCHDCR(gc));
	if (gc == 0) {
		printf("GC02R=0x%08x: Horizontal display total=%03d end=%03d\n", reg,
		    (reg&MQ200_GC1HDC_TOTAL_MASK)>>MQ200_GC1HDC_TOTAL_SHIFT,
		    (reg&MQ200_GCHDC_END_MASK)>>MQ200_GCHDC_END_SHIFT);
	} else {
		printf("GC02R=0x%08x: Horizontal display end=%03d\n", reg,
		    (reg&MQ200_GCHDC_END_MASK)>>MQ200_GCHDC_END_SHIFT);
	}

	/*
	 * GC03R	Vertical Display Control
	 */
	reg = mq200_read(sc, MQ200_GCVDCR(gc));
	if (gc == 0) {
		printf("GC03R=0x%08x:   Vertical display total=%03d end=%03d\n", reg,
		    (reg&MQ200_GC1VDC_TOTAL_MASK)>>MQ200_GC1VDC_TOTAL_SHIFT,
		    (reg&MQ200_GCVDC_END_MASK)>>MQ200_GCVDC_END_SHIFT);
	} else {
		printf("GC03R=0x%08x:   Vertical display end=%03d\n", reg,
		    (reg&MQ200_GCVDC_END_MASK)>>MQ200_GCVDC_END_SHIFT);
	}

	/*
	 * GC04R	Horizontal Sync Control
	 */
	reg = mq200_read(sc, MQ200_GCHSCR(gc));
	printf("GC04R=0x%08x: Horizontal    sync start=%03d end=%03d\n", reg,
	    (reg&MQ200_GCHSC_START_MASK)>>MQ200_GCHSC_START_SHIFT,
	    (reg&MQ200_GCHSC_END_MASK)>>MQ200_GCHSC_END_SHIFT);

	/*
	 * GC05R	Vertical Sync Control
	 */
	reg = mq200_read(sc, MQ200_GCVSCR(gc));
	printf("GC05R=0x%08x:   Vertical    sync start=%03d end=%03d\n", reg,
	    (reg&MQ200_GCVSC_START_MASK)>>MQ200_GCVSC_START_SHIFT,
	    (reg&MQ200_GCVSC_END_MASK)>>MQ200_GCVSC_END_SHIFT);

	if (gc == 0) {
		/*
		 * GC07R	Vertical Display Count
		 */
		reg = mq200_read(sc, MQ200_GC1VDCNTR);
		printf("GC07R=0x%08x: Vertical Display Count=%d\n", reg,
		    (reg&MQ200_GC1VDCNT_MASK));
	}

	/*
	 * GC08R	Window Horizontal Control
	 */
	reg = mq200_read(sc, MQ200_GCWHCR(gc));
	printf("GC08R=0x%08x: Window Horizontal start=%03d width=%03d",
	    reg,
	    (reg&MQ200_GCWHC_START_MASK)>> MQ200_GCWHC_START_SHIFT,
	    (reg&MQ200_GCWHC_WIDTH_MASK)>> MQ200_GCWHC_WIDTH_SHIFT);
	if (gc == 0) {
		printf(" add=%03x",
		    (reg&MQ200_GC1WHC_ALD_MASK)>> MQ200_GC1WHC_ALD_SHIFT);
	}
	printf("\n");

	/*
	 * GC09R	Window Vertical Control
	 */
	reg = mq200_read(sc, MQ200_GCWVCR(gc));
	printf("GC09R=0x%08x:   Window Vertical start=%03d hight=%03d\n",
	    reg,
	    (reg&MQ200_GCWVC_START_MASK)>> MQ200_GCWVC_START_SHIFT,
	    (reg&MQ200_GCWVC_HEIGHT_MASK)>> MQ200_GCWVC_HEIGHT_SHIFT);

	/*
	 * GC0AR	Alternate Window Horizontal Control
	 */
	reg = mq200_read(sc, MQ200_GCAWHCR(gc));
	printf("GC0AR=0x%08x: Altwin Horizontal start=%03d width=%03d",
	    reg,
	    (reg&MQ200_GCAWHC_START_MASK)>> MQ200_GCAWHC_START_SHIFT,
	    (reg&MQ200_GCAWHC_WIDTH_MASK)>> MQ200_GCAWHC_WIDTH_SHIFT);
	if (gc == 0) {
		printf(" add=%03d",
		    (reg&MQ200_GC1AWHC_ALD_MASK)>> MQ200_GC1AWHC_ALD_SHIFT);
	}
	printf("\n");

	/*
	 * GC0BR	Alternate Window Vertical Control
	 */
	reg = mq200_read(sc, MQ200_GCAWVCR(gc));
	printf("GC0BR=0x%08x:   Altwin Vertical start=%03d hight=%03d\n",
	    reg,
	    (reg&MQ200_GCAWVC_START_MASK)>> MQ200_GCAWVC_START_SHIFT,
	    (reg&MQ200_GCAWVC_HEIGHT_MASK)>> MQ200_GCAWVC_HEIGHT_SHIFT);

	/*
	 * GC0CR	Window Start Address
	 */
	reg = mq200_read(sc, MQ200_GCWSAR(gc));
	printf("GC0CR=0x%08x: Window start address=0x%08x\n",
	    reg, (reg&MQ200_GCWSA_MASK));

	/*
	 * GC0DR	Alternate Window Start Address
	 */
	reg = mq200_read(sc, MQ200_GCAWSAR(gc));
	printf("GC0DR=0x%08x: Altwin start address=0x%08x palette_index=%02d\n",
	    reg, (reg&MQ200_GCAWSA_MASK),
	    (reg&MQ200_GCAWPI_MASK)>>MQ200_GCAWPI_SHIFT);

	/*
	 * GC0ER	Windows Stride
	 */
	reg = mq200_read(sc, MQ200_GCWSTR(gc));
	printf("GC0ER=0x%08x: Stride window=%04d  altwin=%04d\n",
	    reg, 
	    (reg&MQ200_GCWST_MASK)>>MQ200_GCWST_SHIFT,
	    (reg&MQ200_GCAWST_MASK)>>MQ200_GCAWST_SHIFT);

	/*
	 * GC10R	Hardware Cursor Position
	 */
	reg = mq200_read(sc, MQ200_GCHCPR(gc));
	printf("GC10R=0x%08x: Hardware Cursor Position %d,%d\n",
	    reg, 
	    (reg&MQ200_GCHCP_HSTART_MASK)>>MQ200_GCHCP_HSTART_SHIFT,
	    (reg&MQ200_GCHCP_VSTART_MASK)>>MQ200_GCHCP_VSTART_SHIFT);

	/*
	 * GC11R	Hardware Cursor Start Address and Offset
	 */
	reg = mq200_read(sc, MQ200_GCHCAOR(gc));
	printf("GC11R=0x%08x: Hardware Cursor Start Address and Offset\n",
	    reg);

	/*
	 * GC12R	Hardware Cursor Foreground Color
	 */
	reg = mq200_read(sc, MQ200_GCHCFCR(gc));
	printf("GC12R=0x%08x: Hardware Cursor Foreground Color\n", reg);

	/*
	 * GC13R	Hardware Cursor Background Color
	 */
	reg = mq200_read(sc, MQ200_GCHCBCR(gc));
	printf("GC13R=0x%08x: Hardware Cursor Background Color\n", reg);

}

void
mq200_dump_fp(struct mq200_softc *sc)
{
	u_int32_t reg;
#define I(type)	((type)>>MQ200_FPC_TYPE_SHIFT)
	static char *panel_type_names[64] = {
		[I(MQ200_FPC_TFT4MONO)] = 	"TFT 4bit mono",
		[I(MQ200_FPC_TFT12)] = 		"TFT 12bit color",
		[I(MQ200_FPC_SSTN4)] = 		"S-STN 4bit color",
		[I(MQ200_FPC_DSTN8)] = 		"D-STN 8bit color",
		[I(MQ200_FPC_TFT6MONO)] = 	"TFT 6bit mono",
		[I(MQ200_FPC_TFT18)] = 		"TFT 18bit color",
		[I(MQ200_FPC_SSTN8)] = 		"S-STN 8bit color",
		[I(MQ200_FPC_DSTN16)] = 	"D-STN 16bit color",
		[I(MQ200_FPC_TFT8MONO)] = 	"TFT 8bit mono",
		[I(MQ200_FPC_TFT24)] =	 	"TFT 24bit color",
		[I(MQ200_FPC_SSTN12)] = 	"S-STN 12bit color",
		[I(MQ200_FPC_DSTN24)] = 	"D-STN 24bit color",
		[I(MQ200_FPC_SSTN16)] = 	"S-STN 16bit color",
		[I(MQ200_FPC_SSTN24)] = 	"S-STN 24bit color",
	};

	reg = mq200_read(sc, MQ200_FPCR);
	printf("FP00R=0x%08x: Flat Panel Control\n", reg);
	printf("  %s, driven by %s, %s\n",
	    ENABLE(reg & MQ200_FPC_ENABLE),
	    (reg & MQ200_FPC_GC2) ? "GC2" : "GC1",
	    panel_type_names[(reg & MQ200_FPC_TYPE_MASK) >> MQ200_FPC_TYPE_SHIFT]);
	reg = mq200_read(sc, MQ200_FPPCR);
	printf("FP01R=0x%08x: Flat Panel Output Pin Control\n", reg);
}

void
mq200_dump_dc(struct mq200_softc *sc)
{
	u_int32_t reg;

	reg = 0;
}
#endif /* 0 */

#endif /* MQ200_DEBUG */
