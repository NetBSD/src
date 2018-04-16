/*	$NetBSD: ldap_krb_helper.c,v 1.2.2.2 2018/04/16 01:59:49 pgoyette Exp $	*/

/* ldap_krb_helper.c

   Helper routings for allowing LDAP to read configuration with GSSAPI/krb auth */

/*
 * Copyright (c) 2015-2017 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 2014 William B.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This helper was written by William Brown <william@adelaide.edu.au>, 
 * inspired by krb5_helper.c from bind-dyndb-ldap by Simo Sorce (Redhat)
 */
#if defined(LDAP_USE_GSSAPI)

#include "dhcpd.h"
#include "ldap_krb_helper.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define KRB_DEFAULT_KEYTAB "FILE:/etc/dhcp/dhcp.keytab"
#define KRB_MIN_TIME 300

#define CHECK_KRB5(ctx, err, msg, ...) \
    do { \
        if (err) { \
            const char * errmsg = krb5_get_error_message(ctx, err); \
            log_error("Err: %s -> %s\n", msg, errmsg); \
            result = ISC_R_FAILURE; \
            goto cleanup; \
        } \
    } while (0)

#define CHECK(ret_code, msg) \
    if (ret_code != 0) { \
        log_error("Error, %i %s\n", ret_code, msg); \
        goto cleanup; \
    }

static isc_result_t
check_credentials(krb5_context context, krb5_ccache ccache, krb5_principal service)
{
    char *realm = NULL;
    krb5_creds creds;
    krb5_creds mcreds;
    krb5_error_code krberr;
    krb5_timestamp now;
    isc_result_t result = ISC_R_FAILURE;

    memset(&mcreds, 0, sizeof(mcreds));
    memset(&creds, 0, sizeof(creds));

    krberr = krb5_get_default_realm(context, &realm);
    CHECK_KRB5(context, krberr, "Failed to retrieve default realm");

    krberr = krb5_build_principal(context, &mcreds.server,
                    strlen(realm), realm,
                    "krbtgt", realm, NULL);
    CHECK_KRB5(context, krberr, "Failed to build 'krbtgt/REALM' principal");

    mcreds.client = service;

    krberr = krb5_cc_retrieve_cred(context, ccache, 0, &mcreds, &creds);

    if (krberr) {
        const char * errmsg = krb5_get_error_message(context, krberr);
        log_error("Credentials are not present in cache (%s)\n", errmsg);
        krb5_free_error_message(context, errmsg);
        result = ISC_R_FAILURE;
        goto cleanup;
    }
    CHECK_KRB5(context, krberr, "Credentials are not present in cache ");
   
    krberr = krb5_timeofday(context, &now);
    CHECK_KRB5(context, krberr, "Failed to get time of day");


    if (now > (creds.times.endtime + KRB_MIN_TIME)) {
        log_error("Credentials cache expired");
        result = ISC_R_FAILURE;
        goto cleanup;
    } else { 
        char buf[255];
        char fill = ' ';
        krb5_timestamp_to_sfstring(creds.times.endtime, buf, 16, &fill);
        log_info("Credentials valid til %s\n", buf);
    }

    result = ISC_R_SUCCESS;

cleanup:
    krb5_free_cred_contents(context, &creds);
    if (mcreds.server) krb5_free_principal(context, mcreds.server);
    if (realm) krb5_free_default_realm(context, realm);
    return result;
}

isc_result_t
krb5_get_tgt(const char *principal, const char *keyfile)
{
    isc_result_t result = ISC_R_FAILURE;
    char *ccname = NULL;
    krb5_context context = NULL;
    krb5_error_code krberr;
    krb5_ccache ccache = NULL;
    krb5_principal kprincpw = NULL;
    krb5_creds my_creds;
    krb5_creds * my_creds_ptr = NULL;
    krb5_get_init_creds_opt options;
    krb5_keytab keytab = NULL;
    int ret;
    
    if (keyfile == NULL || keyfile[0] == '\0') {
        keyfile = KRB_DEFAULT_KEYTAB;
        log_info("Using default keytab %s\n", keyfile);
    } else {
        if (strncmp(keyfile, "FILE:", 5) != 0) {
            log_error("Unknown keytab path format: Does it start with FILE:?\n");
            return ISC_R_FAILURE;
        }
    }

    krberr = krb5_init_context(&context);
    CHECK_KRB5(NULL, krberr, "Kerberos context initialization failed");

    result = ISC_R_SUCCESS;

    ccname = "MEMORY:dhcp_ld_krb5_cc";
    log_info("Using ccache %s\n" , ccname);

    ret = setenv("KRB5CCNAME", ccname, 1);
    if (ret == -1) {
        log_error("Failed to setup environment\n");
        result = ISC_R_FAILURE;
        goto cleanup;
    }

    krberr = krb5_cc_resolve(context, ccname, &ccache);
    CHECK_KRB5(context, krberr, "Couldnt resolve ccache '%s'", ccname);

    krberr = krb5_parse_name(context, principal, &kprincpw);
    CHECK_KRB5(context, krberr, "Failed to parse princ '%s'", princpal);

    result = check_credentials(context, ccache, kprincpw);
    if (result == ISC_R_SUCCESS) {
        log_info("Found valid kerberos credentials\n");
        goto cleanup;
    } else {
        log_error("No valid krb5 credentials\n");
    }

    krberr = krb5_kt_resolve(context, keyfile, &keytab);
    CHECK_KRB5(context, krberr, 
            "Failed to resolve kt files '%s'\n", keyfile);

    memset(&my_creds, 0, sizeof(my_creds));
    memset(&options, 0, sizeof(options));

    krb5_get_init_creds_opt_set_tkt_life(&options, KRB_MIN_TIME * 2);
    krb5_get_init_creds_opt_set_address_list(&options, NULL);
    krb5_get_init_creds_opt_set_forwardable(&options, 0);
    krb5_get_init_creds_opt_set_proxiable(&options, 0);

    krberr = krb5_get_init_creds_keytab(context, &my_creds, kprincpw,
                keytab, 0, NULL, &options);
    CHECK_KRB5(context, krberr, "Failed to get initial credentials TGT\n");
    
    my_creds_ptr = &my_creds;

    krberr = krb5_cc_initialize(context, ccache, kprincpw);
    CHECK_KRB5(context, krberr, "Failed to init ccache\n");

    krberr = krb5_cc_store_cred(context, ccache, &my_creds);
    CHECK_KRB5(context, krberr, "Failed to store credentials\n");

    result = ISC_R_SUCCESS;
    log_info("Successfully init krb tgt %s", principal);

cleanup:
    if (ccache) krb5_cc_close(context, ccache);
    if (keytab) krb5_kt_close(context, keytab);
    if (kprincpw) krb5_free_principal(context, kprincpw);
    if (my_creds_ptr) krb5_free_cred_contents(context, &my_creds);
    if (context) krb5_free_context(context);
    return result;
}

#endif /* defined(LDAP_USE_GSSAPI) */
