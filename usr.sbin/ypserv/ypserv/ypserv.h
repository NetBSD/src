/*	$NetBSD: ypserv.h,v 1.2 1997/07/30 22:55:30 jtc Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

struct ypresp_all {
	bool_t more;
	union {
		struct ypresp_key_val val;
	} ypresp_all_u;
};

void	*ypproc_null_2_svc __P((void *, struct svc_req *));
void	*ypproc_domain_2_svc __P((void *, struct svc_req *));
void	*ypproc_domain_nonack_2_svc __P((void *, struct svc_req *));
void	*ypproc_match_2_svc __P((void *, struct svc_req *));
void	*ypproc_first_2_svc __P((void *, struct svc_req *));
void	*ypproc_next_2_svc __P((void *, struct svc_req *));
void	*ypproc_xfr_2_svc __P((void *, struct svc_req *));
void	*ypproc_clear_2_svc __P((void *, struct svc_req *));
void	*ypproc_all_2_svc __P((void *, struct svc_req *));
void	*ypproc_master_2_svc __P((void *, struct svc_req *));
void	*ypproc_order_2_svc __P((void *, struct svc_req *));
void	*ypproc_maplist_2_svc __P((void *, struct svc_req *));

bool_t	xdr_ypresp_all __P((XDR *, struct ypresp_all *));

struct ypresp_val ypdb_get_record __P((const char *, const char *, datum, int));
struct ypresp_key_val ypdb_get_first __P((const char *, const char *, int));
struct ypresp_key_val ypdb_get_next __P((const char *, const char *,
	    datum, int));
struct ypresp_order ypdb_get_order __P((const char *, const char *));
struct ypresp_master ypdb_get_master __P((const char *, const char *));

void	ypdb_init __P((void));
bool_t	ypdb_xdr_get_all __P((XDR *, struct ypreq_nokey *));
void	ypdb_close_all __P((void));
