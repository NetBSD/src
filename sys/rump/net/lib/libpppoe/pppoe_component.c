/*	$NetBSD: pppoe_component.c,v 1.3 2026/06/18 09:14:58 yamaguchi Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pppoe_component.c,v 1.3 2026/06/18 09:14:58 yamaguchi Exp $");

#include <sys/param.h>

#include <rump-sys/kern.h>

int pppoeattach(int);
extern unsigned int	sppp_keepalive_interval;

RUMP_COMPONENT(RUMP_COMPONENT_NET_IF)
{
	char buf[64];

	if (rumpuser_getparam("RUMP_PPPOE_KEEPALIVE_INTERVAL",
	    buf, sizeof(buf))  == 0) {
		unsigned long ul;

		ul = strtoul(buf, NULL, 10);
		if (ul != 0) {
			sppp_keepalive_interval = ul;
			printf("pppoe: keepalive interval set to %ds\n",
			    sppp_keepalive_interval);
		}
	}

	pppoeattach(0);
}
