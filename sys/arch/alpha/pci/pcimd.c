/* $NetBSD: pcimd.c,v 1.1.2.3 1997/06/01 04:13:45 cgd Exp $ */

/*
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
 *
 * THIS SOURCE CODE AND ANY OBJECT CODE DERIVED FROM IT MAY NOT BE
 * MODIFIED OR DISTRIBUTED TO ANYONE, FOR ANY REASON, WITHOUT EXPRESS 
 * WRITTEN PERMISSION OF CHRISTOPHER G. DEMETRIOU.
 *
 * (XXX COPYRIGHT NOTICE)
 */

/* XXX ID AND COPYRIGHT */

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
		/* NONE of these operations are supported by this driver. */
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
