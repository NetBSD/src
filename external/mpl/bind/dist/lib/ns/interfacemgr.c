/*	$NetBSD: interfacemgr.c,v 1.14.2.2 2024/02/25 15:47:34 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <stdbool.h>

#include <isc/interfaceiter.h>
#include <isc/netmgr.h>
#include <isc/os.h>
#include <isc/random.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/acl.h>
#include <dns/dispatch.h>

#include <ns/client.h>
#include <ns/interfacemgr.h>
#include <ns/log.h>
#include <ns/server.h>
#include <ns/stats.h>

#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#if defined(RTM_VERSION) && defined(RTM_NEWADDR) && defined(RTM_DELADDR)
#define MSGHDR	rt_msghdr
#define MSGTYPE rtm_type
#endif /* if defined(RTM_VERSION) && defined(RTM_NEWADDR) && \
	* defined(RTM_DELADDR) */
#endif /* ifdef HAVE_NET_ROUTE_H */

#if defined(HAVE_LINUX_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)
#define LINUX_NETLINK_AVAILABLE
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#if defined(RTM_NEWADDR) && defined(RTM_DELADDR)
#define MSGHDR	nlmsghdr
#define MSGTYPE nlmsg_type
#endif /* if defined(RTM_NEWADDR) && defined(RTM_DELADDR) */
#endif /* if defined(HAVE_LINUX_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H) \
	*/

#define LISTENING(ifp) (((ifp)->flags & NS_INTERFACEFLAG_LISTENING) != 0)

#ifdef TUNE_LARGE
#define UDPBUFFERS 32768
#else /* ifdef TUNE_LARGE */
#define UDPBUFFERS 1000
#endif /* TUNE_LARGE */

#define IFMGR_MAGIC		 ISC_MAGIC('I', 'F', 'M', 'G')
#define NS_INTERFACEMGR_VALID(t) ISC_MAGIC_VALID(t, IFMGR_MAGIC)

#define IFMGR_COMMON_LOGARGS \
	ns_lctx, NS_LOGCATEGORY_NETWORK, NS_LOGMODULE_INTERFACEMGR

/*% nameserver interface manager structure */
struct ns_interfacemgr {
	unsigned int magic; /*%< Magic number */
	isc_refcount_t references;
	isc_mutex_t lock;
	isc_mem_t *mctx;	  /*%< Memory context */
	ns_server_t *sctx;	  /*%< Server context */
	isc_taskmgr_t *taskmgr;	  /*%< Task manager */
	isc_task_t *task;	  /*%< Task */
	isc_timermgr_t *timermgr; /*%< Timer manager */
	isc_nm_t *nm;		  /*%< Net manager */
	int ncpus;		  /*%< Number of workers */
	dns_dispatchmgr_t *dispatchmgr;
	unsigned int generation; /*%< Current generation no */
	ns_listenlist_t *listenon4;
	ns_listenlist_t *listenon6;
	dns_aclenv_t *aclenv;		     /*%< Localhost/localnets ACLs */
	ISC_LIST(ns_interface_t) interfaces; /*%< List of interfaces */
	ISC_LIST(isc_sockaddr_t) listenon;
	int backlog;		     /*%< Listen queue size */
	atomic_bool shuttingdown;    /*%< Interfacemgr shutting down */
	ns_clientmgr_t **clientmgrs; /*%< Client managers */
	isc_nmhandle_t *route;
};

static void
purge_old_interfaces(ns_interfacemgr_t *mgr);

static void
clearlistenon(ns_interfacemgr_t *mgr);

static bool
need_rescan(ns_interfacemgr_t *mgr, struct MSGHDR *rtm, size_t len) {
	if (rtm->MSGTYPE != RTM_NEWADDR && rtm->MSGTYPE != RTM_DELADDR) {
		return (false);
	}

#ifndef LINUX_NETLINK_AVAILABLE
	UNUSED(mgr);
	UNUSED(len);
	/* On most systems, any NEWADDR or DELADDR means we rescan */
	return (true);
#else  /* LINUX_NETLINK_AVAILABLE */
	/* ...but on linux we need to check the messages more carefully */
	for (struct MSGHDR *nlh = rtm;
	     NLMSG_OK(nlh, len) && nlh->nlmsg_type != NLMSG_DONE;
	     nlh = NLMSG_NEXT(nlh, len))
	{
		struct ifaddrmsg *ifa = (struct ifaddrmsg *)NLMSG_DATA(nlh);
		struct rtattr *rth = IFA_RTA(ifa);
		size_t rtl = IFA_PAYLOAD(nlh);

		while (rtl > 0 && RTA_OK(rth, rtl)) {
			/*
			 * Look for IFA_ADDRESS to detect IPv6 interface
			 * state changes.
			 */
			if (rth->rta_type == IFA_ADDRESS &&
			    ifa->ifa_family == AF_INET6)
			{
				bool existed = false;
				bool was_listening = false;
				isc_netaddr_t addr = { 0 };
				ns_interface_t *ifp = NULL;

				isc_netaddr_fromin6(&addr, RTA_DATA(rth));
				INSIST(isc_netaddr_getzone(&addr) == 0);

				/*
				 * Check whether we were listening on the
				 * address. We need to do this as the
				 * Linux kernel seems to issue messages
				 * containing IFA_ADDRESS far more often
				 * than the actual state changes (on
				 * router advertisements?)
				 */
				LOCK(&mgr->lock);
				for (ifp = ISC_LIST_HEAD(mgr->interfaces);
				     ifp != NULL;
				     ifp = ISC_LIST_NEXT(ifp, link))
				{
					isc_netaddr_t tmp = { 0 };
					isc_netaddr_fromsockaddr(&tmp,
								 &ifp->addr);
					if (tmp.family != AF_INET6) {
						continue;
					}

					/*
					 * We have to nullify the zone (IPv6
					 * scope ID) because we haven't got one
					 * from the kernel. Otherwise match
					 * could fail even for an existing
					 * address.
					 */
					isc_netaddr_setzone(&tmp, 0);
					if (isc_netaddr_equal(&tmp, &addr)) {
						was_listening = LISTENING(ifp);
						existed = true;
						break;
					}
				}
				UNLOCK(&mgr->lock);

				/*
				 * Do rescan if the state of the interface
				 * has changed.
				 */
				if ((!existed && rtm->MSGTYPE == RTM_NEWADDR) ||
				    (existed && was_listening &&
				     rtm->MSGTYPE == RTM_DELADDR))
				{
					return (true);
				}
			} else if (rth->rta_type == IFA_ADDRESS &&
				   ifa->ifa_family == AF_INET)
			{
				/*
				 * It seems that the IPv4 P2P link state
				 * has changed.
				 */
				return (true);
			} else if (rth->rta_type == IFA_LOCAL) {
				/*
				 * Local address state has changed - do
				 * rescan.
				 */
				return (true);
			}
			rth = RTA_NEXT(rth, rtl);
		}
	}
#endif /* LINUX_NETLINK_AVAILABLE */

	return (false);
}

static void
route_recv(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	   void *arg) {
	ns_interfacemgr_t *mgr = (ns_interfacemgr_t *)arg;
	struct MSGHDR *rtm = NULL;
	size_t rtmlen;

	isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_DEBUG(9), "route_recv: %s",
		      isc_result_totext(eresult));

	if (handle == NULL) {
		return;
	}

	if (eresult != ISC_R_SUCCESS) {
		if (eresult != ISC_R_CANCELED && eresult != ISC_R_SHUTTINGDOWN)
		{
			isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
				      "automatic interface scanning "
				      "terminated: %s",
				      isc_result_totext(eresult));
		}
		isc_nmhandle_detach(&mgr->route);
		ns_interfacemgr_detach(&mgr);
		return;
	}

	rtm = (struct MSGHDR *)region->base;
	rtmlen = region->length;

#ifdef RTM_VERSION
	if (rtm->rtm_version != RTM_VERSION) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "automatic interface rescanning disabled: "
			      "rtm->rtm_version mismatch (%u != %u) "
			      "recompile required",
			      rtm->rtm_version, RTM_VERSION);
		isc_nmhandle_detach(&mgr->route);
		ns_interfacemgr_detach(&mgr);
		return;
	}
#endif /* ifdef RTM_VERSION */

	REQUIRE(mgr->route != NULL);

	if (need_rescan(mgr, rtm, rtmlen) && mgr->sctx->interface_auto) {
		ns_interfacemgr_scan(mgr, false, false);
	}

	isc_nm_read(handle, route_recv, mgr);
	return;
}

static void
route_connected(isc_nmhandle_t *handle, isc_result_t eresult, void *arg) {
	ns_interfacemgr_t *mgr = (ns_interfacemgr_t *)arg;

	isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_DEBUG(9),
		      "route_connected: %s", isc_result_totext(eresult));

	if (eresult != ISC_R_SUCCESS) {
		ns_interfacemgr_detach(&mgr);
		return;
	}

	INSIST(mgr->route == NULL);

	isc_nmhandle_attach(handle, &mgr->route);
	isc_nm_read(handle, route_recv, mgr);
}

isc_result_t
ns_interfacemgr_create(isc_mem_t *mctx, ns_server_t *sctx,
		       isc_taskmgr_t *taskmgr, isc_timermgr_t *timermgr,
		       isc_nm_t *nm, dns_dispatchmgr_t *dispatchmgr,
		       isc_task_t *task, dns_geoip_databases_t *geoip,
		       int ncpus, bool scan, ns_interfacemgr_t **mgrp) {
	isc_result_t result;
	ns_interfacemgr_t *mgr = NULL;

	UNUSED(task);

	REQUIRE(mctx != NULL);
	REQUIRE(mgrp != NULL);
	REQUIRE(*mgrp == NULL);

	mgr = isc_mem_get(mctx, sizeof(*mgr));
	*mgr = (ns_interfacemgr_t){ .taskmgr = taskmgr,
				    .timermgr = timermgr,
				    .nm = nm,
				    .dispatchmgr = dispatchmgr,
				    .generation = 1,
				    .ncpus = ncpus };

	isc_mem_attach(mctx, &mgr->mctx);
	ns_server_attach(sctx, &mgr->sctx);

	isc_mutex_init(&mgr->lock);

	result = isc_task_create_bound(taskmgr, 0, &mgr->task, 0);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_lock;
	}

	atomic_init(&mgr->shuttingdown, false);

	ISC_LIST_INIT(mgr->interfaces);
	ISC_LIST_INIT(mgr->listenon);

	/*
	 * The listen-on lists are initially empty.
	 */
	result = ns_listenlist_create(mctx, &mgr->listenon4);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_task;
	}
	ns_listenlist_attach(mgr->listenon4, &mgr->listenon6);

	result = dns_aclenv_create(mctx, &mgr->aclenv);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_listenon;
	}
#if defined(HAVE_GEOIP2)
	mgr->aclenv->geoip = geoip;
#else  /* if defined(HAVE_GEOIP2) */
	UNUSED(geoip);
#endif /* if defined(HAVE_GEOIP2) */

	isc_refcount_init(&mgr->references, 1);
	mgr->magic = IFMGR_MAGIC;
	*mgrp = mgr;

	mgr->clientmgrs = isc_mem_get(mgr->mctx,
				      mgr->ncpus * sizeof(mgr->clientmgrs[0]));
	for (size_t i = 0; i < (size_t)mgr->ncpus; i++) {
		result = ns_clientmgr_create(mgr->sctx, mgr->taskmgr,
					     mgr->timermgr, mgr->aclenv, (int)i,
					     &mgr->clientmgrs[i]);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
	}

	if (scan) {
		ns_interfacemgr_t *imgr = NULL;

		ns_interfacemgr_attach(mgr, &imgr);

		result = isc_nm_routeconnect(nm, route_connected, imgr, 0);
		if (result == ISC_R_NOTIMPLEMENTED) {
			ns_interfacemgr_detach(&imgr);
		}
		if (result != ISC_R_SUCCESS) {
			isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_INFO,
				      "unable to open route socket: %s",
				      isc_result_totext(result));
		}
	}

	return (ISC_R_SUCCESS);

cleanup_listenon:
	ns_listenlist_detach(&mgr->listenon4);
	ns_listenlist_detach(&mgr->listenon6);
cleanup_task:
	isc_task_detach(&mgr->task);
cleanup_lock:
	isc_mutex_destroy(&mgr->lock);
	ns_server_detach(&mgr->sctx);
	isc_mem_putanddetach(&mgr->mctx, mgr, sizeof(*mgr));
	return (result);
}

static void
ns_interfacemgr_destroy(ns_interfacemgr_t *mgr) {
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	isc_refcount_destroy(&mgr->references);

	dns_aclenv_detach(&mgr->aclenv);
	ns_listenlist_detach(&mgr->listenon4);
	ns_listenlist_detach(&mgr->listenon6);
	clearlistenon(mgr);
	isc_mutex_destroy(&mgr->lock);
	for (size_t i = 0; i < (size_t)mgr->ncpus; i++) {
		ns_clientmgr_detach(&mgr->clientmgrs[i]);
	}
	isc_mem_put(mgr->mctx, mgr->clientmgrs,
		    mgr->ncpus * sizeof(mgr->clientmgrs[0]));

	if (mgr->sctx != NULL) {
		ns_server_detach(&mgr->sctx);
	}
	isc_task_detach(&mgr->task);
	mgr->magic = 0;
	isc_mem_putanddetach(&mgr->mctx, mgr, sizeof(*mgr));
}

void
ns_interfacemgr_setbacklog(ns_interfacemgr_t *mgr, int backlog) {
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));
	LOCK(&mgr->lock);
	mgr->backlog = backlog;
	UNLOCK(&mgr->lock);
}

dns_aclenv_t *
ns_interfacemgr_getaclenv(ns_interfacemgr_t *mgr) {
	dns_aclenv_t *aclenv = NULL;

	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	LOCK(&mgr->lock);
	aclenv = mgr->aclenv;
	UNLOCK(&mgr->lock);

	return (aclenv);
}

void
ns_interfacemgr_attach(ns_interfacemgr_t *source, ns_interfacemgr_t **target) {
	REQUIRE(NS_INTERFACEMGR_VALID(source));
	isc_refcount_increment(&source->references);
	*target = source;
}

void
ns_interfacemgr_detach(ns_interfacemgr_t **targetp) {
	ns_interfacemgr_t *target = *targetp;
	*targetp = NULL;
	REQUIRE(target != NULL);
	REQUIRE(NS_INTERFACEMGR_VALID(target));
	if (isc_refcount_decrement(&target->references) == 1) {
		ns_interfacemgr_destroy(target);
	}
}

void
ns_interfacemgr_shutdown(ns_interfacemgr_t *mgr) {
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	/*%
	 * Shut down and detach all interfaces.
	 * By incrementing the generation count, we make
	 * purge_old_interfaces() consider all interfaces "old".
	 */
	mgr->generation++;
	atomic_store(&mgr->shuttingdown, true);

	purge_old_interfaces(mgr);

	if (mgr->route != NULL) {
		isc_nm_cancelread(mgr->route);
	}

	for (size_t i = 0; i < (size_t)mgr->ncpus; i++) {
		ns_clientmgr_shutdown(mgr->clientmgrs[i]);
	}
}

static void
interface_create(ns_interfacemgr_t *mgr, isc_sockaddr_t *addr, const char *name,
		 ns_interface_t **ifpret) {
	ns_interface_t *ifp = NULL;

	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	ifp = isc_mem_get(mgr->mctx, sizeof(*ifp));
	*ifp = (ns_interface_t){ .generation = mgr->generation, .addr = *addr };

	strlcpy(ifp->name, name, sizeof(ifp->name));

	isc_mutex_init(&ifp->lock);

	isc_refcount_init(&ifp->ntcpaccepting, 0);
	isc_refcount_init(&ifp->ntcpactive, 0);

	ISC_LINK_INIT(ifp, link);

	ns_interfacemgr_attach(mgr, &ifp->mgr);
	ifp->magic = IFACE_MAGIC;

	LOCK(&mgr->lock);
	ISC_LIST_APPEND(mgr->interfaces, ifp, link);
	UNLOCK(&mgr->lock);

	*ifpret = ifp;
}

static isc_result_t
ns_interface_listenudp(ns_interface_t *ifp) {
	isc_result_t result;

	/* Reserve space for an ns_client_t with the netmgr handle */
	result = isc_nm_listenudp(ifp->mgr->nm, &ifp->addr, ns__client_request,
				  ifp, sizeof(ns_client_t),
				  &ifp->udplistensocket);
	return (result);
}

static isc_result_t
ns_interface_listentcp(ns_interface_t *ifp) {
	isc_result_t result;

	result = isc_nm_listentcpdns(
		ifp->mgr->nm, &ifp->addr, ns__client_request, ifp,
		ns__client_tcpconn, ifp, sizeof(ns_client_t), ifp->mgr->backlog,
		&ifp->mgr->sctx->tcpquota, &ifp->tcplistensocket);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "creating TCP socket: %s",
			      isc_result_totext(result));
	}

	/*
	 * We call this now to update the tcp-highwater statistic:
	 * this is necessary because we are adding to the TCP quota just
	 * by listening.
	 */
	result = ns__client_tcpconn(NULL, ISC_R_SUCCESS, ifp);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "connecting TCP socket: %s",
			      isc_result_totext(result));
	}

	return (result);
}

/*
 * XXXWPK we should probably pass a complete object with key, cert, and other
 * TLS related options.
 */
static isc_result_t
ns_interface_listentls(ns_interface_t *ifp, isc_tlsctx_t *sslctx) {
	isc_result_t result;

	result = isc_nm_listentlsdns(
		ifp->mgr->nm, &ifp->addr, ns__client_request, ifp,
		ns__client_tcpconn, ifp, sizeof(ns_client_t), ifp->mgr->backlog,
		&ifp->mgr->sctx->tcpquota, sslctx, &ifp->tcplistensocket);

	if (result != ISC_R_SUCCESS) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "creating TLS socket: %s",
			      isc_result_totext(result));
		return (result);
	}

	/*
	 * We call this now to update the tcp-highwater statistic:
	 * this is necessary because we are adding to the TCP quota just
	 * by listening.
	 */
	result = ns__client_tcpconn(NULL, ISC_R_SUCCESS, ifp);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "updating TCP stats: %s",
			      isc_result_totext(result));
	}

	return (result);
}

#ifdef HAVE_LIBNGHTTP2
static isc_result_t
load_http_endpoints(isc_nm_http_endpoints_t *epset, ns_interface_t *ifp,
		    char **eps, size_t neps) {
	isc_result_t result = ISC_R_FAILURE;

	for (size_t i = 0; i < neps; i++) {
		result = isc_nm_http_endpoints_add(epset, eps[i],
						   ns__client_request, ifp,
						   sizeof(ns_client_t));
		if (result != ISC_R_SUCCESS) {
			break;
		}
	}

	return (result);
}
#endif /* HAVE_LIBNGHTTP2 */

static isc_result_t
ns_interface_listenhttp(ns_interface_t *ifp, isc_tlsctx_t *sslctx, char **eps,
			size_t neps, uint32_t max_clients,
			uint32_t max_concurrent_streams) {
#if HAVE_LIBNGHTTP2
	isc_result_t result = ISC_R_FAILURE;
	isc_nmsocket_t *sock = NULL;
	isc_nm_http_endpoints_t *epset = NULL;
	isc_quota_t *quota = NULL;

	epset = isc_nm_http_endpoints_new(ifp->mgr->mctx);

	result = load_http_endpoints(epset, ifp, eps, neps);

	if (result == ISC_R_SUCCESS) {
		quota = isc_mem_get(ifp->mgr->mctx, sizeof(*quota));
		isc_quota_init(quota, max_clients);
		result = isc_nm_listenhttp(
			ifp->mgr->nm, &ifp->addr, ifp->mgr->backlog, quota,
			sslctx, epset, max_concurrent_streams, &sock);
	}

	isc_nm_http_endpoints_detach(&epset);

	if (quota != NULL) {
		if (result != ISC_R_SUCCESS) {
			isc_quota_destroy(quota);
			isc_mem_put(ifp->mgr->mctx, quota, sizeof(*quota));
		} else {
			ifp->http_quota = quota;
			ns_server_append_http_quota(ifp->mgr->sctx, quota);
		}
	}

	if (result != ISC_R_SUCCESS) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "creating %s socket: %s",
			      sslctx ? "HTTPS" : "HTTP",
			      isc_result_totext(result));
		return (result);
	}

	if (sslctx) {
		ifp->http_secure_listensocket = sock;
	} else {
		ifp->http_listensocket = sock;
	}

	/*
	 * We call this now to update the tcp-highwater statistic:
	 * this is necessary because we are adding to the TCP quota just
	 * by listening.
	 */
	result = ns__client_tcpconn(NULL, ISC_R_SUCCESS, ifp);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "updating TCP stats: %s",
			      isc_result_totext(result));
	}

	return (result);
#else
	UNUSED(ifp);
	UNUSED(sslctx);
	UNUSED(eps);
	UNUSED(neps);
	UNUSED(max_clients);
	UNUSED(max_concurrent_streams);
	return (ISC_R_NOTIMPLEMENTED);
#endif
}

static isc_result_t
interface_setup(ns_interfacemgr_t *mgr, isc_sockaddr_t *addr, const char *name,
		ns_interface_t **ifpret, ns_listenelt_t *elt,
		bool *addr_in_use) {
	isc_result_t result;
	ns_interface_t *ifp = NULL;

	REQUIRE(ifpret != NULL);
	REQUIRE(addr_in_use == NULL || !*addr_in_use);

	ifp = *ifpret;

	if (ifp == NULL) {
		interface_create(mgr, addr, name, &ifp);
	} else {
		REQUIRE(!LISTENING(ifp));
	}

	ifp->flags |= NS_INTERFACEFLAG_LISTENING;

	if (elt->is_http) {
		result = ns_interface_listenhttp(
			ifp, elt->sslctx, elt->http_endpoints,
			elt->http_endpoints_number, elt->http_max_clients,
			elt->max_concurrent_streams);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_interface;
		}
		*ifpret = ifp;
		return (result);
	}

	if (elt->sslctx != NULL) {
		result = ns_interface_listentls(ifp, elt->sslctx);
		if (result != ISC_R_SUCCESS) {
			goto cleanup_interface;
		}
		*ifpret = ifp;
		return (result);
	}

	result = ns_interface_listenudp(ifp);
	if (result != ISC_R_SUCCESS) {
		if ((result == ISC_R_ADDRINUSE) && (addr_in_use != NULL)) {
			*addr_in_use = true;
		}
		goto cleanup_interface;
	}

	if (((mgr->sctx->options & NS_SERVER_NOTCP) == 0)) {
		result = ns_interface_listentcp(ifp);
		if (result != ISC_R_SUCCESS) {
			if ((result == ISC_R_ADDRINUSE) &&
			    (addr_in_use != NULL))
			{
				*addr_in_use = true;
			}

			/*
			 * XXXRTH We don't currently have a way to easily stop
			 * dispatch service, so we currently return
			 * ISC_R_SUCCESS (the UDP stuff will work even if TCP
			 * creation failed).  This will be fixed later.
			 */
			result = ISC_R_SUCCESS;
		}
	}
	*ifpret = ifp;
	return (result);

cleanup_interface:
	ns_interface_shutdown(ifp);
	return (result);
}

void
ns_interface_shutdown(ns_interface_t *ifp) {
	ifp->flags &= ~NS_INTERFACEFLAG_LISTENING;

	if (ifp->udplistensocket != NULL) {
		isc_nm_stoplistening(ifp->udplistensocket);
		isc_nmsocket_close(&ifp->udplistensocket);
	}
	if (ifp->tcplistensocket != NULL) {
		isc_nm_stoplistening(ifp->tcplistensocket);
		isc_nmsocket_close(&ifp->tcplistensocket);
	}
	if (ifp->http_listensocket != NULL) {
		isc_nm_stoplistening(ifp->http_listensocket);
		isc_nmsocket_close(&ifp->http_listensocket);
	}
	if (ifp->http_secure_listensocket != NULL) {
		isc_nm_stoplistening(ifp->http_secure_listensocket);
		isc_nmsocket_close(&ifp->http_secure_listensocket);
	}
	ifp->http_quota = NULL;
}

static void
interface_destroy(ns_interface_t **interfacep) {
	ns_interface_t *ifp = NULL;
	ns_interfacemgr_t *mgr = NULL;

	REQUIRE(interfacep != NULL);

	ifp = *interfacep;
	*interfacep = NULL;

	REQUIRE(NS_INTERFACE_VALID(ifp));

	mgr = ifp->mgr;

	ns_interface_shutdown(ifp);

	ifp->magic = 0;
	isc_mutex_destroy(&ifp->lock);
	ns_interfacemgr_detach(&ifp->mgr);
	isc_refcount_destroy(&ifp->ntcpactive);
	isc_refcount_destroy(&ifp->ntcpaccepting);

	isc_mem_put(mgr->mctx, ifp, sizeof(*ifp));
}

/*%
 * Search the interface list for an interface whose address and port
 * both match those of 'addr'.  Return a pointer to it, or NULL if not found.
 */
static ns_interface_t *
find_matching_interface(ns_interfacemgr_t *mgr, isc_sockaddr_t *addr) {
	ns_interface_t *ifp;
	LOCK(&mgr->lock);
	for (ifp = ISC_LIST_HEAD(mgr->interfaces); ifp != NULL;
	     ifp = ISC_LIST_NEXT(ifp, link))
	{
		if (isc_sockaddr_equal(&ifp->addr, addr)) {
			break;
		}
	}
	UNLOCK(&mgr->lock);
	return (ifp);
}

/*%
 * Remove any interfaces whose generation number is not the current one.
 */
static void
purge_old_interfaces(ns_interfacemgr_t *mgr) {
	ns_interface_t *ifp = NULL, *next = NULL;
	ISC_LIST(ns_interface_t) interfaces;

	ISC_LIST_INIT(interfaces);

	LOCK(&mgr->lock);
	for (ifp = ISC_LIST_HEAD(mgr->interfaces); ifp != NULL; ifp = next) {
		INSIST(NS_INTERFACE_VALID(ifp));
		next = ISC_LIST_NEXT(ifp, link);
		if (ifp->generation != mgr->generation) {
			ISC_LIST_UNLINK(ifp->mgr->interfaces, ifp, link);
			ISC_LIST_APPEND(interfaces, ifp, link);
		}
	}
	UNLOCK(&mgr->lock);

	for (ifp = ISC_LIST_HEAD(interfaces); ifp != NULL; ifp = next) {
		next = ISC_LIST_NEXT(ifp, link);
		if (LISTENING(ifp)) {
			char sabuf[256];
			isc_sockaddr_format(&ifp->addr, sabuf, sizeof(sabuf));
			isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_INFO,
				      "no longer listening on %s", sabuf);
			ns_interface_shutdown(ifp);
		}
		ISC_LIST_UNLINK(interfaces, ifp, link);
		interface_destroy(&ifp);
	}
}

static bool
listenon_is_ip6_any(ns_listenelt_t *elt) {
	REQUIRE(elt && elt->acl);
	return (dns_acl_isany(elt->acl));
}

static isc_result_t
setup_locals(isc_interface_t *interface, dns_acl_t *localhost,
	     dns_acl_t *localnets) {
	isc_result_t result;
	unsigned int prefixlen;
	isc_netaddr_t *netaddr;

	netaddr = &interface->address;

	/* First add localhost address */
	prefixlen = (netaddr->family == AF_INET) ? 32 : 128;
	result = dns_iptable_addprefix(localhost->iptable, netaddr, prefixlen,
				       true);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/* Then add localnets prefix */
	result = isc_netaddr_masktoprefixlen(&interface->netmask, &prefixlen);

	/* Non contiguous netmasks not allowed by IPv6 arch. */
	if (result != ISC_R_SUCCESS && netaddr->family == AF_INET6) {
		return (result);
	}

	if (result != ISC_R_SUCCESS) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_WARNING,
			      "omitting IPv4 interface %s from "
			      "localnets ACL: %s",
			      interface->name, isc_result_totext(result));
		return (ISC_R_SUCCESS);
	}

	if (prefixlen == 0U) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_WARNING,
			      "omitting %s interface %s from localnets ACL: "
			      "zero prefix length detected",
			      (netaddr->family == AF_INET) ? "IPv4" : "IPv6",
			      interface->name);
		return (ISC_R_SUCCESS);
	}

	result = dns_iptable_addprefix(localnets->iptable, netaddr, prefixlen,
				       true);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	return (ISC_R_SUCCESS);
}

static void
setup_listenon(ns_interfacemgr_t *mgr, isc_interface_t *interface,
	       in_port_t port) {
	isc_sockaddr_t *addr;
	isc_sockaddr_t *old;

	addr = isc_mem_get(mgr->mctx, sizeof(*addr));

	isc_sockaddr_fromnetaddr(addr, &interface->address, port);

	LOCK(&mgr->lock);
	for (old = ISC_LIST_HEAD(mgr->listenon); old != NULL;
	     old = ISC_LIST_NEXT(old, link))
	{
		if (isc_sockaddr_equal(addr, old)) {
			/* We found an existing address */
			isc_mem_put(mgr->mctx, addr, sizeof(*addr));
			goto unlock;
		}
	}

	ISC_LIST_APPEND(mgr->listenon, addr, link);
unlock:
	UNLOCK(&mgr->lock);
}

static void
clearlistenon(ns_interfacemgr_t *mgr) {
	ISC_LIST(isc_sockaddr_t) listenon;
	isc_sockaddr_t *old;

	ISC_LIST_INIT(listenon);

	LOCK(&mgr->lock);
	ISC_LIST_MOVE(listenon, mgr->listenon);
	UNLOCK(&mgr->lock);

	old = ISC_LIST_HEAD(listenon);
	while (old != NULL) {
		ISC_LIST_UNLINK(listenon, old, link);
		isc_mem_put(mgr->mctx, old, sizeof(*old));
		old = ISC_LIST_HEAD(listenon);
	}
}

static void
replace_listener_tlsctx(ns_interface_t *ifp, isc_tlsctx_t *newctx) {
	char sabuf[ISC_SOCKADDR_FORMATSIZE];

	isc_sockaddr_format(&ifp->addr, sabuf, sizeof(sabuf));
	isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_INFO,
		      "updating TLS context on %s", sabuf);
	if (ifp->tcplistensocket != NULL) {
		/* 'tcplistensocket' is used for DoT */
		isc_nmsocket_set_tlsctx(ifp->tcplistensocket, newctx);
	} else if (ifp->http_secure_listensocket != NULL) {
		isc_nmsocket_set_tlsctx(ifp->http_secure_listensocket, newctx);
	}
}

#ifdef HAVE_LIBNGHTTP2
static void
update_http_settings(ns_interface_t *ifp, ns_listenelt_t *le) {
	isc_result_t result;
	isc_nmsocket_t *listener;
	isc_nm_http_endpoints_t *epset;

	REQUIRE(le->is_http);

	INSIST(ifp->http_quota != NULL);
	isc_quota_max(ifp->http_quota, le->http_max_clients);

	if (ifp->http_secure_listensocket != NULL) {
		listener = ifp->http_secure_listensocket;
	} else {
		INSIST(ifp->http_listensocket != NULL);
		listener = ifp->http_listensocket;
	}

	isc_nmsocket_set_max_streams(listener, le->max_concurrent_streams);

	epset = isc_nm_http_endpoints_new(ifp->mgr->mctx);

	result = load_http_endpoints(epset, ifp, le->http_endpoints,
				     le->http_endpoints_number);

	if (result == ISC_R_SUCCESS) {
		isc_nm_http_set_endpoints(listener, epset);
	}

	isc_nm_http_endpoints_detach(&epset);
}
#endif /* HAVE_LIBNGHTTP2 */

static void
update_listener_configuration(ns_interfacemgr_t *mgr, ns_interface_t *ifp,
			      ns_listenelt_t *le) {
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));
	REQUIRE(NS_INTERFACE_VALID(ifp));
	REQUIRE(le != NULL);

	LOCK(&mgr->lock);
	/*
	 * We need to update the TLS contexts
	 * inside the TLS/HTTPS listeners during
	 * a reconfiguration because the
	 * certificates could have been changed.
	 */
	if (le->sslctx != NULL) {
		replace_listener_tlsctx(ifp, le->sslctx);
	}

#ifdef HAVE_LIBNGHTTP2
	/*
	 * Let's update HTTP listener settings
	 * on reconfiguration.
	 */
	if (le->is_http) {
		update_http_settings(ifp, le);
	}
#endif /* HAVE_LIBNGHTTP2 */

	UNLOCK(&mgr->lock);
}

static isc_result_t
do_scan(ns_interfacemgr_t *mgr, bool verbose, bool config) {
	isc_interfaceiter_t *iter = NULL;
	bool scan_ipv4 = false;
	bool scan_ipv6 = false;
	bool ipv6only = true;
	bool ipv6pktinfo = true;
	isc_result_t result;
	isc_netaddr_t zero_address, zero_address6;
	ns_listenelt_t *le = NULL;
	isc_sockaddr_t listen_addr;
	ns_interface_t *ifp = NULL;
	bool log_explicit = false;
	bool dolistenon;
	char sabuf[ISC_SOCKADDR_FORMATSIZE];
	bool tried_listening;
	bool all_addresses_in_use;
	dns_acl_t *localhost = NULL;
	dns_acl_t *localnets = NULL;

	if (isc_net_probeipv6() == ISC_R_SUCCESS) {
		scan_ipv6 = true;
	} else if ((mgr->sctx->options & NS_SERVER_DISABLE6) == 0) {
		isc_log_write(IFMGR_COMMON_LOGARGS,
			      verbose ? ISC_LOG_INFO : ISC_LOG_DEBUG(1),
			      "no IPv6 interfaces found");
	}

	if (isc_net_probeipv4() == ISC_R_SUCCESS) {
		scan_ipv4 = true;
	} else if ((mgr->sctx->options & NS_SERVER_DISABLE4) == 0) {
		isc_log_write(IFMGR_COMMON_LOGARGS,
			      verbose ? ISC_LOG_INFO : ISC_LOG_DEBUG(1),
			      "no IPv4 interfaces found");
	}

	/*
	 * A special, but typical case; listen-on-v6 { any; }.
	 * When we can make the socket IPv6-only, open a single wildcard
	 * socket for IPv6 communication.  Otherwise, make separate
	 * socket for each IPv6 address in order to avoid accepting IPv4
	 * packets as the form of mapped addresses unintentionally
	 * unless explicitly allowed.
	 */
	if (scan_ipv6 && isc_net_probe_ipv6only() != ISC_R_SUCCESS) {
		ipv6only = false;
		log_explicit = true;
	}
	if (scan_ipv6 && isc_net_probe_ipv6pktinfo() != ISC_R_SUCCESS) {
		ipv6pktinfo = false;
		log_explicit = true;
	}
	if (scan_ipv6 && ipv6only && ipv6pktinfo) {
		for (le = ISC_LIST_HEAD(mgr->listenon6->elts); le != NULL;
		     le = ISC_LIST_NEXT(le, link))
		{
			struct in6_addr in6a;

			if (!listenon_is_ip6_any(le)) {
				continue;
			}

			in6a = in6addr_any;
			isc_sockaddr_fromin6(&listen_addr, &in6a, le->port);

			ifp = find_matching_interface(mgr, &listen_addr);
			if (ifp != NULL) {
				ifp->generation = mgr->generation;
				if (LISTENING(ifp)) {
					if (config) {
						update_listener_configuration(
							mgr, ifp, le);
					}
					continue;
				}
			}

			isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_INFO,
				      "listening on IPv6 "
				      "interfaces, port %u",
				      le->port);
			result = interface_setup(mgr, &listen_addr, "<any>",
						 &ifp, le, NULL);
			if (result == ISC_R_SUCCESS) {
				ifp->flags |= NS_INTERFACEFLAG_ANYADDR;
			} else {
				isc_log_write(IFMGR_COMMON_LOGARGS,
					      ISC_LOG_ERROR,
					      "listening on all IPv6 "
					      "interfaces failed");
			}
			/* Continue. */
		}
	}

	isc_netaddr_any(&zero_address);
	isc_netaddr_any6(&zero_address6);

	result = isc_interfaceiter_create(mgr->mctx, &iter);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_acl_create(mgr->mctx, 0, &localhost);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_iter;
	}
	result = dns_acl_create(mgr->mctx, 0, &localnets);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_localhost;
	}

	clearlistenon(mgr);

	tried_listening = false;
	all_addresses_in_use = true;
	for (result = isc_interfaceiter_first(iter); result == ISC_R_SUCCESS;
	     result = isc_interfaceiter_next(iter))
	{
		isc_interface_t interface;
		ns_listenlist_t *ll = NULL;
		unsigned int family;

		result = isc_interfaceiter_current(iter, &interface);
		if (result != ISC_R_SUCCESS) {
			break;
		}

		family = interface.address.family;
		if (family != AF_INET && family != AF_INET6) {
			continue;
		}
		if (!scan_ipv4 && family == AF_INET) {
			continue;
		}
		if (!scan_ipv6 && family == AF_INET6) {
			continue;
		}

		/*
		 * Test for the address being nonzero rather than testing
		 * INTERFACE_F_UP, because on some systems the latter
		 * follows the media state and we could end up ignoring
		 * the interface for an entire rescan interval due to
		 * a temporary media glitch at rescan time.
		 */
		if (family == AF_INET &&
		    isc_netaddr_equal(&interface.address, &zero_address))
		{
			continue;
		}
		if (family == AF_INET6 &&
		    isc_netaddr_equal(&interface.address, &zero_address6))
		{
			continue;
		}

		/*
		 * If running with -T fixedlocal, then we only
		 * want 127.0.0.1 and ::1 in the localhost ACL.
		 */
		if (((mgr->sctx->options & NS_SERVER_FIXEDLOCAL) != 0) &&
		    !isc_netaddr_isloopback(&interface.address))
		{
			goto listenon;
		}

		result = setup_locals(&interface, localhost, localnets);
		if (result != ISC_R_SUCCESS) {
			goto ignore_interface;
		}

	listenon:
		ll = (family == AF_INET) ? mgr->listenon4 : mgr->listenon6;
		dolistenon = true;
		for (le = ISC_LIST_HEAD(ll->elts); le != NULL;
		     le = ISC_LIST_NEXT(le, link))
		{
			int match;
			bool addr_in_use = false;
			bool ipv6_wildcard = false;
			isc_sockaddr_t listen_sockaddr;

			isc_sockaddr_fromnetaddr(&listen_sockaddr,
						 &interface.address, le->port);

			/*
			 * See if the address matches the listen-on statement;
			 * if not, ignore the interface, but store it in
			 * the interface table so we know we've seen it
			 * before.
			 */
			(void)dns_acl_match(&interface.address, NULL, le->acl,
					    mgr->aclenv, &match, NULL);
			if (match <= 0) {
				ns_interface_t *new = NULL;
				interface_create(mgr, &listen_sockaddr,
						 interface.name, &new);
				continue;
			}

			if (dolistenon) {
				setup_listenon(mgr, &interface, le->port);
				dolistenon = false;
			}

			/*
			 * The case of "any" IPv6 address will require
			 * special considerations later, so remember it.
			 */
			if (family == AF_INET6 && ipv6only && ipv6pktinfo &&
			    listenon_is_ip6_any(le))
			{
				ipv6_wildcard = true;
			}

			ifp = find_matching_interface(mgr, &listen_sockaddr);
			if (ifp != NULL) {
				ifp->generation = mgr->generation;
				if (LISTENING(ifp)) {
					if (config) {
						update_listener_configuration(
							mgr, ifp, le);
					}
					continue;
				}
			}

			if (ipv6_wildcard) {
				continue;
			}

			if (log_explicit && family == AF_INET6 &&
			    listenon_is_ip6_any(le))
			{
				isc_log_write(IFMGR_COMMON_LOGARGS,
					      verbose ? ISC_LOG_INFO
						      : ISC_LOG_DEBUG(1),
					      "IPv6 socket API is "
					      "incomplete; explicitly "
					      "binding to each IPv6 "
					      "address separately");
				log_explicit = false;
			}
			isc_sockaddr_format(&listen_sockaddr, sabuf,
					    sizeof(sabuf));
			isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_INFO,
				      "listening on %s interface "
				      "%s, %s",
				      (family == AF_INET) ? "IPv4" : "IPv6",
				      interface.name, sabuf);

			result = interface_setup(mgr, &listen_sockaddr,
						 interface.name, &ifp, le,
						 &addr_in_use);

			tried_listening = true;
			if (!addr_in_use) {
				all_addresses_in_use = false;
			}

			if (result != ISC_R_SUCCESS) {
				isc_log_write(
					IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
					"creating %s interface "
					"%s failed; interface ignored",
					(family == AF_INET) ? "IPv4" : "IPv6",
					interface.name);
			}
			/* Continue. */
		}
		continue;

	ignore_interface:
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "ignoring %s interface %s: %s",
			      (family == AF_INET) ? "IPv4" : "IPv6",
			      interface.name, isc_result_totext(result));
		continue;
	}
	if (result != ISC_R_NOMORE) {
		UNEXPECTED_ERROR("interface iteration failed: %s",
				 isc_result_totext(result));
	} else {
		result = ((tried_listening && all_addresses_in_use)
				  ? ISC_R_ADDRINUSE
				  : ISC_R_SUCCESS);
	}

	dns_aclenv_set(mgr->aclenv, localhost, localnets);

	/* cleanup_localnets: */
	dns_acl_detach(&localnets);

cleanup_localhost:
	dns_acl_detach(&localhost);

cleanup_iter:
	isc_interfaceiter_destroy(&iter);
	return (result);
}

isc_result_t
ns_interfacemgr_scan(ns_interfacemgr_t *mgr, bool verbose, bool config) {
	isc_result_t result;
	bool purge = true;

	REQUIRE(NS_INTERFACEMGR_VALID(mgr));
	REQUIRE(isc_nm_tid() == 0);

	mgr->generation++; /* Increment the generation count. */

	result = do_scan(mgr, verbose, config);
	if ((result != ISC_R_SUCCESS) && (result != ISC_R_ADDRINUSE)) {
		purge = false;
	}

	/*
	 * Now go through the interface list and delete anything that
	 * does not have the current generation number.  This is
	 * how we catch interfaces that go away or change their
	 * addresses.
	 */
	if (purge) {
		purge_old_interfaces(mgr);
	}

	/*
	 * Warn if we are not listening on any interface.
	 */
	if (ISC_LIST_EMPTY(mgr->interfaces)) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_WARNING,
			      "not listening on any interfaces");
	}

	return (result);
}

bool
ns_interfacemgr_islistening(ns_interfacemgr_t *mgr) {
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	return (ISC_LIST_EMPTY(mgr->interfaces) ? false : true);
}

void
ns_interfacemgr_setlistenon4(ns_interfacemgr_t *mgr, ns_listenlist_t *value) {
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	LOCK(&mgr->lock);
	ns_listenlist_detach(&mgr->listenon4);
	ns_listenlist_attach(value, &mgr->listenon4);
	UNLOCK(&mgr->lock);
}

void
ns_interfacemgr_setlistenon6(ns_interfacemgr_t *mgr, ns_listenlist_t *value) {
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	LOCK(&mgr->lock);
	ns_listenlist_detach(&mgr->listenon6);
	ns_listenlist_attach(value, &mgr->listenon6);
	UNLOCK(&mgr->lock);
}

void
ns_interfacemgr_dumprecursing(FILE *f, ns_interfacemgr_t *mgr) {
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	LOCK(&mgr->lock);
	for (size_t i = 0; i < (size_t)mgr->ncpus; i++) {
		ns_client_dumprecursing(f, mgr->clientmgrs[i]);
	}
	UNLOCK(&mgr->lock);
}

bool
ns_interfacemgr_listeningon(ns_interfacemgr_t *mgr,
			    const isc_sockaddr_t *addr) {
	isc_sockaddr_t *old;
	bool result = false;

	REQUIRE(NS_INTERFACEMGR_VALID(mgr));
	/*
	 * If the manager is shutting down it's safer to
	 * return true.
	 */
	if (atomic_load(&mgr->shuttingdown)) {
		return (true);
	}
	LOCK(&mgr->lock);
	for (old = ISC_LIST_HEAD(mgr->listenon); old != NULL;
	     old = ISC_LIST_NEXT(old, link))
	{
		if (isc_sockaddr_equal(old, addr)) {
			result = true;
			break;
		}
	}
	UNLOCK(&mgr->lock);

	return (result);
}

ns_server_t *
ns_interfacemgr_getserver(ns_interfacemgr_t *mgr) {
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	return (mgr->sctx);
}

ns_clientmgr_t *
ns_interfacemgr_getclientmgr(ns_interfacemgr_t *mgr) {
	int tid = isc_nm_tid();

	REQUIRE(NS_INTERFACEMGR_VALID(mgr));
	REQUIRE(tid >= 0);
	REQUIRE(tid < mgr->ncpus);

	return (mgr->clientmgrs[tid]);
}
