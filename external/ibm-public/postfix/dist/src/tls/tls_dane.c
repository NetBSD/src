/*	$NetBSD: tls_dane.c,v 1.1.1.1.2.2 2014/08/10 07:12:50 tls Exp $	*/

/*++
/* NAME
/*	tls_dane 3
/* SUMMARY
/*	Support for RFC 6698 (DANE) certificate matching
/* SYNOPSIS
/*	#include <tls.h>
/*
/*	int	tls_dane_avail()
/*
/*	void	tls_dane_flush()
/*
/*	void	tls_dane_verbose(on)
/*	int	on;
/*
/*	TLS_DANE *tls_dane_alloc()
/*
/*	void	tls_dane_free(dane)
/*	TLS_DANE *dane;
/*
/*	void	tls_dane_add_ee_digests(dane, mdalg, digest, delim)
/*	TLS_DANE *dane;
/*	const char *mdalg;
/*	const char *digest;
/*	const char *delim;
/*
/*	int	tls_dane_load_trustfile(dane, tafile)
/*	TLS_DANE *dane;
/*	const char *tafile;
/*
/*	int	tls_dane_match(TLSContext, usage, cert, depth)
/*	TLS_SESS_STATE *TLScontext;
/*	int	usage;
/*	X509	*cert;
/*	int	depth;
/*
/*	void	tls_dane_set_callback(ssl_ctx, TLScontext)
/*	SSL_CTX *ssl_ctx;
/*	TLS_SESS_STATE *TLScontext;
/*
/*	TLS_DANE *tls_dane_resolve(port, proto, hostrr, forcetlsa)
/*	unsigned port;
/*	const char *proto;
/*	DNS_RR *hostrr;
/*	int	forcetlsa;
/*
/*	int	tls_dane_unusable(dane)
/*	const TLS_DANE *dane;
/*
/*	int	tls_dane_notfound(dane)
/*	const TLS_DANE *dane;
/* DESCRIPTION
/*	tls_dane_avail() returns true if the features required to support DANE
/*	are present in OpenSSL's libcrypto and in libresolv.  Since OpenSSL's
/*	libcrypto is not initialized until we call tls_client_init(), calls
/*	to tls_dane_avail() must be deferred until this initialization is
/*	completed successufully.
/*
/*	tls_dane_flush() flushes all entries from the cache, and deletes
/*	the cache.
/*
/*	tls_dane_verbose() turns on verbose logging of TLSA record lookups.
/*
/*	tls_dane_alloc() returns a pointer to a newly allocated TLS_DANE
/*	structure with null ta and ee digest sublists.
/*
/*	tls_dane_free() frees the structure allocated by tls_dane_alloc().
/*
/*	tls_dane_add_ee_digests() splits "digest" using the characters in
/*	"delim" as delimiters and stores the results on the EE match list
/*	to match either a certificate or a public key.  This is an incremental
/*	interface, that builds a TLS_DANE structure outside the cache by
/*	manually adding entries.
/*
/*	tls_dane_load_trustfile() imports trust-anchor certificates and
/*	public keys from a file (rather than DNS TLSA records).
/*
/*	tls_dane_match() matches the full and/or public key digest of
/*	"cert" against each candidate digest in TLScontext->dane. If usage
/*	is TLS_DANE_EE, the match is against end-entity digests, otherwise
/*	it is against trust-anchor digests.  Returns true if a match is found,
/*	false otherwise.
/*
/*	tls_dane_set_callback() wraps the SSL certificate verification logic
/*	in a function that modifies the input trust chain and trusted
/*	certificate store to map DANE TA validation onto the existing PKI
/*	verification model.  When TLScontext is NULL the callback is
/*	cleared, otherwise it is set.  This callback should only be set
/*	when out-of-band trust-anchors (via DNSSEC DANE TLSA records or
/*	per-destination local configuration) are provided.  Such trust
/*	anchors always override the legacy public CA PKI.  Otherwise, the
/*	callback MUST be cleared.
/*
/*	tls_dane_resolve() maps a (port, protocol, hostrr) tuple to a
/*	corresponding TLS_DANE policy structure found in the DNS.  The port
/*	argument is in network byte order.  A null pointer is returned when
/*	the DNS query for the TLSA record tempfailed.  In all other cases the
/*	return value is a pointer to the corresponding TLS_DANE structure.
/*	The caller must free the structure via tls_dane_free().
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
/* .IP TLScontext
/*	Client context with TA/EE matching data and related state.
/* .IP usage
/*	Trust anchor (TLS_DANE_TA) or end-entity (TLS_DANE_EE) digests?
/* .IP cert
/*	Certificate from peer trust chain (CA or leaf server).
/* .IP depth
/*	The certificate depth for logging.
/* .IP ssl_ctx
/*	The global SSL_CTX structure used to initialize child SSL
/*	conenctions.
/* .IP mdalg
/*	Name of a message digest algorithm suitable for computing secure
/*	(1st pre-image resistant) message digests of certificates. For now,
/*	md5, sha1, or member of SHA-2 family if supported by OpenSSL.
/* .IP digest
/*	The digest (or list of digests concatenated with characters from
/*	"delim") to be added to the TLS_DANE record.
/* .IP delim
/*	The set of delimiter characters used above.
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
/*	Viktor Dukhovni
/*--*/

/* System library. */

#include <sys_defs.h>
#include <ctype.h>

#ifdef USE_TLS
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
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

#undef TRUST_ANCHOR_SUPPORT
#undef DANE_TLSA_SUPPORT
#undef WRAP_SIGNED

#if OPENSSL_VERSION_NUMBER >= 0x1000000fL && \
	(defined(X509_V_FLAG_PARTIAL_CHAIN) || !defined(OPENSSL_NO_ECDH))
#define TRUST_ANCHOR_SUPPORT

#ifndef X509_V_FLAG_PARTIAL_CHAIN
#define WRAP_SIGNED
#endif

#if defined(TLSEXT_MAXLEN_host_name) && RES_USE_DNSSEC && RES_USE_EDNS0
#define DANE_TLSA_SUPPORT
#endif

#endif					/* OPENSSL_VERSION_NUMBER ... */

#ifdef TRUST_ANCHOR_SUPPORT
static int ta_support = 1;

#else
static int ta_support = 0;

#endif

#ifdef WRAP_SIGNED
static int wrap_signed = 1;

#else
static int wrap_signed = 0;

#endif

#ifdef DANE_TLSA_SUPPORT
static int dane_tlsa_support = 1;

#else
static int dane_tlsa_support = 0;

#endif

static EVP_PKEY *signkey;
static const EVP_MD *signmd;
static const char *signalg;
static ASN1_OBJECT *serverAuth;

/*
 * https://www.iana.org/assignments/dane-parameters/dane-parameters.xhtml
 */
typedef struct {
    const char *mdalg;
    uint8_t dane_id;
} iana_digest;

static iana_digest iana_table[] = {
    {"", DNS_TLSA_MATCHING_TYPE_NO_HASH_USED},
    {"sha256", DNS_TLSA_MATCHING_TYPE_SHA256},
    {"sha512", DNS_TLSA_MATCHING_TYPE_SHA512},
    {0, 0}
};

typedef struct dane_digest {
    struct dane_digest *next;		/* linkage */
    const char *mdalg;			/* OpenSSL name */
    const EVP_MD *md;			/* OpenSSL EVP handle */
    int     len;			/* digest octet length */
    int     pref;			/* tls_dane_digests index or -1 */
    uint8_t dane_id;			/* IANA id */
} dane_digest;

#define MAXDIGESTS 256			/* RFC limit */
static dane_digest *digest_list;
static int digest_agility = -1;

#define AGILITY_OFF	0
#define AGILITY_ON	1
#define AGILITY_MAYBE	2

static NAME_CODE agility[] = {
    {TLS_DANE_AGILITY_OFF, AGILITY_OFF},
    {TLS_DANE_AGILITY_ON, AGILITY_ON},
    {TLS_DANE_AGILITY_MAYBE, AGILITY_MAYBE},
    {0, -1}
};

/*
 * This is not intended to be a long-term cache of pre-parsed TLSA data,
 * rather we primarily want to avoid fetching and parsing the TLSA records
 * for a single multi-homed MX host more than once per delivery. Therefore,
 * we keep the table reasonably small.
 */
#define CACHE_SIZE 20
static CTABLE *dane_cache;

static int dane_initialized;
static int dane_verbose;

/* tls_dane_verbose - enable/disable verbose logging */

void    tls_dane_verbose(int on)
{
    dane_verbose = on;
}

/* add_digest - validate and append digest to digest list */

static dane_digest *add_digest(char *mdalg, int pref)
{
    iana_digest *i;
    dane_digest *d;
    int     dane_id = -1;
    const char *dane_mdalg = mdalg;
    char   *value = split_at(mdalg, '=');
    const EVP_MD *md = 0;
    size_t  mdlen = 0;

    if (value && *value) {
	unsigned long l;
	char   *endcp;

	/*
	 * XXX: safe_strtoul() does not flag empty or white-space only input.
	 * Since we get idbuf by splitting white-space/comma delimited
	 * tokens, this is not a problem here. Fixed as of 210131209.
	 */
	l = safe_strtoul(value, &endcp, 10);
	if ((l == 0 && (errno == EINVAL || endcp == value))
	    || l >= MAXDIGESTS
	    || *endcp) {
	    msg_warn("Invalid matching type number in %s: %s=%s",
		     VAR_TLS_DANE_DIGESTS, mdalg, value);
	    return (0);
	}
	dane_id = l;
    }

    /*
     * Check for known IANA conflicts
     */
    for (i = iana_table; i->mdalg; ++i) {
	if (*mdalg && strcasecmp(i->mdalg, mdalg) == 0) {
	    if (dane_id >= 0 && i->dane_id != dane_id) {
		msg_warn("Non-standard value in %s: %s%s%s",
			 VAR_TLS_DANE_DIGESTS, mdalg,
			 value ? "=" : "", value ? value : "");
		return (0);
	    }
	    dane_id = i->dane_id;
	} else if (i->dane_id == dane_id) {
	    if (*mdalg) {
		msg_warn("Non-standard algorithm in %s: %s%s%s",
			 VAR_TLS_DANE_DIGESTS, mdalg,
			 value ? "=" : "", value ? value : "");
		return (0);
	    }
	    dane_mdalg = i->mdalg;
	}
    }

    /*
     * Check for unknown implicit digest or value
     */
    if (dane_id < 0 || (dane_id > 0 && !*dane_mdalg)) {
	msg_warn("Unknown incompletely specified element in %s: %s%s%s",
		 VAR_TLS_DANE_DIGESTS, mdalg,
		 value ? "=" : "", value ? value : "");
	return 0;
    }

    /*
     * Check for duplicate entries
     */
    for (d = digest_list; d; d = d->next) {
	if (strcasecmp(d->mdalg, dane_mdalg) == 0
	    || d->dane_id == dane_id) {
	    msg_warn("Duplicate element in %s: %s%s%s",
		     VAR_TLS_DANE_DIGESTS, mdalg,
		     value ? "=" : "", value ? value : "");
	    return (0);
	}
    }

    if (*dane_mdalg
	&& ((md = EVP_get_digestbyname(dane_mdalg)) == 0
	    || (mdlen = EVP_MD_size(md)) <= 0
	    || mdlen > EVP_MAX_MD_SIZE)) {
	msg_warn("Unimplemented digest algoritm in %s: %s%s%s",
		 VAR_TLS_DANE_DIGESTS, mdalg,
		 value ? "=" : "", value ? value : "");
	return (0);
    }
    d = (dane_digest *) mymalloc(sizeof(*d));
    d->next = digest_list;
    d->mdalg = mystrdup(dane_mdalg);
    d->md = md;
    d->len = mdlen;
    d->pref = pref;
    d->dane_id = dane_id;

    return (digest_list = d);
}

/* digest_byid - locate digest_table entry for given IANA id */

static dane_digest *digest_byid(uint8_t dane_id)
{
    dane_digest *d;

    for (d = digest_list; d; d = d->next)
	if (d->dane_id == dane_id)
	    return (d);
    return (0);
}

/* digest_pref_byid - digest preference by IANA id */

static int digest_pref_byid(uint8_t dane_id)
{
    dane_digest *d = digest_byid(dane_id);

    return (d ? (d->pref) : (MAXDIGESTS + dane_id));
}

/* gencakey - generate interal DANE root CA key */

static EVP_PKEY *gencakey(void)
{
    EVP_PKEY *key = 0;

#ifdef WRAP_SIGNED
    EC_KEY *eckey;
    EC_GROUP *group = 0;

    ERR_clear_error();

    if ((eckey = EC_KEY_new()) != 0
	&& (group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1)) != 0
	&& (EC_GROUP_set_asn1_flag(group, OPENSSL_EC_NAMED_CURVE),
	    EC_KEY_set_group(eckey, group))
	&& EC_KEY_generate_key(eckey)
	&& (key = EVP_PKEY_new()) != 0
	&& !EVP_PKEY_set1_EC_KEY(key, eckey)) {
	EVP_PKEY_free(key);
	key = 0;
    }
    if (group)
	EC_GROUP_free(group);
    if (eckey)
	EC_KEY_free(eckey);
#endif						/* WRAP_SIGNED */
    return (key);
}

/* dane_init - initialize DANE parameters */

static void dane_init(void)
{
    int     digest_pref = 0;
    char   *cp;
    char   *save;
    char   *tok;
    static char fullmtype[] = "=0";
    dane_digest *d;

    /*
     * Add the full matching type at highest preference and then the users
     * configured list.
     * 
     * The most preferred digest will be used for cert signing and hashing full
     * values for comparison.
     */
    if ((digest_agility = name_code(agility, 0, var_tls_dane_agility)) < 0) {
	msg_warn("Invalid %s syntax: %s. DANE support disabled.",
		 VAR_TLS_DANE_AGILITY, var_tls_dane_agility);
    } else if (add_digest(fullmtype, 0)) {
	save = cp = mystrdup(var_tls_dane_digests);
	while ((tok = mystrtok(&cp, "\t\n\r ,")) != 0) {
	    if ((d = add_digest(tok, ++digest_pref)) == 0) {
		signalg = 0;
		signmd = 0;
		break;
	    }
	    if (digest_pref == 1) {
		signalg = d->mdalg;
		signmd = d->md;
	    }
	}
	myfree(save);
    }
    /* Don't report old news */
    ERR_clear_error();

    /*
     * DANE TLSA support requires trust-anchor support plus working DANE
     * digests.
     */
    if (!ta_support
	|| (wrap_signed && (signkey = gencakey()) == 0)
	|| (serverAuth = OBJ_nid2obj(NID_server_auth)) == 0) {
	msg_warn("cannot generate TA certificates, "
		 "no trust-anchor or DANE support");
	tls_print_errors();
	dane_tlsa_support = ta_support = 0;
    } else if (signmd == 0) {
	msg_warn("digest algorithm initializaton failed, no DANE support");
	tls_print_errors();
	dane_tlsa_support = 0;
    }
    dane_initialized = 1;
}

/* tls_dane_avail - check for availability of dane required digests */

int     tls_dane_avail(void)
{
    if (!dane_initialized)
	dane_init();
    return (dane_tlsa_support);
}

/* tls_dane_flush - flush the cache */

void    tls_dane_flush(void)
{
    if (dane_cache)
	ctable_free(dane_cache);
    dane_cache = 0;
}

/* tls_dane_alloc - allocate a TLS_DANE structure */

TLS_DANE *tls_dane_alloc(void)
{
    TLS_DANE *dane = (TLS_DANE *) mymalloc(sizeof(*dane));

    dane->ta = 0;
    dane->ee = 0;
    dane->certs = 0;
    dane->pkeys = 0;
    dane->base_domain = 0;
    dane->flags = 0;
    dane->expires = 0;
    dane->refs = 1;
    return (dane);
}

static void ta_cert_insert(TLS_DANE *d, X509 *x)
{
    TLS_CERTS *new = (TLS_CERTS *) mymalloc(sizeof(*new));

    CRYPTO_add(&x->references, 1, CRYPTO_LOCK_X509);
    new->cert = x;
    new->next = d->certs;
    d->certs = new;
}

static void free_ta_certs(TLS_DANE *d)
{
    TLS_CERTS *head;
    TLS_CERTS *next;

    for (head = d->certs; head; head = next) {
	next = head->next;
	X509_free(head->cert);
	myfree((char *) head);
    }
}

static void ta_pkey_insert(TLS_DANE *d, EVP_PKEY *k)
{
    TLS_PKEYS *new = (TLS_PKEYS *) mymalloc(sizeof(*new));

    CRYPTO_add(&k->references, 1, CRYPTO_LOCK_EVP_PKEY);
    new->pkey = k;
    new->next = d->pkeys;
    d->pkeys = new;
}

static void free_ta_pkeys(TLS_DANE *d)
{
    TLS_PKEYS *head;
    TLS_PKEYS *next;

    for (head = d->pkeys; head; head = next) {
	next = head->next;
	EVP_PKEY_free(head->pkey);
	myfree((char *) head);
    }
}

static void tlsa_free(TLS_TLSA *tlsa)
{

    myfree(tlsa->mdalg);
    if (tlsa->certs)
	argv_free(tlsa->certs);
    if (tlsa->pkeys)
	argv_free(tlsa->pkeys);
    myfree((char *) tlsa);
}

/* tls_dane_free - free a TLS_DANE structure */

void    tls_dane_free(TLS_DANE *dane)
{
    TLS_TLSA *tlsa;
    TLS_TLSA *next;

    if (--dane->refs > 0)
	return;

    /* De-allocate TA and EE lists */
    for (tlsa = dane->ta; tlsa; tlsa = next) {
	next = tlsa->next;
	tlsa_free(tlsa);
    }
    for (tlsa = dane->ee; tlsa; tlsa = next) {
	next = tlsa->next;
	tlsa_free(tlsa);
    }

    /* De-allocate full trust-anchor certs and pkeys */
    free_ta_certs(dane);
    free_ta_pkeys(dane);
    if (dane->base_domain)
	myfree(dane->base_domain);

    myfree((char *) dane);
}

/* dane_free - ctable style */

static void dane_free(void *dane, void *unused_context)
{
    tls_dane_free((TLS_DANE *) dane);
}

/* dane_locate - list head address of TLSA sublist for given algorithm */

static TLS_TLSA **dane_locate(TLS_TLSA **tlsap, const char *mdalg)
{
    TLS_TLSA *new;

    /*
     * Correct computation of the session cache serverid requires a TLSA
     * digest list that is sorted by algorithm name.  Below we maintain the
     * sort order (by algorithm name canonicalized to lowercase).
     */
    for (; *tlsap; tlsap = &(*tlsap)->next) {
	int     cmp = strcasecmp(mdalg, (*tlsap)->mdalg);

	if (cmp == 0)
	    return (tlsap);
	if (cmp < 0)
	    break;
    }

    new = (TLS_TLSA *) mymalloc(sizeof(*new));
    new->mdalg = lowercase(mystrdup(mdalg));
    new->certs = 0;
    new->pkeys = 0;
    new->next = *tlsap;
    *tlsap = new;

    return (tlsap);
}

/* tls_dane_add_ee_digests - split and append digests */

void    tls_dane_add_ee_digests(TLS_DANE *dane, const char *mdalg,
			              const char *digest, const char *delim)
{
    TLS_TLSA **tlsap = dane_locate(&dane->ee, mdalg);
    TLS_TLSA *tlsa = *tlsap;

    /* Delimited append, may append nothing */
    if (tlsa->pkeys == 0)
	tlsa->pkeys = argv_split(digest, delim);
    else
	argv_split_append(tlsa->pkeys, digest, delim);

    /* Remove empty elements from the list */
    if (tlsa->pkeys->argc == 0) {
	argv_free(tlsa->pkeys);
	tlsa->pkeys = 0;

	if (tlsa->certs == 0) {
	    *tlsap = tlsa->next;
	    tlsa_free(tlsa);
	}
	return;
    }

    /*
     * At the "fingerprint" security level certificate digests and public key
     * digests are interchangeable.  Each leaf certificate is matched via
     * either the public key digest or full certificate digest.  The DER
     * encoding of a certificate is not a valid public key, and conversely,
     * the DER encoding of a public key is not a valid certificate.  An
     * attacker would need a 2nd-preimage that is feasible across types
     * (given cert digest == some pkey digest) and yet presumably difficult
     * within a type (e.g. given cert digest == some other cert digest).  No
     * such attacks are known at this time, and it is expected that if any
     * are found they would work within as well as across the cert/pkey data
     * types.
     */
    if (tlsa->certs == 0)
	tlsa->certs = argv_split(digest, delim);
    else
	argv_split_append(tlsa->certs, digest, delim);
}

/* dane_add - add a digest entry */

static void dane_add(TLS_DANE *dane, int certusage, int selector,
		             const char *mdalg, char *digest)
{
    TLS_TLSA **tlsap;
    TLS_TLSA *tlsa;
    ARGV  **argvp;

    switch (certusage) {
    case DNS_TLSA_USAGE_TRUST_ANCHOR_ASSERTION:
	certusage = TLS_DANE_TA;
	break;
    case DNS_TLSA_USAGE_SERVICE_CERTIFICATE_CONSTRAINT:
    case DNS_TLSA_USAGE_DOMAIN_ISSUED_CERTIFICATE:
	certusage = TLS_DANE_EE;		/* Collapse 1/3 -> 3 */
	break;
    default:
	msg_panic("Unsupported DANE certificate usage: %d", certusage);
    }

    switch (selector) {
    case DNS_TLSA_SELECTOR_FULL_CERTIFICATE:
	selector = TLS_DANE_CERT;
	break;
    case DNS_TLSA_SELECTOR_SUBJECTPUBLICKEYINFO:
	selector = TLS_DANE_PKEY;
	break;
    default:
	msg_panic("Unsupported DANE selector: %d", selector);
    }

    tlsap = (certusage == TLS_DANE_EE) ? &dane->ee : &dane->ta;
    tlsa = *(tlsap = dane_locate(tlsap, mdalg));
    argvp = (selector == TLS_DANE_PKEY) ? &tlsa->pkeys : &tlsa->certs;

    if (*argvp == 0)
	*argvp = argv_alloc(1);
    argv_add(*argvp, digest, ARGV_END);
}

#define FILTER_CTX_AGILITY_OK		(1<<0)
#define FILTER_CTX_CHECK_AGILITY	(1<<1)
#define FILTER_CTX_APPLY_AGILITY	(1<<2)
#define FILTER_CTX_PARSE_DATA		(1<<3)

#define FILTER_RR_DROP			0
#define FILTER_RR_KEEP			1

typedef struct filter_ctx {
    TLS_DANE *dane;			/* Parsed result */
    int     count;			/* Digest mtype count */
    int     target;			/* Digest mtype target count */
    int     flags;			/* Action/result bitmask */
} filter_ctx;

typedef int (*tlsa_filter) (DNS_RR *, filter_ctx *);

/* tlsa_apply - apply filter to each rr in turn */

static DNS_RR *tlsa_apply(DNS_RR *rr, tlsa_filter filter, filter_ctx *ctx)
{
    DNS_RR *head = 0;			/* First retained RR */
    DNS_RR *tail = 0;			/* Last retained RR */
    DNS_RR *next;

    /*
     * XXX Code that modifies or destroys DNS_RR lists or entries belongs in
     * the DNS library, not here.
     */
    for ( /* nop */ ; rr; rr = next) {
	next = rr->next;

	if (filter(rr, ctx) == FILTER_RR_KEEP) {
	    tail = rr;
	    if (!head)
		head = rr;
	} else {
	    if (tail)
		tail->next = rr->next;
	    rr->next = 0;
	    dns_rr_free(rr);
	}
    }
    return (head);
}

/* usmdelta - packed usage/selector/mtype bits changing in next record */

static unsigned int usmdelta(uint8_t u, uint8_t s, uint8_t m, DNS_RR *next)
{
    uint8_t *ip = (next && next->data_len >= 3) ? (uint8_t *) next->data : 0;
    uint8_t nu = ip ? *ip++ : ~u;
    uint8_t ns = ip ? *ip++ : ~s;
    uint8_t nm = ip ? *ip++ : ~m;

    return (((u ^ nu) << 16) | ((s ^ ns) << 8) | (m ^ nm));
}

/* tlsa_rr_cmp - qsort TLSA rrs in case shuffled by name server */

static int tlsa_rr_cmp(DNS_RR *a, DNS_RR *b)
{
    int     cmp;

    /*
     * Sort in ascending order, by usage, selector, matching type preference
     * and payload.  The usage, selector and matching type are the first
     * three unsigned octets of the RR data.
     */
    if (a->data_len > 2 && b->data_len > 2) {
	uint8_t *ai = (uint8_t *) a->data;
	uint8_t *bi = (uint8_t *) b->data;

#define signedcmp(x, y) (((int)(x)) - ((int)(y)))

	if ((cmp = signedcmp(ai[0], bi[0])) != 0
	    || (cmp = signedcmp(ai[1], bi[1])) != 0
	    || (cmp = digest_pref_byid(ai[2]) -
		digest_pref_byid(bi[2])) != 0)
	    return (cmp);
    }
    if ((cmp = a->data_len - b->data_len) != 0)
	return (cmp);
    return (memcmp(a->data, b->data, a->data_len));
}

/* parse_tlsa_rr - parse a validated TLSA RRset */

static int parse_tlsa_rr(DNS_RR *rr, filter_ctx *ctx)
{
    uint8_t *ip;
    uint8_t usage;
    uint8_t selector;
    uint8_t mtype;
    ssize_t dlen;
    D2I_const unsigned char *data;
    D2I_const unsigned char *p;
    int     iscname = strcasecmp(rr->rname, rr->qname);
    const char *q = (iscname) ? (rr)->qname : "";
    const char *a = (iscname) ? " -> " : "";
    const char *r = rr->rname;
    unsigned int change;

    if (rr->type != T_TLSA)
	msg_panic("unexpected non-TLSA RR type %u for %s%s%s", rr->type,
		  q, a, r);

    /* Drop truncated records */
    if ((dlen = rr->data_len - 3) < 0) {
	msg_warn("truncated length %u RR: %s%s%s IN TLSA ...",
		 (unsigned) rr->data_len, q, a, r);
	ctx->flags &= ~FILTER_CTX_AGILITY_OK;
	return (FILTER_RR_DROP);
    }
    ip = (uint8_t *) rr->data;
    usage = *ip++;
    selector = *ip++;
    mtype = *ip++;
    change = usmdelta(usage, selector, mtype, rr->next);
    p = data = (D2I_const unsigned char *) ip;

    /*
     * Handle digest agility for non-zero matching types.
     */
    if (mtype) {
	if (ctx->count && (ctx->flags & FILTER_CTX_APPLY_AGILITY)) {
	    if (change & 0xffff00)		/* New usage/selector, */
		ctx->count = 0;			/* disable drop */
	    return (FILTER_RR_DROP);
	}
	if ((ctx->flags & FILTER_CTX_CHECK_AGILITY)
	    && (ctx->flags & FILTER_CTX_AGILITY_OK)) {
	    ++ctx->count;
	    if (change) {
		/*-
		 * Count changed from last mtype for same usage/selector?
		 * Yes, disable agility.
		 * Else, set or (on usage/selector change) reset target.
		 */
		if (ctx->target && ctx->target != ctx->count)
		    ctx->flags &= ~FILTER_CTX_AGILITY_OK;
		else
		    ctx->target = (change & 0xffff00) ? 0 : ctx->count;
		ctx->count = 0;
	    }
	}
    }
    /*-
     * Drop unsupported usages.
     * Note: NO SUPPORT for usage 0 which does not apply to SMTP.
     * Note: Best-effort support for usage 1, which simply maps to 3.
     */
    switch (usage) {
    case DNS_TLSA_USAGE_TRUST_ANCHOR_ASSERTION:
    case DNS_TLSA_USAGE_SERVICE_CERTIFICATE_CONSTRAINT:
    case DNS_TLSA_USAGE_DOMAIN_ISSUED_CERTIFICATE:
	break;
    default:
	msg_warn("unsupported certificate usage %u in RR: "
		 "%s%s%s IN TLSA %u ...", usage,
		 q, a, r, usage);
	return (FILTER_RR_DROP);
    }

    /*
     * Drop unsupported selectors
     */
    switch (selector) {
    case DNS_TLSA_SELECTOR_FULL_CERTIFICATE:
    case DNS_TLSA_SELECTOR_SUBJECTPUBLICKEYINFO:
	break;
    default:
	msg_warn("unsupported selector %u in RR: "
		 "%s%s%s IN TLSA %u %u ...", selector,
		 q, a, r, usage, selector);
	return (FILTER_RR_DROP);
    }

    if (mtype) {
	dane_digest *d = digest_byid(mtype);

	if (d == 0) {
	    msg_warn("unsupported matching type %u in RR: "
		     "%s%s%s IN TLSA %u %u %u ...", mtype,
		     q, a, r, usage, selector, mtype);
	    return (FILTER_RR_DROP);
	}
	if (dlen != d->len) {
	    msg_warn("malformed %s digest, length %lu, in RR: "
		     "%s%s%s IN TLSA %u %u %u ...",
		     d->mdalg, (unsigned long) dlen,
		     q, a, r, usage, selector, mtype);
	    ctx->flags &= ~FILTER_CTX_AGILITY_OK;
	    return (FILTER_RR_DROP);
	}
	if (!var_tls_dane_taa_dgst
	    && usage == DNS_TLSA_USAGE_TRUST_ANCHOR_ASSERTION) {
	    msg_warn("trust-anchor digests disabled, ignoring RR: "
		     "%s%s%s IN TLSA %u %u %u ...", q, a, r,
		     usage, selector, mtype);
	    return (FILTER_RR_DROP);
	}
	/* New digest mtype next? Prepare to drop following RRs */
	if (change && (change & 0xffff00) == 0
	    && (ctx->flags & FILTER_CTX_APPLY_AGILITY))
	    ++ctx->count;

	if (ctx->flags & FILTER_CTX_PARSE_DATA) {
	    char   *digest = tls_digest_encode(data, dlen);

	    dane_add(ctx->dane, usage, selector, d->mdalg, digest);
	    if (msg_verbose || dane_verbose)
		msg_info("using DANE RR: %s%s%s IN TLSA %u %u %u %s",
			 q, a, r, usage, selector, mtype, digest);
	    myfree(digest);
	}
    } else {
	X509   *x = 0;			/* OpenSSL re-uses *x if x!=0 */
	EVP_PKEY *k = 0;		/* OpenSSL re-uses *k if k!=0 */

	/* Validate the cert or public key via d2i_mumble() */
	switch (selector) {
	case DNS_TLSA_SELECTOR_FULL_CERTIFICATE:
	    if (!d2i_X509(&x, &p, dlen) || dlen != p - data) {
		msg_warn("malformed %s in RR: "
			 "%s%s%s IN TLSA %u %u %u ...", "certificate",
			 q, a, r, usage, selector, mtype);
		if (x)
		    X509_free(x);
		return (FILTER_RR_DROP);
	    }
	    /* Also unusable if public key is malformed or unsupported */
	    k = X509_get_pubkey(x);
	    EVP_PKEY_free(k);
	    if (k == 0) {
		msg_warn("malformed %s in RR: %s%s%s IN TLSA %u %u %u ...",
			 "or unsupported certificate public key",
			 q, a, r, usage, selector, mtype);
		X509_free(x);
		return (FILTER_RR_DROP);
	    }

	    /*
	     * When a full trust-anchor certificate is published via DNS, we
	     * may need to use it to validate the server trust chain. Store
	     * it away for later use.
	     */
	    if (usage == DNS_TLSA_USAGE_TRUST_ANCHOR_ASSERTION
		&& (ctx->flags & FILTER_CTX_PARSE_DATA))
		ta_cert_insert(ctx->dane, x);
	    X509_free(x);
	    break;

	case DNS_TLSA_SELECTOR_SUBJECTPUBLICKEYINFO:
	    if (!d2i_PUBKEY(&k, &p, dlen) || dlen != p - data) {
		msg_warn("malformed %s in RR: %s%s%s IN TLSA %u %u %u ...",
			 "public key", q, a, r, usage, selector, mtype);
		if (k)
		    EVP_PKEY_free(k);
		return (FILTER_RR_DROP);
	    }

	    /*
	     * When a full trust-anchor public key is published via DNS, we
	     * may need to use it to validate the server trust chain. Store
	     * it away for later use.
	     */
	    if (usage == DNS_TLSA_USAGE_TRUST_ANCHOR_ASSERTION
		&& (ctx->flags & FILTER_CTX_PARSE_DATA))
		ta_pkey_insert(ctx->dane, k);
	    EVP_PKEY_free(k);
	    break;
	}

	/*
	 * The cert or key was valid, just digest the raw object, and encode
	 * the digest value.
	 */
	if (ctx->flags & FILTER_CTX_PARSE_DATA) {
	    char   *digest = tls_data_fprint((char *) data, dlen, signalg);

	    dane_add(ctx->dane, usage, selector, signalg, digest);
	    if (msg_verbose || dane_verbose)
		msg_info("using DANE RR: %s%s%s IN TLSA %u %u %u <%s>; "
			 "%s digest %s", q, a, r, usage, selector, mtype,
			 (selector == DNS_TLSA_SELECTOR_FULL_CERTIFICATE) ?
			 "certificate" : "public key", signalg, digest);
	    myfree(digest);
	}
    }
    return (FILTER_RR_KEEP);
}

/* process_rrs - filter and parse the TLSA RRset */

static DNS_RR *process_rrs(TLS_DANE *dane, DNS_RR *rrset)
{
    filter_ctx ctx;

    ctx.dane = dane;
    ctx.count = ctx.target = 0;
    ctx.flags = 0;

    switch (digest_agility) {
    case AGILITY_ON:
	ctx.flags |= FILTER_CTX_APPLY_AGILITY | FILTER_CTX_PARSE_DATA;
	break;
    case AGILITY_OFF:
	ctx.flags |= FILTER_CTX_PARSE_DATA;
	break;
    case AGILITY_MAYBE:
	ctx.flags |= FILTER_CTX_CHECK_AGILITY | FILTER_CTX_AGILITY_OK;
	break;
    }

    rrset = tlsa_apply(rrset, parse_tlsa_rr, &ctx);

    if (digest_agility == AGILITY_MAYBE) {
	/* Two-pass algorithm */
	if (ctx.flags & FILTER_CTX_AGILITY_OK)
	    ctx.flags = FILTER_CTX_APPLY_AGILITY | FILTER_CTX_PARSE_DATA;
	else
	    ctx.flags = FILTER_CTX_PARSE_DATA;
	rrset = tlsa_apply(rrset, parse_tlsa_rr, &ctx);
    }
    if (dane->ta == 0 && dane->ee == 0)
	dane->flags |= TLS_DANE_FLAG_EMPTY;

    return (rrset);
}

/* dane_lookup - TLSA record lookup, ctable style */

static void *dane_lookup(const char *tlsa_fqdn, void *unused_ctx)
{
    static VSTRING *why = 0;
    int     ret;
    DNS_RR *rrs = 0;
    TLS_DANE *dane;

    if (why == 0)
	why = vstring_alloc(10);

    dane = tls_dane_alloc();
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

	    /*
	     * Sort for deterministic digest in session cache lookup key. In
	     * addition we must arrange for more preferred matching types
	     * (full value or digest) to precede less preferred ones for the
	     * same usage and selector.
	     */
	    rrs = dns_rr_sort(rrs, tlsa_rr_cmp);
	    rrs = process_rrs(dane, rrs);
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
#ifdef TRUST_ANCHOR_SUPPORT
    BIO    *bp;
    char   *name = 0;
    char   *header = 0;
    unsigned char *data = 0;
    long    len;
    int     tacount;
    char   *errtype = 0;		/* if error: cert or pkey? */
    const char *mdalg;

    /* nop */
    if (tafile == 0 || *tafile == 0)
	return (1);

    if (!dane_initialized)
	dane_init();

    if (!ta_support) {
	msg_warn("trust-anchor files not supported");
	return (0);
    }
    mdalg = signalg ? signalg : "sha1";

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

    for (tacount = 0;
	 errtype == 0 && PEM_read_bio(bp, &name, &header, &data, &len);
	 ++tacount) {
	D2I_const unsigned char *p = data;
	int     usage = DNS_TLSA_USAGE_TRUST_ANCHOR_ASSERTION;
	int     selector;
	char   *digest;

	if (strcmp(name, PEM_STRING_X509) == 0
	    || strcmp(name, PEM_STRING_X509_OLD) == 0) {
	    X509   *cert = d2i_X509(0, &p, len);

	    if (cert && (p - data) == len) {
		selector = DNS_TLSA_SELECTOR_FULL_CERTIFICATE;
		digest = tls_data_fprint((char *) data, len, mdalg);
		dane_add(dane, usage, selector, mdalg, digest);
		myfree(digest);
		ta_cert_insert(dane, cert);
	    } else
		errtype = "certificate";
	    if (cert)
		X509_free(cert);
	} else if (strcmp(name, PEM_STRING_PUBLIC) == 0) {
	    EVP_PKEY *pkey = d2i_PUBKEY(0, &p, len);

	    if (pkey && (p - data) == len) {
		selector = DNS_TLSA_SELECTOR_SUBJECTPUBLICKEYINFO;
		digest = tls_data_fprint((char *) data, len, mdalg);
		dane_add(dane, usage, selector, mdalg, digest);
		myfree(digest);
		ta_pkey_insert(dane, pkey);
	    } else
		errtype = "public key";
	    if (pkey)
		EVP_PKEY_free(pkey);
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
#else
    msg_warn("Trust anchor files not supported");
#endif
    return (0);
}

/* tls_dane_match - match cert against given list of TA or EE digests */

int     tls_dane_match(TLS_SESS_STATE *TLScontext, int usage,
		               X509 *cert, int depth)
{
    const TLS_DANE *dane = TLScontext->dane;
    TLS_TLSA *tlsa = (usage == TLS_DANE_EE) ? dane->ee : dane->ta;
    const char *namaddr = TLScontext->namaddr;
    const char *ustr = (usage == TLS_DANE_EE) ? "end entity" : "trust anchor";
    int     matched;

    for (matched = 0; tlsa && !matched; tlsa = tlsa->next) {
	char  **dgst;

	/*
	 * Note, set_trust() needs to know whether the match was for a pkey
	 * digest or a certificate digest.  We return MATCHED_PKEY or
	 * MATCHED_CERT accordingly.
	 */
#define MATCHED_CERT 1
#define MATCHED_PKEY 2

	if (tlsa->pkeys) {
	    char   *pkey_dgst = tls_pkey_fprint(cert, tlsa->mdalg);

	    for (dgst = tlsa->pkeys->argv; !matched && *dgst; ++dgst)
		if (strcasecmp(pkey_dgst, *dgst) == 0)
		    matched = MATCHED_PKEY;
	    if (TLScontext->log_mask & (TLS_LOG_VERBOSE | TLS_LOG_CERTMATCH)
		&& matched)
		msg_info("%s: depth=%d matched %s public-key %s digest=%s",
			 namaddr, depth, ustr, tlsa->mdalg, pkey_dgst);
	    myfree(pkey_dgst);
	}
	if (tlsa->certs != 0 && !matched) {
	    char   *cert_dgst = tls_cert_fprint(cert, tlsa->mdalg);

	    for (dgst = tlsa->certs->argv; !matched && *dgst; ++dgst)
		if (strcasecmp(cert_dgst, *dgst) == 0)
		    matched = MATCHED_CERT;
	    if (TLScontext->log_mask & (TLS_LOG_VERBOSE | TLS_LOG_CERTMATCH)
		&& matched)
		msg_info("%s: depth=%d matched %s certificate %s digest %s",
			 namaddr, depth, ustr, tlsa->mdalg, cert_dgst);
	    myfree(cert_dgst);
	}
    }

    return (matched);
}

/* push_ext - push extension onto certificate's stack, else free it */

static int push_ext(X509 *cert, X509_EXTENSION *ext)
{
    x509_extension_stack_t *exts;

    if (ext) {
	if ((exts = cert->cert_info->extensions) == 0)
	    exts = cert->cert_info->extensions = sk_X509_EXTENSION_new_null();
	if (exts && sk_X509_EXTENSION_push(exts, ext))
	    return 1;
	X509_EXTENSION_free(ext);
    }
    return 0;
}

/* add_ext - add simple extension (no config section references) */

static int add_ext(X509 *issuer, X509 *subject, int ext_nid, char *ext_val)
{
    X509V3_CTX v3ctx;

    X509V3_set_ctx(&v3ctx, issuer, subject, 0, 0, 0);
    return push_ext(subject, X509V3_EXT_conf_nid(0, &v3ctx, ext_nid, ext_val));
}

/* set_serial - set serial number to match akid or use subject's plus 1 */

static int set_serial(X509 *cert, AUTHORITY_KEYID *akid, X509 *subject)
{
    int     ret = 0;
    BIGNUM *bn;

    if (akid && akid->serial)
	return (X509_set_serialNumber(cert, akid->serial));

    /*
     * Add one to subject's serial to avoid collisions between TA serial and
     * serial of signing root.
     */
    if ((bn = ASN1_INTEGER_to_BN(X509_get_serialNumber(subject), 0)) != 0
	&& BN_add_word(bn, 1)
	&& BN_to_ASN1_INTEGER(bn, X509_get_serialNumber(cert)))
	ret = 1;

    if (bn)
	BN_free(bn);
    return (ret);
}

/* add_akid - add authority key identifier */

static int add_akid(X509 *cert, AUTHORITY_KEYID *akid)
{
    ASN1_STRING *id;
    unsigned char c = 0;
    int     nid = NID_authority_key_identifier;
    int     ret = 0;

    /*
     * 0 will never be our subject keyid from a SHA-1 hash, but it could be
     * our subject keyid if forced from child's akid.  If so, set our
     * authority keyid to 1.  This way we are never self-signed, and thus
     * exempt from any potential (off by default for now in OpenSSL)
     * self-signature checks!
     */
    id = (ASN1_STRING *) ((akid && akid->keyid) ? akid->keyid : 0);
    if (id && M_ASN1_STRING_length(id) == 1 && *M_ASN1_STRING_data(id) == c)
	c = 1;

    if ((akid = AUTHORITY_KEYID_new()) != 0
	&& (akid->keyid = ASN1_OCTET_STRING_new()) != 0
	&& M_ASN1_OCTET_STRING_set(akid->keyid, (void *) &c, 1)
	&& X509_add1_ext_i2d(cert, nid, akid, 0, X509V3_ADD_DEFAULT) > 0)
	ret = 1;
    if (akid)
	AUTHORITY_KEYID_free(akid);
    return (ret);
}

/* add_skid - add subject key identifier to match child's akid */

static int add_skid(X509 *cert, AUTHORITY_KEYID *akid)
{
    int     nid = NID_subject_key_identifier;

    if (!akid || !akid->keyid)
	return (add_ext(0, cert, nid, "hash"));
    else
	return (X509_add1_ext_i2d(cert, nid, akid->keyid, 0,
				  X509V3_ADD_DEFAULT) > 0);
}

/* akid_issuer_name - get akid issuer directory name */

static X509_NAME *akid_issuer_name(AUTHORITY_KEYID *akid)
{
    if (akid && akid->issuer) {
	int     i;
	general_name_stack_t *gens = akid->issuer;

	for (i = 0; i < sk_GENERAL_NAME_num(gens); ++i) {
	    GENERAL_NAME *gn = sk_GENERAL_NAME_value(gens, i);

	    if (gn->type == GEN_DIRNAME)
		return (gn->d.dirn);
	}
    }
    return (0);
}

/* set_issuer - set issuer DN to match akid if specified */

static int set_issuer_name(X509 *cert, AUTHORITY_KEYID *akid)
{
    X509_NAME *name = akid_issuer_name(akid);

    /*
     * If subject's akid specifies an authority key identifer issuer name, we
     * must use that.
     */
    if (name)
	return (X509_set_issuer_name(cert, name));
    return (X509_set_issuer_name(cert, X509_get_subject_name(cert)));
}

/* grow_chain - add certificate to trusted or untrusted chain */

static void grow_chain(TLS_SESS_STATE *TLScontext, int trusted, X509 *cert)
{
    x509_stack_t **xs = trusted ? &TLScontext->trusted : &TLScontext->untrusted;

#define UNTRUSTED 0
#define TRUSTED 1

    if (!*xs && (*xs = sk_X509_new_null()) == 0)
	msg_fatal("out of memory");
    if (cert) {
	if (trusted && !X509_add1_trust_object(cert, serverAuth))
	    msg_fatal("out of memory");
	CRYPTO_add(&cert->references, 1, CRYPTO_LOCK_X509);
	if (!sk_X509_push(*xs, cert))
	    msg_fatal("out of memory");
    }
}

/* wrap_key - wrap TA "key" as issuer of "subject" */

static void wrap_key(TLS_SESS_STATE *TLScontext, int depth,
		             EVP_PKEY *key, X509 *subject)
{
    X509   *cert = 0;
    AUTHORITY_KEYID *akid;
    X509_NAME *name = X509_get_issuer_name(subject);

    /*
     * The subject name is never a NULL object unless we run out of memory.
     * It may be an empty sequence, but the containing object always exists
     * and its storage is owned by the certificate itself.
     */
    if (name == 0 || (cert = X509_new()) == 0)
	msg_fatal("Out of memory");

    /*
     * Record the depth of the intermediate wrapper certificate, logged in
     * the verify callback.
     */
    if (TLScontext->tadepth < 0) {
	TLScontext->tadepth = depth + 1;
	if (TLScontext->log_mask & (TLS_LOG_VERBOSE | TLS_LOG_CERTMATCH))
	    msg_info("%s: depth=%d chain is trust-anchor signed",
		     TLScontext->namaddr, depth);
    }
    akid = X509_get_ext_d2i(subject, NID_authority_key_identifier, 0, 0);

    ERR_clear_error();

    /*
     * If key is NULL generate a self-signed root CA, with key "signkey",
     * otherwise an intermediate CA signed by above.
     * 
     * CA cert valid for +/- 30 days.
     */
    if (!X509_set_version(cert, 2)
	|| !set_serial(cert, akid, subject)
	|| !X509_set_subject_name(cert, name)
	|| !set_issuer_name(cert, akid)
	|| !X509_gmtime_adj(X509_get_notBefore(cert), -30 * 86400L)
	|| !X509_gmtime_adj(X509_get_notAfter(cert), 30 * 86400L)
	|| !X509_set_pubkey(cert, key ? key : signkey)
	|| !add_ext(0, cert, NID_basic_constraints, "CA:TRUE")
	|| (key && !add_akid(cert, akid))
	|| !add_skid(cert, akid)
	|| (wrap_signed && !X509_sign(cert, signkey, signmd))) {
	tls_print_errors();
	msg_fatal("error generating DANE wrapper certificate");
    }
    if (akid)
	AUTHORITY_KEYID_free(akid);
    if (key && wrap_signed) {
	wrap_key(TLScontext, depth + 1, 0, cert);
	grow_chain(TLScontext, UNTRUSTED, cert);
    } else
	grow_chain(TLScontext, TRUSTED, cert);
    if (cert)
	X509_free(cert);
}

/* wrap_cert - wrap "tacert" as trust-anchor. */

static void wrap_cert(TLS_SESS_STATE *TLScontext, X509 *tacert, int depth)
{
    X509   *cert;
    int     len;
    unsigned char *asn1;
    unsigned char *buf;

    if (TLScontext->tadepth < 0)
	TLScontext->tadepth = depth + 1;

    if (TLScontext->log_mask & (TLS_LOG_VERBOSE | TLS_LOG_CERTMATCH))
	msg_info("%s: depth=%d trust-anchor certificate",
		 TLScontext->namaddr, depth);

    /*
     * If the TA certificate is self-issued, use it directly.
     */
    if (!wrap_signed || X509_check_issued(tacert, tacert) == X509_V_OK) {
	grow_chain(TLScontext, TRUSTED, tacert);
	return;
    }
    /* Deep-copy tacert by converting to ASN.1 and back */
    len = i2d_X509(tacert, NULL);
    asn1 = buf = (unsigned char *) mymalloc(len);
    i2d_X509(tacert, &buf);
    if (buf - asn1 != len)
	msg_panic("i2d_X509 failed to encode TA certificate");

    buf = asn1;
    cert = d2i_X509(0, (D2I_const unsigned char **) &buf, len);
    if (!cert || (buf - asn1) != len)
	msg_panic("d2i_X509 failed to decode TA certificate");
    myfree((char *) asn1);

    grow_chain(TLScontext, UNTRUSTED, cert);

    /* Sign and wrap TA cert with internal "signkey" */
    if (!X509_sign(cert, signkey, signmd)) {
	tls_print_errors();
	msg_fatal("error generating DANE wrapper certificate");
    }
    wrap_key(TLScontext, depth + 1, signkey, cert);
    X509_free(cert);
}

/* ta_signed - is certificate signed by a TLSA cert or pkey */

static int ta_signed(TLS_SESS_STATE *TLScontext, X509 *cert, int depth)
{
    const TLS_DANE *dane = TLScontext->dane;
    EVP_PKEY *pk;
    TLS_PKEYS *k;
    TLS_CERTS *x;
    int     done = 0;

    /*
     * First check whether issued and signed by a TA cert, this is cheaper
     * than the bare-public key checks below, since we can determine whether
     * the candidate TA certificate issued the certificate to be checked
     * first (name comparisons), before we bother with signature checks
     * (public key operations).
     */
    for (x = dane->certs; !done && x; x = x->next) {
	if (X509_check_issued(x->cert, cert) == X509_V_OK) {
	    if ((pk = X509_get_pubkey(x->cert)) == 0)
		continue;
	    /* Check signature, since some other TA may work if not this. */
	    if ((done = (X509_verify(cert, pk) > 0)) != 0)
		wrap_cert(TLScontext, x->cert, depth);
	    EVP_PKEY_free(pk);
	}
    }

    /*
     * With bare TA public keys, we can't check whether the trust chain is
     * issued by the key, but we can determine whether it is signed by the
     * key, so we go with that.
     * 
     * Ideally, the corresponding certificate was presented in the chain, and we
     * matched it by its public key digest one level up.  This code is here
     * to handle adverse conditions imposed by sloppy administrators of
     * receiving systems with poorly constructed chains.
     * 
     * We'd like to optimize out keys that should not match when the cert's
     * authority key id does not match the key id of this key computed via
     * the RFC keyid algorithm (SHA-1 digest of public key bit-string sans
     * ASN1 tag and length thus also excluding the unused bits field that is
     * logically part of the length).  However, some CAs have a non-standard
     * authority keyid, so we lose.  Too bad.
     * 
     * This may push errors onto the stack when the certificate signature is not
     * of the right type or length, throw these away.
     */
    for (k = dane->pkeys; !done && k; k = k->next)
	if ((done = (X509_verify(cert, k->pkey) > 0)) != 0)
	    wrap_key(TLScontext, depth, k->pkey, cert);
	else
	    ERR_clear_error();

    return (done);
}

/* set_trust - configure for DANE validation */

static void set_trust(TLS_SESS_STATE *TLScontext, X509_STORE_CTX *ctx)
{
    int     n;
    int     i;
    int     match;
    int     depth = 0;
    EVP_PKEY *takey;
    X509   *ca;
    X509   *cert = ctx->cert;		/* XXX: Accessor? */
    x509_stack_t *in = ctx->untrusted;	/* XXX: Accessor? */

    /* shallow copy */
    if ((in = sk_X509_dup(in)) == 0)
	msg_fatal("out of memory");

    /*
     * At each iteration we consume the issuer of the current cert.  This
     * reduces the length of the "in" chain by one.  If no issuer is found,
     * we are done.  We also stop when a certificate matches a TA in the
     * peer's TLSA RRset.
     * 
     * Caller ensures that the initial certificate is not self-signed.
     */
    for (n = sk_X509_num(in); n > 0; --n, ++depth) {
	for (i = 0; i < n; ++i)
	    if (X509_check_issued(sk_X509_value(in, i), cert) == X509_V_OK)
		break;

	/*
	 * Final untrusted element with no issuer in the peer's chain, it may
	 * however be signed by a pkey or cert obtained via a TLSA RR.
	 */
	if (i == n)
	    break;

	/* Peer's chain contains an issuer ca. */
	ca = sk_X509_delete(in, i);

	/* Is it a trust anchor? */
	match = tls_dane_match(TLScontext, TLS_DANE_TA, ca, depth + 1);
	if (match) {
	    switch (match) {
	    case MATCHED_CERT:
		wrap_cert(TLScontext, ca, depth);
		break;
	    case MATCHED_PKEY:
		if ((takey = X509_get_pubkey(ca)) == 0)
		    msg_panic("trust-anchor certificate has null pkey");
		wrap_key(TLScontext, depth, takey, cert);
		EVP_PKEY_free(takey);
		break;
	    default:
		msg_panic("unexpected tls_dane_match result: %d", match);
	    }
	    cert = 0;
	    break;
	}
	/* Add untrusted ca. */
	grow_chain(TLScontext, UNTRUSTED, ca);

	/* Final untrusted self-signed element? */
	if (X509_check_issued(ca, ca) == X509_V_OK) {
	    cert = 0;
	    break;
	}
	/* Restart with issuer as subject */
	cert = ca;
    }

    /*
     * When the loop exits, if "cert" is set, it is not self-signed and has
     * no issuer in the chain, we check for a possible signature via a DNS
     * obtained TA cert or public key.  Otherwise, we found no TAs and no
     * issuer, so set an empty list of TAs.
     */
    if (!cert || !ta_signed(TLScontext, cert, depth)) {
	/* Create empty trust list if null, else NOP */
	grow_chain(TLScontext, TRUSTED, 0);
    }
    /* shallow free */
    if (in)
	sk_X509_free(in);
}

/* dane_cb - wrap chain verification for DANE */

static int dane_cb(X509_STORE_CTX *ctx, void *app_ctx)
{
    const char *myname = "dane_cb";
    TLS_SESS_STATE *TLScontext = (TLS_SESS_STATE *) app_ctx;
    X509   *cert = ctx->cert;		/* XXX: accessor? */

    /*
     * Degenerate case: depth 0 self-signed cert.
     * 
     * XXX: Should we suppress name checks, ... when the leaf certificate is a
     * TA.  After all they could sign any name they want.  However, this
     * requires a bit of additional code.  For now we allow depth 0 TAs, but
     * then the peer name has to match.
     */
    if (X509_check_issued(cert, cert) == X509_V_OK) {

	/*
	 * Empty untrusted chain, could be NULL, but then ABI check less
	 * reliable, we may zero some other field, ...
	 */
	grow_chain(TLScontext, UNTRUSTED, 0);
	if (tls_dane_match(TLScontext, TLS_DANE_TA, cert, 0)) {
	    TLScontext->tadepth = 0;
	    grow_chain(TLScontext, TRUSTED, cert);
	} else
	    grow_chain(TLScontext, TRUSTED, 0);
    } else {
	set_trust(TLScontext, ctx);
    }

    /*
     * Check that setting the untrusted chain updates the expected structure
     * member at the expected offset.
     */
    X509_STORE_CTX_trusted_stack(ctx, TLScontext->trusted);
    X509_STORE_CTX_set_chain(ctx, TLScontext->untrusted);
    if (ctx->untrusted != TLScontext->untrusted)
	msg_panic("%s: OpenSSL ABI change", myname);

    return X509_verify_cert(ctx);
}

/* tls_dane_set_callback - set or clear verification wrapper callback */

void    tls_dane_set_callback(SSL_CTX *ctx, TLS_SESS_STATE *TLScontext)
{
    if (ta_support && TLS_DANE_HASTA(TLScontext->dane))
	SSL_CTX_set_cert_verify_callback(ctx, dane_cb, (void *) TLScontext);
    else
	SSL_CTX_set_cert_verify_callback(ctx, 0, 0);
}

#ifdef TEST

#include <unistd.h>
#include <stdarg.h>

#include <mail_params.h>
#include <mail_conf.h>
#include <msg_vstream.h>

/* Cut/paste from OpenSSL 1.0.1: ssl/ssl_cert.c */

static int ssl_verify_cert_chain(SSL *s, x509_stack_t *sk)
{
    X509   *x;
    int     i;
    X509_STORE_CTX ctx;

    if ((sk == NULL) || (sk_X509_num(sk) == 0))
	return (0);

    x = sk_X509_value(sk, 0);
    if (!X509_STORE_CTX_init(&ctx, s->ctx->cert_store, x, sk)) {
	SSLerr(SSL_F_SSL_VERIFY_CERT_CHAIN, ERR_R_X509_LIB);
	return (0);
    }
    X509_STORE_CTX_set_ex_data(&ctx, SSL_get_ex_data_X509_STORE_CTX_idx(), s);
    X509_STORE_CTX_set_default(&ctx, s->server ? "ssl_client" : "ssl_server");
    X509_VERIFY_PARAM_set1(X509_STORE_CTX_get0_param(&ctx), s->param);

    if (s->verify_callback)
	X509_STORE_CTX_set_verify_cb(&ctx, s->verify_callback);

    if (s->ctx->app_verify_callback != NULL)
	i = s->ctx->app_verify_callback(&ctx, s->ctx->app_verify_arg);
    else
	i = X509_verify_cert(&ctx);

    s->verify_result = ctx.error;
    X509_STORE_CTX_cleanup(&ctx);

    return (i);
}

static void add_tlsa(TLS_DANE *dane, char *argv[])
{
    char   *digest;
    X509   *cert = 0;
    BIO    *bp;
    unsigned char *buf;
    unsigned char *buf2;
    int     len;
    uint8_t u = atoi(argv[1]);
    uint8_t s = atoi(argv[2]);
    const char *mdname = argv[3];
    EVP_PKEY *pkey;

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
	buf2 = buf = (unsigned char *) mymalloc(len);
	i2d_X509(cert, &buf2);
	if (!*mdname)
	    ta_cert_insert(dane, cert);
	break;
    case DNS_TLSA_SELECTOR_SUBJECTPUBLICKEYINFO:
	pkey = X509_get_pubkey(cert);
	len = i2d_PUBKEY(pkey, NULL);
	buf2 = buf = (unsigned char *) mymalloc(len);
	i2d_PUBKEY(pkey, &buf2);
	if (!*mdname)
	    ta_pkey_insert(dane, pkey);
	EVP_PKEY_free(pkey);
	break;
    }
    OPENSSL_assert(buf2 - buf == len);

    digest = tls_data_fprint((char *) buf, len, *mdname ? mdname : signalg);
    dane_add(dane, u, s, *mdname ? mdname : signalg, digest);
    myfree((char *) digest);
    myfree((char *) buf);
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

/* match_servername -  match servername against pattern */

static int match_servername(const char *certid, ARGV *margv)
{
    const char *domain;
    const char *parent;
    int     match_subdomain;
    int     i;
    int     idlen;
    int     domlen;

    /*
     * Match the certid against each pattern until we find a match.
     */
    for (i = 0; i < margv->argc; ++i) {
	match_subdomain = 0;
	domain = margv->argv[i];
	if (*domain == '.' && domain[1] != '\0') {
	    ++domain;
	    match_subdomain = 1;
	}

	/*
	 * Sub-domain match: certid is any sub-domain of hostname.
	 */
	if (match_subdomain) {
	    if ((idlen = strlen(certid)) > (domlen = strlen(domain)) + 1
		&& certid[idlen - domlen - 1] == '.'
		&& !strcasecmp(certid + (idlen - domlen), domain))
		return (1);
	    else
		continue;
	}

	/*
	 * Exact match and initial "*" match. The initial "*" in a certid
	 * matches one (if var_tls_multi_label is false) or more hostname
	 * components under the condition that the certid contains multiple
	 * hostname components.
	 */
	if (!strcasecmp(certid, domain)
	    || (certid[0] == '*' && certid[1] == '.' && certid[2] != 0
		&& (parent = strchr(domain, '.')) != 0
		&& (idlen = strlen(certid + 1)) <= (domlen = strlen(parent))
		&& strcasecmp(var_tls_multi_wildcard == 0 ? parent :
			      parent + domlen - idlen,
			      certid + 1) == 0))
	    return (1);
    }
    return (0);
}

static void check_name(TLS_SESS_STATE *tctx, X509 *cert, ARGV *margs)
{
    char   *cn;
    int     matched = 0;
    general_name_stack_t *gens;

    if (SSL_get_verify_result(tctx->con) != X509_V_OK)
	return;

    tctx->peer_status |= TLS_CERT_FLAG_TRUSTED;

    gens = X509_get_ext_d2i(cert, NID_subject_alt_name, 0, 0);
    if (gens) {
	int     has_dnsname = 0;
	int     num_gens = sk_GENERAL_NAME_num(gens);
	int     i;

	for (i = 0; !matched && i < num_gens; ++i) {
	    const GENERAL_NAME *gn = sk_GENERAL_NAME_value(gens, i);
	    const char *dnsname;

	    if (gn->type != GEN_DNS)
		continue;
	    has_dnsname = 1;
	    tctx->peer_status |= TLS_CERT_FLAG_ALTNAME;
	    dnsname = tls_dns_name(gn, tctx);
	    if (dnsname && *dnsname
		&& (matched = match_servername(dnsname, margs)) != 0)
		tctx->peer_status |= TLS_CERT_FLAG_MATCHED;
	}
	sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
	if (has_dnsname)
	    return;
    }
    cn = tls_peer_CN(cert, tctx);
    if (match_servername(cn, margs))
	tctx->peer_status |= TLS_CERT_FLAG_MATCHED;
    myfree(cn);
}

static void check_print(TLS_SESS_STATE *tctx, X509 *cert)
{
    if (TLS_DANE_HASEE(tctx->dane)
	&& tls_dane_match(tctx, TLS_DANE_EE, cert, 0))
	tctx->peer_status |= TLS_CERT_FLAG_TRUSTED | TLS_CERT_FLAG_MATCHED;
}

static void check_peer(TLS_SESS_STATE *tctx, X509 *cert, int argc, char **argv)
{
    ARGV    match;

    tctx->peer_status |= TLS_CERT_FLAG_PRESENT;
    check_print(tctx, cert);
    if (!TLS_CERT_IS_MATCHED(tctx)) {
	match.argc = argc;
	match.argv = argv;
	check_name(tctx, cert, &match);
    }
}

static SSL_CTX *ctx_init(const char *CAfile)
{
    SSL_CTX *client_ctx;

    tls_param_init();
    tls_check_version();

    SSL_load_error_strings();
    SSL_library_init();

    if (!tls_validate_digest(LN_sha1))
	msg_fatal("%s digest algorithm not available", LN_sha1);

    if (TLScontext_index < 0)
	if ((TLScontext_index = SSL_get_ex_new_index(0, 0, 0, 0, 0)) < 0)
	    msg_fatal("Cannot allocate SSL application data index");

    ERR_clear_error();
    if ((client_ctx = SSL_CTX_new(SSLv23_client_method())) == 0)
	msg_fatal("cannot allocate client SSL_CTX");
    SSL_CTX_set_verify_depth(client_ctx, 5);

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
    TLS_SESS_STATE *tctx;
    x509_stack_t *chain;

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
    tctx->mdalg = LN_sha1;
    tctx->dane = tls_dane_alloc();

    if ((tctx->con = SSL_new(ssl_ctx)) == 0
	|| !SSL_set_ex_data(tctx->con, TLScontext_index, tctx)) {
	tls_print_errors();
	msg_fatal("Error allocating SSL connection");
    }
    SSL_set_connect_state(tctx->con);
    add_tlsa((TLS_DANE *) tctx->dane, argv);
    tls_dane_set_callback(ssl_ctx, tctx);

    /* Verify saved server chain */
    chain = load_chain(argv[6]);
    ssl_verify_cert_chain(tctx->con, chain);
    check_peer(tctx, sk_X509_value(chain, 0), argc - 7, argv + 7);
    tls_print_errors();

    msg_info("%s %s", TLS_CERT_IS_MATCHED(tctx) ? "Verified" :
	     TLS_CERT_IS_TRUSTED(tctx) ? "Trusted" : "Untrusted", argv[7]);

    return (TLS_CERT_IS_MATCHED(tctx) ? 0 : 1);
}

#endif					/* TEST */

#endif					/* USE_TLS */
