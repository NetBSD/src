/* $NetBSD: eisabusprint.c,v 1.2 2005/05/24 05:14:37 lukem Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: eisabusprint.c,v 1.2 2005/05/24 05:14:37 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/eisa/eisavar.h>

int
eisabusprint(void *vea, const char *pnp)
{
#if 0
	struct eisabus_attach_args *ea = vea;
#endif
	if (pnp)
		aprint_normal("eisa at %s", pnp);
	return (UNCONF);
}
