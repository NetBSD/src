/* $NetBSD: agpbusprint.c,v 1.1 2004/08/30 10:30:38 drochner Exp $ */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/agpvar.h>

int
agpbusprint(void *vaa, const char *pnp)
{
#if 0
	struct agpbus_attach_args *aa = vaa;
#endif
	if (pnp)
		aprint_normal("agp at %s", pnp);
	return (UNCONF);
}
