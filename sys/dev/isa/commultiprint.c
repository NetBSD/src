/* $NetBSD: commultiprint.c,v 1.2.46.2 2008/01/09 01:53:11 matt Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: commultiprint.c,v 1.2.46.2 2008/01/09 01:53:11 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <dev/isa/com_multi.h>

int
commultiprint(void *vca, const char *pnp)
{
	struct commulti_attach_args *ca = vca;

	if (pnp)
		aprint_normal("com at %s", pnp);
	aprint_normal(" slave %d", ca->ca_slave);
	return (UNCONF);
}
