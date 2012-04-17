/*	$NetBSD: load_hashnode.c,v 1.1.1.1.2.2 2012/04/17 00:03:18 yamt Exp $	*/

/*
 * Copyright (C) 2009 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 *
 * Id
 */

#include <fcntl.h>
#include <sys/ioctl.h>
#include "ipf.h"
#include "netinet/ip_lookup.h"
#include "netinet/ip_htable.h"


int
load_hashnode(unit, name, node, ttl, iocfunc)
	int unit;
	char *name;
	iphtent_t *node;
	int ttl;
	ioctlfunc_t iocfunc;
{
	iplookupop_t op;
	iphtent_t ipe;
	int err;

	if (pool_open() == -1)
		return -1;

	op.iplo_type = IPLT_HASH;
	op.iplo_unit = unit;
	op.iplo_arg = 0;
	op.iplo_size = sizeof(ipe);
	op.iplo_struct = &ipe;
	strncpy(op.iplo_name, name, sizeof(op.iplo_name));

	bzero((char *)&ipe, sizeof(ipe));
	ipe.ipe_family = node->ipe_family;
	ipe.ipe_die = ttl;
	bcopy((char *)&node->ipe_addr, (char *)&ipe.ipe_addr,
	      sizeof(ipe.ipe_addr));
	bcopy((char *)&node->ipe_mask, (char *)&ipe.ipe_mask,
	      sizeof(ipe.ipe_mask));
	bcopy((char *)&node->ipe_group, (char *)&ipe.ipe_group,
	      sizeof(ipe.ipe_group));

	if ((opts & OPT_REMOVE) == 0)
		err = pool_ioctl(iocfunc, SIOCLOOKUPADDNODE, &op);
	else
		err = pool_ioctl(iocfunc, SIOCLOOKUPDELNODE, &op);

	if (err != 0)
		if (!(opts & OPT_DONOTHING)) {
			perror("load_hash:SIOCLOOKUP*NODE");
			return -1;
		}
	return 0;
}
