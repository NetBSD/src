/*	$NetBSD: p9100.c,v 1.26 2005/12/14 00:35:31 christos Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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

/*
 * color display (p9100) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * XXX should defer colormap updates to vertical retrace interrupts
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: p9100.c,v 1.26 2005/12/14 00:35:31 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/sun/fbio.h>
#include <dev/sun/fbvar.h>
#include <dev/sun/btreg.h>
#include <dev/sun/btvar.h>

#include <dev/sbus/p9100reg.h>

#include <dev/sbus/sbusvar.h>

/*#include <dev/wscons/wsdisplayvar.h>*/
#include <dev/wscons/wsconsio.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>

#include "opt_wsemul.h"
#include "rasops_glue.h"

#include "tctrl.h"
#if NTCTRL > 0
#include <machine/tctrl.h>
#include <sparc/dev/tctrlvar.h>/*XXX*/
#endif

struct pnozz_cursor {
	short	pc_enable;		/* cursor is enabled */
	struct	fbcurpos pc_pos;	/* position */
	struct	fbcurpos pc_hot;	/* hot-spot */
	struct	fbcurpos pc_size;	/* size of mask & image fields */
	uint32_t pc_bits[0x100];	/* space for mask & image bits */
	unsigned char red[3], green[3];
	unsigned char blue[3];		/* cursor palette */
};

/* per-display variables */
struct p9100_softc {
	struct device	sc_dev;		/* base device */
	struct sbusdev	sc_sd;		/* sbus device */
	struct fbdevice	sc_fb;		/* frame buffer device */

	bus_space_tag_t	sc_bustag;

	bus_addr_t	sc_ctl_paddr;	/* phys address description */
	bus_size_t	sc_ctl_psize;	/*   for device mmap() */
	bus_space_handle_t sc_ctl_memh;	/*   bus space handle */

	bus_addr_t	sc_cmd_paddr;	/* phys address description */
	bus_size_t	sc_cmd_psize;	/*   for device mmap() */
	bus_space_handle_t sc_cmd_memh;	/*   bus space handle */

	bus_addr_t	sc_fb_paddr;	/* phys address description */
	bus_size_t	sc_fb_psize;	/*   for device mmap() */
	bus_space_handle_t sc_fb_memh;	/*   bus space handle */

	uint32_t sc_junk;
	uint32_t sc_mono_width;	/* for setup_mono */
	
	uint32_t sc_width;
	uint32_t sc_height;	/* panel width / height */
	uint32_t sc_stride;
	uint32_t sc_depth;
	union	bt_cmap sc_cmap;	/* Brooktree color map */

	struct pnozz_cursor sc_cursor;

#ifdef PNOZZ_SOFT_PUTCHAR
	void (*putchar)(void *c, int row, int col, u_int uc, long attr);
#endif
	int sc_mode;
	uint32_t sc_bg;
	void (*switchcb)(void *, int, int);
	void *switchcbarg;
	struct callout switch_callout;
	LIST_HEAD(, p9100_screen) screens;
	struct p9100_screen *active, *wanted;
	const struct wsscreen_descr *currenttype;

};

struct p9100_screen {
	struct rasops_info ri;
	LIST_ENTRY(p9100_screen) next;
	struct p9100_softc *sc;
	const struct wsscreen_descr *type;
	int active;
	u_int16_t *chars;
	long *attrs;
	int dispoffset;
	int mindispoffset;
	int maxdispoffset;

	int cursoron;
	int cursorcol;
	int cursorrow;
	int cursordrawn;
};

static struct p9100_screen p9100_console_screen;

extern const u_char rasops_cmap[768];

struct wsscreen_descr p9100_defscreendesc = {
	"default",
	0, 0,
	NULL,
	8, 16,
	WSSCREEN_WSCOLORS,
};

const struct wsscreen_descr *_p9100_scrlist[] = {
	&p9100_defscreendesc,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list p9100_screenlist = {
	sizeof(_p9100_scrlist) / sizeof(struct wsscreen_descr *), _p9100_scrlist
};

/* autoconfiguration driver */
static int	p9100_sbus_match(struct device *, struct cfdata *, void *);
static void	p9100_sbus_attach(struct device *, struct device *, void *);

static void	p9100unblank(struct device *);
static void	p9100_shutdown(void *);

CFATTACH_DECL(pnozz, sizeof(struct p9100_softc),
    p9100_sbus_match, p9100_sbus_attach, NULL, NULL);

extern struct cfdriver pnozz_cd;

dev_type_open(p9100open);
dev_type_ioctl(p9100ioctl);
dev_type_mmap(p9100mmap);

const struct cdevsw pnozz_cdevsw = {
	p9100open, nullclose, noread, nowrite, p9100ioctl,
	nostop, notty, nopoll, p9100mmap, nokqfilter,
};

/* frame buffer generic driver */
static struct fbdriver p9100fbdriver = {
	p9100unblank, p9100open, nullclose, p9100ioctl, nopoll,
	p9100mmap, nokqfilter
};

static void	p9100loadcmap(struct p9100_softc *, int, int);
static void	p9100_set_video(struct p9100_softc *, int);
static int	p9100_get_video(struct p9100_softc *);
static uint32_t p9100_ctl_read_4(struct p9100_softc *, bus_size_t);
static void	p9100_ctl_write_4(struct p9100_softc *, bus_size_t, uint32_t);
uint8_t		p9100_ramdac_read(struct p9100_softc *, bus_size_t);
void		p9100_ramdac_write(struct p9100_softc *, bus_size_t, uint8_t);

static void	p9100_sync(struct p9100_softc *);
void 		p9100_bitblt(struct p9100_softc *, int, int, int, int, int, int, 		    uint32_t);	/* coordinates, rasop */
void 		p9100_rectfill(struct p9100_softc *, int, int, int, int, 
		    uint32_t);	/* coordinates, colour */
static void 	p9100_init_engine(struct p9100_softc *);
void		p9100_setup_mono(struct p9100_softc *, int, int, int, int, 
		    uint32_t, uint32_t); 
void		p9100_feed_line(struct p9100_softc *, int, uint8_t *);
static void	p9100_set_color_reg(struct p9100_softc *, int, int32_t);

void	p9100_cursor(void *, int, int, int);
int	p9100_mapchar(void *, int, u_int *);
void	p9100_putchar(void *, int, int, u_int, long);
void	p9100_copycols(void *, int, int, int, int);
void	p9100_erasecols(void *, int, int, int, long);
void	p9100_copyrows(void *, int, int, int);
void	p9100_eraserows(void *, int, int, long);
int	p9100_allocattr(void *, int, int, int, long *);

void	p9100_scroll(void *, void *, int);

int	p9100_putcmap(struct p9100_softc *, struct wsdisplay_cmap *);
int 	p9100_getcmap(struct p9100_softc *, struct wsdisplay_cmap *);
int	p9100_ioctl(void *, u_long, caddr_t, int, struct lwp *);
paddr_t	p9100_mmap(void *, off_t, int);
int	p9100_alloc_screen(void *, const struct wsscreen_descr *, void **, 
	    int *, int *, long *);
void	p9100_free_screen(void *, void *);
int	p9100_show_screen(void *, void *, int, void (*)(void *, int, int),
	    void *);
void	p9100_switch_screen(struct p9100_softc *);
void	p9100_restore_screen(struct p9100_screen *, 
	    const struct wsscreen_descr *, u_int16_t *);
void	p9100_clearscreen(struct p9100_softc *);
		
int	p9100_load_font(void *, void *, struct wsdisplay_font *);

void	p9100_init_screen(struct p9100_softc *, struct p9100_screen *, int, 
	    long *);
void	p9100_init_cursor(struct p9100_softc *);
void	p9100_set_fbcursor(struct p9100_softc *);
void	p9100_setcursorcmap(struct p9100_softc *);
void	p9100_loadcursor(struct p9100_softc *);

int	p9100_intr(void *);

struct wsdisplay_accessops p9100_accessops = {
	p9100_ioctl,
	p9100_mmap,
	p9100_alloc_screen,
	p9100_free_screen,
	p9100_show_screen,
	NULL,	/* load_font */
	NULL,	/* polls */
	NULL,	/* getwschar */
	NULL,	/* putwschar */
	NULL,	/* scroll */
	NULL,	/* getborder */
	NULL	/* setborder */
};

/*
 * Match a p9100.
 */
static int
p9100_sbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sbus_attach_args *sa = aux;

	return (strcmp("p9100", sa->sa_name) == 0);
}


/*
 * Attach a display.  We need to notice if it is the console, too.
 */
static void
p9100_sbus_attach(struct device *parent, struct device *self, void *args)
{
	struct p9100_softc *sc = (struct p9100_softc *)self;
	struct sbus_attach_args *sa = args;
	struct fbdevice *fb = &sc->sc_fb;
	int isconsole;
	int node;
	int i, j;

#if NWSDISPLAY > 0
	struct wsemuldisplaydev_attach_args aa;
	struct rasops_info *ri;
	unsigned long defattr;
#endif

	/* Remember cookies for p9100_mmap() */
	sc->sc_bustag = sa->sa_bustag;
	sc->sc_ctl_paddr = sbus_bus_addr(sa->sa_bustag,
		sa->sa_reg[0].oa_space, sa->sa_reg[0].oa_base);
	sc->sc_ctl_psize = 0x8000;/*(bus_size_t)sa->sa_reg[0].oa_size;*/

	sc->sc_cmd_paddr = sbus_bus_addr(sa->sa_bustag,
		sa->sa_reg[1].oa_space, sa->sa_reg[1].oa_base);
	sc->sc_cmd_psize = (bus_size_t)sa->sa_reg[1].oa_size;

	sc->sc_fb_paddr = sbus_bus_addr(sa->sa_bustag,
		sa->sa_reg[2].oa_space, sa->sa_reg[2].oa_base);
	sc->sc_fb_psize = (bus_size_t)sa->sa_reg[2].oa_size;

	fb->fb_driver = &p9100fbdriver;
	fb->fb_device = &sc->sc_dev;
	fb->fb_flags = sc->sc_dev.dv_cfdata->cf_flags & FB_USERMASK;
#ifdef PNOZZ_EMUL_CG3
	fb->fb_type.fb_type = FBTYPE_SUN3COLOR;
#else
	fb->fb_type.fb_type = FBTYPE_P9100;
#endif
	fb->fb_pixels = NULL;
	
	sc->sc_mode = WSDISPLAYIO_MODE_EMUL;
#ifdef PNOZZ_SOFT_PUTCHAR
	sc->putchar = NULL;
#endif

	LIST_INIT(&sc->screens);
	sc->active = NULL;
	sc->currenttype = &p9100_defscreendesc;
	callout_init(&sc->switch_callout);

	node = sa->sa_node;
	isconsole = fb_is_console(node);
	if (!isconsole) {
		printf("\n%s: fatal error: PROM didn't configure device\n", 
		    self->dv_xname);
		return;
	}

	/*
	 * When the ROM has mapped in a p9100 display, the address
	 * maps only the video RAM, so in any case we have to map the
	 * registers ourselves.  We only need the video RAM if we are
	 * going to print characters via rconsole.
	 */
	if (sbus_bus_map(sc->sc_bustag,
			 sa->sa_reg[0].oa_space,
			 sa->sa_reg[0].oa_base,
			 /* 
			  * XXX for some reason the SBus resources don't cover 
			  * all registers, so we just map what we need 
			  */
			 /*sc->sc_ctl_psize*/ 0x8000,
			 /*BUS_SPACE_MAP_LINEAR*/0, &sc->sc_ctl_memh) != 0) {
		printf("%s: cannot map control registers\n", self->dv_xname);
		return;
	}

	if (sa->sa_npromvaddrs != 0)
		fb->fb_pixels = (caddr_t)sa->sa_promvaddrs[0];

	if (fb->fb_pixels == NULL) {
		if (sbus_bus_map(sc->sc_bustag,
				sa->sa_reg[2].oa_space,
				sa->sa_reg[2].oa_base,
				sc->sc_fb_psize,
				BUS_SPACE_MAP_LINEAR, &sc->sc_fb_memh) != 0) {
			printf("%s: cannot map framebuffer\n", self->dv_xname);
			return;
		}
		fb->fb_pixels = (char *)sc->sc_fb_memh;
	} else {
		sc->sc_fb_memh = (bus_space_handle_t) fb->fb_pixels;
	}

	i = p9100_ctl_read_4(sc, 0x0004);
	switch ((i >> 26) & 7) {
	    case 5: fb->fb_type.fb_depth = 32; break;
	    case 7: fb->fb_type.fb_depth = 24; break;
	    case 3: fb->fb_type.fb_depth = 16; break;
	    case 2: fb->fb_type.fb_depth = 8; break;
	    default: {
		panic("pnozz: can't determine screen depth (0x%02x)", i);
	    }
	}
	sc->sc_depth = (fb->fb_type.fb_depth >> 3);
	
	/* XXX for some reason I get a kernel trap with this */
	sc->sc_width = prom_getpropint(node, "width", 800);
	sc->sc_height = prom_getpropint(node, "height", 600);
	
	sc->sc_stride = prom_getpropint(node, "linebytes", sc->sc_width * 
	    (fb->fb_type.fb_depth >> 3));

	p9100_init_engine(sc);
	
	fb_setsize_obp(fb, fb->fb_type.fb_depth, sc->sc_width, sc->sc_height, 
	    node);

	sbus_establish(&sc->sc_sd, &sc->sc_dev);

	fb->fb_type.fb_size = fb->fb_type.fb_height * fb->fb_linebytes;
	printf(": rev %d, %dx%d, depth %d mem %x",
	       (i & 7), fb->fb_type.fb_width, fb->fb_type.fb_height,
	       fb->fb_type.fb_depth, (unsigned int)sc->sc_fb_psize);

	fb->fb_type.fb_cmsize = prom_getpropint(node, "cmsize", 256);
	if ((1 << fb->fb_type.fb_depth) != fb->fb_type.fb_cmsize)
		printf(", %d entry colormap", fb->fb_type.fb_cmsize);

	/* Initialize the default color map. */
	/*bt_initcmap(&sc->sc_cmap, 256);*/
	j = 0;
	for (i = 0; i < 256; i++) {
		sc->sc_cmap.cm_map[i][0] = rasops_cmap[j];
		j++;
		sc->sc_cmap.cm_map[i][1] = rasops_cmap[j];
		j++;
		sc->sc_cmap.cm_map[i][2] = rasops_cmap[j];
		j++;
	}
	p9100loadcmap(sc, 0, 256);

	/* make sure we are not blanked */
	if (isconsole)
		p9100_set_video(sc, 1);

	if (shutdownhook_establish(p9100_shutdown, sc) == NULL) {
		panic("%s: could not establish shutdown hook",
		      sc->sc_dev.dv_xname);
	}

	if (isconsole) {
		printf(" (console)\n");
#ifdef RASTERCONSOLE
		/*p9100loadcmap(sc, 255, 1);*/
		fbrcons_init(fb);
#endif
	} else
		printf("\n");
		
#if NWSDISPLAY > 0
	wsfont_init();

	p9100_init_screen(sc, &p9100_console_screen, 1, &defattr);
	p9100_console_screen.active = 1;
	sc->active = &p9100_console_screen;
	ri = &p9100_console_screen.ri;

	p9100_defscreendesc.nrows = ri->ri_rows;
	p9100_defscreendesc.ncols = ri->ri_cols;
	p9100_defscreendesc.textops = &ri->ri_ops;
	p9100_defscreendesc.capabilities = ri->ri_caps;
	
	if(isconsole) {
		wsdisplay_cnattach(&p9100_defscreendesc, ri, 0, 0, defattr);
	}

	sc->sc_bg = (defattr >> 16) & 0xff;

	p9100_clearscreen(sc);
	p9100_init_cursor(sc);

	aa.console = isconsole;
	aa.scrdata = &p9100_screenlist;
	aa.accessops = &p9100_accessops;
	aa.accesscookie = sc;

	config_found(self, &aa, wsemuldisplaydevprint);
#endif
	/* attach the fb */
	fb_attach(fb, isconsole);

#if 0
	p9100_rectfill(sc, 10, 10, 200, 200, 0x01);
	p9100_rectfill(sc, 210, 10, 200, 200, 0xff);
	p9100_bitblt(sc, 10, 10, 110, 110, 400, 200, ROP_SRC);
#endif
#if 0
	p9100_setup_mono(sc, 750, 500, 32, 1, 1, 6);
	{
		uint32_t ev = 0xc3000000, odd = 0x3c000000;
		for (i = 0; i < 16; i++) { 
			p9100_feed_line(sc, 1, (uint8_t*)&ev);
			p9100_feed_line(sc, 1, (uint8_t*)&odd);
		}
	}
	delay(4000000);
#endif
}

static void
p9100_shutdown(arg)
	void *arg;
{
	struct p9100_softc *sc = arg;
	
#ifdef RASTERCONSOLE
	sc->sc_cmap.cm_map[0][0] = 0xff;
	sc->sc_cmap.cm_map[0][1] = 0xff;
	sc->sc_cmap.cm_map[0][2] = 0xff;
	sc->sc_cmap.cm_map[1][0] = 0;
	sc->sc_cmap.cm_map[1][1] = 0;
	sc->sc_cmap.cm_map[1][2] = 0x00;
	p9100loadcmap(sc, 0, 2);
	sc->sc_cmap.cm_map[255][0] = 0;
	sc->sc_cmap.cm_map[255][1] = 0;
	sc->sc_cmap.cm_map[255][2] = 0;
	p9100loadcmap(sc, 255, 1);
#endif
	p9100_set_video(sc, 1);
}

int
p9100open(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = minor(dev);

	if (unit >= pnozz_cd.cd_ndevs || pnozz_cd.cd_devs[unit] == NULL)
		return (ENXIO);
	return (0);
}

int
p9100ioctl(dev_t dev, u_long cmd, caddr_t data, int flags, struct lwp *l)
{
	struct p9100_softc *sc = pnozz_cd.cd_devs[minor(dev)];
	struct fbgattr *fba;
	int error, v;

	switch (cmd) {

	case FBIOGTYPE:
		*(struct fbtype *)data = sc->sc_fb.fb_type;
		break;

	case FBIOGATTR:
		fba = (struct fbgattr *)data;
		fba->real_type = sc->sc_fb.fb_type.fb_type;
		fba->owner = 0;		/* XXX ??? */
		fba->fbtype = sc->sc_fb.fb_type;
		fba->sattr.flags = 0;
		fba->sattr.emu_type = sc->sc_fb.fb_type.fb_type;
		fba->sattr.dev_specific[0] = -1;
		fba->emu_types[0] = sc->sc_fb.fb_type.fb_type;
		fba->emu_types[1] = -1;
		break;

	case FBIOGETCMAP:
#define p ((struct fbcmap *)data)
		return (bt_getcmap(p, &sc->sc_cmap, 256, 1));

	case FBIOPUTCMAP:
		/* copy to software map */
		error = bt_putcmap(p, &sc->sc_cmap, 256, 1);
		if (error)
			return (error);
		/* now blast them into the chip */
		/* XXX should use retrace interrupt */
		p9100loadcmap(sc, p->index, p->count);
#undef p
		break;

	case FBIOGVIDEO:
		*(int *)data = p9100_get_video(sc);
		break;

	case FBIOSVIDEO:
		p9100_set_video(sc, *(int *)data);
		break;

/* these are for both FBIOSCURSOR and FBIOGCURSOR */
#define p ((struct fbcursor *)data)
#define pc (&sc->sc_cursor)

	case FBIOGCURSOR:
		p->set = FB_CUR_SETALL;	/* close enough, anyway */
		p->enable = pc->pc_enable;
		p->pos = pc->pc_pos;
		p->hot = pc->pc_hot;
		p->size = pc->pc_size;

		if (p->image != NULL) {
			error = copyout(pc->pc_bits, p->image, 0x200);
			if (error)
				return error;
			error = copyout(&pc->pc_bits[0x80], p->mask, 0x200);
			if (error)
				return error;
		}
		
		p->cmap.index = 0;
		p->cmap.count = 3;
		if (p->cmap.red != NULL) {
			copyout(pc->red, p->cmap.red, 3);
			copyout(pc->green, p->cmap.green, 3);
			copyout(pc->blue, p->cmap.blue, 3);
		}
		break;

	case FBIOSCURSOR:
	{
		int count;
		uint32_t image[0x80], mask[0x80];
		uint8_t red[3], green[3], blue[3];
		
		v = p->set;
		if (v & FB_CUR_SETCMAP) {
			error = copyin(p->cmap.red, red, 3);
			error |= copyin(p->cmap.green, green, 3);
			error |= copyin(p->cmap.blue, blue, 3);
			if (error)
				return error;
		}
		if (v & FB_CUR_SETSHAPE) {
			if (p->size.x > 64 || p->size.y > 64)
				return EINVAL;
			memset(&mask, 0, 0x200);
			memset(&image, 0, 0x200);
			count = p->size.y * 8;
			error = copyin(p->image, image, count);
			if (error)
				return error;
			error = copyin(p->mask, mask, count);
			if (error)
				return error;
		}

		/* parameters are OK; do it */
		if (v & (FB_CUR_SETCUR | FB_CUR_SETPOS | FB_CUR_SETHOT)) {
			if (v & FB_CUR_SETCUR)
				pc->pc_enable = p->enable;
			if (v & FB_CUR_SETPOS)
				pc->pc_pos = p->pos;
			if (v & FB_CUR_SETHOT)
				pc->pc_hot = p->hot;
			p9100_set_fbcursor(sc);
		}
		
		if (v & FB_CUR_SETCMAP) {
			memcpy(pc->red, red, 3);
			memcpy(pc->green, green, 3);
			memcpy(pc->blue, blue, 3);
			p9100_setcursorcmap(sc);
		}
		
		if (v & FB_CUR_SETSHAPE) {
			memcpy(pc->pc_bits, image, 0x200);
			memcpy(&pc->pc_bits[0x80], mask, 0x200);
			p9100_loadcursor(sc);
		}
	}
	break;

#undef p
#undef cc

	case FBIOGCURPOS:
		*(struct fbcurpos *)data = sc->sc_cursor.pc_pos;
		break;

	case FBIOSCURPOS:
		sc->sc_cursor.pc_pos = *(struct fbcurpos *)data;
		p9100_set_fbcursor(sc);
		break;

	case FBIOGCURMAX:
		/* max cursor size is 64x64 */
		((struct fbcurpos *)data)->x = 64;
		((struct fbcurpos *)data)->y = 64;
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

static uint32_t
p9100_ctl_read_4(struct p9100_softc *sc, bus_size_t off)
{
	sc->sc_junk = bus_space_read_4(sc->sc_bustag, sc->sc_fb_memh, off);
	return bus_space_read_4(sc->sc_bustag, sc->sc_ctl_memh, off);
}

static void
p9100_ctl_write_4(struct p9100_softc *sc, bus_size_t off, uint32_t v)
{
	sc->sc_junk = bus_space_read_4(sc->sc_bustag, sc->sc_fb_memh, off);
	bus_space_write_4(sc->sc_bustag, sc->sc_ctl_memh, off, v);
}

/* wait until the engine is idle */
static void
p9100_sync(struct p9100_softc *sc)
{
	while((p9100_ctl_read_4(sc, ENGINE_STATUS) & 
	    (ENGINE_BUSY | BLITTER_BUSY)) != 0);
}

static void	
p9100_set_color_reg(struct p9100_softc *sc, int reg, int32_t col)
{
	uint32_t out;
	
	switch(sc->sc_depth)
	{
		case 1:	/* 8 bit */
			out = (col << 8) | col;
			out |= out << 16;	
			break;
		case 2: /* 16 bit */
			out = col | (col << 16);	
			break;
		default:
			out = col;
	}
	p9100_ctl_write_4(sc, reg, out);
}

/* initialize the drawing engine */
static void
p9100_init_engine(struct p9100_softc *sc)
{
	/* reset clipping rectangles */
	uint32_t rmax = ((sc->sc_width & 0x3fff) << 16) | 
	    (sc->sc_height & 0x3fff);
	
	p9100_ctl_write_4(sc, WINDOW_OFFSET, 0);
	p9100_ctl_write_4(sc, WINDOW_MIN, 0);
	p9100_ctl_write_4(sc, WINDOW_MAX, rmax);
	p9100_ctl_write_4(sc, BYTE_CLIP_MIN, 0);
	p9100_ctl_write_4(sc, BYTE_CLIP_MAX, rmax);
	p9100_ctl_write_4(sc, DRAW_MODE, 0);
	p9100_ctl_write_4(sc, PLANE_MASK, 0xffffffff);	
	p9100_ctl_write_4(sc, PATTERN0, 0xffffffff);	
	p9100_ctl_write_4(sc, PATTERN1, 0xffffffff);	
	p9100_ctl_write_4(sc, PATTERN2, 0xffffffff);	
	p9100_ctl_write_4(sc, PATTERN3, 0xffffffff);	
}

/* screen-to-screen blit */
void 
p9100_bitblt(struct p9100_softc *sc, int xs, int ys, int xd, int yd, int wi,
    int he, uint32_t rop)
{
	uint32_t src, dst, srcw, dstw, junk;
	
	src = ((xs & 0x3fff) << 16) | (ys & 0x3fff);
	dst = ((xd & 0x3fff) << 16) | (yd & 0x3fff);
	srcw = (((xs + wi - 1) & 0x3fff) << 16) | ((ys + he - 1) & 0x3fff);
	dstw = (((xd + wi - 1) & 0x3fff) << 16) | ((yd + he - 1) & 0x3fff);
	p9100_sync(sc);
	p9100_ctl_write_4(sc, RASTER_OP, rop);
	
	p9100_ctl_write_4(sc, ABS_XY0, src);
	
	p9100_ctl_write_4(sc, ABS_XY1, srcw);
	p9100_ctl_write_4(sc, ABS_XY2, dst);
	p9100_ctl_write_4(sc, ABS_XY3, dstw);
	junk = p9100_ctl_read_4(sc, COMMAND_BLIT);
}

/* solid rectangle fill */
void 
p9100_rectfill(struct p9100_softc *sc, int xs, int ys, int wi, int he, uint32_t col)
{
	uint32_t src, srcw, junk;
	
	src = ((xs & 0x3fff) << 16) | (ys & 0x3fff);
	srcw = (((xs + wi) & 0x3fff) << 16) | ((ys + he) & 0x3fff);
	p9100_sync(sc);
	p9100_set_color_reg(sc, FOREGROUND_COLOR, col);
	p9100_set_color_reg(sc, BACKGROUND_COLOR, col);
	p9100_ctl_write_4(sc, RASTER_OP, ROP_PAT);
	p9100_ctl_write_4(sc, COORD_INDEX, 0);
	p9100_ctl_write_4(sc, RECT_RTW_XY, src);
	p9100_ctl_write_4(sc, RECT_RTW_XY, srcw);
	junk=p9100_ctl_read_4(sc, COMMAND_QUAD);
}

/* setup for mono->colour expansion */
void 
p9100_setup_mono(struct p9100_softc *sc, int x, int y, int wi, int he, 
    uint32_t fg, uint32_t bg) 
{
	p9100_sync(sc);
	/* 
	 * this doesn't make any sense to me either, but for some reason the
	 * chip applies the foreground colour to 0 pixels 
	 */
	 
	p9100_set_color_reg(sc,FOREGROUND_COLOR,bg);
	p9100_set_color_reg(sc,BACKGROUND_COLOR,fg);

	p9100_ctl_write_4(sc, RASTER_OP, ROP_SRC);
	p9100_ctl_write_4(sc, ABS_X0, x);
	p9100_ctl_write_4(sc, ABS_XY1, (x << 16) | (y & 0xFFFFL));
	p9100_ctl_write_4(sc, ABS_X2, (x + wi));
	p9100_ctl_write_4(sc, ABS_Y3, he);
	/* now feed the data into the chip */
	sc->sc_mono_width = wi;
}

/* write monochrome data to the screen through the blitter */
void 
p9100_feed_line(struct p9100_softc *sc, int count, uint8_t *data)
{
	int i;
	uint32_t latch = 0, bork;
	int shift = 24;
	int to_go = sc->sc_mono_width;
	
	for (i = 0; i < count; i++) {
		bork = data[i];
		latch |= (bork << shift);
		if (shift == 0) {
			/* check how many bits are significant */
			if (to_go > 31) {
				p9100_ctl_write_4(sc, (PIXEL_1 + 
				    (31 << 2)), latch);
				to_go -= 32;
			} else
			{
				p9100_ctl_write_4(sc, (PIXEL_1 + 
				    ((to_go - 1) << 2)), latch);
				to_go = 0;
			}
			latch = 0;
			shift = 24;
		} else
			shift -= 8;
		}
	if (shift != 24)
		p9100_ctl_write_4(sc, (PIXEL_1 + ((to_go - 1) << 2)), latch);
}	

void
p9100_clearscreen(struct p9100_softc *sc)
{
	p9100_rectfill(sc, 0, 0, sc->sc_width, sc->sc_height, sc->sc_bg);
}

uint8_t
p9100_ramdac_read(struct p9100_softc *sc, bus_size_t off)
{
	sc->sc_junk = p9100_ctl_read_4(sc, PWRUP_CNFG);
	sc->sc_junk = bus_space_read_4(sc->sc_bustag, sc->sc_fb_memh, off);
	return ((bus_space_read_4(sc->sc_bustag, 
	    sc->sc_ctl_memh, off) >> 16) & 0xff);
}

void
p9100_ramdac_write(struct p9100_softc *sc, bus_size_t off, uint8_t v)
{
	sc->sc_junk = p9100_ctl_read_4(sc, PWRUP_CNFG);
	sc->sc_junk = bus_space_read_4(sc->sc_bustag, sc->sc_fb_memh, off);
	bus_space_write_4(sc->sc_bustag, sc->sc_ctl_memh, off, 
	    ((uint32_t)v) << 16);
}

/*
 * Undo the effect of an FBIOSVIDEO that turns the video off.
 */
static void
p9100unblank(struct device *dev)
{
	p9100_set_video((struct p9100_softc *)dev, 1);
}

static void
p9100_set_video(struct p9100_softc *sc, int enable)
{
	u_int32_t v = p9100_ctl_read_4(sc, SCRN_RPNT_CTL_1);
	
	if (enable)
		v |= VIDEO_ENABLED;
	else
		v &= ~VIDEO_ENABLED;
	p9100_ctl_write_4(sc, SCRN_RPNT_CTL_1, v);
#if NTCTRL > 0
	/* Turn On/Off the TFT if we know how.
	 */
	tadpole_set_video(enable);
#endif
}

static int
p9100_get_video(struct p9100_softc *sc)
{
	return (p9100_ctl_read_4(sc, SCRN_RPNT_CTL_1) & VIDEO_ENABLED) != 0;
}

/*
 * Load a subset of the current (new) colormap into the IBM RAMDAC.
 */
static void
p9100loadcmap(struct p9100_softc *sc, int start, int ncolors)
{
	int i;
	p9100_ramdac_write(sc, DAC_CMAP_WRIDX, start);

	for (i=0;i<ncolors;i++) {
		p9100_ramdac_write(sc, DAC_CMAP_DATA, 
		    sc->sc_cmap.cm_map[i + start][0]);
		p9100_ramdac_write(sc, DAC_CMAP_DATA, 
		    sc->sc_cmap.cm_map[i + start][1]);
		p9100_ramdac_write(sc, DAC_CMAP_DATA, 
		    sc->sc_cmap.cm_map[i + start][2]);
	}
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 */
paddr_t
p9100mmap(dev_t dev, off_t off, int prot)
{
	struct p9100_softc *sc = pnozz_cd.cd_devs[minor(dev)];

	if (off & PGOFSET)
		panic("p9100mmap");
	if (off < 0)
		return (-1);

#ifdef PNOZZ_EMUL_CG3
#define CG3_MMAP_OFFSET	0x04000000
	/* Make Xsun think we are a CG3 (SUN3COLOR)
	 */
	if (off >= CG3_MMAP_OFFSET && off < CG3_MMAP_OFFSET + sc->sc_fb_psize) {
		off -= CG3_MMAP_OFFSET;
		return (bus_space_mmap(sc->sc_bustag,
			sc->sc_fb_paddr,
			off,
			prot,
			BUS_SPACE_MAP_LINEAR));
	}
#endif

	if (off >= sc->sc_fb_psize + sc->sc_ctl_psize + sc->sc_cmd_psize)
		return (-1);

	if (off < sc->sc_fb_psize) {
		return (bus_space_mmap(sc->sc_bustag,
			sc->sc_fb_paddr,
			off,
			prot,
			BUS_SPACE_MAP_LINEAR));
	}
	off -= sc->sc_fb_psize;
	if (off < sc->sc_ctl_psize) {
		return (bus_space_mmap(sc->sc_bustag,
			sc->sc_ctl_paddr,
			off,
			prot,
			BUS_SPACE_MAP_LINEAR));
	}
	off -= sc->sc_ctl_psize;

	return (bus_space_mmap(sc->sc_bustag,
		sc->sc_cmd_paddr,
		off,
		prot,
		BUS_SPACE_MAP_LINEAR));
}

/* wscons stuff */

void
p9100_switch_screen(struct p9100_softc *sc)
{
	struct p9100_screen *scr, *oldscr;

	scr = sc->wanted;
	if (!scr) {
		printf("p9100_switch_screen: disappeared\n");
		(*sc->switchcb)(sc->switchcbarg, EIO, 0);
		return;
	}
	oldscr = sc->active; /* can be NULL! */
#ifdef DIAGNOSTIC
	if (oldscr) {
		if (!oldscr->active)
			panic("p9100_switch_screen: not active");
	}
#endif
	if (scr == oldscr)
		return;

#ifdef DIAGNOSTIC
	if (scr->active)
		panic("p9100_switch_screen: active");
#endif

	if (oldscr)
		oldscr->active = 0;
#ifdef notyet
	if (sc->currenttype != type) {
		p9100_set_screentype(sc, type);
		sc->currenttype = type;
	}
#endif

	/* Clear the entire screen. */		

	scr->active = 1;
	p9100_restore_screen(scr, &p9100_defscreendesc, scr->chars);

	sc->active = scr;
	
	scr->ri.ri_ops.cursor(scr, scr->cursoron, scr->cursorrow, 
	    scr->cursorcol);

	sc->wanted = 0;
	if (sc->switchcb)
		(*sc->switchcb)(sc->switchcbarg, 0, 0);
}

void
p9100_restore_screen(struct p9100_screen *scr,
    const struct wsscreen_descr *type, u_int16_t *mem)
{
	int i, j, offset = 0;
	uint16_t *charptr = scr->chars;
	long *attrptr = scr->attrs;
	
	p9100_clearscreen(scr->sc);
	for (i = 0; i < scr->ri.ri_rows; i++) {
		for (j = 0; j < scr->ri.ri_cols; j++) {
			p9100_putchar(scr, i, j, charptr[offset], 
			    attrptr[offset]);
			offset++;
		}
	}
	scr->cursordrawn = 0;
}

void
p9100_cursor(void *cookie, int on, int row, int col)
{
	struct rasops_info *ri = cookie;
	struct p9100_screen *scr = ri->ri_hw;
	struct p9100_softc *sc = scr->sc;
	int x, y, wi,he;
	
	wi = ri->ri_font->fontwidth;
	he = ri->ri_font->fontheight;
	
	if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = scr->cursorcol * wi + ri->ri_xorigin;
		y = scr->cursorrow * he + ri->ri_yorigin;
		if (scr->cursordrawn) {
			p9100_bitblt(sc, x, y, x, y, wi, he, (ROP_SRC ^ 0xff));
			scr->cursordrawn = 0;
		}
		scr->cursorrow = row;
		scr->cursorcol = col;
		if ((scr->cursoron = on) != 0)
		{
			x = scr->cursorcol * wi + ri->ri_xorigin;
			y = scr->cursorrow * he + ri->ri_yorigin;
			p9100_bitblt(sc, x, y, x, y, wi, he, (ROP_SRC ^ 0xff));
			scr->cursordrawn = 1;
		}
	} else {
		scr->cursoron = on;
		scr->cursorrow = row;
		scr->cursorcol = col;
		scr->cursordrawn = 0;
	}
}

#if 0
int
p9100_mapchar(void *cookie, int uni, u_int *index)
{
	return 0;
}
#endif

void
p9100_putchar(void *cookie, int row, int col, u_int c, long attr)
{
	struct rasops_info *ri = cookie;
	struct p9100_screen *scr = ri->ri_hw;
	struct p9100_softc *sc = scr->sc;
	int pos;
	
	if ((row >= 0) && (row < ri->ri_rows) && (col >= 0) && 
	     (col < ri->ri_cols)) {
		pos = col + row * ri->ri_cols;
		scr->attrs[pos] = attr;
		scr->chars[pos] = c;

#ifdef PNOZZ_SOFT_PUTCHAR
		if ((sc->putchar != NULL) && (	scr->active) && 
		    (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
			p9100_sync(sc);
			sc->putchar(cookie, row, col, c, attr);
		}
#else
		if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
			int fg, bg, uc, i;
			uint8_t *data;
			int x, y, wi,he;
			
			wi = ri->ri_font->fontwidth;
			he = ri->ri_font->fontheight;
		
			if (!CHAR_IN_FONT(c, ri->ri_font))
				return;
			bg = (u_char)ri->ri_devcmap[(attr >> 16) & 0xff];
			fg = (u_char)ri->ri_devcmap[(attr >> 24) & 0xff];
			x = ri->ri_xorigin + col * wi;
			y = ri->ri_yorigin + row * he;
			if (c == 0x20) {
				p9100_rectfill(sc, x, y, wi, he, bg);
			} else {
				uc = c-ri->ri_font->firstchar;
				data = (uint8_t *)ri->ri_font->data + uc * 
				    ri->ri_fontscale;

				p9100_setup_mono(sc, x, y, wi, 1, fg, bg);		
				for (i = 0; i < he; i++) {
					p9100_feed_line(sc, ri->ri_font->stride,
					    data);
					data += ri->ri_font->stride;
				}
				/*p9100_sync(sc);*/
			}
		}
#endif
	}
}

void
p9100_copycols(void *cookie, int row, int srccol, int dstcol, int ncols)
{
	struct rasops_info *ri = cookie;
	struct p9100_screen *scr = ri->ri_hw;
	struct p9100_softc *sc = scr->sc;
	int32_t xs, xd, y, width, height;
	int from = srccol + row * ri->ri_cols;
	int to = dstcol + row * ri->ri_cols;
	
	memmove(&scr->attrs[to], &scr->attrs[from], ncols * sizeof(long));
	memmove(&scr->chars[to], &scr->chars[from], ncols * sizeof(uint16_t));

	if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		xs = ri->ri_xorigin + ri->ri_font->fontwidth * srccol;
		xd = ri->ri_xorigin + ri->ri_font->fontwidth * dstcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;
		p9100_bitblt(sc, xs, y, xd, y, width, height, ROP_SRC);
	}
}

void
p9100_erasecols(void *cookie, int row, int startcol, int ncols, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct p9100_screen *scr = ri->ri_hw;
	struct p9100_softc *sc = scr->sc;
	int32_t x, y, width, height, bg;
	int start = startcol + row * ri->ri_cols;
	int end = start + ncols, i;
	
	for (i = start; i < end; i++) {
		scr->attrs[i] = fillattr;
		scr->chars[i] = 0x20;
	}
	if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin + ri->ri_font->fontwidth * startcol;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_font->fontwidth * ncols;
		height = ri->ri_font->fontheight;		
		bg = (fillattr >> 16) & 0xff;	
		p9100_rectfill(sc, x, y, width, height, bg);
	}
}

void
p9100_copyrows(void *cookie, int srcrow, int dstrow, int nrows)
{
	struct rasops_info *ri = cookie;
	struct p9100_screen *scr = ri->ri_hw;
	struct p9100_softc *sc = scr->sc;
	int32_t x, ys, yd, width, height;
	int from, to, len;
	
	from = ri->ri_cols * srcrow;
	to = ri->ri_cols * dstrow;
	len = ri->ri_cols * nrows;
	
	memmove(&scr->attrs[to], &scr->attrs[from], len * sizeof(long));
	memmove(&scr->chars[to], &scr->chars[from], len * sizeof(uint16_t));
	
	if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		ys = ri->ri_yorigin + ri->ri_font->fontheight * srcrow;
		yd = ri->ri_yorigin + ri->ri_font->fontheight * dstrow;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;		
		p9100_bitblt(sc, x, ys, x, yd, width, height, ROP_SRC);
	}
}

void
p9100_eraserows(void *cookie, int row, int nrows, long fillattr)
{
	struct rasops_info *ri = cookie;
	struct p9100_screen *scr = ri->ri_hw;
	struct p9100_softc *sc = scr->sc;
	int32_t x,y,width,height,bg;
	int start, end, i;
	
	start = ri->ri_cols * row;
	end = ri->ri_cols * (row + nrows);
	
	for (i = start; i < end; i++) {
		scr->attrs[i] = fillattr;
		scr->chars[i] = 0x20;
	}

	if ((scr->active) && (sc->sc_mode == WSDISPLAYIO_MODE_EMUL)) {
		x = ri->ri_xorigin;
		y = ri->ri_yorigin + ri->ri_font->fontheight * row;
		width = ri->ri_emuwidth;
		height = ri->ri_font->fontheight * nrows;		
		bg = (fillattr >> 16) & 0xff;		   
		p9100_rectfill(sc, x, y, width, height, bg);
	}
}

int
p9100_allocattr(void *cookie, int fg, int bg, int flags, long *attrp)
{
	if ((fg == 0) && (bg == 0))
	{
		fg = WS_DEFAULT_FG;
		bg = WS_DEFAULT_BG;
	}
	if (flags & WSATTR_REVERSE) {
		*attrp = (bg & 0xff) << 24 | (fg & 0xff) << 16 | 
		    (flags & 0xff) << 8;
	} else
		*attrp = (fg & 0xff) << 24 | (bg & 0xff) << 16 | 
		    (flags & 0xff) << 8;
	return 0;
}

/*
 * wsdisplay_accessops
 */

int
p9100_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct lwp *l)
{
	struct p9100_softc *sc = v;
	struct wsdisplay_fbinfo *wdf;
	struct p9100_screen *ms = sc->active;

	switch (cmd) {
		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_SB_P9100;
			return 0;
	
		case FBIOGVIDEO:
		case WSDISPLAYIO_GVIDEO:
			*(int *)data = p9100_get_video(sc);
			return 0;
			
		case WSDISPLAYIO_SVIDEO:
		case FBIOSVIDEO:
			p9100_set_video(sc, *(int *)data);
			return 0;

		case WSDISPLAYIO_GINFO:
			wdf = (void *)data;
			wdf->height = ms->ri.ri_height;
			wdf->width = ms->ri.ri_width;
			wdf->depth = ms->ri.ri_depth;
			wdf->cmsize = 256;
			return 0;

		case WSDISPLAYIO_GETCMAP:
			return p9100_getcmap(sc, (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_PUTCMAP:
			return p9100_putcmap(sc, (struct wsdisplay_cmap *)data);

		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data;
				if (new_mode != sc->sc_mode)
				{
					sc->sc_mode = new_mode;
					if (new_mode == WSDISPLAYIO_MODE_EMUL)
					{
						p9100_init_engine(sc);		
						p9100loadcmap(sc,0,256);
						p9100_restore_screen(ms, 
						    ms->type, ms->chars);
						p9100_cursor(ms, ms->cursoron, 
						    ms->cursorrow, 
						    ms->cursorcol);
					}
				}
			}
	}
	return EPASSTHROUGH;
}

paddr_t
p9100_mmap(void *v, off_t offset, int prot)
{
	struct p9100_softc *sc = v;	
	paddr_t pa;
	
	/* 'regular' framebuffer mmap()ing */
	if (offset < sc->sc_fb_psize) {
		pa = bus_space_mmap(sc->sc_bustag, sc->sc_fb_paddr + offset, 0, 
		    prot, BUS_SPACE_MAP_LINEAR);	
		return pa;
	}

	if ((offset >= sc->sc_fb_paddr) && (offset < (sc->sc_fb_paddr + 
	    sc->sc_fb_psize))) {
		pa = bus_space_mmap(sc->sc_bustag, offset, 0, prot, 
		    BUS_SPACE_MAP_LINEAR);	
		return pa;
	}

	if ((offset >= sc->sc_ctl_paddr) && (offset < (sc->sc_ctl_paddr + 
	    sc->sc_ctl_psize))) {
		pa = bus_space_mmap(sc->sc_bustag, offset, 0, prot, 
		    BUS_SPACE_MAP_LINEAR);	
		return pa;
	}

	return -1;
}

void
p9100_init_screen(struct p9100_softc *sc, struct p9100_screen *scr,
    int existing, long *defattr)
{
	struct rasops_info *ri = &scr->ri;
	int cnt;

	scr->sc = sc;
	/*scr->type = type;*/
	scr->dispoffset = 0;
	scr->cursorcol = 0;
	scr->cursorrow = 0;
	scr->cursordrawn=0;
	   
	ri->ri_depth = sc->sc_depth << 3;
	ri->ri_width = sc->sc_width;
	ri->ri_height = sc->sc_height;
	ri->ri_stride = sc->sc_stride;
	ri->ri_flg = RI_CENTER;

	ri->ri_bits = bus_space_vaddr(sc->sc_bustag, sc->sc_fb_memh);

#ifdef DEBUG_P9100
	printf("addr: %08lx\n",(ulong)ri->ri_bits);
#endif
	rasops_init(ri, sc->sc_height/8, sc->sc_width/8);
	ri->ri_caps = WSSCREEN_WSCOLORS;
	rasops_reconfig(ri, sc->sc_height / ri->ri_font->fontheight,
		    sc->sc_width / ri->ri_font->fontwidth);

	p9100_allocattr(ri, WS_DEFAULT_FG, WS_DEFAULT_BG, 0, defattr);

	/* 
	 * we allocate both chars and attributes in one chunk, attributes first 
	 * because they have the (potentially) bigger alignment 
	 */
	cnt=ri->ri_rows * ri->ri_cols;
	scr->attrs = (long *)malloc(cnt * (sizeof(long) + sizeof(uint16_t)),
	    M_DEVBUF, M_WAITOK);
	scr->chars = (uint16_t *)&scr->attrs[cnt];

	/* enable acceleration */
	ri->ri_hw = scr;
	ri->ri_ops.copyrows = p9100_copyrows;
	ri->ri_ops.copycols = p9100_copycols;
	ri->ri_ops.eraserows = p9100_eraserows;
	ri->ri_ops.erasecols = p9100_erasecols;
	ri->ri_ops.cursor = p9100_cursor;
	ri->ri_ops.allocattr = p9100_allocattr;
#ifdef PNOZZ_SOFT_PUTCHAR
	if (sc->putchar == NULL)
		sc->putchar=ri->ri_ops.putchar;
#endif
	ri->ri_ops.putchar = p9100_putchar;
	
	if (existing) {
		scr->active = 1;
	} else {
		scr->active = 0;
	}

	p9100_eraserows(&scr->ri, 0, ri->ri_rows, *defattr);

	LIST_INSERT_HEAD(&sc->screens, scr, next);
}

int
p9100_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *curxp, int *curyp, long *defattrp)
{
	struct p9100_softc *sc = v;
	struct p9100_screen *scr;

	scr = malloc(sizeof(struct p9100_screen), M_DEVBUF, M_WAITOK | M_ZERO);
	p9100_init_screen(sc, scr, 0, defattrp);

	if (sc->active == NULL) {
		scr->active = 1;
		sc->active = scr;
		sc->currenttype = type;
	}

	*cookiep = scr;
	*curxp = scr->cursorcol;
	*curyp = scr->cursorrow;
	return 0;
}

void
p9100_free_screen(void *v, void *cookie)
{
	struct p9100_softc *sc = v;
	struct p9100_screen *scr = cookie;

	LIST_REMOVE(scr, next);
	if (scr != &p9100_console_screen) {
		free(scr->attrs, M_DEVBUF);
		free(scr, M_DEVBUF);
	} else
		panic("p9100_free_screen: console");

	if (sc->active == scr)
		sc->active = 0;
}

int
p9100_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{
	struct p9100_softc *sc = v;
	struct p9100_screen *scr, *oldscr;

	scr = cookie;
	oldscr = sc->active;
	if (scr == oldscr)
		return 0;

	sc->wanted = scr;
	sc->switchcb = cb;
	sc->switchcbarg = cbarg;
	if (cb) {
		callout_reset(&sc->switch_callout, 0,
		    (void(*)(void *))p9100_switch_screen, sc);
		return EAGAIN;
	}

	p9100_switch_screen(sc);
	return 0;
}

int
p9100_putcmap(struct p9100_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int i, error;
	u_char rbuf[256], gbuf[256], bbuf[256];
	u_char *r, *g, *b;

	if (cm->index >= 256 || cm->count > 256 ||
	    (cm->index + cm->count) > 256)
		return EINVAL;
	error = copyin(cm->red, &rbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->green, &gbuf[index], count);
	if (error)
		return error;
	error = copyin(cm->blue, &bbuf[index], count);
	if (error)
		return error;

	r = &rbuf[index];
	g = &gbuf[index];
	b = &bbuf[index];

	for (i = 0; i < count; i++) {
		sc->sc_cmap.cm_map[index][0] = *r;
		sc->sc_cmap.cm_map[index][1] = *g;
		sc->sc_cmap.cm_map[index][2] = *b;
		index++;
		r++, g++, b++;
	}
	p9100loadcmap(sc, 0, 256);
	return 0;
}

int
p9100_getcmap(struct p9100_softc *sc, struct wsdisplay_cmap *cm)
{
	u_int index = cm->index;
	u_int count = cm->count;
	int error, i;
	uint8_t red[256],green[256],blue[256];

	if (index >= 255 || count > 256 || index + count > 256)
		return EINVAL;

	i = index;
	while (i < (index + count)) {
		red[i] = sc->sc_cmap.cm_map[i][0];
		green[i] = sc->sc_cmap.cm_map[i][1];
		blue[i] = sc->sc_cmap.cm_map[i][2];
		i++;
	}
	error = copyout(&red[index],   cm->red,   count);
	if (error)
		return error;
	error = copyout(&green[index], cm->green, count);
	if (error)
		return error;
	error = copyout(&blue[index],  cm->blue,  count);
	if (error)
		return error;

	return 0;
}

#if 0
int
p9100_load_font(void *v, void *cookie, struct wsdisplay_font *data)
{

	return 0;
}
#endif

int
p9100_intr(void *arg)
{
	/*p9100_softc *sc=arg;
	printf(".");*/
	return 1;
}

void
p9100_init_cursor(struct p9100_softc *sc)
{
	
	memset(&sc->sc_cursor, 0, sizeof(struct pnozz_cursor));
	sc->sc_cursor.pc_size.x = 64;
	sc->sc_cursor.pc_size.y = 64;

}

void
p9100_set_fbcursor(struct p9100_softc *sc)
{
#ifdef PNOZZ_PARANOID
	int s;
	
	s = splhigh();	/* just in case... */
#endif
	/* set position and hotspot */
	p9100_ramdac_write(sc, DAC_INDX_CTL, DAC_INDX_AUTOINCR);
	p9100_ramdac_write(sc, DAC_INDX_HI, 0);
	p9100_ramdac_write(sc, DAC_INDX_LO, DAC_CURSOR_CTL);
	if (sc->sc_cursor.pc_enable) {
		p9100_ramdac_write(sc, DAC_INDX_DATA, DAC_CURSOR_X11 | 
		    DAC_CURSOR_64);
	} else
		p9100_ramdac_write(sc, DAC_INDX_DATA, DAC_CURSOR_OFF);
	/* next two registers - x low, high, y low, high */
	p9100_ramdac_write(sc, DAC_INDX_DATA, sc->sc_cursor.pc_pos.x & 0xff);
	p9100_ramdac_write(sc, DAC_INDX_DATA, (sc->sc_cursor.pc_pos.x >> 8) & 
	    0xff);
	p9100_ramdac_write(sc, DAC_INDX_DATA, sc->sc_cursor.pc_pos.y & 0xff);
	p9100_ramdac_write(sc, DAC_INDX_DATA, (sc->sc_cursor.pc_pos.y >> 8) & 
	    0xff);
	/* hotspot */
	p9100_ramdac_write(sc, DAC_INDX_DATA, sc->sc_cursor.pc_hot.x & 0xff);
	p9100_ramdac_write(sc, DAC_INDX_DATA, sc->sc_cursor.pc_hot.y & 0xff);
	
#ifdef PNOZZ_PARANOID
	splx(s);
#endif

}

void
p9100_setcursorcmap(struct p9100_softc *sc)
{
	int i;
	
#ifdef PNOZZ_PARANOID
	int s;
	s = splhigh();	/* just in case... */
#endif
	
	/* set cursor colours */
	p9100_ramdac_write(sc, DAC_INDX_CTL, DAC_INDX_AUTOINCR);
	p9100_ramdac_write(sc, DAC_INDX_HI, 0);
	p9100_ramdac_write(sc, DAC_INDX_LO, DAC_CURSOR_COL_1);

	for (i = 0; i < 3; i++) {
		p9100_ramdac_write(sc, DAC_INDX_DATA, sc->sc_cursor.red[i]);
		p9100_ramdac_write(sc, DAC_INDX_DATA, sc->sc_cursor.green[i]);
		p9100_ramdac_write(sc, DAC_INDX_DATA, sc->sc_cursor.blue[i]);
	}
	
#ifdef PNOZZ_PARANOID
	splx(s);
#endif
}

void
p9100_loadcursor(struct p9100_softc *sc)
{
	uint32_t *image, *mask;
	uint32_t bit, bbit, im, ma;
	int i, j, k;
	uint8_t latch1, latch2;
		
#ifdef PNOZZ_PARANOID
	int s;
	s = splhigh();	/* just in case... */
#endif
	/* set cursor shape */
	p9100_ramdac_write(sc, DAC_INDX_CTL, DAC_INDX_AUTOINCR);
	p9100_ramdac_write(sc, DAC_INDX_HI, 1);
	p9100_ramdac_write(sc, DAC_INDX_LO, 0);

	image = sc->sc_cursor.pc_bits;
	mask = &sc->sc_cursor.pc_bits[0x80];

	for (i = 0; i < 0x80; i++) {
		bit = 0x80000000;
		im = image[i];
		ma = mask[i];
		for (k = 0; k < 4; k++) {
			bbit = 0x1;
			latch1 = 0;
			for (j = 0; j < 4; j++) {
				if (im & bit)
					latch1 |= bbit;
				bbit <<= 1;
				if (ma & bit)
					latch1 |= bbit;
				bbit <<= 1;
				bit >>= 1;
			}
			bbit = 0x1;
			latch2 = 0;
			for (j = 0; j < 4; j++) {
				if (im & bit)
					latch2 |= bbit;
				bbit <<= 1;
				if (ma & bit)
					latch2 |= bbit;
				bbit <<= 1;
				bit >>= 1;
			}
			p9100_ramdac_write(sc, DAC_INDX_DATA, latch1);
			p9100_ramdac_write(sc, DAC_INDX_DATA, latch2);
		}
	}
#ifdef DEBUG_CURSOR
	printf("image:\n");
	for (i=0;i<0x80;i+=2)
		printf("%08x %08x\n", image[i], image[i+1]);
	printf("mask:\n");
	for (i=0;i<0x80;i+=2)
		printf("%08x %08x\n", mask[i], mask[i+1]);
#endif
#ifdef PNOZZ_PARANOID
	splx(s);
#endif
}
