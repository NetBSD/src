
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isabusprint.c,v 1.2.2.1 2006/06/21 15:04:21 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/isa/isavar.h>

int
isabusprint(void *via, const char *pnp)
{
#if 0
	struct isabus_attach_args *ia = via;
#endif
	if (pnp)
		aprint_normal("isa at %s", pnp);
	return (UNCONF);
}
