/* $NetBSD: dummy_pci_drv.c,v 1.3 2008/07/09 13:15:40 joerg Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>

struct mist_softc {
};

static int mist_match(device_t, cfdata_t, void *);
static void mist_attach(device_t, device_t, void *);
static int mist_detach(device_t, int);

CFATTACH_DECL_NEW(mist, sizeof(struct mist_softc),
    mist_match, mist_attach, mist_detach, NULL);

int
mist_match(device_t parent, cfdata_t match, void *aux)
{

	return (1);
}

static void
mist_attach(device_t parent, device_t self, void *aux)
{

	aprint_naive(": dummy catch-all PCI device\n");
	aprint_normal(": dummy catch-all PCI device\n");
}

static int
mist_detach(device_t self, int flags)
{

	return (0);
}
