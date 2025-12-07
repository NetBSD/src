/*	$NetBSD: newport.c,v 1.24 2025/12/07 02:11:49 macallan Exp $	*/

/*
 * Copyright (c) 2003 Ilpo Ruotsalainen
 *               2009 Michael Lorenz
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
 * 
 * <<Id: LICENSE_GC,v 1.1 2001/10/01 23:24:05 cgd Exp>>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: newport.c,v 1.24 2025/12/07 02:11:49 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <machine/sysconf.h>
#include <machine/machtype.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <sgimips/gio/giovar.h>
#include <sgimips/gio/newportvar.h>
#include <sgimips/gio/newportreg.h>

struct newport_dcb_cs_params {
	int cs_width;
	int cs_hold;
	int cs_setup;
};

struct newport_monitor_entry {
	const char *monitor;
	int x;
	int y;
	int hz;
	bool unknown;
	int cfreq;
};

/* otherwise (likely PAL/NTSC modes) */
static struct newport_dcb_cs_params newport_dcb_cs_wslow = { 0, 12, 12 };
/* cfreq > 59MHz */
static struct newport_dcb_cs_params newport_dcb_cs_slow = { 0, 5, 5 };
/* cfreq > 119MHz */
static struct newport_dcb_cs_params newport_dcb_cs_fast = { 0, 1, 2 };

/*
 * This (ideally) matches what the boot firmware is configuring as monitor
 * parameters in the RAMDAC, VC2, etc chips.
 */
static struct newport_monitor_entry newport_monitor_list[] = {
	/* Firmware monitor IDs */
	{ "1024x768 60Hz (unknown)", 1024, 768, 60, true, 64 },
	{ "1280x1024 76Hz", 1280, 1024, 76, false, 140 },
	{ "1280x1024 76Hz", 1280, 1024, 76, false, 140 },
	{ "1024x768 60Hz (unknown)", 1024, 768, 60, true, 64 },
	{ "1024x768 60Hz (unknown)", 1024, 768, 60, true, 64 },
	{ "1024x768 60Hz (unknown)", 1024, 768, 60, true, 64 },
	{ "1024x768 70Hz", 1024, 768, 70, false, 75 },
	{ "1024x768 60Hz (unknown)", 1024, 768, 60, true, 64 },
	{ "1024x768 60Hz (unknown)", 1024, 768, 60, true, 64 },
	{ "1280x1024 72Hz", 1280, 1024, 72, false, 130 },
	{ "1280x1024 60Hz", 1280, 1024, 60, false, 108 },
	{ "1280x1024 76Hz", 1280, 1024, 76, false, 140 },
	{ "1280x1024 60Hz", 1280, 1024, 60, false, 108 },
	{ "1280x1024 60Hz", 1280, 1024, 60, false, 108 },
	{ "1024x768 60Hz (unknown)", 1024, 768, 60, true, 64 },
	{ "1024x768 60Hz (unknown)", 1024, 768, 60, true, 64 },
};

struct newport_softc {
	device_t sc_dev;

	struct newport_devconfig *sc_dc;
	
};

struct newport_devconfig {
	bus_space_tag_t		dc_st;
	bus_space_handle_t	dc_sh;
	bus_addr_t		dc_addr;

	struct newport_softc	*dc_sc;

	int			dc_boardrev;
	int			dc_vc2rev;
	int			dc_cmaprev;
	int			dc_xmaprev;
	int			dc_rexrev;
	int			dc_xres;
	int			dc_yres;
	int			dc_depth;
	int			dc_monitor_cmap_id;
	int			dc_monitor_prom_id;

	int			dc_font;
	struct wsscreen_descr	*dc_screen;
	int			dc_mode;
	struct vcons_data	dc_vd;
};

static int  newport_match(device_t, struct cfdata *, void *);
static void newport_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(newport, sizeof(struct newport_softc),
    newport_match, newport_attach, NULL, NULL);

/* textops */
static void newport_cursor(void *, int, int, int);
static void newport_cursor_dummy(void *, int, int, int);
static void newport_putchar(void *, int, int, u_int, long);
static void newport_copycols(void *, int, int, int, int);
static void newport_erasecols(void *, int, int, int, long);
static void newport_copyrows(void *, int, int, int);
static void newport_eraserows(void *, int, int, long);

static void newport_init_screen(void *, struct vcons_screen *, int, long *);

/* accessops */
static int     newport_ioctl(void *, void *, u_long, void *, int,
    struct lwp *);
static paddr_t newport_mmap(void *, void *, off_t, int);

static struct wsdisplay_accessops newport_accessops = {
	.ioctl		= newport_ioctl,
	.mmap		= newport_mmap,
};

static struct wsscreen_descr newport_screen = {
	.name		= "default",
	.ncols		= 160,
	.nrows		= 64,
	.fontwidth	= 8,
	.fontheight	= 16,
	.capabilities	= WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_REVERSE
};

static const struct wsscreen_descr *_newport_screenlist[] = {
	&newport_screen
};

static const struct wsscreen_list newport_screenlist = {
	sizeof(_newport_screenlist) / sizeof(struct wsscreen_descr *),
	_newport_screenlist
};

static struct vcons_screen newport_console_screen;
static struct newport_devconfig newport_console_dc;
static int newport_is_console = 0;

uint8_t our_cmap[768];

/*
 * Return the monitor ID which hopefully matches the firmware selection.
 *
 * The firmware first checks to see if it's an unknown monitor
 * type.  If it is, then the prom ID can override it.
 * If it is a detected and known monitor then that is returned.
 *
 * Note that unknown monitor IDs for the SGI Indy is
 * 1024x768 60Hz, but for Indigo2 is 1280x1024 60Hz.
 *
 * Note that for boardrev < 4, the firmware seems to limit
 * the refresh rate to 72Hz, so don't allow 1280x1024 76Hz.
 */
static uint8_t
newport_get_monitor_id(struct newport_devconfig *dc)
{
	uint8_t id;

	id = dc->dc_monitor_cmap_id;
	KASSERT((id < 16));
	if (id > 15)
		id = 0;

	/*
	 * Unknown monitors on Indigo2 are 1280x1024 60Hz,
	 * on Indy is 1024x768 60Hz unless configured in PROM.
	 */
	if (newport_monitor_list[id].unknown == true) {
		if (mach_subtype == MACH_SGI_IP22_FULLHOUSE) {
			/* Default to 1280x1024 60Hz */
			id = 10;
		} else if (dc->dc_monitor_prom_id != -1)
			id = dc->dc_monitor_prom_id;
	}

	/*
	 * Check board revision, bump down to 72Hz if needed.
	 */
	if ((dc->dc_boardrev < 4) && (newport_monitor_list[id].x == 1280)
	    && (newport_monitor_list[id].hz > 72))
		/* 1280x1024, 72Hz */
		id = 9;

	/* Cap for sanity reasons */
	KASSERT((id < 16));
	if (id > 15)
		id = 0;

	return (id);
}

/*
 * Return the string representation of the monitor ID.
 *
 * This is the monitor ID that has already been populated
 * via the probe routines.
 */
static const char *
newport_get_monitor_str(struct newport_devconfig *dc)
{
	return (newport_monitor_list[newport_get_monitor_id(dc)].monitor);
}

/**** Low-level hardware register groveling functions ****/
static void
rex3_write(struct newport_devconfig *dc, bus_size_t rexreg, uint32_t val)
{
	bus_space_write_4(dc->dc_st, dc->dc_sh, NEWPORT_REX3_OFFSET + rexreg,
	    val);
//	bus_space_barrier(dc->dc_st, dc->dc_sh, NEWPORT_REX3_OFFSET + rexreg,
//	    4, BUS_SPACE_BARRIER_WRITE);
}

static void
rex3_write_go(struct newport_devconfig *dc, bus_size_t rexreg, uint32_t val)
{
	rex3_write(dc, rexreg + REX3_REG_GO, val);
}

static uint32_t
rex3_read(struct newport_devconfig *dc, bus_size_t rexreg)
{
//	bus_space_barrier(dc->dc_st, dc->dc_sh, NEWPORT_REX3_OFFSET + rexreg,
//	    4, BUS_SPACE_BARRIER_READ);
	return bus_space_read_4(dc->dc_st, dc->dc_sh, NEWPORT_REX3_OFFSET +
	    rexreg);
}

/*
 * Wait for the graphics FIFO and for it to be empty.
 *
 * This FIFO is used for submitted graphics register writes.
 */
static void
rex3_wait_gfifo(struct newport_devconfig *dc)
{
	while (rex3_read(dc, REX3_REG_STATUS) &
	    (REX3_STATUS_GFXBUSY | REX3_STATUS_PIPELEVEL_MASK))
		;
}

/*
 * Wait for the backend FIFO and for it to be empty.
 *
 * This FIFO is used for speaking to the data backend, notably
 * the DCB.
 */
static void
rex3_wait_bfifo(struct newport_devconfig *dc)
{
	while (rex3_read(dc, REX3_REG_STATUS) &
	    (REX3_STATUS_BACKBUSY | REX3_STATUS_BPIPELEVEL_MASK))
		;
}

static void
vc2_write_ireg(struct newport_devconfig *dc, uint8_t ireg, uint16_t val)
{
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_3 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_INDEX << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, (ireg << 24) | (val << 8));
}

static uint16_t
vc2_read_ireg(struct newport_devconfig *dc, uint8_t ireg)
{
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_1 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_INDEX << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, ireg << 24);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_IREG << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	return (uint16_t)(rex3_read(dc, REX3_REG_DCBDATA0) >> 16);
}

static uint16_t
vc2_read_ram(struct newport_devconfig *dc, uint16_t addr)
{
	vc2_write_ireg(dc, VC2_IREG_RAM_ADDRESS, addr);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_RAM << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	return (uint16_t)(rex3_read(dc, REX3_REG_DCBDATA0) >> 16);
}

#if 0
static void
vc2_write_ram(struct newport_devconfig *dc, uint16_t addr, uint16_t val)
{
	vc2_write_ireg(dc, VC2_IREG_RAM_ADDRESS, addr);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    (NEWPORT_DCBADDR_VC2 << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (VC2_DCBCRS_RAM << REX3_DCBMODE_DCBCRS_SHIFT) |
	    REX3_DCBMODE_ENASYNCACK |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, val << 16);
}
#endif

/*
 * Read from the given XMAP chip.
 *
 * Note this must not be used to read from NEWPORT_DCBADDR_XMAP_BOTH.
 */
static u_int32_t
xmap9_read(struct newport_devconfig *dc, int chip, int crs)
{

	KASSERT(chip != NEWPORT_DCBADDR_XMAP_BOTH);
	if (chip == NEWPORT_DCBADDR_XMAP_BOTH)
		chip = NEWPORT_DCBADDR_XMAP_0;

	rex3_write(dc, REX3_REG_DCBMODE,
		REX3_DCBMODE_DW_1 |
		(chip << REX3_DCBMODE_DCBADDR_SHIFT) |
		(crs << REX3_DCBMODE_DCBCRS_SHIFT) |
		(3 << REX3_DCBMODE_CSWIDTH_SHIFT) |
		(1 << REX3_DCBMODE_CSHOLD_SHIFT) |
		(2 << REX3_DCBMODE_CSSETUP_SHIFT));
	return (uint8_t) (rex3_read(dc, REX3_REG_DCBDATA0) >> 24);
}

/*
 * write to the given XMAP chip, or to both.
 */
static void
xmap9_write(struct newport_devconfig *dc, int chip, int crs, uint8_t val)
{
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_1 |
	    (chip << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (crs << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (0 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (2 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, val << 24);
}

/*
 * Wait for the given XMAP9 mode FIFO to be ready.
 *
 * Since we're not trying to optimise for bursting mode writes,
 * just wait for /a/ FIFO entry to be avaliable.
 *
 * The XMAP9 documentation covers the gray mode encoding
 * of this register in case there's a future need for
 * bursting mode updates without individual FIFO checks.
 */
static inline void
xmap9_wait_chip(struct newport_devconfig *dc, int chip)
{
	while (xmap9_read(dc, chip, XMAP9_DCBCRS_FIFOAVAIL) == 0)
		;
}

/*
 * Wait for the backend FIFO and then both XMAP FIFOs to become available.
 */
static inline void
xmap9_wait(struct newport_devconfig *dc)
{
	rex3_wait_bfifo(dc);

	xmap9_wait_chip(dc, NEWPORT_DCBADDR_XMAP_0);
	xmap9_wait_chip(dc, NEWPORT_DCBADDR_XMAP_1);
}

#if 0
static uint32_t
xmap9_read_mode(struct newport_devconfig *dc, int chip, uint8_t idx)
{
	uint32_t mode = 0, val;
	int i;

	xmap9_wait_chip(dc, chip);

	for (i = 0; i < 4; i++) {
		xmap9_write(dc, chip, XMAP9_DCBCRS_MODE_SELECT,
		    (idx << 2) | i);
		val = xmap9_read(dc, chip, XMAP9_DCBCRS_MODE_SETUP);
		mode |= (val << (i * 8));
	}
	return (mode);
}
#endif

/*
 * Map the pixel clock frequency to which parameters to use for XMAP9
 * mode writes.
 */
static const struct newport_dcb_cs_params *
newport_hw_get_mode_cs_params(int cfreq)
{
	if (cfreq > 119)
		return &newport_dcb_cs_fast;
	if (cfreq > 59)
		return &newport_dcb_cs_slow;
	return &newport_dcb_cs_wslow;
}

/*
 * Write out the 32 bit mode entry to both XMAP9 chips.
 *
 * This is actually a fun clock domain crossing problem - the
 * XMAP9 isn't signaling an ACK back to the REX3 chip, so
 * the CS setup, width and hold times need to be calculated
 * based on 33MHz GIO clock (REX3) <-> the currently configured
 * pixel clock (XMAP9).
 *
 * The pixel clock is set by the firmware based on the attached
 * monitor; it programs the BT445 RAMDAC to generate a pixelclock
 * that's then divided in half and sent in both phases to the
 * various chips doing odd/even pixel handling (which includes the
 * XMAP9.)
 *
 * Unfortunately we don't have the pixel clock available to us - only
 * the monitor resolution from the VC2 table - so eventually the
 * driver will need to grow a way to read the monitor sense lines and
 * mirror what the firmware is doing.
 *
 * Also note on SGI Indy the PROM recognises "setenv monitor" to
 * force a 1280x1024x60Hz monitor (setenv monitor H) and
 * a 1280x1024*76Hz monitor (setenv monitor S).  To correctly handle
 * that we will ALSO need to parse the PROM environment and make
 * it available here.
 */
static void
xmap9_write_mode(struct newport_devconfig *dc, uint8_t index, uint32_t mode)
{
	const struct newport_dcb_cs_params *cs;
	uint8_t id;

	/* Fetch the monitor ID and then the CS parameters to use */
	id = newport_get_monitor_id(dc);
	cs = newport_hw_get_mode_cs_params(newport_monitor_list[id].cfreq);

	/* wait for FIFO if needed */
	xmap9_wait(dc);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_4 |
	    (NEWPORT_DCBADDR_XMAP_BOTH << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (XMAP9_DCBCRS_MODE_SETUP << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (cs->cs_width << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (cs->cs_hold << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (cs->cs_setup << REX3_DCBMODE_CSSETUP_SHIFT)
	);
	rex3_write(dc, REX3_REG_DCBDATA0, (index << 24) | (mode & 0xffffff));
}

/**** Helper functions ****/
static void
newport_fill_rectangle(struct newport_devconfig *dc, int x1, int y1, int wi,
    int he, uint32_t color)
{
	int x2 = x1 + wi - 1;
	int y2 = y1 + he - 1;

	rex3_wait_gfifo(dc);
	
	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_DRAW |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_DOSETUP |
	    REX3_DRAWMODE0_STOPONX | REX3_DRAWMODE0_STOPONY);
	rex3_write(dc, REX3_REG_CLIPMODE, 0x1e00);
	rex3_write(dc, REX3_REG_DRAWMODE1,
	    REX3_DRAWMODE1_PLANES_RGB |
	    REX3_DRAWMODE1_DD_DD8 |
	    REX3_DRAWMODE1_RWPACKED |
	    REX3_DRAWMODE1_HD_HD8 |
	    REX3_DRAWMODE1_COMPARE_LT |
	    REX3_DRAWMODE1_COMPARE_EQ |
	    REX3_DRAWMODE1_COMPARE_GT |
	    REX3_DRAWMODE1_RGBMODE |
	    REX3_DRAWMODE1_FASTCLEAR |
	    REX3_DRAWMODE1_LO_SRC);
	rex3_write(dc, REX3_REG_WRMASK, 0xffffffff);
	rex3_write(dc, REX3_REG_COLORVRAM, color);
	rex3_write(dc, REX3_REG_XYSTARTI, (x1 << REX3_XYSTARTI_XSHIFT) | y1);

	rex3_write_go(dc, REX3_REG_XYENDI, (x2 << REX3_XYENDI_XSHIFT) | y2);
}

static void
newport_bitblt(struct newport_devconfig *dc, int xs, int ys, int xd,
    int yd, int wi, int he, int rop)
{
	int xe, ye;
	uint32_t tmp;

	rex3_wait_gfifo(dc);
	if (yd > ys) {
		/* need to copy bottom up */
		ye = ys;
		yd += he - 1;
		ys += he - 1;
	} else
		ye = ys + he - 1;

	if (xd > xs) {
		/* need to copy right to left */
		xe = xs;
		xd += wi - 1;
		xs += wi - 1;
	} else
		xe = xs + wi - 1;

	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_SCR2SCR |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_DOSETUP |
	    REX3_DRAWMODE0_STOPONX | REX3_DRAWMODE0_STOPONY);
	rex3_write(dc, REX3_REG_DRAWMODE1,
	    REX3_DRAWMODE1_PLANES_CI |
	    REX3_DRAWMODE1_DD_DD8 |
	    REX3_DRAWMODE1_RWPACKED |
	    REX3_DRAWMODE1_HD_HD8 |
	    REX3_DRAWMODE1_COMPARE_LT |
	    REX3_DRAWMODE1_COMPARE_EQ |
	    REX3_DRAWMODE1_COMPARE_GT |
	    ((rop << 28) & REX3_DRAWMODE1_LOGICOP_MASK));
	rex3_write(dc, REX3_REG_XYSTARTI, (xs << REX3_XYSTARTI_XSHIFT) | ys);
	rex3_write(dc, REX3_REG_XYENDI, (xe << REX3_XYENDI_XSHIFT) | ye);

	tmp = (yd - ys) & 0xffff;
	tmp |= (xd - xs) << REX3_XYMOVE_XSHIFT;

	rex3_write_go(dc, REX3_REG_XYMOVE, tmp);
}

static void
newport_cmap_setrgb(struct newport_devconfig *dc, int index, uint8_t r,
    uint8_t g, uint8_t b)
{
	rex3_wait_bfifo(dc);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_2 |
	    REX3_DCBMODE_ENCRSINC |
	    (NEWPORT_DCBADDR_CMAP_BOTH << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (CMAP_DCBCRS_ADDRESS_LOW << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (1 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT) |
	    REX3_DCBMODE_SWAPENDIAN);

	rex3_write(dc, REX3_REG_DCBDATA0, index << 16);

	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_3 |
	    (NEWPORT_DCBADDR_CMAP_BOTH << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (CMAP_DCBCRS_PALETTE << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (1 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));

	rex3_write(dc, REX3_REG_DCBDATA0, (r << 24) + (g << 16) + (b << 8));
}

static void
newport_get_resolution(struct newport_devconfig *dc)
{
	uint16_t vep,lines;
	uint16_t linep,cols;
	uint16_t data;

	vep = vc2_read_ireg(dc, VC2_IREG_VIDEO_ENTRY);

	dc->dc_xres = 0;
	dc->dc_yres = 0;

	for (;;) {
		/* Iterate over runs in video timing table */

		cols = 0;

		linep = vc2_read_ram(dc, vep++);
		lines = vc2_read_ram(dc, vep++);

		if (lines == 0)
			break;

		do {
			/* Iterate over state runs in line sequence table */
		
			data = vc2_read_ram(dc, linep++);

			if ((data & 0x0001) == 0)
				cols += (data >> 7) & 0xfe;

			if ((data & 0x0080) == 0)
				data = vc2_read_ram(dc, linep++);
		} while ((data & 0x8000) == 0);

		if (cols != 0) {
			if (cols > dc->dc_xres)
				dc->dc_xres = cols;

			dc->dc_yres += lines;
		}
	}
}

/*
 * Read from an 8 bit CMAP register.
 *
 * This must only be used on individual CMAPs (0 or 1), not both.
 */
static uint8_t
cmap_reg_read(struct newport_devconfig *dc, uint8_t id, uint8_t reg)
{
	KASSERT(id != NEWPORT_DCBADDR_CMAP_BOTH);
	if (id == NEWPORT_DCBADDR_CMAP_BOTH)
		id = NEWPORT_DCBADDR_CMAP_0;

	/* Get various revisions */
	rex3_write(dc, REX3_REG_DCBMODE,
	    REX3_DCBMODE_DW_1 |
	    (id << REX3_DCBMODE_DCBADDR_SHIFT) |
	    (reg << REX3_DCBMODE_DCBCRS_SHIFT) |
	    (1 << REX3_DCBMODE_CSWIDTH_SHIFT) |
	    (1 << REX3_DCBMODE_CSHOLD_SHIFT) |
	    (1 << REX3_DCBMODE_CSSETUP_SHIFT));
	return (uint8_t)(rex3_read(dc, REX3_REG_DCBDATA0) >> 24);
}

static void
newport_probe_monitor(struct newport_devconfig *dc)
{
	const char *m;
	uint8_t scratch;

	/*
	 * CMAP1 - the 4 monitor sense bits are on bits 7:4.
	 *
	 * Note that for MACH_SGI_IP22_GUINNESS (Indy), the monitor PROM
	 * variable can override a 'default' monitor setting of 1024x768
	 * with two others - H = 1280x1024x60Hz and S = 1280x1024x76Hz.
	 * So when (eventually) doing a monitor ID lookup we'll also
	 * need to handle that.
	 */
	scratch = cmap_reg_read(dc, NEWPORT_DCBADDR_CMAP_1,
	    CMAP_DCBCRS_REVISION);
	aprint_debug("%s: CMAP_1 REVISION 0x%02x\n", __func__, scratch);
	dc->dc_monitor_cmap_id = (scratch >> 4) & 0x0f;
	dc->dc_monitor_prom_id = -1;

	/* Only check the PROM monitor on SGI Indy */
	if (mach_subtype != MACH_SGI_IP22_GUINNESS)
		return;

	m = arcbios_GetEnvironmentVariable("monitor");
	if (m == NULL)
		return;
	else if (m[0] == '0')
		return;

	/*
	 * H = 1280x1024, 60Hz
	 * S = 1280x1024, 76Hz
	 */
	if (m[0] == 'h' || m[0] == 'H')
		dc->dc_monitor_prom_id = 10;
	else if (m[0] == 'S')
		dc->dc_monitor_prom_id = 1;
}

/*
 * Probe the hardware as handed to us by the boot firmware
 * before it's potentially fiddled with by the console and
 * X11 servers.
 */
static void
newport_probe_hw(struct newport_devconfig *dc)
{
	uint32_t scratch;

	/* Get various revisions */

	/*
	 * CMAP0 - CMAP revision bits 3:0, board revision bits 6:4,
	 * If boardrev > 1 then the b7 == 1 signals an 8 bit framebuffer.
	 */
	rex3_wait_bfifo(dc);
	scratch = cmap_reg_read(dc, NEWPORT_DCBADDR_CMAP_0,
	    CMAP_DCBCRS_REVISION);
	aprint_debug("%s: CMAP_0 REVISION 0x%02x\n", __func__, scratch);

	dc->dc_boardrev = (scratch >> 4) & 0x07;
	dc->dc_cmaprev = scratch & 0x07;

	rex3_wait_bfifo(dc);
	dc->dc_xmaprev = xmap9_read(dc, NEWPORT_DCBADDR_XMAP_0,
	    XMAP9_DCBCRS_REVISION) & 0x07;
	dc->dc_depth = ( (dc->dc_boardrev > 1) && (scratch & 0x80)) ? 8 : 24;

	aprint_debug("%s: XMAP_0 REVISION: 0x%08x\n", __func__,
	    xmap9_read(dc, NEWPORT_DCBADDR_XMAP_0, XMAP9_DCBCRS_REVISION));

	scratch = vc2_read_ireg(dc, VC2_IREG_CONFIG);
	dc->dc_vc2rev = (scratch & VC2_IREG_CONFIG_REVISION) >> 5;
	aprint_debug("%s: VC2_IREG_CONFIG config: 0x%04x\n", __func__, scratch);
}

/*
 * Adjust the XMAP configuration based on the earlier probed board type.
 *
 * This isn't the actual operating mode; this is instead the underlying
 * hardware type.
 */
static void
newport_hw_adjust_xmap_cfg(struct newport_devconfig *dc, uint8_t *dcbcfg)
{

	/* Configure 8 or 24 bit hardware */
	if (dc->dc_depth == 8)
		*dcbcfg |= XMAP9_CONFIG_8BIT_SYSTEM;
	else
		*dcbcfg &= ~XMAP9_CONFIG_8BIT_SYSTEM;

	/* XXX TODO: we aren't using PUP, so disable it? */

	/*
	 * Note: we're leaving the event/odd, fast/slow pclk,
	 * video option board config and colourmap alone.
	 */
}

static void
newport_setup_hw_ci_cmap(struct newport_devconfig *dc)
{
	int i;
	uint8_t ctmp;

	for (i = 0; i < 256; i++) {
		ctmp = i & 0xe0;
		/*
		 * replicate bits so 0xe0 maps to a red value of 0xff
		 * in order to make white look actually white
		 */
		ctmp |= (ctmp >> 3) | (ctmp >> 6);
		our_cmap[i * 3] = ctmp;

		ctmp = (i & 0x1c) << 3;
		ctmp |= (ctmp >> 3) | (ctmp >> 6);
		our_cmap[i * 3 + 1] = ctmp;

		ctmp = (i & 0x03) << 6;
		ctmp |= ctmp >> 2;
		ctmp |= ctmp >> 4;
		our_cmap[i * 3 + 2] = ctmp;

		newport_cmap_setrgb(dc, i, our_cmap[i * 3],
		    our_cmap[i * 3 + 1], our_cmap[i * 3 + 2]);
	}
}

static void
newport_setup_hw_rgb2_cmap(struct newport_devconfig *dc)
{
	int i;

	for (i = 0; i < 256; i++)
		newport_cmap_setrgb(dc, 0x1f00 + i, i, i, i);
}

/*
 * Setup the XMAP9 mode registers with the given mode.
 *
 * This tells the two XMAP9s (one even column, one odd column)
 * how to interpret the pixel data being streamed in from framebuffer
 * memory and what config bits to expose to the CMAP hardware and
 * the DAC as it's converted into final RGB signals for display.
 *
 * Each entry in the mode table is a DID (display ID) in the VC2
 * chip, and the VC2 chip will shift out a DID value to pair with
 * the framebuffer memory contents being fed into the XMAP9s.
 * Since we're not currently filling the VC2 DID table with values,
 * just program them all in here with the same configuration.
 */
static void
newport_setup_hw_xmap9_modes(struct newport_devconfig *dc,
    uint32_t mode_mask)
{
	int i;

	for (i = 0; i < 32; i++) {
		xmap9_write_mode(dc, i, mode_mask);
	}
	rex3_wait_bfifo(dc);
	xmap9_write(dc, NEWPORT_DCBADDR_XMAP_BOTH,
	    XMAP9_DCBCRS_MODE_SELECT, 0);
}

static void
newport_setup_hw(struct newport_devconfig *dc, int depth)
{
	uint16_t __unused(curp), tmp;
	uint8_t dcbcfg;

	/* Setup cursor glyph */
	curp = vc2_read_ireg(dc, VC2_IREG_CURSOR_ENTRY);

	/* Setup VC2 to a known state */
	tmp = vc2_read_ireg(dc, VC2_IREG_CONTROL) & VC2_CONTROL_INTERLACE;
	vc2_write_ireg(dc, VC2_IREG_CONTROL, tmp |
	    VC2_CONTROL_DISPLAY_ENABLE |
	    VC2_CONTROL_VTIMING_ENABLE |
	    VC2_CONTROL_DID_ENABLE |
	    VC2_CONTROL_CURSORFUNC_ENABLE /*|
	    VC2_CONTROL_CURSOR_ENABLE*/);

	/* disable all clipping */
	rex3_write(dc, REX3_REG_CLIPMODE, 0x1e00);

	/* Setup XMAP9s */
	rex3_wait_bfifo(dc);
	xmap9_write(dc, NEWPORT_DCBADDR_XMAP_BOTH, XMAP9_DCBCRS_CURSOR_CMAP,
	    0);

	/*
	 * Always configure the 8 bit or 24 bit configuration regardless
	 * of the desired bit depth.  The XMAP9 documentation mentions this
	 * does have some behaviour changes regardless of the pixel formats
	 * being used.
	 */
	rex3_wait_bfifo(dc);
	dcbcfg = xmap9_read(dc, NEWPORT_DCBADDR_XMAP_0, XMAP9_DCBCRS_CONFIG);
	newport_hw_adjust_xmap_cfg(dc, &dcbcfg);
	rex3_wait_bfifo(dc);
	xmap9_write(dc, NEWPORT_DCBADDR_XMAP_0, XMAP9_DCBCRS_CONFIG, dcbcfg);
	aprint_debug("%s: XMAP_0 config: 0x%02x\n", __func__, dcbcfg);

	dcbcfg = xmap9_read(dc, NEWPORT_DCBADDR_XMAP_1, XMAP9_DCBCRS_CONFIG);
	newport_hw_adjust_xmap_cfg(dc, &dcbcfg);
	rex3_wait_bfifo(dc);
	xmap9_write(dc, NEWPORT_DCBADDR_XMAP_1, XMAP9_DCBCRS_CONFIG, dcbcfg);
	aprint_debug("%s: XMAP_1 config: 0x%02x\n", __func__, dcbcfg);

	if (depth == 8) {
		/*
		 * Configure an 8 bit RGB colour map that uses the netbsd
		 * packed RGB 332 format.  The rendering routines use these
		 * values from the raster map attribute list.
		 */
		newport_setup_hw_xmap9_modes(dc, XMAP9_MODE_GAMMA_BYPASS |
		    XMAP9_MODE_PIXSIZE_8BPP | XMAP9_MODE_PIXMODE_CI);
	} else {
		/*
		 * Configure the hardware to use the a 24 bit RGB table at
		 * RGB2 in CMAP.
		 */
		newport_setup_hw_xmap9_modes(dc, XMAP9_MODE_GAMMA_BYPASS |
		    XMAP9_MODE_PIXSIZE_24BPP | XMAP9_MODE_PIXMODE_RGB2);
	}

	/* Setup REX3 */
	rex3_write(dc, REX3_REG_XYWIN, (4096 << 16) | 4096);
	rex3_write(dc, REX3_REG_TOPSCAN, 0x3ff); /* XXX Why? XXX */

	/*
	 * Setup CMAP CI table 0 for an RGB 332 packing.
	 *
	 * This will be used by 8 bit RGB pixel modes so we can use
	 * RGB 332 and not have to teach the NetBSD console code or
	 * this driver to map RGB 332 -> the Newport interlaced RGB8 format.
	 */
	newport_setup_hw_ci_cmap(dc);

	/*
	 * Write a ramp into RGB2 CMAP.
	 *
	 * This won't be used in 8 bit console mode as that is using
	 * CI pixels, not RGB8 pixels.  For 8 bit framebuffer mode it'll
	 * either use the RGB332 table programmed into CMAP CI table 0
	 * or X11 will allocate private colourmaps as needed.
	 */
	newport_setup_hw_rgb2_cmap(dc);
}

/**** Attach routines ****/
static int
newport_match(device_t parent, struct cfdata *self, void *aux)
{
	struct gio_attach_args *ga = aux;

	/* newport doesn't decode all addresses */
	if (ga->ga_addr != 0x1f000000 && ga->ga_addr != 0x1f400000 &&
	    ga->ga_addr != 0x1f800000 && ga->ga_addr != 0x1fc00000)
		return 0;

	/* Don't do the destructive probe if we're already attached */
	if (newport_is_console && ga->ga_addr == newport_console_dc.dc_addr)
		return 1;

	if (platform.badaddr(
	    (void *)MIPS_PHYS_TO_KSEG1(ga->ga_addr + NEWPORT_REX3_OFFSET + REX3_REG_XSTARTI),
	    sizeof(uint32_t)))
		return 0;
	if (platform.badaddr(
	    (void *)MIPS_PHYS_TO_KSEG1(ga->ga_addr + NEWPORT_REX3_OFFSET + REX3_REG_XSTART),
	    sizeof(uint32_t)))
		return 0;

	/* Ugly, this probe is destructive, blame SGI... */
	/* XXX Should be bus_space_peek/bus_space_poke XXX */
	bus_space_write_4(ga->ga_iot, ga->ga_ioh,
	    NEWPORT_REX3_OFFSET + REX3_REG_XSTARTI, 0x12345678);
	if (bus_space_read_4(ga->ga_iot, ga->ga_ioh,
	      NEWPORT_REX3_OFFSET + REX3_REG_XSTART)
	    != ((0x12345678 & 0xffff) << 11))
		return 0;
	
	return 1;
}

static void
newport_attach_common(struct newport_devconfig *dc, struct gio_attach_args *ga)
{
	dc->dc_addr = ga->ga_addr;

	dc->dc_st = ga->ga_iot;
	dc->dc_sh = ga->ga_ioh;

	newport_probe_hw(dc);
	newport_probe_monitor(dc);
	newport_setup_hw(dc, 8);

	newport_get_resolution(dc);

	newport_fill_rectangle(dc, 0, 0, dc->dc_xres, dc->dc_yres, 0);

#ifdef NEWPORT_DEBUG
	int i;
	for (i = 0; i < 256; i++) 
		newport_fill_rectangle(dc, 10, i * 3 + 10, 500, 2, i);
	delay(10000000);
#endif
	dc->dc_screen = &newport_screen;
	
	dc->dc_mode = WSDISPLAYIO_MODE_EMUL;
}

static void
newport_attach(device_t parent, device_t self, void *aux)
{
	struct gio_attach_args *ga = aux;
	struct newport_softc *sc = device_private(self);
	struct wsemuldisplaydev_attach_args wa;
	unsigned long defattr;

	sc->sc_dev = self;
	if (newport_is_console && ga->ga_addr == newport_console_dc.dc_addr) {
		wa.console = 1;
		sc->sc_dc = &newport_console_dc;
	} else {
		wa.console = 0;
		sc->sc_dc = kmem_zalloc(sizeof(struct newport_devconfig),
		    KM_SLEEP);

		newport_attach_common(sc->sc_dc, ga);
	}

	aprint_naive(": Display adapter\n");

	aprint_normal(": SGI NG1 (board revision %d, cmap revision %d, "
	    "xmap revision %d, vc2 revision %d), depth %d, monitor ID %d "
	    "(%s)\n",
	    sc->sc_dc->dc_boardrev, sc->sc_dc->dc_cmaprev,
	    sc->sc_dc->dc_xmaprev, sc->sc_dc->dc_vc2rev, sc->sc_dc->dc_depth,
	    newport_get_monitor_id(sc->sc_dc),
	    newport_get_monitor_str(sc->sc_dc));
	aprint_normal("%s: 13W3 Monitor ID %d, PROM ID %d, determined "
	    "monitor ID %d (%s)\n",
	    device_xname(self),
	    sc->sc_dc->dc_monitor_cmap_id,
	    sc->sc_dc->dc_monitor_prom_id,
	    newport_get_monitor_id(sc->sc_dc),
	    newport_get_monitor_str(sc->sc_dc));

	vcons_init(&sc->sc_dc->dc_vd, sc->sc_dc, sc->sc_dc->dc_screen,
	    &newport_accessops);
	sc->sc_dc->dc_vd.init_screen = newport_init_screen;
	if (newport_is_console) {
		newport_console_screen.scr_flags |= VCONS_SCREEN_IS_STATIC;
		vcons_init_screen(&sc->sc_dc->dc_vd, &newport_console_screen,
		    1, &defattr);
		sc->sc_dc->dc_screen->textops =
		    &newport_console_screen.scr_ri.ri_ops;
		vcons_replay_msgbuf(&newport_console_screen);
	}
	wa.scrdata = &newport_screenlist;
	wa.accessops = &newport_accessops;
	wa.accesscookie = &sc->sc_dc->dc_vd;

	config_found(sc->sc_dev, &wa, wsemuldisplaydevprint, CFARGS_NONE);
}

int
newport_cnattach(struct gio_attach_args *ga)
{
	struct rasops_info *ri = &newport_console_screen.scr_ri;
	long defattr = 0x000f0000;

	if (!newport_match(NULL, NULL, ga)) {
		return ENXIO;
	}

	newport_attach_common(&newport_console_dc, ga);

	ri->ri_hw = &newport_console_screen;
	ri->ri_depth = newport_console_dc.dc_depth;
	ri->ri_width = newport_console_dc.dc_xres;
	ri->ri_height = newport_console_dc.dc_yres;
	ri->ri_stride = newport_console_dc.dc_xres; /* XXX */
	ri->ri_flg = RI_CENTER | RI_NO_AUTO | RI_FULLCLEAR | RI_8BIT_IS_RGB;
	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, ri->ri_height / ri->ri_font->fontheight,
	    ri->ri_width / ri->ri_font->fontwidth);
	ri->ri_ops.copyrows  = newport_copyrows;
	ri->ri_ops.eraserows = newport_eraserows;
	ri->ri_ops.copycols  = newport_copycols;
	ri->ri_ops.erasecols = newport_erasecols;
	ri->ri_ops.cursor    = newport_cursor_dummy;
	ri->ri_ops.putchar   = newport_putchar;
	newport_screen.textops = &ri->ri_ops;
	newport_screen.ncols = ri->ri_cols;
	newport_screen.nrows = ri->ri_rows;
	newport_console_screen.scr_cookie = &newport_console_dc;

	wsdisplay_cnattach(&newport_screen, ri, 0, 0, defattr);
	newport_is_console = 1;

	return 0;
}

static void
newport_init_screen(void *cookie, struct vcons_screen *scr,
    int existing, long *defattr)
{
	struct newport_devconfig *dc = cookie;
	struct rasops_info *ri = &scr->scr_ri;

	ri->ri_depth = 8;
	ri->ri_width = dc->dc_xres;
	ri->ri_height = dc->dc_yres;
	ri->ri_stride = dc->dc_xres; /* XXX */
	ri->ri_flg = RI_CENTER | RI_FULLCLEAR | RI_8BIT_IS_RGB;

	rasops_init(ri, 0, 0);
	ri->ri_caps = WSSCREEN_WSCOLORS;

	rasops_reconfig(ri, dc->dc_yres / ri->ri_font->fontheight,
		    dc->dc_xres / ri->ri_font->fontwidth);

	ri->ri_hw = scr;
	ri->ri_ops.copyrows  = newport_copyrows;
	ri->ri_ops.eraserows = newport_eraserows;
	ri->ri_ops.copycols  = newport_copycols;
	ri->ri_ops.erasecols = newport_erasecols;
	ri->ri_ops.cursor    = newport_cursor;
	ri->ri_ops.putchar   = newport_putchar;
}

/**** wsdisplay textops ****/
static void
newport_cursor_dummy(void *c, int on, int row, int col)
{
}

static void
newport_cursor(void *c, int on, int row, int col)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	int x, y, wi,he;

	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;

	if (ri->ri_flg & RI_CURSOR) {
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		newport_bitblt(dc, x, y, x, y, wi, he, 12);
		ri->ri_flg &= ~RI_CURSOR;
	}

	ri->ri_crow = row;
	ri->ri_ccol = col;

	if (on)
	{
		x = ri->ri_ccol * wi + ri->ri_xorigin;
		y = ri->ri_crow * he + ri->ri_yorigin;
		newport_bitblt(dc, x, y, x, y, wi, he, 12);
		ri->ri_flg |= RI_CURSOR;
	}
}

static void
newport_putchar(void *c, int row, int col, u_int ch, long attr)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	struct wsdisplay_font *font = PICK_FONT(ri, ch);
	uint8_t *bitmap = (u_int8_t *)font->data + (ch - font->firstchar) * 
	    font->fontheight * font->stride;
	uint32_t pattern;
	int i;
	int x = col * font->fontwidth + ri->ri_xorigin;
	int y = row * font->fontheight + ri->ri_yorigin;

	rex3_wait_gfifo(dc);
	
	rex3_write(dc, REX3_REG_DRAWMODE0, REX3_DRAWMODE0_OPCODE_DRAW |
	    REX3_DRAWMODE0_ADRMODE_BLOCK | REX3_DRAWMODE0_STOPONX |
	    REX3_DRAWMODE0_ENZPATTERN | REX3_DRAWMODE0_ZPOPAQUE);

	rex3_write(dc, REX3_REG_DRAWMODE1,
	    REX3_DRAWMODE1_PLANES_CI |
	    REX3_DRAWMODE1_DD_DD8 |
	    REX3_DRAWMODE1_RWPACKED |
	    REX3_DRAWMODE1_HD_HD8 |
	    REX3_DRAWMODE1_COMPARE_LT |
	    REX3_DRAWMODE1_COMPARE_EQ |
	    REX3_DRAWMODE1_COMPARE_GT |
	    /*REX3_DRAWMODE1_RGBMODE |*/
	    REX3_DRAWMODE1_LO_SRC);

	rex3_write(dc, REX3_REG_XYSTARTI, (x << REX3_XYSTARTI_XSHIFT) | y);
	rex3_write(dc, REX3_REG_XYENDI,
	    (x + font->fontwidth - 1) << REX3_XYENDI_XSHIFT);

	rex3_write(dc, REX3_REG_COLORI, ri->ri_devcmap[(attr >> 24) & 0xf] & 0xff);
	rex3_write(dc, REX3_REG_COLORBACK, ri->ri_devcmap[(attr >> 16) & 0xf] & 0xff);

	rex3_write(dc, REX3_REG_WRMASK, 0xff);

	if (font->stride == 1) {
		for (i = 0; i < font->fontheight; i++) {
			pattern = *bitmap << 24;

			rex3_write_go(dc, REX3_REG_ZPATTERN, pattern);

			bitmap++;
		}
	} else {
		uint16_t *bitmap16 = (uint16_t *)bitmap;
		for (i = 0; i < font->fontheight; i++) {
			pattern = *bitmap16 << 16;

			rex3_write_go(dc, REX3_REG_ZPATTERN, pattern);

			bitmap16++;
		}
		
	}
	rex3_wait_gfifo(dc);
}

static void
newport_copycols(void *c, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	int32_t xs, xd, y, width, height;

	xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
	xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
	y = ri->ri_yorigin + ri->ri_font->fontheight * row;
	width = ri->ri_font->fontwidth * ncols;
	height = ri->ri_font->fontheight;
	newport_bitblt(dc, xs, y, xd, y, width, height, 3);
}

static void
newport_erasecols(void *c, int row, int startcol, int ncols,
    long attr)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	struct wsdisplay_font *font = ri->ri_font;

	newport_fill_rectangle(dc,
	    startcol * font->fontwidth + ri->ri_xorigin,	/* x1 */
	    row * font->fontheight + ri->ri_yorigin,		/* y1 */
	    ncols * font->fontwidth,				/* wi */
	    font->fontheight,					/* he */
	    ri->ri_devcmap[(attr >> 16) & 0xf]);
}

static void
newport_copyrows(void *c, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	int32_t x, ys, yd, width, height;

	x = ri->ri_xorigin;
	ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
	yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
	width = ri->ri_emuwidth;
	height = ri->ri_font->fontheight * nrows;

	newport_bitblt(dc, x, ys, x, yd, width, height, 3);
}

static void
newport_eraserows(void *c, int startrow, int nrows, long attr)
{
	struct rasops_info *ri = c;
	struct vcons_screen *scr = ri->ri_hw;
	struct newport_devconfig *dc = scr->scr_cookie;
	struct wsdisplay_font *font = ri->ri_font;

	if (startrow == 0 && nrows == ri->ri_rows) {
		newport_fill_rectangle(dc,
		    0,							/* x1 */
		    0,							/* y1 */
		    dc->dc_xres,					/* wi */
		    dc->dc_yres,					/* he */
		    ri->ri_devcmap[(attr >> 16) & 0xf]);
	} else {
		newport_fill_rectangle(dc,
		    0,							/* x1 */
		    startrow * font->fontheight + ri->ri_yorigin,	/* y1 */
		    dc->dc_xres,					/* wi */
		    nrows * font->fontheight,				/* he */
		    ri->ri_devcmap[(attr >> 16) & 0xf]);
	}
}

/**** wsdisplay accessops ****/

static int
newport_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
	struct lwp *l)
{
	struct vcons_data *vd;
	struct newport_devconfig *dc;
	struct vcons_screen *__unused(ms);
	int nmode;

	vd = (struct vcons_data *)v;
	dc = (struct newport_devconfig *)vd->cookie;
	ms = (struct vcons_screen *)vd->active;

#define FBINFO (*(struct wsdisplay_fbinfo*)data)

	switch (cmd) {
	case WSDISPLAYIO_GINFO:
		FBINFO.width  = dc->dc_xres;
		FBINFO.height = dc->dc_yres;
		FBINFO.depth  = dc->dc_depth;
		FBINFO.cmsize = 256;
		return 0;
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_NEWPORT;
		return 0;
	case WSDISPLAYIO_SMODE:
		nmode = *(int *)data;
		if (nmode != dc->dc_mode) {
			dc->dc_mode = nmode;
			if (nmode == WSDISPLAYIO_MODE_EMUL) {
				rex3_wait_gfifo(dc);
				newport_setup_hw(dc, 8);
				vcons_redraw_screen(vd->active);
			} else {
				rex3_wait_gfifo(dc);
				newport_setup_hw(dc, dc->dc_depth);
			}
		}
		return 0;
	}
	return EPASSTHROUGH;
}

static paddr_t
newport_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct vcons_data *vd;
	struct newport_devconfig *dc;

	vd = (struct vcons_data *)v;
	dc = (struct newport_devconfig *)vd->cookie;

	if ( offset >= 0xfffff)
		return -1;

	return bus_space_mmap(dc->dc_st, dc->dc_addr, offset, prot, 0);
}
