/*	$NetBSD: spx.c,v 1.1.14.1 2009/05/13 17:18:41 jym Exp $ */
/*
 * SPX/LCSPX/SPXg/SPXgt accelerated framebuffer driver for NetBSD/VAX
 * Copyright (c) 2005 Blaz Antonic
 * All rights reserved.
 *
 * This software contains code written by Michael L. Hitch.
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
 *    must display the abovementioned copyrights
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: spx.c,v 1.1.14.1 2009/05/13 17:18:41 jym Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/time.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/kernel.h>

#include <uvm/uvm.h>

#include <machine/vsbus.h>
#include <machine/sid.h>
#include <machine/cpu.h>
#include <machine/ka420.h>

#include <dev/cons.h>

#include <dev/dec/dzreg.h>
#include <dev/dec/dzvar.h>
#include <dev/dec/dzkbdvar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_callbacks.h>
#include <dev/wsfont/wsfont.h>

#include "machine/scb.h"

#include "dzkbd.h"

#define CONF_LCSPX		0x02
#define CONF_SPXg		0x10

#define FB_IS_SPX		1
#define FB_IS_SPXg		2

/* Screen hardware defs */
#define	SPX_FB_ADDR		0x38000000	/* Frame buffer */
#define SPXg_FB_ADDR_KA46	0x22200000
#define SPXg_FB_ADDR_KA49	0x28200000
#define SPXg_FB_ADDR		((vax_boardtype == VAX_BTYP_46) ? \
				 SPXg_FB_ADDR_KA46 : SPXg_FB_ADDR_KA49)

#define SPXg_WIN_SIZE		0x00100000	/* Window size */
/* # of pixels that fit into single window */
#define SPXg_WIN_LINEAR		(SPXg_WIN_SIZE / 2)

#define SPXg_DELAY		5

/*
 * off-screen font storage space 
 * 32x16 glyphs, 256 regular and underliend chars
 */
#define FONT_STORAGE_START	(spx_xsize * spx_ysize)
#define FONT_STORAGE_SIZE	(32 * 16 * 256 * 2)

/* register space defines */
#define SPX_REG_SIZE		0x00002000
#define SPX_REG_ADDR		0x39302000	/* 1st set of SPX registers */

#define SPXg_REG_SIZE		0x00004000
#define SPXg_REG_ADDR_KA46	0x22004000
#define SPXg_REG_ADDR_KA49	0x28004000
#define SPXg_REG_ADDR		((vax_boardtype == VAX_BTYP_46) ? \
				 SPXg_REG_ADDR_KA46 : SPXg_REG_ADDR_KA49)

#define SPX_REG1_SIZE		0x00001000
#define SPX_REG1_ADDR		0x39b00000	/* 2nd set of SPX registers */

#define SPXg_REG1_SIZE		0x00001000
#define SPXg_REG1_ADDR_KA46	0x22100000
#define SPXg_REG1_ADDR_KA49	0x28100000
#define SPXg_REG1_ADDR		((vax_boardtype == VAX_BTYP_46) ? \
				 SPXg_REG1_ADDR_KA46 : SPXg_REG1_ADDR_KA49)
#define SPX_REG(reg)		regaddr[(reg - 0x2000) / 4]
#define SPXg_REG(reg)		regaddr[(reg - 0x2000) / 2]

#define SPX_REG1(reg)		regaddr1[(reg) / 4]
#define SPXg_REG1(reg)		regaddr1[(reg) / 4]

/* few SPX register names */
#define SPX_COMMAND		0x21ec
#define SPX_XSTART		0x21d0
#define SPX_YSTART		0x21d4
#define SPX_XEND		0x21d8
#define SPX_YEND		0x21dc
#define SPX_DSTPIX		0x21e0
#define SPX_DSTPIX1		0x20e0
#define SPX_SRCPIX		0x21e4
#define SPX_SRCPIX1		0x20e4
#define SPX_MAIN3		0x219c
#define SPX_STRIDE		0x21e8	/* SRC | DST */ 
#define SPX_SRCMASK		0x21f0
#define SPX_DSTMASK		0x21f4
#define SPX_FG			0x2260
#define SPX_BG			0x2264
#define SPX_DESTLOOP		0x2270
#define SPX_MPC			0x21fc

/* few SPX opcodes (microcode entry points); rasterop # = opcode ? */
#define SPX_OP_FILLRECT		0x19
#define	SPX_OP_COPYRECT		0x1a

/* bt459 locations */
#define SPX_BT459_ADDRL		0x39b10000
#define SPX_BT459_ADDRH		0x39b14000
#define SPX_BT459_REG		0x39b18000
#define SPX_BT459_CMAP		0x39b1c000

/* bt460 locations */
#define SPXg_BT460_BASE_KA46	0x200f0000
#define SPXg_BT460_BASE_KA49	0x2a000000
#define SPXg_BT460_BASE		((vax_boardtype == VAX_BTYP_46) ? \
				 SPXg_BT460_BASE_KA46 : SPXg_BT460_BASE_KA49)

#define	SPX_BG_COLOR		WS_DEFAULT_BG
#define	SPX_FG_COLOR		WS_DEFAULT_FG

/* 
 * cursor X = 0, Y = 0 on-screen bias
 * FIXME: BIAS for various HW types
 */
#define CUR_XBIAS		360
#define CUR_YBIAS		37

/* few BT459 & BT460 indirect register defines */
#define SPXDAC_REG_CCOLOR_1	0x181
#define SPXDAC_REG_CCOLOR_2	0x182
#define SPXDAC_REG_CCOLOR_3	0x183
#define SPXDAC_REG_ID		0x200
#define SPXDAC_REG_CMD0		0x201
#define SPXDAC_REG_CMD1		0x202
#define SPXDAC_REG_CMD2		0x203
#define SPXDAC_REG_PRM		0x204
#define SPXDAC_REG_CCR		0x300
#define SPXDAC_REG_CXLO		0x301
#define SPXDAC_REG_CXHI		0x302
#define SPXDAC_REG_CYLO		0x303
#define SPXDAC_REG_CYHI		0x304
#define SPXDAC_REG_WXLO		0x305
#define SPXDAC_REG_WXHI		0x306
#define SPXDAC_REG_WYLO		0x307
#define SPXDAC_REG_WYHI		0x308
#define SPXDAC_REG_WWLO		0x309
#define SPXDAC_REG_WWHI		0x30a
#define SPXDAC_REG_WHLO		0x30b
#define SPXDAC_REG_WHHI		0x30c
#define SPXDAC_REG_CRAM_BASE	0x400

/*
 * used to access top/bottom 8 bits of a 16-bit argument for split RAMDAC
 * addressing
 */
#define HI(x)	(((x) >> 8) & 0xff)
#define LO(x)	((x) & 0xff)

static	int	spx_match(device_t, cfdata_t, void *);
static	void	spx_attach(device_t, device_t, void *);

struct	spx_softc {
	device_t ss_dev;
};

CFATTACH_DECL_NEW(spx, sizeof(struct spx_softc),
		  spx_match, spx_attach, NULL, NULL);

static void	spx_cursor(void *, int, int, int);
static int	spx_mapchar(void *, int, unsigned int *);
static void	spx_putchar(void *, int, int, u_int, long);
static void	spx_copycols(void *, int, int, int,int);
static void	spx_erasecols(void *, int, int, int, long);
static void	spx_copyrows(void *, int, int, int);
static void	spx_eraserows(void *, int, int, long);
static int	spx_allocattr(void *, int, int, int, long *);
static int	spx_get_cmap(struct wsdisplay_cmap *);
static int	spx_set_cmap(struct wsdisplay_cmap *);

static int	spx_map_regs(device_t, u_int, u_int, u_int, u_int);
static void	SPX_render_font(void);
static void	SPXg_render_font(void);
static int	SPX_map_fb(device_t, struct vsbus_attach_args *);
static int	SPXg_map_fb(device_t, struct vsbus_attach_args *);
static void	spx_init_common(device_t, struct vsbus_attach_args *);
	
#define SPX_MAP_FB(self, va, type) if (! type ## _map_fb(self, va)) return   
#define SPX_MAP_REGS(self, type) if (!spx_map_regs(self,		\
						   type ## _REG_ADDR,	\
						   type ## _REG_SIZE,	\
						   type ## _REG1_ADDR,	\
						   type ## _REG1_SIZE)) \
					return

const struct wsdisplay_emulops spx_emulops = {
	spx_cursor,
	spx_mapchar,
	spx_putchar,
	spx_copycols,
	spx_erasecols,
	spx_copyrows,
	spx_eraserows,
	spx_allocattr
};

static char spx_stdscreen_name[10] = "160x128";
struct wsscreen_descr spx_stdscreen = {
	spx_stdscreen_name, 160, 128,		/* dynamically set */
	&spx_emulops,
	8, 15,					/* dynamically set */
	WSSCREEN_UNDERLINE|WSSCREEN_REVERSE|WSSCREEN_WSCOLORS,
};

const struct wsscreen_descr *_spx_scrlist[] = {
	&spx_stdscreen,
};

const struct wsscreen_list spx_screenlist = {
	sizeof(_spx_scrlist) / sizeof(struct wsscreen_descr *),
	_spx_scrlist,
};

static volatile char *spxaddr;
static volatile long *regaddr;
static volatile long *regaddr1;

static volatile char *bt_addrl;
static volatile char *bt_addrh;
static volatile char *bt_reg;
static volatile char *bt_cmap;
static volatile char *bt_wait; /* SPXg/gt only */

static int	spx_xsize;
static int	spx_ysize;
static int	spx_depth;
static int	spx_cols;
static int	spx_rows;
static long	spx_fb_size;

/* Our current hardware colormap */
static struct hwcmap256 {
#define	CMAP_SIZE	256	/* 256 R/G/B entries */
	u_int8_t r[CMAP_SIZE];
	u_int8_t g[CMAP_SIZE];
	u_int8_t b[CMAP_SIZE];
} spx_cmap;

/* The default colormap */
static struct {
	u_int8_t r[8];
	u_int8_t g[8];
	u_int8_t b[8];
} spx_default_cmap = {
	{ 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff },
	{ 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0xff },
	{ 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff }
};

static char fb_type = 0;
static char spxg_current_page = 0;
#define spxg_switch_page(nr) (SPXg_REG1(0x20) = ((1 << (nr) & 0xff)) << 16)

struct wsdisplay_font spx_font;
static u_char *qf;
static u_short *qf2;

#define QCHAR_INVALID(c)     (((c) < spx_font.firstchar) ||		\
			      ((c) >= spx_font.firstchar + spx_font.numchars))
#define QCHAR(c)	     (QCHAR_INVALID(c) ? 0 : (c) - spx_font.firstchar)

#define QFONT_INDEX(c, line) (QCHAR(c) * spx_font.fontheight + line)
#define QFONT_QF(c, line)    qf[QFONT_INDEX(c, line)]
#define QFONT_QF2(c, line)   qf2[QFONT_INDEX(c, line)]
#define QFONT(c, line)       (spx_font.stride == 2 ? \
			      QFONT_QF2(c, line) :   \
			      QFONT_QF(c, line))

/* replicate color value */
#define COLOR(x) (((x) << 24) | ((x) << 16) | ((x) << 8) | (x))

/* convert coordinates into linear offset */
#define LINEAR(x, y) ((y * spx_xsize) + x)

/* Linear pixel address */
#define SPX_POS(row, col, line, dot)		   \
	((col) * spx_font.fontwidth +		   \
	 (row) * spx_font.fontheight * spx_xsize + \
	 (line) * spx_xsize + dot)

#define	SPX_ADDR(row, col, line, dot) spxaddr[SPX_POS(row, col, line, dot)]

/* Recalculate absolute pixel address from linear address */
#define SPXg_ADDR(linear) spxaddr[((((linear) % SPXg_WIN_LINEAR) / 4) * 8) + \
				  (((linear) % SPXg_WIN_LINEAR) % 4)]


static int	spx_ioctl(void *, void *, u_long, void *, int, struct lwp *);
static paddr_t	spx_mmap(void *, void *, off_t, int);
static int	spx_alloc_screen(void *, const struct wsscreen_descr *,
				      void **, int *, int *, long *);
static void	spx_free_screen(void *, void *);
static int	spx_show_screen(void *, void *, int, void (*) (void *, int, int),
				void *);

static void	spx_update_cmap(int entry, char red, char green, char blue);
static void	set_btreg(u_int addr, u_char value);
static u_char	get_btreg(u_int addr);

/* SPXg/gt delay */
static void	spxg_delay(void);

/*
 * SPX HW accelerated block copy
 * common code in spx_blkcpy,
 * HW-specific code accessed through spx_blkcpy_func pointer
 */
static void	spx_blkcpy(u_int sxpos, u_int sypos,
			   u_int dxpos, u_int dypos,
			   u_int xdim, u_int ydim);

static void	SPX_blkcpy(u_int sxpos, u_int sypos,
			   u_int dxpos, u_int dypos,
			   u_int xdim, u_int ydim,
			   char direction);
static void	SPXg_blkcpy(u_int sxpos, u_int sypos,
			    u_int dxpos, u_int dypos,
			    u_int xdim, u_int ydim,
			    char direction);
static void	(*spx_blkcpy_func)(u_int, u_int,
				   u_int, u_int,
				   u_int, u_int,
				   char);

/* SPX HW accelerated block set, no common code */
static void	SPX_blkset(u_int xpos, u_int ypos,
			   u_int xdim, u_int ydim, char color);
static void	SPXg_blkset(u_int xpos, u_int ypos,
			    u_int xdim, u_int ydim, char color);
static void	(*spx_blkset_func)(u_int, u_int, u_int, u_int, char);
#define spx_blkset(x, y, xd, yd, c) spx_blkset_func(x, y, xd, yd, c)

/* SPX HW accelerated part of spx_putchar */
static void	SPX_putchar(int, int, u_int, char, char);
static void	SPXg_putchar(int, int, u_int, char, char);
static void	(*spx_putchar_func)(int, int, u_int, char, char);

const struct wsdisplay_accessops spx_accessops = {
	spx_ioctl,
	spx_mmap,
	spx_alloc_screen,
	spx_free_screen,
	spx_show_screen,
	0 /* load_font */
};

/* TODO allocate ss_image dynamically for consoles beyond first one */
struct	spx_screen {
	int	ss_curx;
	int	ss_cury;
	int	ss_cur_fg;
	int	ss_cur_bg;
	struct {
		u_char	data;	/* Image character */
		u_char	attr;	/* Attribute: 80/70/08/07 */
	} ss_image[160 * 128];	/* allow for maximum possible cell matrix */
};

#define	SPX_ATTR_UNDERLINE	0x80
#define	SPX_BG_MASK		0x70
#define	SPX_ATTR_REVERSE	0x08
#define	SPX_FG_MASK		0x07

static	struct spx_screen spx_conscreen;
static	struct spx_screen *prevscr, *curscr;
static	struct spx_screen *savescr;

static	int cur_fg, cur_bg;
static	int spx_off = 1;

static void
spx_update_cmap(int entry, char red, char green, char blue)
{
	*bt_addrl = LO(entry);
	*bt_addrh = HI(entry);
	if ((entry >= 0x181) && (entry <= 0x183)) {
		*bt_reg = red;
		*bt_reg = green;
		*bt_reg = blue;
	} else {
		*bt_cmap = red;
		*bt_cmap = green;
		*bt_cmap = blue;
	}
}

static void
set_btreg(u_int addr, u_char value)
{
	*bt_addrl = LO(addr);
	*bt_addrh = HI(addr);
	*bt_reg = value;
}

static u_char
get_btreg(u_int addr)
{
	*bt_addrl = LO(addr);
	*bt_addrh = HI(addr);
	return *bt_reg & 0xff;
}

static void
spxg_delay(void)
{
	char i;
	
	for (i = 0; i <= SPXg_DELAY; i++)
		*bt_wait = *bt_wait + i;
}

static void
SPX_blkcpy(u_int sxpos, u_int sypos,
	   u_int dxpos, u_int dypos,
	   u_int xdim, u_int ydim,
	   char direction)
{
	u_int counter = 0xffffe;
	long temp = 0;

	SPX_REG(SPX_COMMAND) = 0x84884648 | direction; /* 0x848c4648? */
	SPX_REG(SPX_XSTART) = dxpos << 16;
	SPX_REG(SPX_YSTART) = dypos << 16;
	SPX_REG(SPX_XEND) = (dxpos + xdim) << 16;
	SPX_REG(SPX_YEND) = (dypos + ydim) << 16;
	
	temp = ((SPX_REG1(0x10) & 0xc0) << (30 - 6)) |		\
		((SPX_REG1(0x10) & 0x3f) << 24);
	
	SPX_REG(SPX_DSTPIX) = temp + LINEAR(dxpos, dypos);
	SPX_REG(SPX_SRCPIX) = temp + LINEAR(sxpos, sypos);
	SPX_REG(SPX_SRCPIX1) = 0;
	SPX_REG(SPX_STRIDE) = (spx_xsize << 16) | spx_xsize;
	SPX_REG(SPX_SRCMASK) = 0xff;
	SPX_REG(SPX_DSTMASK) = 0xff;
	
	SPX_REG(SPX_DESTLOOP) = 0x00112003;
	SPX_REG(SPX_MPC) = 0x2000 | SPX_OP_COPYRECT;
	
	SPX_REG1(0x18) = 0xffffffff;
	while ((counter) && ((SPX_REG1(0x18) & 2) == 0))
		counter--;
}

static void
SPXg_blkcpy(u_int sxpos, u_int sypos,
	    u_int dxpos, u_int dypos,
	    u_int xdim, u_int ydim,
	    char direction)
{
	u_int counter = 0xffffe;
	long temp = 0;

	SPXg_REG(SPX_COMMAND) = 0x84884648 | direction; spxg_delay();
	SPXg_REG(SPX_XSTART) = dxpos << 16; spxg_delay();
	SPXg_REG(SPX_YSTART) = dypos << 16; spxg_delay();
	SPXg_REG(SPX_XEND) = (dxpos + xdim) << 16; spxg_delay();
	SPXg_REG(SPX_YEND) = (dypos + ydim) << 16; spxg_delay();

	temp = 0x3f << 24;

	SPXg_REG(SPX_DSTPIX) = temp + LINEAR(dxpos, dypos); spxg_delay();
	SPXg_REG(SPX_DSTPIX1) = 0xff000000; spxg_delay();
	SPXg_REG(SPX_SRCPIX) = temp + LINEAR(sxpos, sypos); spxg_delay();
	SPXg_REG(SPX_SRCPIX1) = 0; spxg_delay();
	SPXg_REG(SPX_STRIDE) = (spx_xsize << 16) | spx_xsize; spxg_delay();
	SPXg_REG(SPX_SRCMASK) = 0xffffffff; spxg_delay();
	SPXg_REG(SPX_DSTMASK) = 0xffffffff; spxg_delay();

	SPXg_REG(SPX_DESTLOOP) = 0x00112003; spxg_delay();
	SPXg_REG(SPX_MPC) = 0x2000 | SPX_OP_COPYRECT; spxg_delay();

	while ((counter) && ((SPXg_REG1(0x0) & 0x8000) == 0))
		counter--;
}

static void
spx_blkcpy(u_int sxpos, u_int sypos,
	   u_int dxpos, u_int dypos,
	   u_int xdim, u_int ydim)
{
	char direction = 0;

	/* don't waste time checking whether src and dest actually overlap */
	if (sxpos < dxpos) {
		direction |= 0x01; /* X decrement */
		sxpos += xdim;
		dxpos += xdim;
		xdim = -xdim;
	}
	if (sypos < dypos) {
		direction |= 0x02; /* Y decrement */
		sypos += ydim;
		dypos += ydim;
		ydim = -ydim;
	}

	spx_blkcpy_func(sxpos, sypos, dxpos, dypos, xdim, ydim, direction);
}

static void
SPX_blkset(u_int xpos, u_int ypos, u_int xdim, u_int ydim, char color)
{
	u_int counter = 0xfffe;
	long temp = 0;

	SPX_REG(SPX_COMMAND) = 0x84884608;
	SPX_REG(SPX_XSTART) = xpos << 16;
	SPX_REG(SPX_YSTART) = ypos << 16;
	SPX_REG(SPX_XEND) = (xpos + xdim) << 16;
	SPX_REG(SPX_YEND) = (ypos + ydim) << 16;
	SPX_REG(SPX_STRIDE) = (spx_xsize << 16) | spx_xsize;
	SPX_REG(SPX_DESTLOOP) = 0x20102003;

	temp = ((SPX_REG1(0x10) & 0xc0) << (30 - 6)) | 
		((SPX_REG1(0x10) & 0x3f) << 24);
	SPX_REG(SPX_DSTPIX) = temp + LINEAR(xpos, ypos);

	SPX_REG(SPX_FG) = COLOR(color);

	SPX_REG(SPX_MPC) = 0x2000 | SPX_OP_FILLRECT;

	SPX_REG1(0x18) = 0xffffffff;
	while ((counter) && ((SPX_REG1(0x18) & 2) == 0))
		counter--;
}

static void
SPXg_blkset(u_int xpos, u_int ypos, u_int xdim, u_int ydim, char color)
{
	u_int counter = 0xfffe;
	long temp = 0;

	SPXg_REG(SPX_COMMAND) = 0x84884608; spxg_delay();
	SPXg_REG(SPX_XSTART) = xpos << 16; spxg_delay();
	SPXg_REG(SPX_YSTART) = ypos << 16; spxg_delay();
	SPXg_REG(SPX_XEND) = (xpos + xdim) << 16; spxg_delay();
	SPXg_REG(SPX_YEND) = (ypos + ydim) << 16; spxg_delay();
	SPXg_REG(SPX_STRIDE) = (spx_xsize << 16) | spx_xsize; spxg_delay();

	temp = 0x3f << 24;
	SPXg_REG(SPX_DSTPIX) = temp + LINEAR(xpos, ypos); spxg_delay();
	SPXg_REG(SPX_DSTPIX1) = 0xff000000; spxg_delay();

	SPXg_REG(SPX_FG) = COLOR(color); spxg_delay();

	SPXg_REG(SPX_DESTLOOP) = 0x20102003; spxg_delay();
	SPXg_REG(SPX_MPC) = 0x2000 | SPX_OP_FILLRECT; spxg_delay();

	while ((counter) && ((SPXg_REG1(0x0) & 0x8000) == 0))
		counter--;
}

int spx_match(device_t parent, cfdata_t match, void *aux)
{
	struct vsbus_softc *sc = device_private(parent);
#if 0
	struct vsbus_attach_args *va = aux;
	char *ch = (char *)va->va_addr;
#endif

/*
 * FIXME:
 * add KA46 when/if ever somebody reports SPXg vax_confdata ID on VS 4000/60
 * Ditto for SPX ID on KA42 & KA43
 */
	if (vax_boardtype != VAX_BTYP_49)
		return 0;

	/* KA49: Identify framebuffer type */
	switch (vax_confdata & (CONF_LCSPX | CONF_SPXg)) {
		case CONF_LCSPX:
			fb_type = FB_IS_SPX;
			spx_blkcpy_func = SPX_blkcpy;
			spx_blkset_func = SPX_blkset;
			spx_putchar_func = SPX_putchar;
			break;
		case CONF_SPXg:
			fb_type = FB_IS_SPXg;
			spx_blkcpy_func = SPXg_blkcpy;
			spx_blkset_func = SPXg_blkset;
			spx_putchar_func = SPXg_putchar;
			break;
		case 0: 
			aprint_error("spx_match: no framebuffer found\n");
			break;
		case CONF_LCSPX | CONF_SPXg:
			panic("spx_match: incorrect FB configuration\n");
			break;
	}

/* FIXME add RAMDAC ID code ??? */
#if 0
	/* 
	 * FIXME:
	 * are memory addresses besides va->va_addr off limits at this time ?
	 * RAMDAC ID register test, should read 0x4a for LCSPX, 0x4b for SPXg
	 */
	if (get_btreg(SPXDAC_REG_ID) != 0x4a)
		return 0;
#endif

	sc->sc_mask = 0x04; /* XXX - should be generated */
	scb_fake(0x14c, 0x15);	

	return 20;
}

static void
spx_attach(device_t parent, device_t self, void *aux)
{
	struct spx_softc *sc = device_private(self);
	struct vsbus_attach_args *va = aux;
	struct wsemuldisplaydev_attach_args aa;

	sc->ss_dev = self;

	aprint_normal("\n");
	aa.console = spxaddr != NULL;

	spx_init_common(self, va);

	curscr = &spx_conscreen;
	prevscr = curscr;

	aa.scrdata = &spx_screenlist;
	aa.accessops = &spx_accessops;

	/* enable cursor */
	set_btreg(SPXDAC_REG_CCR, 0xc1);

	config_found(self, &aa, wsemuldisplaydevprint);
}

static	int cur_on;

static void
spx_cursor(void *id, int on, int row, int col)
{
	struct spx_screen *ss = id;
	int attr, data;

	attr = ss->ss_image[row * spx_cols + col].attr;
	data = ss->ss_image[row * spx_cols + col].data;

	if (attr & SPX_ATTR_REVERSE) {
		cur_bg = attr & SPX_FG_MASK;
		cur_fg = (attr & SPX_BG_MASK) >> 4;
	} else {
		cur_fg = attr & SPX_FG_MASK;
		cur_bg = (attr & SPX_BG_MASK) >> 4;
	}

	if (ss == curscr) {
#ifdef SOFTCURSOR
		if ((cur_on = on))
			spx_putchar_func(row, col, data, cur_bg, cur_fg);
		else
			spx_putchar_func(row, col, data, cur_fg, cur_bg);
#else
		/* change cursor foreground color colormap entry */
		spx_update_cmap(SPXDAC_REG_CCOLOR_3, 
				spx_cmap.r[cur_fg],
				spx_cmap.g[cur_fg],
				spx_cmap.b[cur_fg]);
		
		/* update cursor position registers */
		set_btreg(SPXDAC_REG_CXLO,
			  LO(col * spx_font.fontwidth + CUR_XBIAS));
		set_btreg(SPXDAC_REG_CXHI,
			  HI(col * spx_font.fontwidth + CUR_XBIAS));
		set_btreg(SPXDAC_REG_CYLO,
			  LO(row * spx_font.fontheight + CUR_YBIAS));
		set_btreg(SPXDAC_REG_CYHI,
			  HI(row * spx_font.fontheight + CUR_YBIAS));
		
		if ((cur_on = on))
			/* enable cursor */
			set_btreg(SPXDAC_REG_CCR, 0xc1);
		else
			/* disable cursor */
			set_btreg(SPXDAC_REG_CCR, 0x01);
#endif
	}

	ss->ss_curx = col;
	ss->ss_cury = row;
	ss->ss_cur_bg = cur_bg;
	ss->ss_cur_fg = cur_fg;
}

static int
spx_mapchar(void *id, int uni, unsigned int *index)
{
	if (uni < 256) {
		*index = uni;
		return (5);
	}
	*index = ' ';
	return (0);
}

static void
SPX_putchar(int row, int col, u_int c, char dot_fg, char dot_bg)
{
	u_int counter = 0xffffe;
	long temp = 0;

	SPX_REG(SPX_FG) = COLOR(dot_fg);
	SPX_REG(SPX_BG) = COLOR(dot_bg);
	SPX_REG(SPX_MAIN3) = 0x01010101;
	SPX_REG(SPX_COMMAND) = 0x84884648; 
	SPX_REG(SPX_XSTART) = (col * spx_font.fontwidth) << 16;
	SPX_REG(SPX_YSTART) = (row * spx_font.fontheight) << 16;
	SPX_REG(SPX_XEND) = ((col + 1) * spx_font.fontwidth) << 16;
	SPX_REG(SPX_YEND) = ((row + 1) * spx_font.fontheight) << 16;

	temp = ((SPX_REG1(0x10) & 0xc0) << (30 - 6)) |	\
		((SPX_REG1(0x10) & 0x3f) << 24);

	SPX_REG(SPX_DSTPIX) = temp + LINEAR(col * spx_font.fontwidth,
					    row * spx_font.fontheight);
	SPX_REG(SPX_SRCPIX) = temp + FONT_STORAGE_START
		+ (c * spx_font.fontheight * spx_font.fontwidth);
	SPX_REG(SPX_SRCPIX1) = 0;
	SPX_REG(SPX_STRIDE) = (spx_font.fontwidth << 16) | spx_xsize;
	SPX_REG(SPX_SRCMASK) = 0xff;
	SPX_REG(SPX_DSTMASK) = 0xff;
	
	SPX_REG(SPX_DESTLOOP) = 0x20142003;
	SPX_REG(SPX_MPC) = 0x2000 | SPX_OP_COPYRECT;
	
	SPX_REG1(0x18) = 0xffffffff;

	while ((counter) && ((SPX_REG1(0x18) & 2) == 0))
		counter--;
}

static void
SPXg_putchar(int row, int col, u_int c, char dot_fg, char dot_bg)
{
	u_int counter = 0xffffe;
	long temp = 0;

	SPXg_REG(SPX_FG) = COLOR(dot_fg);
	spxg_delay();
	SPXg_REG(SPX_BG) = COLOR(dot_bg);
	spxg_delay();
	SPXg_REG(SPX_MAIN3) = 0x01010101;
	spxg_delay();
	SPXg_REG(SPX_COMMAND) = 0x84884648;
	spxg_delay();
	SPXg_REG(SPX_XSTART) = (col * spx_font.fontwidth) << 16;
	spxg_delay();
	SPXg_REG(SPX_YSTART) = (row * spx_font.fontheight) << 16;
	spxg_delay();
	SPXg_REG(SPX_XEND) = ((col + 1) * spx_font.fontwidth) << 16;
	spxg_delay();
	SPXg_REG(SPX_YEND) = ((row + 1) * spx_font.fontheight) << 16;
	spxg_delay();

	temp = 0x3f << 24;

	SPXg_REG(SPX_DSTPIX) = temp + LINEAR(col * spx_font.fontwidth,
					     row * spx_font.fontheight);
	spxg_delay();
	SPXg_REG(SPX_DSTPIX1) = 0xff000000;
	spxg_delay();
	SPXg_REG(SPX_SRCPIX) = temp + FONT_STORAGE_START +
		(c * spx_font.fontheight * spx_font.fontwidth);
	spxg_delay();
	SPXg_REG(SPX_SRCPIX1) = 0;
	spxg_delay();
	SPXg_REG(SPX_STRIDE) = (spx_font.fontwidth << 16) | spx_xsize;
	spxg_delay();
	SPXg_REG(SPX_SRCMASK) = 0xffffffff;
	spxg_delay();
	SPXg_REG(SPX_DSTMASK) = 0xffffffff;
	spxg_delay();

	SPXg_REG(SPX_DESTLOOP) = 0x20142003;
	spxg_delay();
	SPXg_REG(SPX_MPC) = 0x2000 | SPX_OP_COPYRECT;
	spxg_delay();

	while ((counter) && ((SPXg_REG1(0x0) & 0x8000) == 0))
		counter--;
}

static void
spx_putchar(void *id, int row, int col, u_int c, long attr)
{
	struct spx_screen *ss = id;
	char dot_fg, dot_bg;

	c &= 0xff;

	ss->ss_image[row * spx_cols + col].data = c;
	ss->ss_image[row * spx_cols + col].attr = attr;
	if (ss != curscr)
		return;

	dot_fg = attr & SPX_FG_MASK;
	dot_bg = (attr & SPX_BG_MASK) >> 4;
	if (attr & SPX_ATTR_REVERSE) {
		dot_fg = (attr & SPX_BG_MASK) >> 4;
		dot_bg = attr & SPX_FG_MASK;
	}

	/* adjust glyph index for underlined text */
	if (attr & SPX_ATTR_UNDERLINE)
		c += 0x100;

	spx_putchar_func(row, col, c, dot_fg, dot_bg);
}

/*
 * copies columns inside a row.
 */
static void
spx_copycols(void *id, int row, int srccol, int dstcol, int ncols)
{
	struct spx_screen *ss = id;

	bcopy(&ss->ss_image[row * spx_cols + srccol],
	      &ss->ss_image[row * spx_cols + dstcol],
	      ncols * sizeof(ss->ss_image[0]));

	if (ss == curscr)
		spx_blkcpy(srccol * spx_font.fontwidth,
			   row * spx_font.fontheight,
			   dstcol * spx_font.fontwidth,
			   row * spx_font.fontheight,
			   ncols * spx_font.fontwidth,
			   spx_font.fontheight);
}

/*
 * Erases a bunch of chars inside one row.
 */
static void
spx_erasecols(void *id, int row, int startcol, int ncols, long fillattr)
{
	struct spx_screen *ss = id;
	int offset = row * spx_cols + startcol;
	int i;

	memset(&ss->ss_image[row * spx_cols + startcol], 0, 
	      ncols * sizeof(ss->ss_image[0]));

	for (i = offset; i < offset + ncols; ++i)
		ss->ss_image[i].attr = fillattr;

	if (ss == curscr)
		spx_blkset(startcol * spx_font.fontwidth,
			   row * spx_font.fontheight,
			   ncols * spx_font.fontwidth,
			   spx_font.fontheight,
			   (fillattr & SPX_BG_MASK) >> 4);
}

static void
spx_copyrows(void *id, int srcrow, int dstrow, int nrows)
{
	struct spx_screen *ss = id;

	bcopy(&ss->ss_image[srcrow * spx_cols],
	      &ss->ss_image[dstrow * spx_cols],
	      nrows * spx_cols * sizeof(ss->ss_image[0]));

	if (ss == curscr)
		spx_blkcpy(0, (srcrow * spx_font.fontheight),
			   0, (dstrow * spx_font.fontheight),
			   spx_xsize, (nrows * spx_font.fontheight));
}

static void
spx_eraserows(void *id, int startrow, int nrows, long fillattr)
{
	struct spx_screen *ss = id;
	int i;
	
	memset(&ss->ss_image[startrow * spx_cols], 0,
	      nrows * spx_cols * sizeof(ss->ss_image[0]));

	for (i = startrow * spx_cols; i < startrow + nrows * spx_cols; ++i)
		ss->ss_image[i].attr = fillattr;

	if (ss == curscr)
		spx_blkset(0, (startrow * spx_font.fontheight),
			   spx_xsize, (nrows * spx_font.fontheight),
			   (fillattr & SPX_BG_MASK) >> 4);
}

static int
spx_allocattr(void *id, int fg, int bg, int flags, long *attrp)
{
	long myattr;

	if (flags & WSATTR_WSCOLORS)
		myattr = (fg & SPX_FG_MASK) | ((bg << 4) & SPX_BG_MASK);
	else
		myattr = WSCOL_WHITE << 4;	/* XXXX */
	if (flags & WSATTR_REVERSE)
		myattr |= SPX_ATTR_REVERSE;
	if (flags & WSATTR_UNDERLINE)
		myattr |= SPX_ATTR_UNDERLINE;
	*attrp = myattr;
	return 0;
}

static int
spx_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct wsdisplay_fbinfo *fb = data;
	int i;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_SPX;
		break;

	case WSDISPLAYIO_GINFO:
		fb->height = spx_ysize;
		fb->width = spx_xsize;
		fb->depth = spx_depth;
		fb->cmsize = 1 << spx_depth;
		break;

	case WSDISPLAYIO_GETCMAP:
		return spx_get_cmap((struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_PUTCMAP:
		return spx_set_cmap((struct wsdisplay_cmap *)data);

	case WSDISPLAYIO_GMODE:
		return EPASSTHROUGH;

	case WSDISPLAYIO_SMODE:
		/*
		 * if setting WSDISPLAYIO_MODE_EMUL, restore console cmap,
		 * current screen
		 */
		if (*(u_int *)data == WSDISPLAYIO_MODE_EMUL) {
			for (i = 0; i < 8; i++) {
				spx_cmap.r[i] = spx_default_cmap.r[i];
				spx_cmap.g[i] = spx_default_cmap.g[i];
				spx_cmap.b[i] = spx_default_cmap.b[i];
				spx_update_cmap(i, spx_cmap.r[i], spx_cmap.g[i],
						spx_cmap.b[i]);
			}
			if (savescr != NULL)
				spx_show_screen(NULL, savescr, 0, NULL, NULL);
			savescr = NULL;
		} else {		/* WSDISPLAYIO_MODE_MAPPED */
			savescr = curscr;
			curscr = NULL;
			/* clear screen? */
		}

		return EPASSTHROUGH;

	case WSDISPLAYIO_SVIDEO:
		if (*(u_int *)data == WSDISPLAYIO_VIDEO_ON && spx_off) {
			/* Unblank video */
			set_btreg(SPXDAC_REG_CMD0, 0x48);

			/* Enable sync */
			set_btreg(SPXDAC_REG_CMD2, 0xc0);

			spx_off = 0;
		} else if (*(u_int *)data == WSDISPLAYIO_VIDEO_OFF && !spx_off) {
			/* Blank video */
			set_btreg(SPXDAC_REG_CMD0, 68);

			/* Disable sync */
			set_btreg(SPXDAC_REG_CMD2, 0x40);
			spx_off = 1;
		}
		break;

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = spx_off == 0 ?
		WSDISPLAYIO_VIDEO_ON : WSDISPLAYIO_VIDEO_OFF;
		break;

	default:
		return EPASSTHROUGH;
	}
	return 0;
}

/*
 * Bad idea on SPXg/gt due to windowed framebuffer access 
 */
static paddr_t
spx_mmap(void *v, void *vs, off_t offset, int prot)
{
	if (fb_type != FB_IS_SPX)
		return -1;

	if (offset >= spx_fb_size || offset < 0)
		return -1;

	return (SPX_FB_ADDR + offset) >> PGSHIFT;
}

static int
spx_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
		 int *curxp, int *curyp, long *defattrp)
{
	int i;
	struct spx_screen *ss;

	ss = malloc(sizeof(struct spx_screen), M_DEVBUF, M_WAITOK | M_ZERO);

	*cookiep = ss;
	*curxp = *curyp = 0;
	*defattrp = (SPX_BG_COLOR << 4) | SPX_FG_COLOR;

	for (i = 0; i < spx_cols * spx_rows; ++i)
		ss->ss_image[i].attr = *defattrp;

	return 0;
}

static void
spx_free_screen(void *v, void *cookie)
{
/* FIXME add something to actually free malloc()ed screen ? */
}

static int
spx_show_screen(void *v, void *cookie, int waitok,
		void (*cb)(void *, int, int), void *cbarg)
{
	struct spx_screen *ss = cookie;
	u_int row, col, attr;
	u_char c;

	if (ss == curscr)
		return (0);

	prevscr = curscr;
	curscr = ss;

	for (row = 0; row < spx_rows; row++) {
		for (col = 0; col < spx_cols; col++) {
			u_int idx = row * spx_cols + col;
			attr = ss->ss_image[idx].attr;
			c = ss->ss_image[idx].data;

			/* check if cell requires updating */
			if ((c != prevscr->ss_image[idx].data) ||
			    (attr != prevscr->ss_image[idx].attr)) {
				if (c < 32) c = 32;
				spx_putchar(ss, row, col, c, attr);
			}
		}
	}

	row = ss->ss_cury; col = ss->ss_curx;

	spx_cursor(ss, cur_on, row, col);

	cur_fg = ss->ss_cur_fg;
	cur_bg = ss->ss_cur_bg;

	return (0);
}

static int
spx_get_cmap(struct wsdisplay_cmap *p)
{
	u_int index = p->index, count = p->count;
	int error;

	if (index >= (1 << spx_depth) || count > (1 << spx_depth) - index)
		return (EINVAL);

	error = copyout(&spx_cmap.r[index], p->red, count);
	if (error)
		return error;

	error = copyout(&spx_cmap.g[index], p->green, count);
	if (error)
		return error;

	error = copyout(&spx_cmap.b[index], p->blue, count);
	return error;
}

static int
spx_set_cmap(struct wsdisplay_cmap *p)
{
	u_int index = p->index, count = p->count;
	int error, s;

	if (index >= (1 << spx_depth) || count > (1 << spx_depth) - index)
		return (EINVAL);

	error = copyin(p->red, &spx_cmap.r[index], count);
	if (error)
		return error;

	error = copyin(p->green, &spx_cmap.g[index], count);
	if (error)
		return error;

	error = copyin(p->blue, &spx_cmap.b[index], count);

	if (error)
		return error;

	s = spltty();
	while (count-- > 0) {
		spx_update_cmap(index, 
				spx_cmap.r[index],
				spx_cmap.g[index],
				spx_cmap.b[index]);
		index++;
	}
	splx(s);
	return (0);
}

cons_decl(spx);

void
spxcninit(struct consdev *cndev)
{
	int i;

	spx_blkset(0, 0, spx_xsize, spx_ysize, SPX_BG_COLOR);

	curscr = &spx_conscreen;
	
	for (i = 0; i < spx_cols * spx_rows; i++)
		spx_conscreen.ss_image[i].attr =
			(SPX_BG_COLOR << 4) | SPX_FG_COLOR;
	wsdisplay_cnattach(&spx_stdscreen, &spx_conscreen, 0, 0,
			   (SPX_BG_COLOR << 4) | SPX_FG_COLOR);
	cn_tab->cn_pri = CN_INTERNAL;

#if NDZKBD > 0
	dzkbd_cnattach(0); /* Connect keyboard and screen together */
#endif
}

/*
 * Called very early to setup the glass tty as console.
 * Because it's called before the VM system is inited, virtual memory
 * for the framebuffer can be stolen directly without disturbing anything.
 */
void
spxcnprobe(struct consdev *cndev)
{
	extern const struct cdevsw wsdisplay_cdevsw;

	/* Only for VS 4000/90, 90A and 96 with LCSPX or SPXg/gt*/
	if ((vax_boardtype != VAX_BTYP_49) ||
	    ((vax_confdata & (CONF_LCSPX | CONF_SPXg)) != 0))
		return;

	if (((vax_confdata & 8) && (vax_boardtype == VAX_BTYP_49)) ||
	    (((vax_confdata & KA420_CFG_L3CON) ||
	      (vax_confdata & KA420_CFG_MULTU)) && 
	     ((vax_boardtype == VAX_BTYP_420) || 
	      (vax_boardtype == VAX_BTYP_43)))) {
		aprint_normal("spxcnprobe: Diagnostic console\n");
		return; /* Diagnostic console */
	}

	/* KA49: Identify framebuffer type */
	switch (vax_confdata & (CONF_LCSPX | CONF_SPXg)) {
	case CONF_LCSPX:
		fb_type = FB_IS_SPX;
		spx_blkcpy_func = SPX_blkcpy;
		spx_blkset_func = SPX_blkset;
		break;
	case CONF_SPXg:
		fb_type = FB_IS_SPXg;
		spx_blkcpy_func = SPXg_blkcpy;
		spx_blkset_func = SPXg_blkset;
		break;
	case 0:
		aprint_error("spxcnprobe: no framebuffer found\n");
		break;
	case CONF_LCSPX | CONF_SPXg:
		panic("spxcnprobe: incorrect FB configuration\n"); break;
	}

	spx_init_common(NULL, NULL);

	cndev->cn_pri = CN_INTERNAL;
	cndev->cn_dev = makedev(cdevsw_lookup_major(&wsdisplay_cdevsw), 0);
}

static int
spx_map_regs(device_t self,
	     u_int raddr, u_int rsize, u_int r1addr, u_int r1size)
{
	extern vaddr_t virtual_avail;

	if (!self) {
		regaddr = (long*)virtual_avail;
		virtual_avail += rsize;
		ioaccess((vaddr_t)regaddr, raddr, rsize/VAX_NBPG);

		regaddr1 = (long*)virtual_avail;
		virtual_avail += r1size;
		ioaccess((vaddr_t)regaddr1, r1addr, r1size/VAX_NBPG);

		return 1;
	}

	regaddr = (long*)vax_map_physmem(raddr, rsize/VAX_NBPG);
	if (regaddr == 0) {
		aprint_error_dev(self,
				 "unable to allocate register memory (1).\n");
		return 0;
	}

	regaddr1 = (long*)vax_map_physmem(r1addr, r1size/VAX_NBPG);
	if (regaddr1 == 0) {
		aprint_error_dev(self,
				 "unable to allocate register memory (2).\n");
		return 0;
	}

	return 1;
}

static int
SPX_map_fb(device_t self, struct vsbus_attach_args *va)
{
	int fbsize = (spx_fb_size + FONT_STORAGE_SIZE)/VAX_NBPG;
	extern vaddr_t virtual_avail;

	if (!self) {
		spxaddr = (char *)virtual_avail;
		virtual_avail += (spx_fb_size + FONT_STORAGE_SIZE);
		ioaccess((vaddr_t)spxaddr, SPX_FB_ADDR, fbsize);

		bt_addrl = (char *)virtual_avail;
		virtual_avail += VAX_NBPG;
		ioaccess((vaddr_t)bt_addrl, SPX_BT459_ADDRL, 1);

		bt_addrh = (char *)virtual_avail;
		virtual_avail += VAX_NBPG;
		ioaccess((vaddr_t)bt_addrh, SPX_BT459_ADDRH, 1);

		bt_reg = (char *)virtual_avail;
		virtual_avail += VAX_NBPG;
		ioaccess((vaddr_t)bt_reg, SPX_BT459_REG, 1);

		bt_cmap = (char *)virtual_avail;
		virtual_avail += VAX_NBPG;
		ioaccess((vaddr_t)bt_cmap, SPX_BT459_CMAP, 1);

		return 1;
	}

	spxaddr = (char *)vax_map_physmem(va->va_paddr, fbsize);
	if (spxaddr == 0) {
		aprint_error_dev(self, "unable to map framebuffer memory.\n");
		return 0;
	}

	/* map all four RAMDAC registers */
	bt_addrl = (char *)vax_map_physmem(SPX_BT459_ADDRL, 1);
	if (bt_addrl == 0) {
		aprint_error_dev(self,
				 "unable to map BT459 Low Address register.\n");
		return 0;
	}

	bt_addrh = (char *)vax_map_physmem(SPX_BT459_ADDRH, 1);
	if (bt_addrh == 0) {
		aprint_error_dev(self,
				 "unable to map BT459 High Address register.\n");
		return 0;
	}

	bt_reg = (char *)vax_map_physmem(SPX_BT459_REG, 1);
	if (bt_reg == 0) {
		aprint_error_dev(self,
				 "unable to map BT459 I/O Register.\n");
		return 0;
	}

	bt_cmap = (char *)vax_map_physmem(SPX_BT459_CMAP, 1);
	if (bt_cmap == 0) {
		aprint_error_dev(self,
				 "unable to map BT459 Color Map register.\n");
		return 0;
	}

	return 1;
}

static int
SPXg_map_fb(device_t self, struct vsbus_attach_args *va)
{
	int fbsize = SPXg_WIN_SIZE/VAX_NBPG;
	extern vaddr_t virtual_avail;

	if (!self) {
		spxaddr = (char *)virtual_avail;
		virtual_avail += SPXg_WIN_SIZE;
		ioaccess((vaddr_t)spxaddr, SPXg_FB_ADDR, fbsize);

		bt_addrl = (char *)virtual_avail;
		virtual_avail += VAX_NBPG;
		ioaccess((vaddr_t)bt_addrl, SPXg_BT460_BASE, 1);

		bt_addrh = bt_addrl + 0x04;
		bt_reg = bt_addrl + 0x08;
		bt_cmap = bt_addrl + 0x0c;
		bt_wait = bt_addrl + 0x34;

		return 1;
	}

	spxaddr = (char *)vax_map_physmem(va->va_paddr, fbsize);
	if (spxaddr == 0) {
		aprint_error_dev(self,
				 "unable to map framebuffer memory window.\n");
		return 0;
	}
	
	bt_addrl = (char *)vax_map_physmem(SPXg_BT460_BASE, 1);
	if (bt_addrl == 0) {
		aprint_error_dev(self,
				 "unable to map BT460 Low Address register.\n");
		return 0;
	}
	bt_addrh = bt_addrl + 0x04;
	bt_reg   = bt_addrl + 0x08;
	bt_cmap  = bt_addrl + 0x0c;
	bt_wait  = bt_addrl + 0x34;

	return 1;
}

static void
SPX_render_font(void)
{
	volatile char *fontaddr = spxaddr + FONT_STORAGE_START;
	int i, j, k, ch, pixel;

	for (i = 0; i < 256; i++) for (j = 0; j < spx_font.fontheight; j++) {
		ch = QFONT(i, j);

		/* regular characters */
		pixel = ((i * spx_font.fontheight)+ j) * spx_font.fontwidth;
		for (k = 0; k < spx_font.fontwidth; k++)
			if (ch & (1 << k))
				fontaddr[pixel++] = 0xff;
			else 
				fontaddr[pixel++] = 0x00;
		
		/* underlined characters */
		pixel = (((i + 256) * spx_font.fontheight) + j)
			* spx_font.fontwidth;
		for (k = 0; k < spx_font.fontwidth; k++)
			if ((ch & (1 << k)) || (j == (spx_font.fontheight - 1)))
				fontaddr[pixel++] = 0xff;
			else
				fontaddr[pixel++] = 0x00;
	}
}

static void
SPXg_render_font(void)
{
	int i, j, k, ch, pixel;

	for (i = 0; i < 256; i++) for (j = 0; j < spx_font.fontheight; j++) {
		ch = QFONT(i, j);
		/* regular characters */
		for (k = 0; k < spx_font.fontwidth; k++) {
			pixel = FONT_STORAGE_START
				+ (((i * spx_font.fontheight) + j)
				   * spx_font.fontwidth) + k;
			if ((pixel / SPXg_WIN_LINEAR) != spxg_current_page) {
				spxg_switch_page(pixel / SPXg_WIN_LINEAR);
				spxg_current_page = (pixel / SPXg_WIN_LINEAR);
			}
			SPXg_ADDR(pixel) = ch & (1 << k) ? 0xff : 0x00;
			spxg_delay();
		}

		/* underlined characters */
		for (k = 0; k < spx_font.fontwidth; k++) {
			pixel = FONT_STORAGE_START
				+ ((((i + 256) * spx_font.fontheight) + j)
				   * spx_font.fontwidth) + k;
			if ((pixel / SPXg_WIN_LINEAR) != spxg_current_page) {
				spxg_switch_page(pixel / SPXg_WIN_LINEAR);
				spxg_current_page = (pixel / SPXg_WIN_LINEAR);
			}
			if ((ch & (1 << k)) || (j == (spx_font.fontheight - 1)))
				SPXg_ADDR(pixel) = 0xff;
			else
				SPXg_ADDR(pixel) = 0x00;
			spxg_delay();
		}
	}
}

static void
spx_init_common(device_t self, struct vsbus_attach_args *va)
{
	u_int i, j, k;
	int cookie;
	struct wsdisplay_font *wf;

	/* map SPX registers first */
	if (fb_type == FB_IS_SPX) {
		SPX_MAP_REGS(self, SPX);
	} else if(fb_type == FB_IS_SPXg) {
		SPX_MAP_REGS(self, SPXg);
	}

	if (fb_type == FB_IS_SPXg)
		spxg_switch_page(0);

	/* any KA42/43 SPX resolution detection code should be placed here */
	spx_depth = 8;
	spx_xsize = 1280;
	spx_ysize = 1024;

	wsfont_init();

	cookie = wsfont_find(NULL, 12, 21, 0, WSDISPLAY_FONTORDER_R2L,
			     WSDISPLAY_FONTORDER_L2R);
	if (cookie == -1)
		cookie = wsfont_find(NULL, 16, 0, 0, WSDISPLAY_FONTORDER_R2L, 0);
	if (cookie == -1)
		cookie = wsfont_find(NULL, 12, 0, 0, WSDISPLAY_FONTORDER_R2L, 0);
	if (cookie == -1)
		cookie = wsfont_find(NULL, 8, 0, 0, WSDISPLAY_FONTORDER_R2L, 0);
	if (cookie == -1)
		cookie = wsfont_find(NULL, 0, 0, 0, WSDISPLAY_FONTORDER_R2L,
				     WSDISPLAY_FONTORDER_L2R);

	if (cookie == -1)
		aprint_error_dev(self, "spx_common_init: cookie = -1\n");

	if (cookie == -1 || wsfont_lock(cookie, &wf))
		panic("spx_common_init: unable to load console font");

	spx_font = *wf;

	if (self != NULL) {
		aprint_normal_dev(self, "Using %s %dx%d font\n", wf->name,
				  spx_font.fontwidth, spx_font.fontheight);
	}

	spx_cols = spx_xsize / spx_font.fontwidth;
	spx_rows = spx_ysize / spx_font.fontheight;
	spx_fb_size = spx_xsize * spx_ysize;
	spx_stdscreen.ncols = spx_cols;
	spx_stdscreen.nrows = spx_rows;
	spx_stdscreen.fontwidth = spx_font.fontwidth;
	spx_stdscreen.fontheight = spx_font.fontheight;
	sprintf(spx_stdscreen_name, "%dx%d", spx_cols, spx_rows);

	/* for SPXg spx_fb_size represents FB window size, not FB length */
	if (fb_type == FB_IS_SPXg)
		spx_fb_size = SPXg_WIN_SIZE;

	qf = spx_font.data;
	qf2 = (u_short *)spx_font.data;

	if (fb_type == FB_IS_SPX) {
		SPX_MAP_FB(self, va, SPX);
	} else if (fb_type == FB_IS_SPXg) {
		SPX_MAP_FB(self, va, SPXg);
	}

	/* display FB type based on RAMDAC ID */
	switch (get_btreg(SPXDAC_REG_ID) & 0xff) {
	case 0x4a:
		aprint_normal_dev(
			self, "RAMDAC ID: 0x%x, Bt459 (SPX/LCSPX) RAMDAC type\n",
			get_btreg(SPXDAC_REG_ID) & 0xff);
		break;
			
	case 0x4b:
		aprint_normal_dev(
			self, "RAMDAC ID: 0x%x, Bt460 (SPXg) RAMDAC type\n", 
			get_btreg(SPXDAC_REG_ID) & 0xff);
		break;
	default:
		aprint_error_dev(self, "unknown RAMDAC type 0x%x\n",
				 get_btreg(SPXDAC_REG_ID) & 0xff);
	}

	/* render font glyphs to off-screen memory */
	if (fb_type == FB_IS_SPX)
		SPX_render_font();
	else if (fb_type == FB_IS_SPXg)
		SPXg_render_font();

	/* set up default console color palette */
	for (i = 0; i < 8; i++) {
		spx_cmap.r[i] = spx_default_cmap.r[i];
		spx_cmap.g[i] = spx_default_cmap.g[i];
		spx_cmap.b[i] = spx_default_cmap.b[i];
		spx_update_cmap(i, spx_cmap.r[i], spx_cmap.g[i], spx_cmap.b[i]);
	}
	
	/*
	 * Build 64x64 console cursor bitmap based on font dimensions.
	 * Palette colors are not used, but 4 pixels 2bpp of cursor sprite,
	 * fg/bg respectively.
	 */
	for (i = 0; i < 1024; i++)
		set_btreg(SPXDAC_REG_CRAM_BASE + i, 0);

	/* build cursor line, 1 to 3 pixels high depending on font height */
	if (spx_font.fontheight > 26)
		i = spx_font.fontheight - 3;
	else if (spx_font.fontheight > 16)
		i = spx_font.fontheight - 2;
	else
		i = spx_font.fontheight - 1;

	for (; i <= spx_font.fontheight - 1; i++) {
		for (j = 0; j < (spx_font.fontwidth >> 2); j++)
			set_btreg(SPXDAC_REG_CRAM_BASE + i*16 + j, 0xff);
		k = (spx_font.fontwidth & 3) << 1;
		set_btreg(SPXDAC_REG_CRAM_BASE + i*16 + j, (1 << k) - 1);
	}

	if (fb_type == FB_IS_SPX) {
		/* set SPX write/read plane masks */
		SPX_REG(0x3170) = 0xffffffff;
		SPX_REG(0x3174) = 0xffffffff;
	}

	/* RAMDAC type-specific configuration */
	if (fb_type == FB_IS_SPX)
		set_btreg(SPXDAC_REG_CMD2, 0xc0);
	
	/* common configuration */
	set_btreg(SPXDAC_REG_CMD0, 0x48);
	set_btreg(SPXDAC_REG_CMD1, 0x00);
	set_btreg(SPXDAC_REG_PRM, 0xff);

	set_btreg(SPXDAC_REG_CXLO, 0);
	set_btreg(SPXDAC_REG_CXHI, 0);
	set_btreg(SPXDAC_REG_CYLO, 0);
	set_btreg(SPXDAC_REG_CYHI, 0);

	set_btreg(SPXDAC_REG_WXLO, 0);
	set_btreg(SPXDAC_REG_WXHI, 0);
	set_btreg(SPXDAC_REG_WYLO, 0);
	set_btreg(SPXDAC_REG_WYHI, 0);

	set_btreg(SPXDAC_REG_WWLO, 0);
	set_btreg(SPXDAC_REG_WWHI, 0);
	set_btreg(SPXDAC_REG_WHLO, 0);
	set_btreg(SPXDAC_REG_WHHI, 0);
}
