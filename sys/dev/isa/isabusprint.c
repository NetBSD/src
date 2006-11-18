
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isabusprint.c,v 1.3.20.1 2006/11/18 21:34:21 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/isa/isavar.h>

int
isabusprint(void *via, const char *pnp)
{
	if (pnp)
		aprint_normal("isa at %s", pnp);
	return (UNCONF);
}
