/*	$NetBSD$	*/

/*
 * Copyright (C) 2010 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */

#include <sys/ioctl.h>
#include "ipf.h"
#include "netinet/ipl.h"


/*
 * Because the ipf_dstnode_t can vary in size because of the interface name,
 * the size may be larger than just sizeof().
 */
ippool_dst_t *
printdstl_live(d, fd, name, opts, fields)
	ippool_dst_t *d;
	int fd;
	char *name;
	int opts;
	wordtab_t *fields;
{
	ipf_dstnode_t *entry, *top, *node;
	ipflookupiter_t iter;
	int printed, last;
	ipfobj_t obj;

	if ((name != NULL) && strncmp(name, d->ipld_name, FR_GROUPLEN))
		return d->ipld_next;

	entry = calloc(1, sizeof(*entry) + 64);
	if (entry == NULL)
		return d->ipld_next;

	if (fields == NULL)
		printdstlistdata(d, opts);

	if ((d->ipld_flags & IPHASH_DELETE) != 0)
		PRINTF("# ");

	if ((opts & OPT_DEBUG) == 0)
		PRINTF("\t{");

	obj.ipfo_rev = IPFILTER_VERSION;
	obj.ipfo_type = IPFOBJ_LOOKUPITER;
	obj.ipfo_ptr = &iter;
	obj.ipfo_size = sizeof(iter);

	iter.ili_data = entry;
	iter.ili_type = IPLT_DSTLIST;
	iter.ili_otype = IPFLOOKUPITER_NODE;
	iter.ili_ival = IPFGENITER_LOOKUP;
	iter.ili_unit = d->ipld_unit;
	strncpy(iter.ili_name, d->ipld_name, FR_GROUPLEN);

	last = 0;
	top = NULL;
	printed = 0;
	entry = NULL;

	if (d->ipld_nodes > 0) {
		while (!last && (ioctl(fd, SIOCLOOKUPITER, &obj) == 0)) {
			if (entry->ipfd_next == NULL)
				last = 1;
			entry->ipfd_next = top;
			top = malloc(entry->ipfd_size);
			if (top == NULL)
				break;
			bcopy(entry, top, entry->ipfd_size);
		}
	}

	while (top != NULL) {
		node = top;
		(void) printdstlistnode(node, bcopywrap, opts, fields);
		top = node->ipfd_next;
		free(node);
		printed++;
	}

	if (entry != NULL)
		free(entry);

	if (printed == 0)
		putchar(';');

	if ((opts & OPT_DEBUG) == 0)
		PRINTF(" };\n");
	return d->ipld_next;
}
