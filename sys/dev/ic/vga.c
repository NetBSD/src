/* $NetBSD: vga.c,v 1.62 2002/07/07 07:37:50 junyoung Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vga.c,v 1.62 2002/07/07 07:37:50 junyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <machine/bus.h>

#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/unicode.h>
#include <dev/wsfont/wsfont.h>

#include <dev/ic/pcdisplay.h>

/* for WSCONS_SUPPORT_PCVTFONTS and WSDISPLAY_CHARFUNCS */
#include "opt_wsdisplay_compat.h"

static struct wsdisplay_font _vga_builtinfont = {
	"builtin",
	0, 256,
	WSDISPLAY_FONTENC_IBM,
	8, 16, 1,
	WSDISPLAY_FONTORDER_L2R, 0,
	0
};

struct egavga_font {
	struct wsdisplay_font *wsfont;
	int cookie; /* wsfont handle, -1 invalid */
	int slot; /* in adapter RAM */
	int usecount;
	TAILQ_ENTRY(egavga_font) next; /* LRU queue */
};

static struct egavga_font vga_builtinfont = {
	&_vga_builtinfont,
	-1, 0
};

#ifdef VGA_CONSOLE_SCREENTYPE
static struct egavga_font vga_consolefont;
#endif

struct vgascreen {
	struct pcdisplayscreen pcs;

	LIST_ENTRY(vgascreen) next;

	struct vga_config *cfg;

	/* videostate */
	struct egavga_font *fontset1, *fontset2;
	/* font data */
	/* palette */

	int mindispoffset, maxdispoffset;
};

static int vgaconsole, vga_console_type, vga_console_attached;
static struct vgascreen vga_console_screen;
static struct vga_config vga_console_vc;

struct egavga_font *egavga_getfont(struct vga_config *, struct vgascreen *,
				   char *, int);
void egavga_unreffont(struct vga_config *, struct egavga_font *);

int vga_selectfont(struct vga_config *, struct vgascreen *, char *, char *);
void vga_init_screen(struct vga_config *, struct vgascreen *,
		     const struct wsscreen_descr *, int, long *);
void vga_init(struct vga_config *, bus_space_tag_t, bus_space_tag_t);
static void vga_setfont(struct vga_config *, struct vgascreen *);

static int vga_mapchar(void *, int, unsigned int *);
static int vga_allocattr(void *, int, int, int, long *);
static void vga_copyrows(void *, int, int, int);

const struct wsdisplay_emulops vga_emulops = {
	pcdisplay_cursor,
	vga_mapchar,
	pcdisplay_putchar,
	pcdisplay_copycols,
	pcdisplay_erasecols,
	vga_copyrows,
	pcdisplay_eraserows,
	vga_allocattr
};

/*
 * translate WS(=ANSI) color codes to standard pc ones
 */
static const unsigned char fgansitopc[] = {
#ifdef __alpha__
	/*
	 * XXX DEC HAS SWITCHED THE CODES FOR BLUE AND RED!!!
	 * XXX We should probably not bother with this
	 * XXX (reinitialize the palette registers).
	 */
	FG_BLACK, FG_BLUE, FG_GREEN, FG_CYAN, FG_RED,
	FG_MAGENTA, FG_BROWN, FG_LIGHTGREY
#else
	FG_BLACK, FG_RED, FG_GREEN, FG_BROWN, FG_BLUE,
	FG_MAGENTA, FG_CYAN, FG_LIGHTGREY
#endif
}, bgansitopc[] = {
#ifdef __alpha__
	BG_BLACK, BG_BLUE, BG_GREEN, BG_CYAN, BG_RED,
	BG_MAGENTA, BG_BROWN, BG_LIGHTGREY
#else
	BG_BLACK, BG_RED, BG_GREEN, BG_BROWN, BG_BLUE,
	BG_MAGENTA, BG_CYAN, BG_LIGHTGREY
#endif
};

const struct wsscreen_descr vga_25lscreen = {
	"80x25", 80, 25,
	&vga_emulops,
	8, 16,
	WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_BLINK
}, vga_25lscreen_mono = {
	"80x25", 80, 25,
	&vga_emulops,
	8, 16,
	WSSCREEN_HILIT | WSSCREEN_UNDERLINE | WSSCREEN_BLINK | WSSCREEN_REVERSE
}, vga_25lscreen_bf = {
	"80x25bf", 80, 25,
	&vga_emulops,
	8, 16,
	WSSCREEN_WSCOLORS | WSSCREEN_BLINK
}, vga_40lscreen = {
	"80x40", 80, 40,
	&vga_emulops,
	8, 10,
	WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_BLINK
}, vga_40lscreen_mono = {
	"80x40", 80, 40,
	&vga_emulops,
	8, 10,
	WSSCREEN_HILIT | WSSCREEN_UNDERLINE | WSSCREEN_BLINK | WSSCREEN_REVERSE
}, vga_40lscreen_bf = {
	"80x40bf", 80, 40,
	&vga_emulops,
	8, 10,
	WSSCREEN_WSCOLORS | WSSCREEN_BLINK
}, vga_50lscreen = {
	"80x50", 80, 50,
	&vga_emulops,
	8, 8,
	WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_BLINK
}, vga_50lscreen_mono = {
	"80x50", 80, 50,
	&vga_emulops,
	8, 8,
	WSSCREEN_HILIT | WSSCREEN_UNDERLINE | WSSCREEN_BLINK | WSSCREEN_REVERSE
}, vga_50lscreen_bf = {
	"80x50bf", 80, 50,
	&vga_emulops,
	8, 8,
	WSSCREEN_WSCOLORS | WSSCREEN_BLINK
}, vga_24lscreen = {
	"80x24", 80, 24,
	&vga_emulops,
	8, 16,
	WSSCREEN_WSCOLORS | WSSCREEN_HILIT | WSSCREEN_BLINK
}, vga_24lscreen_mono = {
	"80x24", 80, 24,
	&vga_emulops,
	8, 16,
	WSSCREEN_HILIT | WSSCREEN_UNDERLINE | WSSCREEN_BLINK | WSSCREEN_REVERSE
}, vga_24lscreen_bf = {
	"80x24bf", 80, 24,
	&vga_emulops,
	8, 16,
	WSSCREEN_WSCOLORS | WSSCREEN_BLINK
};

#define VGA_SCREEN_CANTWOFONTS(type) (!((type)->capabilities & WSSCREEN_HILIT))

const struct wsscreen_descr *_vga_scrlist[] = {
	&vga_25lscreen,
	&vga_25lscreen_bf,
	&vga_40lscreen,
	&vga_40lscreen_bf,
	&vga_50lscreen,
	&vga_50lscreen_bf,
	&vga_24lscreen,
	&vga_24lscreen_bf,
	/* XXX other formats, graphics screen? */
}, *_vga_scrlist_mono[] = {
	&vga_25lscreen_mono,
	&vga_40lscreen_mono,
	&vga_50lscreen_mono,
	&vga_24lscreen_mono,
	/* XXX other formats, graphics screen? */
};

const struct wsscreen_list vga_screenlist = {
	sizeof(_vga_scrlist) / sizeof(struct wsscreen_descr *),
	_vga_scrlist
}, vga_screenlist_mono = {
	sizeof(_vga_scrlist_mono) / sizeof(struct wsscreen_descr *),
	_vga_scrlist_mono
};

static int	vga_ioctl(void *, u_long, caddr_t, int, struct proc *);
static paddr_t	vga_mmap(void *, off_t, int);
static int	vga_alloc_screen(void *, const struct wsscreen_descr *,
				 void **, int *, int *, long *);
static void	vga_free_screen(void *, void *);
static int	vga_show_screen(void *, void *, int,
				void (*)(void *, int, int), void *);
static int	vga_load_font(void *, void *, struct wsdisplay_font *);
#ifdef WSDISPLAY_CHARFUNCS
static int	vga_getwschar(void *, struct wsdisplay_char *);
static int	vga_putwschar(void *, struct wsdisplay_char *);
#endif /* WSDISPLAY_CHARFUNCS */

void vga_doswitch(struct vga_config *);

const struct wsdisplay_accessops vga_accessops = {
	vga_ioctl,
	vga_mmap,
	vga_alloc_screen,
	vga_free_screen,
	vga_show_screen,
	vga_load_font,
	NULL,
#ifdef WSDISPLAY_CHARFUNCS
	vga_getwschar,
	vga_putwschar
#else /* WSDISPLAY_CHARFUNCS */
	NULL,
	NULL
#endif /* WSDISPLAY_CHARFUNCS */
};

/*
 * The following functions implement back-end configuration grabbing
 * and attachment.
 */
int
vga_common_probe(bus_space_tag_t iot, bus_space_tag_t memt)
{
	bus_space_handle_t ioh_vga, ioh_6845, memh;
	u_int8_t regval;
	u_int16_t vgadata;
	int gotio_vga, gotio_6845, gotmem, mono, rv;
	int dispoffset;

	gotio_vga = gotio_6845 = gotmem = rv = 0;

	if (bus_space_map(iot, 0x3c0, 0x10, 0, &ioh_vga))
		goto bad;
	gotio_vga = 1;

	/* read "misc output register" */
	regval = bus_space_read_1(iot, ioh_vga, 0xc);
	mono = !(regval & 1);

	if (bus_space_map(iot, (mono ? 0x3b0 : 0x3d0), 0x10, 0, &ioh_6845))
		goto bad;
	gotio_6845 = 1;

	if (bus_space_map(memt, 0xa0000, 0x20000, 0, &memh))
		goto bad;
	gotmem = 1;

	dispoffset = (mono ? 0x10000 : 0x18000);

	vgadata = bus_space_read_2(memt, memh, dispoffset);
	bus_space_write_2(memt, memh, dispoffset, 0xa55a);
	if (bus_space_read_2(memt, memh, dispoffset) != 0xa55a)
		goto bad;
	bus_space_write_2(memt, memh, dispoffset, vgadata);

	/*
	 * check if this is really a VGA
	 * (try to write "Color Select" register as XFree86 does)
	 * XXX check before if at least EGA?
	 */
	/* reset state */
	(void) bus_space_read_1(iot, ioh_6845, 10);
	bus_space_write_1(iot, ioh_vga, VGA_ATC_INDEX,
			  20 | 0x20); /* colselect | enable */
	regval = bus_space_read_1(iot, ioh_vga, VGA_ATC_DATAR);
	/* toggle the implemented bits */
	bus_space_write_1(iot, ioh_vga, VGA_ATC_DATAW, regval ^ 0x0f);
	bus_space_write_1(iot, ioh_vga, VGA_ATC_INDEX,
			  20 | 0x20);
	/* read back */
	if (bus_space_read_1(iot, ioh_vga, VGA_ATC_DATAR) != (regval ^ 0x0f))
		goto bad;
	/* restore contents */
	bus_space_write_1(iot, ioh_vga, VGA_ATC_DATAW, regval);

	rv = 1;
bad:
	if (gotio_vga)
		bus_space_unmap(iot, ioh_vga, 0x10);
	if (gotio_6845)
		bus_space_unmap(iot, ioh_6845, 0x10);
	if (gotmem)
		bus_space_unmap(memt, memh, 0x20000);

	return (rv);
}

/*
 * We want at least ASCII 32..127 be present in the
 * first font slot.
 */
#define vga_valid_primary_font(f) \
	(f->wsfont->encoding == WSDISPLAY_FONTENC_IBM || \
	f->wsfont->encoding == WSDISPLAY_FONTENC_ISO || \
	f->wsfont->encoding == WSDISPLAY_FONTENC_ISO7)

struct egavga_font *
egavga_getfont(struct vga_config *vc, struct vgascreen *scr, char *name,
	       int primary)
{
	struct egavga_font *f;
	int cookie;
	struct wsdisplay_font *wf;

	TAILQ_FOREACH(f, &vc->vc_fontlist, next) {
		if (wsfont_matches(f->wsfont, name,
				   8, scr->pcs.type->fontheight, 0) &&
		    (!primary || vga_valid_primary_font(f))) {
#ifdef VGAFONTDEBUG
			if (scr != &vga_console_screen || vga_console_attached)
				printf("vga_getfont: %s already present\n",
				       name ? name : "<default>");
#endif
			goto found;
		}
	}

	cookie = wsfont_find(name, 8, scr->pcs.type->fontheight, 0,
	                     WSDISPLAY_FONTORDER_L2R, 0);
	/* XXX obey "primary" */
	if (cookie == -1) {
#ifdef VGAFONTDEBUG
		if (scr != &vga_console_screen || vga_console_attached)
			printf("vga_getfont: %s not found\n",
			       name ? name : "<default>");
#endif
		return (0);
	}

	if (wsfont_lock(cookie, &wf))
		return (0);

#ifdef VGA_CONSOLE_SCREENTYPE
	if (scr == &vga_console_screen)
		f = &vga_consolefont;
	else
#endif
	f = malloc(sizeof(struct egavga_font), M_DEVBUF, M_NOWAIT);
	if (!f) {
		wsfont_unlock(cookie);
		return (0);
	}
	f->wsfont = wf;
	f->cookie = cookie;
	f->slot = -1; /* not yet loaded */
	f->usecount = 0; /* incremented below */
	TAILQ_INSERT_TAIL(&vc->vc_fontlist, f, next);

found:
	f->usecount++;
#ifdef VGAFONTDEBUG
	if (scr != &vga_console_screen || vga_console_attached)
		printf("vga_getfont: usecount=%d\n", f->usecount);
#endif
	return (f);
}

void
egavga_unreffont(struct vga_config *vc, struct egavga_font *f)
{

	f->usecount--;
#ifdef VGAFONTDEBUG
	printf("vga_unreffont: usecount=%d\n", f->usecount);
#endif
	if (f->usecount == 0 && f->cookie != -1) {
		TAILQ_REMOVE(&vc->vc_fontlist, f, next);
		if (f->slot != -1) {
			KASSERT(vc->vc_fonts[f->slot] == f);
			vc->vc_fonts[f->slot] = 0;
		}
		wsfont_unlock(f->cookie);
#ifdef VGA_CONSOLE_SCREENTYPE
		if (f != &vga_consolefont)
#endif
		free(f, M_DEVBUF);
	}
}

int
vga_selectfont(struct vga_config *vc, struct vgascreen *scr, char *name1,
	       char *name2)
{
	const struct wsscreen_descr *type = scr->pcs.type;
	struct egavga_font *f1, *f2;

	f1 = egavga_getfont(vc, scr, name1, 1);
	if (!f1)
		return (ENXIO);

	if (VGA_SCREEN_CANTWOFONTS(type) && name2) {
		f2 = egavga_getfont(vc, scr, name2, 0);
		if (!f2) {
			egavga_unreffont(vc, f1);
			return (ENXIO);
		}
	} else
		f2 = 0;

#ifdef VGAFONTDEBUG
	if (scr != &vga_console_screen || vga_console_attached) {
		printf("vga (%s): font1=%s (slot %d)", type->name,
		       f1->wsfont->name, f1->slot);
		if (f2)
			printf(", font2=%s (slot %d)",
			       f2->wsfont->name, f2->slot);
		printf("\n");
	}
#endif
	if (scr->fontset1)
		egavga_unreffont(vc, scr->fontset1);
	scr->fontset1 = f1;
	if (scr->fontset2)
		egavga_unreffont(vc, scr->fontset2);
	scr->fontset2 = f2;
	return (0);
}

void
vga_init_screen(struct vga_config *vc, struct vgascreen *scr,
		const struct wsscreen_descr *type, int existing, long *attrp)
{
	int cpos;
	int res;

	scr->cfg = vc;
	scr->pcs.hdl = (struct pcdisplay_handle *)&vc->hdl;
	scr->pcs.type = type;
	scr->pcs.active = existing;
	scr->mindispoffset = 0;
	scr->maxdispoffset = 0x8000 - type->nrows * type->ncols * 2;

	if (existing) {
		vc->active = scr;

		cpos = vga_6845_read(&vc->hdl, cursorh) << 8;
		cpos |= vga_6845_read(&vc->hdl, cursorl);

		/* make sure we have a valid cursor position */
		if (cpos < 0 || cpos >= type->nrows * type->ncols)
			cpos = 0;

		scr->pcs.dispoffset = vga_6845_read(&vc->hdl, startadrh) << 9;
		scr->pcs.dispoffset |= vga_6845_read(&vc->hdl, startadrl) << 1;

		/* make sure we have a valid memory offset */
		if (scr->pcs.dispoffset < scr->mindispoffset ||
		    scr->pcs.dispoffset > scr->maxdispoffset)
			scr->pcs.dispoffset = scr->mindispoffset;

		if (type != vc->currenttype) {
			vga_setscreentype(&vc->hdl, type);
			vc->currenttype = type;
		}
	} else {
		cpos = 0;
		scr->pcs.dispoffset = scr->mindispoffset;
	}

	scr->pcs.cursorrow = cpos / type->ncols;
	scr->pcs.cursorcol = cpos % type->ncols;
	pcdisplay_cursor_init(&scr->pcs, existing);

#ifdef __alpha__
	if (!vc->hdl.vh_mono)
		/*
		 * DEC firmware uses a blue background.
		 */
		res = vga_allocattr(scr, WSCOL_WHITE, WSCOL_BLUE,
				     WSATTR_WSCOLORS, attrp);
	else
#endif
	res = vga_allocattr(scr, 0, 0, 0, attrp);
#ifdef DIAGNOSTIC
	if (res)
		panic("vga_init_screen: attribute botch");
#endif

	scr->pcs.mem = NULL;

	wsfont_init();
	scr->fontset1 = scr->fontset2 = 0;
	if (vga_selectfont(vc, scr, 0, 0)) {
		if (scr == &vga_console_screen)
			panic("vga_init_screen: no font");
		else
			printf("vga_init_screen: no font\n");
	}
	if (existing)
		vga_setfont(vc, scr);

	vc->nscreens++;
	LIST_INSERT_HEAD(&vc->screens, scr, next);
}

void
vga_init(struct vga_config *vc, bus_space_tag_t iot, bus_space_tag_t memt)
{
	struct vga_handle *vh = &vc->hdl;
	u_int8_t mor;
	int i;

        vh->vh_iot = iot;
        vh->vh_memt = memt;

        if (bus_space_map(vh->vh_iot, 0x3c0, 0x10, 0, &vh->vh_ioh_vga))
                panic("vga_init: couldn't map vga io");

	/* read "misc output register" */
	mor = bus_space_read_1(vh->vh_iot, vh->vh_ioh_vga, 0xc);
	vh->vh_mono = !(mor & 1);

	if (bus_space_map(vh->vh_iot, (vh->vh_mono ? 0x3b0 : 0x3d0), 0x10, 0,
			  &vh->vh_ioh_6845))
                panic("vga_init: couldn't map 6845 io");

        if (bus_space_map(vh->vh_memt, 0xa0000, 0x20000, 0, &vh->vh_allmemh))
                panic("vga_init: couldn't map memory");

        if (bus_space_subregion(vh->vh_memt, vh->vh_allmemh,
				(vh->vh_mono ? 0x10000 : 0x18000), 0x8000,
				&vh->vh_memh))
                panic("vga_init: mem subrange failed");

	/* should only reserve the space (no need to map - save KVM) */
	vc->vc_biostag = memt;
	if (bus_space_map(vc->vc_biostag, 0xc0000, 0x8000, 0,
			  &vc->vc_bioshdl))
		vc->vc_biosmapped = 0;
	else
		vc->vc_biosmapped = 1;

	vc->nscreens = 0;
	LIST_INIT(&vc->screens);
	vc->active = NULL;
	vc->currenttype = vh->vh_mono ? &vga_25lscreen_mono : &vga_25lscreen;
	callout_init(&vc->vc_switch_callout);

	vc->vc_fonts[0] = &vga_builtinfont;
	for (i = 1; i < 8; i++)
		vc->vc_fonts[i] = 0;
	TAILQ_INIT(&vc->vc_fontlist);
	TAILQ_INSERT_HEAD(&vc->vc_fontlist, &vga_builtinfont, next);

	vc->currentfontset1 = vc->currentfontset2 = 0;
}

void
vga_common_attach(struct vga_softc *sc, bus_space_tag_t iot,
		  bus_space_tag_t memt, int type, int quirks,
		  const struct vga_funcs *vf)
{
	int console;
	struct vga_config *vc;
	struct wsemuldisplaydev_attach_args aa;

	console = vga_is_console(iot, type);

	if (console) {
		vc = &vga_console_vc;
		vga_console_attached = 1;
	} else {
		vc = malloc(sizeof(struct vga_config), M_DEVBUF, M_WAITOK);
		vga_init(vc, iot, memt);
	}

	if (quirks & VGA_QUIRK_ONEFONT) {
		vc->vc_nfontslots = 1;
#ifndef VGA_CONSOLE_ATI_BROKEN_FONTSEL
		/*
		 * XXX maybe invalidate font in slot > 0, but this can
		 * only be happen with VGA_CONSOLE_SCREENTYPE, and then
		 * we require VGA_CONSOLE_ATI_BROKEN_FONTSEL anyway.
		 */
#endif
	} else {
		vc->vc_nfontslots = 8;
#ifndef VGA_CONSOLE_ATI_BROKEN_FONTSEL
		/*
		 * XXX maybe validate builtin font shifted to slot 1 if
		 * slot 0 got overwritten because of VGA_CONSOLE_SCREENTYPE,
		 * but it will be reloaded anyway if needed.
		 */
#endif
	}

	/*
	 * Save the builtin font to memory. In case it got overwritten
	 * in console initialization, use the copy in slot 1.
	 */
#ifdef VGA_CONSOLE_ATI_BROKEN_FONTSEL
#define BUILTINFONTLOC (vga_builtinfont.slot == -1 ? 1 : 0)
#else
	KASSERT(vga_builtinfont.slot == 0);
#define BUILTINFONTLOC (0)
#endif
	vga_builtinfont.wsfont->data =
		malloc(256 * vga_builtinfont.wsfont->fontheight,
		       M_DEVBUF, M_WAITOK);
	vga_readoutchars(&vc->hdl, BUILTINFONTLOC, 0, 256,
			 vga_builtinfont.wsfont->fontheight,
			 vga_builtinfont.wsfont->data);

	vc->vc_type = type;
	vc->vc_funcs = vf;

	sc->sc_vc = vc;
	vc->softc = sc;

	aa.console = console;
	aa.scrdata = (vc->hdl.vh_mono ? &vga_screenlist_mono : &vga_screenlist);
	aa.accessops = &vga_accessops;
	aa.accesscookie = vc;

        config_found(&sc->sc_dev, &aa, wsemuldisplaydevprint);
}

int
vga_cnattach(bus_space_tag_t iot, bus_space_tag_t memt, int type, int check)
{
	long defattr;
	const struct wsscreen_descr *scr;

	if (check && !vga_common_probe(iot, memt))
		return (ENXIO);

	/* set up bus-independent VGA configuration */
	vga_init(&vga_console_vc, iot, memt);
#ifdef VGA_CONSOLE_SCREENTYPE
	scr = wsdisplay_screentype_pick(vga_console_vc.hdl.vh_mono ?
	       &vga_screenlist_mono : &vga_screenlist, VGA_CONSOLE_SCREENTYPE);
	if (!scr)
		panic("vga_cnattach: invalid screen type");
#else
	scr = vga_console_vc.currenttype;
#endif
#ifdef VGA_CONSOLE_ATI_BROKEN_FONTSEL
	/*
	 * On some (most/all?) ATI cards, only font slot 0 is usable.
	 * vga_init_screen() might need font slot 0 for a non-default
	 * console font, so save the builtin VGA font to another font slot.
	 * The attach() code will take care later.
	 */
	vga_copyfont01(&vga_console_vc.hdl);
	vga_console_vc.vc_nfontslots = 1;
#else
	vga_console_vc.vc_nfontslots = 8;
#endif
	vga_init_screen(&vga_console_vc, &vga_console_screen, scr, 1, &defattr);

	wsdisplay_cnattach(scr, &vga_console_screen,
			   vga_console_screen.pcs.cursorcol,
			   vga_console_screen.pcs.cursorrow,
			   defattr);

	vgaconsole = 1;
	vga_console_type = type;
	return (0);
}

int
vga_is_console(bus_space_tag_t iot, int type)
{
	if (vgaconsole &&
	    !vga_console_attached &&
	    iot == vga_console_vc.hdl.vh_iot &&
	    (vga_console_type == -1 || (type == vga_console_type)))
		return (1);
	return (0);
}

#define	VGA_TS_BLANK	0x20

static int
vga_get_video(struct vga_config *vc)
{
	return (vga_ts_read(&vc->hdl, mode) & VGA_TS_BLANK) == 0;
}

static void
vga_set_video(struct vga_config *vc, int state)
{
	int val;

	vga_ts_write(&vc->hdl, syncreset, 0x01);
	if (state) {					/* unblank screen */
		val = vga_ts_read(&vc->hdl, mode);
		vga_ts_write(&vc->hdl, mode, val & ~VGA_TS_BLANK);
#ifndef VGA_NO_VBLANK
		val = vga_6845_read(&vc->hdl, mode);
		vga_6845_write(&vc->hdl, mode, val | 0x80);
#endif
	} else {					/* blank screen */
		val = vga_ts_read(&vc->hdl, mode);
		vga_ts_write(&vc->hdl, mode, val | VGA_TS_BLANK);
#ifndef VGA_NO_VBLANK
		val = vga_6845_read(&vc->hdl, mode);
		vga_6845_write(&vc->hdl, mode, val & ~0x80);
#endif
	}
	vga_ts_write(&vc->hdl, syncreset, 0x03);
}

int
vga_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct vga_config *vc = v;
	const struct vga_funcs *vf = vc->vc_funcs;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = vc->vc_type;
		return 0;

	case WSDISPLAYIO_GINFO:
		/* XXX should get detailed hardware information here */
		return EPASSTHROUGH;

	case WSDISPLAYIO_GVIDEO:
		*(int *)data = (vga_get_video(vc) ? WSDISPLAYIO_VIDEO_ON :
				WSDISPLAYIO_VIDEO_OFF);
		return 0;

	case WSDISPLAYIO_SVIDEO:
		vga_set_video(vc, *(int *)data == WSDISPLAYIO_VIDEO_ON);
		return 0;

	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		/* NONE of these operations are by the generic VGA driver. */
		return EPASSTHROUGH;
	}

	if (vc->vc_funcs == NULL)
		return (EPASSTHROUGH);

	if (vf->vf_ioctl == NULL)
		return (EPASSTHROUGH);

	return ((*vf->vf_ioctl)(v, cmd, data, flag, p));
}

static paddr_t
vga_mmap(void *v, off_t offset, int prot)
{
	struct vga_config *vc = v;
	const struct vga_funcs *vf = vc->vc_funcs;

	if (vc->vc_funcs == NULL)
		return (-1);

	if (vf->vf_mmap == NULL)
		return (-1);

	return ((*vf->vf_mmap)(v, offset, prot));
}

int
vga_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
		 int *curxp, int *curyp, long *defattrp)
{
	struct vga_config *vc = v;
	struct vgascreen *scr;

	if (vc->nscreens == 1) {
		struct vgascreen *scr1 = vc->screens.lh_first;
		/*
		 * When allocating the second screen, get backing store
		 * for the first one too.
		 * XXX We could be more clever and use video RAM.
		 */
		scr1->pcs.mem =
		  malloc(scr1->pcs.type->ncols * scr1->pcs.type->nrows * 2,
			 M_DEVBUF, M_WAITOK);
	}

	scr = malloc(sizeof(struct vgascreen), M_DEVBUF, M_WAITOK);
	vga_init_screen(vc, scr, type, vc->nscreens == 0, defattrp);

	if (vc->nscreens > 1) {
		scr->pcs.mem = malloc(type->ncols * type->nrows * 2,
				      M_DEVBUF, M_WAITOK);
		pcdisplay_eraserows(&scr->pcs, 0, type->nrows, *defattrp);
	}

	*cookiep = scr;
	*curxp = scr->pcs.cursorcol;
	*curyp = scr->pcs.cursorrow;

	return (0);
}

void
vga_free_screen(void *v, void *cookie)
{
	struct vgascreen *vs = cookie;
	struct vga_config *vc = vs->cfg;

	LIST_REMOVE(vs, next);
	if (vs->fontset1)
		egavga_unreffont(vc, vs->fontset1);
	if (vs->fontset2)
		egavga_unreffont(vc, vs->fontset2);

	if (vs != &vga_console_screen)
		free(vs, M_DEVBUF);
	else
		panic("vga_free_screen: console");

	if (vc->active == vs)
		vc->active = 0;
}

static void vga_usefont(struct vga_config *, struct egavga_font *);

static void
vga_usefont(struct vga_config *vc, struct egavga_font *f)
{
	int slot;
	struct egavga_font *of;

	if (f->slot != -1)
		goto toend;

	for (slot = 0; slot < vc->vc_nfontslots; slot++) {
		if (!vc->vc_fonts[slot])
			goto loadit;
	}

	/* have to kick out another one */
	TAILQ_FOREACH(of, &vc->vc_fontlist, next) {
		if (of->slot != -1) {
			KASSERT(vc->vc_fonts[of->slot] == of);
			slot = of->slot;
			of->slot = -1;
			goto loadit;
		}
	}
	panic("vga_usefont");

loadit:
	vga_loadchars(&vc->hdl, slot, f->wsfont->firstchar,
		      f->wsfont->numchars, f->wsfont->fontheight,
		      f->wsfont->data);
	f->slot = slot;
	vc->vc_fonts[slot] = f;

toend:
	TAILQ_REMOVE(&vc->vc_fontlist, f, next);
	TAILQ_INSERT_TAIL(&vc->vc_fontlist, f, next);
}

static void
vga_setfont(struct vga_config *vc, struct vgascreen *scr)
{
	int fontslot1, fontslot2;

	if (scr->fontset1)
		vga_usefont(vc, scr->fontset1);
	if (scr->fontset2)
		vga_usefont(vc, scr->fontset2);

	fontslot1 = (scr->fontset1 ? scr->fontset1->slot : 0);
	fontslot2 = (scr->fontset2 ? scr->fontset2->slot : fontslot1);
	if (vc->currentfontset1 != fontslot1 ||
	    vc->currentfontset2 != fontslot2) {
		vga_setfontset(&vc->hdl, fontslot1, fontslot2);
		vc->currentfontset1 = fontslot1;
		vc->currentfontset2 = fontslot2;
	}
}

int
vga_show_screen(void *v, void *cookie, int waitok,
		void (*cb)(void *, int, int), void *cbarg)
{
	struct vgascreen *scr = cookie, *oldscr;
	struct vga_config *vc = scr->cfg;

	oldscr = vc->active; /* can be NULL! */
	if (scr == oldscr) {
		return (0);
	}

	vc->wantedscreen = cookie;
	vc->switchcb = cb;
	vc->switchcbarg = cbarg;
	if (cb) {
		callout_reset(&vc->vc_switch_callout, 0,
		    (void(*)(void *))vga_doswitch, vc);
		return (EAGAIN);
	}

	vga_doswitch(vc);
	return (0);
}

void
vga_doswitch(struct vga_config *vc)
{
	struct vgascreen *scr, *oldscr;
	struct vga_handle *vh = &vc->hdl;
	const struct wsscreen_descr *type;

	scr = vc->wantedscreen;
	if (!scr) {
		printf("vga_doswitch: disappeared\n");
		(*vc->switchcb)(vc->switchcbarg, EIO, 0);
		return;
	}
	type = scr->pcs.type;
	oldscr = vc->active; /* can be NULL! */
#ifdef DIAGNOSTIC
	if (oldscr) {
		if (!oldscr->pcs.active)
			panic("vga_show_screen: not active");
		if (oldscr->pcs.type != vc->currenttype)
			panic("vga_show_screen: bad type");
	}
#endif
	if (scr == oldscr) {
		return;
	}
#ifdef DIAGNOSTIC
	if (scr->pcs.active)
		panic("vga_show_screen: active");
#endif

	if (oldscr) {
		const struct wsscreen_descr *oldtype = oldscr->pcs.type;

		oldscr->pcs.active = 0;
		bus_space_read_region_2(vh->vh_memt, vh->vh_memh,
					oldscr->pcs.dispoffset, oldscr->pcs.mem,
					oldtype->ncols * oldtype->nrows);
	}

	if (vc->currenttype != type) {
		vga_setscreentype(vh, type);
		vc->currenttype = type;
	}

	vga_setfont(vc, scr);
	/* XXX swich colours! */

	scr->pcs.dispoffset = scr->mindispoffset;
	if (!oldscr || (scr->pcs.dispoffset != oldscr->pcs.dispoffset)) {
		vga_6845_write(vh, startadrh, scr->pcs.dispoffset >> 9);
		vga_6845_write(vh, startadrl, scr->pcs.dispoffset >> 1);
	}

	bus_space_write_region_2(vh->vh_memt, vh->vh_memh,
				scr->pcs.dispoffset, scr->pcs.mem,
				type->ncols * type->nrows);
	scr->pcs.active = 1;

	vc->active = scr;

	pcdisplay_cursor(&scr->pcs, scr->pcs.cursoron,
			 scr->pcs.cursorrow, scr->pcs.cursorcol);

	vc->wantedscreen = 0;
	if (vc->switchcb)
		(*vc->switchcb)(vc->switchcbarg, 0, 0);
}

static int
vga_load_font(void *v, void *cookie, struct wsdisplay_font *data)
{
	struct vga_config *vc = v;
	struct vgascreen *scr = cookie;
	char *name2;
	int res;

	if (scr) {
		name2 = NULL;
		if (data->name) {
			name2 = strchr(data->name, ',');
			if (name2)
				*name2++ = '\0';
		}
		res = vga_selectfont(vc, scr, data->name, name2);
		if (!res && scr->pcs.active)
			vga_setfont(vc, scr);
		return (res);
	}

	return (0);
}

static int
vga_allocattr(void *id, int fg, int bg, int flags, long *attrp)
{
	struct vgascreen *scr = id;
	struct vga_config *vc = scr->cfg;

	if (vc->hdl.vh_mono) {
		if (flags & WSATTR_WSCOLORS)
			return (EINVAL);
		if (flags & WSATTR_REVERSE)
			*attrp = 0x70;
		else
			*attrp = 0x07;
		if (flags & WSATTR_UNDERLINE)
			*attrp |= FG_UNDERLINE;
		if (flags & WSATTR_HILIT)
			*attrp |= FG_INTENSE;
	} else {
		if (flags & (WSATTR_UNDERLINE | WSATTR_REVERSE))
			return (EINVAL);
		if (flags & WSATTR_WSCOLORS)
			*attrp = fgansitopc[fg] | bgansitopc[bg];
		else
			*attrp = 7;
		if (flags & WSATTR_HILIT)
			*attrp += 8;
	}
	if (flags & WSATTR_BLINK)
		*attrp |= FG_BLINK;
	return (0);
}

static void
vga_copyrows(void *id, int srcrow, int dstrow, int nrows)
{
	struct vgascreen *scr = id;
	bus_space_tag_t memt = scr->pcs.hdl->ph_memt;
	bus_space_handle_t memh = scr->pcs.hdl->ph_memh;
	int ncols = scr->pcs.type->ncols;
	bus_size_t srcoff, dstoff;

	srcoff = srcrow * ncols + 0;
	dstoff = dstrow * ncols + 0;

	if (scr->pcs.active) {
		if (dstrow == 0 && (srcrow + nrows == scr->pcs.type->nrows)) {
#ifdef PCDISPLAY_SOFTCURSOR
			int cursoron = scr->pcs.cursoron;

			if (cursoron)
				pcdisplay_cursor(&scr->pcs, 0,
				    scr->pcs.cursorrow, scr->pcs.cursorcol);
#endif
			/* scroll up whole screen */
			if ((scr->pcs.dispoffset + srcrow * ncols * 2)
			    <= scr->maxdispoffset) {
				scr->pcs.dispoffset += srcrow * ncols * 2;
			} else {
				bus_space_copy_region_2(memt, memh,
					scr->pcs.dispoffset + srcoff * 2,
					memh, scr->mindispoffset,
					nrows * ncols);
				scr->pcs.dispoffset = scr->mindispoffset;
			}
			vga_6845_write(&scr->cfg->hdl, startadrh,
				       scr->pcs.dispoffset >> 9);
			vga_6845_write(&scr->cfg->hdl, startadrl,
				       scr->pcs.dispoffset >> 1);
#ifdef PCDISPLAY_SOFTCURSOR
			if (cursoron)
				pcdisplay_cursor(&scr->pcs, 1,
				    scr->pcs.cursorrow, scr->pcs.cursorcol);
#endif
		} else {
			bus_space_copy_region_2(memt, memh,
					scr->pcs.dispoffset + srcoff * 2,
					memh, scr->pcs.dispoffset + dstoff * 2,
					nrows * ncols);
		}
	} else
		memcpy(&scr->pcs.mem[dstoff], &scr->pcs.mem[srcoff],
		      nrows * ncols * 2);
}

#ifdef WSCONS_SUPPORT_PCVTFONTS

#define NOTYET 0xffff
static const u_int16_t pcvt_unichars[0xa0] = {
/* 0 */	_e006U, /* N/L control */
	NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET,
	NOTYET,
	0x2409, /* SYMBOL FOR HORIZONTAL TABULATION */
	0x240a, /* SYMBOL FOR LINE FEED */
	0x240b, /* SYMBOL FOR VERTICAL TABULATION */
	0x240c, /* SYMBOL FOR FORM FEED */
	0x240d, /* SYMBOL FOR CARRIAGE RETURN */
	NOTYET, NOTYET,
/* 1 */	NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET,
	NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET,
/* 2 */	NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET,
	NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET,
/* 3 */	NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET,
	NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET, NOTYET,
/* 4 */	0x03c1, /* GREEK SMALL LETTER RHO */
	0x03c8, /* GREEK SMALL LETTER PSI */
	0x2202, /* PARTIAL DIFFERENTIAL */
	0x03bb, /* GREEK SMALL LETTER LAMDA */
	0x03b9, /* GREEK SMALL LETTER IOTA */
	0x03b7, /* GREEK SMALL LETTER ETA */
	0x03b5, /* GREEK SMALL LETTER EPSILON */
	0x03c7, /* GREEK SMALL LETTER CHI */
	0x2228, /* LOGICAL OR */
	0x2227, /* LOGICAL AND */
	0x222a, /* UNION */
	0x2283, /* SUPERSET OF */
	0x2282, /* SUBSET OF */
	0x03a5, /* GREEK CAPITAL LETTER UPSILON */
	0x039e, /* GREEK CAPITAL LETTER XI */
	0x03a8, /* GREEK CAPITAL LETTER PSI */
/* 5 */	0x03a0, /* GREEK CAPITAL LETTER PI */
	0x21d2, /* RIGHTWARDS DOUBLE ARROW */
	0x21d4, /* LEFT RIGHT DOUBLE ARROW */
	0x039b, /* GREEK CAPITAL LETTER LAMDA */
	0x0398, /* GREEK CAPITAL LETTER THETA */
	0x2243, /* ASYMPTOTICALLY EQUAL TO */
	0x2207, /* NABLA */
	0x2206, /* INCREMENT */
	0x221d, /* PROPORTIONAL TO */
	0x2234, /* THEREFORE */
	0x222b, /* INTEGRAL */
	0x2215, /* DIVISION SLASH */
	0x2216, /* SET MINUS */
	_e00eU, /* angle? */
	_e00dU, /* inverted angle? */
	_e00bU, /* braceleftmid */
/* 6 */	_e00cU, /* bracerightmid */
	_e007U, /* bracelefttp */
	_e008U, /* braceleftbt */
	_e009U, /* bracerighttp */
	_e00aU, /* bracerightbt */
	0x221a, /* SQUARE ROOT */
	0x03c9, /* GREEK SMALL LETTER OMEGA */
	0x00a5, /* YEN SIGN */
	0x03be, /* GREEK SMALL LETTER XI */
	0x00fd, /* LATIN SMALL LETTER Y WITH ACUTE */
	0x00fe, /* LATIN SMALL LETTER THORN */
	0x00f0, /* LATIN SMALL LETTER ETH */
	0x00de, /* LATIN CAPITAL LETTER THORN */
	0x00dd, /* LATIN CAPITAL LETTER Y WITH ACUTE */
	0x00d7, /* MULTIPLICATION SIGN */
	0x00d0, /* LATIN CAPITAL LETTER ETH */
/* 7 */	0x00be, /* VULGAR FRACTION THREE QUARTERS */
	0x00b8, /* CEDILLA */
	0x00b4, /* ACUTE ACCENT */
	0x00af, /* MACRON */
	0x00ae, /* REGISTERED SIGN */
	0x00ad, /* SOFT HYPHEN */
	0x00ac, /* NOT SIGN */
	0x00a8, /* DIAERESIS */
	0x2260, /* NOT EQUAL TO */
	_e005U, /* scan 9 */
	_e004U, /* scan 7 */
	_e003U, /* scan 5 */
	_e002U, /* scan 3 */
	_e001U, /* scan 1 */
	0x03c5, /* GREEK SMALL LETTER UPSILON */
	0x00f8, /* LATIN SMALL LETTER O WITH STROKE */
/* 8 */	0x0153, /* LATIN SMALL LIGATURE OE */
	0x00f5, /* LATIN SMALL LETTER O WITH TILDE !!!doc bug */
	0x00e3, /* LATIN SMALL LETTER A WITH TILDE */
	0x0178, /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
	0x00db, /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
	0x00da, /* LATIN CAPITAL LETTER U WITH ACUTE */
	0x00d9, /* LATIN CAPITAL LETTER U WITH GRAVE */
	0x00d8, /* LATIN CAPITAL LETTER O WITH STROKE */
	0x0152, /* LATIN CAPITAL LIGATURE OE */
	0x00d5, /* LATIN CAPITAL LETTER O WITH TILDE */
	0x00d4, /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
	0x00d3, /* LATIN CAPITAL LETTER O WITH ACUTE */
	0x00d2, /* LATIN CAPITAL LETTER O WITH GRAVE */
	0x00cf, /* LATIN CAPITAL LETTER I WITH DIAERESIS */
	0x00ce, /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
	0x00cd, /* LATIN CAPITAL LETTER I WITH ACUTE */
/* 9 */	0x00cc, /* LATIN CAPITAL LETTER I WITH GRAVE */
	0x00cb, /* LATIN CAPITAL LETTER E WITH DIAERESIS */
	0x00ca, /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
	0x00c8, /* LATIN CAPITAL LETTER E WITH GRAVE */
	0x00c3, /* LATIN CAPITAL LETTER A WITH TILDE */
	0x00c2, /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
	0x00c1, /* LATIN CAPITAL LETTER A WITH ACUTE */
	0x00c0, /* LATIN CAPITAL LETTER A WITH GRAVE */
	0x00b9, /* SUPERSCRIPT ONE */
	0x00b7, /* MIDDLE DOT */
	0x03b6, /* GREEK SMALL LETTER ZETA */
	0x00b3, /* SUPERSCRIPT THREE */
	0x00a9, /* COPYRIGHT SIGN */
	0x00a4, /* CURRENCY SIGN */
	0x03ba, /* GREEK SMALL LETTER KAPPA */
	_e000U  /* mirrored question mark? */
};

static int vga_pcvt_mapchar(int, u_int *);

static int
vga_pcvt_mapchar(int uni, u_int *index)
{
	int i;

	for (i = 0; i < 0xa0; i++) /* 0xa0..0xff are reserved */
		if (uni == pcvt_unichars[i]) {
			*index = i;
			return (5);
		}
	*index = 0x99; /* middle dot */
	return (0);
}

#endif /* WSCONS_SUPPORT_PCVTFONTS */

#ifdef WSCONS_SUPPORT_ISO7FONTS

static int
vga_iso7_mapchar(int uni, u_int *index)
{

	/*
	 * U+0384 (GREEK TONOS) to
	 * U+03ce (GREEK SMALL LETTER OMEGA WITH TONOS)
	 * map directly to the iso-9 font
	 */
	if (uni >= 0x0384 && uni <= 0x03ce) {
		/* U+0384 is at offset 0xb4 in the font */
		*index = uni - 0x0384 + 0xb4;
		return (5);
	}

	/* XXX more chars in the iso-9 font */

	*index = 0xa4; /* shaded rectangle */
	return (0);
}

#endif /* WSCONS_SUPPORT_ISO7FONTS */

static int _vga_mapchar(void *, const struct egavga_font *, int, u_int *);

static int
_vga_mapchar(void *id, const struct egavga_font *font, int uni, u_int *index)
{

	switch (font->wsfont->encoding) {
	case WSDISPLAY_FONTENC_ISO:
		if (uni < 256) {
			*index = uni;
			return (5);
		} else {
			*index = ' ';
			return (0);
		}
		break;
	case WSDISPLAY_FONTENC_IBM:
		return (pcdisplay_mapchar(id, uni, index));
#ifdef WSCONS_SUPPORT_PCVTFONTS
	case WSDISPLAY_FONTENC_PCVT:
		return (vga_pcvt_mapchar(uni, index));
#endif
#ifdef WSCONS_SUPPORT_ISO7FONTS
	case WSDISPLAY_FONTENC_ISO7:
		return (vga_iso7_mapchar(uni, index));
#endif
	default:
#ifdef VGAFONTDEBUG
		printf("_vga_mapchar: encoding=%d\n", font->wsfont->encoding);
#endif
		*index = ' ';
		return (0);
	}
}

static int
vga_mapchar(void *id, int uni, u_int *index)
{
	struct vgascreen *scr = id;
	u_int idx1, idx2;
	int res1, res2;

	res1 = 0;
	idx1 = ' '; /* space */
	if (scr->fontset1)
		res1 = _vga_mapchar(id, scr->fontset1, uni, &idx1);
	res2 = -1;
	if (scr->fontset2) {
		KASSERT(VGA_SCREEN_CANTWOFONTS(scr->pcs.type));
		res2 = _vga_mapchar(id, scr->fontset2, uni, &idx2);
	}
	if (res2 > res1) {
		*index = idx2 | 0x0800; /* attribute bit 3 */
		return (res2);
	}
	*index = idx1;
	return (res1);
}

#ifdef WSDISPLAY_CHARFUNCS
int
vga_getwschar(void *cookie, struct wsdisplay_char *wschar)
{
	struct vgascreen *scr = cookie;

	if (scr == NULL) return 0;
	return (pcdisplay_getwschar(&scr->pcs, wschar));
}

int
vga_putwschar(void *cookie, struct wsdisplay_char *wschar)
{
	struct vgascreen *scr = cookie;

	if (scr == NULL) return 0;
	return (pcdisplay_putwschar(&scr->pcs, wschar));
}
#endif /* WSDISPLAY_CHARFUNCS */
