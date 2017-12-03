/*	$NetBSD: net_if.h,v 1.2.18.2 2017/12/03 11:39:14 jdolecek Exp $	*/

/*
 * Automatically generated.  DO NOT EDIT.
 * from: NetBSD: rumpnet.ifspec,v 1.4 2016/01/26 23:22:22 pooka Exp 
 * by:   NetBSD: makerumpif.sh,v 1.10 2016/01/26 23:21:18 pooka Exp 
 */

#ifndef _RUMP_PRIF_NET_H_
#define _RUMP_PRIF_NET_H_

int rump_shmif_create(const char *, int *);
typedef int (*rump_shmif_create_fn)(const char *, int *);

#endif /* _RUMP_PRIF_NET_H_ */
