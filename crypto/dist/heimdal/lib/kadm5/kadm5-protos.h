/*
 * Copyright (c) 2000-2002 Kungliga Tekniska Högskolan
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

/* $NetBSD: kadm5-protos.h,v 1.2 2002/09/12 13:19:10 joda Exp $ */

/* This is a generated file */
#ifndef __kadm5_protos_h__
#define __kadm5_protos_h__

#include <stdarg.h>

const char *
kadm5_check_password_quality (
	krb5_context /*context*/,
	krb5_principal /*principal*/,
	krb5_data */*pwd_data*/);

kadm5_ret_t
kadm5_chpass_principal (
	void */*server_handle*/,
	krb5_principal /*princ*/,
	char */*password*/);

kadm5_ret_t
kadm5_chpass_principal_with_key (
	void */*server_handle*/,
	krb5_principal /*princ*/,
	int /*n_key_data*/,
	krb5_key_data */*key_data*/);

kadm5_ret_t
kadm5_create_principal (
	void */*server_handle*/,
	kadm5_principal_ent_t /*princ*/,
	u_int32_t /*mask*/,
	char */*password*/);

kadm5_ret_t
kadm5_delete_principal (
	void */*server_handle*/,
	krb5_principal /*princ*/);

kadm5_ret_t
kadm5_destroy (void */*server_handle*/);

kadm5_ret_t
kadm5_flush (void */*server_handle*/);

void
kadm5_free_key_data (
	void */*server_handle*/,
	int16_t */*n_key_data*/,
	krb5_key_data */*key_data*/);

void
kadm5_free_name_list (
	void */*server_handle*/,
	char **/*names*/,
	int */*count*/);

void
kadm5_free_principal_ent (
	void */*server_handle*/,
	kadm5_principal_ent_t /*princ*/);

kadm5_ret_t
kadm5_get_principal (
	void */*server_handle*/,
	krb5_principal /*princ*/,
	kadm5_principal_ent_t /*out*/,
	u_int32_t /*mask*/);

kadm5_ret_t
kadm5_get_principals (
	void */*server_handle*/,
	const char */*exp*/,
	char ***/*princs*/,
	int */*count*/);

kadm5_ret_t
kadm5_get_privs (
	void */*server_handle*/,
	u_int32_t */*privs*/);

kadm5_ret_t
kadm5_init_with_creds (
	const char */*client_name*/,
	krb5_ccache /*ccache*/,
	const char */*service_name*/,
	kadm5_config_params */*realm_params*/,
	unsigned long /*struct_version*/,
	unsigned long /*api_version*/,
	void **/*server_handle*/);

kadm5_ret_t
kadm5_init_with_creds_ctx (
	krb5_context /*context*/,
	const char */*client_name*/,
	krb5_ccache /*ccache*/,
	const char */*service_name*/,
	kadm5_config_params */*realm_params*/,
	unsigned long /*struct_version*/,
	unsigned long /*api_version*/,
	void **/*server_handle*/);

kadm5_ret_t
kadm5_init_with_password (
	const char */*client_name*/,
	const char */*password*/,
	const char */*service_name*/,
	kadm5_config_params */*realm_params*/,
	unsigned long /*struct_version*/,
	unsigned long /*api_version*/,
	void **/*server_handle*/);

kadm5_ret_t
kadm5_init_with_password_ctx (
	krb5_context /*context*/,
	const char */*client_name*/,
	const char */*password*/,
	const char */*service_name*/,
	kadm5_config_params */*realm_params*/,
	unsigned long /*struct_version*/,
	unsigned long /*api_version*/,
	void **/*server_handle*/);

kadm5_ret_t
kadm5_init_with_skey (
	const char */*client_name*/,
	const char */*keytab*/,
	const char */*service_name*/,
	kadm5_config_params */*realm_params*/,
	unsigned long /*struct_version*/,
	unsigned long /*api_version*/,
	void **/*server_handle*/);

kadm5_ret_t
kadm5_init_with_skey_ctx (
	krb5_context /*context*/,
	const char */*client_name*/,
	const char */*keytab*/,
	const char */*service_name*/,
	kadm5_config_params */*realm_params*/,
	unsigned long /*struct_version*/,
	unsigned long /*api_version*/,
	void **/*server_handle*/);

kadm5_ret_t
kadm5_modify_principal (
	void */*server_handle*/,
	kadm5_principal_ent_t /*princ*/,
	u_int32_t /*mask*/);

kadm5_ret_t
kadm5_randkey_principal (
	void */*server_handle*/,
	krb5_principal /*princ*/,
	krb5_keyblock **/*new_keys*/,
	int */*n_keys*/);

kadm5_ret_t
kadm5_rename_principal (
	void */*server_handle*/,
	krb5_principal /*source*/,
	krb5_principal /*target*/);

kadm5_ret_t
kadm5_ret_key_data (
	krb5_storage */*sp*/,
	krb5_key_data */*key*/);

kadm5_ret_t
kadm5_ret_principal_ent (
	krb5_storage */*sp*/,
	kadm5_principal_ent_t /*princ*/);

kadm5_ret_t
kadm5_ret_principal_ent_mask (
	krb5_storage */*sp*/,
	kadm5_principal_ent_t /*princ*/,
	u_int32_t */*mask*/);

kadm5_ret_t
kadm5_ret_tl_data (
	krb5_storage */*sp*/,
	krb5_tl_data */*tl*/);

void
kadm5_setup_passwd_quality_check (
	krb5_context /*context*/,
	const char */*check_library*/,
	const char */*check_function*/);

kadm5_ret_t
kadm5_store_key_data (
	krb5_storage */*sp*/,
	krb5_key_data */*key*/);

kadm5_ret_t
kadm5_store_principal_ent (
	krb5_storage */*sp*/,
	kadm5_principal_ent_t /*princ*/);

kadm5_ret_t
kadm5_store_principal_ent_mask (
	krb5_storage */*sp*/,
	kadm5_principal_ent_t /*princ*/,
	u_int32_t /*mask*/);

kadm5_ret_t
kadm5_store_tl_data (
	krb5_storage */*sp*/,
	krb5_tl_data */*tl*/);

#endif /* __kadm5_protos_h__ */
