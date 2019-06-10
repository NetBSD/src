/*	$NetBSD: pppoe_component.c,v 1.1.22.1 2019/06/10 22:09:55 christos Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pppoe_component.c,v 1.1.22.1 2019/06/10 22:09:55 christos Exp $");

#include <sys/param.h>

#include <rump-sys/kern.h>

int pppoeattach(int);

RUMP_COMPONENT(RUMP_COMPONENT_NET_IF)
{
	pppoeattach(0);
}
