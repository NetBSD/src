/*	$NetBSD: pcimd.c,v 1.1.2.2 1996/12/08 00:31:34 cgd Exp $	*/

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/pte.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <machine/wsconsio.h>
#include <alpha/wscons/wsdisplayvar.h>

int	pcimd_match __P((struct device *, struct cfdata *, void *));
void	pcimd_attach __P((struct device *, struct device *, void *));

struct cfattach pcimd_ca = {
	sizeof (struct device), pcimd_match, pcimd_attach,
};

struct cfdriver pcimd_cd = {
	NULL, "pcimd", DV_DULL,
};

static int	pcimd_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static int	pcimd_mmap __P((void *, off_t, int));

struct wsdisplay_accessops pcimd_accessops = {
	pcimd_ioctl,
	pcimd_mmap,
};      

int
pcimd_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	int potential;

	potential = 0;

	/*
	 * If it's display/miscellaneous, we might match.
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_DISPLAY &&
	     PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_DISPLAY_MISC)
		potential = 1;

	if (!potential)
		return (0);

	/* XXX TRY MAPPING MEMORY */

	return (1);
}

void
pcimd_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pap = aux;
	struct wsdisplaydev_attach_args wa;
	char devinfo[256];

        pci_devinfo(pap->pa_id, pap->pa_class, 0, devinfo);
        printf(": %s (rev. 0x%02x)\n", devinfo,
            PCI_REVISION(pap->pa_class));

	wa.accessops = &pcimd_accessops;
	wa.accesscookie = self;

	config_found(self, &wa, wsdisplaydevprint);
}

static int
pcimd_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(int *)data = WSDISPLAY_TYPE_PCIMISC;
		return 0;

	case WSDISPLAYIO_GINFO:
	case WSDISPLAYIO_GETCMAP:
	case WSDISPLAYIO_PUTCMAP:
	case WSDISPLAYIO_GVIDEO:
	case WSDISPLAYIO_SVIDEO:
	case WSDISPLAYIO_GCURPOS:
	case WSDISPLAYIO_SCURPOS:
	case WSDISPLAYIO_GCURMAX:
	case WSDISPLAYIO_GCURSOR:
	case WSDISPLAYIO_SCURSOR:
		/* NONE of these operations are by the generic VGA driver. */
		return ENOTTY;
	}
	return -1;
}

static int
pcimd_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{

	/* XXX */
	return -1;
}
