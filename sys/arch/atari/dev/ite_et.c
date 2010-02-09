/*	$NetBSD: ite_et.c,v 1.28 2010/02/09 23:05:16 wiz Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ite_et.c,v 1.28 2010/02/09 23:05:16 wiz Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <dev/cons.h>

#include <machine/cpu.h>

#include <atari/atari/device.h>

#include <atari/dev/itevar.h>
#include <atari/dev/iteioctl.h>
#include <atari/dev/grfioctl.h>
#include <atari/dev/grf_etreg.h>
#include <atari/dev/grfabs_reg.h>
#include <atari/dev/grfabs_et.h>
#include <atari/dev/grfvar.h>
#include <atari/dev/font.h>
#include <atari/dev/viewioctl.h>
#include <atari/dev/viewvar.h>

#include "grfet.h"

/*
 * This is what ip->priv points to;
 * it contains local variables for custom-chip ites.
 */
struct ite_priv {
	volatile u_char	*regkva;
};

typedef struct ite_priv ipriv_t;

static ipriv_t	con_ipriv;

/* Console colors */
static u_char etconscolors[3][3] = {	/* background, foreground, hilite */
	{0x0, 0x0, 0x0}, {0x30, 0x30, 0x30}, { 0x3f,  0x3f,  0x3f}
};

/* XXX: Shouldn't these be in font.h???? */
extern font_info	font_info_8x8;
extern font_info	font_info_8x16;

static void grfet_iteinit(struct grf_softc *);
static void view_init(struct ite_softc *);
static void view_deinit(struct ite_softc *);
static int  iteet_ioctl(struct ite_softc *, u_long, void *, int,
							struct lwp *);
static int  ite_newsize(struct ite_softc *, struct itewinsize *);
static void et_inittextmode(struct ite_softc *, et_sv_reg_t *, int);
void et_cursor(struct ite_softc *ip, int flag);
void et_clear(struct ite_softc *ip, int sy, int sx, int h, int w);
void et_putc(struct ite_softc *ip, int c, int dy, int dx, int mode);
void et_scroll(struct ite_softc *ip, int sy, int sx, int count,
    int dir);

/*
 * grfet config stuff
 */
void grfetattach(struct device *, struct device *, void *);
int  grfetmatch(struct device *, struct cfdata *, void *);
int  grfetprint(void *, const char *);

CFATTACH_DECL(grfet, sizeof(struct grf_softc),
    grfetmatch, grfetattach, NULL, NULL);

/*
 * only used in console init.
 */
static struct cfdata *cfdata_grf   = NULL;

int
grfetmatch(struct device *pdp, struct cfdata *cfp, void *auxp)
{
	static int	card_probed  = -1;
	static int	did_consinit = 0;
	grf_auxp_t	*grf_auxp = auxp;
	extern const struct cdevsw view_cdevsw;

	if (card_probed <= 0) {
		if (card_probed == 0) /* Probed but failed */
			return 0;
		card_probed = 0;

		/*
		 * Check if the layers we depend on exist
		 */
		if(!(machineid & ATARI_HADES))
			return 0;
		if (!et_probe_card())
			return 0;
		if (grfabs_probe(&et_probe_video) == 0)
			return 0;
		viewprobe();
		card_probed = 1; /* Probed and found */
	}

	if (atari_realconfig == 0) {
		/*
		 * Early console init. Only match first unit.
		 */
		if (did_consinit)
			return 0;
		if ((*view_cdevsw.d_open)(cfp->cf_unit, 0, 0, NULL))
			return 0;
		cfdata_grf = cfp;
		did_consinit = 1;
		return 1;
	}

	/*
	 * Normal config. When we are called directly from the grfbus,
	 * we only match the first unit. The attach function will call us for
	 * the other configured units.
	 */
	if (grf_auxp->from_bus_match
	    && ((did_consinit > 1) || !et_probe_card()))
		return 0;

	if (!grf_auxp->from_bus_match && (grf_auxp->unit != cfp->cf_unit))
		return 0;

	/*
	 * Final constraint: each grf needs a view....
	 */
	if((cfdata_grf == NULL) || (did_consinit > 1)) {
	    if((*view_cdevsw.d_open)(cfp->cf_unit, 0, 0, NULL))
		return 0;
	}
	did_consinit = 2;
	return 1;
}

/*
 * attach: initialize the grf-structure and try to attach an ite to us.
 * note  : dp is NULL during early console init.
 */
void
grfetattach(struct device *pdp, struct device *dp, void *auxp)
{
	static struct grf_softc		congrf;
	static int			first_attach = 1;
	       grf_auxp_t		*grf_bus_auxp = auxp;
	       grf_auxp_t		grf_auxp;
	       struct grf_softc		*gp;
	       int			maj;
	extern const struct cdevsw grf_cdevsw;

	/*
	 * find our major device number 
	 */
	maj = cdevsw_lookup_major(&grf_cdevsw);

	/*
	 * Handle exception case: early console init
	 */
	if(dp == NULL) {
		congrf.g_unit    = cfdata_grf->cf_unit;
		congrf.g_grfdev  = makedev(maj, congrf.g_unit);
		congrf.g_itedev  = (dev_t)-1;
		congrf.g_flags   = GF_ALIVE;
		congrf.g_mode    = grf_mode;
		congrf.g_conpri  = CN_INTERNAL;
		congrf.g_viewdev = congrf.g_unit;
		grfet_iteinit(&congrf);
		grf_viewsync(&congrf);

		/* Attach console ite */
		atari_config_found(cfdata_grf, NULL, &congrf, grfetprint);
		return;
	}

	gp = (struct grf_softc *)dp;
	gp->g_unit = device_unit(&gp->g_device);
	grfsp[gp->g_unit] = gp;

	if((cfdata_grf != NULL) && (gp->g_unit == congrf.g_unit)) {
		/*
		 * We inited earlier just copy the info, take care
		 * not to copy the device struct though.
		 */
		memcpy(&gp->g_display, &congrf.g_display,
			(char *)&gp[1] - (char *)&gp->g_display);
	}
	else {
		gp->g_grfdev  = makedev(maj, gp->g_unit);
		gp->g_itedev  = (dev_t)-1;
		gp->g_flags   = GF_ALIVE;
		gp->g_mode    = grf_mode;
		gp->g_conpri  = 0;
		gp->g_viewdev = gp->g_unit;
		grfet_iteinit(gp);
		grf_viewsync(gp);
	}

	printf(": %dx%d", gp->g_display.gd_dwidth, gp->g_display.gd_dheight);
	if(gp->g_display.gd_colors == 2)
		printf(" monochrome\n");
	else printf(" colors %d\n", gp->g_display.gd_colors);
	
	/*
	 * try and attach an ite
	 */
	config_found(dp, gp, grfetprint);

	/*
	 * If attaching the first unit, go ahead and 'find' the rest of us
	 */
	if (first_attach) {
		first_attach = 0;
		grf_auxp.from_bus_match = 0;
		for (grf_auxp.unit=0; grf_auxp.unit < NGRFET; grf_auxp.unit++) {
		    config_found(pdp, (void*)&grf_auxp, grf_bus_auxp->busprint);
		}
	}
}

int
grfetprint(void *auxp, const char *pnp)
{
	if(pnp) /* XXX */
		aprint_normal("ite at %s", pnp);
	return(UNCONF);
}

/*
 * Init ite portion of grf_softc struct
 */
static void
grfet_iteinit(struct grf_softc *gp)
{

	gp->g_itecursor = et_cursor;
	gp->g_iteputc   = et_putc;
	gp->g_iteclear  = et_clear;
	gp->g_itescroll = et_scroll;
	gp->g_iteinit   = view_init;
	gp->g_itedeinit = view_deinit;
}

static void
view_deinit(struct ite_softc *ip)
{
	ip->flags &= ~ITE_INITED;
}

static void
view_init(register struct ite_softc *ip)
{
	struct itewinsize	wsz;
	ipriv_t			*cci;
	view_t			*view;
	save_area_t		*et_save;

	if((cci = ip->priv) != NULL)
		return;

	ip->itexx_ioctl = iteet_ioctl;

#if defined(KFONT_8X8)
	ip->font = font_info_8x8;
#else
	ip->font = font_info_8x16;
#endif

	/* Find the correct set of rendering routines for this font.  */
	if(ip->font.width != 8)
		panic("kernel font size not supported");

	if(!atari_realconfig)
		ip->priv = cci = &con_ipriv;
	else ip->priv = cci = (ipriv_t*)malloc(sizeof(*cci), M_DEVBUF,M_WAITOK);
	if(cci == NULL)
		panic("No memory for ite-view");
	memset(cci, 0, sizeof(*cci));

	wsz.x      = ite_default_x;
	wsz.y      = ite_default_y;
	wsz.width  = ite_default_width;
	wsz.height = ite_default_height;
	wsz.depth  = ite_default_depth;

	ite_newsize (ip, &wsz);

	view  = viewview(ip->grf->g_viewdev);
	cci->regkva = view->bitmap->regs;

	/*
	 * Only console will be turned on by default..
	 */
	if(ip->flags & ITE_ISCONS)
		ip->grf->g_mode(ip->grf, GM_GRFON, NULL, 0, 0);

	/*
	 * Activate text-mode settings
	 */
	et_save = (save_area_t *)view->save_area;
	if (et_save == NULL)
		et_inittextmode(ip, NULL, view->flags & VF_DISPLAY);
	else {
		et_inittextmode(ip, &et_save->sv_regs, view->flags&VF_DISPLAY);
		et_save->fb_size = ip->cols * ip->rows;
	}
}

static int
ite_newsize(struct ite_softc *ip, struct itewinsize *winsz)
{
	struct view_size	vs;
	int			error = 0;
	save_area_t		*et_save;
	view_t			*view;
	extern const struct cdevsw view_cdevsw;

	vs.x      = winsz->x;
	vs.y      = winsz->y;
	vs.width  = winsz->width;
	vs.height = winsz->height;
	vs.depth  = winsz->depth;

	error = (*view_cdevsw.d_ioctl)(ip->grf->g_viewdev, VIOCSSIZE,
				       (void *)&vs, 0, NOLWP);
	view  = viewview(ip->grf->g_viewdev);

	/*
	 * Reinitialize our structs
	 */
	ip->cols = view->display.width  / ip->font.width; 
	ip->rows = view->display.height / ip->font.height;

	/*
	 * save new values so that future opens use them
	 * this may not be correct when we implement Virtual Consoles
	 */
	ite_default_height = view->display.height;
	ite_default_width  = view->display.width;
	ite_default_x      = view->display.x;
	ite_default_y      = view->display.y;
	ite_default_depth  = view->bitmap->depth;

	et_save = (save_area_t *)view->save_area;
	if (et_save == NULL)
	    et_inittextmode(ip, NULL, view->flags & VF_DISPLAY);
	else {
	    et_inittextmode(ip, &et_save->sv_regs, view->flags & VF_DISPLAY);
	    et_save->fb_size = ip->cols * ip->rows;
	}
	et_clear(ip, 0, 0, ip->rows, ip->cols);

	return(error);
}

int
iteet_ioctl(struct ite_softc *ip, u_long cmd, void * addr, int flag, struct lwp *l)
{
	struct winsize		ws;
	struct itewinsize	*is;
	int			error = 0;
	view_t			*view = viewview(ip->grf->g_viewdev);
	extern const struct cdevsw ite_cdevsw;
	extern const struct cdevsw view_cdevsw;

	switch (cmd) {
	case ITEIOCSWINSZ:
		is = (struct itewinsize *)addr;

		if(ite_newsize(ip, is))
			error = ENOMEM;
		else {
			view         = viewview(ip->grf->g_viewdev);
			ws.ws_row    = ip->rows;
			ws.ws_col    = ip->cols;
			ws.ws_xpixel = view->display.width;
			ws.ws_ypixel = view->display.height;
			ite_reset(ip);
			/*
			 * XXX tell tty about the change 
			 * XXX this is messy, but works 
			 */
			(*ite_cdevsw.d_ioctl)(ip->grf->g_itedev, TIOCSWINSZ,
					      (void *)&ws, 0, l);
		}
		break;
	case VIOCSCMAP:
	case VIOCGCMAP:
		/*
		 * XXX watchout for that NOLWP. its not really the kernel
		 * XXX talking these two commands don't use the proc pointer
		 * XXX though.
		 */
		error = (*view_cdevsw.d_ioctl)(ip->grf->g_viewdev, cmd, addr,
					       flag, NOLWP);
		break;
	default:
		error = EPASSTHROUGH;
		break;
	}
	return (error);
}

void
et_cursor(struct ite_softc *ip, int flag)
{
	volatile u_char	*ba;
		 view_t	*v;
		 u_long cpos;

	ba = ((ipriv_t*)ip->priv)->regkva;
	v  = viewview(ip->grf->g_viewdev);

	/*
	 * Don't update the cursor when not on display
	 */
	if (!(v->flags & VF_DISPLAY))
		return;

	switch (flag) {
 	    case DRAW_CURSOR:
		/*WCrt(ba, CRT_ID_CURSOR_START, & ~0x20); */
	    case MOVE_CURSOR:
		cpos  =  RCrt(ba, CRT_ID_START_ADDR_LOW) & 0xff;
		cpos |= (RCrt(ba, CRT_ID_START_ADDR_HIGH) & 0xff) << 8;
		cpos += ip->curx + ip->cury * ip->cols;
		WCrt(ba, CRT_ID_CURSOR_LOC_LOW, cpos & 0xff);
		WCrt(ba, CRT_ID_CURSOR_LOC_HIGH, (cpos >> 8) & 0xff);
		WCrt(ba, CTR_ID_EXT_START, (cpos >> (16-2)) & 0x0c);

		ip->cursorx = ip->curx;
		ip->cursory = ip->cury;
		break;
	    case ERASE_CURSOR:
		/*WCrt(ba, CRT_ID_CURSOR_START, | 0x20); */
	    case START_CURSOROPT:
	    case END_CURSOROPT:
	    default:
		break;
    	}
}

void
et_putc(struct ite_softc *ip, int c, int dy, int dx, int mode)
{
	view_t	*v   = viewview(ip->grf->g_viewdev);
	u_char	attr;
	u_short	*cp;

	attr = (unsigned char) ((mode & ATTR_INV) ? (0x70) : (0x07));
	if (mode & ATTR_UL)     attr |= 0x01;
	if (mode & ATTR_BOLD)   attr |= 0x08;
	if (mode & ATTR_BLINK)  attr |= 0x80;

	cp  = (u_short*)v->bitmap->plane;
	cp[(dy * ip->cols) + dx] = (c << 8) | attr;
}

void
et_clear(struct ite_softc *ip, int sy, int sx, int h, int w)
{
	/* et_clear and et_scroll both rely on ite passing arguments
	 * which describe continuous regions.  For a VT200 terminal,
	 * this is safe behavior.
	 */
	view_t		*v   = viewview(ip->grf->g_viewdev);
	u_short		*dest;
	int		len;

	dest = (u_short *)v->bitmap->plane + (sy * ip->cols) + sx;
	for(len = w * h; len-- ;)
		*dest++ = 0x2007;
}

void
et_scroll(struct ite_softc *ip, int sy, int sx, int count, int dir)
{
	view_t	*v   = viewview(ip->grf->g_viewdev);
	u_short	*fb;
	u_short	*src, *dst;
	int	len;

	fb = (u_short*)v->bitmap->plane + sy * ip->cols;
	switch (dir) {
	    case SCROLL_UP:
			src = fb;
			dst = fb - count * ip->cols;
			len = (ip->bottom_margin + 1 - sy) * ip->cols;
			break;
	    case SCROLL_DOWN:
			src = fb;
			dst = fb + count * ip->cols;
			len = (ip->bottom_margin + 1 - (sy + count)) * ip->cols;
			break;
	    case SCROLL_RIGHT:
			src = fb + sx;
			dst = fb + sx + count;
			len = ip->cols - sx + count;
			break;
	    case SCROLL_LEFT:
			src = fb + sx;
			dst = fb + sx - count;
			len = ip->cols - sx;
			break;
	    default:
			return;
	}
	if (src > dst) {
		while (len--)
			*dst++ = *src++;
	}
	else {
		src = &src[len];
		dst = &dst[len];
		while (len--)
			*--dst = *--src;
	}
}

static void
et_inittextmode(struct ite_softc *ip, et_sv_reg_t *etregs, int loadfont)
{
	volatile u_char *ba;
	font_info	*fd;
	u_char		*fb;
	u_char		*c, *f, tmp;
	u_short		z, y;
	int		s;
	view_t		*v   = viewview(ip->grf->g_viewdev);
	et_sv_reg_t	loc_regs;

	if (etregs == NULL) {
		etregs = &loc_regs;
		et_hwsave(etregs);
	}

	ba = ((ipriv_t*)ip->priv)->regkva;
	fb = v->bitmap->plane;

#if defined(KFONT_8X8)
	fd = &font_info_8x8;
#else
	fd = &font_info_8x16;
#endif

	if (loadfont) { /* XXX: We should set the colormap */
		/*
		 * set colors (B&W)
		 */
		vgaw(ba, VDAC_ADDRESS_W, 0);
		for (z = 0; z < 256; z++) {
			y = (z & 1) ? ((z > 7) ? 2 : 1) : 0;
    
			vgaw(ba, VDAC_DATA, etconscolors[y][0]);
			vgaw(ba, VDAC_DATA, etconscolors[y][1]);
			vgaw(ba, VDAC_DATA, etconscolors[y][2]);
		}

		/*
		 * Enter a suitable mode to download the font. This
		 * basically means sequential addressing mode
		 */
		s = splhigh();

		WAttr(ba, 0x20 | ACT_ID_ATTR_MODE_CNTL, 0x0a);
		WSeq(ba, SEQ_ID_MAP_MASK,	 0x04);
		WSeq(ba, SEQ_ID_MEMORY_MODE,	 0x06);
		WGfx(ba, GCT_ID_READ_MAP_SELECT, 0x02);
		WGfx(ba, GCT_ID_GRAPHICS_MODE,	 0x00);
		WGfx(ba, GCT_ID_MISC,		 0x04);
		splx(s);
	
		/*
		 * load text font into beginning of display memory. Each
		 * character cell is 32 bytes long (enough for 4 planes)
		 */
		for (z = 0, c = fb; z < 256 * 32; z++)
			*c++ = 0;

		c = (unsigned char *) (fb) + (32 * fd->font_lo);
		f = fd->font_p;
		z = fd->font_lo;
		for (; z <= fd->font_hi; z++, c += (32 - fd->height))
			for (y = 0; y < fd->height; y++) {
				*c++ = *f++;
			}
	}

	/*
	 * Odd/Even addressing
	 */
	etregs->seq[SEQ_ID_MAP_MASK]        = 0x03;
	etregs->seq[SEQ_ID_MEMORY_MODE]     = 0x03;
	etregs->grf[GCT_ID_READ_MAP_SELECT] = 0x00;
	etregs->grf[GCT_ID_GRAPHICS_MODE]   = 0x10;
	etregs->grf[GCT_ID_MISC]            = 0x06;

	/*
	 * Font height + underline location
	 */
	tmp = etregs->crt[CRT_ID_MAX_ROW_ADDRESS] & 0xe0;
	etregs->crt[CRT_ID_MAX_ROW_ADDRESS] = tmp | (fd->height - 1);
	tmp = etregs->crt[CRT_ID_UNDERLINE_LOC] & 0xe0;
	etregs->crt[CRT_ID_UNDERLINE_LOC] = tmp | (fd->height - 1);

	/*
	 * Cursor setup
	 */
	etregs->crt[CRT_ID_CURSOR_START]    = 0x00;
	etregs->crt[CRT_ID_CURSOR_END]      = fd->height - 1;
	etregs->crt[CRT_ID_CURSOR_LOC_HIGH] = 0x00;
	etregs->crt[CRT_ID_CURSOR_LOC_LOW]  = 0x00;

	/*
	 * Enter text mode
	 */
	etregs->crt[CRT_ID_MODE_CONTROL]    = 0xa3;
	etregs->attr[ACT_ID_ATTR_MODE_CNTL] = 0x0a;

#if 1
	if (loadfont || (etregs == &loc_regs))
#else
	if (etregs == &loc_regs)
#endif
		et_hwrest(etregs);
}
