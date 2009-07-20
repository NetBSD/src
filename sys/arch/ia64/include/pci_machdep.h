typedef int pci_chipset_tag_t;
typedef int pci_intr_handle_t;
typedef int pcitag_t;

pcireg_t pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg);
void pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t val);

pcitag_t pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function);
