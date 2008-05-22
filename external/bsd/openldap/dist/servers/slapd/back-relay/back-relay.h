/* back-relay.h - relay backend header file */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-relay/back-relay.h,v 1.6.2.3 2008/02/12 01:03:16 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2008 The OpenLDAP Foundation.
 * Portions Copyright 2004 Pierangelo Masarati.
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
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#ifndef SLAPD_RELAY_H
#define SLAPD_RELAY_H

#include "proto-back-relay.h"

/* String rewrite library */

LDAP_BEGIN_DECL

typedef struct relay_back_info {
	BackendDB	*ri_bd;
	struct berval	ri_realsuffix;
	int		ri_massage;
} relay_back_info;

LDAP_END_DECL

#endif /* SLAPD_RELAY_H */
