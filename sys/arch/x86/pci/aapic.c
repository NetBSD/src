/* 	$NetBSD: aapic.c,v 1.8 2009/11/25 15:06:26 njoly Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aapic.c,v 1.8 2009/11/25 15:06:26 njoly Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <arch/x86/pci/amd8131reg.h>

#include "ioapic.h"

#if NIOAPIC > 0
extern int nioapics;
#endif

static int	aapic_match(device_t, cfdata_t, void *);
static void	aapic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(aapic, 0, aapic_match, aapic_attach, NULL, NULL);

static int
aapic_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_AMD &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_AMD_PCIX8131_APIC)
		return (1);

	return (0);
}

static void
aapic_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	char devinfo[256];
	int bus, dev, func, rev;
	pcitag_t tag;
	pcireg_t reg;

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
	rev = PCI_REVISION(pa->pa_class);
	aprint_naive("\n");
	aprint_normal(": %s (rev. 0x%02x)\n", devinfo, rev);

#if NIOAPIC > 0
	if (nioapics == 0)
		return;
#else
	return;
#endif
	
	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, AMD8131_IOAPIC_CTL);
	reg |= AMD8131_IOAEN;
	pci_conf_write(pa->pa_pc, pa->pa_tag, AMD8131_IOAPIC_CTL, reg);

	pci_decompose_tag(pa->pa_pc, pa->pa_tag, &bus, &dev, &func);
	func = 0;
	tag = pci_make_tag(pa->pa_pc, bus, dev, func);
	reg = pci_conf_read(pa->pa_pc, tag, AMD8131_PCIX_MISC);
	reg &= ~AMD8131_NIOAMODE;
	pci_conf_write(pa->pa_pc, tag, AMD8131_PCIX_MISC, reg);
}
