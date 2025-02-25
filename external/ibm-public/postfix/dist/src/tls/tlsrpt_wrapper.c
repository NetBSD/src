/*	$NetBSD: tlsrpt_wrapper.c,v 1.2 2025/02/25 19:15:50 christos Exp $	*/

/*++
/* NAME
/*	tlsrpt_wrapper 3
/* SUMMARY
/*	TLSRPT support for the SMTP and TLS protocol engines
/* SYNOPSIS
/*	#include <tlsrpt_wrapper.h>
/*
/*	#ifdef USE_TLS
/*	#ifdef USE_TLSRPT
/*	TLS_RPT *trw_create(
/*	const char *rpt_socket_name,
/*	const char *rpt_policy_domain,
/*	const char *rpt_policy_string,
/*	int	skip_reused_hs)
/*
/*	void	trw_free(
/*	TLSRPT_WRAPPER *trw)
/*
/*	void	trw_set_tls_policy(
/*	TLSRPT_WRAPPER *trw,
/*	tlsrpt_policy_type_t tls_policy_type,
/*	const char *const *tls_policy_strings,
/*	const char *tls_policy_domain
/*	const char *const *mx_host_patterns)
/*
/*	void	trw_set_tcp_connection(
/*	TLSRPT_WRAPPER *trw,
/*	const char *snd_mta_addr,
/*	const char *rcv_mta_name,
/*	const char *rcv_mta_addr)
/*
/*	void	trw_set_ehlo_resp(
/*	TLSRPT_WRAPPER *trw,
/*	const char *rcv_mta_ehlo)
/*
/*	void	trw_report_failure(
/*	TLSRPT_WRAPPER *trw,
/*	tlsrpt_failure_t failure_type,
/*	const char *additional_info,
/*	const char *failure_reason)
/*
/*	void	trw_report_success(
/*	TLSRPT_WRAPPER *trw)
/*
/*	int	trw_is_reported(
/*	TLSRPT_WRAPPER *trw)
/*
/*	int	trw_is_skip_reused_hs(
/*	TLSRPT_WRAPPER *trw)
/*
/*	tlsrpt_policy_type_t convert_tlsrpt_policy_type(
/*	const char *policy_type)
/*
/*	tlsrpt_failure_t convert_tlsrpt_policy_failure(
/*	const char *policy_failure)
/*	#endif /* USE_TLS_RPT */
/*
/*	int	valid_tlsrpt_policy_type(
/*	const char *type_name)
/*
/*	int	valid_tlsrpt_policy_failure(
/*	const char *failure_name)
/*	#endif /* USE_TLS */
/* POSTFIX ARCHITECTURE, BOTTOM-UP VIEW
/* .ad
/* .fi
/*	This module encapsulates TLSRPT support for Postfix's
/*	multi-process and multi-layer architecture. The text that follows
/*	explains the purpose of this software layer.
/*
/*	First, Postfix TLSRPT support uses the TLSRPT client library
/*	from sys4.de. That library makes the reasonable assumption that
/*	all calls concerning one SMTP session will be made from within
/*	one process.
/*
/*	Second, some TLS errors are detected in the SMTP protocol
/*	engine (example: a remote SMTP server does not announce STARTTLS
/*	support), while other TLS errors are detected in the TLS protocol
/*	engine (example: certificate verification error).
/*
/*	Third, the Postfix TLS protocol engine may be located in a
/*	different process than the SMTP protocol engine. And even if the
/*	two are located in the same process, the TLS protocol engine knows
/*	nothing about SMTP. Hence, there needs to be an abstraction that
/*	isolates the TLS protocol engine from the SMTP-specific details
/*	of TLSRPT.
/*
/*	Fourth, Postfix has a pipelined and layered architecture where
/*	each process (or architectural layer) handles a problem as it
/*	runs into it, instead of reporting problem details back to its
/*	pipeline predecessor (or back to a higher architectural layer).
/* TLSRPT_WRAPPER IMPLEMENTATION
/* .ad
/* .fi
/*	At a high level, the SMTP protocol engine encapsulates SMTP
/*	session and TLS policy information in an opaque TLSRPT_WRAPPER
/*	object, and passes that object to the TLS protocol engine. The
/*	TLS protocol engine can invoke TLSRPT_WRAPPER methods to report a
/*	TLS error through the sys4.de TLSRPT client library. In a similar
/*	manner, the SMTP protocol engine can invoke TLSRPT_WRAPPER object
/*	methods to report a TLS error or success.
/*
/*	At a low level, The Postfix SMTP protocol engine (smtp_proto.c)
/*	reports TLS errors when TLS support is required but unavailable,
/*	or requests the Postfix TLS protocol engine to perform a TLS
/*	protocol handshake over an open SMTP connection. The SMTP
/*	protocol engine either calls the TLS protocol engine directly,
/*	or calls it over local IPC in a tlsproxy(8) process.
/*
/*	The TLS protocol engine may report a TLS error by invoking
/*	TLSRPT_WRAPPER methods, and either returns no TLS session object,
/*	or a TLS session object for a completed handshake. The TLS session
/*	object will indicate if the TLS protocol engine reported any
/*	TLS error through TLSRPT (for example an error that resulted in
/*	a successful TLS handshake with a downgraded TLS security level).
/*
/*	The Postfix SMTP protocol engine reports success or failure
/*	by invoking TLSRPT_WRAPPER methods, depending on whether all
/*	matching requirements were satisfied. The SMTP protocol engine
/*	does not report success or failure by invoking TLSRPT_WRAPPER
/*	methods if the TLS protocol engine already reported a failure.
/* TLSRPT_WRAPPER API
/* .ad
/* .fi
/*	The functions below must be called in a specific order. All
/*	string inputs are copied. If a required call is missing then
/*	the request will be ignored, and a warning will be logged,
/*	but this not affect email deliveries.
/* .PP
/*	trw_create() must be called before other trw_xxx() requests can
/*	be made. Arguments:
/* .IP rpt_socket_name
/*	The name of a socket that will be managed by local TLSRPT
/*	infrastructure.
/* .IP rpt_policy_domain
/*	The TLSRPT policy domain name, i.e. the domain that wishes to
/*	receive TLSRPT summary reports. An internationalized domain name
/*	must be in A-label form (i.e. punycode).
/* .IP rpt_policy_string
/*	The TLSRPT policy record content, i.e. how to submit TLSRPT
/*	summary reports.
/* .IP skip_reused_hs
/*	If non-zero, do not report the TLSRPT status for TLS handshakes
/*	that reuse a previously-negotiated TLS session; such sessions
/*	were already reported when they were used for the first time.
/* .PP
/*	trw_free() destroys storage allocated with other trw_xxx()
/*	requests.
/* .PP
/*	trw_set_tls_policy() must be called by the SMTP protocol engine
/*	after it found a DANE, STS, or no policy, and before it tries to
/*	establish a new SMTP connection. This function clears information
/*	that was specified earlier with trw_set_tls_policy() or
/*	trw_set_tcp_connection(), and resets whether trw_report_failure()
/*	or trw_report_success() were called. Mapping from arguments to
/*	TLSRPT report fields:
/* .IP tls_policy_type
/*	policies[].policy.policy-type.
/* .IP tls_policy_strings (may be null)
/*	policies[].policy.policy-string[]. Ignored if the tls_policy_type
/*	value is TLSRPT_NO_POLICY_FOUND.
/* .IP tls_policy_domain (may be null)
/*	policies[].policy.policy-domain.
/* .IP mx_host_patterns (may be null)
/*	policies[].policy.mx-host[]. Ignored if the tls_policy_type
/*	value is TLSRPT_NO_POLICY_FOUND.
/* .PP
/*	trw_set_tcp_connection() and trw_set_ehlo_resp() are optionally
/*	called by the SMTP protcol engine, after it has established
/*	a new SMTP connection, before it requests a TLS protocol
/*	handshake. Mapping from arguments to TLSRPT report fields:
/* .IP snd_mta_addr (may be null)
/*	policies[].failure-details[].sending-mta-ip.
/* .IP rcv_mta_name (may be null)
/*	policies[].failure-details[].receiving-mx-hostname.
/* .IP rcv_mta_addr (may be null)
/*	policies[].failure-details[].receiving-ip.
/* .PP
/*	trw_set_ehlo_resp() is optionally called by the SMTP protcol
/*	engine to pass on the EHLO response. Presumably this is the EHLO
/*	response before STARTTLS (TLSRPT is primarily interested in
/*	pre-handshake and handshake errors).
/* .IP rcv_mta_ehlo (may be null)
/*	policies[].failure-details[].receiving-mx-helo.
/* .PP
/*	trw_report_failure() is called by the TLS protocol engine or
/*	SMTP protocol engine to report a TLS error. The result value
/*	is 0 for success, -1 for failure as indicated with the errno
/*	value. The call is successfully skipped if information is missing
/*	or if failure or success were already reported for the
/*	connection. Mapping from arguments to TLSRPT report fields:
/* .IP failure_type
/*	policies[].failure-details[].result-type.
/* .IP additional_info (may be null)
/*	policies[].failure-details[].additional-information.
/* .IP failure_reason (may be null)
/*	policies[].failure-details[].failure-reason-code
/* .PP
/*	trw_report_success() is called by the SMTP protocol engine
/*	to report a successful TLS handshake. The result value is
/*	0 for success, -1 for failure with errno indicating the
/*	error type. The call is successfully skipped if information if
/*	missing or if failure or success were already reported for
/*	the connection.
/* .PP
/*	trw_is_reported() returns non-zero when the contents of the
/*	specified TLSRPT_WRAPPER have been reported.
/* .PP
/*	trw_is_skip_reused_hs() returns non-zero if the skip_reused_hs
/*	argument of trw_create() was non-zero.
/* .PP
/*	convert_tlsrpt_policy_type() and convert_tlsrpt_policy_failure()
/*	convert a valid policy type or failure name to the corresponding
/*	enum value. The result is < 0 if the name is not valid.
/* .PP
/*	valid_tlsrpt_policy_type() and valid_tlsrpt_policy_failure()
/*	return non-zero if the specified policy type or failure name
/*	is valid in TLSRPT. These functions do not require that the
/*	module is built with TLSRPT support. This allows the names to
/*	be used even if TLSRPT is disabled.
/* DIAGNOSTICS
/*	Some functions will log a warning when information is missing.
/*	Such warnings will not affect the operation of the SMTP or TLS
/*	protocol engine.
/* BUGS
/*	This implementation is suitable to report successful TLS policy
/*	compliance, and to report a failure that prevents TLS policy
/*	compliance (example: all TLSA records are unusable). Do not use
/*	this implementation to report other errors (example: some TLSA
/*	record is non-parsable).
/* SEE ALSO
/*	https://github.com/sys4/tlsrpt, TLSRPT client library
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#if defined(USE_TLS)

 /*
  * System library.
  */
#include <sys_defs.h>
#include <errno.h>
#include <string.h>
#if defined(USE_TLSRPT)
#include <tlsrpt.h>
#endif

#if !defined(TLSRPT_PREREQ)
#define TLSRPT_PREREQ(maj, min) \
    (defined(TLSRPT_VERSION_MAJOR) && \
	((TLSRPT_VERSION_MAJOR << 16) + TLSRPT_VERSION_MINOR >= \
	    ((maj) << 16) + (min)))
#endif

 /*
  * Utility library.
  */
#include <argv.h>
#include <msg.h>
#include <mymalloc.h>
#include <name_code.h>
#include <stringops.h>

 /*
  * Some functions are not #ifdef USE_TLSRPT.
  */
#define TLSRPT_WRAPPER_INTERNAL
#include <tlsrpt_wrapper.h>
#if defined(USE_TLSRPT)

 /*
  * Macros to make repetitive code more readable.
  */
#define MYFREE_IF_SET(member) do { \
	if (member) \
	    myfree(member); \
    } while (0)

#define MYFREE_IF_SET_AND_CLEAR(member, value) do { \
	if (member) { \
	    myfree(member); \
	    (member) = 0; \
	} \
    } while (0)

#define MYFREE_IF_SET_AND_COPY(member, value) do { \
	MYFREE_IF_SET(member); \
	(member) = (value) ? mystrdup(value) : 0; \
    } while (0)

#define ARGV_FREE_IF_SET(member) do { \
	if (member) \
	    argv_free(member); \
    } while (0)

#define ARGV_FREE_IF_SET_AND_CLEAR(member) do { \
	if (member) { \
	    argv_free(member); \
	    (member) = 0; \
	} \
    } while (0)

#define ARGV_FREE_IF_SET_AND_COPY(member, value) do { \
	ARGV_FREE_IF_SET(member); \
	(member) = (value) ? argv_addv((ARGV *) 0, value) : 0; \
    } while (0)

/* trw_create - create initial TLSRPT_WRAPPER instance */

TLSRPT_WRAPPER *trw_create(const char *rpt_socket_name,
			           const char *rpt_policy_domain,
			           const char *rpt_policy_string,
			           int skip_reused_hs)
{
    const char myname[] = "trw_create";
    TLSRPT_WRAPPER *trw;

    if (msg_verbose > 1)
	msg_info("%s(rpt_socket_name=%s, rpt_policy_domain=%s, "
		 "rpt_policy_string=%s, skip_reused_hs=%d)",
		 myname, rpt_socket_name, rpt_policy_domain,
		 rpt_policy_string, skip_reused_hs);

#if TLSRPT_PREREQ(0, 6)
    if (tlsrpt_version_check(TLSRPT_VERSION_MAJOR, TLSRPT_VERSION_MINOR,
			     TLSRPT_VERSION_PATCH) == 0)
	msg_warn("run-time library vs. compile-time header version mismatch: "
		 "libtlsrpt API version '%s' is not compatible with "
		 "libtlsrpt API version '%s' ", tlsrpt_version(),
		 TLSRPT_VERSION_STRING);
#endif

    /*
     * memset() is not portable for pointer etc. types.
     */
    trw = (TLSRPT_WRAPPER *) mymalloc(sizeof(*trw));
    trw->rpt_socket_name = mystrdup(rpt_socket_name);
    trw->rpt_policy_domain = mystrdup(rpt_policy_domain);
    trw->rpt_policy_string = mystrdup(rpt_policy_string);;
    trw->tls_policy_type = 0;
    trw->tls_policy_strings = 0;
    trw->tls_policy_domain = 0;
    trw->mx_host_patterns = 0;
    trw->snd_mta_addr = 0;
    trw->rcv_mta_name = 0;
    trw->rcv_mta_addr = 0;
    trw->rcv_mta_ehlo = 0;
    trw->skip_reused_hs = skip_reused_hs;
    trw->flags = 0;
    return (trw);
}

/* trw_free - destroy TLSRPT_WRAPPER instance. */

void    trw_free(TLSRPT_WRAPPER *trw)
{
    if (msg_verbose > 1)
	msg_info("trw_free: rpt_socket_name=%s, rpt_policy_domain=%s, ...",
		 trw->rpt_socket_name, trw->rpt_policy_domain);

    /* Destroy fields set with trw_create(). */
    myfree(trw->rpt_socket_name);
    myfree(trw->rpt_policy_domain);
    myfree(trw->rpt_policy_string);
    /* Destroy fields set with trw_set_tls_policy(). */
    ARGV_FREE_IF_SET(trw->tls_policy_strings);
    MYFREE_IF_SET(trw->tls_policy_domain);
    ARGV_FREE_IF_SET(trw->mx_host_patterns);
    /* Destroy fields set with trw_set_tcp_connection(). */
    trw_set_tcp_connection(trw, (char *) 0, (char *) 0, (char *) 0);
    /* Destroy fields set with trw_set_ehlo_resp(). */
    trw_set_ehlo_resp(trw, (char *) 0);
    /* That's all. */
    myfree((void *) trw);
}

/* trw_set_tls_policy - set TLS policy info, clear SMTP info */

void    trw_set_tls_policy(TLSRPT_WRAPPER *trw,
			           tlsrpt_policy_type_t tls_policy_type,
			           const char *const * tls_policy_strings,
			           const char *tls_policy_domain,
			           const char *const * mx_host_patterns)
{
    const char myname[] = "trw_set_tls_policy";

#define STR_OR_NULL(s)	((s) ? (s) : "(Null)")
#define PSTR_OR_NULL(p)	((p) ? STR_OR_NULL(*p) : "(Null)")

    if (msg_verbose > 1)
	msg_info("%s(tlsrpt_policy_type_t=%d, tls_policy_strings=%s..., "
		 "tls_policy_domain=%s, mx_host_patterns=%s...)",
		 myname, tls_policy_type,
		 PSTR_OR_NULL(tls_policy_strings),
		 STR_OR_NULL(tls_policy_domain),
		 PSTR_OR_NULL(mx_host_patterns));

    trw->tls_policy_type = tls_policy_type;
    MYFREE_IF_SET_AND_COPY(trw->tls_policy_domain, tls_policy_domain);
    if (tls_policy_type == TLSRPT_NO_POLICY_FOUND) {
	ARGV_FREE_IF_SET_AND_CLEAR(trw->tls_policy_strings);
	ARGV_FREE_IF_SET_AND_CLEAR(trw->tls_policy_strings);
    } else {
	ARGV_FREE_IF_SET_AND_COPY(trw->tls_policy_strings, tls_policy_strings);
	ARGV_FREE_IF_SET_AND_COPY(trw->mx_host_patterns, mx_host_patterns);
    }
    trw->flags = TRW_FLAG_HAVE_TLS_POLICY;
    trw_set_tcp_connection(trw, (char *) 0, (char *) 0, (char *) 0);
    trw_set_ehlo_resp(trw, (char *) 0);
}

/* trw_set_tcp_connection - set SMTP endpoint info */

void    trw_set_tcp_connection(TLSRPT_WRAPPER *trw,
			               const char *snd_mta_addr,
			               const char *rcv_mta_name,
			               const char *rcv_mta_addr)
{
    const char myname[] = "trw_set_tcp_connection";

    if (msg_verbose > 1 && (snd_mta_addr || rcv_mta_name || rcv_mta_addr))
	msg_info("%s(snd_mta_addr=%s, rcv_mta_name=%s, rcv_mta_addr=%s)",
		 myname, STR_OR_NULL(snd_mta_addr),
		 STR_OR_NULL(rcv_mta_name), STR_OR_NULL(rcv_mta_addr));

    /*
     * Sanity check: usage errors are not a show stopper.
     */
    if ((snd_mta_addr || rcv_mta_name || rcv_mta_addr)
	&& ((trw->flags & TRW_FLAG_HAVE_TLS_POLICY) == 0
	    || (trw->flags & TRW_FLAG_REPORTED))) {
	msg_warn("%s: missing trw_set_tls_policy call", myname);
	return;
    }
    MYFREE_IF_SET_AND_COPY(trw->snd_mta_addr, snd_mta_addr);
    MYFREE_IF_SET_AND_COPY(trw->rcv_mta_name, rcv_mta_name);
    MYFREE_IF_SET_AND_COPY(trw->rcv_mta_addr, rcv_mta_addr);
}

/* trw_set_ehlo_resp - set EHLO response */

void    trw_set_ehlo_resp(TLSRPT_WRAPPER *trw, const char *rcv_mta_ehlo)
{
    const char myname[] = "trw_set_ehlo_resp";

    if (msg_verbose > 1 && rcv_mta_ehlo)
	msg_info("%s(rcv_mta_ehlo=%s)", myname, rcv_mta_ehlo);

    /*
     * Sanity check: usage errors are not a show stopper.
     */
    if (rcv_mta_ehlo && ((trw->flags & TRW_FLAG_HAVE_TLS_POLICY) == 0
			 || (trw->flags & TRW_FLAG_REPORTED))) {
	msg_warn("%s: missing trw_set_tls_policy call", myname);
	return;
    }
    MYFREE_IF_SET_AND_COPY(trw->rcv_mta_ehlo, rcv_mta_ehlo);
}

/* trw_munge_report_result - helper to map and log libtlsrpt result value */

static int trw_munge_report_result(int libtlsrpt_errorcode)
{
    int     err;

    /*
     * First, deal with the non-error cases.
     */
    if (libtlsrpt_errorcode == 0) {
	return (0);
    }

    /*
     * Report a tlsrpt library internal error.
     */
    else if (tlsrpt_error_code_is_internal(libtlsrpt_errorcode)) {
	msg_warn("Could not report TLS handshake result to tlsrpt library:"
		 " %s (error %d)", tlsrpt_strerror(libtlsrpt_errorcode),
		 libtlsrpt_errorcode);
	return (-1);
    }

    /*
     * Report a libc error. Do not report success if errno was zero. When
     * debug logging is enabled, also log some library-internal info.
     */
    else {
	err = tlsrpt_errno_from_error_code(libtlsrpt_errorcode);
	msg_warn("Could not report TLS handshake result to tlsrpt library:"
		 " %s (errno %d)", mystrerror(err), err);
	if (msg_verbose)
	    msg_warn("Error location in tlsrpt library: %s (error %d)",
		     tlsrpt_strerror(libtlsrpt_errorcode),
		     libtlsrpt_errorcode);
	errno = err;
	return (-1);
    }
}

/* trw_tlsrpt_failure_to_string - make debug logging readable */

static const char *trw_failure_type_to_string(tlsrpt_failure_t failure_type)
{
    static const NAME_CODE failure_types[] = {
	"starttls_not_supported", TLSRPT_STARTTLS_NOT_SUPPORTED,
	"certificate_host_mismatch", TLSRPT_CERTIFICATE_HOST_MISMATCH,
	"certificate_not_trusted", TLSRPT_CERTIFICATE_NOT_TRUSTED,
	"certificate_expired", TLSRPT_CERTIFICATE_EXPIRED,
	"validation_failure", TLSRPT_VALIDATION_FAILURE,
	"sts_policy_fetch_error", TLSRPT_STS_POLICY_FETCH_ERROR,
	"sts_policy_invalid", TLSRPT_STS_POLICY_INVALID,
	"sts_webpki_invalid", TLSRPT_STS_WEBPKI_INVALID,
	"tlsa_invalid", TLSRPT_TLSA_INVALID,
	"dnssec_invalid", TLSRPT_DNSSEC_INVALID,
	"dane_required", TLSRPT_DANE_REQUIRED,
	"unfinished_policY", TLSRPT_UNFINISHED_POLICY,
	0, -1
    };
    const char *cp;
    static VSTRING *buf;

    if ((cp = str_name_code(failure_types, failure_type)) == 0) {
	if (buf == 0)
	    buf = vstring_alloc(20);
	msg_warn("unknown tlsrpt_failure_t value %d", failure_type);
	vstring_sprintf(buf, "failure_type_%d", failure_type);
	cp = vstring_str(buf);
    }
    return (cp);
}

/* trw_report_failure - one-shot failure reporter */

int     trw_report_failure(TLSRPT_WRAPPER *trw,
			           tlsrpt_failure_t failure_type,
			           const char *additional_info,
			           const char *failure_reason)
{
    const char myname[] = "trw_report_failure";
    struct tlsrpt_connection_t *con;
    int     res;

    if (msg_verbose > 1)
	msg_info("%s(failure_type=%d, additional_info=%s, failure_reason=%s)",
		 myname, failure_type, STR_OR_NULL(additional_info),
		 STR_OR_NULL(failure_reason));

    /*
     * Sanity check: usage errors are not a show stopper.
     */
    if ((trw->flags & TRW_FLAG_HAVE_TLS_POLICY) == 0) {
	msg_warn("%s: missing trw_set_tls_policy call", myname);
	return (0);
    }

    /*
     * Report a failure only when it is seen first. If a failure was already
     * reported by a lower-level function close to the root cause, then skip
     * the less detailed failure report from a later caller who is further
     * away from the point where trouble was found.
     * 
     * TODO(wietse) Is it worthwhile to distinguish between failure versus
     * success already reported?
     */
    if (trw->flags & TRW_FLAG_REPORTED) {
	if (msg_verbose)
	    msg_info("%s: success or failure already reported", myname);
	return (0);
    }
    trw->flags |= TRW_FLAG_REPORTED;

    /* Give the local admin a clue. */
    msg_info("TLSRPT: status=failure, domain=%s, receiving_mx=%s[%s],"
	     " failure_type=%s%s%s",
	     trw->rpt_policy_domain, trw->rcv_mta_name, trw->rcv_mta_addr,
	     trw_failure_type_to_string(failure_type),
	     failure_reason ? ", failure_reason=" : "",
	     failure_reason ? failure_reason : "");

    if ((res = tlsrpt_open(&con, trw->rpt_socket_name)) == 0) {
	struct tlsrpt_dr_t *dr;
	char  **cpp;

	if ((res = tlsrpt_init_delivery_request(&dr, con,
						trw->rpt_policy_domain,
					    trw->rpt_policy_string)) == 0) {
	    if ((res = tlsrpt_init_policy(dr, trw->tls_policy_type,
					  trw->tls_policy_domain)) == 0) {
		if (trw->tls_policy_strings)
		    for (cpp = trw->tls_policy_strings->argv;
			 res == 0 && *cpp; cpp++)
			res = tlsrpt_add_policy_string(dr, *cpp);
		if (trw->mx_host_patterns)
		    for (cpp = trw->mx_host_patterns->argv;
			 res == 0 && *cpp; cpp++)
			res = tlsrpt_add_mx_host_pattern(dr, *cpp);
		if (res == 0)
		    res = tlsrpt_add_delivery_request_failure(dr,
					   /* failure_code= */ failure_type,
				    /* sending_mta_ip= */ trw->snd_mta_addr,
			     /* receiving_mx_hostname= */ trw->rcv_mta_name,
				 /* receiving_mx_helo= */ trw->rcv_mta_ehlo,
				      /* receiving_ip= */ trw->rcv_mta_addr,
			      /* additional_information= */ additional_info,
				 /* failure_reason_code= */ failure_reason);
		if (res == 0)
		    res = tlsrpt_finish_policy(dr, TLSRPT_FINAL_FAILURE);
	    }
	    if (res == 0) {
		res = tlsrpt_finish_delivery_request(&dr);
	    } else {
		(void) tlsrpt_cancel_delivery_request(&dr);
	    }
	}
	(void) tlsrpt_close(&con);
    }
    return (trw_munge_report_result(res));
}

/* trw_report_success - one-shot success reporter */

int     trw_report_success(TLSRPT_WRAPPER *trw)
{
    const char myname[] = "trw_report_success";
    struct tlsrpt_connection_t *con;
    int     res;

    if (msg_verbose > 1)
	msg_info("trw_report_success");

    /*
     * Sanity check: usage errors are not a show stopper.
     */
    if ((trw->flags & TRW_FLAG_HAVE_TLS_POLICY) == 0) {
	msg_warn("%s: missing trw_set_tls_policy call", myname);
	return (0);
    }
    /* This should not happen. Log a warning. */
    if (trw->flags & TRW_FLAG_REPORTED) {
	msg_warn("%s: success or failure was already reported", myname);
	return (0);
    }
    trw->flags |= TRW_FLAG_REPORTED;

    /* Give the local admin a clue. */
    msg_info("TLSRPT: status=success, domain=%s, receiving_mx=%s[%s]",
	     trw->rpt_policy_domain, trw->rcv_mta_name, trw->rcv_mta_addr);

    if ((res = tlsrpt_open(&con, trw->rpt_socket_name)) == 0) {
	struct tlsrpt_dr_t *dr;

	if ((res = tlsrpt_init_delivery_request(&dr, con,
						trw->rpt_policy_domain,
					    trw->rpt_policy_string)) == 0) {
	    if ((res = tlsrpt_init_policy(dr, trw->tls_policy_type,
					  trw->tls_policy_domain)) == 0) {
		char  **cpp;

		if (trw->tls_policy_strings)
		    for (cpp = trw->tls_policy_strings->argv;
			 res == 0 && *cpp; cpp++)
			res = tlsrpt_add_policy_string(dr, *cpp);
		if (trw->mx_host_patterns)
		    for (cpp = trw->mx_host_patterns->argv;
			 res == 0 && *cpp; cpp++)
			res = tlsrpt_add_mx_host_pattern(dr, *cpp);
		if (res == 0)
		    res = tlsrpt_finish_policy(dr, TLSRPT_FINAL_SUCCESS);
	    }
	    if (res == 0) {
		res = tlsrpt_finish_delivery_request(&dr);
	    } else {
		(void) tlsrpt_cancel_delivery_request(&dr);
	    }
	}
	(void) tlsrpt_close(&con);
    }
    return (trw_munge_report_result(res));
}

/* trw_is_reported - trw_report_success() or trw_report_failure() called */

int     trw_is_reported(const TLSRPT_WRAPPER *trw)
{
    return (trw->flags & TRW_FLAG_REPORTED);
}

/* trw_is_skip_reused_hs - don't report TLS handshakes that reuse a session */

int     trw_is_skip_reused_hs(const TLSRPT_WRAPPER *trw)
{
    return (trw->skip_reused_hs);
}

#endif					/* USE_TLS_RPT */

 /*
  * Dummy definitions for builds without the TLSRPT library, so that we can
  * still validate names.
  */
#if !defined(USE_TLSRPT)
#define TLSRPT_POLICY_DANE	0
#define TLSRPT_POLICY_STS	0
#define TLSRPT_NO_POLICY_FOUND  0

#define TLSRPT_VALIDATION_FAILURE       0
#define TLSRPT_STS_POLICY_FETCH_ERROR   0
#define TLSRPT_STS_POLICY_INVALID       0
#define TLSRPT_STS_WEBPKI_INVALID       0
#endif

 /*
  * Mapping from RFC 8460 string to libtlsrpt enum for policy types and
  * policy failures. The mapping assumes that all enum values are
  * non-negative.
  */
const NAME_CODE tlsrpt_policy_type_mapping[] = {
    "sts", TLSRPT_POLICY_STS,
    "no-policy-found", TLSRPT_NO_POLICY_FOUND,
    0, -1,
};

const NAME_CODE tlsrpt_policy_failure_mapping[] = {
    "sts-policy-fetch-error", TLSRPT_STS_POLICY_FETCH_ERROR,
    "sts-policy-invalid", TLSRPT_STS_POLICY_INVALID,
    "sts-webpki-invalid", TLSRPT_STS_WEBPKI_INVALID,
    "validation-failure", TLSRPT_VALIDATION_FAILURE,
    0, -1,
};

/* valid_tlsrpt_policy_type - validate policy_type attribute value */

int     valid_tlsrpt_policy_type(const char *policy_type)
{
    return (name_code(tlsrpt_policy_type_mapping, NAME_CODE_FLAG_NONE,
		      policy_type) >= 0);
}

/* valid_tlsrpt_policy_failure - validate policy_failure attribute value */

int     valid_tlsrpt_policy_failure(const char *policy_failure)
{
    return (name_code(tlsrpt_policy_failure_mapping, NAME_CODE_FLAG_NONE,
		      policy_failure) >= 0);
}

#if defined(USE_TLSRPT)

/* convert_tlsrpt_policy_type - convert string to enum */

tlsrpt_policy_type_t convert_tlsrpt_policy_type(const char *policy_type)
{
    return (name_code(tlsrpt_policy_type_mapping, NAME_CODE_FLAG_NONE,
		      policy_type));
}

/* convert_tlsrpt_policy_failure - convert string to enum */

tlsrpt_failure_t convert_tlsrpt_policy_failure(const char *policy_failure)
{
    return (name_code(tlsrpt_policy_failure_mapping, NAME_CODE_FLAG_NONE,
		      policy_failure));
}

#endif					/* USE_TLSRPT */

#endif					/* USE_TLS */
