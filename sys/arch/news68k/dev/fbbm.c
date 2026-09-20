/*	$NetBSD: fbbm.c,v 1.2 2026/09/20 11:52:56 tsutsui Exp $	*/

/*-
 * Copyright (c) 2026 Izumi Tsutsui.  All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fbbm.c,v 1.2 2026/09/20 11:52:56 tsutsui Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/kmem.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <news68k/dev/hbvar.h>
#include <news68k/dev/nwb225reg.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

struct fbbm_devconfig {
	bus_space_tag_t dc_bst;
	bus_space_handle_t dc_rcont_bsh;
	bus_space_handle_t dc_krom_bsh;

	int dc_nplanes;
	uint32_t dc_planemask;
	int dc_dispver;

	int dc_fbwidth;
	int dc_fbheight;
	int dc_width;
	int dc_height;

	int dc_fontwidth;
	int dc_fontheight;
	int dc_cellwidth;
	int dc_cellheight;

	int dc_ncols;
	int dc_nrows;
	int dc_xorigin;
	int dc_yorigin;

	int dc_cursoron;
	int dc_cursorrow;
	int dc_cursorcol;

	struct wsscreen_descr dc_wsscrdescr;
};

struct fbbm_softc {
	device_t sc_dev;
	struct fbbm_devconfig *sc_dc;
	int sc_nscreens;
	int sc_wsmode;
	const struct wsscreen_descr *sc_scrdescs[1];
	struct wsscreen_list sc_scrlist;
};

/* autoconf functions and helpers */
static int fbbm_match(device_t, cfdata_t, void *);
static void fbbm_attach(device_t, device_t, void *);

static bool fbbm_console_initialized(void);
static int fbbm_probe(bus_space_tag_t, bus_addr_t);

/* bus_space mappings */
static int nwb225_map(struct fbbm_devconfig *, bus_space_tag_t, bus_addr_t);
static void nwb225_unmap(struct fbbm_devconfig *);

/* ROP helper functions */
static void nwb225_rop_reset(struct fbbm_devconfig *);
static int nwb225_rop_wait(struct fbbm_devconfig *);
static void nwb225_ctl_write(struct fbbm_devconfig *, uint16_t, uint16_t);
static void nwb225_rop_set_plane_mask(struct fbbm_devconfig *, uint32_t);
static void nwb225_rop_make_funcvec(struct fbbm_devconfig *, u_int,
    uint32_t, uint32_t, uint8_t *);
static void nwb225_rop_funcvec_constant_s(struct fbbm_devconfig *, uint8_t *,
    u_int);
static void nwb225_rop_set_funcvec(struct fbbm_devconfig *, const uint8_t *);

/* Hardware primitive ROP functions */
static int nwb225_rop_copy_rect(struct fbbm_devconfig *, int, int, int, int,
    int, int, u_int);
static int nwb225_rop_fill_rect(struct fbbm_devconfig *, int, int, int, int,
    uint32_t);
static int nwb225_rop_hwrite(struct fbbm_devconfig *, const uint16_t *, int,
    int, int, int, int, int, int, uint32_t, uint32_t);

/* ROM font functions */
static bus_size_t nwb225_krom_ascii_offset(u_int);
static void nwb225_krom_load_cell(struct fbbm_devconfig *, u_int,
    uint16_t *);

/* Initialization */
static int nwb225_init_hardware(struct fbbm_devconfig *);

static void fbbm_init_screen(struct fbbm_devconfig *);
static int fbbm_clear_screen(struct fbbm_devconfig *);

/* wsdisplay_emulops functions */
static void fbbm_cursor(void *, int, int, int);
static int fbbm_mapchar(void *, int, u_int *);
static void fbbm_putchar(void *, int, int, u_int, long);
static void fbbm_copycols(void *, int, int, int, int);
static void fbbm_erasecols(void *, int, int, int, long);
static void fbbm_copyrows(void *, int, int, int);
static void fbbm_eraserows(void *, int, int, long);
static int fbbm_allocattr(void *, int, int, int, long *);

/* wsdisplay_accessops functions */
static int fbbm_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static int fbbm_alloc_screen(void *, const struct wsscreen_descr *, void **,
    int *, int *, long *);
static void fbbm_free_screen(void *, void *);
static int fbbm_show_screen(void *, void *, int, void (*)(void *, int, int),
    void *);

/* cnattach function for early console */
int fbbm_cnattach(void);

CFATTACH_DECL_NEW(fbbm, sizeof(struct fbbm_softc),
    fbbm_match, fbbm_attach, NULL, NULL);

static struct fbbm_devconfig fbbm_console_dc;

static const struct wsdisplay_emulops fbbm_emulops = {
	.cursor = fbbm_cursor,
	.mapchar = fbbm_mapchar,
	.putchar = fbbm_putchar,
	.copycols = fbbm_copycols,
	.erasecols = fbbm_erasecols,
	.copyrows = fbbm_copyrows,
	.eraserows = fbbm_eraserows,
	.allocattr = fbbm_allocattr,
	.replaceattr = NULL,
};

static const struct wsdisplay_accessops fbbm_accessops = {
	.ioctl = fbbm_ioctl,
	.mmap = NULL,
	.alloc_screen = fbbm_alloc_screen,
	.free_screen = fbbm_free_screen,
	.show_screen = fbbm_show_screen,
	.load_font = NULL,
	.pollc = NULL,
	.scroll = NULL,
};

/* Timeout count to check ROP op complete */
#define NWB225_WAIT_MAX		100000

/* Maximum number of planes of NWB-225 (4 or 8) */
#define NWB225_MAX_PLANES	8

/*
 * Software logical functions F(S,D).
 * These values index nwb225_fccnvtab[] and are not hardware function values.
 */
#define NWB225_ROP_LF_0		0x00
#define NWB225_ROP_LF_S		0x03
#define NWB225_ROP_LF_D		0x05
#define NWB225_ROP_LF_DI	0x0a
#define NWB225_ROP_LF_1		0x0f

/*
 * Logical FBBM function to 51X hardware function conversion table
 * taken from NEWS-OS fbbm_51x.o.
 */
static const uint8_t nwb225_fccnvtab[16] = {
	0x03, 0x0b, 0x07, 0x0f,
	0x02, 0x0a, 0x06, 0x0e,
	0x01, 0x09, 0x05, 0x0d,
	0x00, 0x08, 0x04, 0x0c,
};

/*
 * Fixed initialization values written by the NWS-1750 PROM and NEWS-OS
 * 51X setup.  Their individual timing/control semantics are intentionally
 * not inferred here.
 */
static const uint16_t nwb225_init_words[] = {
	0x0400, 0x0002, 0x0005, 0x00d8,
	0x0000, 0x0006, 0x0000,
};

/*
 * 4-bit component values taken from the NWS-1750 PROM NWB-225 module.
 * The PROM repeats this 16-entry table through all 256 palette indices
 * and writes each component after shifting it left by four bits.
 */
static const uint8_t nwb225_prom_palette[16][3] = {
	{  0,  0,  0 },
	{  0,  0,  4 },
	{  0,  4,  0 },
	{  0,  4,  4 },
	{  4,  0,  0 },
	{  4,  0,  4 },
	{  4,  4,  0 },
	{  4,  4,  4 },
	{  8,  8,  8 },
	{  0,  0, 15 },
	{  0, 15,  0 },
	{  0, 15, 15 },
	{ 15,  0,  0 },
	{ 15,  0, 15 },
	{ 15, 15,  0 },
	{ 15, 15, 15 },
};

static bool
fbbm_console_initialized(void)
{

	/*
	 * This is initialized in fbbm_init_screen() via
	 * fbbm_cnattach() or fbbm_cnattach().
	 */
	return fbbm_console_dc.dc_wsscrdescr.textops == &fbbm_emulops;
}

static int
fbbm_probe(bus_space_tag_t bst, bus_addr_t rcont_addr)
{
	bus_space_handle_t bsh;
	int rv;

	if (bus_space_map(bst, rcont_addr, NWB225_RCONT_SIZE, 0, &bsh) != 0)
		return 0;

	/* NEWS-OS fb225_probe() used a byte-sized badaddr() at this base */
	rv = news68k_bus_space_probe(bst, bsh, 0, 1);
	bus_space_unmap(bst, bsh, NWB225_RCONT_SIZE);

	return rv;
}

static int
fbbm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct hb_attach_args *ha = aux;

	if (strcmp(ha->ha_name, "fbbm") != 0)
		return 0;
	if (ha->ha_address != NWB225_RCONT_BASE)
		return 0;

	return fbbm_probe(ha->ha_bust, ha->ha_address);
}

static void
fbbm_attach(device_t parent, device_t self, void *aux)
{
	struct fbbm_softc *sc = device_private(self);
	struct hb_attach_args *ha = aux;
	struct fbbm_devconfig *dc;
	struct wsemuldisplaydev_attach_args waa;
	bool console;

	sc->sc_dev = self;
	console = fbbm_console_initialized();

	if (console) {
		dc = &fbbm_console_dc;
		sc->sc_nscreens = 1;
	} else {
		dc = kmem_zalloc(sizeof(*dc), KM_SLEEP);
		if (nwb225_map(dc, ha->ha_bust, ha->ha_address) != 0) {
			aprint_error(": unable to map device registers");
			goto out_free;
		}

		if (nwb225_init_hardware(dc) != 0) {
			aprint_error(": controller initialization failed");
			goto out_unmap;
		}

		fbbm_init_screen(dc);
		if (fbbm_clear_screen(dc) != 0) {
			aprint_error(": unable to clear text screen");
			goto out_unmap;
		}
		sc->sc_nscreens = 0;
	}

	sc->sc_dc = dc;
	sc->sc_wsmode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_scrdescs[0] = &dc->dc_wsscrdescr;
	sc->sc_scrlist.nscreens = __arraycount(sc->sc_scrdescs);
	sc->sc_scrlist.screens = sc->sc_scrdescs;

	aprint_normal(": NWB-225, %d x %d, %d planes, display version %d\n",
	    dc->dc_width, dc->dc_height, dc->dc_nplanes, dc->dc_dispver);

	waa.console = console;
	waa.scrdata = &sc->sc_scrlist;
	waa.accessops = &fbbm_accessops;
	waa.accesscookie = sc;

	config_found(self, &waa, wsemuldisplaydevprint, CFARGS_NONE);
	return;

 out_unmap:
	nwb225_unmap(dc);
 out_free:
	kmem_free(dc, sizeof(*dc));
}

static int
nwb225_map(struct fbbm_devconfig *dc, bus_space_tag_t bst,
    bus_addr_t rcont_addr)
{
	int error;

	dc->dc_bst = bst;

	error = bus_space_map(bst, rcont_addr, NWB225_RCONT_SIZE, 0,
	    &dc->dc_rcont_bsh);
	if (error != 0)
		return error;

	error = bus_space_map(bst, NWB225_KROM_BASE, NWB225_KROM_SIZE, 0,
	    &dc->dc_krom_bsh);
	if (error != 0) {
		bus_space_unmap(bst, dc->dc_rcont_bsh, NWB225_RCONT_SIZE);
		return error;
	}

	return 0;
}

static void
nwb225_unmap(struct fbbm_devconfig *dc)
{

	bus_space_unmap(dc->dc_bst, dc->dc_krom_bsh, NWB225_KROM_SIZE);
	bus_space_unmap(dc->dc_bst, dc->dc_rcont_bsh, NWB225_RCONT_SIZE);
}

static void
nwb225_rop_reset(struct fbbm_devconfig *dc)
{

	/*
	 * Both NEWS-OS fb51X_rop_reset() and the NWS-1750 PROM perform
	 * this 0 -> 1 sequence.
	 */
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh, NWB225_STATUS, 0);
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh, NWB225_STATUS, 1);
}

static int
nwb225_rop_wait(struct fbbm_devconfig *dc)
{
	uint16_t status;
	int i;

	for (i = 0; i < NWB225_WAIT_MAX; i++) {
		status = bus_space_read_2(dc->dc_bst, dc->dc_rcont_bsh,
		    NWB225_STATUS);
		if ((status & NWB225_STATUS_BUSY) == 0)
			return 0;
	}

	/* Reset ROP engine on timeout as NEWS-OS does */
	nwb225_rop_reset(dc);
	return ETIMEDOUT;
}

static void
nwb225_ctl_write(struct fbbm_devconfig *dc, uint16_t selector, uint16_t value)
{

	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_CTL_SELECT, selector);
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_CTL_DATA, value);
}

static void
nwb225_rop_set_plane_mask(struct fbbm_devconfig *dc, uint32_t planemask)
{
	uint8_t disabled_planes;
	uint16_t hwmask;

	disabled_planes = (uint8_t)~planemask;
	hwmask = (uint16_t)disabled_planes << NWB225_PLANE_CTRL_SHIFT;
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_PLANE_CTRL, hwmask);
}

static void
nwb225_rop_make_funcvec(struct fbbm_devconfig *dc, u_int logical_func,
    uint32_t src1, uint32_t src0, uint8_t *funcvec)
{
	uint8_t f_s0_d, f_s1_d;
	uint8_t func_by_srcmap[4];
	uint8_t src1_bit, src0_bit, src_map;
	int i;

	KASSERT(logical_func <= 0x0f);
	KASSERT(dc->dc_nplanes == 4 || dc->dc_nplanes == 8);

	/* F(0,D) */
	f_s0_d = (logical_func >> 2) & 0x03;
	/* F(1,D) */
	f_s1_d = logical_func & 0x03;

	func_by_srcmap[0] = (f_s0_d << 2) | f_s0_d;
	func_by_srcmap[1] = (f_s1_d << 2) | f_s0_d;
	func_by_srcmap[2] = (f_s0_d << 2) | f_s1_d;
	func_by_srcmap[3] = (f_s1_d << 2) | f_s1_d;

	for (i = 0; i < dc->dc_nplanes; i++) {
		src1_bit = src1 & 0x01;
		src0_bit = src0 & 0x01;
		src_map = (src1_bit << 1) | src0_bit;
		funcvec[i] = func_by_srcmap[src_map];
		src1 >>= 1;
		src0 >>= 1;
	}
}

static void
nwb225_rop_funcvec_constant_s(struct fbbm_devconfig *dc, uint8_t *funcvec,
    u_int s)
{
	uint8_t logical_func, f_s0_d, f_s1_d, d_func, result;
	int i;

	KASSERT(dc->dc_nplanes == 4 || dc->dc_nplanes == 8);
	KASSERT(s <= 1);

	for (i = 0; i < dc->dc_nplanes; i++) {
		logical_func = funcvec[i];
		KASSERT(logical_func <= 0x0f);

		/* F(0,D) */
		f_s0_d = (logical_func >> 2) & 0x03;
		/* F(1,D) */
		f_s1_d = logical_func & 0x03;

		d_func = (s == 0) ? f_s0_d : f_s1_d;
		switch (d_func) {
		case 0:
			result = NWB225_ROP_LF_0;
			break;
		case 1:
			result = NWB225_ROP_LF_D;
			break;
		case 2:
			result = NWB225_ROP_LF_DI;
			break;
		case 3:
			result = NWB225_ROP_LF_1;
			break;
		default:
			KASSERT(false);
			result = NWB225_ROP_LF_0;
			break;
		}
		funcvec[i] = result;
	}
}

static void
nwb225_rop_set_funcvec(struct fbbm_devconfig *dc, const uint8_t *funcvec)
{
	uint8_t logical_func, hw_func;
	int i;

	for (i = 0; i < dc->dc_nplanes; i++) {
		KASSERT(funcvec[i] <= 0x0f);
		logical_func = funcvec[i];
		hw_func = nwb225_fccnvtab[logical_func];
		bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
		    NWB225_ROP_FUNC(i), hw_func);
	}
}

static int
nwb225_rect_valid(struct fbbm_devconfig *dc, int x, int y, int width,
    int height)
{

	if (x < 0 || y < 0 || width <= 0 || height <= 0)
		return 0;
	if (x >= dc->dc_fbwidth || y >= dc->dc_fbheight)
		return 0;
	if (width > dc->dc_fbwidth - x || height > dc->dc_fbheight - y)
		return 0;

	return 1;
}

static int
nwb225_rop_copy_rect(struct fbbm_devconfig *dc, int sx, int sy, int width,
    int height, int dx, int dy, u_int logical_func)
{
	uint8_t funcvec[NWB225_MAX_PLANES];
	bus_size_t copy_mode;
	bool reverse_x, reverse_y;
	int src_x, src_y, dst_x, dst_y, limit_x;
	int error;

	if (logical_func > 0x0f)
		return EINVAL;

	if (nwb225_rect_valid(dc, sx, sy, width, height) == 0 ||
	    nwb225_rect_valid(dc, dx, dy, width, height) == 0)
		return EINVAL;

	error = nwb225_rop_wait(dc);
	if (__predict_false(error != 0))
		return error;

	nwb225_rop_make_funcvec(dc, logical_func, dc->dc_planemask, 0, funcvec);
	nwb225_rop_set_funcvec(dc, funcvec);
	nwb225_rop_set_plane_mask(dc, dc->dc_planemask);

	/* Wait again as NEWS-OS does */
	error = nwb225_rop_wait(dc);
	if (__predict_false(error != 0))
		return error;

	reverse_x = (sx < dx);
	reverse_y = (sy < dy);
	src_x     = sx + (reverse_x ? width : 0);
	dst_x     = dx + (reverse_x ? width : 0);
	limit_x   = dx + (reverse_x ? 0 : width);
	src_y     = sy + (reverse_y ? height - 1 : 0);
	dst_y     = dy + (reverse_y ? height - 1 : 0);

	if (reverse_x) {
		if (reverse_y) {
			copy_mode = NWB225_ROP_COPY_XR_YR;
		} else {
			copy_mode = NWB225_ROP_COPY_XR_YF;
		}
	} else {
		if (reverse_y) {
			copy_mode = NWB225_ROP_COPY_XF_YR;
		} else {
			copy_mode = NWB225_ROP_COPY_XF_YF;
		}
	}

	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    copy_mode, 0);
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_DST_ADDR(dst_y), dst_x);
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_SRC_ADDR(src_y) | NWB225_ROP_ADDR_FLAG_SET_SHIFT, src_x);
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_ROP_LIMIT(height) | NWB225_ROP_ADDR_FLAG_EXECUTE, limit_x);

	/* Wait complete for polling ops */
	error = nwb225_rop_wait(dc);

	return error;
}

static int
nwb225_rop_fill_rect(struct fbbm_devconfig *dc, int x, int y, int width,
    int height, uint32_t color)
{
	uint8_t funcvec[NWB225_MAX_PLANES];
	int error;

	if (nwb225_rect_valid(dc, x, y, width, height) == 0)
		return EINVAL;

	error = nwb225_rop_wait(dc);
	if (__predict_false(error != 0))
		return error;

	color &= dc->dc_planemask;
	nwb225_rop_make_funcvec(dc, NWB225_ROP_LF_S, color, 0, funcvec);
	nwb225_rop_funcvec_constant_s(dc, funcvec, 1);
	nwb225_rop_set_plane_mask(dc, dc->dc_planemask);
	nwb225_rop_set_funcvec(dc, funcvec);

	/* Wait again as NEWS-OS does */
	error = nwb225_rop_wait(dc);
	if (__predict_false(error != 0))
		return error;

	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_ROP_CONST_FILL, 0);
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_DST_ADDR(0), 0);
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_SRC_ADDR(0) | NWB225_ROP_ADDR_FLAG_SET_SHIFT, 0);
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_DST_ADDR(y), x);
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_ROP_LIMIT(height) | NWB225_ROP_ADDR_FLAG_EXECUTE, x + width);

	/* Wait complete for polling ops */
	error = nwb225_rop_wait(dc);

	return error;
}

static int
nwb225_rop_hwrite(struct fbbm_devconfig *dc, const uint16_t *src_words,
    int src_stride_words, int srcx, int srcy, int width, int height,
    int dx, int dy, uint32_t foreground, uint32_t background)
{
	uint8_t funcvec[NWB225_MAX_PLANES];
	int source_word_offset, source_bit_offset;
	int row_words, row_skip_words, row, word;
	const uint16_t *sp;
	int error;

	if (src_words == NULL || src_stride_words <= 0 || srcx < 0 || srcy < 0)
		return EINVAL;

	if (nwb225_rect_valid(dc, dx, dy, width, height) == 0)
		return EINVAL;

	source_word_offset = srcx / NWB225_HWRITE_BITS_PER_WORD;
	source_bit_offset  = srcx % NWB225_HWRITE_BITS_PER_WORD;
	row_words =
	    howmany(source_bit_offset + width, NWB225_HWRITE_BITS_PER_WORD);
	if (source_word_offset + row_words > src_stride_words)
		return EINVAL;

	error = nwb225_rop_wait(dc);
	if (__predict_false(error != 0))
		return error;

	foreground &= dc->dc_planemask;
	background &= dc->dc_planemask;
	nwb225_rop_make_funcvec(dc, NWB225_ROP_LF_S, foreground, background,
	    funcvec);
	nwb225_rop_set_funcvec(dc, funcvec);
	nwb225_rop_set_plane_mask(dc, dc->dc_planemask);

	/* Wait again as NEWS-OS does */
	error = nwb225_rop_wait(dc);
	if (__predict_false(error != 0))
		return error;

	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_DST_ADDR(dy), dx);
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_SRC_ADDR(0) | NWB225_ROP_ADDR_FLAG_SET_SHIFT,
	    source_bit_offset);
	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
	    NWB225_HWRITE_LIMIT(height) | NWB225_ROP_ADDR_FLAG_EXECUTE,
	    dx + width);

	sp = src_words + (srcy * src_stride_words) + source_word_offset;
	row_skip_words = src_stride_words - row_words;
	for (row = 0; row < height; row++) {
		for (word = 0; word < row_words; word++)
			bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
			    NWB225_HWRITE_DATA, *sp++);
		sp += row_skip_words;
	}

	/* Wait complete for polling ops */
	error = nwb225_rop_wait(dc);

	return error;
}

static bus_size_t
nwb225_krom_ascii_offset(u_int c)
{
	u_int index;

	index = c - NWB225_KROM_ASCII_FIRST;
	return ((index & 0xe0) << 9) | ((index & 0x1f) << 7);
}

static void
nwb225_krom_load_cell(struct fbbm_devconfig *dc, u_int c, uint16_t *cell)
{
	bus_size_t offset;
	int row;

	memset(cell, 0, NWB225_CELL_HEIGHT * sizeof(*cell));
	offset = nwb225_krom_ascii_offset(c);

	/*
	 * The PROM renderer reads one 32-bit KROM row, keeps the upper word,
	 * masks its low four bits, and places the 24 glyph rows after four
	 * blank rows in the 16x30 cell.
	 */
	for (row = 0; row < NWB225_FONT_HEIGHT; row++)
		cell[NWB225_GLYPH_YOFFSET + row] =
		    bus_space_read_2(dc->dc_bst, dc->dc_krom_bsh,
		    offset + row * NWB225_KROM_ROWBYTES) &
		    NWB225_KROM_ASCII_MASK;
}

static int
nwb225_init_hardware(struct fbbm_devconfig *dc)
{
	uint16_t status, value;
	int i, component;

	/*
	 * Use the cold-init sequence used by the NWB-225 routine in the
	 * NWS-1750 PROM. In particular, the second reset is intentional.
	 * The serial console cold-boot experiment required the init block
	 * followed by this reset before stable sync was established by CRTC.
	 */
	nwb225_rop_reset(dc);

	for (i = 0; i < __arraycount(nwb225_init_words); i++)
		bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
		    NWB225_INIT_BASE + i * sizeof(uint16_t),
		    nwb225_init_words[i]);

	status = bus_space_read_2(dc->dc_bst, dc->dc_rcont_bsh, NWB225_STATUS);
	if ((status & NWB225_STATUS_4PLANE) != 0)
		dc->dc_nplanes = 4;
	else
		dc->dc_nplanes = 8;
	dc->dc_planemask = (1U << dc->dc_nplanes) - 1;
	dc->dc_dispver =
	    (status & NWB225_STATUS_DISPVER) >> NWB225_STATUS_DISPVER_SHIFT;

	nwb225_ctl_write(dc, 4, dc->dc_planemask);
	nwb225_ctl_write(dc, 5, 0);
	nwb225_ctl_write(dc, 6, 0x40);
	nwb225_ctl_write(dc, 7, 0);

	nwb225_rop_reset(dc);

	/* Complete the fixed PROM palette/control initialization */
	for (i = 0; i < 256; i++) {
		bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
		    NWB225_PAL_INDEX, i);
		for (component = 0; component < 3; component++) {
			value = nwb225_prom_palette[i & 0x0f][component] << 4;
			bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh,
			    NWB225_PAL_DATA, value);
		}
	}

	bus_space_write_2(dc->dc_bst, dc->dc_rcont_bsh, NWB225_PLANE_CTRL, 0);

	return nwb225_rop_wait(dc);
}

static void
fbbm_init_screen(struct fbbm_devconfig *dc)
{

	dc->dc_fbwidth = NWB225_FB_WIDTH;
	dc->dc_fbheight = NWB225_FB_HEIGHT;
	dc->dc_width = NWB225_VIS_WIDTH;
	dc->dc_height = NWB225_VIS_HEIGHT;

	dc->dc_fontwidth = NWB225_FONT_WIDTH;
	dc->dc_fontheight = NWB225_FONT_HEIGHT;
	dc->dc_cellwidth = NWB225_CELL_WIDTH;
	dc->dc_cellheight = NWB225_CELL_HEIGHT;

	dc->dc_ncols = NWB225_TEXT_COLS;
	dc->dc_nrows = NWB225_TEXT_ROWS;
	dc->dc_xorigin = NWB225_TEXT_XORIGIN;
	dc->dc_yorigin = NWB225_TEXT_YORIGIN;

	dc->dc_wsscrdescr.name = "std";
	dc->dc_wsscrdescr.ncols = dc->dc_ncols;
	dc->dc_wsscrdescr.nrows = dc->dc_nrows;
	dc->dc_wsscrdescr.textops = &fbbm_emulops;
	dc->dc_wsscrdescr.fontwidth = dc->dc_cellwidth;
	dc->dc_wsscrdescr.fontheight = dc->dc_cellheight;
	dc->dc_wsscrdescr.capabilities = WSSCREEN_REVERSE;
	dc->dc_wsscrdescr.modecookie = NULL;

	dc->dc_cursoron = 0;
	dc->dc_cursorrow = 0;
	dc->dc_cursorcol = 0;
}

/*
 * Clear the visible display region.
 * Called from both normal attach and early cnattach.
 */
static int
fbbm_clear_screen(struct fbbm_devconfig *dc)
{

	return nwb225_rop_fill_rect(dc, 0, 0, dc->dc_width, dc->dc_height, 0);
}

static uint32_t
fbbm_attr_background(struct fbbm_devconfig *dc, long attr)
{

	return (attr & WSATTR_REVERSE) != 0 ? dc->dc_planemask : 0;
}

/*
 * wsdisplay_emulops functions
 */
static void
fbbm_cursor(void *cookie, int on, int row, int col)
{
	struct fbbm_devconfig *dc = cookie;
	int x, y, error;

	if (dc->dc_cursoron != 0) {
		x = dc->dc_xorigin + dc->dc_cursorcol * dc->dc_cellwidth;
		y = dc->dc_yorigin + dc->dc_cursorrow * dc->dc_cellheight;
		error = nwb225_rop_copy_rect(dc, x, y, dc->dc_cellwidth,
		    dc->dc_cellheight, x, y, NWB225_ROP_LF_DI);
		if (error != 0)
			return;
		dc->dc_cursoron = 0;
	}

	dc->dc_cursorrow = row;
	dc->dc_cursorcol = col;
	if (on == 0)
		return;
	if (row < 0 || row >= dc->dc_nrows || col < 0 || col >= dc->dc_ncols)
		return;

	x = dc->dc_xorigin + col * dc->dc_cellwidth;
	y = dc->dc_yorigin + row * dc->dc_cellheight;
	error = nwb225_rop_copy_rect(dc, x, y, dc->dc_cellwidth,
	    dc->dc_cellheight, x, y, NWB225_ROP_LF_DI);
	if (error == 0)
		dc->dc_cursoron = 1;
}

static int
fbbm_mapchar(void *cookie, int c, u_int *cp)
{

	if (c < NWB225_KROM_ASCII_FIRST || c > NWB225_KROM_ASCII_LAST) {
		*cp = ' ';
		return 0;
	}

	*cp = c;
	return 5;
}

static void
fbbm_putchar(void *cookie, int row, int col, u_int uc, long attr)
{
	struct fbbm_devconfig *dc = cookie;
	uint16_t cell[NWB225_CELL_HEIGHT];
	uint32_t foreground, background;
	int x, y;

	if (__predict_false(row < 0 || row >= dc->dc_nrows ||
	    col < 0 || col >= dc->dc_ncols))
		return;
	if (uc < NWB225_KROM_ASCII_FIRST || uc > NWB225_KROM_ASCII_LAST)
		uc = ' ';

	nwb225_krom_load_cell(dc, uc, cell);
	background = fbbm_attr_background(dc, attr);
	foreground = (~background) & dc->dc_planemask;
	x = dc->dc_xorigin + col * dc->dc_cellwidth;
	y = dc->dc_yorigin + row * dc->dc_cellheight;

	(void)nwb225_rop_hwrite(dc, cell, 1, 0, 0, dc->dc_cellwidth,
	    dc->dc_cellheight, x, y, foreground, background);
}

static void
fbbm_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct fbbm_devconfig *dc = cookie;
	int sx, dx, y, width;

	if (ncols <= 0 || row < 0 || row >= dc->dc_nrows)
		return;
	if (srccol < 0 || dstcol < 0 || srccol + ncols > dc->dc_ncols ||
	    dstcol + ncols > dc->dc_ncols)
		return;
	if (srccol == dstcol)
		return;

	sx = dc->dc_xorigin + srccol * dc->dc_cellwidth;
	dx = dc->dc_xorigin + dstcol * dc->dc_cellwidth;
	y = dc->dc_yorigin + row * dc->dc_cellheight;
	width = ncols * dc->dc_cellwidth;
	(void)nwb225_rop_copy_rect(dc, sx, y, width, dc->dc_cellheight,
	    dx, y, NWB225_ROP_LF_S);
}

static void
fbbm_erasecols(void *cookie, int row, int col, int ncols, long attr)
{
	struct fbbm_devconfig *dc = cookie;
	uint32_t background;
	int x, y, width;

	if (ncols <= 0 || row < 0 || row >= dc->dc_nrows)
		return;
	if (col < 0 || col + ncols > dc->dc_ncols)
		return;

	x = dc->dc_xorigin + col * dc->dc_cellwidth;
	y = dc->dc_yorigin + row * dc->dc_cellheight;
	width = ncols * dc->dc_cellwidth;
	background = fbbm_attr_background(dc, attr);
	(void)nwb225_rop_fill_rect(dc, x, y, width, dc->dc_cellheight,
	    background);
}

static void
fbbm_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct fbbm_devconfig *dc = cookie;
	int x, sy, dy, height;

	if (nrows <= 0 || srcrow < 0 || dstrow < 0 ||
	    srcrow + nrows > dc->dc_nrows || dstrow + nrows > dc->dc_nrows)
		return;
	if (srcrow == dstrow)
		return;

	x = dc->dc_xorigin;
	sy = dc->dc_yorigin + srcrow * dc->dc_cellheight;
	dy = dc->dc_yorigin + dstrow * dc->dc_cellheight;
	height = nrows * dc->dc_cellheight;
	(void)nwb225_rop_copy_rect(dc, x, sy,
	    dc->dc_ncols * dc->dc_cellwidth, height, x, dy, NWB225_ROP_LF_S);
}

static void
fbbm_eraserows(void *cookie, int row, int nrows, long attr)
{
	struct fbbm_devconfig *dc = cookie;
	uint32_t background;
	int x, y, height;

	if (nrows <= 0 || row < 0 || row + nrows > dc->dc_nrows)
		return;

	x = dc->dc_xorigin;
	y = dc->dc_yorigin + row * dc->dc_cellheight;
	height = nrows * dc->dc_cellheight;
	background = fbbm_attr_background(dc, attr);
	(void)nwb225_rop_fill_rect(dc, x, y,
	    dc->dc_ncols * dc->dc_cellwidth, height, background);
}

static int
fbbm_allocattr(void *cookie, int fg, int bg, int flags, long *attrp)
{

	if ((flags & ~WSATTR_REVERSE) != 0)
		return EINVAL;

	*attrp = flags & WSATTR_REVERSE;
	return 0;
}
 
/*
 * wsdisplay_accessops functions
 */
static int
fbbm_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct fbbm_softc *sc = v;
	struct fbbm_devconfig *dc = sc->sc_dc;
	struct wsdisplay_fbinfo *fbinfo;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_NWB225;
		return 0;

	case WSDISPLAYIO_GINFO:
		fbinfo = data;
		fbinfo->height = dc->dc_height;
		fbinfo->width = dc->dc_width;
		fbinfo->depth = dc->dc_nplanes;
		fbinfo->cmsize = 0;
		return 0;

	case WSDISPLAYIO_SMODE:
		sc->sc_wsmode = *(int *)data;
		return 0;

	default:
		break;
	}

	return EPASSTHROUGH;
}

static int
fbbm_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *ccolp, int *crowp, long *attrp)
{
	struct fbbm_softc *sc = v;
	struct fbbm_devconfig *dc = sc->sc_dc;
	long attr;
	int error;

	KASSERT(type == &dc->dc_wsscrdescr);

	if (sc->sc_nscreens != 0)
		return ENOMEM;

	error = fbbm_allocattr(dc, 0, 0, 0, &attr);
	if (error != 0)
		return error;

	*cookiep = dc;
	*ccolp = 0;
	*crowp = 0;
	*attrp = attr;
	sc->sc_nscreens = 1;

	return 0;
}

static void
fbbm_free_screen(void *v, void *cookie)
{
	struct fbbm_softc *sc = v;

	KASSERT(sc->sc_nscreens == 1);
	KASSERT(cookie == sc->sc_dc);
	KASSERT(sc->sc_dc != &fbbm_console_dc);
	sc->sc_nscreens = 0;
}

static int
fbbm_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{

	return 0;
}

int
fbbm_cnattach(void)
{
	struct fbbm_devconfig *dc;
	long attr;

	if (fbbm_console_initialized())
		return 0;

	dc = &fbbm_console_dc;
	memset(dc, 0, sizeof(*dc));
	if (fbbm_probe(NEWS68K_BUS_SPACE_EIO, NWB225_RCONT_BASE) == 0)
		return ENXIO;

	if (nwb225_map(dc, NEWS68K_BUS_SPACE_EIO, NWB225_RCONT_BASE) != 0)
		goto out_clear;

	if (nwb225_init_hardware(dc) != 0)
		goto out_unmap;

	fbbm_init_screen(dc);
	if (fbbm_clear_screen(dc) != 0)
		goto out_unmap;

	if (fbbm_allocattr(dc, 0, 0, 0, &attr))
		goto out_unmap;

	wsdisplay_cnattach(&dc->dc_wsscrdescr, dc, 0, 0, attr);
	return 0;

 out_unmap:
	nwb225_unmap(dc);
 out_clear:
	memset(dc, 0, sizeof(*dc));
	return 1;
}
