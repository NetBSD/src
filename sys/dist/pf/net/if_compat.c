/*	$OpenBSD: if.c,v 1.165 2007/07/06 14:00:59 naddy Exp $	*/
/*	$NetBSD: if_compat.c,v 1.1.2.3 2008/04/23 19:33:56 peter Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1980, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if.c	8.3 (Berkeley) 1/4/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_compat.c,v 1.1.2.3 2008/04/23 19:33:56 peter Exp $");

#include "pf.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_var.h>

#include <net/if_compat.h>

#if NPF > 0
#include <net/pfvar.h>
#endif

#if 0 /* XXX unused - remove later */
static int if_getgroup(void *, struct ifnet *);
static int if_getgroupmembers(void *);

static int if_group_egress_build(void);
#endif

TAILQ_HEAD(, ifg_group) ifg_head = TAILQ_HEAD_INITIALIZER(ifg_head);

void
if_init_groups(struct ifnet *ifp)
{
	struct ifg_list_head *ifgh;

	ifgh = malloc(sizeof(struct ifg_list_head), M_TEMP, M_WAITOK);
	TAILQ_INIT(ifgh);

	ifp->if_pf_groups = ifgh;
}

void
if_destroy_groups(struct ifnet *ifp)
{
	struct ifg_list_head *ifgh = if_get_groups(ifp);

	free(ifgh, M_TEMP);
}

struct ifg_list_head *
if_get_groups(struct ifnet *ifp)
{
	return (ifp->if_pf_groups);
}

/*
 * Create interface group without members.
 */
struct ifg_group *
if_creategroup(const char *groupname)
{
	struct ifg_group	*ifg = NULL;

	if ((ifg = (struct ifg_group *)malloc(sizeof(struct ifg_group),
	    M_TEMP, M_NOWAIT)) == NULL)
		return (NULL);

	strlcpy(ifg->ifg_group, groupname, sizeof(ifg->ifg_group));
	ifg->ifg_refcnt = 0;
	ifg->ifg_carp_demoted = 0;
	TAILQ_INIT(&ifg->ifg_members);
#if NPF > 0
	pfi_attach_ifgroup(ifg);
#endif
	TAILQ_INSERT_TAIL(&ifg_head, ifg, ifg_next);

	return (ifg);
}

/*
 * Add a group to an interface.
 */
int
if_addgroup(struct ifnet *ifp, const char *groupname)
{
	struct ifg_list_head	*ifgh = if_get_groups(ifp);
	struct ifg_list		*ifgl;
	struct ifg_group	*ifg = NULL;
	struct ifg_member	*ifgm;

	if (groupname[0] && groupname[strlen(groupname) - 1] >= '0' &&
	    groupname[strlen(groupname) - 1] <= '9')
		return (EINVAL);

	TAILQ_FOREACH(ifgl, ifgh, ifgl_next)
		if (!strcmp(ifgl->ifgl_group->ifg_group, groupname))
			return (EEXIST);

	if ((ifgl = (struct ifg_list *)malloc(sizeof(struct ifg_list), M_TEMP,
	    M_NOWAIT)) == NULL)
		return (ENOMEM);

	if ((ifgm = (struct ifg_member *)malloc(sizeof(struct ifg_member),
	    M_TEMP, M_NOWAIT)) == NULL) {
		free(ifgl, M_TEMP);
		return (ENOMEM);
	}

	TAILQ_FOREACH(ifg, &ifg_head, ifg_next)
		if (!strcmp(ifg->ifg_group, groupname))
			break;

	if (ifg == NULL && (ifg = if_creategroup(groupname)) == NULL) {
		free(ifgl, M_TEMP);
		free(ifgm, M_TEMP);
		return (ENOMEM);
	}

	ifg->ifg_refcnt++;
	ifgl->ifgl_group = ifg;
	ifgm->ifgm_ifp = ifp;

	TAILQ_INSERT_TAIL(&ifg->ifg_members, ifgm, ifgm_next);
	TAILQ_INSERT_TAIL(ifgh, ifgl, ifgl_next);

#if NPF > 0
	pfi_group_change(groupname);
#endif

	return (0);
}

/*
 * Remove a group from an interface.
 */
int
if_delgroup(struct ifnet *ifp, const char *groupname)
{
	struct ifg_list_head	*ifgh = if_get_groups(ifp);
	struct ifg_list		*ifgl;
	struct ifg_member	*ifgm;

	TAILQ_FOREACH(ifgl, ifgh, ifgl_next)
		if (!strcmp(ifgl->ifgl_group->ifg_group, groupname))
			break;
	if (ifgl == NULL)
		return (ENOENT);

	TAILQ_REMOVE(ifgh, ifgl, ifgl_next);

	TAILQ_FOREACH(ifgm, &ifgl->ifgl_group->ifg_members, ifgm_next)
		if (ifgm->ifgm_ifp == ifp)
			break;

	if (ifgm != NULL) {
		TAILQ_REMOVE(&ifgl->ifgl_group->ifg_members, ifgm, ifgm_next);
		free(ifgm, M_TEMP);
	}

	if (--ifgl->ifgl_group->ifg_refcnt == 0) {
		TAILQ_REMOVE(&ifg_head, ifgl->ifgl_group, ifg_next);
#if NPF > 0
		pfi_detach_ifgroup(ifgl->ifgl_group);
#endif
		free(ifgl->ifgl_group, M_TEMP);
	}

	free(ifgl, M_TEMP);

#if NPF > 0
	pfi_group_change(groupname);
#endif

	return (0);
}

#if 0
/*
 * Stores all groups from an interface in memory pointed
 * to by data.
 */
static int
if_getgroup(void *data, struct ifnet *ifp)
{
	int			 len, error;
	struct ifg_list_head	*ifgh = if_get_groups(ifp);
	struct ifg_list		*ifgl;
	struct ifg_req		 ifgrq, *ifgp;
	struct ifgroupreq	*ifgr = (struct ifgroupreq *)data;

	if (ifgr->ifgr_len == 0) {
		TAILQ_FOREACH(ifgl, ifgh, ifgl_next)
			ifgr->ifgr_len += sizeof(struct ifg_req);
		return (0);
	}

	len = ifgr->ifgr_len;
	ifgp = ifgr->ifgr_groups;
	TAILQ_FOREACH(ifgl, ifgh, ifgl_next) {
		if (len < sizeof(ifgrq))
			return (EINVAL);
		bzero(&ifgrq, sizeof ifgrq);
		strlcpy(ifgrq.ifgrq_group, ifgl->ifgl_group->ifg_group,
		    sizeof(ifgrq.ifgrq_group));
		if ((error = copyout(&ifgrq, ifgp, sizeof(struct ifg_req))))
			return (error);
		len -= sizeof(ifgrq);
		ifgp++;
	}

	return (0);
}

/*
 * Stores all members of a group in memory pointed to by data.
 */
static int
if_getgroupmembers(void *data)
{
	struct ifgroupreq	*ifgr = (struct ifgroupreq *)data;
	struct ifg_group	*ifg;
	struct ifg_member	*ifgm;
	struct ifg_req		 ifgrq, *ifgp;
	int			 len, error;

	TAILQ_FOREACH(ifg, &ifg_head, ifg_next)
		if (!strcmp(ifg->ifg_group, ifgr->ifgr_name))
			break;
	if (ifg == NULL)
		return (ENOENT);

	if (ifgr->ifgr_len == 0) {
		TAILQ_FOREACH(ifgm, &ifg->ifg_members, ifgm_next)
			ifgr->ifgr_len += sizeof(ifgrq);
		return (0);
	}

	len = ifgr->ifgr_len;
	ifgp = ifgr->ifgr_groups;
	TAILQ_FOREACH(ifgm, &ifg->ifg_members, ifgm_next) {
		if (len < sizeof(ifgrq))
			return (EINVAL);
		bzero(&ifgrq, sizeof ifgrq);
		strlcpy(ifgrq.ifgrq_member, ifgm->ifgm_ifp->if_xname,
		    sizeof(ifgrq.ifgrq_member));
		if ((error = copyout(&ifgrq, ifgp, sizeof(struct ifg_req))))
			return (error);
		len -= sizeof(ifgrq);
		ifgp++;
	}

	return (0);
}

void
if_group_routechange(struct sockaddr *dst, struct sockaddr *mask)
{
	switch (dst->sa_family) {
	case AF_INET:
		if (satosin(dst)->sin_addr.s_addr == INADDR_ANY)
			if_group_egress_build();
		break;
#ifdef INET6
	case AF_INET6:
		if (IN6_ARE_ADDR_EQUAL(&(satosin6(dst))->sin6_addr,
		    &in6addr_any) &&
		    mask && IN6_ARE_ADDR_EQUAL(&(satosin6(mask))->sin6_addr,
		    &in6addr_any))
			if_group_egress_build();
		break;
#endif
	}
}

static int
if_group_egress_build(void)
{
	struct ifg_group	*ifg;
	struct ifg_member	*ifgm, *next;
	struct sockaddr_in	 sa_in;
#ifdef INET6
	struct sockaddr_in6	 sa_in6;
#endif
	struct radix_node	*rn;
	struct rtentry		*rt;

	TAILQ_FOREACH(ifg, &ifg_head, ifg_next)
		if (!strcmp(ifg->ifg_group, IFG_EGRESS))
			break;

	if (ifg != NULL)
		for (ifgm = TAILQ_FIRST(&ifg->ifg_members); ifgm; ifgm = next) {
			next = TAILQ_NEXT(ifgm, ifgm_next);
			if_delgroup(ifgm->ifgm_ifp, IFG_EGRESS);
		}

	bzero(&sa_in, sizeof(sa_in));
	sa_in.sin_len = sizeof(sa_in);
	sa_in.sin_family = AF_INET;
	if ((rn = rt_lookup(sintosa(&sa_in), sintosa(&sa_in), 0)) != NULL) {
		do {
			rt = (struct rtentry *)rn;
			if (rt->rt_ifp)
				if_addgroup(rt->rt_ifp, IFG_EGRESS);
#ifndef SMALL_KERNEL
			rn = rn_mpath_next(rn);
#else
			rn = NULL;
#endif
		} while (rn != NULL);
	}

#ifdef INET6
	bcopy(&sa6_any, &sa_in6, sizeof(sa_in6));
	if ((rn = rt_lookup(sin6tosa(&sa_in6), sin6tosa(&sa_in6), 0)) != NULL) {
		do {
			rt = (struct rtentry *)rn;
			if (rt->rt_ifp)
				if_addgroup(rt->rt_ifp, IFG_EGRESS);
#ifndef SMALL_KERNEL
			rn = rn_mpath_next(rn);
#else
			rn = NULL;
#endif
		} while (rn != NULL);
	}
#endif

	return (0);
}
#endif
