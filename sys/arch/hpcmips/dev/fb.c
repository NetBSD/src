/*	$NetBSD: fb.c,v 1.2 1999/09/26 10:22:10 takemura Exp $	*/

/*-
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
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

static const char _copyright[] __attribute__ ((unused)) =
    "Copyright (c) 19999 Shin Takemura.  All rights reserved.";
static const char _rcsid[] __attribute__ ((unused)) =
    "$Id: fb.c,v 1.2 1999/09/26 10:22:10 takemura Exp $";


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/ioctl.h>

#include <vm/vm.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/bootinfo.h>

#include <arch/hpcmips/dev/fbvar.h>

#include <dev/rcons/raster.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wscons_raster.h>
#include <dev/wscons/wsdisplayvar.h>

#define FBDEBUG
#ifdef FBDEBUG
int	fb_debug = 1;
#define	DPRINTF(arg) if (fb_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

/*
 *  function prototypes
 */
int	fbmatch __P((struct device *, struct cfdata *, void *));
void	fbattach __P((struct device *, struct device *, void *));
int	fbprint __P((void *, const char *));

int	fb_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
int	fb_mmap __P((void *, off_t, int));
static int	fb_getdevconfig __P((struct fb_devconfig *dc));
static int	fb_alloc_screen __P((void *, const struct wsscreen_descr *,
				     void **, int *, int *, long *));
static void	fb_free_screen __P((void *, void *));
static void	fb_show_screen __P((void *, void *));

static int	pow __P((int, int));

/*
 *  static variables
 */
struct cfattach fb_ca = {
	sizeof(struct fb_softc), fbmatch, fbattach,
};

struct wsdisplay_emulops fb_emulops = {
	rcons_cursor,
	rcons_mapchar,
	rcons_putchar,
	rcons_copycols,
	rcons_erasecols,
	rcons_copyrows,
	rcons_eraserows,
	rcons_alloc_attr
};

struct wsscreen_descr fb_stdscreen = {
	"std",
	0, 0,	/* will be filled in -- XXX shouldn't, it's global */
	&fb_emulops,
	0, 0,
	WSSCREEN_REVERSE
};

const struct wsscreen_descr *_fb_scrlist[] = {
	&fb_stdscreen,
	/* XXX other formats, graphics screen? */
};

struct wsscreen_list fb_screenlist = {
	sizeof(_fb_scrlist) / sizeof(struct wsscreen_descr *), _fb_scrlist
};

struct wsdisplay_accessops fb_accessops = {
	fb_ioctl,
	fb_mmap,
	fb_alloc_screen,
	fb_free_screen,
	fb_show_screen,
	0 /* load_font */
};

struct fb_devconfig fb_console_dc;

/*
 *  function bodies
 */
static int
pow(int x, int n)
{
	int res = 1;
	while (0 < n--) {
		res *= x;
	}
	return (res);
}

int
fbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct mainbus_attach_args *ma = aux;
    
	if (strcmp(ma->ma_name, match->cf_driver->cd_name))
		return 0;

	return (1);
}

void
fbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fb_softc *sc = (struct fb_softc *)self;
	struct mainbus_attach_args *ma = aux;
	struct wsemuldisplaydev_attach_args wa;
	int console;

	/* avoid warning */
	console = (int)ma->ma_iot;

	console = 0;
	if (console) {
		sc->sc_dc = &fb_console_dc;
		sc->nscreens = 1;
	} else {
		sc->sc_dc = (struct fb_devconfig *)
		    malloc(sizeof(struct fb_devconfig), M_DEVBUF, M_WAITOK);
		bzero(sc->sc_dc, sizeof(struct fb_devconfig));
		if (fb_getdevconfig(sc->sc_dc) != 0) {
			return;
		}
	}

	printf(": %dx%d pixels, %d colors, %dx%d chars",
	       sc->sc_dc->dc_raster.width,
	       sc->sc_dc->dc_raster.height,
	       pow(2, sc->sc_dc->dc_raster.depth),
	       sc->sc_dc->dc_rcons.rc_maxcol,
	       sc->sc_dc->dc_rcons.rc_maxrow);

	printf("\n");

	wa.console = 0;
	wa.scrdata = &fb_screenlist;
	wa.accessops = &fb_accessops;
	wa.accesscookie = sc;

	config_found(self, &wa, wsemuldisplaydevprint);
}

int
fb_getdevconfig(dc)
	struct fb_devconfig *dc;
{
	int i;
	int depth;
	struct rcons *rcp;

	if (bootinfo == NULL ||
	    bootinfo->fb_addr == 0 ||
	    bootinfo->fb_line_bytes == 0 ||
	    bootinfo->fb_width == 0 ||
	    bootinfo->fb_height == 0) {
		printf(": no frame buffer infomation.\n");
		return (-1);
	}

	dc->dc_width = bootinfo->fb_width;
	dc->dc_height = bootinfo->fb_height;
	dc->dc_rowbytes = bootinfo->fb_line_bytes;
	dc->dc_fbaddr = (vaddr_t)bootinfo->fb_addr;

	/* clear the screen */
	for (i = 0;
	     i < dc->dc_height * dc->dc_rowbytes;
	     i += sizeof(u_int32_t)) {
		*(u_int32_t *)(dc->dc_fbaddr + i) = 0;
	}

	switch (bootinfo->fb_type) {
	case BIFB_D2_M2L_3:
	case BIFB_D2_M2L_3x2:
	case BIFB_D2_M2L_0:
	case BIFB_D2_M2L_0x2:
		depth = 2;
		break;
	case BIFB_D8_00:
	case BIFB_D8_FF:
		depth = 8;
		break;
	case BIFB_D16_0000:
	case BIFB_D16_FFFF:
		depth = 16;
		break;
	default:
		printf(": unknown type (=%d).\n", bootinfo->fb_type);
		return (-1);
		break;
	}

	/* initialize the raster */
	dc->dc_raster.width = dc->dc_width;
	dc->dc_raster.height = dc->dc_height;
	dc->dc_raster.depth = depth;
	dc->dc_raster.linelongs = dc->dc_rowbytes / sizeof(u_int32_t);
	dc->dc_raster.pixels = (u_int32_t *)dc->dc_fbaddr;
	dc->dc_raster.data = (caddr_t)dc;

	/* initialize the raster console blitter */
	rcp = &dc->dc_rcons;
	rcp->rc_sp = &dc->dc_raster;
	rcp->rc_crow = rcp->rc_ccol = -1;
	rcp->rc_crowp = &rcp->rc_crow;
	rcp->rc_ccolp = &rcp->rc_ccol;
	rcons_init(rcp, 34, 80);

	fb_stdscreen.nrows = dc->dc_rcons.rc_maxrow;
	fb_stdscreen.ncols = dc->dc_rcons.rc_maxcol;

	return (0);
}

int
fb_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	DPRINTF(("%s(%d): fb_ioctl()\n", __FILE__, __LINE__));

	return (-1);
}

int
fb_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	DPRINTF(("%s(%d): fb_mmap()\n", __FILE__, __LINE__));

	return (-1);
}

int
fb_alloc_screen(v, type, cookiep, curxp, curyp, attrp)
	void *v;
	const struct wsscreen_descr *type;
	void **cookiep;
	int *curxp, *curyp;
	long *attrp;
{
	struct fb_softc *sc = v;
	long defattr;

	DPRINTF(("%s(%d): fb_alloc_screen()\n", __FILE__, __LINE__));

	if (sc->nscreens > 0)
		return (ENOMEM);

	*cookiep = &sc->sc_dc->dc_rcons; /* one and only for now */
	*curxp = 0;
	*curyp = 0;
	rcons_alloc_attr(&sc->sc_dc->dc_rcons, 0, 0, 0, &defattr);
	*attrp = defattr;
	sc->nscreens++;
	return (0);
}

void
fb_free_screen(v, cookie)
	void *v;
	void *cookie;
{
	DPRINTF(("%s(%d): fb_free_screen()\n", __FILE__, __LINE__));
}

void
fb_show_screen(v, cookie)
	void *v;
	void *cookie;
{
	DPRINTF(("%s(%d): fb_show_screen()\n", __FILE__, __LINE__));
}
