
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: agpbusprint.c,v 1.3.22.2 2006/12/10 07:17:41 yamt Exp $");

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
