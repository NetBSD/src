/* $NetBSD: pcibusprint.c,v 1.2 2005/05/24 05:14:37 lukem Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcibusprint.c,v 1.2 2005/05/24 05:14:37 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/pci/pcivar.h>

int
pcibusprint(void *vpa, const char *pnp)
{
	struct pcibus_attach_args *pa = vpa;

	if (pnp)
		aprint_normal("pci at %s", pnp);
	aprint_normal(" bus %d", pa->pba_bus);
	return (UNCONF);
}
