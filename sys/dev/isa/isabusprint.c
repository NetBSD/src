/* $NetBSD: isabusprint.c,v 1.1 2004/08/30 10:30:38 drochner Exp $ */

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
