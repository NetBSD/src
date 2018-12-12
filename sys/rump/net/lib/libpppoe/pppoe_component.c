/*	$NetBSD: pppoe_component.c,v 1.2 2018/12/12 00:48:44 alnsn Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pppoe_component.c,v 1.2 2018/12/12 00:48:44 alnsn Exp $");

#include <sys/param.h>

#include <rump-sys/kern.h>

int pppoeattach(int);

RUMP_COMPONENT(RUMP_COMPONENT_NET_IF)
{
	pppoeattach(0);
}
