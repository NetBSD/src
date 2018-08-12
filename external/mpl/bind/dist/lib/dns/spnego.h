/*	$NetBSD: spnego.h,v 1.1.1.1 2018/08/12 12:08:08 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */


/*! \file
 * \brief
 * Entry points into portable SPNEGO implementation.
 * See spnego.c for information on the SPNEGO implementation itself.
 */

#ifndef _SPNEGO_H_
#define _SPNEGO_H_

/*%
 * Wrapper for GSSAPI gss_init_sec_context(), using portable SPNEGO
 * implementation instead of the one that's part of the GSSAPI
 * library.  Takes arguments identical to the standard GSSAPI
 * function, uses standard gss_init_sec_context() to handle
 * everything inside the SPNEGO wrapper.
 */
OM_uint32
gss_init_sec_context_spnego(OM_uint32 *,
			    const gss_cred_id_t,
			    gss_ctx_id_t *,
			    const gss_name_t,
			    const gss_OID,
			    OM_uint32,
			    OM_uint32,
			    const gss_channel_bindings_t,
			    const gss_buffer_t,
			    gss_OID *,
			    gss_buffer_t,
			    OM_uint32 *,
			    OM_uint32 *);

/*%
 * Wrapper for GSSAPI gss_accept_sec_context(), using portable SPNEGO
 * implementation instead of the one that's part of the GSSAPI
 * library.  Takes arguments identical to the standard GSSAPI
 * function.  Checks the OID of the input token to see if it's SPNEGO;
 * if so, processes it, otherwise hands the call off to the standard
 * gss_accept_sec_context() function.
 */
OM_uint32 gss_accept_sec_context_spnego(OM_uint32 *,
					gss_ctx_id_t *,
					const gss_cred_id_t,
					const gss_buffer_t,
					const gss_channel_bindings_t,
					gss_name_t *,
					gss_OID *,
					gss_buffer_t,
					OM_uint32 *,
					OM_uint32 *,
					gss_cred_id_t *);


#endif
