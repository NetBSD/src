/* $NetBSD: eisabusprint.c,v 1.1 2004/08/30 10:30:38 drochner Exp $ */

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
