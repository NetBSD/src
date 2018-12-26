/*	$NetBSD: pppoe_component.c,v 1.1.20.1 2018/12/26 14:02:07 pgoyette Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pppoe_component.c,v 1.1.20.1 2018/12/26 14:02:07 pgoyette Exp $");

#include <sys/param.h>

#include <rump-sys/kern.h>

int pppoeattach(int);

RUMP_COMPONENT(RUMP_COMPONENT_NET_IF)
{
	pppoeattach(0);
}
