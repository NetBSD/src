#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_stub.c,v 1.7.8.1 2018/06/25 07:25:52 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_pci.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

int default_pci_bus_devorder(pci_chipset_tag_t, int, uint8_t *, int);
int default_pci_chipset_tag_create(pci_chipset_tag_t, uint64_t,
    const struct pci_overrides *, void *, pci_chipset_tag_t *);
void default_pci_chipset_tag_destroy(pci_chipset_tag_t);
void *default_pci_intr_establish_xname(pci_chipset_tag_t, pci_intr_handle_t,
    int, int (*)(void *), void *, const char *);

__strict_weak_alias(pci_bus_devorder, default_pci_bus_devorder);
__strict_weak_alias(pci_chipset_tag_create, default_pci_chipset_tag_create);
__strict_weak_alias(pci_chipset_tag_destroy, default_pci_chipset_tag_destroy);
__strict_weak_alias(pci_intr_establish_xname,
    default_pci_intr_establish_xname);

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

void *
default_pci_intr_establish_xname(pci_chipset_tag_t pc, pci_intr_handle_t ih,
    int level, int (*func)(void *), void *arg, const char *__nouse)
{

	return pci_intr_establish(pc, ih, level, func, arg);
}

#ifndef __HAVE_PCI_MSI_MSIX
pci_intr_type_t
pci_intr_type(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{

	return PCI_INTR_TYPE_INTX;
}

int
pci_intr_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int *counts, pci_intr_type_t max_type)
{

	if (counts != NULL && counts[PCI_INTR_TYPE_INTX] == 0)
		return EINVAL;

	return pci_intx_alloc(pa, ihps);
}

void
pci_intr_release(pci_chipset_tag_t pc, pci_intr_handle_t *pih, int count)
{

	kmem_free(pih, sizeof(*pih));
}

int
pci_intx_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihp)
{
	pci_intr_handle_t *pih;

	if (ihp == NULL)
		return EINVAL;

	pih = kmem_alloc(sizeof(*pih), KM_SLEEP);
	if (pci_intr_map(pa, pih)) {
		kmem_free(pih, sizeof(*pih));
		return EINVAL;
	}

	*ihp = pih;
	return 0;
}

int
pci_msi_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int *count)
{

	return EOPNOTSUPP;
}

int
pci_msi_alloc_exact(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int count)
{

	return EOPNOTSUPP;
}

int
pci_msix_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int *count)
{

	return EOPNOTSUPP;
}

int
pci_msix_alloc_exact(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int count)
{

	return EOPNOTSUPP;
}

int
pci_msix_alloc_map(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    u_int *table_indexes, int count)
{

	return EOPNOTSUPP;
}
#endif	/* __HAVE_PCI_MSI_MSIX */
