/*	$NetBSD: rewrite-map.h,v 1.1.1.6.6.1 2019/08/10 06:17:16 martin Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2019 The OpenLDAP Foundation.
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
