/* result.c - sock backend result reading function */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-sock/result.c,v 1.3.2.2 2008/07/08 21:05:03 quanah Exp $ */
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

#include "portable.h"

#include <stdio.h>

#include <ac/errno.h>
#include <ac/string.h>
#include <ac/socket.h>
#include <ac/unistd.h>

#include "slap.h"
#include "back-sock.h"

/*
 * FIXME: make a RESULT section compulsory from the socket response.
 * Otherwise, a partial/aborted response is treated as 'success'.
 * This is a divergence from the back-shell protocol, but makes things
 * more robust.
 */

int
sock_read_and_send_results(
    Operation	*op,
    SlapReply	*rs,
    FILE	*fp )
{
	int	bsize, len;
	char	*buf, *bp;
	char	line[BUFSIZ];
	char	ebuf[128];

	/* read in the result and send it along */
	buf = (char *) ch_malloc( BUFSIZ );
	buf[0] = '\0';
	bsize = BUFSIZ;
	bp = buf;
	while ( !feof(fp) ) {
		errno = 0;
		if ( fgets( line, sizeof(line), fp ) == NULL ) {
			if ( errno == EINTR ) continue;

			Debug( LDAP_DEBUG_ANY, "sock: fgets failed: %s (%d)\n",
				AC_STRERROR_R(errno, ebuf, sizeof ebuf), errno, 0 ); 
			break;
		}

		Debug( LDAP_DEBUG_SHELL, "sock search reading line (%s)\n",
		    line, 0, 0 );

		/* ignore lines beginning with # (LDIFv1 comments) */
		if ( *line == '#' ) {
			continue;
		}

		/* ignore lines beginning with DEBUG: */
		if ( strncasecmp( line, "DEBUG:", 6 ) == 0 ) {
			continue;
		}

		len = strlen( line );
		while ( bp + len + 1 - buf > bsize ) {
			size_t offset = bp - buf;
			bsize += BUFSIZ;
			buf = (char *) ch_realloc( buf, bsize );
			bp = &buf[offset];
		}
		strcpy( bp, line );
		bp += len;

		/* line marked the end of an entry or result */
		if ( *line == '\n' ) {
			if ( strncasecmp( buf, "RESULT", 6 ) == 0 ) {
				break;
			}

			if ( (rs->sr_entry = str2entry( buf )) == NULL ) {
				Debug( LDAP_DEBUG_ANY, "str2entry(%s) failed\n",
				    buf, 0, 0 );
			} else {
				rs->sr_attrs = op->oq_search.rs_attrs;
				rs->sr_flags = REP_ENTRY_MODIFIABLE;
				send_search_entry( op, rs );
				entry_free( rs->sr_entry );
			}

			bp = buf;
		}
	}
	(void) str2result( buf, &rs->sr_err, (char **)&rs->sr_matched, (char **)&rs->sr_text );

	/* otherwise, front end will send this result */
	if ( rs->sr_err != 0 || op->o_tag != LDAP_REQ_BIND ) {
		send_ldap_result( op, rs );
	}

	free( buf );

	return( rs->sr_err );
}

void
sock_print_suffixes(
    FILE	*fp,
    Backend	*be
)
{
	int	i;

	for ( i = 0; be->be_suffix[i].bv_val != NULL; i++ ) {
		fprintf( fp, "suffix: %s\n", be->be_suffix[i].bv_val );
	}
}

void
sock_print_conn(
    FILE	*fp,
    Connection	*conn,
    struct sockinfo *si
)
{
	if ( conn == NULL ) return;

	if( si->si_extensions & SOCK_EXT_BINDDN ) {
		fprintf( fp, "binddn: %s\n",
			conn->c_dn.bv_len ? conn->c_dn.bv_val : "" );
	}
	if( si->si_extensions & SOCK_EXT_PEERNAME ) {
		fprintf( fp, "peername: %s\n",
			conn->c_peer_name.bv_len ? conn->c_peer_name.bv_val : "" );
	}
	if( si->si_extensions & SOCK_EXT_SSF ) {
		fprintf( fp, "ssf: %d\n", conn->c_ssf );
	}
}
