/*	$NetBSD: fb_sbdio.c,v 1.2.16.3 2007/03/24 14:54:41 yamt Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
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

#define WIRED_FB_TLB

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fb_sbdio.c,v 1.2.16.3 2007/03/24 14:54:41 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <dev/cons.h>

#include <uvm/uvm_extern.h>     /* pmap function to remap FB */

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wsfont/wsfont.h>
#include <dev/rasops/rasops.h>

#include <mips/pte.h>

#include <machine/locore.h>
#include <machine/sbdiovar.h>

#include <machine/gareg.h>
#include <machine/gavar.h>

#include <machine/vmparam.h>
#include <machine/wired_map.h>


struct fb_softc {
	struct device sc_dv;
	struct rasops_info *sc_ri;
	struct ga *sc_ga;
	int sc_nscreens;
};

int fb_sbdio_match(struct device *, struct cfdata *, void *);
void fb_sbdio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(fb_sbdio, sizeof(struct fb_softc),
    fb_sbdio_match, fb_sbdio_attach, NULL, NULL);

int _fb_ioctl(void *, void *, u_long, void *, int, struct lwp *);
paddr_t _fb_mmap(void *, void *, off_t, int);
int _fb_alloc_screen(void *, const struct wsscreen_descr *, void **,
    int *, int *, long *);
void _fb_free_screen(void *, void *);
int _fb_show_screen(void *, void *, int, void (*)(void *, int, int), void *);
void _fb_pollc(void *, int);
void fb_common_init(struct rasops_info *, struct ga *);
int fb_sbdio_cnattach(uint32_t, uint32_t, int);
void fb_pmap_enter(paddr_t, paddr_t, vaddr_t *, vaddr_t *);

struct wsscreen_descr _fb_std_screen = {
	"std", 0, 0,
	0, /* textops */
	0, 0,
	0
};

const struct wsscreen_descr *_fb_screen_table[] = {
	&_fb_std_screen,
};

struct wsscreen_list _fb_screen_list = {
	.nscreens	=
	    sizeof(_fb_screen_table) / sizeof(_fb_screen_table[0]),
	.screens	= _fb_screen_table
};

struct wsdisplay_accessops _fb_accessops = {
	.ioctl		= _fb_ioctl,
	.mmap		= _fb_mmap,
	.alloc_screen	= _fb_alloc_screen,
	.free_screen	= _fb_free_screen,
	.show_screen	= _fb_show_screen,
	.load_font	= 0,
	.pollc		= _fb_pollc
};

/* console stuff */
static struct rasops_info fb_console_ri;
static struct ga fb_console_ga;
static paddr_t fb_consaddr;


int
fb_sbdio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct sbdio_attach_args *sa = aux;

	return strcmp(sa->sa_name, "fb") ? 0 : 1;
}

void
fb_sbdio_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbdio_attach_args *sa = aux;
	struct wsemuldisplaydev_attach_args wa;
	struct fb_softc *sc = (void *)self;
	struct rasops_info *ri;
	struct ga *ga;
	vaddr_t memva, regva;
	int console;

	printf(" at %p, %p\n", (void *)sa->sa_addr1, (void *)sa->sa_addr2);

	console = (sa->sa_addr1 == fb_consaddr);
	if (console) {
		/* already initialized in fb_cnattach() */
		sc->sc_ri = ri = &fb_console_ri;
		sc->sc_ga = &fb_console_ga;
		sc->sc_nscreens = 1;
	} else {
		ri = malloc(sizeof(struct rasops_info), M_DEVBUF,
		    M_NOWAIT | M_ZERO);
		if (ri == NULL) {
			printf(":can't allocate rasops memory\n");
			return;
		}
		ga = malloc(sizeof(struct ga), M_DEVBUF, M_NOWAIT | M_ZERO);
		if (ga == NULL) {
			printf(":can't allocate ga memory\n");
			return;
		}
		ga->reg_paddr = sa->sa_addr2;
		ga->flags = sa->sa_flags;
		fb_pmap_enter(sa->sa_addr1, sa->sa_addr2,
		    &memva, &regva);
		ri->ri_bits = (void *)memva;
		ga->reg_addr = regva;
		fb_common_init(ri, ga);
		sc->sc_ri = ri;
		sc->sc_ga = ga;
	}

	wa.console = console;
	wa.scrdata = &_fb_screen_list;
	wa.accessops = &_fb_accessops;
	wa.accesscookie	= (void *)sc;

	config_found(self, &wa, wsdisplaydevprint);
}

void
fb_common_init(struct rasops_info *ri, struct ga *ga)
{
	int ga_active, cookie;

	/* XXX */
	ga_active = 0;
	if (ga->flags == 0x0000 || ga->flags == 0x0001)
		ga_active = ga_init(ga);

	/*
	 * TR2 IPL CLUT.
	 * 0     black
	 * 1	 red
	 * 2	 green
	 * 4	 blue
	 * 8	 yellow
	 * 16	 cyan
	 * 32	 magenta
	 * 64	 light gray
	 * 128	 dark gray
	 * 255	 white
	 * other black
	 * When CLUT isn't initialized for NetBSD, use black-red pair.
	 */
	ri->ri_flg = RI_CENTER | RI_CLEAR;
	if (!ga_active)
		ri->ri_flg |= RI_FORCEMONO;

	ri->ri_depth = 8;
	ri->ri_width = 1280;
	ri->ri_height = 1024;
	ri->ri_stride = 2048;
	ri->ri_hw = (void *)ga;

	wsfont_init();
	/* prefer 12 pixel wide font */
	cookie = wsfont_find(NULL, 12, 0, 0, 0, 0);
	if (cookie <= 0)
		cookie = wsfont_find(NULL, 0, 0, 0, 0, 0);
	if (cookie <= 0) {
		printf("sfb: font table is empty\n");
		return;
	}

	if (wsfont_lock(cookie, &ri->ri_font)) {
		printf("fb: can't lock font\n");
		return;
	}
	ri->ri_wsfcookie = cookie;

	rasops_init(ri, 34, 80);

#if 0
	/* XXX no accelarated functions yet */
	ri->ri_ops.putchar = xxx_putchar;
	ri->ri_ops.erasecols = xxx_erasecols;
	ri->ri_ops.copyrows = xxx_copyrows;
	ri->ri_ops.eraserows = xxx_eraserows;
	ir->ri_do_cursor = xxx_do_cursor;
#endif

	/* XXX shouldn't be global */
	_fb_std_screen.nrows = ri->ri_rows;
	_fb_std_screen.ncols = ri->ri_cols;
	_fb_std_screen.textops = &ri->ri_ops;
	_fb_std_screen.capabilities = ri->ri_caps;
}

int
fb_sbdio_cnattach(uint32_t mem, uint32_t reg, int flags)
{
	struct rasops_info *ri;
	struct ga *ga;
	vaddr_t memva, regva;
	long defattr;

	ri = &fb_console_ri;
	ga = &fb_console_ga;

	ga->reg_paddr = reg;
	ga->flags = flags;
	fb_pmap_enter((paddr_t)mem, (paddr_t)reg, &memva, &regva);
	ri->ri_bits = (void *)memva;
	ga->reg_addr = regva;

	fb_common_init(ri, ga);

	(*ri->ri_ops.allocattr)(ri, 0, 0, 0, &defattr);
	wsdisplay_cnattach(&_fb_std_screen, ri, 0, 0, defattr);
	fb_consaddr = mem;

	return 0;
}

int
_fb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct fb_softc *sc = v;
	struct wsdisplay_fbinfo *fbinfo = (void *)data;
	struct wsdisplay_cmap *cmap = (void *)data;
	struct rasops_info *ri = sc->sc_ri;
	struct ga *ga = sc->sc_ga;
	int i;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(uint32_t *)data = WSDISPLAY_TYPE_UNKNOWN;
		return 0;

	case WSDISPLAYIO_GINFO:
		fbinfo->height = ri->ri_height;
		fbinfo->width  = ri->ri_width;
		fbinfo->depth  = ri->ri_depth;
		fbinfo->cmsize = 256;
		return 0;

#if 1
	case WSDISPLAYIO_LINEBYTES:
		*(u_int *)data = ri->ri_stride;
		return 0;

#endif
	case WSDISPLAYIO_GETCMAP:
		if (ri->ri_flg == RI_FORCEMONO)
			break;
		ga_clut_get(ga);
		for (i = 0; i < cmap->count; i++) {
			cmap->red[i] = ga->clut[cmap->index + i][0];
			cmap->green[i] = ga->clut[cmap->index + i][1];
			cmap->blue[i] = ga->clut[cmap->index + i][2];
		}
		return 0;

	case WSDISPLAYIO_PUTCMAP:
		if (ri->ri_flg == RI_FORCEMONO)
			break;
		for (i = 0; i < cmap->count; i++) {
			ga->clut[cmap->index + i][0] = cmap->red[i];
			ga->clut[cmap->index + i][1] = cmap->green[i];
			ga->clut[cmap->index + i][2] = cmap->blue[i];
		}
		ga_clut_set(ga);
		return 0;
	}

	return EPASSTHROUGH;
}

paddr_t
_fb_mmap(void *v, void *vs, off_t offset, int prot)
{

	if (offset < 0 || offset >= GA_FRB_SIZE)
		return -1;

	return mips_btop(GA_FRB_ADDR + offset);
}

int
_fb_alloc_screen(void *v, const struct wsscreen_descr *type, void **cookie,
    int *curx, int *cury, long *attr)
{
	struct fb_softc *sc = v;
	struct rasops_info *ri = sc->sc_ri;
	long defattr;

#if 0
	if (sc->sc_nscreens > 0)
		return ENOMEM;
#endif

	*cookie = ri;
	*curx = *cury = 0;
	ri->ri_ops.allocattr(ri, 0, 0, 0, &defattr);
	*attr = defattr;
	sc->sc_nscreens++;

	return 0;
}

void
_fb_free_screen(void *v, void *cookie)
{
	struct fb_softc *sc = v;

	sc->sc_nscreens--;
}

int
_fb_show_screen(void *v, void *cookie, int waitok,
    void (*cb)(void *, int, int), void *cbarg)
{

	return 0;
}

void
_fb_pollc(void *v, int on)
{

	/* no interrupt used. */
}

void
fb_pmap_enter(paddr_t fb_paddr, paddr_t reg_paddr,
    vaddr_t *fb_vaddr, vaddr_t *reg_vaddr)
{
#ifdef WIRED_FB_TLB	/* Make wired 16MPage for frame buffer */
	vaddr_t va;
	vsize_t fb_off, reg_off, pgsize;

	pgsize = MIPS3_WIRED_SIZE;
	fb_off  = fb_paddr  & MIPS3_WIRED_OFFMASK;
	reg_off = reg_paddr & MIPS3_WIRED_OFFMASK;
	fb_paddr  = fb_paddr  & ~MIPS3_WIRED_OFFMASK;
	reg_paddr = reg_paddr & ~MIPS3_WIRED_OFFMASK;
	va = GA_FRB_ADDR;

	if (mips3_wired_enter_page(va, fb_paddr, pgsize) == false) {
		printf("cannot allocate fb memory\n");
		return;
	}

	if (mips3_wired_enter_page(va + pgsize, reg_paddr, pgsize) == false) {
		printf("cannot allocate fb register\n");
		return;
	}

	*fb_vaddr  = va + fb_off;
	*reg_vaddr = va + pgsize + reg_off;
#else /* WIRED_FB_TLB */
	paddr_t pa, epa;
	vaddr_t va, tva;

	pa = addr;
	epa = pa + size;

	va = uvm_km_alloc(kernel_map, epa - pa, 0, UVM_KMF_VAONLY);
	if (va == 0)
		for (;;)
			ROM_MONITOR();

	for (tva = va; pa < epa; pa += PAGE_SIZE, tva += PAGE_SIZE)
		pmap_kenter_pa(tva, pa, VM_PROT_READ | VM_PROT_WRITE);

	pmap_update(pmap_kernel());

	*fb_vaddr  = va;
#endif /* WIRED_FB_TLB */
}
