#include <sys/param.h>
#include <sys/device.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <arm/footbridge/dc21285reg.h>

void
netwinder_pci_attach_hook (struct device *parent,
	struct device *self, struct pcibus_attach_args *pba)
{
	pcireg_t regval;
	pcireg_t intreg;
	pcitag_t tag;

	/*
	 * Initialize the TULIP
	 */
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 9, 0);
	pci_conf_write(pba->pba_pc, tag,
		PCI_COMMAND_STATUS_REG,
		PCI_COMMAND_IO_ENABLE|
		PCI_COMMAND_MASTER_ENABLE);
	intreg = pci_conf_read(pba->pba_pc, tag, PCI_INTERRUPT_REG);
	intreg = PCI_INTERRUPT_CODE(
		PCI_INTERRUPT_LATENCY(intreg),
		PCI_INTERRUPT_GRANT(intreg),
		PCI_INTERRUPT_PIN(intreg),
		0x40|IRQ_IN_L1);
	pci_conf_write(pba->pba_pc, tag, PCI_INTERRUPT_REG, intreg);
	pci_conf_write(pba->pba_pc, tag, 0x10, 0x400 | PCI_MAPREG_TYPE_IO);
	pci_conf_write(pba->pba_pc, tag, 0x14, 0);

	/*
	 * Initialize the PCI NE2000
	 */
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 12, 0);
	pci_conf_write(pba->pba_pc, tag,
				PCI_COMMAND_STATUS_REG,
				PCI_COMMAND_IO_ENABLE|
				PCI_COMMAND_MASTER_ENABLE);
	intreg = pci_conf_read(pba->pba_pc, tag, PCI_INTERRUPT_REG);
	intreg = PCI_INTERRUPT_CODE(
		PCI_INTERRUPT_LATENCY(intreg),
		PCI_INTERRUPT_GRANT(intreg),
		PCI_INTERRUPT_PIN(intreg),
		0x40|IRQ_IN_L0);
	pci_conf_write(pba->pba_pc, tag, PCI_INTERRUPT_REG, intreg);
	pci_conf_write(pba->pba_pc, tag, 0x10, 0x300 | PCI_MAPREG_TYPE_IO);

#if 0
	/*
	 * Initialize the PCI-ISA Bridge
	 */
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 11, 0);
	pci_conf_write(pba->pba_pc, tag,
		PCI_COMMAND_STATUS_REG,
		PCI_COMMAND_IO_ENABLE|
		PCI_COMMAND_MEM_ENABLE|
		PCI_COMMAND_MASTER_ENABLE);
	pci_conf_write(pba->pba_pc, tag, 0x10, 0);
	pci_conf_write(pba->pba_pc, tag, 0x48,
		pci_conf_read(pba->pba_pc, tag, 0x48)|0xff);

	regval = pci_conf_read(pba->pba_pc, tag, 0x40);
	regval &= 0xff00ff00;
	regval |= 0x00000022;
	pci_conf_write(pba->pba_pc, tag, 0x40, regval);

	regval = pci_conf_read(pba->pba_pc, tag, 0x80);
	regval &= 0x0000ff00;
	regval |= 0xe0010002;
	pci_conf_write(pba->pba_pc, tag, 0x80, regval);
#endif

	/*
	 * Initialize the PCIIDE Controller
	 */
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 11, 1);
	pci_conf_write(pba->pba_pc, tag,
		PCI_COMMAND_STATUS_REG,
		PCI_COMMAND_IO_ENABLE|
		PCI_COMMAND_MASTER_ENABLE);

	regval = pci_conf_read(pba->pba_pc, tag, PCI_CLASS_REG);
	regval = PCI_CLASS_CODE(PCI_CLASS(regval), PCI_SUBCLASS(regval), 0x8A);
	pci_conf_write(pba->pba_pc, tag, PCI_CLASS_REG, regval);

	regval = pci_conf_read(pba->pba_pc, tag, 0x40);
	regval &= ~0x10;	/* disable secondary port */
	pci_conf_write(pba->pba_pc, tag, 0x40, regval);

	pci_conf_write(pba->pba_pc, tag, 0x10, 0x01f0 | PCI_MAPREG_TYPE_IO);
	pci_conf_write(pba->pba_pc, tag, 0x14, 0x03f4 | PCI_MAPREG_TYPE_IO);
	pci_conf_write(pba->pba_pc, tag, 0x18, 0x0170 | PCI_MAPREG_TYPE_IO);
	pci_conf_write(pba->pba_pc, tag, 0x1c, 0x0374 | PCI_MAPREG_TYPE_IO);
	pci_conf_write(pba->pba_pc, tag, 0x20, 0xe800 | PCI_MAPREG_TYPE_IO);
	intreg = pci_conf_read(pba->pba_pc, tag, PCI_INTERRUPT_REG);
	intreg = PCI_INTERRUPT_CODE(
		PCI_INTERRUPT_LATENCY(intreg),
		PCI_INTERRUPT_GRANT(intreg),
		PCI_INTERRUPT_PIN(intreg),
		0x8e);
	pci_conf_write(pba->pba_pc, tag, PCI_INTERRUPT_REG, intreg);

	/*
	 * Make sure we are in legacy mode
	 */
	regval = pci_conf_read(pba->pba_pc, tag, 0x40);
	regval &= ~0x800;
	pci_conf_write(pba->pba_pc, tag, 0x40, regval);
}
