/* $NetBSD: commultiprint.c,v 1.3 2007/10/19 12:00:15 ad Exp $ */

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
