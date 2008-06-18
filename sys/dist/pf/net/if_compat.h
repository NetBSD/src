/*	$OpenBSD: if.h,v 1.91 2007/06/25 16:37:58 henning Exp $	*/
/*	$NetBSD: if_compat.h,v 1.1.6.1 2008/06/18 16:33:34 simonb Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)if.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _NET_IF_COMPAT_H_
#define _NET_IF_COMPAT_H_

#include <sys/queue.h>

/*
 * interface groups
 */

#define	IFG_ALL		"all"		/* group contains all interfaces */
#define	IFG_EGRESS	"egress"	/* if(s) default route(s) point to */

struct ifg_group {
	char				 ifg_group[IFNAMSIZ];
	u_int				 ifg_refcnt;
	void				*ifg_pf_kif;
	int				 ifg_carp_demoted;
	TAILQ_HEAD(, ifg_member)	 ifg_members;
	TAILQ_ENTRY(ifg_group)		 ifg_next;
};

struct ifg_member {
	TAILQ_ENTRY(ifg_member)	 ifgm_next;
	struct ifnet		*ifgm_ifp;
};

struct ifg_list {
	struct ifg_group	*ifgl_group;
	TAILQ_ENTRY(ifg_list)	 ifgl_next;
};

TAILQ_HEAD(ifg_list_head, ifg_list);

struct ifg_req {
	union {
		char			 ifgrqu_group[IFNAMSIZ];
		char			 ifgrqu_member[IFNAMSIZ];
	} ifgrq_ifgrqu;
#define	ifgrq_group	ifgrq_ifgrqu.ifgrqu_group
#define	ifgrq_member	ifgrq_ifgrqu.ifgrqu_member
};

struct ifg_attrib {
	int	ifg_carp_demoted;
};

/*
 * Used to lookup groups for an interface
 */
struct ifgroupreq {
	char	ifgr_name[IFNAMSIZ];
	u_int	ifgr_len;
	union {
		char			 ifgru_group[IFNAMSIZ];
		struct	ifg_req		*ifgru_groups;
		struct	ifg_attrib	 ifgru_attrib;
	} ifgr_ifgru;
#define ifgr_group	ifgr_ifgru.ifgru_group
#define ifgr_groups	ifgr_ifgru.ifgru_groups
#define ifgr_attrib	ifgr_ifgru.ifgru_attrib
};

#ifdef _KERNEL
void	if_init_groups(struct ifnet *);
void	if_destroy_groups(struct ifnet *);
struct	ifg_list_head *if_get_groups(struct ifnet *);

struct	ifg_group *if_creategroup(const char *);
int	if_addgroup(struct ifnet *, const char *);
int	if_delgroup(struct ifnet *, const char *);
void	if_group_routechange(struct sockaddr *, struct sockaddr *);
#endif /* _KERNEL */

#endif /* !_NET_IF_COMPAT_H_ */
