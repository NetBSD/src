
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: agpbusprint.c,v 1.6 2026/06/21 17:09:44 andvar Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/agpvar.h>
#include <dev/pci/pcivar.h>

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
