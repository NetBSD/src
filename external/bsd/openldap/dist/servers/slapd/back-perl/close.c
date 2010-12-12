/*	$NetBSD: close.c,v 1.1.1.3 2010/12/12 15:23:20 adam Exp $	*/

/* OpenLDAP: pkg/ldap/servers/slapd/back-perl/close.c,v 1.17.2.6 2010/04/13 20:23:37 kurt Exp */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2010 The OpenLDAP Foundation.
 * Portions Copyright 1999 John C. Quillan.
 * Portions Copyright 2002 myinternet Limited.
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

#include "perl_back.h"
#include "../config.h"
/**********************************************************
 *
 * Close
 *
 **********************************************************/

int
perl_back_close(
	BackendInfo *bd
)
{
	perl_destruct(PERL_INTERPRETER);
	perl_free(PERL_INTERPRETER);
	PERL_INTERPRETER = NULL;
#ifdef PERL_SYS_TERM
	PERL_SYS_TERM();
#endif

	ldap_pvt_thread_mutex_destroy( &perl_interpreter_mutex );	

	return 0;
}

int
perl_back_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	free( be->be_private );
	be->be_private = NULL;

	return 0;
}
