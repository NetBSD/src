/*	$NetBSD: assert.c,v 1.1.1.5 2017/02/09 01:46:44 christos Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2016 The OpenLDAP Foundation.
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: assert.c,v 1.1.1.5 2017/02/09 01:46:44 christos Exp $");

#include "portable.h"

#ifdef LDAP_NEED_ASSERT

#include <stdio.h>

/*
 * helper for our private assert() macro
 *
 * note: if assert() doesn't exist, like abort() or raise() won't either.
 * could use kill() but that might be problematic.  I'll just ignore this
 * issue for now.
 */

void
ber_pvt_assert( const char *file, int line, const char *test )
{
	fprintf(stderr,
		_("Assertion failed: %s, file %s, line %d\n"),
			test, file, line);

	abort();
}

#endif
