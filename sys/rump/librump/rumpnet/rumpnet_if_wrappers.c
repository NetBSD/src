/*	$NetBSD: rumpnet_if_wrappers.c,v 1.2 2009/10/14 18:16:41 pooka Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpnet.ifspec,v 1.1 2009/10/14 17:17:00 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.3 2009/10/14 18:14:48 pooka Exp 
 */

#include <sys/cdefs.h>
#include <sys/systm.h>

#include <rump/rump.h>
#include <rump/rumpnet_if_pub.h>

#include "rumpnet_if_priv.h"

void __dead rump_net_unavailable(void);
void __dead
rump_net_unavailable(void)
{

	panic("net interface unavailable");
}

int
rump_pub_virtif_create(int arg1)
{
	int rv;

	rv = rump_virtif_create(arg1);

	return rv;
}
__weak_alias(rump_virtif_create,rump_net_unavailable);
