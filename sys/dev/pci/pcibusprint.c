/* $NetBSD: pcibusprint.c,v 1.1 2004/08/30 10:30:38 drochner Exp $ */

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
