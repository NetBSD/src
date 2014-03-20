/*	$NetBSD: altq_afmap.c,v 1.20 2014/03/20 20:51:54 christos Exp $	*/
/*	$KAME: altq_afmap.c,v 1.12 2005/04/13 03:44:24 suz Exp $	*/

/*
 * Copyright (C) 1997-2002
 *	Sony Computer Science Laboratories Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY SONY CSL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL SONY CSL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * experimental:
 * mapping an ip flow to atm vpi/vci.
 * this module is not related to queueing at all, but uses the altq
 * flowinfo mechanism.  it's just put in the altq framework since
 * it is easy to add devices to altq.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: altq_afmap.c,v 1.20 2014/03/20 20:51:54 christos Exp $");

#ifdef _KERNEL_OPT
#include "opt_altq.h"
#include "opt_inet.h"
#endif

#ifdef ALTQ_AFMAP

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/kauth.h>

#include <net/if.h>
#include <net/if_types.h>
#include <netinet/in.h>

#include <altq/altq.h>
#include <altq/altq_conf.h>
#include <altq/altq_afmap.h>

#ifdef ALTQ3_COMPAT

LIST_HEAD(, afm_head) afhead_chain;

static struct afm *afm_match4(struct afm_head *, struct flowinfo_in *);
#ifdef INET6
static struct afm *afm_match6(struct afm_head *, struct flowinfo_in6 *);
#endif

/*
 * rules to block interrupts: afm_match can be called from a net
 * level interrupt so that other routines handling the lists should
 * be called in splnet().
 */
int
afm_alloc(struct ifnet *ifp)
{
	struct afm_head *head;

	head = malloc(sizeof(struct afm_head), M_DEVBUF, M_WAITOK|M_ZERO);
	if (head == NULL)
		panic("afm_alloc: malloc failed!");

	/* initialize per interface afmap list */
	LIST_INIT(&head->afh_head);

	head->afh_ifp = ifp;

	/* add this afm_head to the chain */
	LIST_INSERT_HEAD(&afhead_chain, head, afh_chain);

	return (0);
}

int
afm_dealloc(struct ifnet *ifp)
{
	struct afm_head *head;

	for (head = afhead_chain.lh_first; head != NULL;
	     head = head->afh_chain.le_next)
		if (head->afh_ifp == ifp)
			break;
	if (head == NULL)
		return (-1);

	afm_removeall(ifp);

	LIST_REMOVE(head, afh_chain);

	free(head, M_DEVBUF);
	return 0;
}

struct afm *
afm_top(struct ifnet *ifp)
{
	struct afm_head *head;

	for (head = afhead_chain.lh_first; head != NULL;
	     head = head->afh_chain.le_next)
		if (head->afh_ifp == ifp)
			break;
	if (head == NULL)
		return NULL;

	return (head->afh_head.lh_first);
}

int
afm_add(struct ifnet *ifp, struct atm_flowmap *flowmap)
{
	struct afm_head *head;
	struct afm *afm;

	for (head = afhead_chain.lh_first; head != NULL;
	     head = head->afh_chain.le_next)
		if (head->afh_ifp == ifp)
			break;
	if (head == NULL)
		return (-1);

	if (flowmap->af_flowinfo.fi_family == AF_INET) {
		if (flowmap->af_flowinfo.fi_len != sizeof(struct flowinfo_in))
			return (EINVAL);
#ifdef INET6
	} else if (flowmap->af_flowinfo.fi_family == AF_INET6) {
		if (flowmap->af_flowinfo.fi_len != sizeof(struct flowinfo_in6))
			return (EINVAL);
#endif
	} else
		return (EINVAL);

	afm = malloc(sizeof(struct afm), M_DEVBUF, M_WAITOK|M_ZERO);
	if (afm == NULL)
		return (ENOMEM);

	afm->afm_vci = flowmap->af_vci;
	afm->afm_vpi = flowmap->af_vpi;
	(void)memcpy(&afm->afm_flowinfo, &flowmap->af_flowinfo,
	      flowmap->af_flowinfo.fi_len);

	LIST_INSERT_HEAD(&head->afh_head, afm, afm_list);
	return 0;
}

int
afm_remove(struct afm *afm)
{
	LIST_REMOVE(afm, afm_list);
	free(afm, M_DEVBUF);
	return (0);
}

int
afm_removeall(struct ifnet *ifp)
{
	struct afm_head *head;
	struct afm *afm;

	for (head = afhead_chain.lh_first; head != NULL;
	     head = head->afh_chain.le_next)
		if (head->afh_ifp == ifp)
			break;
	if (head == NULL)
		return (-1);

	while ((afm = head->afh_head.lh_first) != NULL)
		afm_remove(afm);
	return (0);
}

struct afm *
afm_lookup(struct ifnet *ifp, int vpi, int vci)
{
	struct afm_head *head;
	struct afm *afm;

	for (head = afhead_chain.lh_first; head != NULL;
	     head = head->afh_chain.le_next)
		if (head->afh_ifp == ifp)
			break;
	if (head == NULL)
		return NULL;

	for (afm = head->afh_head.lh_first; afm != NULL;
	     afm = afm->afm_list.le_next)
		if (afm->afm_vpi == vpi && afm->afm_vci == vci)
			break;
	return afm;
}

static struct afm *
afm_match4(struct afm_head *head, struct flowinfo_in *fp)
{
	struct afm *afm;

	for (afm = head->afh_head.lh_first; afm != NULL;
	     afm = afm->afm_list.le_next) {
		if (afm->afm_flowinfo4.fi_dst.s_addr != 0 &&
		    afm->afm_flowinfo4.fi_dst.s_addr != fp->fi_dst.s_addr)
			continue;
		if (afm->afm_flowinfo4.fi_dport != 0 &&
		    afm->afm_flowinfo4.fi_dport != fp->fi_dport)
			continue;
		if (afm->afm_flowinfo4.fi_src.s_addr != 0 &&
		    afm->afm_flowinfo4.fi_src.s_addr != fp->fi_src.s_addr)
			continue;
		if (afm->afm_flowinfo4.fi_sport != 0 &&
		    afm->afm_flowinfo4.fi_sport != fp->fi_sport)
			continue;
		if (afm->afm_flowinfo4.fi_proto != 0 &&
		    afm->afm_flowinfo4.fi_proto != fp->fi_proto)
			continue;
		/* match found! */
		return (afm);
	}
	return NULL;
}

#ifdef INET6
static struct afm *
afm_match6(struct afm_head *head, struct flowinfo_in6 *fp)
{
	struct afm *afm;

	for (afm = head->afh_head.lh_first; afm != NULL;
	     afm = afm->afm_list.le_next) {
		if (afm->afm_flowinfo6.fi6_flowlabel != 0 &&
		    afm->afm_flowinfo6.fi6_flowlabel != fp->fi6_flowlabel)
			continue;
#ifdef notyet
		if (!IN6_IS_ADDR_UNSPECIFIED(&afm->afm_flowinfo6.fi6_dst) &&
		    !IN6_ARE_ADDR_EQUAL(&afm->afm_flowinfo6.fi6_dst,
					&fp->fi6_dst))
			continue;
		if (afm->afm_flowinfo6.fi6_dport != 0 &&
		    afm->afm_flowinfo6.fi6_dport != fp->fi6_dport)
			continue;
#endif
		if (!IN6_IS_ADDR_UNSPECIFIED(&afm->afm_flowinfo6.fi6_src) &&
		    !IN6_ARE_ADDR_EQUAL(&afm->afm_flowinfo6.fi6_src,
					&fp->fi6_src))
			continue;
#ifdef notyet
		if (afm->afm_flowinfo6.fi6_sport != 0 &&
		    afm->afm_flowinfo6.fi6_sport != fp->fi6_sport)
			continue;
#endif
		if (afm->afm_flowinfo6.fi6_proto != 0 &&
		    afm->afm_flowinfo6.fi6_proto != fp->fi6_proto)
			continue;
		/* match found! */
		return (afm);
	}
	return NULL;
}
#endif

/* should be called in splnet() */
struct afm *
afm_match(struct ifnet *ifp, struct flowinfo *flow)
{
	struct afm_head *head;

	for (head = afhead_chain.lh_first; head != NULL;
	     head = head->afh_chain.le_next)
		if (head->afh_ifp == ifp)
			break;
	if (head == NULL)
		return NULL;

	switch (flow->fi_family) {
	case AF_INET:
		return (afm_match4(head, (struct flowinfo_in *)flow));

#ifdef INET6
	case AF_INET6:
		return (afm_match6(head, (struct flowinfo_in6 *)flow));
#endif

	default:
		return NULL;
	}
}

/*
 * afm device interface
 */
altqdev_decl(afm);

int
afmopen(dev_t dev, int flag, int fmt,
    struct lwp *l)
{
	return 0;
}

int
afmclose(dev_t dev, int flag, int fmt, struct lwp *l)
{
	int err, error = 0;
	struct atm_flowmap fmap;
	struct afm_head *head;

	for (head = afhead_chain.lh_first; head != NULL;
	     head = head->afh_chain.le_next) {

		/* call interface to clean up maps */
		snprintf(fmap.af_ifname, sizeof(fmap.af_ifname),
		    "%s", head->afh_ifp->if_xname);
		err = afmioctl(dev, AFM_CLEANFMAP, (void *)&fmap, flag, l);
		if (err && error == 0)
			error = err;
	}

	return error;
}

int
afmioctl(dev_t dev, ioctlcmd_t cmd, void *addr, int flag,
    struct lwp *l)
{
	int	error = 0;
	struct atm_flowmap *flowmap;
	struct ifnet *ifp;

	/* check cmd for superuser only */
	switch (cmd) {
	case AFM_GETFMAP:
		break;
	default:
#if (__FreeBSD_version > 400000)
		error = suser(p);
#else
		error = kauth_authorize_network(l->l_cred, KAUTH_NETWORK_ALTQ,
		    KAUTH_REQ_NETWORK_ALTQ_AFMAP, NULL, NULL, NULL);
#endif
		if (error)
			return (error);
		break;
	}

	/* lookup interface */
	flowmap = (struct atm_flowmap *)addr;
	flowmap->af_ifname[IFNAMSIZ-1] = '\0';
	ifp = ifunit(flowmap->af_ifname);
	if (ifp == NULL || (ifp->if_flags & IFF_RUNNING) == 0)
		error = ENXIO;
	else
		error = ifp->if_ioctl(ifp, cmd, addr);

	return error;
}

#endif /* ALTQ3_COMPAT */
#endif /* ALTQ_AFMAP */
