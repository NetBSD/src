/*	$NetBSD: interfaceiter.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */


#ifndef ISC_INTERFACEITER_H
#define ISC_INTERFACEITER_H 1

/*****
 ***** Module Info
 *****/

/*! \file isc/interfaceiter.h
 * \brief Iterates over the list of network interfaces.
 *
 * Interfaces whose address family is not supported are ignored and never
 * returned by the iterator.  Interfaces whose netmask, interface flags,
 * or similar cannot be obtained are also ignored, and the failure is logged.
 *
 * Standards:
 *	The API for scanning varies greatly among operating systems.
 *	This module attempts to hide the differences.
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>
#include <isc/netaddr.h>
#include <isc/types.h>

/*!
 * \brief Public structure describing a network interface.
 */

struct isc_interface {
	char name[32];			/*%< Interface name, null-terminated. */
	unsigned int af;		/*%< Address family. */
	isc_netaddr_t address;		/*%< Local address. */
	isc_netaddr_t netmask;		/*%< Network mask. */
	isc_netaddr_t dstaddress; 	/*%< Destination address (point-to-point only). */
	isc_uint32_t flags;		/*%< Flags; see INTERFACE flags. */
};

/*@{*/
/*! Interface flags. */

#define INTERFACE_F_UP			0x00000001U
#define INTERFACE_F_POINTTOPOINT	0x00000002U
#define INTERFACE_F_LOOPBACK		0x00000004U
/*@}*/

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
isc_interfaceiter_create(isc_mem_t *mctx, isc_interfaceiter_t **iterp);
/*!<
 * \brief Create an iterator for traversing the operating system's list
 * of network interfaces.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 *\li	Various network-related errors
 */

isc_result_t
isc_interfaceiter_first(isc_interfaceiter_t *iter);
/*!<
 * \brief Position the iterator on the first interface.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS		Success.
 *\li	#ISC_R_NOMORE		There are no interfaces.
 */

isc_result_t
isc_interfaceiter_current(isc_interfaceiter_t *iter,
			  isc_interface_t *ifdata);
/*!<
 * \brief Get information about the interface the iterator is currently
 * positioned at and store it at *ifdata.
 *
 * Requires:
 *\li 	The iterator has been successfully positioned using
 * 	isc_interface_iter_first() / isc_interface_iter_next().
 *
 * Returns:
 *\li	#ISC_R_SUCCESS		Success.
 */

isc_result_t
isc_interfaceiter_next(isc_interfaceiter_t *iter);
/*!<
 * \brief Position the iterator on the next interface.
 *
 * Requires:
 * \li	The iterator has been successfully positioned using
 * 	isc_interface_iter_first() / isc_interface_iter_next().
 *
 * Returns:
 *\li	#ISC_R_SUCCESS		Success.
 *\li	#ISC_R_NOMORE		There are no more interfaces.
 */

void
isc_interfaceiter_destroy(isc_interfaceiter_t **iterp);
/*!<
 * \brief Destroy the iterator.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_INTERFACEITER_H */
