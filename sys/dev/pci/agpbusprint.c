
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: agpbusprint.c,v 1.3.22.1 2006/10/22 06:06:15 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/agpvar.h>

int
agpbusprint(void *vaa __unused, const char *pnp)
{
#if 0
	struct agpbus_attach_args *aa = vaa;
#endif
	if (pnp)
		aprint_normal("agp at %s", pnp);
	return (UNCONF);
}
