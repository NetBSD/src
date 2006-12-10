
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isabusprint.c,v 1.3.22.2 2006/12/10 07:17:29 yamt Exp $");

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
