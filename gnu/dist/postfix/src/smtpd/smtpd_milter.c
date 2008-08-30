/*++
/* NAME
/*	smtpd_milter 3
/* SUMMARY
/*	SMTP server milter glue
/* SYNOPSIS
/*	#include <smtpd.h>
/*	#include <smtpd_milter.h>
/*
/*	const char *smtpd_milter_eval(name, context)
/*	const char *name;
/*	void	*context;
/* DESCRIPTION
/*	smtpd_milter_eval() is a milter(3) call-back routine to
/*	expand Sendmail macros before they are sent to filters.
/* DIAGNOSTICS
/*	Panic: interface violations. Fatal errors: out of memory.
/*	internal protocol errors.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

/* Global library. */

#include <mail_params.h>
#include <quote_821_local.h>

/* Milter library. */

#include <milter.h>

/* Application-specific. */

#include <smtpd.h>
#include <smtpd_milter.h>

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)

/* smtpd_milter_eval - evaluate milter macro */

const char *smtpd_milter_eval(const char *name, void *ptr)
{
    SMTPD_STATE *state = (SMTPD_STATE *) ptr;

    /*
     * Canonicalize the name.
     */
    if (*name != '{') {				/* } */
	if (state->expand_buf == 0)
	    state->expand_buf = vstring_alloc(10);
	vstring_sprintf(state->expand_buf, "{%s}", name);
	name = STR(state->expand_buf);
    }

    /*
     * System macros.
     */
    if (strcmp(name, S8_MAC_DAEMON_NAME) == 0)
	return (var_milt_daemon_name);
    if (strcmp(name, S8_MAC_V) == 0)
	return (var_milt_v);

    /*
     * Connect macros.
     */
    if (strcmp(name, S8_MAC__) == 0) {
	if (state->expand_buf == 0)
	    state->expand_buf = vstring_alloc(10);
	vstring_sprintf(state->expand_buf, "%s [%s]",
			state->reverse_name, state->addr);
	if (strcasecmp(state->name, state->reverse_name) != 0)
	    vstring_strcat(state->expand_buf, " (may be forged)");
	return (STR(state->expand_buf));
    }
    if (strcmp(name, S8_MAC_J) == 0)
	return (var_myhostname);
    if (strcmp(name, S8_MAC_CLIENT_ADDR) == 0)
	return (state->rfc_addr);
    if (strcmp(name, S8_MAC_CLIENT_PORT) == 0)
	return (strcmp(state->port, CLIENT_PORT_UNKNOWN) ? state->port : "0");
    if (strcmp(name, S8_MAC_CLIENT_CONN) == 0) {
	if (state->expand_buf == 0)
	    state->expand_buf = vstring_alloc(10);
	vstring_sprintf(state->expand_buf, "%d", state->conn_count);
	return (STR(state->expand_buf));
    }
    if (strcmp(name, S8_MAC_CLIENT_NAME) == 0)
	return (state->name);
    if (strcmp(name, S8_MAC_CLIENT_PTR) == 0)
	return (state->reverse_name);
    if (strcmp(name, S8_MAC_CLIENT_RES) == 0)
	return (state->name_status == SMTPD_PEER_CODE_OK ? "OK" :
		state->name_status == SMTPD_PEER_CODE_FORGED ? "FORGED" :
	      state->name_status == SMTPD_PEER_CODE_TEMP ? "TEMP" : "FAIL");

    /*
     * HELO macros.
     */
#ifdef USE_TLS
#define IF_ENCRYPTED(x) (state->tls_context ? (x) : 0)
#define IF_TRUSTED(x) (TLS_CERT_IS_TRUSTED(state->tls_context) ? (x) : 0)

    if (strcmp(name, S8_MAC_TLS_VERSION) == 0)
	return (IF_ENCRYPTED(state->tls_context->protocol));
    if (strcmp(name, S8_MAC_CIPHER) == 0)
	return (IF_ENCRYPTED(state->tls_context->cipher_name));
    if (strcmp(name, S8_MAC_CIPHER_BITS) == 0) {
	if (state->tls_context == 0)
	    return (0);
	if (state->expand_buf == 0)
	    state->expand_buf = vstring_alloc(10);
	vstring_sprintf(state->expand_buf, "%d",
			IF_ENCRYPTED(state->tls_context->cipher_usebits));
	return (STR(state->expand_buf));
    }
    if (strcmp(name, S8_MAC_CERT_SUBJECT) == 0)
	return (IF_TRUSTED(state->tls_context->peer_CN));
    if (strcmp(name, S8_MAC_CERT_ISSUER) == 0)
	return (IF_TRUSTED(state->tls_context->issuer_CN));
#endif

    /*
     * MAIL FROM macros.
     */
#define IF_SASL_ENABLED(s) (var_smtpd_sasl_enable && (s) ? (s) : 0)

    if (strcmp(name, S8_MAC_I) == 0)
	return (state->queue_id);
#ifdef USE_SASL_AUTH
    if (strcmp(name, S8_MAC_AUTH_TYPE) == 0)
	return (IF_SASL_ENABLED(state->sasl_method));
    if (strcmp(name, S8_MAC_AUTH_AUTHEN) == 0)
	return (IF_SASL_ENABLED(state->sasl_username));
    if (strcmp(name, S8_MAC_AUTH_AUTHOR) == 0)
	return (IF_SASL_ENABLED(state->sasl_sender));
#endif
    if (strcmp(name, S8_MAC_MAIL_ADDR) == 0) {
	if (state->sender == 0)
	    return (0);
	if (state->expand_buf == 0)
	    state->expand_buf = vstring_alloc(10);
	/* Sendmail 8.13 does not externalize the null string. */
	if (state->sender[0])
	    quote_821_local(state->expand_buf, state->sender);
	else
	    vstring_strcpy(state->expand_buf, state->sender);
	return (STR(state->expand_buf));
    }

    /*
     * RCPT TO macros.
     */
    if (strcmp(name, S8_MAC_RCPT_ADDR) == 0) {
	if (state->recipient == 0)
	    return (0);
	if (state->expand_buf == 0)
	    state->expand_buf = vstring_alloc(10);
	/* Sendmail 8.13 does not externalize the null string. */
	if (state->recipient[0])
	    quote_821_local(state->expand_buf, state->recipient);
	else
	    vstring_strcpy(state->expand_buf, state->recipient);
	return (STR(state->expand_buf));
    }
    return (0);
}
