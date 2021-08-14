/*	$NetBSD: proto-asyncmeta.h,v 1.2 2021/08/14 16:14:59 christos Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2016-2021 The OpenLDAP Foundation.
 * Portions Copyright 2016 Symas Corporation.
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
 * This work was developed by Symas Corporation
 * based on back-meta module for inclusion in OpenLDAP Software.
 * This work was sponsored by Ericsson. */

#ifndef PROTO_ASYNCMETA_H
#define PROTO_ASYNCMETA_H

LDAP_BEGIN_DECL

extern BI_init			asyncmeta_back_initialize;

extern BI_open			asyncmeta_back_open;
extern BI_close			asyncmeta_back_close;
extern BI_destroy		asyncmeta_back_destroy;

extern BI_db_init		asyncmeta_back_db_init;
extern BI_db_open		asyncmeta_back_db_open;
extern BI_db_destroy		asyncmeta_back_db_destroy;
extern BI_db_close		asyncmeta_back_db_close;
extern BI_db_config		asyncmeta_back_db_config;

extern BI_op_bind		asyncmeta_back_bind;
extern BI_op_search		asyncmeta_back_search;
extern BI_op_compare		asyncmeta_back_compare;
extern BI_op_modify		asyncmeta_back_modify;
extern BI_op_modrdn		asyncmeta_back_modrdn;
extern BI_op_add		asyncmeta_back_add;
extern BI_op_delete		asyncmeta_back_delete;

extern BI_connection_destroy	asyncmeta_back_conn_destroy;

int asyncmeta_back_init_cf( BackendInfo *bi );

LDAP_END_DECL

#endif /* PROTO_ASYNCMETA_H */
