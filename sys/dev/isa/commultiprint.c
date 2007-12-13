/* $NetBSD: commultiprint.c,v 1.3.8.1 2007/12/13 21:55:40 bouyer Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: commultiprint.c,v 1.3.8.1 2007/12/13 21:55:40 bouyer Exp $");

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
