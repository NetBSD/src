/*	$NetBSD: rtsock.c,v 1.249 2019/04/29 05:42:09 pgoyette Exp $	*/

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
 * Copyright (c) 1988, 1991, 1993
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
 *	@(#)rtsock.c	8.7 (Berkeley) 10/12/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtsock.c,v 1.249 2019/04/29 05:42:09 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/intr.h>
#include <sys/condvar.h>
#include <sys/compat_stub.h>

#include <net/if.h>
#include <net/if_llatbl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/raw_cb.h>

#include <netinet/in_var.h>
#include <netinet/if_inarp.h>

#include <netmpls/mpls.h>

#include <compat/net/if.h>
#include <compat/net/route.h>

#ifdef COMPAT_RTSOCK
#undef COMPAT_RTSOCK
#endif

static int if_addrflags(struct ifaddr *);

#include <net/rtsock_shared.c>

/*
 * XXX avoid using void * once msghdr compat disappears.
 */
void
rt_setmetrics(void *in, struct rtentry *out)
{
	const struct rt_xmsghdr *rtm = in;

	_rt_setmetrics(rtm->rtm_inits, rtm, out);
}

int
rt_msg3(int type, struct rt_addrinfo *rtinfo, void *cpv, struct rt_walkarg *w,
	int *lenp)
{
	return rt_msg2(type, rtinfo, cpv, w, lenp);
}

static int
if_addrflags(struct ifaddr *ifa)
{

	switch (ifa->ifa_addr->sa_family) {
#ifdef INET
	case AF_INET:
		return ((struct in_ifaddr *)ifa)->ia4_flags;
#endif
#ifdef INET6
	case AF_INET6:
		return ((struct in6_ifaddr *)ifa)->ia6_flags;
#endif
	default:
		return 0;
	}
}


/*
 * Send a routing message as mimicing that a cloned route is added.
 */
void
rt_clonedmsg(const struct sockaddr *dst, const struct ifnet *ifp,
    const struct rtentry *rt)
{
	struct rt_addrinfo info;
	/* Mimic flags exactly */
#define RTF_LLINFO	0x400
#define RTF_CLONED	0x2000
	int flags = RTF_UP | RTF_HOST | RTF_DONE | RTF_LLINFO | RTF_CLONED;
	union {
		struct sockaddr sa;
		struct sockaddr_storage ss;
		struct sockaddr_dl sdl;
	} u;
	uint8_t namelen = strlen(ifp->if_xname);
	uint8_t addrlen = ifp->if_addrlen;

	if (rt == NULL)
		return; /* XXX */

	memset(&info, 0, sizeof(info));
	info.rti_info[RTAX_DST] = dst;
	sockaddr_dl_init(&u.sdl, sizeof(u.ss), ifp->if_index, ifp->if_type,
	    NULL, namelen, NULL, addrlen);
	info.rti_info[RTAX_GATEWAY] = &u.sa;

	rt_missmsg(RTM_ADD, &info, flags, 0);
#undef RTF_LLINFO
#undef RTF_CLONED
}


/*
 * The remaining code implements the routing-table sysctl node.  It is
 * compiled only for the non-COMPAT case.
 */

/*
 * This is used in dumping the kernel table via sysctl().
 */
static int
sysctl_dumpentry(struct rtentry *rt, void *v)
{
	struct rt_walkarg *w = v;
	int error = 0, size;
	struct rt_addrinfo info;

	if (w->w_op == NET_RT_FLAGS && !(rt->rt_flags & w->w_arg))
		return 0;
	memset(&info, 0, sizeof(info));
	info.rti_info[RTAX_DST] = rt_getkey(rt);
	info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
	info.rti_info[RTAX_NETMASK] = rt_mask(rt);
	info.rti_info[RTAX_TAG] = rt_gettag(rt);
	if (rt->rt_ifp) {
		const struct ifaddr *rtifa;
		info.rti_info[RTAX_IFP] = rt->rt_ifp->if_dl->ifa_addr;
		/* rtifa used to be simply rt->rt_ifa.  If rt->rt_ifa != NULL,
		 * then rt_get_ifa() != NULL.  So this ought to still be safe.
		 * --dyoung
		 */
		rtifa = rt_get_ifa(rt);
		info.rti_info[RTAX_IFA] = rtifa->ifa_addr;
		if (rt->rt_ifp->if_flags & IFF_POINTOPOINT)
			info.rti_info[RTAX_BRD] = rtifa->ifa_dstaddr;
	}
	if ((error = rt_msg2(RTM_GET, &info, 0, w, &size)))
		return error;
	if (w->w_where && w->w_tmem && w->w_needed <= 0) {
		struct rt_xmsghdr *rtm = (struct rt_xmsghdr *)w->w_tmem;

		rtm->rtm_flags = rt->rt_flags;
		rtm->rtm_use = rt->rt_use;
		rtm_setmetrics(rt, rtm);
		KASSERT(rt->rt_ifp != NULL);
		rtm->rtm_index = rt->rt_ifp->if_index;
		rtm->rtm_errno = rtm->rtm_pid = rtm->rtm_seq = 0;
		rtm->rtm_addrs = info.rti_addrs;
		if ((error = copyout(rtm, w->w_where, size)) != 0)
			w->w_where = NULL;
		else
			w->w_where = (char *)w->w_where + size;
	}
	return error;
}

static int
sysctl_iflist_if(struct ifnet *ifp, struct rt_walkarg *w,
    struct rt_addrinfo *info, size_t len)
{
	struct if_xmsghdr *ifm;
	int error;

	ifm = (struct if_xmsghdr *)w->w_tmem;
	ifm->ifm_index = ifp->if_index;
	ifm->ifm_flags = ifp->if_flags;
	ifm->ifm_data = ifp->if_data;
	ifm->ifm_addrs = info->rti_addrs;
	if ((error = copyout(ifm, w->w_where, len)) == 0)
		w->w_where = (char *)w->w_where + len;
	return error;
}

static int
sysctl_iflist_addr(struct rt_walkarg *w, struct ifaddr *ifa,
     struct rt_addrinfo *info)
{
	int len, error;

	if ((error = rt_msg2(RTM_XNEWADDR, info, 0, w, &len)))
		return error;
	if (w->w_where && w->w_tmem && w->w_needed <= 0) {
		struct ifa_xmsghdr *ifam;

		ifam = (struct ifa_xmsghdr *)w->w_tmem;
		ifam->ifam_index = ifa->ifa_ifp->if_index;
		ifam->ifam_flags = ifa->ifa_flags;
		ifam->ifam_metric = ifa->ifa_metric;
		ifam->ifam_addrs = info->rti_addrs;
		ifam->ifam_pid = 0;
		ifam->ifam_addrflags = if_addrflags(ifa);
		if ((error = copyout(w->w_tmem, w->w_where, len)) == 0)
			w->w_where = (char *)w->w_where + len;
	}
	return error;
}

static int
sysctl_iflist(int af, struct rt_walkarg *w, int type)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct	rt_addrinfo info;
	int	cmd, len, error = 0;
	int s;
	struct psref psref;
	int bound;

	switch (type) {
	case NET_RT_IFLIST:
		cmd = RTM_IFINFO;
		break;
	case NET_RT_OOOIFLIST:
		cmd = RTM_OOIFINFO;
		break;
	case NET_RT_OOIFLIST:
		cmd = RTM_OIFINFO;
		break;
	case NET_RT_OIFLIST:
		cmd = RTM_IFINFO;
		break;
	default:
#ifdef RTSOCK_DEBUG
		printf("%s: unsupported IFLIST type %d\n", __func__, type);
#endif
		return EINVAL;
	}

	memset(&info, 0, sizeof(info));

	bound = curlwp_bind();
	s = pserialize_read_enter();
	IFNET_READER_FOREACH(ifp) {
		int _s;
		if (w->w_arg && w->w_arg != ifp->if_index)
			continue;
		if (IFADDR_READER_EMPTY(ifp))
			continue;

		if_acquire(ifp, &psref);
		pserialize_read_exit(s);

		info.rti_info[RTAX_IFP] = ifp->if_dl->ifa_addr;
		if ((error = rt_msg2(cmd, &info, NULL, w, &len)) != 0)
			goto release_exit;
		info.rti_info[RTAX_IFP] = NULL;
		if (w->w_where && w->w_tmem && w->w_needed <= 0) {
			switch (type) {
			case NET_RT_OIFLIST: /* old _70 */
				if (!rtsock_iflist_70_hook.hooked) {
					error = EINVAL;
					break;
				}
				/* FALLTHROUGH */
			case NET_RT_IFLIST: /* current */
				error = sysctl_iflist_if(ifp, w, &info, len);
				break;
			case NET_RT_OOIFLIST: /* old _50 */
				MODULE_HOOK_CALL(rtsock_iflist_50_hook,
				    (ifp, w, &info, len), enosys(), error);
				break;
			case NET_RT_OOOIFLIST: /* old _14 */
				MODULE_HOOK_CALL(rtsock_iflist_14_hook,
				   (ifp, w, &info, len), enosys(), error);
				break;
			default:
				error = EINVAL;
			}
			if (error != 0) {
				if (error == ENOSYS)
					error = EINVAL;
				goto release_exit;
			}
		}
		_s = pserialize_read_enter();
		IFADDR_READER_FOREACH(ifa, ifp) {
			struct psref _psref;
			if (af && af != ifa->ifa_addr->sa_family)
				continue;
			ifa_acquire(ifa, &_psref);
			pserialize_read_exit(_s);

			info.rti_info[RTAX_IFA] = ifa->ifa_addr;
			info.rti_info[RTAX_NETMASK] = ifa->ifa_netmask;
			info.rti_info[RTAX_BRD] = ifa->ifa_dstaddr;
			switch (type) {
			case NET_RT_IFLIST:
				error = sysctl_iflist_addr(w, ifa, &info);
				break;
			case NET_RT_OIFLIST:
			case NET_RT_OOIFLIST:
			case NET_RT_OOOIFLIST:
				MODULE_HOOK_CALL(rtsock_iflist_70_hook,
				    (w, ifa, &info), enosys(), error);
				break;
			default:
				error = EINVAL;
			}

			_s = pserialize_read_enter();
			ifa_release(ifa, &_psref);
			if (error != 0) {
				pserialize_read_exit(_s);
				goto release_exit;
			}
		}
		pserialize_read_exit(_s);
		info.rti_info[RTAX_IFA] = info.rti_info[RTAX_NETMASK] =
		    info.rti_info[RTAX_BRD] = NULL;

		s = pserialize_read_enter();
		if_release(ifp, &psref);
	}
	pserialize_read_exit(s);
	curlwp_bindx(bound);

	return 0;

release_exit:
	if_release(ifp, &psref);
	curlwp_bindx(bound);
	return error;
}

static int
sysctl_rtable(SYSCTLFN_ARGS)
{
	void 	*where = oldp;
	size_t	*given = oldlenp;
	int	i, s, error = EINVAL;
	u_char  af;
	struct	rt_walkarg w;

	if (namelen == 1 && name[0] == CTL_QUERY)
		return sysctl_query(SYSCTLFN_CALL(rnode));

	if (newp)
		return EPERM;
	if (namelen != 3)
		return EINVAL;
	af = name[0];
	w.w_tmemneeded = 0;
	w.w_tmemsize = 0;
	w.w_tmem = NULL;
again:
	/* we may return here if a later [re]alloc of the t_mem buffer fails */
	if (w.w_tmemneeded) {
		w.w_tmem = kmem_zalloc(w.w_tmemneeded, KM_SLEEP);
		w.w_tmemsize = w.w_tmemneeded;
		w.w_tmemneeded = 0;
	}
	w.w_op = name[1];
	w.w_arg = name[2];
	w.w_given = *given;
	w.w_needed = 0 - w.w_given;
	w.w_where = where;

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();
	s = splsoftnet();
	switch (w.w_op) {

	case NET_RT_DUMP:
	case NET_RT_FLAGS:
#if defined(INET) || defined(INET6)
		/*
		 * take care of llinfo entries, the caller must
		 * specify an AF
		 */
		if (w.w_op == NET_RT_FLAGS &&
		    (w.w_arg == 0 || w.w_arg & RTF_LLDATA)) {
			if (af != 0)
				error = lltable_sysctl_dump(af, &w);
			else
				error = EINVAL;
			break;
		}
#endif

		for (i = 1; i <= AF_MAX; i++) {
			if (af == 0 || af == i) {
				error = rt_walktree(i, sysctl_dumpentry, &w);
				if (error != 0)
					break;
#if defined(INET) || defined(INET6)
				/*
				 * Return ARP/NDP entries too for
				 * backward compatibility.
				 */
				error = lltable_sysctl_dump(i, &w);
				if (error != 0)
					break;
#endif
			}
		}
		break;

	case NET_RT_OOOIFLIST:		/* compat_14 */
	case NET_RT_OOIFLIST:		/* compat_50 */
	case NET_RT_OIFLIST:		/* compat_70 */
	case NET_RT_IFLIST:		/* current */
		error = sysctl_iflist(af, &w, w.w_op);
		break;
	}
	splx(s);
	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();

	/* check to see if we couldn't allocate memory with NOWAIT */
	if (error == ENOBUFS && w.w_tmem == 0 && w.w_tmemneeded)
		goto again;

	if (w.w_tmem)
		kmem_free(w.w_tmem, w.w_tmemsize);
	w.w_needed += w.w_given;
	if (where) {
		*given = (char *)w.w_where - (char *)where;
		if (*given < w.w_needed)
			return ENOMEM;
	} else {
		*given = (11 * w.w_needed) / 10;
	}
	return error;
}

void
sysctl_net_route_setup(struct sysctllog **clog, int pf, const char *name)
{
	const struct sysctlnode *rnode = NULL;

	sysctl_createv(clog, 0, NULL, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, name,
		       SYSCTL_DESCR("PF_ROUTE information"),
		       NULL, 0, NULL, 0,
		       CTL_NET, pf, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "rtable",
		       SYSCTL_DESCR("Routing table information"),
		       sysctl_rtable, 0, NULL, 0,
		       CTL_NET, pf, 0 /* any protocol */, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "stats",
		       SYSCTL_DESCR("Routing statistics"),
		       NULL, 0, &rtstat, sizeof(rtstat),
		       CTL_CREATE, CTL_EOL);
}
