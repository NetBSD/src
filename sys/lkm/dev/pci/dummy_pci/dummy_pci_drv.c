/* $NetBSD: dummy_pci_drv.c,v 1.1.2.3 2004/09/18 14:54:09 skrll Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>

struct mist_softc {
	struct device sc_dev;
};

static int mist_match(struct device *, struct cfdata *, void *);
static void mist_attach(struct device *, struct device *, void *);
static int mist_detach(struct device*, int);

CFATTACH_DECL(mist, sizeof(struct mist_softc),
	      mist_match, mist_attach, mist_detach, NULL);

int
mist_match(struct device *parent, struct cfdata *match, void *aux)
{

	return (1);
}

static void
mist_attach(struct device *parent, struct device *self, void *aux)
{

	printf(": dummy catch-all PCI device\n");
}

static int
mist_detach(struct device* self, int flags)
{

	return (0);
}
