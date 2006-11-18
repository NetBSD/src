
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: eisabusprint.c,v 1.3.20.1 2006/11/18 21:34:04 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/eisa/eisavar.h>

int
eisabusprint(void *vea, const char *pnp)
{
	if (pnp)
		aprint_normal("eisa at %s", pnp);
	return (UNCONF);
}
