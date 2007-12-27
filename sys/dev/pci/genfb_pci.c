/*	$NetBSD: genfb_pci.c,v 1.1.24.2 2007/12/27 00:45:15 mjf Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: genfb_pci.c,v 1.1.24.2 2007/12/27 00:45:15 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/kauth.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciio.h>

#include <dev/wsfb/genfbvar.h>

#include "opt_wsfb.h"
#include "opt_genfb.h"

#ifdef GENFB_PCI_DEBUG
# define DPRINTF printf
#else
# define DPRINTF while (0) printf
#endif

struct range {
	bus_addr_t offset;
	bus_size_t size;
	int flags;
};

struct pci_genfb_softc {
	struct genfb_softc sc_gen;

	pci_chipset_tag_t sc_pc;
	pcitag_t sc_pcitag;
	bus_space_tag_t sc_memt;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_memh;
	pcireg_t sc_bars[9];
	struct range sc_ranges[8];
	int sc_ranges_used;
	int sc_want_wsfb;
};

static int	pci_genfb_match(struct device *, struct cfdata *, void *);
static void	pci_genfb_attach(struct device *, struct device *, void *);
static int	pci_genfb_ioctl(void *, void *, u_long, void *, int,
		    struct lwp *);
static paddr_t	pci_genfb_mmap(void *, void *, off_t, int);
static int	pci_genfb_drm_print(void *, const char *);


CFATTACH_DECL(genfb_pci, sizeof(struct pci_genfb_softc),
    pci_genfb_match, pci_genfb_attach, NULL, NULL);

static int
pci_genfb_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_CONTROL)
		return 1;

	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY)
		return 1;

	return 0;
}

static void
pci_genfb_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_genfb_softc *sc = (struct pci_genfb_softc *)self;
	struct pci_attach_args *pa = aux;
	struct genfb_ops ops;
	int idx, bar, type;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	printf(": %s\n", devinfo);

	sc->sc_memt = pa->pa_memt;
	sc->sc_iot = pa->pa_iot;	
	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;
	sc->sc_want_wsfb = 0;

	genfb_init(&sc->sc_gen);

	if ((sc->sc_gen.sc_width == 0) || (sc->sc_gen.sc_fbsize == 0)) {
		aprint_error("%s: bogus parameters, unable to continue\n", 
		    device_xname(self));
		return;
	}

	if (bus_space_map(sc->sc_memt, sc->sc_gen.sc_fboffset,
	    sc->sc_gen.sc_fbsize, BUS_SPACE_MAP_LINEAR, &sc->sc_memh) != 0) {

		panic("%s: unable to map the framebuffer\n", self->dv_xname);
	}
	sc->sc_gen.sc_fbaddr = bus_space_vaddr(sc->sc_memt, sc->sc_memh);

	/* mmap()able bus ranges */
	idx = 0;
	bar = 0x10;
	while (bar < 0x30) {

		type = pci_mapreg_type(sc->sc_pc, sc->sc_pcitag, bar);
		if ((type == PCI_MAPREG_TYPE_MEM) || 
		    (type == PCI_MAPREG_TYPE_ROM)) {

			pci_mapreg_info(sc->sc_pc, sc->sc_pcitag, bar, type,
			    &sc->sc_ranges[idx].offset,
			    &sc->sc_ranges[idx].size,
			    &sc->sc_ranges[idx].flags);
			idx++;
		}
		sc->sc_bars[(bar - 0x10) >> 2] =
		    pci_conf_read(sc->sc_pc, sc->sc_pcitag, bar);
		bar += 4;
	}
	sc->sc_ranges_used = idx;			    

	ops.genfb_ioctl = pci_genfb_ioctl;
	ops.genfb_mmap = pci_genfb_mmap;

	genfb_attach(&sc->sc_gen, &ops);

	/* now try to attach a DRM */
	config_found_ia(self, "drm", aux, pci_genfb_drm_print);	
}

static int
pci_genfb_drm_print(void *aux, const char *pnp)
{
	if (pnp)
		aprint_normal("direct rendering for %s", pnp);
	return (UNSUPP);
}


static int
pci_genfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{
	struct pci_genfb_softc *sc = v;

	switch (cmd) {
		case WSDISPLAYIO_GTYPE:
			*(u_int *)data = WSDISPLAY_TYPE_PCIMISC;
			return 0;

		/* PCI config read/write passthrough. */
		case PCI_IOC_CFGREAD:
		case PCI_IOC_CFGWRITE:
			return (pci_devioctl(sc->sc_pc, sc->sc_pcitag,
			    cmd, data, flag, l));
		case WSDISPLAYIO_SMODE:
			{
				int new_mode = *(int*)data, i;
				if (new_mode == WSDISPLAYIO_MODE_EMUL) {
					for (i = 0; i < 9; i++)
						pci_conf_write(sc->sc_pc,
						     sc->sc_pcitag,
						     0x10 + (i << 2),
						     sc->sc_bars[i]);
				}
			}
			return 0;
	}

	return EPASSTHROUGH;
}

static paddr_t
pci_genfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct pci_genfb_softc *sc = v;
	struct range *r;
	struct lwp *me;
	int i;

	if (offset == 0)
		sc->sc_want_wsfb = 1;

	/*
	 * regular fb mapping at 0
	 * since some Sun firmware likes to put PCI resources low enough
	 * to collide with the wsfb mapping we only allow it after asking
	 * for offset 0
	 */
	DPRINTF("%s: %08x limit %08x\n", __func__, (uint32_t)offset,
	    (uint32_t)sc->sc_gen.sc_fbsize);
	if ((offset >= 0) && (offset < sc->sc_gen.sc_fbsize) &&
	    (sc->sc_want_wsfb == 1)) {

		return bus_space_mmap(sc->sc_memt, sc->sc_gen.sc_fboffset,
		   offset, prot, BUS_SPACE_MAP_LINEAR);
	}

	/*
	 * restrict all other mappings to processes with superuser privileges
	 * or the kernel itself
	 */
	me = curlwp;
	if (me != NULL) {
		if (kauth_authorize_generic(me->l_cred, KAUTH_GENERIC_ISSUSER,
		    NULL) != 0) {
			aprint_normal("%s: mmap() rejected.\n",
			    sc->sc_gen.sc_dev.dv_xname);
			return -1;
		}
	}

#ifdef WSFB_FAKE_VGA_FB
	if ((offset >= 0xa0000) && (offset < 0xbffff)) {

		return bus_space_mmap(sc->sc_memt, sc->sc_gen.sc_fboffset,
		   offset - 0xa0000, prot, BUS_SPACE_MAP_LINEAR);
	}
#endif

	/*
	 * XXX this should be generalized, let's just
	 * #define PCI_IOAREA_PADDR
	 * #define PCI_IOAREA_OFFSET
	 * #define PCI_IOAREA_SIZE
	 * somewhere in a MD header and compile this code only if all are
	 * present
	 */
#ifdef macppc
	/* allow to map our IO space */
	if ((offset >= 0xf2000000) && (offset < 0xf2800000)) {
		return bus_space_mmap(sc->sc_iot, offset-0xf2000000, 0, prot, 
		    BUS_SPACE_MAP_LINEAR);	
	}
#endif

	/* allow to mmap() our BARs */
	/* maybe the ROM BAR too? */
	for (i = 0; i < sc->sc_ranges_used; i++) {

		r = &sc->sc_ranges[i];
		if ((offset >= r->offset) && (offset < (r->offset + r->size))) {
			return bus_space_mmap(sc->sc_memt, offset, 0, prot,
			    r->flags);
		}
	}

	return -1;
}
