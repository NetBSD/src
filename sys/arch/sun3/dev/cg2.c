/*	$NetBSD: cg2.c,v 1.31 2014/07/25 08:10:35 dholland Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)cgthree.c	8.2 (Berkeley) 10/30/93
 */

/*
 * color display (cg2) driver.
 *
 * Does not handle interrupts, even though they can occur.
 *
 * XXX should defer colormap updates to vertical retrace interrupts
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cg2.c,v 1.31 2014/07/25 08:10:35 dholland Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/tty.h>

#include <uvm/uvm_extern.h>

#include <dev/sun/fbio.h>
#include <machine/autoconf.h>
#include <machine/pmap.h>
#include <machine/cg2reg.h>

#include "ioconf.h"
#include "fbvar.h"

#define	CMSIZE 256

/* offset to and size of mapped part of frame buffer */
#define PLANEMAP_SIZE		0x100000
#define PIXELMAP_SIZE		0x100000
#define ROWOPMAP_SIZE		0x100000

#define	CTLREGS_OFF 		0x300000
#define	CTLREGS_SIZE		 0x10600

#define	CG2_MAPPED_OFFSET	(PLANEMAP_SIZE + PIXELMAP_SIZE)
#define	CG2_MAPPED_SIZE 	(CTLREGS_OFF + CTLREGS_SIZE)

/* per-display variables */
struct cg2_softc {
	device_t sc_dev;		/* base device */
	struct	fbdevice sc_fb;		/* frame buffer device */
	int 	sc_phys;		/* display RAM (phys addr) */
	int 	sc_pmtype;		/* pmap type bits */
	struct	cg2fb *sc_ctlreg;	/* control registers */
};

/* autoconfiguration driver */
static int	cg2match(device_t, cfdata_t, void *);
static void	cg2attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cgtwo, sizeof(struct cg2_softc),
    cg2match, cg2attach, NULL, NULL);

dev_type_open(cg2open);
dev_type_ioctl(cg2ioctl);
dev_type_mmap(cg2mmap);

const struct cdevsw cgtwo_cdevsw = {
	.d_open = cg2open,
	.d_close = nullclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = cg2ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = cg2mmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

static int cg2gattr(struct fbdevice *,  void *);
static int cg2gvideo(struct fbdevice *, void *);
static int cg2svideo(struct fbdevice *, void *);
static int cg2getcmap(struct fbdevice *, void *);
static int cg2putcmap(struct fbdevice *, void *);

static struct fbdriver cg2fbdriver = {
	cg2open, nullclose, cg2mmap, nokqfilter, cg2gattr,
	cg2gvideo, cg2svideo,
	cg2getcmap, cg2putcmap };

static int cg2intr(void *);

/*
 * Match a cg2.
 */
static int 
cg2match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;
	int probe_addr;

	/* No default VME address. */
	if (ca->ca_paddr == -1)
		return 0;

	/* Make sure something is there... */
	probe_addr = ca->ca_paddr + CTLREGS_OFF;
	if (bus_peek(ca->ca_bustype, probe_addr, 1) == -1)
		return 0;

	/* XXX: look at the ID reg? */
	/* aprint_debug("cg2: id=0x%x\n", x); */

	/* Default interrupt priority. */
	if (ca->ca_intpri == -1)
		ca->ca_intpri = 4;

	return 1;
}

/*
 * Attach a display.  We need to notice if it is the console, too.
 */
static void 
cg2attach(device_t parent, device_t self, void *args)
{
	struct cg2_softc *sc = device_private(self);
	struct fbdevice *fb = &sc->sc_fb;
	struct confargs *ca = args;
	struct fbtype *fbt;

	sc->sc_dev = self;
	sc->sc_phys = ca->ca_paddr;
	sc->sc_pmtype = PMAP_NC | PMAP_VME16;

	sc->sc_ctlreg = (struct cg2fb *)bus_mapin(ca->ca_bustype,
	    ca->ca_paddr + CTLREGS_OFF, CTLREGS_SIZE);

	isr_add_vectored(cg2intr, sc, ca->ca_intpri, ca->ca_intvec);

	/*
	 * XXX - Initialize?  Determine type?
	 */
	sc->sc_ctlreg->intrptvec.reg = ca->ca_intvec;
	sc->sc_ctlreg->status.word = 1;

	fb->fb_driver = &cg2fbdriver;
	fb->fb_private = sc;
	fb->fb_name = device_xname(self);

	fbt = &fb->fb_fbtype;
	fbt->fb_type = FBTYPE_SUN2COLOR;
	fbt->fb_depth = 8;
	fbt->fb_cmsize = CMSIZE;

	fbt->fb_width = 1152;
	fbt->fb_height = 900;
	fbt->fb_size = CG2_MAPPED_SIZE;

	aprint_normal(" (%dx%d)\n", fbt->fb_width, fbt->fb_height);
	fb_attach(fb, 2);
}

int 
cg2open(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct cg2_softc *sc;
	int unit = minor(dev);

	sc = device_lookup_private(&cgtwo_cd, unit);
	if (sc == NULL)
		return ENXIO;
	return 0;
}

int 
cg2ioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct cg2_softc *sc = device_lookup_private(&cgtwo_cd, minor(dev));

	return fbioctlfb(&sc->sc_fb, cmd, data);
}

/*
 * Return the address that would map the given device at the given
 * offset, allowing for the given protection, or return -1 for error.
 */
paddr_t 
cg2mmap(dev_t dev, off_t off, int prot)
{
	struct cg2_softc *sc = device_lookup_private(&cgtwo_cd, minor(dev));

	if (off & PGOFSET)
		panic("%s: bad offset", __func__);

	if (off >= CG2_MAPPED_SIZE)
		return -1;

	/*
	 * I turned on PMAP_NC here to disable the cache as I was
	 * getting horribly broken behaviour with it on.
	 */
	return (sc->sc_phys + off) | sc->sc_pmtype;
}

/*
 * Internal ioctl functions.
 */

/* FBIOGATTR: */
static int 
cg2gattr(struct fbdevice *fb, void *data)
{
	struct fbgattr *fba = data;

	fba->real_type = fb->fb_fbtype.fb_type;
	fba->owner = 0;		/* XXX - TIOCCONS stuff? */
	fba->fbtype = fb->fb_fbtype;
	fba->sattr.flags = 0;
	fba->sattr.emu_type = fb->fb_fbtype.fb_type;
	fba->sattr.dev_specific[0] = -1;
	fba->emu_types[0] = fb->fb_fbtype.fb_type;
	fba->emu_types[1] = -1;
	return 0;
}

/* FBIOGVIDEO: */
static int 
cg2gvideo(struct fbdevice *fb, void *data)
{
	int *on = data;
	struct cg2_softc *sc = fb->fb_private;

	*on = sc->sc_ctlreg->status.reg.video_enab;
	return 0;
}

/* FBIOSVIDEO: */
static int 
cg2svideo(struct fbdevice *fb, void *data)
{
	int *on = data;
	struct cg2_softc *sc = fb->fb_private;

	sc->sc_ctlreg->status.reg.video_enab = (*on) & 1;

	return 0;
}

/* FBIOGETCMAP: */
static int 
cg2getcmap(struct fbdevice *fb, void *data)
{
	struct fbcmap *cmap = data;
	struct cg2_softc *sc = fb->fb_private;
	uint8_t red[CMSIZE], green[CMSIZE], blue[CMSIZE];
	int error, start, count, ecount;
	u_int i;
	uint16_t *p;

	start = cmap->index;
	count = cmap->count;
	ecount = start + count;
	if (start >= CMSIZE || count > CMSIZE - start)
		return EINVAL;

	/* XXX - Wait for retrace? */

	/* Copy hardware to local arrays. */
	p = &sc->sc_ctlreg->redmap[start];
	for (i = start; i < ecount; i++)
		red[i] = *p++;
	p = &sc->sc_ctlreg->greenmap[start];
	for (i = start; i < ecount; i++)
		green[i] = *p++;
	p = &sc->sc_ctlreg->bluemap[start];
	for (i = start; i < ecount; i++)
		blue[i] = *p++;

	/* Copy local arrays to user space. */
	if ((error = copyout(red + start, cmap->red, count)) != 0)
		return error;
	if ((error = copyout(green + start, cmap->green, count)) != 0)
		return error;
	if ((error = copyout(blue + start, cmap->blue, count)) != 0)
		return error;

	return 0;
}

/* FBIOPUTCMAP: */
static int 
cg2putcmap(struct fbdevice *fb, void *data)
{
	struct fbcmap *cmap = data;
	struct cg2_softc *sc = fb->fb_private;
	uint8_t red[CMSIZE], green[CMSIZE], blue[CMSIZE];
	int error;
	u_int start, count, ecount;
	u_int i;
	uint16_t *p;

	start = cmap->index;
	count = cmap->count;
	ecount = start + count;
	if (start >= CMSIZE || count > CMSIZE - start)
		return EINVAL;

	/* Copy from user space to local arrays. */
	if ((error = copyin(cmap->red, red + start, count)) != 0)
		return error;
	if ((error = copyin(cmap->green, green + start, count)) != 0)
		return error;
	if ((error = copyin(cmap->blue, blue + start, count)) != 0)
		return error;

	/* XXX - Wait for retrace? */

	/* Copy from local arrays to hardware. */
	p = &sc->sc_ctlreg->redmap[start];
	for (i = start; i < ecount; i++)
		*p++ = red[i];
	p = &sc->sc_ctlreg->greenmap[start];
	for (i = start; i < ecount; i++)
		*p++ = green[i];
	p = &sc->sc_ctlreg->bluemap[start];
	for (i = start; i < ecount; i++)
		*p++ = blue[i];

	return 0;

}

static int 
cg2intr(void *vsc)
{
	struct cg2_softc *sc = vsc;

	/* XXX - Just disable interrupts for now. */
	sc->sc_ctlreg->status.reg.inten = 0;

	printf("cg2intr\n");
	return 1;
}
