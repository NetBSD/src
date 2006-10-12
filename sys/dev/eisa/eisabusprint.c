
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: eisabusprint.c,v 1.4 2006/10/12 01:30:57 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/eisa/eisavar.h>

int
eisabusprint(void *vea __unused, const char *pnp)
{
	if (pnp)
		aprint_normal("eisa at %s", pnp);
	return (UNCONF);
}
