#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

static void obio_attach __P((struct device *, struct device *, void *));
static int obio_match __P((struct device *, struct cfdata *, void *));
static int obio_print __P((void *, const char *));

struct obio_softc {
	struct device sc_dev;
	int sc_node;
};


struct cfattach obio_ca = {
	sizeof(struct obio_softc), obio_match, obio_attach
};

int
obio_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE &&
	    PCI_PRODUCT(pa->pa_id) == 2)
		return 1;

	return 0;
}

/*
 * Attach all the sub-devices we can find
 */
void
obio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct obio_softc *sc = (struct obio_softc *)self;
	struct confargs ca;
	int node, child, namelen;
	u_int reg[20];
	int intr[5];
	char name[32];

	node = OF_finddevice("/bandit/gc");		/* XXX */
	sc->sc_node = node;

	if (OF_getprop(node, "assigned-addresses", reg, sizeof(reg)) < 12)
		return;
	ca.ca_baseaddr = reg[2];

	printf(": addr 0x%x\n", ca.ca_baseaddr);

	for (child = OF_child(node); child; child = OF_peer(child)) {
		namelen = OF_getprop(child, "name", name, sizeof(name));
		if (namelen < 0)
			continue;
		if (namelen >= sizeof(name))
			continue;

		name[namelen] = 0;
		ca.ca_name = name;
		ca.ca_node = child;

		ca.ca_nreg  = OF_getprop(child, "reg", reg, sizeof(reg));
		ca.ca_nintr = OF_getprop(child, "AAPL,interrupts", intr,
				sizeof(intr));
		ca.ca_reg = reg;
		ca.ca_intr = intr;

		config_found(self, &ca, obio_print);
	}
}

int
obio_print(aux, obio)
	void *aux;
	const char *obio;
{
	struct confargs *ca = aux;

	if (obio)
		printf("%s at %s", ca->ca_name, obio);

	if (ca->ca_nreg > 0)
		printf(" offset 0x%x", ca->ca_reg[0]);

	return UNCONF;
}
