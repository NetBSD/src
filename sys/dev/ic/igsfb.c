/*	$NetBSD: igsfb.c,v 1.3 2002/07/04 14:37:11 junyoung Exp $ */

/*
 * Copyright (c) 2002 Valeriy E. Ushakov
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

/*
 * Integraphics Systems IGA 1682 and (untested) CyberPro 2k.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: igsfb.c,v 1.3 2002/07/04 14:37:11 junyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#include <dev/wsfont/wsfont.h>
#include <dev/wscons/wsconsio.h>

#include <dev/ic/igsfbreg.h>
#include <dev/ic/igsfbvar.h>


/*
 * wsscreen
 */

/* filled from rasops_info in igsfb_common_init */
static struct wsscreen_descr igsfb_stdscreen = {
	"std",			/* name */
	0, 0,			/* ncols, nrows */
	NULL,			/* textops */
	0, 0,			/* fontwidth, fontheight */
	0			/* capabilities */
};

static const struct wsscreen_descr *_igsfb_scrlist[] = {
	&igsfb_stdscreen,
};

static const struct wsscreen_list igsfb_screenlist = {
	sizeof(_igsfb_scrlist) / sizeof(struct wsscreen_descr *),
	_igsfb_scrlist
};


/*
 * wsdisplay_accessops
 */

static int	igsfb_ioctl(void *, u_long, caddr_t, int, struct proc *);
static paddr_t	igsfb_mmap(void *, off_t, int);

static int	igsfb_alloc_screen(void *, const struct wsscreen_descr *,
				   void **, int *, int *, long *);
static void	igsfb_free_screen(void *, void *);
static int	igsfb_show_screen(void *, void *, int,
				  void (*) (void *, int, int), void *);

static const struct wsdisplay_accessops igsfb_accessops = {
	igsfb_ioctl,
	igsfb_mmap,
	igsfb_alloc_screen,
	igsfb_free_screen,
	igsfb_show_screen,
	NULL /* load_font */
};


/*
 * internal functions
 */
static void	igsfb_common_init(struct igsfb_softc *);
static void	igsfb_init_bit_tables(struct igsfb_softc *);
static void	igsfb_blank_screen(struct igsfb_softc *, int);
static int	igsfb_get_cmap(struct igsfb_softc *, struct wsdisplay_cmap *);
static int	igsfb_set_cmap(struct igsfb_softc *, struct wsdisplay_cmap *);
static void	igsfb_update_cmap(struct igsfb_softc *sc, u_int, u_int);
static void	igsfb_set_curpos(struct igsfb_softc *,
				 struct wsdisplay_curpos *);
static void	igsfb_update_curpos(struct igsfb_softc *);
static int	igsfb_get_cursor(struct igsfb_softc *,
				 struct wsdisplay_cursor *);
static int	igsfb_set_cursor(struct igsfb_softc *,
				 struct wsdisplay_cursor *);
static void	igsfb_update_cursor(struct igsfb_softc *, u_int);
static void	igsfb_convert_cursor_data(struct igsfb_softc *, u_int, u_int);

/*
 * bit expanders
 */
static u_int16_t igsfb_spread_bits_8(u_int8_t);

struct igs_bittab *igsfb_bittab = NULL;
struct igs_bittab *igsfb_bittab_bswap = NULL;

static __inline__ u_int16_t
igsfb_spread_bits_8(b)
	u_int8_t b;
{
	u_int16_t s = b;

	s = ((s & 0x00f0) << 4) | (s & 0x000f);
	s = ((s & 0x0c0c) << 2) | (s & 0x0303);
	s = ((s & 0x2222) << 1) | (s & 0x1111);
	return (s);
}


/*
 * Enable Video.  This might go through either memory or i/o space and
 * requires access to registers that we don't need for normal
 * operation.  So for greater flexibility this function takes bus tag
 * and base address, not the igsfb_softc.
 */
int
igsfb_io_enable(bt, base)
	bus_space_tag_t bt;
	bus_addr_t base;
{
	bus_space_handle_t vdoh;
	bus_space_handle_t vseh;
	int ret;

	ret = bus_space_map(bt, base + IGS_VDO, 1, 0, &vdoh);
	if (ret != 0) {
		printf("unable to map VDO register\n");
		return (ret);
	}

	ret = bus_space_map(bt, base + IGS_VSE, 1, 0, &vseh);
	if (ret != 0) {
		bus_space_unmap(bt, vdoh, 1);
		printf("unable to map VSE register\n");
		return (ret);
	}

	/* enable video: start decoding i/o space accesses */
	bus_space_write_1(bt, vdoh, 0, IGS_VDO_ENABLE | IGS_VDO_SETUP);
	bus_space_write_1(bt, vseh, 0, IGS_VSE_ENABLE);
	bus_space_write_1(bt, vdoh, 0, IGS_VDO_ENABLE);

	bus_space_unmap(bt, vdoh, 1);
	bus_space_unmap(bt, vseh, 1);

	return (0);
}


/*
 * Enable linear: start decoding memory space accesses.
 * while here, enable coprocessor and set its addrress to 0xbf000.
 */
void
igsfb_mem_enable(sc)
	struct igsfb_softc *sc;
{

	igs_ext_write(sc->sc_iot, sc->sc_ioh, IGS_EXT_BIU_MISC_CTL,
		      IGS_EXT_BIU_LINEAREN
		      | IGS_EXT_BIU_COPREN
		      | IGS_EXT_BIU_COPASELB);
}


/*
 * Finish off the attach.  Bus specific attach method should have
 * enabled io and memory accesses and mapped io and cop registers.
 */
void
igsfb_common_attach(sc, isconsole)
	struct igsfb_softc *sc;
	int isconsole;
{
	bus_addr_t craddr;
	off_t croffset;
	struct rasops_info *ri;
	struct wsemuldisplaydev_attach_args waa;
	u_int8_t busctl, curctl;

	busctl = igs_ext_read(sc->sc_iot, sc->sc_ioh, IGS_EXT_BUS_CTL);
	if (busctl & 0x2)
		sc->sc_memsz = 4 * 1024 * 1024;
	else if (busctl & 0x1)
		sc->sc_memsz = 2 * 1024 * 1024;
	else
		sc->sc_memsz = 1 * 1024 * 1024;

	/*
	 * Don't map in all N megs, just the amount we need for wsscreen
	 */
	sc->sc_fbsz = 1024 * 768; /* XXX: 8bpp specific */
	if (bus_space_map(sc->sc_memt, sc->sc_memaddr, sc->sc_fbsz,
			  sc->sc_memflags | BUS_SPACE_MAP_LINEAR,
			  &sc->sc_fbh) != 0)
	{
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, IGS_IO_SIZE);
		printf("unable to map framebuffer\n");
		return;
	}

	/*
	 * 1Kb for cursor sprite data at the very end of linear space
	 */
	croffset = sc->sc_memsz - IGS_CURSOR_DATA_SIZE;
	craddr = sc->sc_memaddr + croffset;
	if (bus_space_map(sc->sc_memt, craddr, IGS_CURSOR_DATA_SIZE,
			  sc->sc_memflags | BUS_SPACE_MAP_LINEAR,
			  &sc->sc_crh) != 0)
	{
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, IGS_IO_SIZE);
		bus_space_unmap(sc->sc_memt, sc->sc_fbh, sc->sc_fbsz);
		printf("unable to map cursor sprite region\n");
		return;
	}

	/*
	 * Tell device where cursor sprite data are located in linear
	 * space (it takes data offset in 1k units).
	 */
	croffset >>= 10;
	igs_ext_write(sc->sc_iot, sc->sc_ioh,
		      IGS_EXT_SPRITE_DATA_LO, croffset & 0xff);
	igs_ext_write(sc->sc_iot, sc->sc_ioh,
		      IGS_EXT_SPRITE_DATA_HI, (croffset >> 8) & 0xf);

	memset(&sc->sc_cursor, 0, sizeof(struct igs_hwcursor));
	memset(bus_space_vaddr(sc->sc_memt, sc->sc_crh),
	       0xaa, IGS_CURSOR_DATA_SIZE); /* transparent */

	curctl = igs_ext_read(sc->sc_iot, sc->sc_ioh, IGS_EXT_SPRITE_CTL);
	curctl |= IGS_EXT_SPRITE_64x64;
	curctl &= ~IGS_EXT_SPRITE_VISIBLE;
	igs_ext_write(sc->sc_iot, sc->sc_ioh, IGS_EXT_SPRITE_CTL, curctl);

	/* bit expanders for cursor sprite data */
	igsfb_init_bit_tables(sc);

	/* alloc and cross-link raster ops */
	ri = malloc(sizeof(struct rasops_info), M_DEVBUF, M_NOWAIT | M_ZERO);
	if (ri == NULL)
		panic("unable to allocate rasops");
	ri->ri_hw = sc;
	sc->sc_ri = ri;

	igsfb_common_init(sc);

	/*
	 * XXX: console attachment needs rethinking
	 */
	if (isconsole) {
		long defattr;
		(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
		wsdisplay_cnattach(&igsfb_stdscreen, ri, 0, 0, defattr);
	}


	printf("%s: %dx%d, %dbpp\n",
	       sc->sc_dev.dv_xname, ri->ri_width, ri->ri_height, ri->ri_depth);

	/* attach wsdisplay */
	waa.console = isconsole;
	waa.scrdata = &igsfb_screenlist;
	waa.accessops = &igsfb_accessops;
	waa.accesscookie = sc;

	config_found(&sc->sc_dev, &waa, wsemuldisplaydevprint);
}


/*
 * Cursor sprite data are in 2bpp.  Incoming image/mask are in 1bpp.
 * Prebuild tables to expand 1bpp->2bpp with bswapping if neccessary.
 */
static void
igsfb_init_bit_tables(sc)
	struct igsfb_softc *sc;
{
	struct igs_bittab *tab;
	u_int i;

	if (sc->sc_hwflags & IGSFB_HW_BSWAP) {
		if (igsfb_bittab_bswap == NULL) {
			tab = malloc(sizeof(struct igs_bittab),
				     M_DEVBUF, M_NOWAIT);
			for (i = 0; i < 256; ++i) {
				u_int16_t s = igsfb_spread_bits_8(i);
				tab->iexpand[i] = bswap16(s);
				tab->mexpand[i] = bswap16((s << 1) | s);
			}
			igsfb_bittab_bswap = tab;
		}
		sc->sc_bittab = igsfb_bittab_bswap;
	} else {
		if (igsfb_bittab == NULL) {
			tab = malloc(sizeof(struct igs_bittab),
				     M_DEVBUF, M_NOWAIT);
			for (i = 0; i < 256; ++i) {
				u_int16_t s = igsfb_spread_bits_8(i);
				tab->iexpand[i] = s;
				tab->mexpand[i] = (s << 1) | s;
			}
			igsfb_bittab = tab;
		}
		sc->sc_bittab = igsfb_bittab;
	}
}

/*
 * I/O and memory are mapped, video enabled, structures allocated.
 */
static void
igsfb_common_init(sc)
	struct igsfb_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct rasops_info *ri = sc->sc_ri;
	int wsfcookie;
	const u_int8_t *p;
	int i;

	sc->sc_blanked = 0;

	ri->ri_flg = RI_CENTER | RI_CLEAR;
	if (sc->sc_hwflags & IGSFB_HW_BSWAP)
	    ri->ri_flg |= RI_BSWAP;

	ri->ri_depth = 8;
	ri->ri_width = 1024;
	ri->ri_height = 768;
	ri->ri_stride = 1024;
	ri->ri_bits = (u_char *)sc->sc_fbh;

	/*
	 * Initialize wsfont related stuff.
	 */
	wsfont_init();

	/* prefer gallant that is identical to the one the prom uses */
	wsfcookie = wsfont_find("Gallant", 12, 22, 0,
				WSDISPLAY_FONTORDER_L2R,
				WSDISPLAY_FONTORDER_L2R);
	if (wsfcookie <= 0) {
#ifdef DIAGNOSTIC
		printf("%s: unable to find font Gallant 12x22\n",
		       sc->sc_dev.dv_xname);
#endif
		/* any font at all? */
		wsfcookie = wsfont_find(NULL, 0, 0, 0,
					WSDISPLAY_FONTORDER_L2R,
					WSDISPLAY_FONTORDER_L2R);
	}

	if (wsfcookie <= 0) {
		printf("%s: unable to find any fonts\n", sc->sc_dev.dv_xname);
		return;
	}

	if (wsfont_lock(wsfcookie, &ri->ri_font) != 0) {
		printf("%s: unable to lock font\n", sc->sc_dev.dv_xname);
		return;
	}
	ri->ri_wsfcookie = wsfcookie;


	/*
	 * Initialize colormap related stuff.
	 */

	/* ANSI color map */
	p = rasops_cmap;
	for (i = 0; i < IGS_CMAP_SIZE; ++i, p += 3) { /* software copy */
		sc->sc_cmap.r[i] = p[0];
		sc->sc_cmap.g[i] = p[1];
		sc->sc_cmap.b[i] = p[2];
	}
	igsfb_update_cmap(sc, 0, IGS_CMAP_SIZE);

	/* set overscan color r/g/b (XXX: use defattr's rgb?) */
	igs_ext_write(iot, ioh, IGS_EXT_OVERSCAN_RED,   0);
	igs_ext_write(iot, ioh, IGS_EXT_OVERSCAN_GREEN, 0);
	igs_ext_write(iot, ioh, IGS_EXT_OVERSCAN_BLUE,  0);


	/* TODO: compute term size based on font dimensions? */
	rasops_init(ri, 34, 80);

	igsfb_stdscreen.nrows = ri->ri_rows;
	igsfb_stdscreen.ncols = ri->ri_cols;
	igsfb_stdscreen.textops = &ri->ri_ops;
	igsfb_stdscreen.capabilities = ri->ri_caps;
}


/*
 * wsdisplay_accessops: mmap()
 */
static paddr_t
igsfb_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	struct igsfb_softc *sc = v;

	if (offset >= sc->sc_memsz || offset < 0)
		return (-1);

	return (bus_space_mmap(sc->sc_memt, sc->sc_memaddr, offset, prot,
			       sc->sc_memflags | BUS_SPACE_MAP_LINEAR));
}


/*
 * wsdisplay_accessops: ioctl()
 */
static int
igsfb_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct igsfb_softc *sc = v;
	struct rasops_info *ri = sc->sc_ri;
	int turnoff;

	switch (cmd) {

	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
		return (0);

	case WSDISPLAYIO_GINFO:
#define	wsd_fbip ((struct wsdisplay_fbinfo *)data)
		wsd_fbip->height = ri->ri_height;
		wsd_fbip->width = ri->ri_width;
		wsd_fbip->depth = ri->ri_depth;
		wsd_fbip->cmsize = IGS_CMAP_SIZE;
#undef wsd_fbip
		return (0);

	case WSDISPLAYIO_GVIDEO:
		*(u_int *)data = sc->sc_blanked ?
		    WSDISPLAYIO_VIDEO_OFF : WSDISPLAYIO_VIDEO_ON;
		return (0);

	case WSDISPLAYIO_SVIDEO:
		turnoff = (*(u_int *)data == WSDISPLAYIO_VIDEO_OFF);
		if (sc->sc_blanked != turnoff) {
			sc->sc_blanked = turnoff;
			igsfb_blank_screen(sc, sc->sc_blanked);
		}
		return (0);

	case WSDISPLAYIO_GETCMAP:
		return (igsfb_get_cmap(sc, (struct wsdisplay_cmap *)data));

	case WSDISPLAYIO_PUTCMAP:
		return (igsfb_set_cmap(sc, (struct wsdisplay_cmap *)data));

	case WSDISPLAYIO_GCURMAX:
		((struct wsdisplay_curpos *)data)->x = IGS_CURSOR_MAX_SIZE;
		((struct wsdisplay_curpos *)data)->y = IGS_CURSOR_MAX_SIZE;
		return (0);

	case WSDISPLAYIO_GCURPOS:
		*(struct wsdisplay_curpos *)data = sc->sc_cursor.cc_pos;
		return (0);

	case WSDISPLAYIO_SCURPOS:
		igsfb_set_curpos(sc, (struct wsdisplay_curpos *)data);
		return (0);

	case WSDISPLAYIO_GCURSOR:
		return (igsfb_get_cursor(sc, (struct wsdisplay_cursor *)data));

	case WSDISPLAYIO_SCURSOR:
		return (igsfb_set_cursor(sc, (struct wsdisplay_cursor *)data));
	}

	return (EPASSTHROUGH);
}


/*
 * wsdisplay_accessops: ioctl(WSDISPLAYIO_SVIDEO)
 */
static void
igsfb_blank_screen(sc, blank)
	struct igsfb_softc *sc;
	int blank;
{

	igs_ext_write(sc->sc_iot, sc->sc_ioh,
		      IGS_EXT_SYNC_CTL,
		      blank ? IGS_EXT_SYNC_H0 | IGS_EXT_SYNC_V0
			    : 0);
}


/*
 * wsdisplay_accessops: ioctl(WSDISPLAYIO_GETCMAP)
 *   Served from software cmap copy.
 */
static int
igsfb_get_cmap(sc, p)
	struct igsfb_softc *sc;
	struct wsdisplay_cmap *p;
{
	u_int index = p->index, count = p->count;

	if (index >= IGS_CMAP_SIZE || (index + count) > IGS_CMAP_SIZE)
		return (EINVAL);

	if (!uvm_useracc(p->red, count, B_WRITE) ||
	    !uvm_useracc(p->green, count, B_WRITE) ||
	    !uvm_useracc(p->blue, count, B_WRITE))
		return (EFAULT);

	copyout(&sc->sc_cmap.r[index], p->red, count);
	copyout(&sc->sc_cmap.g[index], p->green, count);
	copyout(&sc->sc_cmap.b[index], p->blue, count);

	return (0);
}


/*
 * wsdisplay_accessops: ioctl(WSDISPLAYIO_SETCMAP)
 *   Set software cmap copy and propagate changed range to device.
 */
static int
igsfb_set_cmap(sc, p)
	struct igsfb_softc *sc;
	struct wsdisplay_cmap *p;
{
	u_int index = p->index, count = p->count;

	if (index >= IGS_CMAP_SIZE || (index + count) > IGS_CMAP_SIZE)
		return (EINVAL);

	if (!uvm_useracc(p->red, count, B_READ) ||
	    !uvm_useracc(p->green, count, B_READ) ||
	    !uvm_useracc(p->blue, count, B_READ))
		return (EFAULT);

	copyin(p->red, &sc->sc_cmap.r[index], count);
	copyin(p->green, &sc->sc_cmap.g[index], count);
	copyin(p->blue, &sc->sc_cmap.b[index], count);

	igsfb_update_cmap(sc, p->index, p->count);

	return (0);
}


/*
 * Propagate specified part of the software cmap copy to device.
 */
static void
igsfb_update_cmap(sc, index, count)
	struct igsfb_softc *sc;
	u_int index, count;
{
	bus_space_tag_t t;
	bus_space_handle_t h;
	u_int last, i;

	if (index >= IGS_CMAP_SIZE)
		return;

	last = index + count;
	if (last > IGS_CMAP_SIZE)
		last = IGS_CMAP_SIZE;

	t = sc->sc_iot;
	h = sc->sc_ioh;

	/* start palette writing, index is autoincremented by hardware */
	bus_space_write_1(t, h, IGS_DAC_PEL_WRITE_IDX, index);

	for (i = index; i < last; ++i) {
		bus_space_write_1(t, h, IGS_DAC_PEL_DATA, sc->sc_cmap.r[i]);
		bus_space_write_1(t, h, IGS_DAC_PEL_DATA, sc->sc_cmap.g[i]);
		bus_space_write_1(t, h, IGS_DAC_PEL_DATA, sc->sc_cmap.b[i]);
	}
}


/*
 * wsdisplay_accessops: ioctl(WSDISPLAYIO_SCURPOS)
 */
static void
igsfb_set_curpos(sc, curpos)
	struct igsfb_softc *sc;
	struct wsdisplay_curpos *curpos;
{
	struct rasops_info *ri = sc->sc_ri;
	int x = curpos->x, y = curpos->y;

	if (y < 0)
		y = 0;
	else if (y > ri->ri_height)
		y = ri->ri_height;
	if (x < 0)
		x = 0;
	else if (x > ri->ri_width)
		x = ri->ri_width;
	sc->sc_cursor.cc_pos.x = x;
	sc->sc_cursor.cc_pos.y = y;

	igsfb_update_curpos(sc);
}


static void
igsfb_update_curpos(sc)
	struct igsfb_softc *sc;
{
	bus_space_tag_t t;
	bus_space_handle_t h;
	int x, xoff, y, yoff;

	xoff = 0;
	x = sc->sc_cursor.cc_pos.x - sc->sc_cursor.cc_hot.x;
	if (x < 0) {
		x = 0;
		xoff = -x;
	}

	yoff = 0;
	y = sc->sc_cursor.cc_pos.y - sc->sc_cursor.cc_hot.y;
	if (y < 0) {
		y = 0;
		yoff = -y;
	}

	t = sc->sc_iot;
	h = sc->sc_ioh;

	igs_ext_write(t, h, IGS_EXT_SPRITE_HSTART_LO, x & 0xff);
	igs_ext_write(t, h, IGS_EXT_SPRITE_HSTART_HI, (x >> 8) & 0x07);
	igs_ext_write(t, h, IGS_EXT_SPRITE_HPRESET, xoff & 0x3f);

	igs_ext_write(t, h, IGS_EXT_SPRITE_VSTART_LO, y & 0xff);
	igs_ext_write(t, h, IGS_EXT_SPRITE_VSTART_HI, (y >> 8) & 0x07);
	igs_ext_write(t, h, IGS_EXT_SPRITE_VPRESET, yoff & 0x3f);
}


/*
 * wsdisplay_accessops: ioctl(WSDISPLAYIO_GCURSOR)
 */
static int
igsfb_get_cursor(sc, p)
	struct igsfb_softc *sc;
	struct wsdisplay_cursor *p;
{

	/* XXX: TODO */
	return (0);
}


/*
 * wsdisplay_accessops: ioctl(WSDISPLAYIO_SCURSOR)
 */
static int
igsfb_set_cursor(sc, p)
	struct igsfb_softc *sc;
	struct wsdisplay_cursor *p;
{
	struct igs_hwcursor *cc;
	u_int v, index, count, icount, iwidth;

	cc = &sc->sc_cursor;
	v = p->which;

	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		index = p->cmap.index;
		count = p->cmap.count;
		if (index >= 2 || (index + count) > 2)
			return (EINVAL);
		if (!uvm_useracc(p->cmap.red, count, B_READ)
		    || !uvm_useracc(p->cmap.green, count, B_READ)
		    || !uvm_useracc(p->cmap.blue, count, B_READ))
			return (EFAULT);
	}

	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		if (p->size.x > IGS_CURSOR_MAX_SIZE
		    || p->size.y > IGS_CURSOR_MAX_SIZE)
			return (EINVAL);

		iwidth = (p->size.x + 7) >> 3; /* bytes per scan line */
		icount = iwidth * p->size.y;
		if (!uvm_useracc(p->image, icount, B_READ)
		    || !uvm_useracc(p->mask, icount, B_READ))
			return (EFAULT);
	}

	/* XXX: verify that hot is within size, pos within screen? */

	/* arguments verified, do the processing */

	if (v & WSDISPLAY_CURSOR_DOCUR)
		sc->sc_curenb = p->enable;

	if (v & WSDISPLAY_CURSOR_DOPOS)
		cc->cc_pos = p->pos;

	if (v & WSDISPLAY_CURSOR_DOHOT)
		cc->cc_hot = p->hot;

	if (v & WSDISPLAY_CURSOR_DOCMAP) {
		copyin(p->cmap.red, &cc->cc_color[index], count);
		copyin(p->cmap.green, &cc->cc_color[index + 2], count);
		copyin(p->cmap.blue, &cc->cc_color[index + 4], count);
	}

	if (v & WSDISPLAY_CURSOR_DOSHAPE) {
		u_int trailing_bits;

		copyin(p->image, cc->cc_image, icount);
		copyin(p->mask, cc->cc_mask, icount);
		cc->cc_size = p->size;

		/* clear trailing bits in the "partial" mask bytes */
		trailing_bits = p->size.x & 0x07;
		if (trailing_bits != 0) {
			const u_int cutmask = ~((~0) << trailing_bits);
			u_char *mp;
			u_int i;

			mp = cc->cc_mask + iwidth - 1;
			for (i = 0; i < p->size.y; ++i) {
				*mp &= cutmask;
				mp += iwidth;
			}
		}
		igsfb_convert_cursor_data(sc, iwidth, p->size.y);
	}

	igsfb_update_cursor(sc, v);
	return (0);
}

/*
 * Convert incoming 1bpp cursor image/mask into native 2bpp format.
 */
static void
igsfb_convert_cursor_data(sc, w, h)
	struct igsfb_softc *sc;
	u_int w, h;
{
	struct igs_hwcursor *cc = &sc->sc_cursor;
	struct igs_bittab *btab = sc->sc_bittab;
	u_int8_t *ip, *mp;
	u_int16_t *dp;
	u_int line, i;

	/* init sprite to be all transparent */
	memset(cc->cc_sprite, 0xaa, IGS_CURSOR_DATA_SIZE);

	/* first scanline */
	ip = cc->cc_image;
	mp = cc->cc_mask;
	dp = cc->cc_sprite;

	for (line = 0; line < h; ++line) {
		for (i = 0; i < w; ++i) {
			u_int16_t is = btab->iexpand[ip[i]];
			u_int16_t ms = btab->mexpand[mp[i]];

			/* NB: tables are pre-bswapped if needed */
			dp[i] = (0xaaaa & ~ms) | (is & ms);
		}

		/* next scanline */
		ip += w;
		mp += w;
		dp += 8;	/* 2bpp, 8 pixels per short = 8 shorts */
	}
}


/*
 * Propagate cursor changes to device.
 * "which" is composed from WSDISPLAY_CURSOR_DO* bits.
 */
static void
igsfb_update_cursor(sc, which)
	struct igsfb_softc *sc;
	u_int which;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int8_t curctl;

	/*
	 * We will need to tweak sprite control register for cursor
	 * visibility and cursor color map manipualtion.
	 */
	if (which & (WSDISPLAY_CURSOR_DOCUR | WSDISPLAY_CURSOR_DOCMAP)) {
		curctl = igs_ext_read(iot, ioh, IGS_EXT_SPRITE_CTL);
	}

	if (which & WSDISPLAY_CURSOR_DOSHAPE) {
		u_int8_t *dst = bus_space_vaddr(sc->sc_memt, sc->sc_crh);

		/*
		 * memcpy between spaces of different endianness would
		 * be ... special.  We cannot be sure if memset gonna
		 * push data in 4-byte chunks, we can't pre-bswap it,
		 * so do it byte-by-byte to preserve byte ordering.
		 */
		if (sc->sc_hwflags & IGSFB_HW_BSWAP) {
			u_int8_t *src = (u_int8_t *)sc->sc_cursor.cc_sprite;
			int i;

			for (i = 0; i < 1024; ++i)
				*dst++ = *src++;
		} else {
			memcpy(dst, sc->sc_cursor.cc_sprite, 1024);
		}
	}

	if (which & (WSDISPLAY_CURSOR_DOPOS | WSDISPLAY_CURSOR_DOHOT)) {
		/* code shared with WSDISPLAYIO_SCURPOS */
		igsfb_update_curpos(sc);
	}

	if (which & WSDISPLAY_CURSOR_DOCMAP) {
		u_int8_t *p;

		/* tell DAC we want access to the cursor palette */
		igs_ext_write(iot, ioh, IGS_EXT_SPRITE_CTL,
			      curctl | IGS_EXT_SPRITE_SELECT);

		p = sc->sc_cursor.cc_color;

		bus_space_write_1(iot, ioh, IGS_DAC_PEL_WRITE_IDX, 0);
		bus_space_write_1(iot, ioh, IGS_DAC_PEL_DATA, p[0]);
		bus_space_write_1(iot, ioh, IGS_DAC_PEL_DATA, p[2]);
		bus_space_write_1(iot, ioh, IGS_DAC_PEL_DATA, p[4]);

		bus_space_write_1(iot, ioh, IGS_DAC_PEL_WRITE_IDX, 1);
		bus_space_write_1(iot, ioh, IGS_DAC_PEL_DATA, p[1]);
		bus_space_write_1(iot, ioh, IGS_DAC_PEL_DATA, p[3]);
		bus_space_write_1(iot, ioh, IGS_DAC_PEL_DATA, p[5]);

		/* restore access to normal palette */
		igs_ext_write(iot, ioh, IGS_EXT_SPRITE_CTL, curctl);
	}

	if (which & WSDISPLAY_CURSOR_DOCUR) {
		if ((curctl & IGS_EXT_SPRITE_VISIBLE) == 0
		    && sc->sc_curenb)
			igs_ext_write(iot, ioh, IGS_EXT_SPRITE_CTL,
				      curctl | IGS_EXT_SPRITE_VISIBLE);
		else if ((curctl & IGS_EXT_SPRITE_VISIBLE) != 0
			 && !sc->sc_curenb)
			igs_ext_write(iot, ioh, IGS_EXT_SPRITE_CTL,
				      curctl & ~IGS_EXT_SPRITE_VISIBLE);
	}
}


/*
 * wsdisplay_accessops: alloc_screen()
 */
static int
igsfb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{

	/* TODO */
	return (ENOMEM);
}


/*
 * wsdisplay_accessops: free_screen()
 */
static void
igsfb_free_screen(v, cookie)
	void *v;
	void *cookie;
{

	/* TODO */
	return;
}


/*
 * wsdisplay_accessops: show_screen()
 */
static int
igsfb_show_screen(v, cookie, waitok, cb, cbarg)
	void *v;
	void *cookie;
	int waitok;
	void (*cb)(void *, int, int);
	void *cbarg;
{

	return (0);
}
