/*	$NetBSD: ip_dstlist.c,v 1.1.1.1 2012/01/30 16:05:10 darrenr Exp $	*/

/*
 * Copyright (C) 2012 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
#if defined(KERNEL) || defined(_KERNEL)
# undef KERNEL
# undef _KERNEL
# define        KERNEL	1
# define        _KERNEL	1
#endif
#if defined(__osf__)
# define _PROTO_NET_H_
#endif
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#if !defined(_KERNEL) && !defined(__KERNEL__)
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# define _KERNEL
# ifdef __OpenBSD__
struct file;
# endif
# include <sys/uio.h>
# undef _KERNEL
#else
# include <sys/systm.h>
# if defined(NetBSD) && (__NetBSD_Version__ >= 104000000)
#  include <sys/proc.h>
# endif
#endif
#include <sys/time.h>
#if !defined(linux)
# include <sys/protosw.h>
#endif
#include <sys/socket.h>
#if defined(_KERNEL) && (!defined(__SVR4) && !defined(__svr4__))
# include <sys/mbuf.h>
#endif
#if defined(__SVR4) || defined(__svr4__)
# include <sys/filio.h>
# include <sys/byteorder.h>
# ifdef _KERNEL
#  include <sys/dditypes.h>
# endif
# include <sys/stream.h>
# include <sys/kmem.h>
#endif
#if defined(__FreeBSD_version) && (__FreeBSD_version >= 300000)
# include <sys/malloc.h>
#endif

#include <net/if.h>
#include <netinet/in.h>

#include "netinet/ip_compat.h"
#include "netinet/ip_fil.h"
#include "netinet/ip_nat.h"
#include "netinet/ip_lookup.h"
#include "netinet/ip_dstlist.h"

/* END OF INCLUDES */

#ifdef HAS_SYS_MD5_H
# include <sys/md5.h>
#else
# include "md5.h"
#endif

#if !defined(lint)
static const char rcsid[] = "@(#)Id: ip_dstlist.c,v 2.13.2.5 2012/01/29 05:30:35 darrenr Exp";
#endif

typedef struct ipf_dstl_softc_s {
	ippool_dst_t	*dstlist[LOOKUP_POOL_SZ];
	ipf_dstl_stat_t	stats;
} ipf_dstl_softc_t;


static void *ipf_dstlist_soft_create __P((ipf_main_softc_t *));
static void ipf_dstlist_soft_destroy __P((ipf_main_softc_t *, void *));
static int ipf_dstlist_soft_init __P((ipf_main_softc_t *, void *));
static void ipf_dstlist_soft_fini __P((ipf_main_softc_t *, void *));
static int ipf_dstlist_addr_find __P((ipf_main_softc_t *, void *, int,
				      void *, u_int));
static size_t ipf_dstlist_flush __P((ipf_main_softc_t *, void *,
				     iplookupflush_t *));
static int ipf_dstlist_iter_deref __P((ipf_main_softc_t *, void *, int, int,
				       void *));
static int ipf_dstlist_iter_next __P((ipf_main_softc_t *, void *, ipftoken_t *,
				      ipflookupiter_t *));
static int ipf_dstlist_node_add __P((ipf_main_softc_t *, void *,
				     iplookupop_t *, int));
static int ipf_dstlist_node_del __P((ipf_main_softc_t *, void *,
				     iplookupop_t *, int));
static int ipf_dstlist_stats_get __P((ipf_main_softc_t *, void *,
				      iplookupop_t *));
static int ipf_dstlist_table_add __P((ipf_main_softc_t *, void *,
				      iplookupop_t *));
static int ipf_dstlist_table_del __P((ipf_main_softc_t *, void *,
				      iplookupop_t *));
static int ipf_dstlist_table_deref __P((ipf_main_softc_t *, void *, void *));
static void *ipf_dstlist_table_find __P((void *, int, char *));
static void ipf_dstlist_table_remove __P((ipf_main_softc_t *,
					  ipf_dstl_softc_t *, ippool_dst_t *));
static void ipf_dstlist_table_clearnodes __P((ipf_dstl_softc_t *,
					      ippool_dst_t *));
static ipf_dstnode_t *ipf_dstlist_select __P((fr_info_t *, ippool_dst_t *));
static void *ipf_dstlist_select_ref __P((void *, int, char *));
static void ipf_dstlist_node_free __P((ipf_dstl_softc_t *, ippool_dst_t *, ipf_dstnode_t *));
static int ipf_dstlist_node_deref __P((void *, ipf_dstnode_t *));
static void ipf_dstlist_expire __P((ipf_main_softc_t *, void *));
static void ipf_dstlist_sync __P((ipf_main_softc_t *, void *));

ipf_lookup_t ipf_dstlist_backend = {
	IPLT_DSTLIST,
	ipf_dstlist_soft_create,
	ipf_dstlist_soft_destroy,
	ipf_dstlist_soft_init,
	ipf_dstlist_soft_fini,
	ipf_dstlist_addr_find,
	ipf_dstlist_flush,
	ipf_dstlist_iter_deref,
	ipf_dstlist_iter_next,
	ipf_dstlist_node_add,
	ipf_dstlist_node_del,
	ipf_dstlist_stats_get,
	ipf_dstlist_table_add,
	ipf_dstlist_table_del,
	ipf_dstlist_table_deref,
	ipf_dstlist_table_find,
	ipf_dstlist_select_ref,
	ipf_dstlist_select_node,
	ipf_dstlist_expire,
	ipf_dstlist_sync
};


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_soft_create                                     */
/* Returns:     int - 0 = success, else error                               */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*                                                                          */
/* Allocating a chunk of memory filled with 0's is enough for the current   */
/* soft context used with destination lists.                                */
/* ------------------------------------------------------------------------ */
static void *
ipf_dstlist_soft_create(softc)
	ipf_main_softc_t *softc;
{
	ipf_dstl_softc_t *softd;

	KMALLOC(softd, ipf_dstl_softc_t *);
	if (softd == NULL)
		return NULL;

	bzero((char *)softd, sizeof(*softd));

	return softd;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_soft_destroy                                    */
/* Returns:     Nil                                                         */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*                                                                          */
/* For destination lists, the only thing we have to do when destroying the  */
/* soft context is free it!                                                 */
/* ------------------------------------------------------------------------ */
static void
ipf_dstlist_soft_destroy(softc, arg)
	ipf_main_softc_t *softc;
	void *arg;
{
	ipf_dstl_softc_t *softd = arg;

	KFREE(softd);
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_soft_init                                       */
/* Returns:     int - 0 = success, else error                               */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*                                                                          */
/* There is currently no soft context for destination list management.      */
/* ------------------------------------------------------------------------ */
static int
ipf_dstlist_soft_init(softc, arg)
	ipf_main_softc_t *softc;
	void *arg;
{
	return 0;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_soft_fini                                       */
/* Returns:     Nil                                                         */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*                                                                          */
/* There is currently no soft context for destination list management.      */
/* ------------------------------------------------------------------------ */
static void
ipf_dstlist_soft_fini(softc, arg)
	ipf_main_softc_t *softc;
	void *arg;
{
	ipf_dstl_softc_t *softd = arg;
	int i;

	for (i = -1; i <= IPL_LOGMAX; i++)
		while (softd->dstlist[i + 1] != NULL)
			ipf_dstlist_table_remove(softc, softd,
						 softd->dstlist[i + 1]);

	ASSERT(softd->stats.ipls_numderefnodes == 0);
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_addr_find                                       */
/* Returns:     int - 0 = success, else error                               */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg1(I)  - pointer to local context to use                  */
/*              arg2(I)  - pointer to local context to use                  */
/*              arg3(I)  - pointer to local context to use                  */
/*              arg4(I)  - pointer to local context to use                  */
/*                                                                          */
/* There is currently no such thing as searching a destination list for an  */
/* address so this function becomes a no-op.                                */
/* ------------------------------------------------------------------------ */
/*ARGSUSED*/
static int
ipf_dstlist_addr_find(softc, arg1, arg2, arg3, arg4)
	ipf_main_softc_t *softc;
	void *arg1, *arg3;
	int arg2;
	u_int arg4;
{
	return -1;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_flush                                           */
/* Returns:     int      - number of objects deleted                        */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*              fop(I)   - pointer to lookup flush operation data           */
/*                                                                          */
/* Flush all of the destination tables that match the data passed in with   */
/* the iplookupflush_t. There are two ways to match objects: the device for */
/* which they are to be used with and their name.                           */
/* ------------------------------------------------------------------------ */
static size_t
ipf_dstlist_flush(softc, arg, fop)
	ipf_main_softc_t *softc;
	void *arg;
	iplookupflush_t *fop;
{
	ipf_dstl_softc_t *softd = arg;
	ippool_dst_t *node, *next;
	int n, i;

	for (n = 0, i = -1; i <= IPL_LOGMAX; i++) {
		if (fop->iplf_unit != IPLT_ALL && fop->iplf_unit != i)
			continue;
		for (node = softd->dstlist[i + 1]; node != NULL; node = next) {
			next = node->ipld_next;

			if ((*fop->iplf_name != '\0') &&
			    strncmp(fop->iplf_name, node->ipld_name,
				    FR_GROUPLEN))
				continue;

			ipf_dstlist_table_remove(softc, softd, node);
			n++;
		}
	}
	return n;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_iter_deref                                      */
/* Returns:     int      - 0 = success, else error                          */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*              otype(I) - type of data structure to iterate through        */
/*              unit(I)  - device we are working with                       */
/*              data(I)  - address of object in kernel space                */
/*                                                                          */
/* ------------------------------------------------------------------------ */
static int
ipf_dstlist_iter_deref(softc, arg, otype, unit, data)
	ipf_main_softc_t *softc;
	void *arg;
	int otype, unit;
	void *data;
{
	if (data == NULL) {
		IPFERROR(120001);
		return EINVAL;
	}

	if (unit < -1 || unit > IPL_LOGMAX) {
		IPFERROR(120002);
		return EINVAL;
	}

	switch (otype)
	{
	case IPFLOOKUPITER_LIST :
		ipf_dstlist_table_deref(softc, arg, (ippool_dst_t *)data);
		break;

	case IPFLOOKUPITER_NODE :
		ipf_dstlist_node_deref(arg, (ipf_dstnode_t *)data);
		break;
	}

	return 0;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_iter_next                                       */
/* Returns:     int - 0 = success, else error                               */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*              op(I)    - pointer to lookup operation data                 */
/*              uid(I)   - uid of process doing the ioctl                   */
/*                                                                          */
/* ------------------------------------------------------------------------ */
static int
ipf_dstlist_iter_next(softc, arg, token, iter)
	ipf_main_softc_t *softc;
	void *arg;
	ipftoken_t *token;
	ipflookupiter_t *iter;
{
	ipf_dstnode_t zn, *nextnode = NULL, *node = NULL;
	ippool_dst_t zero, *next = NULL, *list = NULL;
	ipf_dstl_softc_t *softd = arg;
	int err = 0;

	switch (iter->ili_otype)
	{
	case IPFLOOKUPITER_LIST :
		list = token->ipt_data;
		if (list == NULL) {
			next = softd->dstlist[(int)iter->ili_unit + 1];
		} else {
			next = list->ipld_next;
		}

		if (next != NULL) {
			ATOMIC_INC32(list->ipld_ref);
			token->ipt_data = next;
		} else {
			bzero((char *)&zero, sizeof(zero));
			next = &zero;
			token->ipt_data = NULL;
		}
		break;

	case IPFLOOKUPITER_NODE :
		node = token->ipt_data;
		if (node == NULL) {
			list = ipf_dstlist_table_find(arg, iter->ili_unit,
						      iter->ili_name);
			if (list == NULL) {
				IPFERROR(120004);
				err = ESRCH;
				nextnode = NULL;
			} else {
				nextnode = *list->ipld_dests;
				list = NULL;
			}
		} else {
			nextnode = node->ipfd_next;
		}

		if (nextnode != NULL) {
			ATOMIC_INC32(nextnode->ipfd_ref);
			token->ipt_data = nextnode;
		} else {
			bzero((char *)&zn, sizeof(zn));
			nextnode = &zn;
			token->ipt_data = NULL;
		}
		break;
	default :
		IPFERROR(120003);
		err = EINVAL;
		break;
	}

	if (err != 0)
		return err;

	switch (iter->ili_otype)
	{
	case IPFLOOKUPITER_LIST :
		if (node != NULL) {
			ipf_dstlist_table_deref(softc, arg, node);
		}
		token->ipt_data = next;
		err = COPYOUT(next, iter->ili_data, sizeof(*next));
		if (err != 0) {
			IPFERROR(120005);
			err = EFAULT;
		}
		break;

	case IPFLOOKUPITER_NODE :
		if (node != NULL) {
			ipf_dstlist_node_deref(arg, node);
		}
		token->ipt_data = nextnode;
		err = COPYOUT(nextnode, iter->ili_data, sizeof(*nextnode));
		if (err != 0) {
			IPFERROR(120006);
			err = EFAULT;
		}
		break;
	}

	return err;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_node_add                                        */
/* Returns:     int - 0 = success, else error                               */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*              op(I)    - pointer to lookup operation data                 */
/*              uid(I)   - uid of process doing the ioctl                   */
/* Locks:       WRITE(ipf_poolrw)                                           */
/*                                                                          */
/* Add a new node to a destination list. To do this, we only copy in the    */
/* frdest_t structure because that contains the only data required from the */
/* application to create a new node. The frdest_t doesn't contain the name  */
/* itself. When loading filter rules, fd_name is a 'pointer' to the name.   */
/* In this case, the 'pointer' does not work, instead it is the length of   */
/* the name and the name is immediately following the frdest_t structure.   */
/* fd_name must include the trailing \0, so it should be strlen(str) + 1.   */
/* For simple sanity checking, an upper bound on the size of fd_name is     */
/* imposed - 128.                                                          */
/* ------------------------------------------------------------------------ */
static int
ipf_dstlist_node_add(softc, arg, op, uid)
	ipf_main_softc_t *softc;
	void *arg;
	iplookupop_t *op;
	int uid;
{
	ipf_dstl_softc_t *softd = arg;
	ipf_dstnode_t *node, **nodes;
	ippool_dst_t *d;
	frdest_t dest;
	int err;

	if (op->iplo_size < sizeof(frdest_t)) {
		IPFERROR(120007);
		return EINVAL;
	}

	err = COPYIN(op->iplo_struct, &dest, sizeof(dest));
	if (err != 0) {
		IPFERROR(120009);
		return EFAULT;
	}

	d = ipf_dstlist_table_find(arg, op->iplo_unit, op->iplo_name);
	if (d == NULL) {
		IPFERROR(120010);
		return ESRCH;
	}

	switch (dest.fd_addr.adf_family)
	{
	case AF_INET :
	case AF_INET6 :
		break;
	default :
		IPFERROR(120019);
		return EINVAL;
	}

	if (dest.fd_name < -1 || dest.fd_name > 128) {
		IPFERROR(120018);
		return EINVAL;
	}

	KMALLOCS(node, ipf_dstnode_t *, sizeof(*node) + dest.fd_name);
	if (node == NULL) {
		softd->stats.ipls_nomem++;
		IPFERROR(120008);
		return ENOMEM;
	}

	bcopy(&dest, &node->ipfd_dest, sizeof(dest));
	node->ipfd_size = sizeof(*node) + dest.fd_name;

	err = COPYIN((char *)op->iplo_struct + sizeof(dest), node->ipfd_names,
		     dest.fd_name);
	if (err != 0) {
		IPFERROR(120017);
		KFREES(node, node->ipfd_size);
		return EFAULT;
	}
	if (d->ipld_nodes == d->ipld_maxnodes) {
		KMALLOCS(nodes, ipf_dstnode_t **,
			 sizeof(*nodes) * (d->ipld_maxnodes + 1));
		if (nodes == NULL) {
			softd->stats.ipls_nomem++;
			IPFERROR(120022);
			KFREES(node, node->ipfd_size);
			return ENOMEM;
		}
		if (d->ipld_dests != NULL) {
			bcopy(d->ipld_dests, nodes,
			      sizeof(*nodes) * d->ipld_maxnodes);
			KFREES(d->ipld_dests, sizeof(*nodes) * d->ipld_nodes);
			nodes[0]->ipfd_pnext = nodes;
		}
		d->ipld_dests = nodes;
		d->ipld_maxnodes++;
	}
	d->ipld_dests[d->ipld_nodes] = node;
	d->ipld_nodes++;

	if (d->ipld_nodes == 1) {
		node->ipfd_pnext = d->ipld_dests;
	} else if (d->ipld_nodes > 1) {
		node->ipfd_pnext = &d->ipld_dests[d->ipld_nodes - 2]->ipfd_next;
	}
	*node->ipfd_pnext = node;

	MUTEX_INIT(&node->ipfd_lock, "ipf dst node lock");
	node->ipfd_plock = &d->ipld_lock;
	node->ipfd_next = NULL;
	node->ipfd_uid = uid;
	node->ipfd_states = 0;
	node->ipfd_ref = 1;
	node->ipfd_syncat = 0;
	node->ipfd_dest.fd_name = 0;
	(void) ipf_resolvedest(softc, node->ipfd_names, &node->ipfd_dest,
			       AF_INET);
#ifdef USE_INET6
	if (node->ipfd_dest.fd_ptr == (void *)-1)
		(void) ipf_resolvedest(softc, node->ipfd_names,
				       &node->ipfd_dest, AF_INET6);
#endif

	softd->stats.ipls_numnodes++;

	return 0;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_node_deref                                      */
/* Returns:     int - 0 = success, else error                               */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*              op(I)    - pointer to lookup operation data                 */
/*              uid(I)   - uid of process doing the ioctl                   */
/*                                                                          */
/* Dereference the use count by one. If it drops to zero then we can assume */
/* that it has been removed from any lists/tables and is ripe for freeing.  */
/* ------------------------------------------------------------------------ */
static int
ipf_dstlist_node_deref(arg, node)
	void *arg;
	ipf_dstnode_t *node;
{
	ipf_dstl_softc_t *softd = arg;
	int ref;

	/*
	 * ipfd_plock points back to the lock in the ippool_dst_t that is
	 * used to synchronise additions/deletions from its node list.
	 */
	MUTEX_ENTER(node->ipfd_plock);
	ref = --node->ipfd_ref;
	MUTEX_EXIT(node->ipfd_plock);

	if (ref > 0)
		return 0;

	MUTEX_DESTROY(&node->ipfd_lock);

	KFREES(node, node->ipfd_size);

	if ((node->ipfd_flags & IPDST_DELETE) != 0)
		softd->stats.ipls_numderefnodes--;

	return 0;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_node_del                                        */
/* Returns:     int      - 0 = success, else error                          */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*              op(I)    - pointer to lookup operation data                 */
/*              uid(I)   - uid of process doing the ioctl                   */
/*                                                                          */
/* Look for a matching destination node on the named table and free it if   */
/* found. Because the name embedded in the frdest_t is variable in length,  */
/* it is necessary to allocate some memory locally, to complete this op.    */
/* ------------------------------------------------------------------------ */
static int
ipf_dstlist_node_del(softc, arg, op, uid)
	ipf_main_softc_t *softc;
	void *arg;
	iplookupop_t *op;
	int uid;
{
	ipf_dstl_softc_t *softd = arg;
	ipf_dstnode_t *node;
	frdest_t frd, *temp;
	ippool_dst_t *d;
	size_t size;
	int err;

	d = ipf_dstlist_table_find(arg, op->iplo_unit, op->iplo_name);
	if (d == NULL) {
		IPFERROR(120012);
		return ESRCH;
	}

	err = COPYIN(op->iplo_struct, &frd, sizeof(frd));
	if (err != 0) {
		IPFERROR(120011);
		return EFAULT;
	}

	size = sizeof(*temp) + frd.fd_name;
	KMALLOCS(temp, frdest_t *, size);
	if (temp == NULL) {
		softd->stats.ipls_nomem++;
		IPFERROR(120026);
		return ENOMEM;
	}

	err = COPYIN(op->iplo_struct, temp, size);
	if (err != 0) {
		IPFERROR(120027);
		return EFAULT;
	}

	MUTEX_ENTER(&d->ipld_lock);
	for (node = *d->ipld_dests; node != NULL; node = node->ipfd_next) {
		if ((uid != 0) && (node->ipfd_uid != uid))
			continue;
		if (node->ipfd_size != size)
			continue;
		if (!bcmp(&node->ipfd_dest.fd_ip6, &frd.fd_ip6,
			  size - offsetof(frdest_t, fd_ip6))) {
			MUTEX_ENTER(&node->ipfd_lock);
			ipf_dstlist_node_free(softd, d, node);
			MUTEX_EXIT(&d->ipld_lock);
			KFREES(temp, size);
			return 0;
		}
	}
	MUTEX_EXIT(&d->ipld_lock);
	KFREES(temp, size);

	return ESRCH;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_node_free                                       */
/* Returns:     Nil                                                         */
/* Parameters:  node(I) - pointer to node to free                           */
/* Locks:       MUTEX(ipld_lock) or WRITE(ipf_poolrw)                       */
/*                                                                          */
/* Free the destination node by first removing it from any lists and then   */
/* checking if this was the last reference held to the object. While the    */
/* array of pointers to nodes is compacted, its size isn't reduced (by way  */
/* of allocating a new smaller one and copying) because the belief is that  */
/* it is likely the array will again reach that size.                       */
/* ------------------------------------------------------------------------ */
static void
ipf_dstlist_node_free(softd, d, node)
	ipf_dstl_softc_t *softd;
	ippool_dst_t *d;
	ipf_dstnode_t *node;
{
	int ref;
	int i;

	/*
	 * Compact the array of pointers to nodes.
	 */
	for (i = 0; i < d->ipld_nodes; i++)
		if (d->ipld_dests[i] == node)
			break;
	if (d->ipld_nodes - i > 1) {
		bcopy(&d->ipld_dests[i + 1], &d->ipld_dests[i],
		      sizeof(*d->ipld_dests) * (d->ipld_nodes - i - 1));
	}
	d->ipld_nodes--;
	/*
	 * ipfd_plock points back to the lock in the ippool_dst_t that is
	 * used to synchronise additions/deletions from its node list.
	 */
	MUTEX_ENTER(node->ipfd_plock);

	ref = --node->ipfd_ref;

	if (node->ipfd_pnext != NULL)
		*node->ipfd_pnext = node->ipfd_next;
	if (node->ipfd_next != NULL)
		node->ipfd_next->ipfd_pnext = node->ipfd_pnext;
	node->ipfd_pnext = NULL;
	node->ipfd_next = NULL;

	MUTEX_EXIT(node->ipfd_plock);

	if (ref == 0) {
		MUTEX_DESTROY(&node->ipfd_lock);
		KFREES(node, node->ipfd_size);
		softd->stats.ipls_numnodes--;
	} else if ((node->ipfd_flags & IPDST_DELETE) == 0) {
		softd->stats.ipls_numderefnodes++;
		node->ipfd_flags |= IPDST_DELETE;
	}
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_stats_get                                       */
/* Returns:     int - 0 = success, else error                               */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*              op(I)    - pointer to lookup operation data                 */
/*                                                                          */
/* Return the current statistics for destination lists. This may be for all */
/* of them or just information pertaining to a particular table.            */
/* ------------------------------------------------------------------------ */
/*ARGSUSED*/
static int
ipf_dstlist_stats_get(softc, arg, op)
	ipf_main_softc_t *softc;
	void *arg;
	iplookupop_t *op;
{
	ipf_dstl_softc_t *softd = arg;
	ipf_dstl_stat_t stats;
	int unit, i, err = 0;

	if (op->iplo_size != sizeof(ipf_dstl_stat_t)) {
		IPFERROR(120023);
		return EINVAL;
	}

	stats = softd->stats;
	unit = op->iplo_unit;
	if (unit == IPL_LOGALL) {
		for (i = 0; i <= IPL_LOGMAX; i++)
			stats.ipls_list[i] = softd->dstlist[i];
	} else if (unit >= 0 && unit <= IPL_LOGMAX) {
		void *ptr;

		if (op->iplo_name[0] != '\0')
			ptr = ipf_dstlist_table_find(softd, unit,
						     op->iplo_name);
		else
			ptr = softd->dstlist[unit + 1];
		stats.ipls_list[unit + 1] = ptr;
	} else {
		IPFERROR(120024);
		err = EINVAL;
	}

	if (err == 0) {
		err = COPYOUT(&stats, op->iplo_struct, sizeof(stats));
		if (err != 0) {
			IPFERROR(120025);
			return EFAULT;
		}
	}
	return 0;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_table_add                                       */
/* Returns:     int      - 0 = success, else error                          */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*              op(I)    - pointer to lookup operation data                 */
/*                                                                          */
/* Add a new destination table to the list of those available for the given */
/* device. Because we seldom operate on these objects (find/add/delete),    */
/* they are just kept in a simple linked list.                              */
/* ------------------------------------------------------------------------ */
static int
ipf_dstlist_table_add(softc, arg, op)
	ipf_main_softc_t *softc;
	void *arg;
	iplookupop_t *op;
{
	ipf_dstl_softc_t *softd = arg;
	ippool_dst_t user, *d, *new;
	int unit, err;

	KMALLOC(new, ippool_dst_t *);
	if (new == NULL) {
		softd->stats.ipls_nomem++;
		IPFERROR(120014);
		return ENOMEM;
	}

	d = ipf_dstlist_table_find(arg, op->iplo_unit, op->iplo_name);
	if (d != NULL) {
		IPFERROR(120013);
		KFREE(new);
		return EEXIST;
	}

	err = COPYIN(op->iplo_struct, &user, sizeof(user));
	if (err != 0) {
		IPFERROR(120021);
		KFREE(new);
		return EFAULT;
	}

	MUTEX_INIT(&new->ipld_lock, "ipf dst table lock");

	strncpy(new->ipld_name, op->iplo_name, FR_GROUPLEN);
	unit = op->iplo_unit;
	new->ipld_unit = unit;
	new->ipld_policy = user.ipld_policy;
	new->ipld_seed = ipf_random();
	new->ipld_dests = NULL;
	new->ipld_nodes = 0;
	new->ipld_maxnodes = 0;
	new->ipld_selected = NULL;
	new->ipld_ref = 0;
	new->ipld_flags = 0;

	new->ipld_pnext = &softd->dstlist[unit + 1];
	new->ipld_next = softd->dstlist[unit + 1];
	if (softd->dstlist[unit + 1] != NULL)
		softd->dstlist[unit + 1]->ipld_pnext = &new->ipld_next;
	softd->dstlist[unit + 1] = new;
	softd->stats.ipls_numlists++;

	return 0;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_table_del                                       */
/* Returns:     int - 0 = success, else error                               */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*              op(I)    - pointer to lookup operation data                 */
/*                                                                          */
/* Find a named destinstion list table and delete it. If there are other    */
/* references to it, the caller isn't told.                                 */
/* ------------------------------------------------------------------------ */
static int
ipf_dstlist_table_del(softc, arg, op)
	ipf_main_softc_t *softc;
	void *arg;
	iplookupop_t *op;
{
	ippool_dst_t *d;

	d = ipf_dstlist_table_find(arg, op->iplo_unit, op->iplo_name);
	if (d == NULL) {
		IPFERROR(120015);
		return ESRCH;
	}

	if (d->ipld_dests != NULL) {
		IPFERROR(120016);
		return EBUSY;
	}

	ipf_dstlist_table_remove(softc, arg, d);

	return 0;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_table_remove                                    */
/* Returns:     Nil                                                         */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*              op(I)    - pointer to lookup operation data                 */
/*                                                                          */
/* Remove a given destination list from existance. While the IPDST_DELETE   */
/* flag is set every time we call this function and the reference count is  */
/* non-zero, the "numdereflists" counter is only incremented when the entry */
/* is removed from the list as it only becomes dereferenced once.           */
/* ------------------------------------------------------------------------ */
static void
ipf_dstlist_table_remove(softc, softd, d)
	ipf_main_softc_t *softc;
	ipf_dstl_softc_t *softd;
	ippool_dst_t *d;
{

	if (d->ipld_pnext != NULL) {
		*d->ipld_pnext = d->ipld_next;
		if (d->ipld_ref > 1)
			softd->stats.ipls_numdereflists++;
	}
	if (d->ipld_next != NULL)
		d->ipld_next->ipld_pnext = d->ipld_pnext;
	d->ipld_pnext = NULL;
	d->ipld_next = NULL;

	ipf_dstlist_table_clearnodes(softd, d);

	if (d->ipld_ref > 0) {
		d->ipld_flags |= IPDST_DELETE;
		return;
	}

	MUTEX_DESTROY(&d->ipld_lock);

	if ((d->ipld_flags & IPDST_DELETE) != 0)
		softd->stats.ipls_numdereflists--;
	softd->stats.ipls_numlists--;

	if (d->ipld_dests != NULL) {
		KFREES(d->ipld_dests,
		       d->ipld_maxnodes * sizeof(*d->ipld_dests));
	}

	KFREE(d);
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_table_deref                                     */
/* Returns:     int - 0 = success, else error                               */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*              op(I)    - pointer to lookup operation data                 */
/*                                                                          */
/* Drops the reference count on a destination list table object and free's  */
/* it if 0 has been reached.                                                */
/* ------------------------------------------------------------------------ */
static int
ipf_dstlist_table_deref(softc, arg, table)
	ipf_main_softc_t *softc;
	void *arg;
	void *table;
{
	ippool_dst_t *d = table;

	d->ipld_ref--;
	if (d->ipld_ref > 0)
		return d->ipld_ref;

	ipf_dstlist_table_remove(softc, arg, table);

	return 0;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_table_clearnodes                                */
/* Returns:     Nil                                                         */
/* Parameters:  dst(I) - pointer to soft context main structure             */
/*                                                                          */
/* Free all of the destination nodes attached to the given table.           */
/* ------------------------------------------------------------------------ */
static void
ipf_dstlist_table_clearnodes(softd, dst)
	ipf_dstl_softc_t *softd;
	ippool_dst_t *dst;
{
	ipf_dstnode_t *node;

	while ((node = *dst->ipld_dests) != NULL) {
		ipf_dstlist_node_free(softd, dst, node);
	}
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_table_find                                      */
/* Returns:     int      - 0 = success, else error                          */
/* Parameters:  arg(I)   - pointer to local context to use                  */
/*              unit(I)  - device we are working with                       */
/*              name(I)  - destination table name to find                   */
/*                                                                          */
/* Return a pointer to a destination table that matches the unit+name that  */
/* is passed in.                                                            */
/* ------------------------------------------------------------------------ */
static void *
ipf_dstlist_table_find(arg, unit, name)
	void *arg;
	int unit;
	char *name;
{
	ipf_dstl_softc_t *softd = arg;
	ippool_dst_t *d;

	for (d = softd->dstlist[unit + 1]; d != NULL; d = d->ipld_next) {
		if ((d->ipld_unit == unit) &&
		    !strncmp(d->ipld_name, name, FR_GROUPLEN)) {
			return d;
		}
	}

	return NULL;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_select_ref                                      */
/* Returns:     void *   - NULL = failure, else pointer to table            */
/* Parameters:  arg(I)   - pointer to local context to use                  */
/*              unit(I)  - device we are working with                       */
/*              name(I)  - destination table name to find                   */
/*                                                                          */
/* Attempt to find a destination table that matches the name passed in and  */
/* if successful, bump up the reference count on it because we intend to    */
/* store the pointer to it somewhere else.                                  */
/* ------------------------------------------------------------------------ */
static void *
ipf_dstlist_select_ref(arg, unit, name)
	void *arg;
	int unit;
	char *name;
{
	ippool_dst_t *d;

	d = ipf_dstlist_table_find(arg, unit, name);
	if (d != NULL) {
		MUTEX_ENTER(&d->ipld_lock);
		d->ipld_ref++;
		MUTEX_EXIT(&d->ipld_lock);
	}
	return d;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_select                                          */
/* Returns:     void * - NULL = failure, else pointer to table              */
/* Parameters:  d(I)   - pointer to destination list                        */
/*                                                                          */
/* Find the next node in the destination list to be used according to the   */
/* defined policy. Of these, "connection" is the most expensive policy to   */
/* implement as it always looks for the node with the least number of       */
/* connections associated with it.                                          */
/*                                                                          */
/* The hashes exclude the port numbers so that all protocols map to the     */
/* same destination. Otherwise, someone doing a ping would target a         */
/* different server than their TCP connection, etc. MD-5 is used to         */
/* transform the addressese into something random that the other end could  */
/* not easily guess and use in an attack. ipld_seed introduces an unknown   */
/* into the hash calculation to increase the difficult of an attacker       */
/* guessing the bucket.                                                     */
/*                                                                          */
/* One final comment: mixing different address families in a single pool    */
/* will currently result in failures as the address family of the node is   */
/* only matched up with that in the packet as the last step. While this can */
/* be coded around for the weighted connection and round-robin models, it   */
/* cannot be supported for the hash/random models as they do not search and */
/* nor is the algorithm conducive to searching.                             */
/* ------------------------------------------------------------------------ */
static ipf_dstnode_t *
ipf_dstlist_select(fin, d)
	fr_info_t *fin;
	ippool_dst_t *d;
{
	ipf_dstnode_t *node, *sel;
	int connects;
	u_32_t hash[4];
	MD5_CTX ctx;
	int family;
	int x;

	if (d->ipld_dests == NULL || *d->ipld_dests == NULL)
		return NULL;

	family = fin->fin_family;

	MUTEX_ENTER(&d->ipld_lock);

	switch (d->ipld_policy)
	{
	case IPLDP_ROUNDROBIN:
		sel = d->ipld_selected;
		if (sel == NULL) {
			sel = *d->ipld_dests;
		} else {
			sel = sel->ipfd_next;
			if (sel == NULL)
				sel = *d->ipld_dests;
		}
		break;

	case IPLDP_CONNECTION:
		if (d->ipld_selected == NULL) {
			sel = *d->ipld_dests;
			break;
		}

		sel = d->ipld_selected;
		connects = 0x7fffffff;
		node = sel->ipfd_next;
		if (node == NULL)
			node = *d->ipld_dests;
		while (node != d->ipld_selected) {
			if (node->ipfd_states == 0) {
				sel = node;
				break;
			}
			if (node->ipfd_states < connects) {
				sel = node;
				connects = node->ipfd_states;
			}
			node = node->ipfd_next;
			if (node == NULL)
				node = *d->ipld_dests;
		}
		break;

	case IPLDP_RANDOM :
		x = ipf_random() % d->ipld_nodes;
		sel = d->ipld_dests[x];
		break;

	case IPLDP_HASHED :
		MD5Init(&ctx);
		MD5Update(&ctx, (u_char *)&d->ipld_seed, sizeof(d->ipld_seed));
		MD5Update(&ctx, (u_char *)&fin->fin_src6,
			  sizeof(fin->fin_src6));
		MD5Update(&ctx, (u_char *)&fin->fin_dst6,
			  sizeof(fin->fin_dst6));
		MD5Final((u_char *)hash, &ctx);
		x = hash[0] % d->ipld_nodes;
		sel = d->ipld_dests[x];
		break;

	case IPLDP_SRCHASH :
		MD5Init(&ctx);
		MD5Update(&ctx, (u_char *)&d->ipld_seed, sizeof(d->ipld_seed));
		MD5Update(&ctx, (u_char *)&fin->fin_src6,
			  sizeof(fin->fin_src6));
		MD5Final((u_char *)hash, &ctx);
		x = hash[0] % d->ipld_nodes;
		sel = d->ipld_dests[x];
		break;

	case IPLDP_DSTHASH :
		MD5Init(&ctx);
		MD5Update(&ctx, (u_char *)&d->ipld_seed, sizeof(d->ipld_seed));
		MD5Update(&ctx, (u_char *)&fin->fin_dst6,
			  sizeof(fin->fin_dst6));
		MD5Final((u_char *)hash, &ctx);
		x = hash[0] % d->ipld_nodes;
		sel = d->ipld_dests[x];
		break;

	default :
		sel = NULL;
		break;
	}

	if (sel->ipfd_dest.fd_addr.adf_family != family)
		sel = NULL;
	d->ipld_selected = sel;

	MUTEX_EXIT(&d->ipld_lock);

	return sel;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_select_node                                     */
/* Returns:     int      - -1 == failure, 0 == success                      */
/* Parameters:  fin(I)   - pointer to packet information                    */
/*              group(I) - destination pool to search                       */
/*              addr(I)  - pointer to store selected address                */
/*                                                                          */
/* This function is only responsible for obtaining the next IP address for  */
/* use and storing it in the caller's address space (addr). No permanent    */
/* reference is currently kept on the node.                                 */
/* ------------------------------------------------------------------------ */
int
ipf_dstlist_select_node(fin, group, addr, pfdp)
	fr_info_t *fin;
	void *group;
	u_32_t *addr;
	frdest_t *pfdp;
{
#ifdef USE_MUTEXES
	ipf_main_softc_t *softc = fin->fin_main_soft;
#endif
	ippool_dst_t *d = group;
	ipf_dstnode_t *node;
	frdest_t *fdp;

	READ_ENTER(&softc->ipf_poolrw);

	node = ipf_dstlist_select(fin, d);
	if (node == NULL) {
		RWLOCK_EXIT(&softc->ipf_poolrw);
		return -1;
	}

	if (pfdp != NULL) {
		bcopy(&node->ipfd_dest, pfdp, sizeof(*pfdp));
	} else {
		if (fin->fin_family == AF_INET) {
			addr[0] = node->ipfd_dest.fd_addr.adf_addr.i6[0];
		} else if (fin->fin_family == AF_INET6) {
			addr[0] = node->ipfd_dest.fd_addr.adf_addr.i6[0];
			addr[1] = node->ipfd_dest.fd_addr.adf_addr.i6[1];
			addr[2] = node->ipfd_dest.fd_addr.adf_addr.i6[2];
			addr[3] = node->ipfd_dest.fd_addr.adf_addr.i6[3];
		}
	}

	fdp = &node->ipfd_dest;
	if (fdp->fd_ptr == NULL)
		fdp->fd_ptr = fin->fin_ifp;

	MUTEX_ENTER(&node->ipfd_lock);
	node->ipfd_states++;
	MUTEX_EXIT(&node->ipfd_lock);

	RWLOCK_EXIT(&softc->ipf_poolrw);

	return 0;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_expire                                          */
/* Returns:     Nil                                                         */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*                                                                          */
/* There are currently no objects to expire in destination lists.           */
/* ------------------------------------------------------------------------ */
static void
ipf_dstlist_expire(softc, arg)
	ipf_main_softc_t *softc;
	void *arg;
{
	return;
}


/* ------------------------------------------------------------------------ */
/* Function:    ipf_dstlist_sync                                            */
/* Returns:     Nil                                                         */
/* Parameters:  softc(I) - pointer to soft context main structure           */
/*              arg(I)   - pointer to local context to use                  */
/*                                                                          */
/* When a network interface appears or disappears, we need to revalidate    */
/* all of the network interface names that have been configured as a target */
/* in a destination list.                                                   */
/* ------------------------------------------------------------------------ */
void
ipf_dstlist_sync(softc, arg)
	ipf_main_softc_t *softc;
	void *arg;
{
	ipf_dstl_softc_t *softd = arg;
	ipf_dstnode_t *node;
	ippool_dst_t *list;
	int i;
	int j;

	for (i = 0; i < IPL_LOGMAX; i++) {
		for (list = softd->dstlist[i]; list != NULL;
		     list = list->ipld_next) {
			for (j = 0; j < list->ipld_maxnodes; j++) {
				node = list->ipld_dests[j];
				if (node == NULL)
					continue;
				if (node->ipfd_dest.fd_name == -1)
					continue;
				(void) ipf_resolvedest(softc,
						       node->ipfd_names,
						       &node->ipfd_dest,
						       AF_INET);
			}
		}
	}
}
