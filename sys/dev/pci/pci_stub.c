#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_stub.c,v 1.1 2011/08/24 20:27:35 dyoung Exp $");

#include "opt_pci.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

int default_pci_bus_devorder(pci_chipset_tag_t, int, uint8_t *, int);
int default_pci_chipset_tag_create(pci_chipset_tag_t, uint64_t,
    const struct pci_overrides *, void *, pci_chipset_tag_t *);
void default_pci_chipset_tag_destroy(pci_chipset_tag_t);

__strict_weak_alias(pci_bus_devorder, default_pci_bus_devorder);
__strict_weak_alias(pci_chipset_tag_create, default_pci_chipset_tag_create);
__strict_weak_alias(pci_chipset_tag_destroy, default_pci_chipset_tag_destroy);

int
default_pci_bus_devorder(pci_chipset_tag_t pc, int bus, uint8_t *devs,
    int maxdevs)
{
	int i, n;

	n = MIN(pci_bus_maxdevs(pc, bus), maxdevs);
	for (i = 0; i < n; i++)
		devs[i] = i;

	return n;
}

void
default_pci_chipset_tag_destroy(pci_chipset_tag_t pc)
{
}

int
default_pci_chipset_tag_create(pci_chipset_tag_t opc, const uint64_t present,
    const struct pci_overrides *ov, void *ctx, pci_chipset_tag_t *pcp)
{
	return EOPNOTSUPP;
}
