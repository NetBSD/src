/*	$NetBSD: bind.c,v 1.1.1.6.6.1 2019/08/10 06:17:19 martin Exp $	*/

/* bind.c - monitor backend bind routine */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2001-2019 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: bind.c,v 1.1.1.6.6.1 2019/08/10 06:17:19 martin Exp $");

#include "portable.h"

#include <stdio.h>

#include <slap.h>
#include "back-monitor.h"

/*
 * At present, only rootdn can bind with simple bind
 */

int
monitor_back_bind( Operation *op, SlapReply *rs )
{
	Debug(LDAP_DEBUG_ARGS, "==> monitor_back_bind: dn: %s\n", 
			op->o_req_dn.bv_val, 0, 0 );

	if ( be_isroot_pw( op ) ) {
		return LDAP_SUCCESS;
	}

	rs->sr_err = LDAP_INVALID_CREDENTIALS;
	send_ldap_result( op, rs );

	return rs->sr_err;
}

