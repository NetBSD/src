/*	$NetBSD: config.c,v 1.1.1.6.6.1 2019/08/10 06:17:18 martin Exp $	*/

/* config.c - DNS SRV backend configuration file routine */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2019 The OpenLDAP Foundation.
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
__RCSID("$NetBSD: config.c,v 1.1.1.6.6.1 2019/08/10 06:17:18 martin Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "proto-dnssrv.h"

#if 0
int
dnssrv_back_db_config(
    BackendDB	*be,
    const char	*fname,
    int		lineno,
    int		argc,
    char	**argv )
{
#if 0
	struct ldapinfo	*li = (struct ldapinfo *) be->be_private;

	if ( li == NULL ) {
		fprintf( stderr, "%s: line %d: DNSSRV backend info is null!\n",
		    fname, lineno );
		return( 1 );
	}
#endif

	/* no configuration options (yet) */
	return SLAP_CONF_UNKNOWN;
}
#endif
