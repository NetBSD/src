/*	$NetBSD: interfacemgr.c,v 1.15 2023/01/25 21:43:32 christos Exp $	*/

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
#define USE_ROUTE_SOCKET      1
#define ROUTE_SOCKET_PROTOCOL PF_ROUTE
#define MSGHDR		      rt_msghdr
#define MSGTYPE		      rtm_type
#endif /* if defined(RTM_VERSION) && defined(RTM_NEWADDR) && \
	* defined(RTM_DELADDR) */
#endif /* ifdef HAVE_NET_ROUTE_H */

#if defined(HAVE_LINUX_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H)
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#if defined(RTM_NEWADDR) && defined(RTM_DELADDR)
#define USE_ROUTE_SOCKET      1
#define ROUTE_SOCKET_PROTOCOL PF_NETLINK
#define MSGHDR		      nlmsghdr
#define MSGTYPE		      nlmsg_type
#endif /* if defined(RTM_NEWADDR) && defined(RTM_DELADDR) */
#endif /* if defined(HAVE_LINUX_NETLINK_H) && defined(HAVE_LINUX_RTNETLINK_H) \
	*/

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
	unsigned int magic; /*%< Magic number. */
	isc_refcount_t references;
	isc_mutex_t lock;
	isc_mem_t *mctx;	    /*%< Memory context. */
	ns_server_t *sctx;	    /*%< Server context. */
	isc_taskmgr_t *taskmgr;	    /*%< Task manager. */
	isc_task_t *excl;	    /*%< Exclusive task. */
	isc_timermgr_t *timermgr;   /*%< Timer manager. */
	isc_socketmgr_t *socketmgr; /*%< Socket manager. */
	isc_nm_t *nm;		    /*%< Net manager. */
	int ncpus;		    /*%< Number of workers . */
	dns_dispatchmgr_t *dispatchmgr;
	unsigned int generation; /*%< Current generation no. */
	ns_listenlist_t *listenon4;
	ns_listenlist_t *listenon6;
	dns_aclenv_t aclenv;		     /*%< Localhost/localnets ACLs */
	ISC_LIST(ns_interface_t) interfaces; /*%< List of interfaces. */
	ISC_LIST(isc_sockaddr_t) listenon;
	int backlog;		  /*%< Listen queue size */
	unsigned int udpdisp;	  /*%< UDP dispatch count */
	atomic_bool shuttingdown; /*%< Interfacemgr is shutting
				   * down */
#ifdef USE_ROUTE_SOCKET
	isc_task_t *task;
	isc_socket_t *route;
	unsigned char buf[2048];
#endif /* ifdef USE_ROUTE_SOCKET */
};

static void
purge_old_interfaces(ns_interfacemgr_t *mgr);

static void
clearlistenon(ns_interfacemgr_t *mgr);

#ifdef USE_ROUTE_SOCKET
static void
route_event(isc_task_t *task, isc_event_t *event) {
	isc_socketevent_t *sevent = NULL;
	ns_interfacemgr_t *mgr = NULL;
	isc_region_t r;
	isc_result_t result;
	struct MSGHDR *rtm;
	bool done = true;

	UNUSED(task);

	REQUIRE(event->ev_type == ISC_SOCKEVENT_RECVDONE);
	mgr = event->ev_arg;
	sevent = (isc_socketevent_t *)event;

	if (sevent->result != ISC_R_SUCCESS) {
		if (sevent->result != ISC_R_CANCELED) {
			isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
				      "automatic interface scanning "
				      "terminated: %s",
				      isc_result_totext(sevent->result));
		}
		ns_interfacemgr_detach(&mgr);
		isc_event_free(&event);
		return;
	}

	rtm = (struct MSGHDR *)mgr->buf;
#ifdef RTM_VERSION
	if (rtm->rtm_version != RTM_VERSION) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "automatic interface rescanning disabled: "
			      "rtm->rtm_version mismatch (%u != %u) "
			      "recompile required",
			      rtm->rtm_version, RTM_VERSION);
		ns_interfacemgr_detach(&mgr);
		isc_event_free(&event);
		return;
	}
#endif /* ifdef RTM_VERSION */

	switch (rtm->MSGTYPE) {
	case RTM_NEWADDR:
	case RTM_DELADDR:
		if (mgr->route != NULL && mgr->sctx->interface_auto) {
			ns_interfacemgr_scan(mgr, false);
		}
		break;
	default:
		break;
	}

	LOCK(&mgr->lock);
	if (mgr->route != NULL) {
		/*
		 * Look for next route event.
		 */
		r.base = mgr->buf;
		r.length = sizeof(mgr->buf);
		result = isc_socket_recv(mgr->route, &r, 1, mgr->task,
					 route_event, mgr);
		if (result == ISC_R_SUCCESS) {
			done = false;
		}
	}
	UNLOCK(&mgr->lock);

	if (done) {
		ns_interfacemgr_detach(&mgr);
	}
	isc_event_free(&event);
	return;
}
#endif /* ifdef USE_ROUTE_SOCKET */

isc_result_t
ns_interfacemgr_create(isc_mem_t *mctx, ns_server_t *sctx,
		       isc_taskmgr_t *taskmgr, isc_timermgr_t *timermgr,
		       isc_socketmgr_t *socketmgr, isc_nm_t *nm,
		       dns_dispatchmgr_t *dispatchmgr, isc_task_t *task,
		       unsigned int udpdisp, dns_geoip_databases_t *geoip,
		       int ncpus, ns_interfacemgr_t **mgrp) {
	isc_result_t result;
	ns_interfacemgr_t *mgr;

#ifndef USE_ROUTE_SOCKET
	UNUSED(task);
#endif /* ifndef USE_ROUTE_SOCKET */

	REQUIRE(mctx != NULL);
	REQUIRE(mgrp != NULL);
	REQUIRE(*mgrp == NULL);

	mgr = isc_mem_get(mctx, sizeof(*mgr));

	mgr->mctx = NULL;
	isc_mem_attach(mctx, &mgr->mctx);

	mgr->sctx = NULL;
	ns_server_attach(sctx, &mgr->sctx);

	isc_mutex_init(&mgr->lock);

	mgr->excl = NULL;
	result = isc_taskmgr_excltask(taskmgr, &mgr->excl);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_lock;
	}

	mgr->taskmgr = taskmgr;
	mgr->timermgr = timermgr;
	mgr->socketmgr = socketmgr;
	mgr->nm = nm;
	mgr->dispatchmgr = dispatchmgr;
	mgr->generation = 1;
	mgr->listenon4 = NULL;
	mgr->listenon6 = NULL;
	mgr->udpdisp = udpdisp;
	mgr->ncpus = ncpus;
	atomic_init(&mgr->shuttingdown, false);

	ISC_LIST_INIT(mgr->interfaces);
	ISC_LIST_INIT(mgr->listenon);

	/*
	 * The listen-on lists are initially empty.
	 */
	result = ns_listenlist_create(mctx, &mgr->listenon4);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_ctx;
	}
	ns_listenlist_attach(mgr->listenon4, &mgr->listenon6);

	result = dns_aclenv_init(mctx, &mgr->aclenv);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_listenon;
	}
#if defined(HAVE_GEOIP2)
	mgr->aclenv.geoip = geoip;
#else  /* if defined(HAVE_GEOIP2) */
	UNUSED(geoip);
#endif /* if defined(HAVE_GEOIP2) */

#ifdef USE_ROUTE_SOCKET
	mgr->route = NULL;
	result = isc_socket_create(mgr->socketmgr, ROUTE_SOCKET_PROTOCOL,
				   isc_sockettype_raw, &mgr->route);
	switch (result) {
	case ISC_R_NOPERM:
	case ISC_R_SUCCESS:
	case ISC_R_NOTIMPLEMENTED:
	case ISC_R_FAMILYNOSUPPORT:
		break;
	default:
		goto cleanup_aclenv;
	}

	mgr->task = NULL;
	if (mgr->route != NULL) {
		isc_task_attach(task, &mgr->task);
	}
	isc_refcount_init(&mgr->references, (mgr->route != NULL) ? 2 : 1);
#else  /* ifdef USE_ROUTE_SOCKET */
	isc_refcount_init(&mgr->references, 1);
#endif /* ifdef USE_ROUTE_SOCKET */
	mgr->magic = IFMGR_MAGIC;
	*mgrp = mgr;

#ifdef USE_ROUTE_SOCKET
	if (mgr->route != NULL) {
		isc_region_t r = { mgr->buf, sizeof(mgr->buf) };

		result = isc_socket_recv(mgr->route, &r, 1, mgr->task,
					 route_event, mgr);
		if (result != ISC_R_SUCCESS) {
			isc_task_detach(&mgr->task);
			isc_socket_detach(&mgr->route);
			ns_interfacemgr_detach(&mgr);
		}
	}
#endif /* ifdef USE_ROUTE_SOCKET */
	return (ISC_R_SUCCESS);

#ifdef USE_ROUTE_SOCKET
cleanup_aclenv:
	dns_aclenv_destroy(&mgr->aclenv);
#endif /* ifdef USE_ROUTE_SOCKET */
cleanup_listenon:
	ns_listenlist_detach(&mgr->listenon4);
	ns_listenlist_detach(&mgr->listenon6);
cleanup_lock:
	isc_mutex_destroy(&mgr->lock);
cleanup_ctx:
	ns_server_detach(&mgr->sctx);
	isc_mem_putanddetach(&mgr->mctx, mgr, sizeof(*mgr));
	return (result);
}

static void
ns_interfacemgr_destroy(ns_interfacemgr_t *mgr) {
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	isc_refcount_destroy(&mgr->references);

#ifdef USE_ROUTE_SOCKET
	if (mgr->route != NULL) {
		isc_socket_detach(&mgr->route);
	}
	if (mgr->task != NULL) {
		isc_task_detach(&mgr->task);
	}
#endif /* ifdef USE_ROUTE_SOCKET */
	dns_aclenv_destroy(&mgr->aclenv);
	ns_listenlist_detach(&mgr->listenon4);
	ns_listenlist_detach(&mgr->listenon6);
	clearlistenon(mgr);
	isc_mutex_destroy(&mgr->lock);
	if (mgr->sctx != NULL) {
		ns_server_detach(&mgr->sctx);
	}
	if (mgr->excl != NULL) {
		isc_task_detach(&mgr->excl);
	}
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
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	return (&mgr->aclenv);
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
	 * By incrementing the generation count, we make purge_old_interfaces()
	 * consider all interfaces "old".
	 */
	mgr->generation++;
	atomic_store(&mgr->shuttingdown, true);
#ifdef USE_ROUTE_SOCKET
	LOCK(&mgr->lock);
	if (mgr->route != NULL) {
		isc_socket_cancel(mgr->route, mgr->task, ISC_SOCKCANCEL_RECV);
		isc_socket_detach(&mgr->route);
		isc_task_detach(&mgr->task);
	}
	UNLOCK(&mgr->lock);
#endif /* ifdef USE_ROUTE_SOCKET */
	purge_old_interfaces(mgr);
}

static isc_result_t
ns_interface_create(ns_interfacemgr_t *mgr, isc_sockaddr_t *addr,
		    const char *name, ns_interface_t **ifpret) {
	ns_interface_t *ifp = NULL;
	isc_result_t result;
	int disp;

	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	ifp = isc_mem_get(mgr->mctx, sizeof(*ifp));
	*ifp = (ns_interface_t){ .generation = mgr->generation,
				 .addr = *addr,
				 .dscp = -1 };

	strlcpy(ifp->name, name, sizeof(ifp->name));

	isc_mutex_init(&ifp->lock);

	for (disp = 0; disp < MAX_UDP_DISPATCH; disp++) {
		ifp->udpdispatch[disp] = NULL;
	}

	/*
	 * Create a single TCP client object.  It will replace itself
	 * with a new one as soon as it gets a connection, so the actual
	 * connections will be handled in parallel even though there is
	 * only one client initially.
	 */
	isc_refcount_init(&ifp->ntcpaccepting, 0);
	isc_refcount_init(&ifp->ntcpactive, 0);

	ISC_LINK_INIT(ifp, link);

	ns_interfacemgr_attach(mgr, &ifp->mgr);
	isc_refcount_init(&ifp->references, 1);
	ifp->magic = IFACE_MAGIC;

	LOCK(&mgr->lock);
	ISC_LIST_APPEND(mgr->interfaces, ifp, link);
	UNLOCK(&mgr->lock);

	result = ns_clientmgr_create(mgr->mctx, mgr->sctx, mgr->taskmgr,
				     mgr->timermgr, ifp, mgr->ncpus,
				     &ifp->clientmgr);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_ERROR,
			      "ns_clientmgr_create() failed: %s",
			      isc_result_totext(result));
		goto failure;
	}

	*ifpret = ifp;

	return (ISC_R_SUCCESS);

failure:
	LOCK(&ifp->mgr->lock);
	ISC_LIST_UNLINK(ifp->mgr->interfaces, ifp, link);
	UNLOCK(&ifp->mgr->lock);

	ifp->magic = 0;
	ns_interfacemgr_detach(&ifp->mgr);
	isc_refcount_decrement(&ifp->references);
	isc_refcount_destroy(&ifp->references);
	isc_mutex_destroy(&ifp->lock);

	isc_mem_put(mgr->mctx, ifp, sizeof(*ifp));
	return (ISC_R_UNEXPECTED);
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

#if 0
#ifndef ISC_ALLOW_MAPPED
	isc_socket_ipv6only(ifp->tcpsocket,true);
#endif /* ifndef ISC_ALLOW_MAPPED */

	if (ifp->dscp != -1) {
		isc_socket_dscp(ifp->tcpsocket,ifp->dscp);
	}

	(void)isc_socket_filter(ifp->tcpsocket,"dataready");
#endif /* if 0 */
	return (result);
}

static isc_result_t
ns_interface_setup(ns_interfacemgr_t *mgr, isc_sockaddr_t *addr,
		   const char *name, ns_interface_t **ifpret, isc_dscp_t dscp,
		   bool *addr_in_use) {
	isc_result_t result;
	ns_interface_t *ifp = NULL;
	REQUIRE(ifpret != NULL && *ifpret == NULL);
	REQUIRE(addr_in_use == NULL || !*addr_in_use);

	result = ns_interface_create(mgr, addr, name, &ifp);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	ifp->dscp = dscp;

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
	LOCK(&ifp->mgr->lock);
	ISC_LIST_UNLINK(ifp->mgr->interfaces, ifp, link);
	UNLOCK(&ifp->mgr->lock);
	ns_interface_shutdown(ifp);
	ns_interface_detach(&ifp);
	return (result);
}

void
ns_interface_shutdown(ns_interface_t *ifp) {
	if (ifp->udplistensocket != NULL) {
		isc_nm_stoplistening(ifp->udplistensocket);
		isc_nmsocket_close(&ifp->udplistensocket);
	}
	if (ifp->tcplistensocket != NULL) {
		isc_nm_stoplistening(ifp->tcplistensocket);
		isc_nmsocket_close(&ifp->tcplistensocket);
	}
	if (ifp->clientmgr != NULL) {
		ns_clientmgr_shutdown(ifp->clientmgr);
		ns_clientmgr_destroy(&ifp->clientmgr);
	}
}

static void
ns_interface_destroy(ns_interface_t *ifp) {
	REQUIRE(NS_INTERFACE_VALID(ifp));

	isc_mem_t *mctx = ifp->mgr->mctx;

	ns_interface_shutdown(ifp);

	for (int disp = 0; disp < ifp->nudpdispatch; disp++) {
		if (ifp->udpdispatch[disp] != NULL) {
			dns_dispatch_changeattributes(
				ifp->udpdispatch[disp], 0,
				DNS_DISPATCHATTR_NOLISTEN);
			dns_dispatch_detach(&(ifp->udpdispatch[disp]));
		}
	}

	if (ifp->tcpsocket != NULL) {
		isc_socket_detach(&ifp->tcpsocket);
	}

	isc_mutex_destroy(&ifp->lock);

	ns_interfacemgr_detach(&ifp->mgr);

	isc_refcount_destroy(&ifp->ntcpactive);
	isc_refcount_destroy(&ifp->ntcpaccepting);

	ifp->magic = 0;

	isc_mem_put(mctx, ifp, sizeof(*ifp));
}

void
ns_interface_attach(ns_interface_t *source, ns_interface_t **target) {
	REQUIRE(NS_INTERFACE_VALID(source));
	isc_refcount_increment(&source->references);
	*target = source;
}

void
ns_interface_detach(ns_interface_t **targetp) {
	ns_interface_t *target = *targetp;
	*targetp = NULL;
	REQUIRE(target != NULL);
	REQUIRE(NS_INTERFACE_VALID(target));
	if (isc_refcount_decrement(&target->references) == 1) {
		ns_interface_destroy(target);
	}
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
	ns_interface_t *ifp, *next;
	LOCK(&mgr->lock);
	for (ifp = ISC_LIST_HEAD(mgr->interfaces); ifp != NULL; ifp = next) {
		INSIST(NS_INTERFACE_VALID(ifp));
		next = ISC_LIST_NEXT(ifp, link);
		if (ifp->generation != mgr->generation) {
			char sabuf[256];
			ISC_LIST_UNLINK(ifp->mgr->interfaces, ifp, link);
			isc_sockaddr_format(&ifp->addr, sabuf, sizeof(sabuf));
			isc_log_write(IFMGR_COMMON_LOGARGS, ISC_LOG_INFO,
				      "no longer listening on %s", sabuf);
			ns_interface_shutdown(ifp);
			ns_interface_detach(&ifp);
		}
	}
	UNLOCK(&mgr->lock);
}

static isc_result_t
clearacl(isc_mem_t *mctx, dns_acl_t **aclp) {
	dns_acl_t *newacl = NULL;
	isc_result_t result;
	result = dns_acl_create(mctx, 0, &newacl);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	dns_acl_detach(aclp);
	dns_acl_attach(newacl, aclp);
	dns_acl_detach(&newacl);
	return (ISC_R_SUCCESS);
}

static bool
listenon_is_ip6_any(ns_listenelt_t *elt) {
	REQUIRE(elt && elt->acl);
	return (dns_acl_isany(elt->acl));
}

static isc_result_t
setup_locals(ns_interfacemgr_t *mgr, isc_interface_t *interface) {
	isc_result_t result;
	unsigned int prefixlen;
	isc_netaddr_t *netaddr;

	netaddr = &interface->address;

	/* First add localhost address */
	prefixlen = (netaddr->family == AF_INET) ? 32 : 128;
	result = dns_iptable_addprefix(mgr->aclenv.localhost->iptable, netaddr,
				       prefixlen, true);
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

	result = dns_iptable_addprefix(mgr->aclenv.localnets->iptable, netaddr,
				       prefixlen, true);
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
			break;
		}
	}

	if (old != NULL) {
		isc_mem_put(mgr->mctx, addr, sizeof(*addr));
	} else {
		ISC_LIST_APPEND(mgr->listenon, addr, link);
	}
	UNLOCK(&mgr->lock);
}

static void
clearlistenon(ns_interfacemgr_t *mgr) {
	isc_sockaddr_t *old;

	LOCK(&mgr->lock);
	old = ISC_LIST_HEAD(mgr->listenon);
	while (old != NULL) {
		ISC_LIST_UNLINK(mgr->listenon, old, link);
		isc_mem_put(mgr->mctx, old, sizeof(*old));
		old = ISC_LIST_HEAD(mgr->listenon);
	}
	UNLOCK(&mgr->lock);
}

static isc_result_t
do_scan(ns_interfacemgr_t *mgr, bool verbose) {
	isc_interfaceiter_t *iter = NULL;
	bool scan_ipv4 = false;
	bool scan_ipv6 = false;
	bool ipv6only = true;
	bool ipv6pktinfo = true;
	isc_result_t result;
	isc_netaddr_t zero_address, zero_address6;
	ns_listenelt_t *le;
	isc_sockaddr_t listen_addr;
	ns_interface_t *ifp;
	bool log_explicit = false;
	bool dolistenon;
	char sabuf[ISC_SOCKADDR_FORMATSIZE];
	bool tried_listening;
	bool all_addresses_in_use;

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
#ifndef ISC_ALLOW_MAPPED
	if (scan_ipv6 && isc_net_probe_ipv6only() != ISC_R_SUCCESS) {
		ipv6only = false;
		log_explicit = true;
	}
#endif /* ifndef ISC_ALLOW_MAPPED */
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
				if (le->dscp != -1 && ifp->dscp == -1) {
					ifp->dscp = le->dscp;
				} else if (le->dscp != ifp->dscp) {
					isc_sockaddr_format(&listen_addr, sabuf,
							    sizeof(sabuf));
					isc_log_write(IFMGR_COMMON_LOGARGS,
						      ISC_LOG_WARNING,
						      "%s: conflicting DSCP "
						      "values, using %d",
						      sabuf, ifp->dscp);
				}
			} else {
				isc_log_write(IFMGR_COMMON_LOGARGS,
					      ISC_LOG_INFO,
					      "listening on IPv6 "
					      "interfaces, port %u",
					      le->port);
				result = ns_interface_setup(mgr, &listen_addr,
							    "<any>", &ifp,
							    le->dscp, NULL);
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
	}

	isc_netaddr_any(&zero_address);
	isc_netaddr_any6(&zero_address6);

	result = isc_interfaceiter_create(mgr->mctx, &iter);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = clearacl(mgr->mctx, &mgr->aclenv.localhost);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_iter;
	}
	result = clearacl(mgr->mctx, &mgr->aclenv.localnets);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_iter;
	}
	clearlistenon(mgr);

	tried_listening = false;
	all_addresses_in_use = true;
	for (result = isc_interfaceiter_first(iter); result == ISC_R_SUCCESS;
	     result = isc_interfaceiter_next(iter))
	{
		isc_interface_t interface;
		ns_listenlist_t *ll;
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

		result = setup_locals(mgr, &interface);
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
			bool ipv6_wildcard = false;
			isc_netaddr_t listen_netaddr;
			isc_sockaddr_t listen_sockaddr;

			/*
			 * Construct a socket address for this IP/port
			 * combination.
			 */
			if (family == AF_INET) {
				isc_netaddr_fromin(&listen_netaddr,
						   &interface.address.type.in);
			} else {
				isc_netaddr_fromin6(
					&listen_netaddr,
					&interface.address.type.in6);
				isc_netaddr_setzone(&listen_netaddr,
						    interface.address.zone);
			}
			isc_sockaddr_fromnetaddr(&listen_sockaddr,
						 &listen_netaddr, le->port);

			/*
			 * See if the address matches the listen-on statement;
			 * if not, ignore the interface.
			 */
			(void)dns_acl_match(&listen_netaddr, NULL, le->acl,
					    &mgr->aclenv, &match, NULL);
			if (match <= 0) {
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
				if (le->dscp != -1 && ifp->dscp == -1) {
					ifp->dscp = le->dscp;
				} else if (le->dscp != ifp->dscp) {
					isc_sockaddr_format(&listen_sockaddr,
							    sabuf,
							    sizeof(sabuf));
					isc_log_write(IFMGR_COMMON_LOGARGS,
						      ISC_LOG_WARNING,
						      "%s: conflicting DSCP "
						      "values, using %d",
						      sabuf, ifp->dscp);
				}
			} else {
				bool addr_in_use = false;

				if (ipv6_wildcard) {
					continue;
				}

				if (log_explicit && family == AF_INET6 &&
				    listenon_is_ip6_any(le))
				{
					isc_log_write(
						IFMGR_COMMON_LOGARGS,
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
				isc_log_write(
					IFMGR_COMMON_LOGARGS, ISC_LOG_INFO,
					"listening on %s interface "
					"%s, %s",
					(family == AF_INET) ? "IPv4" : "IPv6",
					interface.name, sabuf);

				result = ns_interface_setup(
					mgr, &listen_sockaddr, interface.name,
					&ifp, le->dscp, &addr_in_use);

				tried_listening = true;
				if (!addr_in_use) {
					all_addresses_in_use = false;
				}

				if (result != ISC_R_SUCCESS) {
					isc_log_write(IFMGR_COMMON_LOGARGS,
						      ISC_LOG_ERROR,
						      "creating %s interface "
						      "%s failed; interface "
						      "ignored",
						      (family == AF_INET) ? "IP"
									    "v4"
									  : "IP"
									    "v"
									    "6",
						      interface.name);
				}
				/* Continue. */
			}
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
		UNEXPECTED_ERROR(__FILE__, __LINE__,
				 "interface iteration failed: %s",
				 isc_result_totext(result));
	} else {
		result = ((tried_listening && all_addresses_in_use)
				  ? ISC_R_ADDRINUSE
				  : ISC_R_SUCCESS);
	}
cleanup_iter:
	isc_interfaceiter_destroy(&iter);
	return (result);
}

static isc_result_t
ns_interfacemgr_scan0(ns_interfacemgr_t *mgr, bool verbose) {
	isc_result_t result;
	bool purge = true;

	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	mgr->generation++; /* Increment the generation count. */

	result = do_scan(mgr, verbose);
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

isc_result_t
ns_interfacemgr_scan(ns_interfacemgr_t *mgr, bool verbose) {
	isc_result_t result;
	bool unlock = false;

	/*
	 * Check for success because we may already be task-exclusive
	 * at this point.  Only if we succeed at obtaining an exclusive
	 * lock now will we need to relinquish it later.
	 */
	result = isc_task_beginexclusive(mgr->excl);
	if (result == ISC_R_SUCCESS) {
		unlock = true;
	}

	result = ns_interfacemgr_scan0(mgr, verbose);

	if (unlock) {
		isc_task_endexclusive(mgr->excl);
	}

	return (result);
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
	ns_interface_t *interface;

	REQUIRE(NS_INTERFACEMGR_VALID(mgr));

	LOCK(&mgr->lock);
	interface = ISC_LIST_HEAD(mgr->interfaces);
	while (interface != NULL) {
		if (interface->clientmgr != NULL) {
			ns_client_dumprecursing(f, interface->clientmgr);
		}
		interface = ISC_LIST_NEXT(interface, link);
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

ns_interface_t *
ns__interfacemgr_getif(ns_interfacemgr_t *mgr) {
	ns_interface_t *head;
	REQUIRE(NS_INTERFACEMGR_VALID(mgr));
	LOCK(&mgr->lock);
	head = ISC_LIST_HEAD(mgr->interfaces);
	UNLOCK(&mgr->lock);
	return (head);
}

ns_interface_t *
ns__interfacemgr_nextif(ns_interface_t *ifp) {
	ns_interface_t *next;
	LOCK(&ifp->lock);
	next = ISC_LIST_NEXT(ifp, link);
	UNLOCK(&ifp->lock);
	return (next);
}
