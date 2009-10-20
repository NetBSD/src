/*	$NetBSD: rumpnet_if_wrappers.c,v 1.4 2009/10/20 23:29:39 pooka Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpnet.ifspec,v 1.2 2009/10/20 23:28:50 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.4 2009/10/15 00:29:19 pooka Exp 
 */

#include <sys/cdefs.h>
#include <sys/systm.h>

#include <rump/rump.h>
#include <rump/rumpnet_if_pub.h>

#include "rump_private.h"
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

	rump_schedule();
	rv = rump_virtif_create(arg1);
	rump_unschedule();

	return rv;
}
__weak_alias(rump_virtif_create,rump_net_unavailable);

int
rump_pub_shmif_create(const char *arg1, int *arg2)
{
	int rv;

	rump_schedule();
	rv = rump_shmif_create(arg1, arg2);
	rump_unschedule();

	return rv;
}
__weak_alias(rump_shmif_create,rump_net_unavailable);
