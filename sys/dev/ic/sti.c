/*	$NetBSD: sti.c,v 1.12 2010/11/01 06:41:50 skrll Exp $	*/

/*	$OpenBSD: sti.c,v 1.35 2003/12/16 06:07:13 mickey Exp $	*/

/*
 * Copyright (c) 2000-2003 Michael Shalayeff
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
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * TODO:
 *	call sti procs asynchronously;
 *	implement console scroll-back;
 *	X11 support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sti.c,v 1.12 2010/11/01 06:41:50 skrll Exp $");

#include "wsdisplay.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm.h>

#include <sys/bus.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>

#include <dev/ic/stireg.h>
#include <dev/ic/stivar.h>

void sti_cursor(void *, int, int, int);
int  sti_mapchar(void *, int, u_int *);
void sti_putchar(void *, int, int, u_int, long);
void sti_copycols(void *, int, int, int, int);
void sti_erasecols(void *, int, int, int, long);
void sti_copyrows(void *, int, int, int);
void sti_eraserows(void *, int, int, long);
int  sti_alloc_attr(void *, int, int, int, long *);

struct wsdisplay_emulops sti_emulops = {
	sti_cursor,
	sti_mapchar,
	sti_putchar,
	sti_copycols,
	sti_erasecols,
	sti_copyrows,
	sti_eraserows,
	sti_alloc_attr
};

int sti_ioctl(void *, void *, u_long, void *, int, struct lwp *);
paddr_t sti_mmap(void *, void *, off_t, int);
int sti_alloc_screen(void *, const struct wsscreen_descr *,
	void **, int *, int *, long *);
	void sti_free_screen(void *, void *);
int sti_show_screen(void *, void *, int, void (*cb)(void *, int, int), void *);
int sti_load_font(void *, void *, struct wsdisplay_font *);

const struct wsdisplay_accessops sti_accessops = {
	sti_ioctl,
	sti_mmap,
	sti_alloc_screen,
	sti_free_screen,
	sti_show_screen,
	sti_load_font
};

struct wsscreen_descr sti_default_screen = {
	"default", 0, 0,
	&sti_emulops,
	0, 0,
	WSSCREEN_REVERSE | WSSCREEN_UNDERLINE
};

const struct wsscreen_descr *sti_default_scrlist[] = {
	&sti_default_screen
};

struct wsscreen_list sti_default_screenlist = {
	sizeof(sti_default_scrlist) / sizeof(sti_default_scrlist[0]),
	sti_default_scrlist
};

enum sti_bmove_funcs {
	bmf_clear, bmf_copy, bmf_invert, bmf_underline
};

int sti_init(struct sti_softc *, int);
int sti_inqcfg(struct sti_softc *, struct sti_inqconfout *);
void sti_bmove(struct sti_softc *, int, int, int, int, int, int,
    enum sti_bmove_funcs);
int sti_setcment(struct sti_softc *, u_int, u_char, u_char, u_char);
int sti_fetchfonts(struct sti_softc *, struct sti_inqconfout *, uint32_t);
void sti_attach_deferred(device_t);

void
sti_attach_common(struct sti_softc *sc)
{
	struct sti_inqconfout cfg;
	struct sti_einqconfout ecfg;
	bus_space_handle_t fbh;
	struct sti_dd *dd;
	struct sti_cfg *cc;
	int error, size, i;
	uint8_t *p = (uint8_t *)sc->sc_code;
	uint32_t addr, eaddr;
	struct sti_region r;
	uint32_t *q;
	uint32_t tmp;

	dd = &sc->sc_dd;
	if (sc->sc_devtype == STI_DEVTYPE1) {
#define	parseshort(o) \
	((bus_space_read_1(sc->memt, sc->romh, (o) + 3) <<  8) | \
	 (bus_space_read_1(sc->memt, sc->romh, (o) + 7)))
#define	parseword(o) \
	((bus_space_read_1(sc->memt, sc->romh, (o) +  3) << 24) | \
	 (bus_space_read_1(sc->memt, sc->romh, (o) +  7) << 16) | \
	 (bus_space_read_1(sc->memt, sc->romh, (o) + 11) <<  8) | \
	 (bus_space_read_1(sc->memt, sc->romh, (o) + 15)))

		dd->dd_type  = bus_space_read_1(sc->memt, sc->romh, 0x03);
		dd->dd_nmon  = bus_space_read_1(sc->memt, sc->romh, 0x07);
		dd->dd_grrev = bus_space_read_1(sc->memt, sc->romh, 0x0b);
		dd->dd_lrrev = bus_space_read_1(sc->memt, sc->romh, 0x0f);
		dd->dd_grid[0] = parseword(0x10);
		dd->dd_grid[1] = parseword(0x20);
		dd->dd_fntaddr = parseword(0x30) & ~3;
		dd->dd_maxst   = parseword(0x40);
		dd->dd_romend  = parseword(0x50) & ~3;
		dd->dd_reglst  = parseword(0x60) & ~3;
		dd->dd_maxreent= parseshort(0x70);
		dd->dd_maxtimo = parseshort(0x78);
		dd->dd_montbl  = parseword(0x80) & ~3;
		dd->dd_udaddr  = parseword(0x90) & ~3;
		dd->dd_stimemreq=parseword(0xa0);
		dd->dd_udsize  = parseword(0xb0);
		dd->dd_pwruse  = parseshort(0xc0);
		dd->dd_bussup  = bus_space_read_1(sc->memt, sc->romh, 0xcb);
		dd->dd_ebussup = bus_space_read_1(sc->memt, sc->romh, 0xcf);
		dd->dd_altcodet= bus_space_read_1(sc->memt, sc->romh, 0xd3);
		dd->dd_eddst[0]= bus_space_read_1(sc->memt, sc->romh, 0xd7);
		dd->dd_eddst[1]= bus_space_read_1(sc->memt, sc->romh, 0xdb);
		dd->dd_eddst[2]= bus_space_read_1(sc->memt, sc->romh, 0xdf);
		dd->dd_cfbaddr = parseword(0xe0) & ~3;

		dd->dd_pacode[0x0] = parseword(0x100) & ~3;
		dd->dd_pacode[0x1] = parseword(0x110) & ~3;
		dd->dd_pacode[0x2] = parseword(0x120) & ~3;
		dd->dd_pacode[0x3] = parseword(0x130) & ~3;
		dd->dd_pacode[0x4] = parseword(0x140) & ~3;
		dd->dd_pacode[0x5] = parseword(0x150) & ~3;
		dd->dd_pacode[0x6] = parseword(0x160) & ~3;
		dd->dd_pacode[0x7] = parseword(0x170) & ~3;
		dd->dd_pacode[0x8] = parseword(0x180) & ~3;
		dd->dd_pacode[0x9] = parseword(0x190) & ~3;
		dd->dd_pacode[0xa] = parseword(0x1a0) & ~3;
		dd->dd_pacode[0xb] = parseword(0x1b0) & ~3;
		dd->dd_pacode[0xc] = parseword(0x1c0) & ~3;
		dd->dd_pacode[0xd] = parseword(0x1d0) & ~3;
		dd->dd_pacode[0xe] = parseword(0x1e0) & ~3;
		dd->dd_pacode[0xf] = parseword(0x1f0) & ~3;
	} else	/* STI_DEVTYPE4 */
		bus_space_read_region_4(sc->memt, sc->romh, 0, (uint32_t *)dd,
		    sizeof(*dd) / 4);

#ifdef STIDEBUG
	printf("dd:\n"
	    "devtype=%x, rev=%x;%d, altt=%x, gid=%016llx, font=%x, mss=%x\n"
	    "end=%x, regions=%x, msto=%x, timo=%d, mont=%x, user=%x[%x]\n"
	    "memrq=%x, pwr=%d, bus=%x, ebus=%x, cfb=%x\n"
	    "code=",
	    dd->dd_type & 0xff, dd->dd_grrev, dd->dd_lrrev, dd->dd_altcodet,
	    *(uint64_t *)dd->dd_grid, dd->dd_fntaddr, dd->dd_maxst,
	    dd->dd_romend, dd->dd_reglst, dd->dd_maxreent, dd->dd_maxtimo,
	    dd->dd_montbl, dd->dd_udaddr, dd->dd_udsize, dd->dd_stimemreq,
	    dd->dd_pwruse, dd->dd_bussup, dd->dd_ebussup, dd->dd_cfbaddr);
	printf("%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",
	    dd->dd_pacode[0x0], dd->dd_pacode[0x1], dd->dd_pacode[0x2],
	    dd->dd_pacode[0x3], dd->dd_pacode[0x4], dd->dd_pacode[0x5],
	    dd->dd_pacode[0x6], dd->dd_pacode[0x7], dd->dd_pacode[0x8],
	    dd->dd_pacode[0x9], dd->dd_pacode[0xa], dd->dd_pacode[0xb],
	    dd->dd_pacode[0xc], dd->dd_pacode[0xd], dd->dd_pacode[0xe],
	    dd->dd_pacode[0xf]);
#endif
	/* divise code size, could be less than STI_END entries */
	for (i = STI_END; !dd->dd_pacode[i]; i--);
	size = dd->dd_pacode[i] - dd->dd_pacode[STI_BEGIN];
	if (sc->sc_devtype == STI_DEVTYPE1)
		size = (size + 3) / 4;
	if (!(sc->sc_code = uvm_km_alloc(kernel_map, round_page(size), 0,
	    UVM_KMF_WIRED))) {
		printf(": cannot allocate %u bytes for code\n", size);
		return;
	}
#ifdef STIDEBUG
	printf("code=0x%x[%x]\n", (uint)sc->sc_code, size);
#endif

	/* copy code into memory */
	if (sc->sc_devtype == STI_DEVTYPE1) {
		p = (uint8_t *)sc->sc_code;
		for (addr = dd->dd_pacode[STI_BEGIN], eaddr = addr + size * 4;
		    addr < eaddr; addr += 4 )
			*p++ = bus_space_read_4(sc->memt, sc->romh, addr)
			    & 0xff;

	} else	/* STI_DEVTYPE4 */
		bus_space_read_region_4(sc->memt, sc->romh,
		    dd->dd_pacode[STI_BEGIN], (uint32_t *)sc->sc_code,
		    size / 4);

#define	O(i)	(dd->dd_pacode[(i)] ? (sc->sc_code +	\
	(dd->dd_pacode[(i)] - dd->dd_pacode[0]) /	\
	(sc->sc_devtype == STI_DEVTYPE1 ? 4 : 1)) : 0)
	sc->init	= (sti_init_t)	O(STI_INIT_GRAPH);
	sc->mgmt	= (sti_mgmt_t)	O(STI_STATE_MGMT);
	sc->unpmv	= (sti_unpmv_t)	O(STI_FONT_UNPMV);
	sc->blkmv	= (sti_blkmv_t)	O(STI_BLOCK_MOVE);
	sc->test	= (sti_test_t)	O(STI_SELF_TEST);
	sc->exhdl	= (sti_exhdl_t)	O(STI_EXCEP_HDLR);
	sc->inqconf	= (sti_inqconf_t)O(STI_INQ_CONF);
	sc->scment	= (sti_scment_t)O(STI_SCM_ENT);
	sc->dmac	= (sti_dmac_t)	O(STI_DMA_CTRL);
	sc->flowc	= (sti_flowc_t)	O(STI_FLOW_CTRL);
	sc->utiming	= (sti_utiming_t)O(STI_UTIMING);
	sc->pmgr	= (sti_pmgr_t)	O(STI_PROC_MGR);
	sc->util	= (sti_util_t)	O(STI_UTIL);

	if ((error = uvm_map_protect(kernel_map, sc->sc_code,
	    sc->sc_code + round_page(size), UVM_PROT_RX, FALSE))) {
		printf(": uvm_map_protect failed (%d)\n", error);
		uvm_km_free(kernel_map, sc->sc_code, round_page(size),
		    UVM_KMF_WIRED);
		return;
	}

	cc = &sc->sc_cfg;
	memset(cc, 0, sizeof (*cc));
	i = dd->dd_reglst;
#ifdef STIDEBUG
	printf("stiregions @%d:\n", i);
#endif
	r.last = 0;
	for (q = cc->regions; !r.last &&
	     q < &cc->regions[STI_REGION_MAX]; q++) {

		if (sc->sc_devtype == STI_DEVTYPE1)
			tmp = parseword(i), i += 16;
		else
			tmp = bus_space_read_4(sc->memt, sc->romh, i), i += 4;
		memcpy(&r, &tmp, sizeof (r));

		*q = (q == cc->regions ? sc->romh : sc->base) +
		    (r.offset << PGSHIFT);
#ifdef STIDEBUG
		printf("%x @ 0x%x%s%s%s%s\n",
		    r.length << PGSHIFT, *q, r.sys_only ? " sys" : "",
		    r.cache ? " cache" : "", r.btlb ? " btlb" : "",
		    r.last ? " last" : "");
#endif

		/* rom has already been mapped */
		if (q != cc->regions) {
			if (bus_space_map(sc->memt, *q,
			    r.length << PGSHIFT, 0, &fbh)) {
#ifdef STIDEBUG
				printf("already mapped region\n");
#endif
			} else {
				if (q - cc->regions == 1) {
					sc->fbaddr = *q;
					sc->fblen = r.length << PGSHIFT;
				}
				*q = fbh;
			}
		}
	}

	if ((error = sti_init(sc, 0))) {
		printf(": can not initialize (%d)\n", error);
		return;
	}

	memset(&cfg, 0, sizeof(cfg));
	memset(&ecfg, 0, sizeof(ecfg));
	cfg.ext = &ecfg;
	if ((error = sti_inqcfg(sc, &cfg))) {
		printf(": error %d inquiring config\n", error);
		return;
	}

	if ((error = sti_init(sc, STI_TEXTMODE))) {
		printf(": can not initialize (%d)\n", error);
		return;
	}

#ifdef STIDEBUG
	printf("conf: bpp=%d planes=%d attr=%d\n"
	    "crt=0x%x:0x%x:0x%x hw=0x%x:0x%x:0x%x\n",
	    cfg.bpp, cfg.planes, cfg.attributes,
	    ecfg.crt_config[0], ecfg.crt_config[1], ecfg.crt_config[2],
	    ecfg.crt_hw[0], ecfg.crt_hw[1], ecfg.crt_hw[2]);
#endif
	sc->sc_wsmode = WSDISPLAYIO_MODE_EMUL;
	sc->sc_bpp = cfg.bppu;
	printf(": %s rev %d.%02d;%d, ID 0x%016llX\n"
	    "%s: %dx%d frame buffer, %dx%dx%d display, offset %dx%d\n",
	    cfg.name, dd->dd_grrev >> 4, dd->dd_grrev & 0xf, dd->dd_lrrev,
	    *(uint64_t *)dd->dd_grid, device_xname(sc->sc_dev), cfg.fbwidth,
	    cfg.fbheight, cfg.width, cfg.height, cfg.bppu, cfg.owidth,
	    cfg.oheight);

	if ((error = sti_fetchfonts(sc, &cfg, dd->dd_fntaddr))) {
		aprint_error_dev(sc->sc_dev, "cannot fetch fonts (%d)\n",
		    error);
		return;
	}

	/*
	 * parse screen descriptions:
	 *	figure number of fonts supported;
	 *	allocate wscons structures;
	 *	calculate dimensions.
	 */

	sti_default_screen.ncols = cfg.width / sc->sc_curfont.width;
	sti_default_screen.nrows = cfg.height / sc->sc_curfont.height;
	sti_default_screen.fontwidth = sc->sc_curfont.width;
	sti_default_screen.fontheight = sc->sc_curfont.height;

#if NWSDISPLAY > 0
	config_interrupts(sc->sc_dev, sti_attach_deferred);
#endif

}

void
sti_attach_deferred(device_t dev)
{
	struct sti_softc *sc = device_private(dev);
	struct wsemuldisplaydev_attach_args waa;
	long defattr;

	waa.console = sc->sc_flags & STI_CONSOLE ? 1 : 0;
	waa.scrdata = &sti_default_screenlist;
	waa.accessops = &sti_accessops;
	waa.accesscookie = sc;

	/* attach as console if required */
	if (waa.console) {
		sti_alloc_attr(sc, 0, 0, 0, &defattr);
		wsdisplay_cnattach(&sti_default_screen, sc,
		    0, sti_default_screen.nrows - 1, defattr);
	}

	config_found(sc->sc_dev, &waa, wsemuldisplaydevprint);
}

int
sti_fetchfonts(struct sti_softc *sc, struct sti_inqconfout *cfg, uint32_t addr)
{
	struct sti_font *fp = &sc->sc_curfont;
	int size;
#ifdef notyet
	int uc;
	struct {
		struct sti_unpmvflags flags;
		struct sti_unpmvin in;
		struct sti_unpmvout out;
	} a;
#endif

	/*
	 * Get the first PROM font in memory
	 */
	do {
		if (sc->sc_devtype == STI_DEVTYPE1) {
			fp->first  = parseshort(addr + 0x00);
			fp->last   = parseshort(addr + 0x08);
			fp->width  = bus_space_read_1(sc->memt, sc->romh,
			    addr + 0x13);
			fp->height = bus_space_read_1(sc->memt, sc->romh,
			    addr + 0x17);
			fp->type   = bus_space_read_1(sc->memt, sc->romh,
			    addr + 0x1b);
			fp->bpc    = bus_space_read_1(sc->memt, sc->romh,
			    addr + 0x1f);
			fp->next   = parseword(addr + 0x23);
			fp->uheight= bus_space_read_1(sc->memt, sc->romh,
			    addr + 0x33);
			fp->uoffset= bus_space_read_1(sc->memt, sc->romh,
			    addr + 0x37);
		} else	/* STI_DEVTYPE4 */
			bus_space_read_region_4(sc->memt, sc->romh, addr,
			    (uint32_t *)fp, sizeof(struct sti_font) / 4);

		printf("%s: %dx%d font type %d, %d bpc, charset %d-%d\n",
		    device_xname(sc->sc_dev), fp->width, fp->height, fp->type,
		    fp->bpc, fp->first, fp->last);

		size = sizeof(struct sti_font) +
		    (fp->last - fp->first + 1) * fp->bpc;
		if (sc->sc_devtype == STI_DEVTYPE1)
			size *= 4;
		sc->sc_romfont = malloc(size, M_DEVBUF, M_NOWAIT);
		if (sc->sc_romfont == NULL)
			return (ENOMEM);

		bus_space_read_region_4(sc->memt, sc->romh, addr,
		    (uint32_t *)sc->sc_romfont, size / 4);

		addr = 0; /* fp->next */
	} while (addr);

#ifdef notyet
	/*
	 * If there is enough room in the off-screen framebuffer memory,
	 * display all the characters there in order to display them
	 * faster with blkmv operations rather than unpmv later on.
	 */
	if (size <= cfg->fbheight *
	    (cfg->fbwidth - cfg->width - cfg->owidth)) {
		memset(&a, 0, sizeof(a));
		a.flags.flags = STI_UNPMVF_WAIT;
		a.in.fg_colour = STI_COLOUR_WHITE;
		a.in.bg_colour = STI_COLOUR_BLACK;
		a.in.font_addr = sc->sc_romfont;

		sc->sc_fontmaxcol = cfg->fbheight / fp->height;
		sc->sc_fontbase = cfg->width + cfg->owidth;
		for (uc = fp->first; uc <= fp->last; uc++) {
			a.in.x = ((uc - fp->first) / sc->sc_fontmaxcol) *
			    fp->width + sc->sc_fontbase;
			a.in.y = ((uc - fp->first) % sc->sc_fontmaxcol) *
			    fp->height;
			a.in.index = uc;

			(*sc->unpmv)(&a.flags, &a.in, &a.out, &sc->sc_cfg);
			if (a.out.errno) {
				aprint_error_dev(sc->sc_dev, "unpmv %d returned %d\n",
				    uc, a.out.errno);
				return (0);
			}
		}

		free(sc->sc_romfont, M_DEVBUF);
		sc->sc_romfont = NULL;
	}
#endif

	return (0);
}

int
sti_init(struct sti_softc *sc, int mode)
{
	struct {
		struct sti_initflags flags;
		struct sti_initin in;
		struct sti_initout out;
	} a;

	memset(&a, 0, sizeof(a));

	a.flags.flags = STI_INITF_WAIT | STI_INITF_CMB | STI_INITF_EBET |
	    (mode & STI_TEXTMODE ? STI_INITF_TEXT | STI_INITF_PBET |
	     STI_INITF_PBETI | STI_INITF_ICMT : 0);
	a.in.text_planes = 1;
#ifdef STIDEBUG
	printf("%s: init,%p(%x, %p, %p, %p)\n", device_xname(sc->sc_dev),
	    sc->init, a.flags.flags, &a.in, &a.out, &sc->sc_cfg);
#endif
	(*sc->init)(&a.flags, &a.in, &a.out, &sc->sc_cfg);
	return (a.out.text_planes != a.in.text_planes || a.out.errno);
}

int
sti_inqcfg(struct sti_softc *sc, struct sti_inqconfout *out)
{
	struct {
		struct sti_inqconfflags flags;
		struct sti_inqconfin in;
	} a;

	memset(&a, 0, sizeof(a));

	a.flags.flags = STI_INQCONFF_WAIT;
	(*sc->inqconf)(&a.flags, &a.in, out, &sc->sc_cfg);

	return out->errno;
}

void
sti_bmove(struct sti_softc *sc, int x1, int y1, int x2, int y2, int h, int w,
    enum sti_bmove_funcs f)
{
	struct {
		struct sti_blkmvflags flags;
		struct sti_blkmvin in;
		struct sti_blkmvout out;
	} a;

	memset(&a, 0, sizeof(a));

	a.flags.flags = STI_BLKMVF_WAIT;
	switch (f) {
	case bmf_clear:
		a.flags.flags |= STI_BLKMVF_CLR;
		a.in.bg_colour = STI_COLOUR_BLACK;
		break;
	case bmf_underline:
	case bmf_copy:
		a.in.fg_colour = STI_COLOUR_WHITE;
		a.in.bg_colour = STI_COLOUR_BLACK;
		break;
	case bmf_invert:
		a.flags.flags |= STI_BLKMVF_COLR;
		a.in.fg_colour = STI_COLOUR_BLACK;
		a.in.bg_colour = STI_COLOUR_WHITE;
		break;
	}
	a.in.srcx = x1;
	a.in.srcy = y1;
	a.in.dstx = x2;
	a.in.dsty = y2;
	a.in.height = h;
	a.in.width = w;

	(*sc->blkmv)(&a.flags, &a.in, &a.out, &sc->sc_cfg);
#ifdef STIDEBUG
	if (a.out.errno)
		aprint_error_dev(sc->sc_dev, "blkmv returned %d\n",
		    a.out.errno);
#endif
}

int
sti_setcment(struct sti_softc *sc, u_int i, u_char r, u_char g, u_char b)
{
	struct {
		struct sti_scmentflags flags;
		struct sti_scmentin in;
		struct sti_scmentout out;
	} a;

	memset(&a, 0, sizeof(a));

	a.flags.flags = STI_SCMENTF_WAIT;
	a.in.entry = i;
	a.in.value = (r << 16) | (g << 8) | b;

	(*sc->scment)(&a.flags, &a.in, &a.out, &sc->sc_cfg);

	return a.out.errno;
}

int
sti_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct sti_softc *sc = v;
	struct wsdisplay_fbinfo *wdf;
	struct wsdisplay_cmap *cmapp;
	u_int mode, idx, count;
	int i, ret;

	ret = 0;
	switch (cmd) {
	case WSDISPLAYIO_GMODE:
		*(u_int *)data = sc->sc_wsmode;
		break;

	case WSDISPLAYIO_SMODE:
		mode = *(u_int *)data;
		if (sc->sc_wsmode == WSDISPLAYIO_MODE_EMUL &&
		    mode == WSDISPLAYIO_MODE_DUMBFB)
			ret = sti_init(sc, 0);
		else if (sc->sc_wsmode == WSDISPLAYIO_MODE_DUMBFB &&
		    mode == WSDISPLAYIO_MODE_EMUL)
			ret = sti_init(sc, STI_TEXTMODE);
		sc->sc_wsmode = mode;
		break;

	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_STI;
		break;

	case WSDISPLAYIO_GINFO:
		wdf = (struct wsdisplay_fbinfo *)data;
		wdf->height = sc->sc_cfg.scr_height;
		wdf->width  = sc->sc_cfg.scr_width;
		wdf->depth  = sc->sc_bpp;
		wdf->cmsize = STI_NCMAP;
		break;

	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = sc->sc_cfg.fb_width;
		break;

	case WSDISPLAYIO_GETCMAP:
		if (sc->scment == NULL)
			return ENOTTY;
		cmapp = (struct wsdisplay_cmap *)data;
		idx = cmapp->index;
		count = cmapp->count;
		if (idx > STI_NCMAP || idx + count >= STI_NCMAP)
			return EINVAL;
		if ((ret = copyout(&sc->sc_rcmap[idx], cmapp->red, count)))
			break;
		if ((ret = copyout(&sc->sc_gcmap[idx], cmapp->green, count)))
			break;
		if ((ret = copyout(&sc->sc_bcmap[idx], cmapp->blue, count)))
			break;
		break;

	case WSDISPLAYIO_PUTCMAP:
		if (sc->scment == NULL)
			return ENOTTY;
		cmapp = (struct wsdisplay_cmap *)data;
		idx = cmapp->index;
		count = cmapp->count;
		if (idx > STI_NCMAP || idx + count >= STI_NCMAP)
			return EINVAL;
		if ((ret = copyin(cmapp->red, &sc->sc_rcmap[idx], count)))
			break;
		if ((ret = copyin(cmapp->green, &sc->sc_gcmap[idx], count)))
			break;
		if ((ret = copyin(cmapp->blue, &sc->sc_bcmap[idx], count)))
			break;
		for (i = idx + count - 1; i >= idx; i--)
			if ((ret = sti_setcment(sc, i, sc->sc_rcmap[i],
			    sc->sc_gcmap[i], sc->sc_bcmap[i]))) {
#ifdef STIDEBUG
				printf("sti_ioctl: "
				    "sti_setcment(%d, %u, %u, %u): %%d\n", i,
				    (u_int)sc->sc_rcmap[i],
				    (u_int)sc->sc_gcmap[i],
				    (u_int)sc->sc_bcmap[i]);
#endif
				ret = EINVAL;
				break;
			}
		break;

	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
	default:
		return (ENOTTY);	/* not supported yet */
	}

	return (ret);
}

paddr_t
sti_mmap(void *v, void *vs, off_t offset, int prot)
{
	/* XXX not finished */
	return -1;
}

int
sti_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookiep,
    int *cxp, int *cyp, long *defattr)
{
	struct sti_softc *sc = v;

	if (sc->sc_nscreens > 0)
		return ENOMEM;

	*cookiep = sc;
	*cxp = 0;
	*cyp = 0;
	sti_alloc_attr(sc, 0, 0, 0, defattr);
	sc->sc_nscreens++;
	return 0;
}

void
sti_free_screen(void *v, void *cookie)
{
	struct sti_softc *sc = v;

	sc->sc_nscreens--;
}

int
sti_show_screen(void *v, void *cookie, int waitok, void (*cb)(void *, int, int), void *cbarg)
{
	return 0;
}

int
sti_load_font(void *v, void *cookie, struct wsdisplay_font *font)
{
	return -1;
}

void
sti_cursor(void *v, int on, int row, int col)
{
	struct sti_softc *sc = v;
	struct sti_font *fp = &sc->sc_curfont;

	sti_bmove(sc, col * fp->width, row * fp->height, col * fp->width,
	    row * fp->height, fp->height, fp->width, bmf_invert);
}

int
sti_mapchar(void *v, int uni, u_int *index)
{
	if (uni < 256)
		*index = uni;

	return 1;
}

void
sti_putchar(void *v, int row, int col, u_int uc, long attr)
{
	struct sti_softc *sc = v;
	struct sti_font *fp = &sc->sc_curfont;

	if (sc->sc_romfont != NULL) {
		/*
		 * Font is in memory, use unpmv
		 */
		struct {
			struct sti_unpmvflags flags;
			struct sti_unpmvin in;
			struct sti_unpmvout out;
		} a;

		memset(&a, 0, sizeof(a));

		a.flags.flags = STI_UNPMVF_WAIT;
		/* XXX does not handle text attributes */
		a.in.fg_colour = STI_COLOUR_WHITE;
		a.in.bg_colour = STI_COLOUR_BLACK;
		a.in.x = col * fp->width;
		a.in.y = row * fp->height;
		a.in.font_addr = sc->sc_romfont;
		a.in.index = uc;

		(*sc->unpmv)(&a.flags, &a.in, &a.out, &sc->sc_cfg);
	} else {
		/*
		 * Font is in frame buffer, use blkmv
		 */
		struct {
			struct sti_blkmvflags flags;
			struct sti_blkmvin in;
			struct sti_blkmvout out;
		} a;

		memset(&a, 0, sizeof(a));

		a.flags.flags = STI_BLKMVF_WAIT;
		/* XXX does not handle text attributes */
		a.in.fg_colour = STI_COLOUR_WHITE;
		a.in.bg_colour = STI_COLOUR_BLACK;

		a.in.srcx = ((uc - fp->first) / sc->sc_fontmaxcol) *
		    fp->width + sc->sc_fontbase;
		a.in.srcy = ((uc - fp->first) % sc->sc_fontmaxcol) *
		    fp->height;
		a.in.dstx = col * fp->width;
		a.in.dsty = row * fp->height;
		a.in.height = fp->height;
		a.in.width = fp->width;

		(*sc->blkmv)(&a.flags, &a.in, &a.out, &sc->sc_cfg);
	}
}

void
sti_copycols(void *v, int row, int srccol, int dstcol, int ncols)
{
	struct sti_softc *sc = v;
	struct sti_font *fp = &sc->sc_curfont;

	sti_bmove(sc, srccol * fp->width, row * fp->height, dstcol * fp->width,
	    row * fp->height, fp->height, ncols * fp->width, bmf_copy);
}

void
sti_erasecols(void *v, int row, int startcol, int ncols, long attr)
{
	struct sti_softc *sc = v;
	struct sti_font *fp = &sc->sc_curfont;

	sti_bmove(sc, startcol * fp->width, row * fp->height,
	    startcol * fp->width, row * fp->height, fp->height,
	    ncols * fp->width, bmf_clear);
}

void
sti_copyrows(void *v, int srcrow, int dstrow, int nrows)
{
	struct sti_softc *sc = v;
	struct sti_font *fp = &sc->sc_curfont;

	sti_bmove(sc, sc->sc_cfg.oscr_width, srcrow * fp->height,
	    sc->sc_cfg.oscr_width, dstrow * fp->height,
	    nrows * fp->height, sc->sc_cfg.scr_width, bmf_copy);
}

void
sti_eraserows(void *v, int srcrow, int nrows, long attr)
{
	struct sti_softc *sc = v;
	struct sti_font *fp = &sc->sc_curfont;

	sti_bmove(sc, sc->sc_cfg.oscr_width, srcrow * fp->height,
	    sc->sc_cfg.oscr_width, srcrow * fp->height,
	    nrows * fp->height, sc->sc_cfg.scr_width, bmf_clear);
}

int
sti_alloc_attr(void *v, int fg, int bg, int flags, long *pattr)
{
	/* struct sti_softc *sc = v; */

	*pattr = 0;

	return 0;
}

