/* $NetBSD: x86_stub.c,v 1.5 2016/09/21 00:00:07 jmcneill Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: x86_stub.c,v 1.5 2016/09/21 00:00:07 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kgdb.h>

#include <machine/autoconf.h>
#include <dev/pci/pcivar.h>

int x86_zeroop(void);
void *x86_nullop(void);
void x86_voidop(void);

void
x86_voidop(void)
{
}

void *
x86_nullop(void)
{
	return NULL;
}

int
x86_zeroop(void)
{
	return 0;
}

__weak_alias(device_acpi_register, x86_nullop);
__weak_alias(device_isa_register, x86_nullop);
__weak_alias(device_pci_props_register, x86_voidop);
__weak_alias(device_pci_register, x86_nullop);
__weak_alias(kdb_trap, x86_zeroop);
__weak_alias(kgdb_disconnected, x86_zeroop);
__weak_alias(kgdb_trap, x86_zeroop);
__weak_alias(mca_nmi, x86_voidop);
__weak_alias(pci_ranges_infer, x86_voidop);
__weak_alias(x86_nmi, x86_voidop);
