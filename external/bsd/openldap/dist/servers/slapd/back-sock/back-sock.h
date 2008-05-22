/* sock.h - socket backend header file */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-sock/back-sock.h,v 1.4.2.1 2008/02/09 00:46:09 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2007-2008 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Brian Candler for inclusion
 * in OpenLDAP Software.
 */

#ifndef SLAPD_SOCK_H
#define SLAPD_SOCK_H

#include "proto-sock.h"

LDAP_BEGIN_DECL

struct sockinfo {
	const char	*si_sockpath;
	slap_mask_t	si_extensions;
};

#define	SOCK_EXT_BINDDN	1
#define	SOCK_EXT_PEERNAME	2
#define	SOCK_EXT_SSF		4

extern FILE *opensock LDAP_P((
	const char *sockpath));

extern void sock_print_suffixes LDAP_P((
	FILE *fp,
	BackendDB *bd));

extern void sock_print_conn LDAP_P((
	FILE *fp,
	Connection *conn,
	struct sockinfo *si));

extern int sock_read_and_send_results LDAP_P((
	Operation *op,
	SlapReply *rs,
	FILE *fp));

LDAP_END_DECL

#endif
