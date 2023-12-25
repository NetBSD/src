/*	$NetBSD: tls_dane.c,v 1.4.2.1 2023/12/25 12:43:36 martin Exp $	*/

/*++
/* NAME
/*	tls_dane 3
/* SUMMARY
/*	Support for RFC 6698, 7671, 7672 (DANE) certificate matching
/* SYNOPSIS
/*	#include <tls.h>
/*
/*	void tls_dane_loglevel(log_param, log_level);
/*	const char *log_param;
/*	const char *log_level;
/*
/*	int	tls_dane_avail()
/*
/*	void	tls_dane_flush()
/*
/*	TLS_DANE *tls_dane_alloc()
/*
/*      void    tls_tlsa_free(tlsa)
/*      TLS_TLSA *tlsa;
/*
/*	void	tls_dane_free(dane)
/*	TLS_DANE *dane;
/*
/*	void	tls_dane_add_fpt_digests(dane, digest, delim, smtp_mode)
/*	TLS_DANE *dane;
/*	const char *digest;
/*	const char *delim;
/*	int     smtp_mode;
/*
/*	TLS_TLSA *tlsa_prepend(tlsa, usage, selector, mtype, data, len)
/*	TLS_TLSA *tlsa;
/*	uint8_t usage;
/*	uint8_t selector;
/*	uint8_t mtype;
/*	const unsigned char *data;
/*	uint16_t length;
/*
/*	int	tls_dane_load_trustfile(dane, tafile)
/*	TLS_DANE *dane;
/*	const char *tafile;
/*
/*	TLS_DANE *tls_dane_resolve(port, proto, hostrr, forcetlsa)
/*	unsigned port;
/*	const char *proto;
/*	DNS_RR *hostrr;
/*	int	forcetlsa;
/*
/*	void	tls_dane_digest_init(ctx, fpt_alg)
/*	SSL_CTX *ctx;
/*	const EVP_MD *fpt_alg;
/*
/*	void	tls_dane_enable(TLScontext)
/*	TLS_SESS_STATE *TLScontext;
/*
/*	void    tls_dane_log(TLScontext)
/*	TLS_SESS_STATE *TLScontext;
/*
/*	int	tls_dane_unusable(dane)
/*	const TLS_DANE *dane;
/*
/*	int	tls_dane_notfound(dane)
/*	const TLS_DANE *dane;
/* DESCRIPTION
/*	tls_dane_loglevel() allows the policy lookup functions in the DANE
/*	library to examine the application's TLS loglevel in and possibly
/*	produce a more detailed activity log.
/*
/*	tls_dane_avail() returns true if the features required to support DANE
/*	are present in libresolv.
/*
/*	tls_dane_flush() flushes all entries from the cache, and deletes
/*	the cache.
/*
/*	tls_dane_alloc() returns a pointer to a newly allocated TLS_DANE
/*	structure with null ta and ee digest sublists.
/*
/*	tls_tlsa_free() frees a TLSA record linked list.
/*
/*	tls_dane_free() frees the structure allocated by tls_dane_alloc().
/*
/*	tls_dane_digest_init() configures OpenSSL to support the configured
/*	DANE TLSA digests and private-use fingerprint digest.
/*
/*	tlsa_prepend() prepends a TLSA record to the head of a linked list
/*	which may be null when the list is empty. The result value is the
/*	new list head.
/*
/*	tls_dane_add_fpt_digests() splits "digest" using the characters in
/*	"delim" as delimiters and generates corresponding synthetic DANE TLSA
/*	records with matching type 255 (private-use), which we associated with
/*	the configured fingerprint digest algorithm.  This is an incremental
/*	interface, that builds a TLS_DANE structure outside the cache by
/*	manually adding entries.
/*
/*	tls_dane_load_trustfile() imports trust-anchor certificates and
/*	public keys from a file (rather than DNS TLSA records).
/*
/*	tls_dane_resolve() maps a (port, protocol, hostrr) tuple to a
/*	corresponding TLS_DANE policy structure found in the DNS.  The port
/*	argument is in network byte order.  A null pointer is returned when
/*	the DNS query for the TLSA record tempfailed.  In all other cases the
/*	return value is a pointer to the corresponding TLS_DANE structure.
/*	The caller must free the structure via tls_dane_free().
/*
/*	tls_dane_enable() enables DANE-style certificate checks for connections
/*	that are configured with TLSA records.  The TLSA records may be from
/*	DNS (at the "dane", "dane-only" and "half-dane" security levels), or be
/*	synthetic in support of either the "fingerprint" level or local trust
/*	anchor based validation with the "secure" and "verify" levels.  The
/*	return value is the number of "usable" TLSA records loaded, or negative
/*	if a record failed to load due to an internal OpenSSL problems, rather
/*	than an issue with the record making that record "unusable".
/*
/*	tls_dane_log() logs successful verification via DNS-based or
/*	synthetic DANE TLSA RRs (fingerprint or "tafile").
/*
/*	tls_dane_unusable() checks whether a cached TLS_DANE record is
/*	the result of a validated RRset, with no usable elements.  In
/*	this case, TLS is mandatory, but certificate verification is
/*	not DANE-based.
/*
/*	tls_dane_notfound() checks whether a cached TLS_DANE record is
/*	the result of a validated DNS lookup returning NODATA. In
/*	this case, TLS is not required by RFC, though users may elect
/*	a mandatory TLS fallback policy.
/*
/*	Arguments:
/* .IP  ctx
/*	SSL context to be configured with the chosen digest algorithms.
/* .IP  fpt_alg
/*	The OpenSSL EVP digest algorithm handle for the fingerprint digest.
/* .IP  tlsa
/*	TLSA record linked list head, initially NULL.
/* .IP  usage
/*	DANE TLSA certificate usage field.
/* .IP  selector
/*	DANE TLSA selector field.
/* .IP  mtype
/*	DANE TLSA matching type field
/* .IP  data
/*	DANE TLSA associated data field (raw binary form), copied for internal
/*	use.  The caller is responsible for freeing his own copy.
/* .IP  length
/*	Length of DANE TLSA associated DATA field.
/* .IP dane
/*	Pointer to a TLS_DANE structure that lists the valid trust-anchor
/*	and end-entity full-certificate and/or public-key digests.
/* .IP port
/*	The TCP port in network byte order.
/* .IP proto
/*	Almost certainly "tcp".
/* .IP hostrr
/*	DNS_RR pointer to TLSA base domain data.
/* .IP forcetlsa
/*	When true, TLSA lookups are performed even when the qname and rname
/*	are insecure.  This is only useful in the unlikely case that DLV is
/*	used to secure the TLSA RRset in an otherwise insecure zone.
/* .IP log_param
/*	The TLS log level parameter name whose value is the log_level argument.
/* .IP log_level
/*	The application TLS log level, which may affect dane lookup verbosity.
/* .IP digest
/*	The digest (or list of digests concatenated with characters from
/*	"delim") to be added to the TLS_DANE record.
/* .IP delim
/*	The set of delimiter characters used above.
/* .IP smtp_mode
/*	Is the caller an SMTP client or an LMTP client?
/* .IP tafile;
/*	A file with trust anchor certificates or public keys in PEM format.
/* LICENSE
/* .ad
/* .fi
/*	This software is free. You can do with it whatever you want.
/*	The original author kindly requests that you acknowledge
/*	the use of his software.
/* AUTHOR(S)
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
/*	Viktor Dukhovni
/*--*/

/* System library. */

#include <sys_defs.h>
#include <ctype.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

#ifdef USE_TLS
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <midna_domain.h>
#include <vstring.h>
#include <events.h>			/* event_time() */
#include <timecmp.h>
#include <ctable.h>
#include <hex_code.h>
#include <safe_ultostr.h>
#include <split_at.h>
#include <name_code.h>

#define STR(x)	vstring_str(x)

/* Global library */

#include <mail_params.h>

/* DNS library. */

#include <dns.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* Application-specific. */

#undef DANE_TLSA_SUPPORT

#if RES_USE_DNSSEC && RES_USE_EDNS0
#define DANE_TLSA_SUPPORT
static int dane_tlsa_support = 1;

#else
static int dane_tlsa_support = 0;

#endif

/*
 * A NULL alg field disables the algorithm at the codepoint passed to the
 * SSL_CTX_dane_mtype_set(3) function.  The ordinals are used for digest
 * agility, higher is "better" (presumed stronger).
 */
typedef struct dane_mtype {
    const EVP_MD *alg;
    uint8_t ord;
} dane_mtype;

/*
 * This is not intended to be a long-term cache of pre-parsed TLSA data,
 * rather we primarily want to avoid fetching and parsing the TLSA records
 * for a single multi-homed MX host more than once per delivery. Therefore,
 * we keep the table reasonably small.
 */
#define CACHE_SIZE 20
static CTABLE *dane_cache;

static int log_mask;

/* tls_dane_logmask - configure policy lookup logging */

void    tls_dane_loglevel(const char *log_param, const char *log_level)
{
    log_mask = tls_log_mask(log_param, log_level);
}

/* tls_dane_avail - check for availability of dane required digests */

int     tls_dane_avail(void)
{
    return (dane_tlsa_support);
}

/* tls_dane_alloc - allocate a TLS_DANE structure */

TLS_DANE *tls_dane_alloc(void)
{
    TLS_DANE *dane = (TLS_DANE *) mymalloc(sizeof(*dane));

    dane->tlsa = 0;
    dane->base_domain = 0;
    dane->flags = 0;
    dane->expires = 0;
    dane->refs = 1;
    return (dane);
}

/* tls_tlsa_free - free a TLSA RR linked list */

void    tls_tlsa_free(TLS_TLSA *tlsa)
{
    TLS_TLSA *next;

    for (; tlsa; tlsa = next) {
	next = tlsa->next;
	myfree(tlsa->data);
	myfree(tlsa);
    }
}

/* tls_dane_free - free a TLS_DANE structure */

void    tls_dane_free(TLS_DANE *dane)
{
    if (--dane->refs > 0)
	return;
    if (dane->base_domain)
	myfree(dane->base_domain);
    if (dane->tlsa)
	tls_tlsa_free(dane->tlsa);
    myfree((void *) dane);
}

/* tlsa_prepend - Prepend internal-form TLSA record to the RRset linked list */

TLS_TLSA *tlsa_prepend(TLS_TLSA *tlsa, uint8_t usage, uint8_t selector,
		               uint8_t mtype, const unsigned char *data,
		               uint16_t data_len)
{
    TLS_TLSA *head;

    head = (TLS_TLSA *) mymalloc(sizeof(*head));
    head->usage = usage;
    head->selector = selector;
    head->mtype = mtype;
    head->length = data_len;
    head->data = (unsigned char *) mymemdup(data, data_len);
    head->next = tlsa;
    return (head);
}

#define MAX_HEAD_BYTES 32
#define MAX_TAIL_BYTES 32
#define MAX_DUMP_BYTES (MAX_HEAD_BYTES + MAX_TAIL_BYTES)

/* tlsa_info - log import of a particular TLSA record */

static void tlsa_info(const char *tag, const char *msg,
		              uint8_t u, uint8_t s, uint8_t m,
		              const unsigned char *data, ssize_t dlen)
{
    static VSTRING *top;
    static VSTRING *bot;

    if (top == 0)
	top = vstring_alloc(2 * MAX_HEAD_BYTES);
    if (bot == 0)
	bot = vstring_alloc(2 * MAX_TAIL_BYTES);

    if (dlen > MAX_DUMP_BYTES) {
	hex_encode(top, (char *) data, MAX_HEAD_BYTES);
	hex_encode(bot, (char *) data + dlen - MAX_TAIL_BYTES, MAX_TAIL_BYTES);
    } else if (dlen > 0) {
	hex_encode(top, (char *) data, dlen);
    } else {
	vstring_sprintf(top, "...");
    }

    msg_info("%s: %s: %u %u %u %s%s%s", tag, msg, u, s, m, STR(top),
	     dlen > MAX_DUMP_BYTES ? "..." : "",
	     dlen > MAX_DUMP_BYTES ? STR(bot) : "");
}

/* tlsa_carp - carp about a particular TLSA record */

static void tlsa_carp(const char *s1, const char *s2, const char *s3,
		            const char *s4, uint8_t u, uint8_t s, uint8_t m,
		              const unsigned char *data, ssize_t dlen)
{
    static VSTRING *top;
    static VSTRING *bot;

    if (top == 0)
	top = vstring_alloc(2 * MAX_HEAD_BYTES);
    if (bot == 0)
	bot = vstring_alloc(2 * MAX_TAIL_BYTES);

    if (dlen > MAX_DUMP_BYTES) {
	hex_encode(top, (char *) data, MAX_HEAD_BYTES);
	hex_encode(bot, (char *) data + dlen - MAX_TAIL_BYTES, MAX_TAIL_BYTES);
    } else if (dlen > 0) {
	hex_encode(top, (char *) data, dlen);
    } else {
	vstring_sprintf(top, "...");
    }

    msg_warn("%s%s%s %s: %u %u %u %s%s%s", s1, s2, s3, s4, u, s, m, STR(top),
	     dlen > MAX_DUMP_BYTES ? "..." : "",
	     dlen > MAX_DUMP_BYTES ? STR(bot) : "");
}

/* tls_dane_flush - flush the cache */

void    tls_dane_flush(void)
{
    if (dane_cache)
	ctable_free(dane_cache);
    dane_cache = 0;
}

/* dane_free - ctable style */

static void dane_free(void *dane, void *unused_context)
{
    tls_dane_free((TLS_DANE *) dane);
}

/* tls_dane_add_fpt_digests - map fingerprint list to DANE TLSA RRset */

void    tls_dane_add_fpt_digests(TLS_DANE *dane, const char *digest,
				         const char *delim, int smtp_mode)
{
    ARGV   *values = argv_split(digest, delim);
    ssize_t i;

    if (smtp_mode) {
	if (warn_compat_break_smtp_tls_fpt_dgst)
	    msg_info("using backwards-compatible default setting "
		     VAR_SMTP_TLS_FPT_DGST "=md5 to compute certificate "
		     "fingerprints");
    } else {
	if (warn_compat_break_lmtp_tls_fpt_dgst)
	    msg_info("using backwards-compatible default setting "
		     VAR_LMTP_TLS_FPT_DGST "=md5 to compute certificate "
		     "fingerprints");
    }

    for (i = 0; i < values->argc; ++i) {
	const char *cp = values->argv[i];
	size_t  ilen = strlen(cp);
	VSTRING *raw;

	/*
	 * Decode optionally colon-separated hex-encoded string, the input
	 * value requires at most 3 bytes per byte of payload, which must not
	 * exceed the size of the widest supported hash function.
	 */
	if (ilen > 3 * EVP_MAX_MD_SIZE) {
	    msg_warn("malformed fingerprint value: %.100s...",
		     values->argv[i]);
	    continue;
	}
	raw = vstring_alloc(ilen / 2);
	if (hex_decode_opt(raw, cp, ilen, HEX_DECODE_FLAG_ALLOW_COLON) == 0) {
	    myfree(raw);
	    msg_warn("malformed fingerprint value: %.384s", values->argv[i]);
	    continue;
	}

	/*
	 * At the "fingerprint" security level certificate digests and public
	 * key digests are interchangeable.  Each leaf certificate is matched
	 * via either the public key digest or full certificate digest.  The
	 * DER encoding of a certificate is not a valid public key, and
	 * conversely, the DER encoding of a public key is not a valid
	 * certificate.  An attacker would need a 2nd-preimage that is
	 * feasible across types (given cert digest == some pkey digest) and
	 * yet presumably difficult within a type (e.g. given cert digest ==
	 * some other cert digest).  No such attacks are known at this time,
	 * and it is expected that if any are found they would work within as
	 * well as across the cert/pkey data types.
	 * 
	 * The private-use matching type "255" is mapped to the configured
	 * fingerprint digest, which may (harmlessly) coincide with one of
	 * the standard DANE digest algorithms.  The private code point is
	 * however unconditionally enabled.
	 */
	if (log_mask & (TLS_LOG_VERBOSE | TLS_LOG_DANE))
	    tlsa_info("fingerprint", "digest as private-use TLSA record",
		   3, 0, 255, (unsigned char *) STR(raw), VSTRING_LEN(raw));
	dane->tlsa = tlsa_prepend(dane->tlsa, 3, 0, 255,
			      (unsigned char *) STR(raw), VSTRING_LEN(raw));
	dane->tlsa = tlsa_prepend(dane->tlsa, 3, 1, 255,
			      (unsigned char *) STR(raw), VSTRING_LEN(raw));
	vstring_free(raw);
    }
    argv_free(values);
}

/* parse_tlsa_rr - parse a validated TLSA RRset */

static int parse_tlsa_rr(TLS_DANE *dane, DNS_RR *rr)
{
    const uint8_t *ip;
    uint8_t usage;
    uint8_t selector;
    uint8_t mtype;
    ssize_t dlen;
    unsigned const char *data;
    int     iscname = strcasecmp(rr->rname, rr->qname);
    const char *q = iscname ? rr->qname : "";
    const char *a = iscname ? " -> " : "";
    const char *r = rr->rname;

    if (rr->type != T_TLSA)
	msg_panic("%s%s%s: unexpected non-TLSA RR type: %u",
		  q, a, r, rr->type);

    /* Drop truncated records */
    if ((dlen = rr->data_len - 3) < 0) {
	msg_warn("%s%s%s: truncated TLSA RR length == %u",
		 q, a, r, (unsigned) rr->data_len);
	return (0);
    }
    ip = (const uint8_t *) rr->data;
    usage = *ip++;
    selector = *ip++;
    mtype = *ip++;
    data = (const unsigned char *) ip;

    /*-
     * Drop unsupported usages.
     * Note: NO SUPPORT for usages 0/1 which do not apply to SMTP.
     */
    switch (usage) {
    case DNS_TLSA_USAGE_TRUST_ANCHOR_ASSERTION:
    case DNS_TLSA_USAGE_DOMAIN_ISSUED_CERTIFICATE:
	break;
    default:
	tlsa_carp(q, a, r, "unsupported TLSA certificate usage",
		  usage, selector, mtype, data, dlen);
	return (0);
    }

    /*
     * Drop private-use matching type, reserved for fingerprint matching.
     */
    if (mtype == 255) {
	tlsa_carp(q, a, r, "reserved private-use matching type",
		  usage, selector, mtype, data, dlen);
	return (0);
    }
    if (log_mask & (TLS_LOG_VERBOSE | TLS_LOG_DANE))
	tlsa_info("DNSSEC-signed TLSA record", r,
		  usage, selector, mtype, data, dlen);
    dane->tlsa = tlsa_prepend(dane->tlsa, usage, selector, mtype, data, dlen);
    return (1);
}

/* dane_lookup - TLSA record lookup, ctable style */

static void *dane_lookup(const char *tlsa_fqdn, void *unused_ctx)
{
    static VSTRING *why = 0;
    DNS_RR *rrs = 0;
    DNS_RR *rr;
    TLS_DANE *dane = tls_dane_alloc();
    int     ret;

    if (why == 0)
	why = vstring_alloc(10);

    ret = dns_lookup(tlsa_fqdn, T_TLSA, RES_USE_DNSSEC, &rrs, 0, why);

    switch (ret) {
    case DNS_OK:
	if (TLS_DANE_CACHE_TTL_MIN && rrs->ttl < TLS_DANE_CACHE_TTL_MIN)
	    rrs->ttl = TLS_DANE_CACHE_TTL_MIN;
	if (TLS_DANE_CACHE_TTL_MAX && rrs->ttl > TLS_DANE_CACHE_TTL_MAX)
	    rrs->ttl = TLS_DANE_CACHE_TTL_MAX;

	/* One more second to account for discrete time */
	dane->expires = 1 + event_time() + rrs->ttl;

	if (rrs->dnssec_valid) {
	    int     n = 0;

	    for (rr = rrs; rr != 0; rr = rr->next)
		n += parse_tlsa_rr(dane, rr);
	    if (n == 0)
		dane->flags |= TLS_DANE_FLAG_EMPTY;
	} else
	    dane->flags |= TLS_DANE_FLAG_NORRS;

	if (rrs)
	    dns_rr_free(rrs);
	break;

    case DNS_NOTFOUND:
	dane->flags |= TLS_DANE_FLAG_NORRS;
	dane->expires = 1 + event_time() + TLS_DANE_CACHE_TTL_MIN;
	break;

    default:
	msg_warn("DANE TLSA lookup problem: %s", STR(why));
	dane->flags |= TLS_DANE_FLAG_ERROR;
	break;
    }

    return (void *) dane;
}

/* resolve_host - resolve TLSA RRs for hostname (rname or qname) */

static TLS_DANE *resolve_host(const char *host, const char *proto,
			              unsigned port)
{
    static VSTRING *query_domain;
    TLS_DANE *dane;

    if (query_domain == 0)
	query_domain = vstring_alloc(64);

    vstring_sprintf(query_domain, "_%u._%s.%s", ntohs(port), proto, host);
    dane = (TLS_DANE *) ctable_locate(dane_cache, STR(query_domain));
    if (timecmp(event_time(), dane->expires) > 0)
	dane = (TLS_DANE *) ctable_refresh(dane_cache, STR(query_domain));
    if (dane->base_domain == 0)
	dane->base_domain = mystrdup(host);
    /* Increment ref-count of cached entry */
    ++dane->refs;
    return (dane);
}

/* qname_secure - Lookup qname DNSSEC status */

static int qname_secure(const char *qname)
{
    static VSTRING *why;
    int     ret = 0;
    DNS_RR *rrs;

    if (!why)
	why = vstring_alloc(10);

    /*
     * We assume that qname is already an fqdn, and does not need any
     * suffixes from RES_DEFNAME or RES_DNSRCH.  This is typically the name
     * of an MX host, and must be a complete DNS name.  DANE initialization
     * code in the SMTP client is responsible for checking that the default
     * resolver flags do not include RES_DEFNAME and RES_DNSRCH.
     */
    ret = dns_lookup(qname, T_CNAME, RES_USE_DNSSEC, &rrs, 0, why);
    if (ret == DNS_OK) {
	ret = rrs->dnssec_valid;
	dns_rr_free(rrs);
	return (ret);
    }
    if (ret == DNS_NOTFOUND)
	vstring_sprintf(why, "no longer a CNAME");
    msg_warn("DNSSEC status lookup error for %s: %s", qname, STR(why));
    return (-1);
}

/* tls_dane_resolve - cached map: (name, proto, port) -> TLS_DANE */

TLS_DANE *tls_dane_resolve(unsigned port, const char *proto, DNS_RR *hostrr,
			           int forcetlsa)
{
    TLS_DANE *dane = 0;
    int     iscname = strcasecmp(hostrr->rname, hostrr->qname);
    int     isvalid = 1;

    if (!tls_dane_avail())
	return (0);				/* Error */

    /*
     * By default suppress TLSA lookups for hosts in non-DNSSEC zones.  If
     * the host zone is not DNSSEC validated, the TLSA qname sub-domain is
     * safely assumed to not be in a DNSSEC Look-aside Validation child zone.
     */
    if (!forcetlsa && !hostrr->dnssec_valid) {
	isvalid = iscname ? qname_secure(hostrr->qname) : 0;
	if (isvalid < 0)
	    return (0);				/* Error */
    }
    if (!isvalid) {
	dane = tls_dane_alloc();
	dane->flags = TLS_DANE_FLAG_NORRS;
    } else {
	if (!dane_cache)
	    dane_cache = ctable_create(CACHE_SIZE, dane_lookup, dane_free, 0);

	/*
	 * Try the rname first if secure, if nothing there, try the qname if
	 * different.  Note, lookup errors are distinct from success with
	 * nothing found.  If the rname lookup fails we don't try the qname.
	 */
	if (hostrr->dnssec_valid) {
	    dane = resolve_host(hostrr->rname, proto, port);
	    if (tls_dane_notfound(dane) && iscname) {
		tls_dane_free(dane);
		dane = 0;
	    }
	}
	if (!dane)
	    dane = resolve_host(hostrr->qname, proto, port);
	if (dane->flags & TLS_DANE_FLAG_ERROR) {
	    /* We don't return this object. */
	    tls_dane_free(dane);
	    dane = 0;
	}
    }

    return (dane);
}

/* tls_dane_load_trustfile - load trust anchor certs or keys from file */

int     tls_dane_load_trustfile(TLS_DANE *dane, const char *tafile)
{
    BIO    *bp;
    char   *name = 0;
    char   *header = 0;
    unsigned char *data = 0;
    long    len;
    int     tacount;
    char   *errtype = 0;		/* if error: cert or pkey? */

    /* nop */
    if (tafile == 0 || *tafile == 0)
	return (1);

    /*
     * On each call, PEM_read() wraps a stdio file in a BIO_NOCLOSE bio,
     * calls PEM_read_bio() and then frees the bio.  It is just as easy to
     * open a BIO as a stdio file, so we use BIOs and call PEM_read_bio()
     * directly.
     */
    if ((bp = BIO_new_file(tafile, "r")) == NULL) {
	msg_warn("error opening trust anchor file: %s: %m", tafile);
	return (0);
    }
    /* Don't report old news */
    ERR_clear_error();

    /*
     * OpenSSL implements DANE strictly, with DANE-TA(2) only matching issuer
     * certificates, and never the leaf cert.  We also allow the
     * trust-anchors to directly match the leaf certificate or public key.
     */
    for (tacount = 0;
	 errtype == 0 && PEM_read_bio(bp, &name, &header, &data, &len);
	 ++tacount) {
	uint8_t daneta = DNS_TLSA_USAGE_TRUST_ANCHOR_ASSERTION;
	uint8_t daneee = DNS_TLSA_USAGE_DOMAIN_ISSUED_CERTIFICATE;
	uint8_t mtype = DNS_TLSA_MATCHING_TYPE_NO_HASH_USED;

	if (strcmp(name, PEM_STRING_X509) == 0
	    || strcmp(name, PEM_STRING_X509_OLD) == 0) {
	    uint8_t selector = DNS_TLSA_SELECTOR_FULL_CERTIFICATE;

	    if (log_mask & (TLS_LOG_VERBOSE | TLS_LOG_DANE))
		tlsa_info("TA cert as TLSA record", tafile,
			  daneta, selector, mtype, data, len);
	    dane->tlsa =
		tlsa_prepend(dane->tlsa, daneta, selector, mtype, data, len);
	    dane->tlsa =
		tlsa_prepend(dane->tlsa, daneee, selector, mtype, data, len);
	} else if (strcmp(name, PEM_STRING_PUBLIC) == 0) {
	    uint8_t selector = DNS_TLSA_SELECTOR_SUBJECTPUBLICKEYINFO;

	    if (log_mask & (TLS_LOG_VERBOSE | TLS_LOG_DANE))
		tlsa_info("TA pkey as TLSA record", tafile,
			  daneta, selector, mtype, data, len);
	    dane->tlsa =
		tlsa_prepend(dane->tlsa, daneta, selector, mtype, data, len);
	    dane->tlsa = tlsa_prepend(dane->tlsa, daneee, selector, mtype, data, len);
	}

	/*
	 * If any of these were null, PEM_read() would have failed.
	 */
	OPENSSL_free(name);
	OPENSSL_free(header);
	OPENSSL_free(data);
    }
    BIO_free(bp);

    if (errtype) {
	tls_print_errors();
	msg_warn("error reading: %s: malformed trust-anchor %s",
		 tafile, errtype);
	return (0);
    }
    if (ERR_GET_REASON(ERR_peek_last_error()) == PEM_R_NO_START_LINE) {
	/* Reached end of PEM file */
	ERR_clear_error();
	return (tacount > 0);
    }
    /* Some other PEM read error */
    tls_print_errors();
    return (0);
}

int     tls_dane_enable(TLS_SESS_STATE *TLScontext)
{
    const TLS_DANE *dane = TLScontext->dane;
    TLS_TLSA *tp;
    SSL    *ssl = TLScontext->con;
    int     usable = 0;
    int     ret;

    for (tp = dane->tlsa; tp != 0; tp = tp->next) {
	ret = SSL_dane_tlsa_add(ssl, tp->usage, tp->selector,
				tp->mtype, tp->data, tp->length);
	if (ret > 0) {
	    ++usable;
	    continue;
	}
	if (ret == 0) {
	    tlsa_carp(TLScontext->namaddr, ":", "", "unusable TLSA RR",
		      tp->usage, tp->selector, tp->mtype, tp->data,
		      tp->length);
	    continue;
	}
	/* Internal problem in OpenSSL */
	tlsa_carp(TLScontext->namaddr, ":", "", "error loading trust settings",
		  tp->usage, tp->selector, tp->mtype, tp->data, tp->length);
	tls_print_errors();
	return (-1);
    }
    return (usable);
}

/* tls_dane_digest_init - configure supported DANE digests */

void    tls_dane_digest_init(SSL_CTX *ctx, const EVP_MD *fpt_alg)
{
    dane_mtype mtypes[256];
    char   *cp;
    char   *save;
    char   *algname;
    uint8_t m;
    uint8_t ord = 0;
    uint8_t maxtype;

    memset((char *) mtypes, 0, sizeof(mtypes));

    /*
     * The DANE SHA2-256(1) and SHA2-512(2) algorithms are disabled, unless
     * explicitly enabled.  Other codepoints can be disabled explicitly by
     * giving them an empty digest name, which also implicitly disables all
     * smaller codepoints that are not explicitly assigned.
     * 
     * We reserve the private-use code point (255) for use with fingerprint
     * matching.  It MUST NOT be accepted in DNS replies.
     */
    mtypes[1].alg = NULL;
    mtypes[2].alg = NULL;
    mtypes[255].alg = fpt_alg;
    maxtype = 2;

    save = cp = mystrdup(var_tls_dane_digests);
    while ((algname = mystrtok(&cp, CHARS_COMMA_SP)) != 0) {
	char   *algcode = split_at(algname, '=');
	int     codepoint = -1;

	if (algcode && *algcode) {
	    unsigned long l;
	    char   *endcp;

	    /*
	     * XXX: safe_strtoul() does not flag empty or white-space only
	     * input.  Since we get algcode by splitting white-space/comma
	     * delimited tokens, this is not a problem here.
	     */
	    l = safe_strtoul(algcode, &endcp, 10);
	    if ((l == 0 && (errno == EINVAL || endcp == algcode))
		|| l >= 255 || *endcp) {
		msg_warn("Invalid matching type number in %s: %s=%s",
			 VAR_TLS_DANE_DIGESTS, algname, algcode);
		continue;
	    }
	    if (l == 0 || l == 255) {
		msg_warn("Reserved matching type number in %s: %s=%s",
			 VAR_TLS_DANE_DIGESTS, algname, algcode);
		continue;
	    }
	    codepoint = l;
	}
	/* Disable any codepoint gaps */
	if (codepoint > maxtype) {
	    while (++maxtype < codepoint)
		mtypes[codepoint].alg = NULL;
	    maxtype = codepoint;
	}
	/* Handle explicitly disabled codepoints */
	if (*algname == 0) {
	    /* Skip empty specifiers */
	    if (codepoint < 0)
		continue;
	    mtypes[codepoint].alg = NULL;
	    continue;
	}
	switch (codepoint) {
	case -1:
	    if (strcasecmp(algname, LN_sha256) == 0)
		codepoint = 1;			/* SHA2-256(1) */
	    else if (strcasecmp(algname, LN_sha512) == 0)
		codepoint = 2;			/* SHA2-512(2) */
	    else {
		msg_warn("%s: digest algorithm %s needs an explicit number",
			 VAR_TLS_DANE_DIGESTS, algname);
		continue;
	    }
	    break;
	case 1:
	    if (strcasecmp(algname, LN_sha256) != 0) {
		msg_warn("%s: matching type 1 can only be %s",
			 VAR_TLS_DANE_DIGESTS, LN_sha256);
		continue;
	    }
	    algname = LN_sha256;
	    break;
	case 2:
	    if (strcasecmp(algname, LN_sha512) != 0) {
		msg_warn("%s: matching type 2 can only be %s",
			 VAR_TLS_DANE_DIGESTS, LN_sha512);
		continue;
	    }
	    algname = LN_sha512;
	    break;
	default:
	    break;
	}

	if (mtypes[codepoint].ord != 0) {
	    msg_warn("%s: matching type %d specified more than once",
		     VAR_TLS_DANE_DIGESTS, codepoint);
	    continue;
	}
	mtypes[codepoint].ord = ++ord;

	if ((mtypes[codepoint].alg = tls_digest_byname(algname, NULL)) == 0) {
	    msg_warn("%s: digest algorithm \"%s\"(%d) unknown",
		     VAR_TLS_DANE_DIGESTS, algname, codepoint);
	    continue;
	}
    }
    myfree(save);

    for (m = 1; m != 0; m = m != maxtype ? m + 1 : 255) {

	/*
	 * In OpenSSL higher order ordinals are preferred, but we list the
	 * most preferred algorithms first, so the last ordinal becomes 1,
	 * next-to-last, 2, ...
	 * 
	 * The ordinals of non-disabled algorithms are always positive, and the
	 * computed value cannot overflow 254 (the largest possible value of
	 * 'ord' after loading each valid codepoint at most once).
	 */
	if (SSL_CTX_dane_mtype_set(ctx, mtypes[m].alg, m,
				   ord - mtypes[m].ord + 1) <= 0) {
	    msg_warn("%s: error configuring matching type %d",
		     VAR_TLS_DANE_DIGESTS, m);
	    tls_print_errors();
	}
    }
}

/* tls_dane_log - log DANE-based verification success */

void    tls_dane_log(TLS_SESS_STATE *TLScontext)
{
    static VSTRING *top;
    static VSTRING *bot;
    EVP_PKEY *mspki = 0;
    int     depth = SSL_get0_dane_authority(TLScontext->con, NULL, &mspki);
    uint8_t u, s, m;
    unsigned const char *data;
    size_t  dlen;

    if (depth < 0)
	return;					/* No DANE auth */

    switch (TLScontext->level) {
    case TLS_LEV_SECURE:
    case TLS_LEV_VERIFY:
	msg_info("%s: Matched trust anchor at depth %d",
		 TLScontext->namaddr, depth);
	return;
    }

    if (top == 0)
	top = vstring_alloc(2 * MAX_HEAD_BYTES);
    if (bot == 0)
	bot = vstring_alloc(2 * MAX_TAIL_BYTES);

    (void) SSL_get0_dane_tlsa(TLScontext->con, &u, &s, &m, &data, &dlen);
    if (dlen > MAX_DUMP_BYTES) {
	hex_encode(top, (char *) data, MAX_HEAD_BYTES);
	hex_encode(bot, (char *) data + dlen - MAX_TAIL_BYTES, MAX_TAIL_BYTES);
    } else {
	hex_encode(top, (char *) data, dlen);
    }

    switch (TLScontext->level) {
    case TLS_LEV_FPRINT:
	msg_info("%s: Matched fingerprint: %s%s%s", TLScontext->namaddr,
		 STR(top), dlen > MAX_DUMP_BYTES ? "..." : "",
		 dlen > MAX_DUMP_BYTES ? STR(bot) : "");
	return;

    default:
	msg_info("%s: Matched DANE %s at depth %d: %u %u %u %s%s%s",
		 TLScontext->namaddr, mspki ?
		 "TA public key verified certificate" : depth ?
		 "TA certificate" : "EE certificate", depth, u, s, m,
		 STR(top), dlen > MAX_DUMP_BYTES ? "..." : "",
		 dlen > MAX_DUMP_BYTES ? STR(bot) : "");
	return;
    }
}

#ifdef TEST

#include <unistd.h>
#include <stdarg.h>

#include <mail_params.h>
#include <mail_conf.h>
#include <msg_vstream.h>

static int verify_chain(SSL *ssl, x509_stack_t *chain, TLS_SESS_STATE *tctx)
{
    int     ret;
    X509   *cert;
    X509_STORE_CTX *store_ctx;
    SSL_CTX *ssl_ctx = SSL_get_SSL_CTX(ssl);
    X509_STORE *store = SSL_CTX_get_cert_store(ssl_ctx);
    int     store_ctx_idx = SSL_get_ex_data_X509_STORE_CTX_idx();

    cert = sk_X509_value(chain, 0);
    if ((store_ctx = X509_STORE_CTX_new()) == NULL) {
	SSLerr(SSL_F_SSL_VERIFY_CERT_CHAIN, ERR_R_MALLOC_FAILURE);
	return 0;
    }
    if (!X509_STORE_CTX_init(store_ctx, store, cert, chain)) {
	X509_STORE_CTX_free(store_ctx);
	return 0;
    }
    X509_STORE_CTX_set_ex_data(store_ctx, store_ctx_idx, ssl);

    /* We're *verifying* a server chain */
    X509_STORE_CTX_set_default(store_ctx, "ssl_server");
    X509_VERIFY_PARAM_set1(X509_STORE_CTX_get0_param(store_ctx),
			   SSL_get0_param(ssl));
    X509_STORE_CTX_set0_dane(store_ctx, SSL_get0_dane(ssl));

    ret = X509_verify_cert(store_ctx);

    SSL_set_verify_result(ssl, X509_STORE_CTX_get_error(store_ctx));
    X509_STORE_CTX_free(store_ctx);

    return (ret);
}

static void load_tlsa_args(SSL *ssl, char *argv[])
{
    const EVP_MD *md = 0;
    X509   *cert = 0;
    BIO    *bp;
    unsigned char *buf;
    unsigned char *buf2;
    int     len;
    uint8_t u = atoi(argv[1]);
    uint8_t s = atoi(argv[2]);
    uint8_t m = atoi(argv[3]);
    EVP_PKEY *pkey;

    /* Unsupported usages are fatal */
    switch (u) {
    case DNS_TLSA_USAGE_TRUST_ANCHOR_ASSERTION:
    case DNS_TLSA_USAGE_DOMAIN_ISSUED_CERTIFICATE:
	break;
    default:
	msg_fatal("unsupported certificate usage %u", u);
    }

    /* Unsupported selectors are fatal */
    switch (s) {
    case DNS_TLSA_SELECTOR_FULL_CERTIFICATE:
    case DNS_TLSA_SELECTOR_SUBJECTPUBLICKEYINFO:
	break;
    default:
	msg_fatal("unsupported selector %u", s);
    }

    /* Unsupported selectors are fatal */
    switch (m) {
    case DNS_TLSA_MATCHING_TYPE_NO_HASH_USED:
    case DNS_TLSA_MATCHING_TYPE_SHA256:
    case DNS_TLSA_MATCHING_TYPE_SHA512:
	break;
    default:
	msg_fatal("unsupported matching type %u", m);
    }

    if ((bp = BIO_new_file(argv[4], "r")) == NULL)
	msg_fatal("error opening %s: %m", argv[4]);
    if (!PEM_read_bio_X509(bp, &cert, 0, 0)) {
	tls_print_errors();
	msg_fatal("error loading certificate from %s: %m", argv[4]);
    }
    BIO_free(bp);

    /*
     * Extract ASN.1 DER form of certificate or public key.
     */
    switch (s) {
    case DNS_TLSA_SELECTOR_FULL_CERTIFICATE:
	len = i2d_X509(cert, NULL);
	if (len > 0xffff)
	    msg_fatal("certificate too long: %d", len);
	buf2 = buf = (unsigned char *) mymalloc(len);
	i2d_X509(cert, &buf2);
	break;
    case DNS_TLSA_SELECTOR_SUBJECTPUBLICKEYINFO:
	pkey = X509_get_pubkey(cert);
	len = i2d_PUBKEY(pkey, NULL);
	if (len > 0xffff)
	    msg_fatal("public key too long: %d", len);
	buf2 = buf = (unsigned char *) mymalloc(len);
	i2d_PUBKEY(pkey, &buf2);
	EVP_PKEY_free(pkey);
	break;
    }
    X509_free(cert);
    OPENSSL_assert(buf2 - buf == len);

    switch (m) {
    case 0:
	break;
    case 1:
	if ((md = tls_digest_byname(LN_sha256, NULL)) == 0)
	    msg_fatal("Digest %s not found", LN_sha256);
	break;
    case 2:
	if ((md = tls_digest_byname(LN_sha512, NULL)) == 0)
	    msg_fatal("Digest %s not found", LN_sha512);
	break;
    default:
	msg_fatal("Unsupported DANE mtype: %d", m);
    }

    if (md != 0) {
	unsigned char mdbuf[EVP_MAX_MD_SIZE];
	unsigned int mdlen = sizeof(mdbuf);

	if (!EVP_Digest(buf, len, mdbuf, &mdlen, md, 0))
	    msg_fatal("Digest failure for mtype: %d", m);
	myfree(buf);
	buf = (unsigned char *) mymemdup(mdbuf, len = mdlen);
    }
    SSL_dane_tlsa_add(ssl, u, s, m, buf, len);
    myfree((void *) buf);
}

static x509_stack_t *load_chain(const char *chainfile)
{
    BIO    *bp;
    char   *name = 0;
    char   *header = 0;
    unsigned char *data = 0;
    long    len;
    int     count;
    char   *errtype = 0;		/* if error: cert or pkey? */
    x509_stack_t *chain;
    typedef X509 *(*d2i_X509_t) (X509 **, const unsigned char **, long);

    if ((chain = sk_X509_new_null()) == 0) {
	perror("malloc");
	exit(1);
    }

    /*
     * On each call, PEM_read() wraps a stdio file in a BIO_NOCLOSE bio,
     * calls PEM_read_bio() and then frees the bio.  It is just as easy to
     * open a BIO as a stdio file, so we use BIOs and call PEM_read_bio()
     * directly.
     */
    if ((bp = BIO_new_file(chainfile, "r")) == NULL) {
	fprintf(stderr, "error opening chainfile: %s: %m\n", chainfile);
	exit(1);
    }
    /* Don't report old news */
    ERR_clear_error();

    for (count = 0;
	 errtype == 0 && PEM_read_bio(bp, &name, &header, &data, &len);
	 ++count) {
	const unsigned char *p = data;

	if (strcmp(name, PEM_STRING_X509) == 0
	    || strcmp(name, PEM_STRING_X509_TRUSTED) == 0
	    || strcmp(name, PEM_STRING_X509_OLD) == 0) {
	    d2i_X509_t d;
	    X509   *cert;

	    d = strcmp(name, PEM_STRING_X509_TRUSTED) ? d2i_X509_AUX : d2i_X509;
	    if ((cert = d(0, &p, len)) == 0 || (p - data) != len)
		errtype = "certificate";
	    else if (sk_X509_push(chain, cert) == 0) {
		perror("malloc");
		exit(1);
	    }
	} else {
	    fprintf(stderr, "unexpected chain file object: %s\n", name);
	    exit(1);
	}

	/*
	 * If any of these were null, PEM_read() would have failed.
	 */
	OPENSSL_free(name);
	OPENSSL_free(header);
	OPENSSL_free(data);
    }
    BIO_free(bp);

    if (errtype) {
	tls_print_errors();
	fprintf(stderr, "error reading: %s: malformed %s", chainfile, errtype);
	exit(1);
    }
    if (ERR_GET_REASON(ERR_peek_last_error()) == PEM_R_NO_START_LINE) {
	/* Reached end of PEM file */
	ERR_clear_error();
	if (count > 0)
	    return chain;
	fprintf(stderr, "no certificates found in: %s\n", chainfile);
	exit(1);
    }
    /* Some other PEM read error */
    tls_print_errors();
    fprintf(stderr, "error reading: %s\n", chainfile);
    exit(1);
}

static void usage(const char *progname)
{
    fprintf(stderr, "Usage: %s certificate-usage selector matching-type"
	    " certfile \\\n\t\tCAfile chainfile hostname [certname ...]\n",
	    progname);
    fprintf(stderr, "  where, certificate-usage = TLSA certificate usage,\n");
    fprintf(stderr, "\t selector = TLSA selector,\n");
    fprintf(stderr, "\t matching-type = empty string or OpenSSL digest algorithm name,\n");
    fprintf(stderr, "\t PEM certfile provides certificate association data,\n");
    fprintf(stderr, "\t PEM CAfile contains any usage 0/1 trusted roots,\n");
    fprintf(stderr, "\t PEM chainfile = server chain file to verify\n");
    fprintf(stderr, "\t hostname = destination hostname,\n");
    fprintf(stderr, "\t each certname augments the hostname for name checks.\n");
    exit(1);
}

static SSL_CTX *ctx_init(const char *CAfile)
{
    SSL_CTX *client_ctx;

    tls_param_init();
    tls_check_version();

    if (!tls_validate_digest(LN_sha1))
	msg_fatal("%s digest algorithm not available", LN_sha1);

    if (TLScontext_index < 0)
	if ((TLScontext_index = SSL_get_ex_new_index(0, 0, 0, 0, 0)) < 0)
	    msg_fatal("Cannot allocate SSL application data index");

    ERR_clear_error();
    if ((client_ctx = SSL_CTX_new(TLS_client_method())) == 0)
	msg_fatal("cannot allocate client SSL_CTX");
    SSL_CTX_set_verify_depth(client_ctx, 5);

    /* Enable DANE support in OpenSSL */
    if (SSL_CTX_dane_enable(client_ctx) <= 0) {
	tls_print_errors();
	msg_fatal("OpenSSL DANE initialization failed");
    }
    if (tls_set_ca_certificate_info(client_ctx, CAfile, "") < 0) {
	tls_print_errors();
	msg_fatal("cannot load CAfile: %s", CAfile);
    }
    SSL_CTX_set_verify(client_ctx, SSL_VERIFY_NONE,
		       tls_verify_certificate_callback);
    return (client_ctx);
}

int     main(int argc, char *argv[])
{
    SSL_CTX *ssl_ctx;
    const EVP_MD *fpt_alg;
    TLS_SESS_STATE *tctx;
    x509_stack_t *chain;
    int     i;

    var_procname = mystrdup(basename(argv[0]));
    set_mail_conf_str(VAR_PROCNAME, var_procname);
    msg_vstream_init(var_procname, VSTREAM_OUT);

    if (argc < 8)
	usage(argv[0]);

    ssl_ctx = ctx_init(argv[5]);
    if (!tls_dane_avail())
	msg_fatal("DANE TLSA support not available");

    tctx = tls_alloc_sess_context(TLS_LOG_NONE, argv[7]);
    tctx->namaddr = argv[7];
    tctx->mdalg = LN_sha256;
    tctx->dane = tls_dane_alloc();

    if ((fpt_alg = tls_validate_digest(tctx->mdalg)) == 0)
	msg_fatal("fingerprint digest algorithm %s not found",
		  tctx->mdalg);
    tls_dane_digest_init(ssl_ctx, fpt_alg);

    if ((tctx->con = SSL_new(ssl_ctx)) == 0
	|| !SSL_set_ex_data(tctx->con, TLScontext_index, tctx)) {
	tls_print_errors();
	msg_fatal("Error allocating SSL connection");
    }
    if (SSL_dane_enable(tctx->con, 0) <= 0) {
	tls_print_errors();
	msg_fatal("Error enabling DANE for SSL handle");
    }
    SSL_dane_set_flags(tctx->con, DANE_FLAG_NO_DANE_EE_NAMECHECKS);
    SSL_dane_set_flags(tctx->con, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
    for (i = 7; i < argc; ++i)
	if (!SSL_add1_host(tctx->con, argv[i]))
	    msg_fatal("error adding hostname: %s", argv[i]);
    load_tlsa_args(tctx->con, argv);
    SSL_set_connect_state(tctx->con);

    /* Verify saved server chain */
    chain = load_chain(argv[6]);
    i = verify_chain(tctx->con, chain, tctx);
    tls_print_errors();

    if (i > 0) {
	const char *peername = SSL_get0_peername(tctx->con);

	if (peername == 0)
	    peername = argv[7];
	msg_info("Verified %s", peername);
    } else {
	i = SSL_get_verify_result(tctx->con);
	msg_info("certificate verification failed for %s:%s: num=%d:%s",
		 argv[6], argv[7], i, X509_verify_cert_error_string(i));
    }

    return (i <= 0);
}

#endif					/* TEST */

#endif					/* USE_TLS */
