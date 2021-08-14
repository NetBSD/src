/*	$NetBSD: lload-config.h,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

/* lload-config.h - configuration abstraction structure */
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

#ifndef LLOAD_CONFIG_H /* not CONFIG_H because it overlaps with the one from slapd */
#define LLOAD_CONFIG_H

#include <ac/string.h>
#include "../slapd/slap-config.h"

LDAP_BEGIN_DECL

int lload_config_fp_parse_line( ConfigArgs *c );

int lload_config_get_vals( ConfigTable *ct, ConfigArgs *c );
int lload_config_add_vals( ConfigTable *ct, ConfigArgs *c );

void lload_init_config_argv( ConfigArgs *c );
int lload_read_config_file( const char *fname, int depth, ConfigArgs *cf, ConfigTable *cft );

ConfigTable *lload_config_find_keyword( ConfigTable *ct, ConfigArgs *c );

LloadListener *lload_config_check_my_url( const char *url, LDAPURLDesc *lud );

LDAP_END_DECL

#endif /* LLOAD_CONFIG_H */
