#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/bhareg.h>
#include <dev/ic/bhavar.h>

#define	PCI_CBIO	0x10

int	bha_pci_match __P((struct device *, void *, void *));
void	bha_pci_attach __P((struct device *, struct device *, void *));

struct cfattach bha_pci_ca = {
	sizeof(struct bha_softc), bha_pci_match, bha_pci_attach
};

/*
 * Check the slots looking for a board we recognise
 * If we find one, note it's address (slot) and call
 * the actual probe routine to check it out.
 */
int
bha_pci_match(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct pci_attach_args *pa = aux;
	bus_chipset_tag_t bc = pa->pa_bc;
	bus_io_addr_t iobase;
	bus_io_size_t iosize;
	bus_io_handle_t ioh;
	pci_chipset_tag_t pc = pa->pa_pc;
	int rv;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_BUSLOGIC)
		return (0);

	if (PCI_PRODUCT(pa->pa_id) != PCI_PRODUCT_BUSLOGIC_OLD946C &&
	    PCI_PRODUCT(pa->pa_id) != PCI_PRODUCT_BUSLOGIC_946C)
		return (0);

	if (pci_io_find(pc, pa->pa_tag, PCI_CBIO, &iobase, &iosize))
		return (0);
	if (bus_io_map(bc, iobase, iosize, &ioh))
		return (0);

	rv = bha_find(bc, ioh, NULL);

	bus_io_unmap(bc, ioh, iosize);

	return (rv);
}

/*
 * Attach all the sub-devices we can find
 */
void
bha_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	struct bha_softc *sc = (void *)self;
	bus_chipset_tag_t bc = pa->pa_bc;
	bus_io_addr_t iobase;
	bus_io_size_t iosize;
	bus_io_handle_t ioh;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	pcireg_t csr;
	const char *model, *intrstr;

	if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_BUSLOGIC_OLD946C)
		model = "BusLogic 9xxC SCSI";
	else if (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_BUSLOGIC_946C)
		model = "BusLogic 9xxC SCSI";
	else
		model = "unknown model!";
	printf(": %s\n", model);

	if (pci_io_find(pc, pa->pa_tag, PCI_CBIO, &iobase, &iosize))
		panic("bha_attach: pci_io_find failed!");
	if (bus_io_map(bc, iobase, iosize, &ioh))
		panic("bha_attach: bus_io_map failed!");

	sc->sc_bc = bc;
	sc->sc_ioh = ioh;
	if (!bha_find(bc, ioh, sc))
		panic("bha_attach: bha_find failed!");

	csr = pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    csr | PCI_COMMAND_MASTER_ENABLE | PCI_COMMAND_IO_ENABLE);

	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_BIO, bha_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	bha_attach(sc);
}
