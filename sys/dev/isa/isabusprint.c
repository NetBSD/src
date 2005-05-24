/* $NetBSD: isabusprint.c,v 1.2 2005/05/24 05:14:37 lukem Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isabusprint.c,v 1.2 2005/05/24 05:14:37 lukem Exp $");

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
