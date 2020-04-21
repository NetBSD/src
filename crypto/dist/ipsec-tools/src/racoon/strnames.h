/*	$NetBSD: strnames.h,v 1.4.88.2 2020/04/21 19:37:54 martin Exp $	*/

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

extern char *num2str __P((int n));

extern char *s_isakmp_state __P((int, int, int));
extern char *s_isakmp_certtype __P((int));
extern char *s_isakmp_etype __P((int));
extern char *s_isakmp_notify_msg __P((int));
extern char *s_isakmp_nptype __P((int));
extern char *s_ipsecdoi_proto __P((int));
extern char *s_ipsecdoi_trns_isakmp __P((int));
extern char *s_ipsecdoi_trns_ah __P((int));
extern char *s_ipsecdoi_trns_esp __P((int));
extern char *s_ipsecdoi_trns_ipcomp __P((int));
extern char *s_ipsecdoi_trns __P((int, int));
extern char *s_ipsecdoi_attr __P((int));
extern char *s_ipsecdoi_ltype __P((int));
extern char *s_ipsecdoi_encmode __P((int));
extern char *s_ipsecdoi_auth __P((int));
extern char *s_ipsecdoi_attr_v __P((int, int));
extern char *s_ipsecdoi_ident __P((int));
extern char *s_oakley_attr __P((int));
extern char *s_attr_isakmp_enc __P((int));
extern char *s_attr_isakmp_hash __P((int));
extern char *s_oakley_attr_method __P((int));
extern char *s_attr_isakmp_desc __P((int));
extern char *s_attr_isakmp_group __P((int));
extern char *s_attr_isakmp_ltype __P((int));
extern char *s_oakley_attr_v __P((int, int));
extern char *s_ipsec_level __P((int));
extern char *s_algclass __P((int));
extern char *s_algtype __P((int, int));
extern char *s_pfkey_type __P((int));
extern char *s_pfkey_satype __P((int));
extern char *s_direction __P((int));
extern char *s_proto __P((int));
extern char *s_doi __P((int));
extern char *s_etype __P((int));
extern char *s_idtype __P((int));
extern char *s_switch __P((int));
#ifdef ENABLE_HYBRID
extern char *s_isakmp_cfg_type __P((int));
extern char *s_isakmp_cfg_ptype __P((int));
#endif

#endif /* _STRNAMES_H */
