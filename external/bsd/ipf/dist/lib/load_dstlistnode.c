/*	$NetBSD: load_dstlistnode.c,v 1.1.1.1.2.2 2012/04/17 00:03:17 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id: load_dstlistnode.c,v 1.1.2.1 2012/01/26 05:44:26 darren_r Exp 
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include "ipf.h"
#include "netinet/ip_lookup.h"
#include "netinet/ip_pool.h"


int
load_dstlistnode(role, name, node, ttl, iocfunc)
	int role;
	char *name;
	ipf_dstnode_t *node;
	int ttl;
	ioctlfunc_t iocfunc;
{
	iplookupop_t op;
	frdest_t *dst;
	int err;

	if (pool_open() == -1)
		return -1;

	dst = calloc(1, sizeof(*dst) + node->ipfd_dest.fd_name);
	if (dst == NULL)
		return -1;

	op.iplo_unit = role;
	op.iplo_type = IPLT_DSTLIST;
	op.iplo_arg = 0;
	op.iplo_struct = dst;
	op.iplo_size = sizeof(*dst) + node->ipfd_dest.fd_name;
	strncpy(op.iplo_name, name, sizeof(op.iplo_name));

	dst->fd_addr = node->ipfd_dest.fd_addr;
	dst->fd_type = node->ipfd_dest.fd_type;
	dst->fd_name = node->ipfd_dest.fd_name;
	bcopy(node->ipfd_names, (char *)dst + sizeof(*dst),
	      node->ipfd_dest.fd_name);

	if ((opts & OPT_REMOVE) == 0)
		err = pool_ioctl(iocfunc, SIOCLOOKUPADDNODE, &op);
	else
		err = pool_ioctl(iocfunc, SIOCLOOKUPDELNODE, &op);

	if (err != 0) {
		if ((opts & OPT_DONOTHING) == 0) {
			perror("load_dstlistnode:SIOCLOOKUP*NODE");
			free(dst);
			return -1;
		}
	}
	free(dst);

	return 0;
}
