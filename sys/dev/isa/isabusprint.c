/* $NetBSD: isabusprint.c,v 1.1.2.4 2004/09/21 13:29:46 skrll Exp $ */

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
