/*	$NetBSD: tls_client.c,v 1.12.2.1 2023/12/25 12:43:35 martin Exp $	*/

/*++
/* NAME
/*	tls_client
/* SUMMARY
/*	client-side TLS engine
/* SYNOPSIS
/*	#include <tls.h>
/*
/*	TLS_APPL_STATE *tls_client_init(init_props)
/*	const TLS_CLIENT_INIT_PROPS *init_props;
/*
/*	TLS_SESS_STATE *tls_client_start(start_props)
/*	const TLS_CLIENT_START_PROPS *start_props;
/*
/*	TLS_SESS_STATE *tls_client_post_connect(TLScontext, start_props)
/*	TLS_SESS_STATE *TLScontext;
/*	const TLS_CLIENT_START_PROPS *start_props;
/*
/*	void	tls_client_stop(app_ctx, stream, failure, TLScontext)
/*	TLS_APPL_STATE *app_ctx;
/*	VSTREAM	*stream;
/*	int	failure;
/*	TLS_SESS_STATE *TLScontext;
/* DESCRIPTION
/*	This module is the interface between Postfix TLS clients,
/*	the OpenSSL library and the TLS entropy and cache manager.
/*
/*	The SMTP client will attempt to verify the server hostname
/*	against the names listed in the server certificate. When
/*	a hostname match is required, the verification fails
/*	on certificate verification or hostname mis-match errors.
/*	When no hostname match is required, hostname verification
/*	failures are logged but they do not affect the TLS handshake
/*	or the SMTP session.
/*
/*	The rules for peer name wild-card matching differ between
/*	RFC 2818 (HTTP over TLS) and RFC 2830 (LDAP over TLS), while
/*	RFC RFC3207 (SMTP over TLS) does not specify a rule at all.
/*	Postfix uses a restrictive match algorithm. One asterisk
/*	('*') is allowed as the left-most component of a wild-card
/*	certificate name; it matches the left-most component of
/*	the peer hostname.
/*
/*	Another area where RFCs aren't always explicit is the
/*	handling of dNSNames in peer certificates. RFC 3207 (SMTP
/*	over TLS) does not mention dNSNames. Postfix follows the
/*	strict rules in RFC 2818 (HTTP over TLS), section 3.1: The
/*	Subject Alternative Name/dNSName has precedence over
/*	CommonName.  If at least one dNSName is provided, Postfix
/*	verifies those against the peer hostname and ignores the
/*	CommonName, otherwise Postfix verifies the CommonName
/*	against the peer hostname.
/*
/*	tls_client_init() is called once when the SMTP client
/*	initializes.
/*	Certificate details are also decided during this phase,
/*	so peer-specific certificate selection is not possible.
/*
/*	tls_client_start() activates the TLS session over an established
/*	stream. We expect that network buffers are flushed and
/*	the TLS handshake can begin immediately.
/*
/*	tls_client_stop() sends the "close notify" alert via
/*	SSL_shutdown() to the peer and resets all connection specific
/*	TLS data. As RFC2487 does not specify a separate shutdown, it
/*	is assumed that the underlying TCP connection is shut down
/*	immediately afterwards. Any further writes to the channel will
/*	be discarded, and any further reads will report end-of-file.
/*	If the failure flag is set, no SSL_shutdown() handshake is performed.
/*
/*	Once the TLS connection is initiated, information about the TLS
/*	state is available via the TLScontext structure:
/* .IP TLScontext->protocol
/*	the protocol name (SSLv2, SSLv3, TLSv1),
/* .IP TLScontext->cipher_name
/*	the cipher name (e.g. RC4/MD5),
/* .IP TLScontext->cipher_usebits
/*	the number of bits actually used (e.g. 40),
/* .IP TLScontext->cipher_algbits
/*	the number of bits the algorithm is based on (e.g. 128).
/* .PP
/*	The last two values may differ from each other when export-strength
/*	encryption is used.
/*
/*	If the peer offered a certificate, part of the certificate data are
/*	available as:
/* .IP TLScontext->peer_status
/*	A bitmask field that records the status of the peer certificate
/*	verification. This consists of one or more of TLS_CERT_FLAG_PRESENT,
/*	TLS_CERT_FLAG_TRUSTED, TLS_CERT_FLAG_MATCHED and TLS_CERT_FLAG_SECURED.
/* .IP TLScontext->peer_CN
/*	Extracted CommonName of the peer, or zero-length string if the
/*	information could not be extracted.
/* .IP TLScontext->issuer_CN
/*	Extracted CommonName of the issuer, or zero-length string if the
/*	information could not be extracted.
/* .IP TLScontext->peer_cert_fprint
/*	At the fingerprint security level, if the peer presented a certificate
/*	the fingerprint of the certificate.
/* .PP
/*	If no peer certificate is presented the peer_status is set to 0.
/* EVENT_DRIVEN APPLICATIONS
/* .ad
/* .fi
/*	Event-driven programs manage multiple I/O channels.  Such
/*	programs cannot use the synchronous VSTREAM-over-TLS
/*	implementation that the TLS library historically provides,
/*	including tls_client_stop() and the underlying tls_stream(3)
/*	and tls_bio_ops(3) routines.
/*
/*	With the current TLS library implementation, this means
/*	that an event-driven application is responsible for calling
/*	and retrying SSL_connect(), SSL_read(), SSL_write() and
/*	SSL_shutdown().
/*
/*	To maintain control over TLS I/O, an event-driven client
/*	invokes tls_client_start() with a null VSTREAM argument and
/*	with an fd argument that specifies the I/O file descriptor.
/*	Then, tls_client_start() performs all the necessary
/*	preparations before the TLS handshake and returns a partially
/*	populated TLS context. The event-driven application is then
/*	responsible for invoking SSL_connect(), and if successful,
/*	for invoking tls_client_post_connect() to finish the work
/*	that was started by tls_client_start(). In case of unrecoverable
/*	failure, tls_client_post_connect() destroys the TLS context
/*	and returns a null pointer value.
/* LICENSE
/* .ad
/* .fi
/*	This software is free. You can do with it whatever you want.
/*	The original author kindly requests that you acknowledge
/*	the use of his software.
/* AUTHOR(S)
/*	Originally written by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*
/*	Updated by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

/* System library. */

#include <sys_defs.h>

#ifdef USE_TLS
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <argv.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <stringops.h>
#include <msg.h>
#include <iostuff.h>			/* non-blocking */
#include <midna_domain.h>

/* Global library. */

#include <mail_params.h>

/* TLS library. */

#include <tls_mgr.h>
#define TLS_INTERNAL
#include <tls.h>

/* Application-specific. */

#define STR	vstring_str
#define LEN	VSTRING_LEN

/* load_clnt_session - load session from client cache (non-callback) */

static SSL_SESSION *load_clnt_session(TLS_SESS_STATE *TLScontext)
{
    const char *myname = "load_clnt_session";
    SSL_SESSION *session = 0;
    VSTRING *session_data = vstring_alloc(2048);

    /*
     * Prepare the query.
     */
    if (TLScontext->log_mask & TLS_LOG_CACHE)
	/* serverid contains transport:addr:port information */
	msg_info("looking for session %s in %s cache",
		 TLScontext->serverid, TLScontext->cache_type);

    /*
     * We only get here if the cache_type is not empty. This code is not
     * called unless caching is enabled and the cache_type is stored in the
     * server SSL context.
     */
    if (TLScontext->cache_type == 0)
	msg_panic("%s: null client session cache type in session lookup",
		  myname);

    /*
     * Look up and activate the SSL_SESSION object. Errors are non-fatal,
     * since caching is only an optimization.
     */
    if (tls_mgr_lookup(TLScontext->cache_type, TLScontext->serverid,
		       session_data) == TLS_MGR_STAT_OK) {
	session = tls_session_activate(STR(session_data), LEN(session_data));
	if (session) {
	    if (TLScontext->log_mask & TLS_LOG_CACHE)
		/* serverid contains transport:addr:port information */
		msg_info("reloaded session %s from %s cache",
			 TLScontext->serverid, TLScontext->cache_type);
	}
    }

    /*
     * Clean up.
     */
    vstring_free(session_data);

    return (session);
}

/* new_client_session_cb - name new session and save it to client cache */

static int new_client_session_cb(SSL *ssl, SSL_SESSION *session)
{
    const char *myname = "new_client_session_cb";
    TLS_SESS_STATE *TLScontext;
    VSTRING *session_data;

    /*
     * The cache name (if caching is enabled in tlsmgr(8)) and the cache ID
     * string for this session are stored in the TLScontext. It cannot be
     * null at this point.
     */
    if ((TLScontext = SSL_get_ex_data(ssl, TLScontext_index)) == 0)
	msg_panic("%s: null TLScontext in new session callback", myname);

    /*
     * We only get here if the cache_type is not empty. This callback is not
     * set unless caching is enabled and the cache_type is stored in the
     * server SSL context.
     */
    if (TLScontext->cache_type == 0)
	msg_panic("%s: null session cache type in new session callback",
		  myname);

    if (TLScontext->log_mask & TLS_LOG_CACHE)
	/* serverid contains transport:addr:port information */
	msg_info("save session %s to %s cache",
		 TLScontext->serverid, TLScontext->cache_type);

    /*
     * Passivate and save the session object. Errors are non-fatal, since
     * caching is only an optimization.
     */
    if ((session_data = tls_session_passivate(session)) != 0) {
	tls_mgr_update(TLScontext->cache_type, TLScontext->serverid,
		       STR(session_data), LEN(session_data));
	vstring_free(session_data);
    }

    /*
     * Clean up.
     */
    SSL_SESSION_free(session);			/* 200502 */

    return (1);
}

/* uncache_session - remove session from the external cache */

static void uncache_session(SSL_CTX *ctx, TLS_SESS_STATE *TLScontext)
{
    SSL_SESSION *session = SSL_get_session(TLScontext->con);

    SSL_CTX_remove_session(ctx, session);
    if (TLScontext->cache_type == 0 || TLScontext->serverid == 0)
	return;

    if (TLScontext->log_mask & TLS_LOG_CACHE)
	/* serverid contains transport:addr:port information */
	msg_info("remove session %s from client cache", TLScontext->serverid);

    tls_mgr_delete(TLScontext->cache_type, TLScontext->serverid);
}

/* verify_extract_name - verify peer name and extract peer information */

static void verify_extract_name(TLS_SESS_STATE *TLScontext, X509 *peercert,
				        const TLS_CLIENT_START_PROPS *props)
{
    int     verbose;

    verbose = TLScontext->log_mask &
	(TLS_LOG_CERTMATCH | TLS_LOG_VERBOSE | TLS_LOG_PEERCERT);

    /*
     * On exit both peer_CN and issuer_CN should be set.
     */
    TLScontext->issuer_CN = tls_issuer_CN(peercert, TLScontext);
    TLScontext->peer_CN = tls_peer_CN(peercert, TLScontext);

    /*
     * Is the certificate trust chain trusted and matched?  Any required name
     * checks are now performed internally in OpenSSL.
     */
    if (SSL_get_verify_result(TLScontext->con) == X509_V_OK) {
	TLScontext->peer_status |= TLS_CERT_FLAG_TRUSTED;
	if (TLScontext->must_fail) {
	    msg_panic("%s: cert valid despite trust init failure",
		      TLScontext->namaddr);
	} else if (TLS_MUST_MATCH(TLScontext->level)) {

	    /*
	     * Fully secured only if not insecure like half-dane.  We use
	     * TLS_CERT_FLAG_MATCHED to satisfy policy, but
	     * TLS_CERT_FLAG_SECURED to log the effective security.
	     * 
	     * Would ideally also exclude "verify" (as opposed to "secure")
	     * here, because that can be subject to insecure MX indirection,
	     * but that's rather incompatible (and not even the case with
	     * explicitly chosen non-default match patterns).  Users have
	     * been warned.
	     */
	    if (!TLS_NEVER_SECURED(TLScontext->level))
		TLScontext->peer_status |= TLS_CERT_FLAG_SECURED;
	    TLScontext->peer_status |= TLS_CERT_FLAG_MATCHED;

	    if (verbose) {
		const char *peername = SSL_get0_peername(TLScontext->con);

		if (peername)
		    msg_info("%s: matched peername: %s",
			     TLScontext->namaddr, peername);
		tls_dane_log(TLScontext);
	    }
	}
    }

    /*
     * Give them a clue. Problems with trust chain verification are logged
     * when the session is first negotiated, before the session is stored
     * into the cache. We don't want mystery failures, so log the fact the
     * real problem is to be found in the past.
     */
    if (!TLS_CERT_IS_MATCHED(TLScontext)
	&& (TLScontext->log_mask & TLS_LOG_UNTRUSTED)) {
	if (TLScontext->session_reused == 0)
	    tls_log_verify_error(TLScontext);
	else
	    msg_info("%s: re-using session with untrusted certificate, "
		     "look for details earlier in the log", props->namaddr);
    }
}

/* add_namechecks - tell OpenSSL what names to check */

static void add_namechecks(TLS_SESS_STATE *TLScontext,
			           const TLS_CLIENT_START_PROPS *props)
{
    SSL    *ssl = TLScontext->con;
    int     namechecks_count = 0;
    int     i;

    /* RFC6125: No part-label 'foo*bar.example.com' wildcards for SMTP */
    SSL_set_hostflags(ssl, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);

    for (i = 0; i < props->matchargv->argc; ++i) {
	const char *name = props->matchargv->argv[i];
	const char *aname;
	int     match_subdomain = 0;

	if (strcasecmp(name, "nexthop") == 0) {
	    name = props->nexthop;
	} else if (strcasecmp(name, "dot-nexthop") == 0) {
	    name = props->nexthop;
	    match_subdomain = 1;
	} else if (strcasecmp(name, "hostname") == 0) {
	    name = props->host;
	} else {
	    if (*name == '.') {
		if (*++name == 0) {
		    msg_warn("%s: ignoring invalid match name: \".\"",
			     TLScontext->namaddr);
		    continue;
		}
		match_subdomain = 1;
	    }
#ifndef NO_EAI
	    else {

		/*
		 * Besides U+002E (full stop) IDNA2003 allows labels to be
		 * separated by any of the Unicode variants U+3002
		 * (ideographic full stop), U+FF0E (fullwidth full stop), and
		 * U+FF61 (halfwidth ideographic full stop). Their respective
		 * UTF-8 encodings are: E38082, EFBC8E and EFBDA1.
		 * 
		 * IDNA2008 does not permit (upper) case and other variant
		 * differences in U-labels. The midna_domain_to_ascii()
		 * function, based on UTS46, normalizes such differences
		 * away.
		 * 
		 * The IDNA to_ASCII conversion does not allow empty leading
		 * labels, so we handle these explicitly here.
		 */
		unsigned char *cp = (unsigned char *) name;

		if ((cp[0] == 0xe3 && cp[1] == 0x80 && cp[2] == 0x82)
		    || (cp[0] == 0xef && cp[1] == 0xbc && cp[2] == 0x8e)
		    || (cp[0] == 0xef && cp[1] == 0xbd && cp[2] == 0xa1)) {
		    if (name[3]) {
			name = name + 3;
			match_subdomain = 1;
		    }
		}
	    }
#endif
	}

	/*
	 * DNS subjectAltNames are required to be ASCII.
	 * 
	 * Per RFC 6125 Section 6.4.4 Matching the CN-ID, follows the same rules
	 * (6.4.1, 6.4.2 and 6.4.3) that apply to subjectAltNames.  In
	 * particular, 6.4.2 says that the reference identifier is coerced to
	 * ASCII, but no conversion is stated or implied for the CN-ID, so it
	 * seems it only matches if it is all ASCII.  Otherwise, it is some
	 * other sort of name.
	 */
#ifndef NO_EAI
	if (!allascii(name) && (aname = midna_domain_to_ascii(name)) != 0) {
	    if (msg_verbose)
		msg_info("%s asciified to %s", name, aname);
	    name = aname;
	}
#endif

	if (!match_subdomain) {
	    if (SSL_add1_host(ssl, name))
		++namechecks_count;
	    else
		msg_warn("%s: error loading match name: \"%s\"",
			 TLScontext->namaddr, name);
	} else {
	    char   *dot_name = concatenate(".", name, (char *) 0);

	    if (SSL_add1_host(ssl, dot_name))
		++namechecks_count;
	    else
		msg_warn("%s: error loading match name: \"%s\"",
			 TLScontext->namaddr, dot_name);
	    myfree(dot_name);
	}
    }

    /*
     * If we failed to add any names, OpenSSL will perform no namechecks, so
     * we set the "must_fail" bit to avoid verification false-positives.
     */
    if (namechecks_count == 0) {
	msg_warn("%s: could not configure peer name checks",
		 TLScontext->namaddr);
	TLScontext->must_fail = 1;
    }
}

/* tls_auth_enable - set up TLS authentication */

static int tls_auth_enable(TLS_SESS_STATE *TLScontext,
			           const TLS_CLIENT_START_PROPS *props)
{
    const char *sni = 0;

    if (props->sni && *props->sni) {
#ifndef NO_EAI
	const char *aname;

#endif

	/*
	 * MTA-STS policy plugin compatibility: with servername=hostname,
	 * Postfix must send the MX hostname (not CNAME expanded).
	 */
	if (strcmp(props->sni, "hostname") == 0)
	    sni = props->host;
	else if (strcmp(props->sni, "nexthop") == 0)
	    sni = props->nexthop;
	else
	    sni = props->sni;

	/*
	 * The SSL_set_tlsext_host_name() documentation does not promise that
	 * every implementation will convert U-label form to A-label form.
	 */
#ifndef NO_EAI
	if (!allascii(sni) && (aname = midna_domain_to_ascii(sni)) != 0) {
	    if (msg_verbose)
		msg_info("%s asciified to %s", sni, aname);
	    sni = aname;
	}
#endif
    }
    switch (TLScontext->level) {
    case TLS_LEV_HALF_DANE:
    case TLS_LEV_DANE:
    case TLS_LEV_DANE_ONLY:

	/*
	 * With DANE sessions, send an SNI hint.  We don't care whether the
	 * server reports finding a matching certificate or not, so no
	 * callback is required to process the server response.  Our use of
	 * SNI is limited to giving servers that make use of SNI the best
	 * opportunity to find the certificate they promised via the
	 * associated TLSA RRs.
	 * 
	 * Since the hostname is DNSSEC-validated, it must be a DNS FQDN and
	 * therefore valid for use with SNI.
	 */
	if (SSL_dane_enable(TLScontext->con, 0) <= 0) {
	    msg_warn("%s: error enabling DANE-based certificate validation",
		     TLScontext->namaddr);
	    tls_print_errors();
	    return (0);
	}
	/* RFC7672 Section 3.1.1 specifies no name checks for DANE-EE(3) */
	SSL_dane_set_flags(TLScontext->con, DANE_FLAG_NO_DANE_EE_NAMECHECKS);

	/* Per RFC7672 the SNI name is the TLSA base domain */
	sni = props->dane->base_domain;
	add_namechecks(TLScontext, props);
	break;

    case TLS_LEV_FPRINT:
	/* Synthetic DANE for fingerprint security */
	if (SSL_dane_enable(TLScontext->con, 0) <= 0) {
	    msg_warn("%s: error enabling fingerprint certificate validation",
		     props->namaddr);
	    tls_print_errors();
	    return (0);
	}
	SSL_dane_set_flags(TLScontext->con, DANE_FLAG_NO_DANE_EE_NAMECHECKS);
	break;

    case TLS_LEV_SECURE:
    case TLS_LEV_VERIFY:
	if (TLScontext->dane != 0 && TLScontext->dane->tlsa != 0) {
	    /* Synthetic DANE for per-destination trust-anchors */
	    if (SSL_dane_enable(TLScontext->con, NULL) <= 0) {
		msg_warn("%s: error configuring local trust anchors",
			 props->namaddr);
		tls_print_errors();
		return (0);
	    }
	}
	add_namechecks(TLScontext, props);
	break;
    default:
	break;
    }

    if (sni) {
	if (strlen(sni) > TLSEXT_MAXLEN_host_name) {
	    msg_warn("%s: ignoring too long SNI hostname: %.100s",
		     props->namaddr, sni);
	    return (0);
	}

	/*
	 * Failure to set a valid SNI hostname is a memory allocation error,
	 * and thus transient.  Since we must not cache the session if we
	 * failed to send the SNI name, we have little choice but to abort.
	 */
	if (!SSL_set_tlsext_host_name(TLScontext->con, sni)) {
	    msg_warn("%s: error setting SNI hostname to: %s", props->namaddr,
		     sni);
	    return (0);
	}

	/*
	 * The saved value is not presently used client-side, but could later
	 * be logged if acked by the server (requires new client-side
	 * callback to detect the ack).  For now this just maintains symmetry
	 * with the server code, where do record the received SNI for
	 * logging.
	 */
	TLScontext->peer_sni = mystrdup(sni);
	if (TLScontext->log_mask & TLS_LOG_DEBUG)
	    msg_info("%s: SNI hostname: %s", props->namaddr, sni);
    }
    return (1);
}

/* tls_client_init - initialize client-side TLS engine */

TLS_APPL_STATE *tls_client_init(const TLS_CLIENT_INIT_PROPS *props)
{
    SSL_CTX *client_ctx;
    TLS_APPL_STATE *app_ctx;
    const EVP_MD *fpt_alg;
    long    off = 0;
    int     cachable;
    int     scache_timeout;
    int     log_mask;

    /*
     * Convert user loglevel to internal logmask.
     */
    log_mask = tls_log_mask(props->log_param, props->log_level);

    if (log_mask & TLS_LOG_VERBOSE)
	msg_info("initializing the client-side TLS engine");

    /*
     * Load (mostly cipher related) TLS-library internal main.cf parameters.
     */
    tls_param_init();

    /*
     * Detect mismatch between compile-time headers and run-time library.
     */
    tls_check_version();

    /*
     * Initialize the OpenSSL library, possibly loading its configuration
     * file.
     */
    if (tls_library_init() == 0)
	return (0);

    /*
     * Create an application data index for SSL objects, so that we can
     * attach TLScontext information; this information is needed inside
     * tls_verify_certificate_callback().
     */
    if (TLScontext_index < 0) {
	if ((TLScontext_index = SSL_get_ex_new_index(0, 0, 0, 0, 0)) < 0) {
	    msg_warn("Cannot allocate SSL application data index: "
		     "disabling TLS support");
	    return (0);
	}
    }

    /*
     * If the administrator specifies an unsupported digest algorithm, fail
     * now, rather than in the middle of a TLS handshake.
     */
    if ((fpt_alg = tls_validate_digest(props->mdalg)) == 0) {
	msg_warn("disabling TLS support");
	return (0);
    }

    /*
     * Initialize the PRNG (Pseudo Random Number Generator) with some seed
     * from external and internal sources. Don't enable TLS without some real
     * entropy.
     */
    if (tls_ext_seed(var_tls_daemon_rand_bytes) < 0) {
	msg_warn("no entropy for TLS key generation: disabling TLS support");
	return (0);
    }
    tls_int_seed();

    /*
     * The SSL/TLS specifications require the client to send a message in the
     * oldest specification it understands with the highest level it
     * understands in the message. RFC2487 is only specified for TLSv1, but
     * we want to be as compatible as possible, so we will start off with a
     * SSLv2 greeting allowing the best we can offer: TLSv1. We can restrict
     * this with the options setting later, anyhow.
     */
    ERR_clear_error();
    client_ctx = SSL_CTX_new(TLS_client_method());
    if (client_ctx == 0) {
	msg_warn("cannot allocate client SSL_CTX: disabling TLS support");
	tls_print_errors();
	return (0);
    }
#ifdef SSL_SECOP_PEER
    /* Backwards compatible security as a base for opportunistic TLS. */
    SSL_CTX_set_security_level(client_ctx, 0);
#endif

    /*
     * See the verify callback in tls_verify.c
     */
    SSL_CTX_set_verify_depth(client_ctx, props->verifydepth + 1);

    /*
     * This is a prerequisite for enabling DANE support in OpenSSL, but not a
     * commitment to use DANE, thus suitable for both DANE and non-DANE TLS
     * connections.  Indeed we need this not just for DANE, but aslo for
     * fingerprint and "tafile" support.  Since it just allocates memory, it
     * should never fail except when we're likely to fail anyway.  Rather
     * than try to run with crippled TLS support, just give up using TLS.
     */
    if (SSL_CTX_dane_enable(client_ctx) <= 0) {
	msg_warn("OpenSSL DANE initialization failed: disabling TLS support");
	tls_print_errors();
	return (0);
    }
    tls_dane_digest_init(client_ctx, fpt_alg);

    /*
     * Presently we use TLS only with SMTP where truncation attacks are not
     * possible as a result of application framing.  If we ever use TLS in
     * some other application protocol where truncation could be relevant,
     * we'd need to disable truncation detection conditionally, or explicitly
     * clear the option in that code path.
     */
    off |= SSL_OP_IGNORE_UNEXPECTED_EOF;

    /*
     * Protocol selection is destination dependent, so we delay the protocol
     * selection options to the per-session SSL object.
     */
    off |= tls_bug_bits();
    SSL_CTX_set_options(client_ctx, off);

    /*
     * Set the call-back routine for verbose logging.
     */
    if (log_mask & TLS_LOG_DEBUG)
	SSL_CTX_set_info_callback(client_ctx, tls_info_callback);

    /*
     * Load the CA public key certificates for both the client cert and for
     * the verification of server certificates. As provided by OpenSSL we
     * support two types of CA certificate handling: One possibility is to
     * add all CA certificates to one large CAfile, the other possibility is
     * a directory pointed to by CApath, containing separate files for each
     * CA with softlinks named after the hash values of the certificate. The
     * first alternative has the advantage that the file is opened and read
     * at startup time, so that you don't have the hassle to maintain another
     * copy of the CApath directory for chroot-jail.
     */
    if (tls_set_ca_certificate_info(client_ctx,
				    props->CAfile, props->CApath) < 0) {
	/* tls_set_ca_certificate_info() already logs a warning. */
	SSL_CTX_free(client_ctx);		/* 200411 */
	return (0);
    }

    /*
     * We do not need a client certificate, so the certificates are only
     * loaded (and checked) if supplied. A clever client would handle
     * multiple client certificates and decide based on the list of
     * acceptable CAs, sent by the server, which certificate to submit.
     * OpenSSL does however not do this and also has no call-back hooks to
     * easily implement it.
     * 
     * Load the client public key certificate and private key from file and
     * check whether the cert matches the key. We can use RSA certificates
     * ("cert") DSA certificates ("dcert") or ECDSA certificates ("eccert").
     * All three can be made available at the same time. The CA certificates
     * for all three are handled in the same setup already finished. Which
     * one is used depends on the cipher negotiated (that is: the first
     * cipher listed by the client which does match the server). The client
     * certificate is presented after the server chooses the session cipher,
     * so we will just present the right cert for the chosen cipher (if it
     * uses certificates).
     */
    if (tls_set_my_certificate_key_info(client_ctx,
					props->chain_files,
					props->cert_file,
					props->key_file,
					props->dcert_file,
					props->dkey_file,
					props->eccert_file,
					props->eckey_file) < 0) {
	/* tls_set_my_certificate_key_info() already logs a warning. */
	SSL_CTX_free(client_ctx);		/* 200411 */
	return (0);
    }

    /*
     * With OpenSSL 1.0.2 and later the client EECDH curve list becomes
     * configurable with the preferred curve negotiated via the supported
     * curves extension.  With OpenSSL 3.0 and TLS 1.3, the same applies
     * to the FFDHE groups which become part of a unified "groups" list.
     */
    tls_auto_groups(client_ctx, var_tls_eecdh_auto, var_tls_ffdhe_auto);

    /*
     * Finally, the setup for the server certificate checking, done "by the
     * book".
     */
    SSL_CTX_set_verify(client_ctx, SSL_VERIFY_NONE,
		       tls_verify_certificate_callback);

    /*
     * Initialize the session cache.
     * 
     * Since the client does not search an internal cache, we simply disable it.
     * It is only useful for expiring old sessions, but we do that in the
     * tlsmgr(8).
     * 
     * This makes SSL_CTX_remove_session() not useful for flushing broken
     * sessions from the external cache, so we must delete them directly (not
     * via a callback).
     */
    if (tls_mgr_policy(props->cache_type, &cachable,
		       &scache_timeout) != TLS_MGR_STAT_OK)
	scache_timeout = 0;
    if (scache_timeout <= 0)
	cachable = 0;

    /*
     * Allocate an application context, and populate with mandatory protocol
     * and cipher data.
     */
    app_ctx = tls_alloc_app_context(client_ctx, 0, log_mask);

    /*
     * The external session cache is implemented by the tlsmgr(8) process.
     */
    if (cachable) {

	app_ctx->cache_type = mystrdup(props->cache_type);

	/*
	 * OpenSSL does not use callbacks to load sessions from a client
	 * cache, so we must invoke that function directly. Apparently,
	 * OpenSSL does not provide a way to pass session names from here to
	 * call-back routines that do session lookup.
	 * 
	 * OpenSSL can, however, automatically save newly created sessions for
	 * us by callback (we create the session name in the call-back
	 * function).
	 * 
	 * XXX gcc 2.95 can't compile #ifdef .. #endif in the expansion of
	 * SSL_SESS_CACHE_CLIENT | SSL_SESS_CACHE_NO_INTERNAL_STORE |
	 * SSL_SESS_CACHE_NO_AUTO_CLEAR.
	 */
#ifndef SSL_SESS_CACHE_NO_INTERNAL_STORE
#define SSL_SESS_CACHE_NO_INTERNAL_STORE 0
#endif

	SSL_CTX_set_session_cache_mode(client_ctx,
				       SSL_SESS_CACHE_CLIENT |
				       SSL_SESS_CACHE_NO_INTERNAL_STORE |
				       SSL_SESS_CACHE_NO_AUTO_CLEAR);
	SSL_CTX_sess_set_new_cb(client_ctx, new_client_session_cb);

	/*
	 * OpenSSL ignores timed-out sessions. We need to set the internal
	 * cache timeout at least as high as the external cache timeout. This
	 * applies even if no internal cache is used.  We set the session to
	 * twice the cache lifetime.  This way a session always lasts longer
	 * than its lifetime in the cache.
	 */
	SSL_CTX_set_timeout(client_ctx, 2 * scache_timeout);
    }
    return (app_ctx);
}

 /*
  * This is the actual startup routine for the connection. We expect that the
  * buffers are flushed and the "220 Ready to start TLS" was received by us,
  * so that we can immediately start the TLS handshake process.
  */
TLS_SESS_STATE *tls_client_start(const TLS_CLIENT_START_PROPS *props)
{
    int     sts;
    int     protomask;
    int     min_proto;
    int     max_proto;
    const char *cipher_list;
    SSL_SESSION *session = 0;
    TLS_SESS_STATE *TLScontext;
    TLS_APPL_STATE *app_ctx = props->ctx;
    int     log_mask = app_ctx->log_mask;

    /*
     * When certificate verification is required, log trust chain validation
     * errors even when disabled by default for opportunistic sessions. For
     * DANE this only applies when using trust-anchor associations.
     */
    if (TLS_MUST_MATCH(props->tls_level))
	log_mask |= TLS_LOG_UNTRUSTED;

    if (log_mask & TLS_LOG_VERBOSE)
	msg_info("setting up TLS connection to %s", props->namaddr);

    /*
     * First make sure we have valid protocol and cipher parameters
     * 
     * Per-session protocol restrictions must be applied to the SSL connection,
     * as restrictions in the global context cannot be cleared.
     */
    protomask = tls_proto_mask_lims(props->protocols, &min_proto, &max_proto);
    if (protomask == TLS_PROTOCOL_INVALID) {
	/* tls_protocol_mask() logs no warning. */
	msg_warn("%s: Invalid TLS protocol list \"%s\": aborting TLS session",
		 props->namaddr, props->protocols);
	return (0);
    }

    /*
     * Though RFC7672 set the floor at SSLv3, we really can and should
     * require TLS 1.0, since e.g. we send SNI, which is a TLS 1.0 extension.
     * No DANE domains have been observed to support only SSLv3.
     * 
     * XXX: Would be nice to make that TLS 1.2 at some point.  Users can choose
     * to exclude TLS 1.0 and TLS 1.1 if they find they don't run into any
     * problems doing that.
     */
    if (TLS_DANE_BASED(props->tls_level))
	protomask |= TLS_PROTOCOL_SSLv2 | TLS_PROTOCOL_SSLv3;

    /*
     * Allocate a new TLScontext for the new connection and get an SSL
     * structure. Add the location of TLScontext to the SSL to later retrieve
     * the information inside the tls_verify_certificate_callback().
     * 
     * If session caching was enabled when TLS was initialized, the cache type
     * is stored in the client SSL context.
     */
    TLScontext = tls_alloc_sess_context(log_mask, props->namaddr);
    TLScontext->cache_type = app_ctx->cache_type;
    TLScontext->level = props->tls_level;

    if ((TLScontext->con = SSL_new(app_ctx->ssl_ctx)) == NULL) {
	msg_warn("Could not allocate 'TLScontext->con' with SSL_new()");
	tls_print_errors();
	tls_free_context(TLScontext);
	return (0);
    }

    /*
     * Per session cipher selection for sessions with mandatory encryption
     * 
     * The cipherlist is applied to the global SSL context, since it is likely
     * to stay the same between connections, so we make use of a 1-element
     * cache to return the same result for identical inputs.
     */
    cipher_list = tls_set_ciphers(TLScontext, props->cipher_grade,
				  props->cipher_exclusions);
    if (cipher_list == 0) {
	/* already warned */
	tls_free_context(TLScontext);
	return (0);
    }
    if (log_mask & TLS_LOG_VERBOSE)
	msg_info("%s: TLS cipher list \"%s\"", props->namaddr, cipher_list);

    TLScontext->stream = props->stream;
    TLScontext->mdalg = props->mdalg;

    /* Alias DANE digest info from props */
    TLScontext->dane = props->dane;

    if (!SSL_set_ex_data(TLScontext->con, TLScontext_index, TLScontext)) {
	msg_warn("Could not set application data for 'TLScontext->con'");
	tls_print_errors();
	tls_free_context(TLScontext);
	return (0);
    }
#define CARP_VERSION(which) do { \
        if (which##_proto != 0) \
            msg_warn("%s: error setting %simum TLS version to: 0x%04x", \
                     TLScontext->namaddr, #which, which##_proto); \
        else \
            msg_warn("%s: error clearing %simum TLS version", \
                     TLScontext->namaddr, #which); \
    } while (0)

    /*
     * Apply session protocol restrictions.
     */
    if (protomask != 0)
	SSL_set_options(TLScontext->con, TLS_SSL_OP_PROTOMASK(protomask));
    if (!SSL_set_min_proto_version(TLScontext->con, min_proto))
	CARP_VERSION(min);
    if (!SSL_set_max_proto_version(TLScontext->con, max_proto))
	CARP_VERSION(max);

    /*
     * When applicable, configure DNS-based or synthetic (fingerprint or
     * local trust anchor) DANE authentication, enable an appropriate SNI
     * name and peer name matching.
     * 
     * NOTE, this can change the effective security level, and needs to happen
     * early.
     */
    if (!tls_auth_enable(TLScontext, props)) {
	tls_free_context(TLScontext);
	return (0);
    }

    /*
     * Try to convey the configured TLSA records for this connection to the
     * OpenSSL library.  If none are "usable", we'll fall back to "encrypt"
     * when authentication is not mandatory, otherwise we must arrange to
     * ensure authentication failure.
     */
    if (TLScontext->dane && TLScontext->dane->tlsa) {
	int     usable = tls_dane_enable(TLScontext);
	int     must_fail = usable <= 0;

	if (usable == 0) {
	    switch (TLScontext->level) {
	    case TLS_LEV_HALF_DANE:
	    case TLS_LEV_DANE:
		msg_warn("%s: all TLSA records unusable, fallback to "
			 "unauthenticated TLS", TLScontext->namaddr);
		must_fail = 0;
		TLScontext->level = TLS_LEV_ENCRYPT;
		break;

	    case TLS_LEV_FPRINT:
		msg_warn("%s: all fingerprints unusable", TLScontext->namaddr);
		break;
	    case TLS_LEV_DANE_ONLY:
		msg_warn("%s: all TLSA records unusable", TLScontext->namaddr);
		break;
	    case TLS_LEV_SECURE:
	    case TLS_LEV_VERIFY:
		msg_warn("%s: all trust anchors unusable", TLScontext->namaddr);
		break;
	    }
	}
	TLScontext->must_fail |= must_fail;
    }

    /*
     * We compute the policy digest after we compute the SNI name in
     * tls_auth_enable() and possibly update the TLScontext security level.
     * 
     * OpenSSL will ignore cached sessions that use the wrong protocol. So we do
     * not need to filter out cached sessions with the "wrong" protocol,
     * rather OpenSSL will simply negotiate a new session.
     * 
     * We salt the session lookup key with the protocol list, so that sessions
     * found in the cache are plausibly acceptable.
     * 
     * By the time a TLS client is negotiating ciphers it has already offered to
     * re-use a session, it is too late to renege on the offer. So we must
     * not attempt to re-use sessions whose ciphers are too weak. We salt the
     * session lookup key with the cipher list, so that sessions found in the
     * cache are always acceptable.
     * 
     * With DANE, (more generally any TLScontext where we specified explicit
     * trust-anchor or end-entity certificates) the verification status of
     * the SSL session depends on the specified list.  Since we verify the
     * certificate only during the initial handshake, we must segregate
     * sessions with different TA lists.  Note, that TA re-verification is
     * not possible with cached sessions, since these don't hold the complete
     * peer trust chain.  Therefore, we compute a digest of the sorted TA
     * parameters and append it to the serverid.
     */
    TLScontext->serverid =
	tls_serverid_digest(TLScontext, props, cipher_list);

    /*
     * When authenticating the peer, use 80-bit plus OpenSSL security level
     * 
     * XXX: We should perhaps use security level 1 also for mandatory
     * encryption, with only "may" tolerating weaker algorithms.  But that
     * could mean no TLS 1.0 with OpenSSL >= 3.0 and encrypt, unless I get my
     * patch in on time to conditionally re-enable SHA1 at security level 1,
     * and we add code to make it so.
     * 
     * That said, with "encrypt", we could reasonably require TLS 1.2?
     */
    if (TLS_MUST_MATCH(TLScontext->level))
	SSL_set_security_level(TLScontext->con, 1);

    /*
     * XXX To avoid memory leaks we must always call SSL_SESSION_free() after
     * calling SSL_set_session(), regardless of whether or not the session
     * will be reused.
     */
    if (TLScontext->cache_type) {
	session = load_clnt_session(TLScontext);
	if (session) {
	    SSL_set_session(TLScontext->con, session);
	    SSL_SESSION_free(session);		/* 200411 */
	}
    }

    /*
     * Before really starting anything, try to seed the PRNG a little bit
     * more.
     */
    tls_int_seed();
    (void) tls_ext_seed(var_tls_daemon_rand_bytes);

    /*
     * Connect the SSL connection with the network socket.
     */
    if (SSL_set_fd(TLScontext->con, props->stream == 0 ? props->fd :
		   vstream_fileno(props->stream)) != 1) {
	msg_info("SSL_set_fd error to %s", props->namaddr);
	tls_print_errors();
	uncache_session(app_ctx->ssl_ctx, TLScontext);
	tls_free_context(TLScontext);
	return (0);
    }

    /*
     * If the debug level selected is high enough, all of the data is dumped:
     * TLS_LOG_TLSPKTS will dump the SSL negotiation, TLS_LOG_ALLPKTS will
     * dump everything.
     * 
     * We do have an SSL_set_fd() and now suddenly a BIO_ routine is called?
     * Well there is a BIO below the SSL routines that is automatically
     * created for us, so we can use it for debugging purposes.
     */
    if (log_mask & TLS_LOG_TLSPKTS)
	tls_set_bio_callback(SSL_get_rbio(TLScontext->con), tls_bio_dump_cb);

    /*
     * If we don't trigger the handshake in the library, leave control over
     * SSL_connect/read/write/etc with the application.
     */
    if (props->stream == 0)
	return (TLScontext);

    /*
     * Turn on non-blocking I/O so that we can enforce timeouts on network
     * I/O.
     */
    non_blocking(vstream_fileno(props->stream), NON_BLOCKING);

    /*
     * Start TLS negotiations. This process is a black box that invokes our
     * call-backs for certificate verification.
     * 
     * Error handling: If the SSL handshake fails, we print out an error message
     * and remove all TLS state concerning this session.
     */
    sts = tls_bio_connect(vstream_fileno(props->stream), props->timeout,
			  TLScontext);
    if (sts <= 0) {
	if (ERR_peek_error() != 0) {
	    msg_info("SSL_connect error to %s: %d", props->namaddr, sts);
	    tls_print_errors();
	} else if (errno != 0) {
	    msg_info("SSL_connect error to %s: %m", props->namaddr);
	} else {
	    msg_info("SSL_connect error to %s: lost connection",
		     props->namaddr);
	}
	uncache_session(app_ctx->ssl_ctx, TLScontext);
	tls_free_context(TLScontext);
	return (0);
    }
    return (tls_client_post_connect(TLScontext, props));
}

/* tls_client_post_connect - post-handshake processing */

TLS_SESS_STATE *tls_client_post_connect(TLS_SESS_STATE *TLScontext,
				        const TLS_CLIENT_START_PROPS *props)
{
    const SSL_CIPHER *cipher;
    X509   *peercert;

    /* Turn off packet dump if only dumping the handshake */
    if ((TLScontext->log_mask & TLS_LOG_ALLPKTS) == 0)
	tls_set_bio_callback(SSL_get_rbio(TLScontext->con), 0);

    /*
     * The caller may want to know if this session was reused or if a new
     * session was negotiated.
     */
    TLScontext->session_reused = SSL_session_reused(TLScontext->con);
    if ((TLScontext->log_mask & TLS_LOG_CACHE) && TLScontext->session_reused)
	msg_info("%s: Reusing old session", TLScontext->namaddr);

    /*
     * Do peername verification if requested and extract useful information
     * from the certificate for later use.
     */
    if ((peercert = TLS_PEEK_PEER_CERT(TLScontext->con)) != 0) {
	TLScontext->peer_status |= TLS_CERT_FLAG_PRESENT;

	/*
	 * Peer name or fingerprint verification as requested.
	 * Unconditionally set peer_CN, issuer_CN and peer_cert_fprint. Check
	 * fingerprint first, and avoid logging verified as untrusted in the
	 * call to verify_extract_name().
	 */
	TLScontext->peer_cert_fprint = tls_cert_fprint(peercert, props->mdalg);
	TLScontext->peer_pkey_fprint = tls_pkey_fprint(peercert, props->mdalg);
	verify_extract_name(TLScontext, peercert, props);

	if (TLScontext->log_mask &
	    (TLS_LOG_CERTMATCH | TLS_LOG_VERBOSE | TLS_LOG_PEERCERT))
	    msg_info("%s: subject_CN=%s, issuer_CN=%s, "
		     "fingerprint=%s, pkey_fingerprint=%s", props->namaddr,
		     TLScontext->peer_CN, TLScontext->issuer_CN,
		     TLScontext->peer_cert_fprint,
		     TLScontext->peer_pkey_fprint);
    } else {
	TLScontext->issuer_CN = mystrdup("");
	TLScontext->peer_CN = mystrdup("");
	TLScontext->peer_cert_fprint = mystrdup("");
	TLScontext->peer_pkey_fprint = mystrdup("");
    }

    /*
     * Finally, collect information about protocol and cipher for logging
     */
    TLScontext->protocol = SSL_get_version(TLScontext->con);
    cipher = SSL_get_current_cipher(TLScontext->con);
    TLScontext->cipher_name = SSL_CIPHER_get_name(cipher);
    TLScontext->cipher_usebits = SSL_CIPHER_get_bits(cipher,
					     &(TLScontext->cipher_algbits));

    /*
     * The TLS engine is active. Switch to the tls_timed_read/write()
     * functions and make the TLScontext available to those functions.
     */
    if (TLScontext->stream != 0)
	tls_stream_start(props->stream, TLScontext);

    /*
     * With the handshake done, extract TLS 1.3 signature metadata.
     */
    tls_get_signature_params(TLScontext);

    if (TLScontext->log_mask & TLS_LOG_SUMMARY)
	tls_log_summary(TLS_ROLE_CLIENT, TLS_USAGE_NEW, TLScontext);

    tls_int_seed();

    return (TLScontext);
}

#endif					/* USE_TLS */
