/*	$NetBSD: grf_compat.c,v 1.26.4.1 2015/04/06 15:17:58 skrll Exp $	*/

/*
 * Copyright (C) 1999 Scott Reynolds
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
 *    derived from this software without specific prior written permission.
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
 * macfb compatibility with legacy grf devices
 */

#include "opt_grf_compat.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf_compat.c,v 1.26.4.1 2015/04/06 15:17:58 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>

#include <machine/bus.h>
#include <machine/grfioctl.h>

#include <mac68k/nubus/nubus.h>
#include <mac68k/dev/grfvar.h>
#include <mac68k/dev/macfbvar.h>

#include <miscfs/specfs/specdev.h>

#include <uvm/uvm_extern.h>

dev_type_open(grfopen);
dev_type_close(grfclose);
dev_type_ioctl(grfioctl);
dev_type_mmap(grfmmap);

const struct cdevsw grf_cdevsw = {
	.d_open = grfopen,
	.d_close = grfclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = grfioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = grfmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

void	grf_scinit(struct grf_softc *, const char *, int);
void	grf_init(int);
void	grfattach(int);
int	grfmap(dev_t, struct macfb_softc *, void **, struct proc *);
int	grfunmap(dev_t, struct macfb_softc *, void *, struct proc *);

/* Non-private for the benefit of libkvm. */
struct	grf_softc *grf_softc;
int	numgrf = 0;

/*
 * Initialize a softc to sane defaults.
 */
void
grf_scinit(struct grf_softc *sc, const char *name, int unit)
{ 
	memset(sc, 0, sizeof(struct grf_softc));
	snprintf(sc->sc_xname, sizeof(sc->sc_xname), "%s%d", name, unit);
	sc->mfb_sc = NULL;
}

/*
 * (Re-)initialize the grf_softc block so that at least the requested
 * number of elements has been allocated.  If this results in more
 * elements than we had prior to getting here, we initialize each of
 * them to avoid problems down the road.
 */
void
grf_init(int n)
{
	struct grf_softc *sc;
	int i;

	if (n >= numgrf) {
		i = numgrf;
		numgrf = n + 1;

		if (grf_softc == NULL)
			sc = (struct grf_softc *)
			    malloc(numgrf * sizeof(*sc),
			    M_DEVBUF, M_NOWAIT);
		else
			sc = (struct grf_softc *)
			    realloc(grf_softc, numgrf * sizeof(*sc),
			    M_DEVBUF, M_NOWAIT);
		if (sc == NULL) {
			printf("WARNING: no memory for grf emulation\n");
			if (grf_softc != NULL)
				free(grf_softc, M_DEVBUF);
			return;
		}
		grf_softc = sc;

		/* Initialize per-softc structures. */
		while (i < numgrf) {
			grf_scinit(&grf_softc[i], "grf", i);
			i++;
		}
	}
}

/*
 * Called by main() during pseudo-device attachment.  If we had a
 * way to configure additional grf devices later, this would actually
 * allocate enough space for them.  As it stands, it's nonsensical,
 * so other than a basic sanity check we do nothing.
 */
void
grfattach(int n)
{
	if (n <= 0) {
#ifdef DIAGNOSTIC
		panic("grfattach: count <= 0");
#endif
		return;
	}

#if 0 /* XXX someday, if we implement a way to attach after autoconfig */
	grf_init(n);
#endif
}

/*
 * Called from macfb_attach() after setting up the frame buffer.  Since
 * there is a 1:1 correspondence between the macfb device and the grf
 * device, the only bit of information we really need is the macfb_softc.
 */
void
grf_attach(struct macfb_softc *sc, int unit)
{

	grf_init(unit);
	if (unit < numgrf)
		grf_softc[unit].mfb_sc = sc;
}

/*
 * Standard device ops
 */
int
grfopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct grf_softc *sc;
	int unit = GRFUNIT(dev);
	int rv = 0;

	if (grf_softc == NULL || unit >= numgrf)
		return ENXIO;

	sc = &grf_softc[unit];
	if (sc->mfb_sc == NULL)
		rv = ENXIO;

	return rv;
}

int
grfclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct grf_softc *sc;
	int unit = GRFUNIT(dev);
	int rv = 0;

	if (grf_softc == NULL || unit >= numgrf)
		return ENXIO;

	sc = &grf_softc[unit];
	if (sc->mfb_sc != NULL)
		macfb_clear(sc->mfb_sc->sc_dc);	/* clear the display */
	else
		rv = ENXIO;

	return rv;
}

int
grfioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct grf_softc *sc;
	struct macfb_devconfig *dc;
#if defined(GRF_COMPAT) || (NGRF > 0)
	struct grfinfo *gd;
#endif /* GRF_COMPAT || (NGRF > 0) */
	struct grfmode *gm;
	int unit = GRFUNIT(dev);
	int rv;

	if (grf_softc == NULL || unit >= numgrf)
		return ENXIO;

	sc = &grf_softc[unit];
	if (sc->mfb_sc == NULL)
		return ENXIO;

	dc = sc->mfb_sc->sc_dc;

	switch (cmd) {
#if defined(GRF_COMPAT) || (NGRF > 0)
	case GRFIOCGINFO:
		gd = (struct grfinfo *)data;
		memset(gd, 0, sizeof(struct grfinfo));
		gd->gd_fbaddr     = (void *)dc->dc_paddr;
		gd->gd_fbsize     = dc->dc_size;
		gd->gd_colors     = (short)(1 << dc->dc_depth);
		gd->gd_planes     = (short)dc->dc_depth;
		gd->gd_fbwidth    = dc->dc_wid;
		gd->gd_fbheight   = dc->dc_ht;
		gd->gd_fbrowbytes = dc->dc_rowbytes;
		gd->gd_dwidth     = dc->dc_raster.width;
		gd->gd_dheight    = dc->dc_raster.height;
		rv = 0;
		break;
#endif /* GRF_COMPAT || (NGRF > 0) */

	case GRFIOCON:
	case GRFIOCOFF:
		/* Nothing to do */
		rv = 0;
		break;

#if defined(GRF_COMPAT) || (NGRF > 0)
	case GRFIOCMAP:
		rv = grfmap(dev, sc->mfb_sc, (void **)data, l->l_proc);
		break;

	case GRFIOCUNMAP:
		rv = grfunmap(dev, sc->mfb_sc, *(void **)data, l->l_proc);
		break;
#endif /* GRF_COMPAT || (NGRF > 0) */

	case GRFIOCGMODE:
		gm = (struct grfmode *)data;
		memset(gm, 0, sizeof(struct grfmode));
		gm->fbbase   = (char *)dc->dc_vaddr;
		gm->fbsize   = dc->dc_size;
		gm->fboff    = dc->dc_offset;
		gm->rowbytes = dc->dc_rowbytes;
		gm->width    = dc->dc_wid;
		gm->height   = dc->dc_ht;
		gm->psize    = dc->dc_depth;
		rv = 0;
		break;

	case GRFIOCLISTMODES:
	case GRFIOCGETMODE:
	case GRFIOCSETMODE:
		/* NONE of these operations are (officially) supported. */
	default:
		rv = EINVAL;
		break;
	}
	return rv;
}

paddr_t
grfmmap(dev_t dev, off_t off, int prot)
{
	struct grf_softc *sc;
	struct macfb_devconfig *dc;
	int unit = GRFUNIT(dev);

	if (grf_softc == NULL || unit >= numgrf)
		return ENXIO;

	sc = &grf_softc[unit];
	if (sc->mfb_sc == NULL)
		return ENXIO;

	dc = sc->mfb_sc->sc_dc;

	if ((u_int)off < m68k_round_page(dc->dc_offset + dc->dc_size))
		return m68k_btop(dc->dc_paddr + off);

	return (-1);
}

int
grfmap(dev_t dev, struct macfb_softc *sc, void **addrp, struct proc *p)
{
	size_t len;
	int error;

	len = m68k_round_page(sc->sc_dc->dc_offset + sc->sc_dc->dc_size);
	error = uvm_mmap_dev(p, addrp, len, dev, 0);

	/* Offset into page: */
	*addrp = (char *)*addrp + sc->sc_dc->dc_offset;

	return (error);
}

int
grfunmap(dev_t dev, struct macfb_softc *sc, void *addr, struct proc *p)
{
	size_t size;

	addr = (char *)addr - sc->sc_dc->dc_offset;

	if (addr <= 0)
		return (-1);

	size = m68k_round_page(sc->sc_dc->dc_offset + sc->sc_dc->dc_size);
	uvm_unmap(&p->p_vmspace->vm_map, (vaddr_t)addr, (vaddr_t)addr + size);
	return 0;
}
