/*	$NetBSD: gssapi.h,v 1.4 2006/09/09 16:22:09 manu Exp $	*/

/* Id: gssapi.h,v 1.5 2005/02/11 06:59:01 manubsd Exp */

/*
 * Copyright 2000 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * This software was written by Frank van der Linden of Wasabi Systems
 * for Zembu Labs, Inc. http://www.zembu.com/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef __FreeBSD__
#include "/usr/include/gssapi.h"
#else
#include <gssapi/gssapi.h>
#endif

#define GSSAPI_DEF_NAME         "host"

struct ph1handle;
struct isakmpsa;

struct gssapi_ph1_state {
	int gsscnt;			/* # of token we're working on */
	int gsscnt_p;			/* # of token we're working on */

	gss_buffer_desc gss[3];		/* gss-api tokens. */
					/* NOTE: XXX this restricts the max # */
					/* to 3. More should never happen */

	gss_buffer_desc gss_p[3];

	gss_ctx_id_t gss_context;	/* context for gss_init_sec_context */

	OM_uint32 gss_status;		/* retval from gss_init_sec_context */
	gss_cred_id_t gss_cred;		/* acquired credentials */

	int gss_flags;
#define GSSFLAG_ID_SENT		0x0001
#define GSSFLAG_ID_RCVD		0x0001
};

#define	gssapi_get_state(ph)						\
	((struct gssapi_ph1_state *)((ph)->gssapi_state))

#define	gssapi_set_state(ph, st)					\
	(ph)->gssapi_state = (st)

#define	gssapi_more_tokens(ph)						\
	((gssapi_get_state(ph)->gss_status & GSS_S_CONTINUE_NEEDED) != 0)

int gssapi_get_itoken __P((struct ph1handle *, int *));
int gssapi_get_rtoken __P((struct ph1handle *, int *));
int gssapi_save_received_token __P((struct ph1handle *, vchar_t *));
int gssapi_get_token_to_send __P((struct ph1handle *, vchar_t **));
int gssapi_get_itokens __P((struct ph1handle *, vchar_t **));
int gssapi_get_rtokens __P((struct ph1handle *, vchar_t **));
vchar_t *gssapi_wraphash __P((struct ph1handle *));
vchar_t *gssapi_unwraphash __P((struct ph1handle *));
void gssapi_set_id_sent __P((struct ph1handle *));
int gssapi_id_sent __P((struct ph1handle *));
void gssapi_set_id_rcvd __P((struct ph1handle *));
int gssapi_id_rcvd __P((struct ph1handle *));
void gssapi_free_state __P((struct ph1handle *));
vchar_t *gssapi_get_id __P((struct ph1handle *));
vchar_t *gssapi_get_default_gss_id __P((void));
