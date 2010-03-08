/*	$NetBSD: rewrite-map.h,v 1.1.1.2 2010/03/08 02:14:17 lukem Exp $	*/

/* OpenLDAP: pkg/ldap/libraries/librewrite/rewrite-map.h,v 1.7.2.4 2009/01/22 00:00:59 kurt Exp */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2009 The OpenLDAP Foundation.
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
/* ACKNOWLEDGEMENT:
 * This work was initially developed by Pierangelo Masarati for
 * inclusion in OpenLDAP Software.
 */

#ifndef MAP_H
#define MAP_H

/*
 * Retrieves a builtin map
 */
LDAP_REWRITE_F (struct rewrite_builtin_map *)
rewrite_builtin_map_find(
                struct rewrite_info *info,
                const char *name
);

#endif /* MAP_H */
