/*	$NetBSD: rumpnet_if_wrappers.c,v 1.4.24.2 2017/12/03 11:39:17 jdolecek Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpnet.ifspec,v 1.4 2016/01/26 23:22:22 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.10 2016/01/26 23:21:18 pooka Exp 
 */

#include <sys/cdefs.h>
#include <sys/systm.h>

#include <rump-sys/kern.h>
#include <rump-sys/net_if.h>

#include <rump/rump.h>
#include <rump/rumpnet_if_pub.h>

void __dead rump_net_unavailable(void);
void __dead
rump_net_unavailable(void)
{

	panic("net interface unavailable");
}

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
