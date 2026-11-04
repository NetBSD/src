
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: agpbusprint.c,v 1.5.180.1 2026/11/04 23:22:04 perseant Exp $");

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
