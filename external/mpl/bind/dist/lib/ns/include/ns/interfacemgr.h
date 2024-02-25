/*	$NetBSD: interfacemgr.h,v 1.9.2.1 2024/02/25 15:47:35 martin Exp $	*/

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

#pragma once

/*****
***** Module Info
*****/

/*! \file
 * \brief
 * The interface manager monitors the operating system's list
 * of network interfaces, creating and destroying listeners
 * as needed.
 *
 * Reliability:
 *\li	No impact expected.
 *
 * Resources:
 *
 * Security:
 * \li	The server will only be able to bind to the DNS port on
 *	newly discovered interfaces if it is running as root.
 *
 * Standards:
 *\li	The API for scanning varies greatly among operating systems.
 *	This module attempts to hide the differences.
 */

/***
 *** Imports
 ***/

#include <stdbool.h>

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/refcount.h>
#include <isc/result.h>

#include <dns/geoip.h>

#include <ns/listenlist.h>
#include <ns/types.h>

/***
 *** Types
 ***/

#define IFACE_MAGIC	      ISC_MAGIC('I', ':', '-', ')')
#define NS_INTERFACE_VALID(t) ISC_MAGIC_VALID(t, IFACE_MAGIC)

#define NS_INTERFACEFLAG_ANYADDR   0x01U /*%< bound to "any" address */
#define NS_INTERFACEFLAG_LISTENING 0x02U /*%< listening */
#define MAX_UDP_DISPATCH                           \
	128 /*%< Maximum number of UDP dispatchers \
	     *           to start per interface */
/*% The nameserver interface structure */
struct ns_interface {
	unsigned int	   magic; /*%< Magic number. */
	ns_interfacemgr_t *mgr;	  /*%< Interface manager. */
	isc_mutex_t	   lock;
	unsigned int	   generation; /*%< Generation number. */
	isc_sockaddr_t	   addr;       /*%< Address and port. */
	unsigned int	   flags;      /*%< Interface flags */
	char		   name[32];   /*%< Null terminated. */
	isc_nmsocket_t	  *udplistensocket;
	isc_nmsocket_t	  *tcplistensocket;
	isc_nmsocket_t	  *http_listensocket;
	isc_nmsocket_t	  *http_secure_listensocket;
	isc_quota_t	  *http_quota;
	isc_refcount_t	   ntcpaccepting; /*%< Number of clients
					   *   ready to accept new
					   *   TCP connections on this
					   *   interface */
	isc_refcount_t ntcpactive;	  /*%< Number of clients
					   *   servicing TCP queries
					   *   (whether accepting or
					   *   connected) */
	ns_clientmgr_t *clientmgr;	  /*%< Client manager. */
	ISC_LINK(ns_interface_t) link;
};

/***
 *** Functions
 ***/

isc_result_t
ns_interfacemgr_create(isc_mem_t *mctx, ns_server_t *sctx,
		       isc_taskmgr_t *taskmgr, isc_timermgr_t *timermgr,
		       isc_nm_t *nm, dns_dispatchmgr_t *dispatchmgr,
		       isc_task_t *task, dns_geoip_databases_t *geoip,
		       int ncpus, bool scan, ns_interfacemgr_t **mgrp);
/*%<
 * Create a new interface manager.
 *
 * Initially, the new manager will not listen on any interfaces.
 * Call ns_interfacemgr_setlistenon() and/or ns_interfacemgr_setlistenon6()
 * to set nonempty listen-on lists.
 */

void
ns_interfacemgr_attach(ns_interfacemgr_t *source, ns_interfacemgr_t **target);

void
ns_interfacemgr_detach(ns_interfacemgr_t **targetp);

void
ns_interfacemgr_shutdown(ns_interfacemgr_t *mgr);

void
ns_interfacemgr_setbacklog(ns_interfacemgr_t *mgr, int backlog);
/*%<
 * Set the size of the listen() backlog queue.
 */

bool
ns_interfacemgr_islistening(ns_interfacemgr_t *mgr);
/*%<
 * Return if the manager is listening on any interface. It can be called
 * after a scan or adjust.
 */

isc_result_t
ns_interfacemgr_scan(ns_interfacemgr_t *mgr, bool verbose, bool config);
/*%<
 * Scan the operatings system's list of network interfaces
 * and create listeners when new interfaces are discovered.
 * Shut down the sockets for interfaces that go away.
 *
 * When 'config' is true, also shut down and recreate any existing TLS and HTTPS
 * interfaces in order to use their new configuration.
 *
 * This should be called once on server startup and then
 * periodically according to the 'interface-interval' option
 * in named.conf.
 */

void
ns_interfacemgr_setlistenon4(ns_interfacemgr_t *mgr, ns_listenlist_t *value);
/*%<
 * Set the IPv4 "listen-on" list of 'mgr' to 'value'.
 * The previous IPv4 listen-on list is freed.
 */

void
ns_interfacemgr_setlistenon6(ns_interfacemgr_t *mgr, ns_listenlist_t *value);
/*%<
 * Set the IPv6 "listen-on" list of 'mgr' to 'value'.
 * The previous IPv6 listen-on list is freed.
 */

dns_aclenv_t *
ns_interfacemgr_getaclenv(ns_interfacemgr_t *mgr);

void
ns_interface_shutdown(ns_interface_t *ifp);
/*%<
 * Stop listening for queries on interface 'ifp'.
 * May safely be called multiple times.
 */

void
ns_interfacemgr_dumprecursing(FILE *f, ns_interfacemgr_t *mgr);

bool
ns_interfacemgr_listeningon(ns_interfacemgr_t *mgr, const isc_sockaddr_t *addr);

ns_server_t *
ns_interfacemgr_getserver(ns_interfacemgr_t *mgr);
/*%<
 * Returns the ns_server object associated with the interface manager.
 */

ns_clientmgr_t *
ns_interfacemgr_getclientmgr(ns_interfacemgr_t *mgr);
/*%<
 *
 * Returns the client manager for the current worker thread.
 * (This cannot be run from outside a network manager thread.)
 */
