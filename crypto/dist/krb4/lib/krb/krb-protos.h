/*
 * Copyright (c) 1997 - 2000 Kungliga Tekniska Högskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */

/* $KTH-KRB: krb-protos.h,v 1.33 2001/08/26 01:46:51 assar Exp $
   $NetBSD: krb-protos.h,v 1.5 2002/09/19 19:22:53 joda Exp $ */

#ifndef __krb_protos_h__
#define __krb_protos_h__

#include <stdarg.h>
struct in_addr;
struct sockaddr_in;
struct timeval;

void
afs_string_to_key (
	const char */*pass*/,
	const char */*cell*/,
	des_cblock */*key*/);

int
cr_err_reply (
	KTEXT /*pkt*/,
	char */*pname*/,
	char */*pinst*/,
	char */*prealm*/,
	u_int32_t /*time_ws*/,
	u_int32_t /*e*/,
	char */*e_string*/);

int
create_ciph (
	KTEXT /*c*/,
	unsigned char */*session*/,
	char */*service*/,
	char */*instance*/,
	char */*realm*/,
	u_int32_t /*life*/,
	int /*kvno*/,
	KTEXT /*tkt*/,
	u_int32_t /*kdc_time*/,
	des_cblock */*key*/);

int
decomp_ticket (
	KTEXT /*tkt*/,
	unsigned char */*flags*/,
	char */*pname*/,
	char */*pinstance*/,
	char */*prealm*/,
	u_int32_t */*paddress*/,
	unsigned char */*session*/,
	int */*life*/,
	u_int32_t */*time_sec*/,
	char */*sname*/,
	char */*sinstance*/,
	des_cblock */*key*/,
	des_key_schedule /*schedule*/);

int
dest_tkt (void);

void
encrypt_ktext (
	KTEXT /*cip*/,
	des_cblock */*key*/,
	int /*encrypt*/);

int
get_ad_tkt (
	char */*service*/,
	char */*instance*/,
	char */*realm*/,
	int /*lifetime*/);

int
getst (
	int /*fd*/,
	char */*s*/,
	int /*n*/);

int
in_tkt (
	char */*pname*/,
	char */*pinst*/);

int
k_get_all_addrs (struct in_addr **/*l*/);

int
k_getportbyname (
	const char */*service*/,
	const char */*proto*/,
	int /*default_port*/);

int
k_getsockinst (
	int /*fd*/,
	char */*inst*/,
	size_t /*inst_size*/);

int
k_isinst (char */*s*/);

int
k_isname (char */*s*/);

int
k_isrealm (char */*s*/);

struct tm *
k_localtime (u_int32_t */*tp*/);

int
kname_parse (
	char */*np*/,
	char */*ip*/,
	char */*rp*/,
	char */*fullname*/);

int
krb_add_our_ip_for_realm (
	const char */*user*/,
	const char */*instance*/,
	const char */*realm*/,
	const char */*password*/);

int
krb_atime_to_life (char */*atime*/);

int
krb_check_auth (
	KTEXT /*packet*/,
	u_int32_t /*checksum*/,
	MSG_DAT */*msg_data*/,
	des_cblock */*session*/,
	struct des_ks_struct */*schedule*/,
	struct sockaddr_in */*laddr*/,
	struct sockaddr_in */*faddr*/);

int
krb_check_tm (struct tm /*tm*/);

KTEXT
krb_create_death_packet (char */*a_name*/);

int
krb_create_ticket (
	KTEXT /*tkt*/,
	unsigned char /*flags*/,
	char */*pname*/,
	char */*pinstance*/,
	char */*prealm*/,
	int32_t /*paddress*/,
	void */*session*/,
	int16_t /*life*/,
	int32_t /*time_sec*/,
	char */*sname*/,
	char */*sinstance*/,
	des_cblock */*key*/);

int
krb_decode_as_rep (
	const char */*user*/,
	char */*instance*/,
	const char */*realm*/,
	const char */*service*/,
	const char */*sinstance*/,
	key_proc_t /*key_proc*/,
	decrypt_proc_t /*decrypt_proc*/,
	const void */*arg*/,
	KTEXT /*as_rep*/,
	CREDENTIALS */*cred*/);

int
krb_disable_debug (void);

int
krb_enable_debug (void);

int
krb_equiv (
	u_int32_t /*a*/,
	u_int32_t /*b*/);

void
krb_generate_random_block (
	void */*buf*/,
	size_t /*len*/);

int
krb_get_address (
	void */*from*/,
	u_int32_t */*to*/);

int
krb_get_admhst (
	char */*host*/,
	char */*realm*/,
	int /*nth*/);

int
krb_get_config_bool (const char */*variable*/);

const char *
krb_get_config_string (const char */*variable*/);

int
krb_get_cred (
	const char */*service*/,
	const char */*instance*/,
	const char */*realm*/,
	CREDENTIALS */*c*/);

int
krb_get_cred_kdc (
	const char */*service*/,
	const char */*instance*/,
	const char */*realm*/,
	int /*lifetime*/,
	CREDENTIALS */*ret_cred*/);

int
krb_get_credentials (
	const char */*service*/,
	const char */*instance*/,
	const char */*realm*/,
	CREDENTIALS */*cred*/);

const char *
krb_get_default_keyfile (void);

int
krb_get_default_principal (
	char */*name*/,
	char */*instance*/,
	char */*realm*/);

char *
krb_get_default_realm (void);

const char *
krb_get_default_tkt_root (void);

const char *
krb_get_err_text (int /*code*/);

struct krb_host*
krb_get_host (
	int /*nth*/,
	const char */*realm*/,
	int /*admin*/);

int
krb_get_in_tkt (
	char */*user*/,
	char */*instance*/,
	char */*realm*/,
	char */*service*/,
	char */*sinstance*/,
	int /*life*/,
	key_proc_t /*key_proc*/,
	decrypt_proc_t /*decrypt_proc*/,
	void */*arg*/);

int
krb_get_int (
	void */*f*/,
	u_int32_t */*to*/,
	int /*size*/,
	int /*lsb*/);

int
krb_get_kdc_time_diff (void);

int
krb_get_krbconf (
	int /*num*/,
	char */*buf*/,
	size_t /*len*/);

int
krb_get_krbextra (
	int /*num*/,
	char */*buf*/,
	size_t /*len*/);

int
krb_get_krbhst (
	char */*host*/,
	char */*realm*/,
	int /*nth*/);

int
krb_get_krbrealms (
	int /*num*/,
	char */*buf*/,
	size_t /*len*/);

int
krb_get_lrealm (
	char */*r*/,
	int /*n*/);

int
krb_get_nir (
	void */*from*/,
	char */*name*/,
	size_t /*name_len*/,
	char */*instance*/,
	size_t /*instance_len*/,
	char */*realm*/,
	size_t /*realm_len*/);

int
krb_get_our_ip_for_realm (
	const char */*realm*/,
	struct in_addr */*ip_addr*/);

char *
krb_get_phost (const char */*alias*/);

int
krb_get_pw_in_tkt (
	const char */*user*/,
	const char */*instance*/,
	const char */*realm*/,
	const char */*service*/,
	const char */*sinstance*/,
	int /*life*/,
	const char */*password*/);

int
krb_get_pw_in_tkt2 (
	const char */*user*/,
	const char */*instance*/,
	const char */*realm*/,
	const char */*service*/,
	const char */*sinstance*/,
	int /*life*/,
	const char */*password*/,
	des_cblock */*key*/);

int
krb_get_string (
	void */*from*/,
	char */*to*/,
	size_t /*to_size*/);

int
krb_get_svc_in_tkt (
	char */*user*/,
	char */*instance*/,
	char */*realm*/,
	char */*service*/,
	char */*sinstance*/,
	int /*life*/,
	char */*srvtab*/);

int
krb_get_tf_fullname (
	char */*ticket_file*/,
	char */*name*/,
	char */*instance*/,
	char */*realm*/);

int
krb_get_tf_realm (
	char */*ticket_file*/,
	char */*realm*/);

void
krb_kdctimeofday (struct timeval */*tv*/);

int
krb_kntoln (
	AUTH_DAT */*ad*/,
	char */*lname*/);

int
krb_kuserok (
	char */*name*/,
	char */*instance*/,
	char */*realm*/,
	char */*luser*/);

char *
krb_life_to_atime (int /*life*/);

u_int32_t
krb_life_to_time (
	u_int32_t /*start*/,
	int /*life_*/);

int
krb_lsb_antinet_ulong_cmp (
	u_int32_t /*x*/,
	u_int32_t /*y*/);

int
krb_lsb_antinet_ushort_cmp (
	u_int16_t /*x*/,
	u_int16_t /*y*/);

int
krb_mk_as_req (
	const char */*user*/,
	const char */*instance*/,
	const char */*realm*/,
	const char */*service*/,
	const char */*sinstance*/,
	int /*life*/,
	KTEXT /*cip*/);

int
krb_mk_auth (
	int32_t /*options*/,
	KTEXT /*ticket*/,
	char */*service*/,
	char */*instance*/,
	char */*realm*/,
	u_int32_t /*checksum*/,
	char */*version*/,
	KTEXT /*buf*/);

int32_t
krb_mk_err (
	u_char */*p*/,
	int32_t /*e*/,
	char */*e_string*/);

int32_t
krb_mk_priv (
	void */*in*/,
	void */*out*/,
	u_int32_t /*length*/,
	struct des_ks_struct */*schedule*/,
	des_cblock */*key*/,
	struct sockaddr_in */*sender*/,
	struct sockaddr_in */*receiver*/);

int
krb_mk_req (
	KTEXT /*authent*/,
	const char */*service*/,
	const char */*instance*/,
	const char */*realm*/,
	int32_t /*checksum*/);

int32_t
krb_mk_safe (
	void */*in*/,
	void */*out*/,
	u_int32_t /*length*/,
	des_cblock */*key*/,
	struct sockaddr_in */*sender*/,
	struct sockaddr_in */*receiver*/);

int
krb_net_read (
	int /*fd*/,
	void */*buf*/,
	size_t /*nbytes*/);

int
krb_net_write (
	int /*fd*/,
	const void */*buf*/,
	size_t /*nbytes*/);

int
krb_parse_name (
	const char */*fullname*/,
	krb_principal */*principal*/);

int
krb_put_address (
	u_int32_t /*addr*/,
	void */*to*/,
	size_t /*rem*/);

int
krb_put_int (
	u_int32_t /*from*/,
	void */*to*/,
	size_t /*rem*/,
	int /*size*/);

int
krb_put_nir (
	const char */*name*/,
	const char */*instance*/,
	const char */*realm*/,
	void */*to*/,
	size_t /*rem*/);

int
krb_put_string (
	const char */*from*/,
	void */*to*/,
	size_t /*rem*/);

int
krb_rd_err (
	u_char */*in*/,
	u_int32_t /*in_length*/,
	int32_t */*code*/,
	MSG_DAT */*m_data*/);

int32_t
krb_rd_priv (
	void */*in*/,
	u_int32_t /*in_length*/,
	struct des_ks_struct */*schedule*/,
	des_cblock */*key*/,
	struct sockaddr_in */*sender*/,
	struct sockaddr_in */*receiver*/,
	MSG_DAT */*m_data*/);

int
krb_rd_req (
	KTEXT /*authent*/,
	char */*service*/,
	char */*instance*/,
	int32_t /*from_addr*/,
	AUTH_DAT */*ad*/,
	char */*a_fn*/);

int32_t
krb_rd_safe (
	void */*in*/,
	u_int32_t /*in_length*/,
	des_cblock */*key*/,
	struct sockaddr_in */*sender*/,
	struct sockaddr_in */*receiver*/,
	MSG_DAT */*m_data*/);

int
krb_realm_parse (
	char */*realm*/,
	int /*length*/);

char *
krb_realmofhost (const char */*host*/);

int
krb_recvauth (
	int32_t /*options*/,
	int /*fd*/,
	KTEXT /*ticket*/,
	char */*service*/,
	char */*instance*/,
	struct sockaddr_in */*faddr*/,
	struct sockaddr_in */*laddr*/,
	AUTH_DAT */*kdata*/,
	char */*filename*/,
	struct des_ks_struct */*schedule*/,
	char */*version*/);

int
krb_sendauth (
	int32_t /*options*/,
	int /*fd*/,
	KTEXT /*ticket*/,
	char */*service*/,
	char */*instance*/,
	char */*realm*/,
	u_int32_t /*checksum*/,
	MSG_DAT */*msg_data*/,
	CREDENTIALS */*cred*/,
	struct des_ks_struct */*schedule*/,
	struct sockaddr_in */*laddr*/,
	struct sockaddr_in */*faddr*/,
	char */*version*/);

void
krb_set_kdc_time_diff (int /*diff*/);

int
krb_set_key (
	void */*key*/,
	int /*cvt*/);

int
krb_set_lifetime (int /*newval*/);

void
krb_set_tkt_string (const char */*val*/);

const char *
krb_stime (time_t */*t*/);

int
krb_time_to_life (
	u_int32_t /*start*/,
	u_int32_t /*end*/);

char *
krb_unparse_name (krb_principal */*pr*/);

char *
krb_unparse_name_long (
	char */*name*/,
	char */*instance*/,
	char */*realm*/);

char *
krb_unparse_name_long_r (
	char */*name*/,
	char */*instance*/,
	char */*realm*/,
	char */*fullname*/);

char *
krb_unparse_name_r (
	krb_principal */*pr*/,
	char */*fullname*/);

int
krb_use_admin_server (int /*flag*/);

int
krb_verify_user (
	char */*name*/,
	char */*instance*/,
	char */*realm*/,
	char */*password*/,
	int /*secure*/,
	char */*linstance*/);

int
krb_verify_user_srvtab (
	char */*name*/,
	char */*instance*/,
	char */*realm*/,
	char */*password*/,
	int /*secure*/,
	char */*linstance*/,
	char */*srvtab*/);

int
kuserok (
	AUTH_DAT */*auth*/,
	char */*luser*/);

u_int32_t
lsb_time (
	time_t /*t*/,
	struct sockaddr_in */*src*/,
	struct sockaddr_in */*dst*/);

const char *
month_sname (int /*n*/);

int
passwd_to_5key (
	const char */*user*/,
	const char */*instance*/,
	const char */*realm*/,
	const void */*passwd*/,
	des_cblock */*key*/);

int
passwd_to_afskey (
	const char */*user*/,
	const char */*instance*/,
	const char */*realm*/,
	const void */*passwd*/,
	des_cblock */*key*/);

int
passwd_to_key (
	const char */*user*/,
	const char */*instance*/,
	const char */*realm*/,
	const void */*passwd*/,
	des_cblock */*key*/);

int
read_service_key (
	const char */*service*/,
	char */*instance*/,
	const char */*realm*/,
	int /*kvno*/,
	const char */*file*/,
	void */*key*/);

int
save_credentials (
	char */*service*/,
	char */*instance*/,
	char */*realm*/,
	unsigned char */*session*/,
	int /*lifetime*/,
	int /*kvno*/,
	KTEXT /*ticket*/,
	int32_t /*issue_date*/);

int
save_credentials_cred (CREDENTIALS */*cred*/);

int
send_to_kdc (
	KTEXT /*pkt*/,
	KTEXT /*rpkt*/,
	const char */*realm*/);

int
srvtab_to_key (
	const char */*user*/,
	char */*instance*/,
	const char */*realm*/,
	const void */*srvtab*/,
	des_cblock */*key*/);

void
tf_close (void);

int
tf_create (char */*tf_name*/);

int
tf_get_addr (
	const char */*realm*/,
	struct in_addr */*addr*/);

int
tf_get_cred (CREDENTIALS */*c*/);

int
tf_get_cred_addr (
	char */*realm*/,
	size_t /*realm_sz*/,
	struct in_addr */*addr*/);

int
tf_get_pinst (char */*inst*/);

int
tf_get_pname (char */*p*/);

int
tf_init (
	char */*tf_name*/,
	int /*rw*/);

int
tf_put_pinst (const char */*inst*/);

int
tf_put_pname (const char */*p*/);

int
tf_replace_cred (CREDENTIALS */*cred*/);

int
tf_save_cred (
	char */*service*/,
	char */*instance*/,
	char */*realm*/,
	unsigned char */*session*/,
	int /*lifetime*/,
	int /*kvno*/,
	KTEXT /*ticket*/,
	u_int32_t /*issue_date*/);

int
tf_setup (
	CREDENTIALS */*cred*/,
	const char */*pname*/,
	const char */*pinst*/);

int
tf_store_addr (
	const char */*realm*/,
	struct in_addr */*addr*/);

char *
tkt_string (void);

#endif /* __krb_protos_h__ */
