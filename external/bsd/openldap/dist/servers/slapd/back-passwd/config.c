/* config.c - passwd backend configuration file routine */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-passwd/config.c,v 1.14.2.3 2008/02/11 23:26:47 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
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
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "back-passwd.h"

int
passwd_back_db_config(
    BackendDB	*be,
    const char	*fname,
    int		lineno,
    int		argc,
    char	**argv
)
{
	/* alternate passwd file */
	if ( strcasecmp( argv[0], "file" ) == 0 ) {
#ifdef HAVE_SETPWFILE
		if ( argc < 2 ) {
			fprintf( stderr,
		"%s: line %d: missing filename in \"file <filename>\" line\n",
			    fname, lineno );
			return( 1 );
		}
		be->be_private = ch_strdup( argv[1] );
#else /* HAVE_SETPWFILE */
		fprintf( stderr,
    "%s: line %d: ignoring \"file\" option (not supported on this platform)\n",
			    fname, lineno );
#endif /* HAVE_SETPWFILE */

	/* anything else */
	} else {
		return SLAP_CONF_UNKNOWN;
	}

	return( 0 );
}
