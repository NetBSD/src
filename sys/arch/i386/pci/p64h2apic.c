/*
 * Driver for P64H2 IOxAPIC
 *
 * This is an ioapic which appears in PCI space.
 * This driver currently does nothing useful but will eventually
 * thwak the ioapic into working correctly.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

static int	p64h2match __P((struct device *, struct cfdata *, void *));
static void	p64h2attach __P((struct device *, struct device *, void *));

struct p64h2apic_softc
{
	pcitag_t sc_tag;
};

struct cfattach p64h2apic_ca = {
	sizeof(struct p64h2apic_softc), p64h2match, p64h2attach
};

int	p64h2print __P((void *, const char *pnp));


static int
p64h2match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_INTEL &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_INTEL_82870P2_IOxAPIC)
		return (1);

	return (0);
}

static void
p64h2attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct p64h2apic_softc *sc = (void *) self;
	struct pci_attach_args *pa = aux;
	char devinfo[256];

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

	sc->sc_tag = pa->pa_tag;
}
