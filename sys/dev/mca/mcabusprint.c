
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcabusprint.c,v 1.3.20.1 2006/11/18 21:34:26 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/mca/mcavar.h>

int
mcabusprint(void *vma, const char *pnp)
{
#if 0
	struct mcabus_attach_args *ma = vma;
#endif
	if (pnp)
		aprint_normal("mca at %s", pnp);
	return (UNCONF);
}
