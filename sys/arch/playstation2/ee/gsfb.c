/*	$NetBSD: gsfb.c,v 1.1.6.4 2002/06/23 17:39:09 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "debug_playstation2.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/autoconf.h>

#include <dev/cons.h> 

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wscons_callbacks.h>

#include <dev/wsfont/wsfont.h>

#include <playstation2/ee/eevar.h>
#include <playstation2/ee/gsvar.h>
#include <playstation2/ee/gsreg.h>
#include <playstation2/ee/dmacvar.h>
#include <playstation2/ee/dmacreg.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC	static
#endif

STATIC struct gsfb {
	int initialized;
	int attached;
	int is_console;
	const struct wsscreen_descr *screen;
	struct wsdisplay_font *font;
} gsfb;

STATIC void gsfb_dma_kick(paddr_t, size_t);
STATIC void gsfb_font_expand_psmct32(const struct wsdisplay_font *, u_int,
    long, u_int32_t *);
STATIC __inline__ void gsfb_set_cursor_pos(u_int32_t *, int, int, int, int);

#define ATTR_FG_GET(a)	(((a )>> 24) & 0xf)
#define ATTR_BG_GET(a)	(((a )>> 16) & 0xf)
#define ATTR_FG_SET(x)	(((x) << 24) & 0x0f000000)
#define ATTR_BG_SET(x)	(((x) << 16) & 0x000f0000)

STATIC const u_int32_t gsfb_ansi_psmct32[] = {
	0x80000000, /* black */
	0x800000aa, /* red */
	0x8000aa00, /* green */
	0x8000aaaa, /* brown */
	0x80aa0000, /* blue */
	0x80aa00aa, /* magenta */
	0x80aaaa00, /* cyan */
	0x80aaaaaa, /* white */
	0x80000000, /* black */
	0x800000ff, /* red */
	0x8000ff00, /* green */
	0x8000ffff, /* brown */
	0x80ff0000, /* blue */
	0x80ff00ff, /* magenta */
	0x80ffff00, /* cyan */
	0x80ffffff, /* black */
};

#define TRXPOS_DXY(f, x, y)						\
({									\
	f[9] = ((x) & 0x000007ff) | (((y) << 16) & 0x07ff0000);		\
})

#define TRXPOS_SY_DY(f, sy, dy)						\
({									\
	f[8] = (((sy) << 16) & 0x07ff0000);				\
	f[9] = (((dy) << 16) & 0x07ff0000);				\
})

#define TRXPOS_DXY_SXY(f, dx, dy, sx, sy)				\
({									\
	f[8] = ((((sy) << 16) & 0x07ff0000) | ((sx) & 0x000007ff));	\
	f[9] = ((((dy) << 16) & 0x07ff0000) | ((dx) & 0x000007ff));	\
})

STATIC u_int32_t gsfb_scroll_cmd_640x16[] __attribute__((__aligned__(16))) = {
        0x00008004, 0x10000000, 0x0000000e, 0x00000000,
        0x000a0000, 0x000a0000, 0x00000050, 0x00000000,
        0x07ff0000, 0x07ff0000, 0x00000051, 0x00000000,
        0x00000280, 0x00000010, 0x00000052, 0x00000000,
        0x00000002, 0x00000000, 0x00000053, 0x00000000,
};

STATIC u_int32_t gsfb_cursor_cmd[] __attribute__((__aligned__(16))) = {
	0x00008007, 0x10000000, 0x0000000e, 0x00000000,
	0x00000001, 0x00000000, 0x0000001a, 0x00000000,
        0x000000a4, 0x00000080, 0x00000042, 0x00000000,
	0x00000046, 0x00000000, 0x00000000, 0x00000000,
	0x80ffffff, 0x00000000, 0x00000001, 0x00000000,
	0x00000000, 0x00000000, 0x0000000d, 0x00000000,
	0x80ffffff, 0x00000000, 0x00000001, 0x00000000,
	0x00000000, 0x00000000, 0x00000005, 0x00000000,
};

STATIC u_int32_t gsfb_copy_cmd_8x16[] __attribute__((__aligned__(16))) = {
        0x00008004, 0x10000000, 0x0000000e, 0x00000000,
        0x000a0000, 0x000a0000, 0x00000050, 0x00000000,
        0x07ff07ff, 0x07ff07ff, 0x00000051, 0x00000000,
        0x00000008, 0x00000010, 0x00000052, 0x00000000,
        0x00000002, 0x00000000, 0x00000053, 0x00000000,
};

STATIC u_int32_t gsfb_init_cmd_640x480[] __attribute__((__aligned__(16))) = {
	0x00008008, 0x10000000, 0x0000000e, 0x00000000,
	0x000a0000, 0x00000000, 0x0000004c, 0x00000000,
	0x00000096, 0x00000000, 0x0000004e, 0x00000000,
	0x02800000, 0x01e00000, 0x00000040, 0x00000000,
	0x00000006, 0x00000000, 0x00000000, 0x00000000,
	0x80000000, 0x00000000, 0x00000001, 0x00000000,
	0x00000000, 0x00000000, 0x0000000d, 0x00000000,
	0x80000000, 0x00000000, 0x00000001, 0x00000000,
	0x1e002800, 0x00000000, 0x00000005, 0x00000000,
};

STATIC u_int32_t gsfb_load_cmd_8x16_psmct32[(6 + 32) * 4]
	__attribute__((__aligned__(16))) = {
	/* GIF tag + GS command */
        0x00000004, 0x10000000, 0x0000000e, 0x00000000,
        0x00000000, 0x000a0000, 0x00000050, 0x00000000,
        0x00000000, 0x00000000, 0x00000051, 0x00000000,
        0x00000008, 0x00000016, 0x00000052, 0x00000000,
        0x00000000, 0x00000000, 0x00000053, 0x00000000,
        0x00008020, 0x08000000, 0x00000000, 0x00000000,
	/* Load area */
#define FONT_SCRATCH_BASE	(6 * 4)
};

#ifdef GSFB_DEBUG_MONITOR
#include <machine/stdarg.h>
STATIC const struct _gsfb_debug_window {
	int start, nrow, attr;
} _gsfb_debug_window[3] = {
	{ 24, 2 , ATTR_BG_SET(WSCOL_BROWN) | ATTR_FG_SET(WSCOL_BLUE) },
	{ 26, 2 , ATTR_BG_SET(WSCOL_CYAN) | ATTR_FG_SET(WSCOL_BLUE) },
	{ 28, 2 , ATTR_BG_SET(WSCOL_WHITE) | ATTR_FG_SET(WSCOL_BLUE) },
};
STATIC char _gsfb_debug_buf[80 * 2];
#endif /* GSFB_DEBUG_MONITOR */

STATIC int gsfb_match(struct device *, struct cfdata *, void *);
STATIC void gsfb_attach(struct device *, struct device *, void *);

struct cfattach gsfb_ca = {
	sizeof(struct device), gsfb_match, gsfb_attach
};

STATIC void gsfb_hwinit(void);
STATIC int gsfb_swinit(void);

/* console */
void gsfbcnprobe(struct consdev *);
void gsfbcninit(struct consdev *);

/* emul ops */
STATIC void _gsfb_cursor(void *, int, int, int);
STATIC int _gsfb_mapchar(void *, int, unsigned int *);
STATIC void _gsfb_putchar(void *, int, int, u_int, long);
STATIC void _gsfb_copycols(void *, int, int, int, int);
STATIC void _gsfb_erasecols(void *, int, int, int, long);
STATIC void _gsfb_copyrows(void *, int, int, int);
STATIC void _gsfb_eraserows(void *, int, int, long);
STATIC int _gsfb_alloc_attr(void *, int, int, int, long *);

/* access ops */
STATIC int _gsfb_ioctl(void *, u_long, caddr_t, int, struct proc *);
STATIC paddr_t _gsfb_mmap(void *, off_t, int);
STATIC int _gsfb_alloc_screen(void *, const struct wsscreen_descr *, void **,
    int *, int *, long *);
STATIC void _gsfb_free_screen(void *, void *);
STATIC int _gsfb_show_screen(void *, void *, int, void (*)(void *, int, int),
    void *);
STATIC void _gsfb_pollc(void *, int);

/* 
 * wsdisplay attach args 
 *   std: screen size 640 x 480, font size 8 x 16
 */
#define GSFB_STD_SCREEN_WIDTH		640
#define GSFB_STD_SCREEN_HEIGHT		480
#define GSFB_STD_FONT_WIDTH		8
#define GSFB_STD_FONT_HEIGHT		16
const struct wsdisplay_emulops _gsfb_emulops = {
	.cursor		= _gsfb_cursor,
	.mapchar	= _gsfb_mapchar,
	.putchar	= _gsfb_putchar,
	.copycols	= _gsfb_copycols,
	.erasecols	= _gsfb_erasecols,
	.copyrows	= _gsfb_copyrows,
	.eraserows	= _gsfb_eraserows,
	.alloc_attr	= _gsfb_alloc_attr
};

const struct wsscreen_descr _gsfb_std_screen = {
	.name		= "std",
	.ncols		= 80,
#ifdef GSFB_DEBUG_MONITOR
	.nrows		= 24,
#else
	.nrows		= 30,
#endif
	.textops	= &_gsfb_emulops,
	.fontwidth	= 8,
	.fontheight	= 16,
	.capabilities	= WSSCREEN_UNDERLINE | WSSCREEN_HILIT |
	WSSCREEN_WSCOLORS
};

const struct wsscreen_descr *_gsfb_screen_table[] = {
	&_gsfb_std_screen,
};

struct wsscreen_list _gsfb_screen_list = {
	.nscreens	= sizeof(_gsfb_screen_table) /
	sizeof(_gsfb_screen_table[0]),
	.screens	= _gsfb_screen_table
};

struct wsdisplay_accessops _gsfb_accessops = {
	.ioctl		= _gsfb_ioctl,
	.mmap		= _gsfb_mmap,
	.alloc_screen	= _gsfb_alloc_screen,
	.free_screen	= _gsfb_free_screen,
	.show_screen	= _gsfb_show_screen,
	.load_font	= 0,
	.pollc		= _gsfb_pollc
};

int
gsfb_match(struct device *parent, struct cfdata *cf, void *aux)
{
	extern struct cfdriver gsfb_cd;
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, gsfb_cd.cd_name) != 0)
		return (0);

	return (!gsfb.attached);
}

void
gsfb_attach(struct device *parent, struct device *self, void *aux)
{
	struct wsemuldisplaydev_attach_args wa;

	gsfb.attached = 1;
	if (!gsfb.is_console && gsfb_swinit() != 0)
		return;

	printf("\n");

	wa.console	= gsfb.is_console;
	wa.scrdata	= &_gsfb_screen_list;
	wa.accessops	= &_gsfb_accessops;
	wa.accesscookie	= &gsfb;

	config_found(self, &wa, wsdisplaydevprint);
}

/*
 * console
 */
void
gsfbcnprobe(struct consdev *cndev)
{
	
	cndev->cn_pri = CN_INTERNAL;
}

void
gsfbcninit(struct consdev *cndev)
{
	paddr_t paddr = MIPS_KSEG0_TO_PHYS(gsfb_init_cmd_640x480);
	long defattr =  ATTR_BG_SET(WSCOL_BLACK) | ATTR_FG_SET(WSCOL_WHITE);

	gsfb.is_console = 1;

	gsfb_hwinit();
	gsfb_swinit();

	gsfb_dma_kick(paddr, sizeof gsfb_init_cmd_640x480);
#ifdef GSFB_DEBUG_MONITOR
	{
		const struct _gsfb_debug_window *win;
		int i;

		for (i = 0; i < 3; i++) {
			win = &_gsfb_debug_window[i];
			_gsfb_eraserows(0, win->start, win->nrow, win->attr);
		}
	}
#endif /* GSFB_DEBUG_MONITOR */

	wsdisplay_cnattach(&_gsfb_std_screen, &gsfb, 0, 0, defattr);
}

void
gsfb_hwinit()
{
	gs_init(VESA_1A);
	dmac_init();

	/* reset GIF channel DMA */
	_reg_write_4(D2_QWC_REG, 0);
	_reg_write_4(D2_MADR_REG, 0);
	_reg_write_4(D2_TADR_REG, 0);
	_reg_write_4(D2_CHCR_REG, 0);
}

int
gsfb_swinit()
{
	int font;

	wsfont_init();	
	font = wsfont_find(NULL, 8, 16, 0,  WSDISPLAY_FONTORDER_L2R,
	    WSDISPLAY_FONTORDER_L2R);
	if (font < 0)
		return (1);

	if (wsfont_lock(font, &gsfb.font))
		return (1);

	gsfb.screen = &_gsfb_std_screen;
	gsfb.initialized = 1;

	return (0);
}

/*
 * wsdisplay
 */
void
_gsfb_cursor(void *cookie, int on, int row, int col)
{
	paddr_t paddr = MIPS_KSEG0_TO_PHYS(gsfb_cursor_cmd);
	u_int32_t *buf = (void *)MIPS_PHYS_TO_KSEG1(paddr);
	struct wsdisplay_font *font = gsfb.font;

	gsfb_set_cursor_pos(buf, col, row, font->fontwidth, font->fontheight);

	gsfb_dma_kick(paddr, sizeof gsfb_cursor_cmd);
}

__inline__ void
gsfb_set_cursor_pos(u_int32_t *p, int x, int y, int w, int h)
{

	x *= w;
	y *= h;
	p[20] = ((x << 4) & 0xffff) | ((y << 20) & 0xffff0000);
	p[28] = (((x + w - 1) << 4) & 0xffff) |
	    (((y + h - 1) << 20) & 0xffff0000);
}

int
_gsfb_mapchar(void *cookie, int c, unsigned int *cp)
{
	struct wsdisplay_font *font = gsfb.font;
	
	if (font->encoding != WSDISPLAY_FONTENC_ISO)
		if ((c = wsfont_map_unichar(font, c)) < 0)
			goto nomap;

	if (c < font->firstchar || c >= font->firstchar + font->numchars)
			goto nomap;

	*cp = c;
	return (5);

 nomap:
	*cp = ' ';
	return (0);
}

void
_gsfb_putchar(void *cookie, int row, int col, u_int uc, long attr)
{
	paddr_t paddr = MIPS_KSEG0_TO_PHYS(gsfb_load_cmd_8x16_psmct32);
	u_int32_t *buf = (void *)MIPS_PHYS_TO_KSEG1(paddr);
	struct wsdisplay_font *font = gsfb.font;

	/* copy font data to DMA region */
	gsfb_font_expand_psmct32(font, uc, attr, &buf[FONT_SCRATCH_BASE]);

	/* set destination position */
	TRXPOS_DXY(buf, col * font->fontwidth, row * font->fontheight);

	/* kick to GIF */
	gsfb_dma_kick(paddr, sizeof gsfb_load_cmd_8x16_psmct32);
}

void
_gsfb_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	paddr_t paddr = MIPS_KSEG0_TO_PHYS(gsfb_copy_cmd_8x16);
	u_int32_t *cmd = (void *)MIPS_PHYS_TO_KSEG1(paddr);
	int y = gsfb.font->fontheight * row;
	int w = gsfb.font->fontwidth;
	int i;

	if (dstcol > srccol) {
		for (i = ncols - 1; i >= 0; i--) {
			TRXPOS_DXY_SXY(cmd, (dstcol + i) * w, y, (srccol + i) * w, y);
			gsfb_dma_kick(paddr, sizeof gsfb_copy_cmd_8x16);
		}
	} else {
		for (i = 0; i < ncols; i++) {
			TRXPOS_DXY_SXY(cmd, (dstcol + i) * w, y, (srccol + i) * w, y);
			gsfb_dma_kick(paddr, sizeof gsfb_copy_cmd_8x16);
		}
	}
}

void
_gsfb_erasecols(void *cookie, int row, int startcol, int ncols, long attr)
{
	int i;
	
	for (i = 0; i < ncols; i++)
		_gsfb_putchar(cookie, row, startcol + i, ' ', attr);
}

void
_gsfb_copyrows(void *cookie, int src, int dst, int num)
{
	paddr_t paddr = MIPS_KSEG0_TO_PHYS(gsfb_scroll_cmd_640x16);
	u_int32_t *cmd = (void *)MIPS_PHYS_TO_KSEG1(paddr);
	int i;
	int h = gsfb.font->fontheight;

	if (dst > src) {
		for (i = num - 1; i >= 0; i--) {
			TRXPOS_SY_DY(cmd, (src + i) * h, (dst  + i) * h);
			gsfb_dma_kick(paddr, sizeof gsfb_scroll_cmd_640x16);
		}
	} else {
		for (i = 0; i < num; i++) {
			TRXPOS_SY_DY(cmd, (src + i) * h, (dst  + i) * h);
			gsfb_dma_kick(paddr, sizeof gsfb_scroll_cmd_640x16);
		}
	}
}

void
_gsfb_eraserows(void *cookie, int row, int nrow, long attr)
{
	int i, j;
	
	for (j = 0; j < nrow; j++)
		for (i = 0; i < gsfb.screen->ncols; i++)
			_gsfb_putchar(cookie, row + j, i, ' ', attr);
}

int
_gsfb_alloc_attr(void *cookie, int fg, int bg, int flags, long *attr)
{

	if ((flags & WSATTR_BLINK) != 0)
		return (EINVAL);

	if ((flags & WSATTR_WSCOLORS) == 0) {
		fg = WSCOL_WHITE;
		bg = WSCOL_BLACK;
	}

	if ((flags & WSATTR_HILIT) != 0)
		fg += 8;

	flags = (flags & WSATTR_UNDERLINE) ? 1 : 0;


	*attr = ATTR_BG_SET(bg) | ATTR_FG_SET(fg) | flags;

	return (0);
}

int
_gsfb_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{

	return (EPASSTHROUGH); /* Inappropriate ioctl for device */
}

paddr_t
_gsfb_mmap(void *v, off_t offset, int prot)
{

	return (NULL); /* can't mmap */
}

int
_gsfb_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *curxp, int *curyp, long *attrp)
{

	*attrp = ATTR_BG_SET(WSCOL_BLACK) | ATTR_FG_SET(WSCOL_WHITE);

	return (0);
}

void
_gsfb_free_screen(void *v, void *cookie)
{
}

int
_gsfb_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{

	return (0);
}

void
_gsfb_pollc(void *v, int on)
{

}

/* 
 * font expansion 
 *   PSMCT32 only
 */
void
gsfb_font_expand_psmct32(const struct wsdisplay_font *font, u_int c, long attr,
    u_int32_t *buf)
{
	u_int32_t fg, bg;
	u_int8_t *bitmap;
	int i, j;
	
	KDASSERT(((u_int32_t)buf & 15) == 0);

	fg = gsfb_ansi_psmct32[ATTR_FG_GET(attr)];
	bg = gsfb_ansi_psmct32[ATTR_BG_GET(attr)];

	bitmap = (u_int8_t *)font->data + (c - font->firstchar) * 
	    font->fontheight * font->stride;
	for (i = 0; i < font->fontheight; i++, bitmap++) {
		u_int32_t b = *bitmap;
		for (j = 0; j < font->fontwidth; j++, b <<= 1)
			*buf++ = (b & 0x80) ? fg : bg;
	}
}

void 
gsfb_dma_kick(paddr_t addr, size_t size)
{
	/* Wait for previous DMA request complete */
	while (_reg_read_4(D2_QWC_REG))
		;

	/* Wait until GS FIFO empty */
	while ((_reg_read_8(GS_S_CSR_REG) & (3 << 14)) != (1 << 14))
		;

	/* wait for DMA complete */
	dmac_bus_poll(D_CH2_GIF);

	/* transfer addr */
	_reg_write_4(D2_MADR_REG, addr);
	/* transfer data size (unit qword) */
	_reg_write_4(D2_QWC_REG, bytetoqwc(size));
	
	/* kick DMA (normal-mode) */
	dmac_chcr_write(D_CH2_GIF, D_CHCR_STR);
}

#ifdef GSFB_DEBUG_MONITOR
void
__gsfb_print(int window, const char *fmt, ...)
{
	const struct _gsfb_debug_window *win;
	int i, s, x, y, n, a;
	u_int c;
	va_list ap;
	
	if (!gsfb.initialized)
		return;
	
	s = _intr_suspend();
	win = &_gsfb_debug_window[window];
	x = 0;
	y = win->start;
	n = win->nrow * 80;
	a = win->attr;

	va_start(ap, fmt);
	vsnprintf(_gsfb_debug_buf, n, fmt, ap);
	va_end(ap);

	_gsfb_eraserows(0, y, win->nrow, a);

	for (i = 0; i < n &&
	    (c = (u_int)_gsfb_debug_buf[i] & 0x7f) != 0; i++) {
		if (c == '\n')
			x = 0, y++;
		else
			_gsfb_putchar(0, y, x++, c, a);
	}

	_intr_resume(s);
}

void
__gsfb_print_hex(int a0, int a1, int a2, int a3)
{
	__gsfb_print(2, "a0=%08x a1=%08x a2=%08x a3=%08x",
	    a0, a1, a2, a3);
}
#endif /* GSFB_DEBUG_MONITOR */
