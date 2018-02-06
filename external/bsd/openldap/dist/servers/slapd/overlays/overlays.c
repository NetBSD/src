/*	$NetBSD: overlays.c,v 1.1.1.6 2018/02/06 01:53:15 christos Exp $	*/

/* overlays.c - Static overlay framework */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2017 The OpenLDAP Foundation.
 * Copyright 2003 by Howard Chu.
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
 * This work was initially developed by Howard Chu for inclusion in
 * OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: overlays.c,v 1.1.1.6 2018/02/06 01:53:15 christos Exp $");

#include "portable.h"

#include "slap.h"

extern OverlayInit	slap_oinfo[];

int
overlay_init(void)
{
	int i, rc = 0;

	for ( i= 0 ; slap_oinfo[i].ov_type; i++ ) {
		rc = slap_oinfo[i].ov_init();
		if ( rc ) {
			Debug( LDAP_DEBUG_ANY,
				"%s overlay setup failed, err %d\n",
				slap_oinfo[i].ov_type, rc, 0 );
			break;
		}
	}

	return rc;
}
