/*	$NetBSD: compare.c,v 1.3 2021/08/14 16:14:59 christos Exp $	*/

/* compare.c - DNS SRV backend compare function */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2021 The OpenLDAP Foundation.
 * Portions Copyright 2000-2003 Kurt D. Zeilenga.
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
 * This work was originally developed by Kurt D. Zeilenga for inclusion
 * in OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: compare.c,v 1.3 2021/08/14 16:14:59 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "proto-dnssrv.h"

int
dnssrv_back_compare(
	Operation	*op,
	SlapReply	*rs
)
{
#if 0
	assert( get_manageDSAit( op ) );
#endif
	send_ldap_error( op, rs, LDAP_OTHER,
		"Operation not supported within naming context" );

	/* not implemented */
	return 1;
}
