/*	$NetBSD: smtp_tlsrpt.c,v 1.2 2025/02/25 19:15:49 christos Exp $	*/

/*++
/* NAME
/*	smtp_tlsrpt 3
/* SUMMARY
/*	TLSRPT support for the SMTP protocol engine
/* SYNOPSIS
/*	#include <smtp_tlsrpt.h>
/*
/*	int	smtp_tlsrpt_post_jail(
/*	const char *sockname_pname,
/*	const char *sockname_pval)
/*
/*	void	smtp_tlsrpt_create_wrapper(
/*	SMTP_STATE *state,
/*	const char *domain)
/*
/*	void	smtp_tlsrpt_set_tls_policy(
/*	SMTP_STATE *state)
/*
/*	void	smtp_tlsrpt_set_tcp_connection(
/*	SMTP_STATE *state)
/*
/*	void	smtp_tlsrpt_set_ehlo_resp(
/*	SMTP_STATE *state,
/*	const char *ehlo_resp)
/* DESCRIPTION
/*	This module populates a TLSRPT_WRAPPER object with  a)
/*	remote TLSRPT policy information, b) remote TLSA or STS policy
/*	information, and c) selected SMTP connection information. This
/*	object is passed to a TLS protocol engine, which may run in a
/*	different process than the SMTP protocol engine. The TLS protocol
/*	engine uses the TLSRPT_WRAPPER object to report a TLS handshake
/*	error to a TLSRPT library. The SMTP protocol engine uses the
/*	object to report a TLS handshake error or success.
/*
/*	smtp_tls_post_jail() does configuration sanity checks and returns
/*	0 if successful, i.e. TLSRPT support is properly
/*	configured. Otherwise it returns -1 and logs a warning. Arguments:
/* .IP sockname_pname
/*	The name of a configuration parameter for the endpoint that
/*	is managed by TLSRPT infrastructure. This name is used in a
/*	diagnostic message.
/* .IP sockname_pval
/*	The value of said parameter.
/* .PP
/*	smtp_tlsrpt_create_wrapper() destroys a TLSRPT_WRAPPER referenced
/*	by state->tlsrpt, and looks for a TLSRPT policy for the specified
/*	domain. If one policy exists, smtp_tlsrpt_create_wrapper()
/*	attaches a TLSRPT_WRAPPER instance to state->tlsrpt. Otherwise,
/*	state->tlsrpt will be null, and other smtp_tlsrpt_* calls must not
/*	be made. The TLSRPT_WRAPPER instance may be reused for different
/*	SMTP connections for the same TLSRPT policy domain. Arguments:
/* .IP domain
/*	The name of a domain that may publish a TLSRPT policy. An
/*	internationalized domain name may be in U-label or A-label form
/*	(the U-label form will be converted to A-label internally).
/* .PP
/*	smtp_tlsrpt_set_tls_policy() updates the TLSRPT_WRAPPER
/*	object with DANE or STS TLS policy information, and clears
/*	information that was added with smtp_tlsrpt_set_tcp_connection()
/*	or smtp_tlsrpt_set_ehlo_resp().
/* .PP
/*	smtp_tlsrpt_set_tcp_connection() updates the TLSRPT_WRAPPER
/*	object with TCP connection properties.
/* .PP
/*	smtp_tlsrpt_set_ehlo_resp() updates the TLSRPT_WRAPPER object
/*	with the SMTP server's EHLO response.
/* BUGS
/*	This module inherits all limitations from tlsrpt_wrapper(3).
/* SEE ALSO
/*	tlsrpt_wrapper(3) TLSRPT support for the TLS protocol engine.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <sys/socket.h>

 /*
  * Utility library.
  */
#include <hex_code.h>
#include <midna_domain.h>
#include <msg.h>
#include <myaddrinfo.h>
#include <name_code.h>
#include <stringops.h>

 /*
  * Global library.
  */
#include <mail_params.h>

 /*
  * TLS library.
  */
#include <tls.h>
#include <tlsrpt_wrapper.h>

 /*
  * Application-specific.
  */
#include <smtp.h>

#if defined(USE_TLS) && defined(USE_TLSRPT)

static const char smtp_tlsrpt_support[] = "TLSRPT support";

/* smtp_tlsrpt_post_jail - post-jail configuration sanity check */

int     smtp_tlsrpt_post_jail(const char *sockname_pname,
			              const char *sockname_pval)
{
    if (smtp_dns_support == SMTP_DNS_DISABLED) {
	msg_warn("Cannot enable %s: DNS is disabled", smtp_tlsrpt_support);
	return (-1);
    }
    if (*sockname_pval == 0) {
	msg_warn("%s: parameter %s has empty value -- %s will be disabled",
		 smtp_tlsrpt_support, sockname_pname, smtp_tlsrpt_support);
	return (-1);
    }
    return (0);
}

/* smtp_tlsrpt_find_policy - look up TLSRPT policy and verify version ID */

static DNS_RR *smtp_tlsrpt_find_policy(const char *adomain)
{
    VSTRING *why = vstring_alloc(100);
    VSTRING *qname = vstring_alloc(100);
    DNS_RR *rr_list = 0;
    DNS_RR *rr_result = 0;
    DNS_RR *rr;
    DNS_RR *next;
    int     res_opt = 0;
    int     dns_status;

    /*
     * Preliminaries.
     */
    if (smtp_dns_support == SMTP_DNS_DNSSEC)
	res_opt |= RES_USE_DNSSEC;

    /*
     * Lexical features: As specified in RFC 8460, a TLSRPT policy record
     * must start with a version field ("v=TLSRPTv1") followed by *WSP;*WSP
     * and at least one other field (we must not assume that the second field
     * will be "rua"). We leave further validation to the code that actually
     * needs it.
     */
#define TLSRPTv1_MAGIC		"v=TLSRPTv1"
#define TLSRPTv1_MAGIC_LEN	(sizeof(TLSRPTv1_MAGIC) - 1)
#define RFC5234_WSP		" \t"

    /*
     * Look up TXT records. Ignore records that don't start with the expected
     * version ID, and require that there is exactly one such DNS record.
     */
    vstring_sprintf(qname, "_smtp._tls.%s", adomain);
    dns_status = dns_lookup(STR(qname), T_TXT, res_opt, &rr_list,
			    (VSTRING *) 0, why);
    vstring_free(qname);
    if (dns_status != DNS_OK) {
	switch (dns_status) {
	case DNS_NOTFOUND:
	case DNS_POLICY:
	    /* Expected results. */
	    break;
	default:
	    /* Unexpected results. */
	    msg_warn("%s: policy lookup failed for %s: %s",
		     smtp_tlsrpt_support, adomain, STR(why));
	}
    } else {
	for (rr = rr_list; rr; rr = next) {
	    char   *cp;

	    next = rr->next;
	    if (strncmp(rr->data, TLSRPTv1_MAGIC, TLSRPTv1_MAGIC_LEN) != 0)
		/* Ignore non-TLSRPTv1 info. */
		continue;
	    cp = rr->data + TLSRPTv1_MAGIC_LEN;

	    /*
	     * Should the TLSRPT library validate the entire policy for us?
	     */
	    if (cp[strspn(cp, RFC5234_WSP)] != ';') {
		msg_warn("%s: ignoring malformed policy for %s:, \"%s\"",
			 smtp_tlsrpt_support, adomain, rr->data);
		continue;
	    }
	    if (rr_result) {
		msg_warn("%s: Too many TLSRPT policies for %s",
			 smtp_tlsrpt_support, adomain);
		dns_rr_free(rr_result);
		rr_result = 0;
		break;
	    }
	    rr_result = rr;
	    rr_list = dns_rr_detach(rr_list, rr);
	}
    }
    vstring_free(why);
    if (rr_list)
	dns_rr_free(rr_list);
    return (rr_result);
}

/* smtp_tlsrpt_create_wrapper - look up policy and attach TLSRPT_WRAPPER */

void    smtp_tlsrpt_create_wrapper(SMTP_STATE *state, const char *domain)
{
    const char *adomain;
    DNS_RR *rr;

    /*
     * TODO(wietse): document in a suitable place that state->tlsrpt exists
     * only if the next-hop domain announces a TLSRPT policy.
     */
    if (state->tlsrpt) {
	trw_free(state->tlsrpt);
	state->tlsrpt = 0;
    }

    /*
     * IDNA support. An internationalized domain name must be in A-label form
     * 1) for TLSRPT summaries and 2) for DNS lookups. The A-label lookup
     * result comes from a limited-size in-process cache, so it does not
     * matter that the SMTP client requests the same mapping later.
     */
#ifndef NO_EAI
    if (!allascii(domain) && (adomain = midna_domain_to_ascii(domain)) != 0) {
	if (msg_verbose)
	    msg_info("%s: internationalized domain %s asciified to %s",
		     smtp_tlsrpt_support, domain, adomain);
    } else
#endif
	adomain = domain;

    if ((rr = smtp_tlsrpt_find_policy(adomain)) != 0) {
	if (msg_verbose)
	    msg_info("%s: domain %s has policy %.100s",
		     smtp_tlsrpt_support, domain, rr->data);
	state->tlsrpt = trw_create(
			    /* rpt_socket_name= */ var_smtp_tlsrpt_sockname,
				    /* rpt_policy_domain= */ adomain,
				    /* rpt_policy_string= */ rr->data,
		     /* skip_reused_hs = */ var_smtp_tlsrpt_skip_reused_hs);
	dns_rr_free(rr);
    } else {
	if (msg_verbose)
	    msg_info("%s: no policy for domain %s",
		     smtp_tlsrpt_support, domain);
    }
}

/* smtp_tlsrpt_set_no_policy - no policy found */

static void smtp_tlsrpt_set_no_policy(SMTP_STATE *state)
{
    trw_set_tls_policy(state->tlsrpt, TLSRPT_NO_POLICY_FOUND,
		        /* tls_policy_strings= */ (const char *const *) 0,
		        /* tls_policy_domain= */ (char *) 0,
		        /* mx_host_patterns= */ (const char *const *) 0);
}

/* smtp_tlsrpt_set_dane_policy - add DANE policy properties */

static void smtp_tlsrpt_set_dane_policy(SMTP_STATE *state)
{
    VSTRING *buf = vstring_alloc(200);
    ARGV   *argv = argv_alloc(10);
    TLS_DANE *dane = state->tls->dane;
    TLS_TLSA *tlsa;

    for (tlsa = dane->tlsa; tlsa != 0; tlsa = tlsa->next) {
	vstring_sprintf(buf, "%d %d %d ", tlsa->usage,
			tlsa->selector, tlsa->mtype);
	hex_encode_opt(buf, (char *) tlsa->data, tlsa->length,
		       HEX_ENCODE_FLAG_APPEND);
	argv_add(argv, STR(buf), (char *) 0);
    }
    trw_set_tls_policy(state->tlsrpt, TLSRPT_POLICY_TLSA,
		       (const char *const *) argv->argv, dane->base_domain,
		        /* mx_host_patterns= */ (const char *const *) 0);
    argv_free(argv);
    vstring_free(buf);
}

/* smtp_tlsrpt_set_ext_policy - add external policy from smtp_tls_policy_maps */

static void smtp_tlsrpt_set_ext_policy(SMTP_STATE *state)
{
    SMTP_TLS_POLICY *tls = state->tls;
    tlsrpt_policy_type_t policy_type_val;

    if (tls->ext_policy_type == 0)
	msg_panic("smtp_tlsrpt_set_ext_policy: no policy type");

    switch (policy_type_val =
	    convert_tlsrpt_policy_type(tls->ext_policy_type)) {
    case TLSRPT_POLICY_STS:
	trw_set_tls_policy(state->tlsrpt, policy_type_val,
			(const char *const *) tls->ext_policy_strings->argv,
			   tls->ext_policy_domain,
		     (const char *const *) tls->ext_mx_host_patterns->argv);
	break;
    case TLSRPT_NO_POLICY_FOUND:
	smtp_tlsrpt_set_no_policy(state);
	break;
    default:
	/* Policy type must be validated in smtp_tls_policy_maps parser. */
	msg_panic("unexpected policy type: \"%s\"",
		  tls->ext_policy_type);
    }

    /*
     * TODO(wietse) propagate tls->policy_failure to force policy enforcement
     * to fail with the indicated error, and prevent a false positive match
     * when a certificate would satisfy conventional PKI constraints.
     */
}

/* smtp_tlsrpt_set_tls_policy - set built-in or external policy */

void    smtp_tlsrpt_set_tls_policy(SMTP_STATE *state)
{
    SMTP_TLS_POLICY *tls = state->tls;

    if (TLS_DANE_BASED(tls->level)) {		/* Desired by local policy */
	if (tls->dane != 0)			/* Actual policy */
	    smtp_tlsrpt_set_dane_policy(state);
	else					/* No policy */
	    smtp_tlsrpt_set_no_policy(state);
    } else if (tls->ext_policy_type) {
	smtp_tlsrpt_set_ext_policy(state);
    } else {
	smtp_tlsrpt_set_no_policy(state);
    }
}

/* smtp_tlsrpt_set_tcp_connection - set TCP connection info from SMTP_STATE */

void    smtp_tlsrpt_set_tcp_connection(SMTP_STATE *state)
{
    SMTP_ITERATOR *iter = state->iterator;
    SMTP_SESSION *session = state->session;
    MAI_HOSTADDR_STR client_addr;
    struct sockaddr_storage addr_storage;
    SOCKADDR_SIZE addr_storage_len = sizeof(addr_storage);
    int     aierr;

    /*
     * Get the IP client address string. The Postfix SMTP_ITERATOR already
     * contains strings with server-side connection information.
     */
    if (getsockname(vstream_fileno(session->stream),
		    (struct sockaddr *) &addr_storage,
		    &addr_storage_len) < 0) {
	msg_warn("%s: getsockname() failed (%m)"
		 " skipping the ignoring client-side IP address",
		 smtp_tlsrpt_support);
	client_addr.buf[0] = 0;
    } else if ((aierr = sane_sockaddr_to_hostaddr(
					  (struct sockaddr *) &addr_storage,
					     addr_storage_len, &client_addr,
						  (MAI_SERVPORT_STR *) 0,
						  SOCK_STREAM)) != 0) {
	msg_warn("%s: cannot convert IP address to string (%s)"
		 " -- skipping the client-side IP address",
		 smtp_tlsrpt_support, MAI_STRERROR(aierr));
	client_addr.buf[0] = 0;
    }
    trw_set_tcp_connection(state->tlsrpt, client_addr.buf, STR(iter->host),
			   STR(iter->addr));
}

/* smtp_tlsrpt_set_ehlo_resp - format and set EHLO response */

void    smtp_tlsrpt_set_ehlo_resp(SMTP_STATE *state, const char *reply)
{
    ARGV   *argv;
    VSTRING *buf;
    char  **cpp;

    /*
     * Generate SMTP-style line breaks ("\r\n") for a multiline response.
     * Internally, smtp_chat_resp() returns a multiline response as text
     * separated with "\n". This is because Postfix by design removes
     * protocol-specific line endings on input, uses its own internal form to
     * represent text lines, and generates protocol-specific line endings on
     * output. The conversion to "\r\n" below is such an output conversion.
     */
    buf = vstring_alloc(100);
    argv = argv_split(reply, "\n");
    for (cpp = argv->argv; *cpp; cpp++) {
	vstring_strcat(buf, *cpp);
	if (cpp[1])
	    vstring_strcat(buf, "\r\n");
    }
    argv_free(argv);
    trw_set_ehlo_resp(state->tlsrpt, STR(buf));
    vstring_free(buf);
}

#endif					/* USE_TLSRPT  && USE_TLS */
