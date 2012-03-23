/*	$NetBSD: printpool_live.c,v 1.1.1.1 2012/03/23 21:20:08 christos Exp $	*/

/*
 * Copyright (C) 2011 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */

#include <sys/ioctl.h>
#include "ipf.h"
#include "netinet/ipl.h"


ip_pool_t *
printpool_live(pool, fd, name, opts, fields)
	ip_pool_t *pool;
	int fd;
	char *name;
	int opts;
	wordtab_t *fields;
{
	ip_pool_node_t entry, *top, *node;
	ipflookupiter_t iter;
	int printed, last;
	ipfobj_t obj;

	if ((name != NULL) && strncmp(name, pool->ipo_name, FR_GROUPLEN))
		return pool->ipo_next;

	if (fields == NULL)
		printpooldata(pool, opts);

	if ((pool->ipo_flags & IPOOL_DELETE) != 0)
		PRINTF("# ");
	if ((opts & OPT_DEBUG) == 0)
		PRINTF("\t{");

	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_type = IPFOBJ_LOOKUPITER;
	obj.ipfo_ptr = &iter;
	obj.ipfo_size = sizeof(iter);

	iter.ili_data = &entry;
	iter.ili_type = IPLT_POOL;
	iter.ili_otype = IPFLOOKUPITER_NODE;
	iter.ili_ival = IPFGENITER_LOOKUP;
	iter.ili_unit = pool->ipo_unit;
	strncpy(iter.ili_name, pool->ipo_name, FR_GROUPLEN);

	last = 0;
	top = NULL;
	printed = 0;

	if (pool->ipo_list != NULL) {
		while (!last && (ioctl(fd, SIOCLOOKUPITER, &obj) == 0)) {
			if (entry.ipn_next == NULL)
				last = 1;
			node = malloc(sizeof(*top));
			if (node == NULL)
				break;
			bcopy(&entry, node, sizeof(entry));
			node->ipn_next = top;
			top = node;
		}
	}

	while (top != NULL) {
		node = top;
		(void) printpoolnode(node, opts, fields);
		if ((opts & OPT_DEBUG) == 0)
			putchar(';');
		top = node->ipn_next;
		free(node);
		printed++;
	}

	if (printed == 0)
		putchar(';');

	if ((opts & OPT_DEBUG) == 0)
		PRINTF(" };\n");

	(void) ioctl(fd,SIOCIPFDELTOK, &iter.ili_key);

	return pool->ipo_next;
}
