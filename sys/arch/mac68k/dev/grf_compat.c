/*	$NetBSD: grf_compat.c,v 1.1.2.5 1999/11/20 08:35:20 scottr Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cdefs.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/grfioctl.h>

#include <mac68k/nubus/nubus.h>
#include <mac68k/dev/grfvar.h>
#include <mac68k/dev/macfbvar.h>

#include <miscfs/specfs/specdev.h>

#include <vm/vm.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>

cdev_decl(grf);

void	grf_scinit __P((struct grf_softc *, const char *, int));
void	grf_init __P((int));
void	grfattach __P((int));
int	grfmap __P((dev_t, struct macfb_softc *, caddr_t *, struct proc *));
int	grfunmap __P((dev_t, struct macfb_softc *, caddr_t, struct proc *));

/* Non-private for the benefit of libkvm. */
struct	grf_softc *grf_softc;
int	numgrf = 0;

/*
 * Initialize a softc to sane defaults.
 */
void
grf_scinit(sc, name, unit)
	struct grf_softc *sc;
	const char *name;
	int unit;
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
grf_init(n)
	int n;
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
grfattach(n)
	int n;
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
grf_attach(sc, unit)
	struct macfb_softc *sc;
	int unit;
{
	grf_init(unit);

	if (unit < numgrf)
		grf_softc[unit].mfb_sc = sc;
}

/*
 * Standard device ops
 */
int
grfopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
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
grfclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
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
grfread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	return ENXIO;
}

int
grfwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	return ENXIO;
}

int
grfioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct grf_softc *sc;
	struct macfb_devconfig *dc;
	struct grfinfo *gd;
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
	case GRFIOCGINFO:
		gd = (struct grfinfo *)data;
		memset(gd, 0, sizeof(struct grfinfo));
		gd->gd_fbaddr     = (caddr_t)dc->dc_paddr;
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

	case GRFIOCON:
	case GRFIOCOFF:
		/* Nothing to do */
		rv = 0;
		break;

	case GRFIOCMAP:
		rv = grfmap(dev, sc->mfb_sc, (caddr_t *)data, p);
		break;

	case GRFIOCUNMAP:
		rv = grfunmap(dev, sc->mfb_sc, *(caddr_t *)data, p);
		break;

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

int
grfpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	return EINVAL;
}

int
grfmmap(dev, off, prot)
	dev_t dev;
	int off;
	int prot;
{
	struct grf_softc *sc;
	struct macfb_devconfig *dc;
	u_long addr;
	int unit = GRFUNIT(dev);

	if (grf_softc == NULL || unit >= numgrf)
		return ENXIO;

	sc = &grf_softc[unit];
	if (sc->mfb_sc == NULL)
		return ENXIO;

	dc = sc->mfb_sc->sc_dc;

	if (off >= 0 &&
	    off < m68k_round_page(dc->dc_offset + dc->dc_size))
		addr = m68k_btop(dc->dc_paddr + off);
	else
		addr = (-1);	/* XXX bogus */

	return (int)addr;
}

int
grfmap(dev, sc, addrp, p)
	dev_t dev;
	struct macfb_softc *sc;
	caddr_t *addrp;
	struct proc *p;
{
	struct specinfo si;
	struct vnode vn;
	u_long len;
	int error, flags;

	*addrp = (caddr_t)sc->sc_dc->dc_paddr;
	len = m68k_round_page(sc->sc_dc->dc_offset + sc->sc_dc->dc_size);
	flags = MAP_SHARED | MAP_FIXED;

	vn.v_type = VCHR;		/* XXX */
	vn.v_specinfo = &si;		/* XXX */
	vn.v_rdev = dev;		/* XXX */

	error = uvm_mmap(&p->p_vmspace->vm_map, (vaddr_t *)addrp,
	    (vsize_t)len, VM_PROT_ALL, VM_PROT_ALL,
	    flags, (caddr_t)&vn, 0, p->p_rlimit[RLIMIT_MEMLOCK].rlim_cur);

	/* Offset into page: */
	*addrp += sc->sc_dc->dc_offset;

	return (error);
}

int
grfunmap(dev, sc, addr, p)
	dev_t dev;
	struct macfb_softc *sc;
	caddr_t addr;
	struct proc *p;
{
	vm_size_t size;
	int     rv;

	addr -= sc->sc_dc->dc_offset;

	if (addr <= 0)
		return (-1);

	size = m68k_round_page(sc->sc_dc->dc_offset + sc->sc_dc->dc_size);

	rv = uvm_unmap(&p->p_vmspace->vm_map, (vaddr_t)addr,
	    (vaddr_t)addr + size);

	return (rv == KERN_SUCCESS ? 0 : EINVAL);
}
