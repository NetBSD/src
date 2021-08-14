/*	$NetBSD: value.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

/* value.c - routines for dealing with values */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2021 The OpenLDAP Foundation.
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
/*
 * Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: value.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include "lload.h"

int
value_add_one( BerVarray *vals, struct berval *addval )
{
    int n;
    BerVarray v2;

    if ( *vals == NULL ) {
        *vals = (BerVarray)SLAP_MALLOC( 2 * sizeof(struct berval) );
        if ( *vals == NULL ) {
            Debug( LDAP_DEBUG_TRACE, "value_add_one: "
                    "SLAP_MALLOC failed.\n" );
            return LBER_ERROR_MEMORY;
        }
        n = 0;

    } else {
        for ( n = 0; !BER_BVISNULL( &(*vals)[n] ); n++ ) {
            ; /* Empty */
        }
        *vals = (BerVarray)SLAP_REALLOC(
                (char *)*vals, ( n + 2 ) * sizeof(struct berval) );
        if ( *vals == NULL ) {
            Debug( LDAP_DEBUG_TRACE, "value_add_one: "
                    "SLAP_MALLOC failed.\n" );
            return LBER_ERROR_MEMORY;
        }
    }

    v2 = &(*vals)[n];
    ber_dupbv( v2, addval );

    v2++;
    BER_BVZERO( v2 );

    return LDAP_SUCCESS;
}
