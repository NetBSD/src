/* $NetBSD: krb5_passwd.c,v 1.8 2000/06/20 06:00:37 thorpej Exp $ */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to 
 * The NetBSD Foundation by Johan Danielsson.
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

/* uses the `Kerberos Change Password Protocol' */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <err.h>

#include <openssl/des.h>
#include <krb5.h>

#include "extern.h"

static krb5_context context;
static krb5_principal defprinc;
static int usage = PW_USE;

int
krb5_init(const char *progname)
{
    return krb5_init_context(&context);
}

int
krb5_arg (char ch, const char *optarg)
{
    krb5_error_code ret;
    switch(ch) {
    case '5':
    case 'k':
	usage = PW_USE_FORCE;
	return 1;
    case 'u':
	ret = krb5_parse_name(context, optarg, &defprinc);
	if(ret) {
	    krb5_warn(context, ret, "%s", optarg);
	    return 0;
	}
	return 1;
    }
    return 0;
}

int
krb5_arg_end(void)
{
    return usage;
}

void
krb5_end(void)
{
    if(defprinc)
	krb5_free_principal(context, defprinc);
    krb5_free_context(context);
}


int
krb5_chpw(const char *username)
{
    krb5_error_code ret;
    krb5_context context;
    krb5_principal principal;
    krb5_get_init_creds_opt opt;
    krb5_creds cred;
    int result_code;
    krb5_data result_code_string, result_string;
    char pwbuf[BUFSIZ];

    ret = krb5_init_context (&context);
    if (ret) {
	warnx("failed kerberos initialisation: %s", 
	      krb5_get_err_text(context, ret));
	return 1;
    }

    krb5_get_init_creds_opt_init (&opt);
    
    krb5_get_init_creds_opt_set_tkt_life (&opt, 300);
    krb5_get_init_creds_opt_set_forwardable (&opt, FALSE);
    krb5_get_init_creds_opt_set_proxiable (&opt, FALSE);

    if(username != NULL) {
        ret = krb5_parse_name (context, username, &principal);
        if (ret) {
	    warnx("failed to parse principal: %s", 
		  krb5_get_err_text(context, ret));
	    return 1;
	}
    } else
        principal = defprinc;
    
    ret = krb5_get_init_creds_password (context,
                                        &cred,
                                        principal,
                                        NULL,
                                        krb5_prompter_posix,
                                        NULL,
                                        0,
                                        "kadmin/changepw",
                                        &opt);

    switch (ret) {
    case 0:
        break;
    case KRB5_LIBOS_PWDINTR :
	/* XXX */
        return 1;
    case KRB5KRB_AP_ERR_BAD_INTEGRITY :
    case KRB5KRB_AP_ERR_MODIFIED :
	fprintf(stderr, "Password incorrect\n");
	return 1;
        break;
    default:
	warnx("failed to get credentials: %s", 
	      krb5_get_err_text(context, ret));
	return 1;
    }
    krb5_data_zero (&result_code_string);
    krb5_data_zero (&result_string);

    /* XXX use getpass? It has a broken interface. */
    if(des_read_pw_string (pwbuf, sizeof(pwbuf), "New password: ", 1) != 0)
        return 1;

    ret = krb5_change_password (context, &cred, pwbuf,
                                &result_code,
                                &result_code_string,
                                &result_string);
    if (ret)
        krb5_err (context, 1, ret, "krb5_change_password");

    printf ("%.*s\n", (int)result_string.length, (char *)result_string.data);

    krb5_data_free (&result_code_string);
    krb5_data_free (&result_string);
    
    krb5_free_creds_contents (context, &cred);
    krb5_free_context (context);
    return result_code;
}
