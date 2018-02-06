/*	$NetBSD: memcmp.c,v 1.1.1.6 2018/02/06 01:53:08 christos Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2017 The OpenLDAP Foundation.
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
__RCSID("$NetBSD: memcmp.c,v 1.1.1.6 2018/02/06 01:53:08 christos Exp $");

#include "portable.h"

#include <ac/string.h>

/* 
 * Memory Compare
 */
int
(lutil_memcmp)(const void *v1, const void *v2, size_t n) 
{
    if (n != 0) {
		const unsigned char *s1=v1, *s2=v2;
        do {
            if (*s1++ != *s2++) return *--s1 - *--s2;
        } while (--n != 0);
    }
    return 0;
} 
