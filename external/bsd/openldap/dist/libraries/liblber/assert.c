/*	$NetBSD: assert.c,v 1.1.1.3 2010/12/12 15:21:27 adam Exp $	*/

/* OpenLDAP: pkg/ldap/libraries/liblber/assert.c,v 1.13.2.5 2010/04/13 20:22:53 kurt Exp */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2010 The OpenLDAP Foundation.
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
