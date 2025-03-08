/*	$NetBSD: strnames.h,v 1.6 2025/03/08 16:39:08 christos Exp $	*/

/* Id: strnames.h,v 1.7 2005/04/18 10:04:26 manubsd Exp */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _STRNAMES_H
#define _STRNAMES_H

extern const char *num2str(int n);

extern const char *s_isakmp_state(int, int, int);
extern const char *s_isakmp_certtype(int);
extern const char *s_isakmp_etype(int);
extern const char *s_isakmp_notify_msg(int);
extern const char *s_isakmp_nptype(int);
extern const char *s_ipsecdoi_proto(int);
extern const char *s_ipsecdoi_trns_isakmp(int);
extern const char *s_ipsecdoi_trns_ah(int);
extern const char *s_ipsecdoi_trns_esp(int);
extern const char *s_ipsecdoi_trns_ipcomp(int);
extern const char *s_ipsecdoi_trns(int, int);
extern const char *s_ipsecdoi_attr(int);
extern const char *s_ipsecdoi_ltype(int);
extern const char *s_ipsecdoi_encmode(int);
extern const char *s_ipsecdoi_auth(int);
extern const char *s_ipsecdoi_attr_v(int, int);
extern const char *s_ipsecdoi_ident(int);
extern const char *s_oakley_attr(int);
extern const char *s_attr_isakmp_enc(int);
extern const char *s_attr_isakmp_hash(int);
extern const char *s_oakley_attr_method(int);
extern const char *s_attr_isakmp_desc(int);
extern const char *s_attr_isakmp_group(int);
extern const char *s_attr_isakmp_ltype(int);
extern const char *s_oakley_attr_v(int, int);
extern const char *s_ipsec_level(int);
extern const char *s_algclass(int);
extern const char *s_algtype(int, int);
extern const char *s_pfkey_type(int);
extern const char *s_pfkey_satype(int);
extern const char *s_direction(int);
extern const char *s_proto(int);
extern const char *s_doi(int);
extern const char *s_etype(int);
extern const char *s_idtype(int);
extern const char *s_switch(int);
#ifdef ENABLE_HYBRID
extern const char *s_isakmp_cfg_type(int);
extern const char *s_isakmp_cfg_ptype(int);
#endif

#endif /* _STRNAMES_H */
