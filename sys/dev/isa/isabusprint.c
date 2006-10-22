
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isabusprint.c,v 1.3.22.1 2006/10/22 06:06:04 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/isa/isavar.h>

int
isabusprint(void *via __unused, const char *pnp)
{
	if (pnp)
		aprint_normal("isa at %s", pnp);
	return (UNCONF);
}
