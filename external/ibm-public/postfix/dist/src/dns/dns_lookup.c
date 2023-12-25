/*	$NetBSD: dns_lookup.c,v 1.7.2.1 2023/12/25 12:43:31 martin Exp $	*/

/*++
/* NAME
/*	dns_lookup 3
/* SUMMARY
/*	domain name service lookup
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	int	dns_lookup(name, type, rflags, list, fqdn, why)
/*	const char *name;
/*	unsigned type;
/*	unsigned rflags;
/*	DNS_RR	**list;
/*	VSTRING *fqdn;
/*	VSTRING *why;
/*
/*	int	dns_lookup_l(name, rflags, list, fqdn, why, lflags, ltype, ...)
/*	const char *name;
/*	unsigned rflags;
/*	DNS_RR	**list;
/*	VSTRING *fqdn;
/*	VSTRING *why;
/*	int	lflags;
/*	unsigned ltype;
/*
/*	int	dns_lookup_v(name, rflags, list, fqdn, why, lflags, ltype)
/*	const char *name;
/*	unsigned rflags;
/*	DNS_RR	**list;
/*	VSTRING *fqdn;
/*	VSTRING *why;
/*	int	lflags;
/*	unsigned *ltype;
/*
/*	int	dns_get_h_errno()
/* AUXILIARY FUNCTIONS
/*	extern int var_dns_ncache_ttl_fix;
/*
/*	int	dns_lookup_r(name, type, rflags, list, fqdn, why, rcode)
/*	const char *name;
/*	unsigned type;
/*	unsigned rflags;
/*	DNS_RR	**list;
/*	VSTRING *fqdn;
/*	VSTRING *why;
/*	int	*rcode;
/*
/*	int	dns_lookup_rl(name, rflags, list, fqdn, why, rcode, lflags,
/*				ltype, ...)
/*	const char *name;
/*	unsigned rflags;
/*	DNS_RR	**list;
/*	VSTRING *fqdn;
/*	VSTRING *why;
/*	int	*rcode;
/*	int	lflags;
/*	unsigned ltype;
/*
/*	int	dns_lookup_rv(name, rflags, list, fqdn, why, rcode, lflags,
/*				ltype)
/*	const char *name;
/*	unsigned rflags;
/*	DNS_RR	**list;
/*	VSTRING *fqdn;
/*	VSTRING *why;
/*	int	*rcode;
/*	int	lflags;
/*	unsigned *ltype;
/*
/*	int	dns_lookup_x(name, type, rflags, list, fqdn, why, rcode, lflags)
/*	const char *name;
/*	unsigned type;
/*	unsigned rflags;
/*	DNS_RR	**list;
/*	VSTRING *fqdn;
/*	VSTRING *why;
/*	int	*rcode;
/*	unsigned lflags;
/* DESCRIPTION
/*	dns_lookup() looks up DNS resource records. When requested to
/*	look up data other than type CNAME, it will follow a limited
/*	number of CNAME indirections. All result names (including
/*	null terminator) will fit a buffer of size DNS_NAME_LEN.
/*	All name results are validated by \fIvalid_hostname\fR();
/*	an invalid name is reported as a DNS_INVAL result, while
/*	malformed replies are reported as transient errors.
/*
/*	dns_get_h_errno() returns the last error. This deprecates
/*	usage of the global h_errno variable. We should not rely
/*	on that being updated.
/*
/*	dns_lookup_l() and dns_lookup_v() allow the user to specify
/*	a list of resource types.
/*
/*	dns_lookup_x, dns_lookup_r(), dns_lookup_rl() and dns_lookup_rv()
/*	accept or return additional information.
/*
/*	The var_dns_ncache_ttl_fix variable controls a workaround
/*	for res_search(3) implementations that break the
/*	DNS_REQ_FLAG_NCACHE_TTL feature. The workaround does not
/*	support EDNS0 or DNSSEC, but it should be sufficient for
/*	DNSBL/DNSWL lookups.
/* INPUTS
/* .ad
/* .fi
/* .IP name
/*	The name to be looked up in the domain name system.
/*	This name must pass the valid_hostname() test; it
/*	must not be an IP address.
/* .IP type
/*	The resource record type to be looked up (T_A, T_MX etc.).
/* .IP rflags
/*	Resolver flags. These are a bitwise OR of:
/* .RS
/* .IP RES_DEBUG
/*	Print debugging information.
/* .IP RES_DNSRCH
/*	Search local domain and parent domains.
/* .IP RES_DEFNAMES
/*	Append local domain to unqualified names.
/* .IP RES_USE_DNSSEC
/*	Request DNSSEC validation. This flag is silently ignored
/*	when the system stub resolver API, resolver(3), does not
/*	implement DNSSEC.
/*	Automatically turns on the RES_TRUSTAD flag on systems that
/*	support this flag (this behavior will be more configurable
/*	in a later release).
/* .RE
/* .IP lflags
/*	Flags that control the operation of the dns_lookup*()
/*	functions.  DNS_REQ_FLAG_NONE requests no special processing.
/*	Otherwise, specify one or more of the following:
/* .RS
/* .IP DNS_REQ_FLAG_STOP_INVAL
/*	This flag is used by dns_lookup_l() and dns_lookup_v().
/*	Invoke dns_lookup() for the resource types in the order as
/*	specified, and return when dns_lookup() returns DNS_INVAL.
/* .IP DNS_REQ_FLAG_STOP_NULLMX
/*	This flag is used by dns_lookup_l() and dns_lookup_v().
/*	Invoke dns_lookup() for the resource types in the order as
/*	specified, and return when dns_lookup() returns DNS_NULLMX.
/* .IP DNS_REQ_FLAG_STOP_MX_POLICY
/*	This flag is used by dns_lookup_l() and dns_lookup_v().
/*	Invoke dns_lookup() for the resource types in the order as
/*	specified, and return when dns_lookup() returns DNS_POLICY
/*	for an MX query.
/* .IP DNS_REQ_FLAG_STOP_OK
/*	This flag is used by dns_lookup_l() and dns_lookup_v().
/*	Invoke dns_lookup() for the resource types in the order as
/*	specified, and return when dns_lookup() returns DNS_OK.
/* .IP DNS_REQ_FLAG_NCACHE_TTL
/*	When the lookup result status is DNS_NOTFOUND, return the
/*	SOA record(s) from the authority section in the reply, if
/*	available. The per-record reply TTL specifies how long the
/*	DNS_NOTFOUND answer is valid. The caller should pass the
/*	record(s) to dns_rr_free().
/*	Logs a warning if the RES_DNSRCH or RES_DEFNAMES resolver
/*	flags are set, and disables those flags.
/* .RE
/* .IP ltype
/*	The resource record types to be looked up. In the case of
/*	dns_lookup_l(), this is a null-terminated argument list.
/*	In the case of dns_lookup_v(), this is a null-terminated
/*	integer array.
/* OUTPUTS
/* .ad
/* .fi
/* .IP list
/*	A null pointer, or a pointer to a variable that receives a
/*	list of requested resource records.
/* .IP fqdn
/*	A null pointer, or storage for the fully-qualified domain
/*	name found for \fIname\fR.
/* .IP why
/*	A null pointer, or storage for the reason for failure.
/* .IP rcode
/*	Pointer to storage for the reply RCODE value. This gives
/*	more detailed information than DNS_FAIL, DNS_RETRY, etc.
/* DIAGNOSTICS
/*	If DNSSEC validation is requested but the response is not
/*	DNSSEC validated, dns_lookup() will send a one-time probe
/*	query as configured with the \fBdnssec_probe\fR configuration
/*	parameter, and will log a warning when the probe response
/*	was not DNSSEC validated.
/* .PP
/*	dns_lookup() returns one of the following codes and sets the
/*	\fIwhy\fR argument accordingly:
/* .IP DNS_OK
/*	The DNS query succeeded.
/* .IP DNS_POLICY
/*	The DNS query succeeded, but the answer did not pass the
/*	policy filter.
/* .IP DNS_NOTFOUND
/*	The DNS query succeeded; the requested information was not found.
/* .IP DNS_NULLMX
/*	The DNS query succeeded; the requested service is unavailable.
/*	This is returned when the list argument is not a null
/*	pointer, and an MX lookup result contains a null server
/*	name (so-called "nullmx" record).
/* .IP DNS_INVAL
/*	The DNS query succeeded; the result failed the valid_hostname() test.
/*
/*	NOTE: the valid_hostname() test is skipped for results that
/*	the caller suppresses explicitly.  For example, when the
/*	caller requests MX record lookup but specifies a null
/*	resource record list argument, no syntax check will be done
/*	for MX server names.
/* .IP DNS_RETRY
/*	The query failed, or the reply was malformed.
/*	The problem is considered transient.
/* .IP DNS_FAIL
/*	The query failed.
/* BUGS
/*	dns_lookup() implements a subset of all possible resource types:
/*	CNAME, MX, A, and some records with similar formatting requirements.
/*	It is unwise to specify the T_ANY wildcard resource type.
/*
/*	It takes a surprising amount of code to accomplish what appears
/*	to be a simple task. Later versions of the mail system may implement
/*	their own DNS client software.
/* SEE ALSO
/*	dns_rr(3) resource record memory and list management
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
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
/*	SRV Support by
/*	Tomas Korbar
/*	Red Hat, Inc.
/*--*/

/* System library. */

#include <sys_defs.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>

/* Utility library. */

#include <mymalloc.h>
#include <vstring.h>
#include <msg.h>
#include <valid_hostname.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>

/* DNS library. */

#define LIBDNS_INTERNAL
#include "dns.h"

/* Local stuff. */

 /*
  * Structure to keep track of things while decoding a name server reply.
  */
#define DEF_DNS_REPLY_SIZE	4096	/* in case we're using TCP */
#define MAX_DNS_REPLY_SIZE	65536	/* in case we're using TCP */
#define MAX_DNS_QUERY_SIZE	2048	/* XXX */

typedef struct DNS_REPLY {
    unsigned char *buf;			/* raw reply data */
    size_t  buf_len;			/* reply buffer length */
    int     rcode;			/* unfiltered reply code */
    int     dnssec_ad;			/* DNSSEC AD bit */
    int     query_count;		/* number of queries */
    int     answer_count;		/* number of answers */
    int     auth_count;			/* number of authority records */
    unsigned char *query_start;		/* start of query data */
    unsigned char *answer_start;	/* start of answer data */
    unsigned char *end;			/* first byte past reply */
} DNS_REPLY;

 /*
  * Test/set primitives to determine if the reply buffer contains a server
  * response. We use this when the caller requests DNS_REQ_FLAG_NCACHE_TTL,
  * and the DNS server replies that the requested record does not exist.
  */
#define TEST_HAVE_DNS_REPLY_PACKET(r)	((r)->end > (r)->buf)
#define SET_HAVE_DNS_REPLY_PACKET(r, l)	((r)->end = (r)->buf + (l))
#define SET_NO_DNS_REPLY_PACKET(r)	((r)->end = (r)->buf)

#define INET_ADDR_LEN	4		/* XXX */
#define INET6_ADDR_LEN	16		/* XXX */

 /*
  * Use the threadsafe resolver API if available, not because it is
  * theadsafe, but because it has more functionality.
  */
#ifdef USE_RES_NCALLS
static struct __res_state dns_res_state;

#define DNS_RES_NINIT		res_ninit
#define DNS_RES_NMKQUERY	res_nmkquery
#define DNS_RES_NSEARCH		res_nsearch
#define DNS_RES_NSEND		res_nsend
#define DNS_GET_H_ERRNO(statp)	((statp)->res_h_errno)

 /*
  * Alias new resolver API calls to the legacy resolver API which stores
  * resolver and error state in global variables.
  */
#else
#define dns_res_state		_res
#define DNS_RES_NINIT(statp)	res_init()
#define DNS_RES_NMKQUERY(statp, op, dname, class, type, data, datalen, \
		newrr, buf, buflen) \
	res_mkquery((op), (dname), (class), (type), (data), (datalen), \
		(newrr), (buf), (buflen))
#define DNS_RES_NSEARCH(statp, dname, class, type, answer, anslen) \
	res_search((dname), (class), (type), (answer), (anslen))
#define DNS_RES_NSEND(statp, msg, msglen, answer, anslen) \
	res_send((msg), (msglen), (answer), (anslen))
#define DNS_GET_H_ERRNO(statp)	(h_errno)
#endif

#ifdef USE_SET_H_ERRNO
#define DNS_SET_H_ERRNO(statp, err)	(set_h_errno(err))
#else
#define DNS_SET_H_ERRNO(statp, err)	(DNS_GET_H_ERRNO(statp) = (err))
#endif

 /*
  * To improve postscreen's allowlisting support, we need to know how long a
  * DNSBL "not found" answer is valid. The 2010 implementation assumed it was
  * valid for 3600 seconds. That is too long by 2015 standards.
  * 
  * Instead of guessing, Postfix 3.1 and later implement RFC 2308 (DNS NCACHE),
  * where a DNS server provides the TTL of a "not found" response as the TTL
  * of an SOA record in the authority section.
  * 
  * Unfortunately, the res_search() and res_query() API gets in the way. These
  * functions overload their result value, the server reply length, and
  * return -1 when the requested record does not exist. With libbind-based
  * implementations, the server response is still available in an application
  * buffer, thanks to the promise that res_query() and res_search() invoke
  * res_send(), which returns the full server response even if the requested
  * record does not exist.
  * 
  * If this promise is broken (for example, res_search() does not call
  * res_send(), but some non-libbind implementation that updates the
  * application buffer only when the requested record exists), then we have a
  * way out by setting the var_dns_ncache_ttl_fix variable. This enables a
  * limited res_query() clone that should be sufficient for DNSBL / DNSWL
  * lookups.
  * 
  * The libunbound API does not comingle the reply length and reply status
  * information, but that will have to wait until it is safe to make
  * libunbound a mandatory dependency for Postfix.
  */
#ifdef HAVE_RES_SEND

/* dns_neg_query - a res_query() clone that can return negative replies */

static int dns_neg_query(const char *name, int class, int type,
			         unsigned char *answer, int anslen)
{
    unsigned char msg_buf[MAX_DNS_QUERY_SIZE];
    HEADER *reply_header = (HEADER *) answer;
    int     len;

    /*
     * Differences with res_query() from libbind:
     * 
     * - This function returns a positive server reply length not only in case
     * of success, but in all cases where a server reply is available that
     * passes the preliminary checks in res_send().
     * 
     * - This function clears h_errno in case of success. The caller must use
     * h_errno instead of the return value to decide if the lookup was
     * successful.
     * 
     * - No support for EDNS0 and DNSSEC (including turning off EDNS0 after
     * error). That should be sufficient for DNS reputation lookups where the
     * reply contains a small number of IP addresses.  TXT records are out of
     * scope for this workaround.
     */
    reply_header->rcode = NOERROR;

#define NO_MKQUERY_DATA_BUF     ((unsigned char *) 0)
#define NO_MKQUERY_DATA_LEN     ((int) 0)
#define NO_MKQUERY_NEWRR        ((unsigned char *) 0)

    if ((len = DNS_RES_NMKQUERY(&dns_res_state,
			      QUERY, name, class, type, NO_MKQUERY_DATA_BUF,
				NO_MKQUERY_DATA_LEN, NO_MKQUERY_NEWRR,
				msg_buf, sizeof(msg_buf))) < 0) {
	DNS_SET_H_ERRNO(&dns_res_state, NO_RECOVERY);
	if (msg_verbose)
	    msg_info("res_nmkquery() failed");
	return (len);
    } else if ((len = DNS_RES_NSEND(&dns_res_state,
				    msg_buf, len, answer, anslen)) < 0) {
	DNS_SET_H_ERRNO(&dns_res_state, TRY_AGAIN);
	if (msg_verbose)
	    msg_info("res_nsend() failed");
	return (len);
    } else {
	switch (reply_header->rcode) {
	case NXDOMAIN:
	    DNS_SET_H_ERRNO(&dns_res_state, HOST_NOT_FOUND);
	    break;
	case NOERROR:
	    if (reply_header->ancount != 0)
		DNS_SET_H_ERRNO(&dns_res_state, 0);
	    else
		DNS_SET_H_ERRNO(&dns_res_state, NO_DATA);
	    break;
	case SERVFAIL:
	    DNS_SET_H_ERRNO(&dns_res_state, TRY_AGAIN);
	    break;
	default:
	    DNS_SET_H_ERRNO(&dns_res_state, NO_RECOVERY);
	    break;
	}
	return (len);
    }
}

#endif

/* dns_neg_search - res_search() that can return negative replies */

static int dns_neg_search(const char *name, int class, int type,
	               unsigned char *answer, int anslen, int keep_notfound)
{
    int     len;

    /*
     * Differences with res_search() from libbind:
     * 
     * - With a non-zero keep_notfound argument, this function returns a
     * positive server reply length not only in case of success, but also in
     * case of a "notfound" reply status. The keep_notfound argument is
     * usually zero, which allows us to avoid an unnecessary memset() call in
     * the most common use case.
     * 
     * - This function clears h_errno in case of success. The caller must use
     * h_errno instead of the return value to decide if a lookup was
     * successful.
     */
#define NOT_FOUND_H_ERRNO(he) ((he) == HOST_NOT_FOUND || (he) == NO_DATA)

    if (keep_notfound)
	/* Prepare for returning a null-padded server reply. */
	memset(answer, 0, anslen);
    len = DNS_RES_NSEARCH(&dns_res_state, name, class, type, answer, anslen);
    /* Begin API creep workaround. */
    if (len < 0 && DNS_GET_H_ERRNO(&dns_res_state) == 0) {
	DNS_SET_H_ERRNO(&dns_res_state, TRY_AGAIN);
	msg_warn("res_nsearch(state, \"%s\", %d, %d, %p, %d) returns %d"
		 " with h_errno==0 -- setting h_errno=TRY_AGAIN",
		 name, class, type, answer, anslen, len);
    }
    /* End API creep workaround. */
    if (len > 0) {
	DNS_SET_H_ERRNO(&dns_res_state, 0);
    } else if (keep_notfound
	       && NOT_FOUND_H_ERRNO(DNS_GET_H_ERRNO(&dns_res_state))) {
	/* Expect to return a null-padded server reply. */
	len = anslen;
    }
    return (len);
}

/* dns_query - query name server and pre-parse the reply */

static int dns_query(const char *name, int type, unsigned flags,
		             DNS_REPLY *reply, VSTRING *why, unsigned lflags)
{
    HEADER *reply_header;
    int     len;
    unsigned long saved_options;
    int     keep_notfound = (lflags & DNS_REQ_FLAG_NCACHE_TTL);

    /*
     * Initialize the reply buffer.
     */
    if (reply->buf == 0) {
	reply->buf = (unsigned char *) mymalloc(DEF_DNS_REPLY_SIZE);
	reply->buf_len = DEF_DNS_REPLY_SIZE;
    }

    /*
     * Initialize the name service.
     */
    if ((dns_res_state.options & RES_INIT) == 0
	&& DNS_RES_NINIT(&dns_res_state) < 0) {
	if (why)
	    vstring_strcpy(why, "Name service initialization failure");
	return (DNS_FAIL);
    }

    /*
     * Set search options: debugging, parent domain search, append local
     * domain. Do not allow the user to control other features.
     */
#define USER_FLAGS (RES_DEBUG | RES_DNSRCH | RES_DEFNAMES | RES_USE_DNSSEC)

    if ((flags & USER_FLAGS) != flags)
	msg_panic("dns_query: bad flags: %d", flags);

    /*
     * Set extra options that aren't exposed to the application.
     */
#define XTRA_FLAGS (RES_USE_EDNS0 | RES_TRUSTAD)

    if (DNS_WANT_DNSSEC_VALIDATION(flags))
	flags |= (RES_USE_EDNS0 | RES_TRUSTAD);

    /*
     * Can't append domains: we need the right SOA TTL.
     */
#define APPEND_DOMAIN_FLAGS (RES_DNSRCH | RES_DEFNAMES)

    if (keep_notfound && (flags & APPEND_DOMAIN_FLAGS)) {
	msg_warn("negative caching disables RES_DEFNAMES and RES_DNSRCH");
	flags &= ~APPEND_DOMAIN_FLAGS;
    }

    /*
     * Save and restore resolver options that we overwrite, to avoid
     * surprising behavior in other code that also invokes the resolver.
     */
#define SAVE_FLAGS (USER_FLAGS | XTRA_FLAGS)

    saved_options = (dns_res_state.options & SAVE_FLAGS);

    /*
     * Perform the lookup. Claim that the information cannot be found if and
     * only if the name server told us so.
     */
    for (;;) {
	dns_res_state.options &= ~saved_options;
	dns_res_state.options |= flags;
	if (keep_notfound && var_dns_ncache_ttl_fix) {
#ifdef HAVE_RES_SEND
	    len = dns_neg_query((char *) name, C_IN, type, reply->buf,
				reply->buf_len);
#else
	    var_dns_ncache_ttl_fix = 0;
	    msg_warn("system library does not support %s=yes"
		     " -- ignoring this setting", VAR_DNS_NCACHE_TTL_FIX);
	    len = dns_neg_search((char *) name, C_IN, type, reply->buf,
				 reply->buf_len, keep_notfound);
#endif
	} else {
	    len = dns_neg_search((char *) name, C_IN, type, reply->buf,
				 reply->buf_len, keep_notfound);
	}
	dns_res_state.options &= ~flags;
	dns_res_state.options |= saved_options;
	reply_header = (HEADER *) reply->buf;
	reply->rcode = reply_header->rcode;
	if ((reply->dnssec_ad = !!reply_header->ad) != 0)
	    DNS_SEC_STATS_SET(DNS_SEC_FLAG_AVAILABLE);
	if (DNS_GET_H_ERRNO(&dns_res_state) != 0) {
	    if (why)
		vstring_sprintf(why, "Host or domain name not found. "
				"Name service error for name=%s type=%s: %s",
				name, dns_strtype(type),
			     dns_strerror(DNS_GET_H_ERRNO(&dns_res_state)));
	    if (msg_verbose)
		msg_info("dns_query: %s (%s): %s",
			 name, dns_strtype(type),
			 dns_strerror(DNS_GET_H_ERRNO(&dns_res_state)));
	    switch (DNS_GET_H_ERRNO(&dns_res_state)) {
	    case NO_RECOVERY:
		return (DNS_FAIL);
	    case HOST_NOT_FOUND:
	    case NO_DATA:
		if (keep_notfound)
		    break;
		SET_NO_DNS_REPLY_PACKET(reply);
		return (DNS_NOTFOUND);
	    default:
		return (DNS_RETRY);
	    }
	} else {
	    if (msg_verbose)
		msg_info("dns_query: %s (%s): OK", name, dns_strtype(type));
	}

	if (reply_header->tc == 0 || reply->buf_len >= MAX_DNS_REPLY_SIZE)
	    break;
	reply->buf = (unsigned char *)
	    myrealloc((void *) reply->buf, 2 * reply->buf_len);
	reply->buf_len *= 2;
    }

    /*
     * Future proofing. If this reaches the panic call, then some code change
     * introduced a bug.
     */
    if (len < 0)
	msg_panic("dns_query: bad length %d (h_errno=%s)",
		  len, dns_strerror(DNS_GET_H_ERRNO(&dns_res_state)));

    /*
     * Paranoia.
     */
    if (len > reply->buf_len) {
	msg_warn("reply length %d > buffer length %d for name=%s type=%s",
		 len, (int) reply->buf_len, name, dns_strtype(type));
	len = reply->buf_len;
    }

    /*
     * Initialize the reply structure. Some structure members are filled on
     * the fly while the reply is being parsed.
     */
    SET_HAVE_DNS_REPLY_PACKET(reply, len);
    reply->query_start = reply->buf + sizeof(HEADER);
    reply->answer_start = 0;
    reply->query_count = ntohs(reply_header->qdcount);
    reply->answer_count = ntohs(reply_header->ancount);
    reply->auth_count = ntohs(reply_header->nscount);
    if (msg_verbose > 1)
	msg_info("dns_query: reply len=%d ancount=%d nscount=%d",
		 len, reply->answer_count, reply->auth_count);

    /*
     * Future proofing. If this reaches the panic call, then some code change
     * introduced a bug.
     */
    if (DNS_GET_H_ERRNO(&dns_res_state) == 0) {
	return (DNS_OK);
    } else if (keep_notfound) {
	return (DNS_NOTFOUND);
    } else {
	msg_panic("dns_query: unexpected reply status: %s",
		  dns_strerror(DNS_GET_H_ERRNO(&dns_res_state)));
    }
}

/* dns_skip_query - skip query data in name server reply */

static int dns_skip_query(DNS_REPLY *reply)
{
    int     query_count = reply->query_count;
    unsigned char *pos = reply->query_start;
    int     len;

    /*
     * For each query, skip over the domain name and over the fixed query
     * data.
     */
    while (query_count-- > 0) {
	if (pos >= reply->end)
	    return DNS_RETRY;
	len = dn_skipname(pos, reply->end);
	if (len < 0)
	    return (DNS_RETRY);
	pos += len + QFIXEDSZ;
    }
    reply->answer_start = pos;
    return (DNS_OK);
}

/* dns_get_fixed - extract fixed data from resource record */

static int dns_get_fixed(unsigned char *pos, DNS_FIXED *fixed)
{
    GETSHORT(fixed->type, pos);
    GETSHORT(fixed->class, pos);
    GETLONG(fixed->ttl, pos);
    GETSHORT(fixed->length, pos);

    if (fixed->class != C_IN) {
	msg_warn("dns_get_fixed: bad class: %u", fixed->class);
	return (DNS_RETRY);
    }
    return (DNS_OK);
}

/* valid_rr_name - validate hostname in resource record */

static int valid_rr_name(const char *name, const char *location,
			         unsigned type, DNS_REPLY *reply)
{
    char    temp[DNS_NAME_LEN];
    char   *query_name;
    int     len;
    char   *gripe;
    int     result;

    /*
     * People aren't supposed to specify numeric names where domain names are
     * required, but it "works" with some mailers anyway, so people complain
     * when software doesn't bend over backwards.
     */
#define PASS_NAME	1
#define REJECT_NAME	0

    if (valid_hostaddr(name, DONT_GRIPE)) {
	result = PASS_NAME;
	gripe = "numeric domain name";
    } else if (!valid_hostname(name, DO_GRIPE | DO_WILDCARD)) {
	result = REJECT_NAME;
	gripe = "malformed domain name";
    } else {
	result = PASS_NAME;
	gripe = 0;
    }

    /*
     * If we have a gripe, show some context, including the name used in the
     * query and the type of reply that we're looking at.
     */
    if (gripe) {
	len = dn_expand(reply->buf, reply->end, reply->query_start,
			temp, DNS_NAME_LEN);
	query_name = (len < 0 ? "*unparsable*" : temp);
	msg_warn("%s in %s of %s record for %s: %.100s",
		 gripe, location, dns_strtype(type), query_name, name);
    }
    return (result);
}

/* dns_get_rr - extract resource record from name server reply */

static int dns_get_rr(DNS_RR **list, const char *orig_name, DNS_REPLY *reply,
		              unsigned char *pos, char *rr_name,
		              DNS_FIXED *fixed)
{
    char    temp[DNS_NAME_LEN];
    char   *tempbuf = temp;
    UINT32_TYPE soa_buf[5];
    int     comp_len;
    ssize_t data_len;
    unsigned pref = 0;
    unsigned weight = 0;
    unsigned port = 0;
    unsigned char *src;
    unsigned char *dst;
    int     ch;

#define MIN2(a, b)	((unsigned)(a) < (unsigned)(b) ? (a) : (b))

    *list = 0;

    switch (fixed->type) {
    default:
	msg_panic("dns_get_rr: don't know how to extract resource type %s",
		  dns_strtype(fixed->type));
    case T_CNAME:
    case T_DNAME:
    case T_MB:
    case T_MG:
    case T_MR:
    case T_NS:
    case T_PTR:
	if (dn_expand(reply->buf, reply->end, pos, temp, sizeof(temp)) < 0)
	    return (DNS_RETRY);
	if (!valid_rr_name(temp, "resource data", fixed->type, reply))
	    return (DNS_INVAL);
	data_len = strlen(temp) + 1;
	break;
    case T_SRV:
	GETSHORT(pref, pos);
	GETSHORT(weight, pos);
	GETSHORT(port, pos);
	if (dn_expand(reply->buf, reply->end, pos, temp, sizeof(temp)) < 0)
	    return (DNS_RETRY);
	if (*temp == 0)
	    return (DNS_NULLSRV);
	if (!valid_rr_name(temp, "resource data", fixed->type, reply))
	    return (DNS_INVAL);
	data_len = strlen(temp) + 1;
	break;
    case T_MX:
	GETSHORT(pref, pos);
	if (dn_expand(reply->buf, reply->end, pos, temp, sizeof(temp)) < 0)
	    return (DNS_RETRY);
	/* Don't even think of returning an invalid hostname to the caller. */
	if (*temp == 0)
	    return (DNS_NULLMX);		/* TODO: descriptive text */
	if (!valid_rr_name(temp, "resource data", fixed->type, reply))
	    return (DNS_INVAL);
	data_len = strlen(temp) + 1;
	break;
    case T_A:
	if (fixed->length != INET_ADDR_LEN) {
	    msg_warn("extract_answer: bad address length: %d", fixed->length);
	    return (DNS_RETRY);
	}
	if (fixed->length > sizeof(temp))
	    msg_panic("dns_get_rr: length %d > DNS_NAME_LEN",
		      fixed->length);
	memcpy(temp, pos, fixed->length);
	data_len = fixed->length;
	break;
#ifdef T_AAAA
    case T_AAAA:
	if (fixed->length != INET6_ADDR_LEN) {
	    msg_warn("extract_answer: bad address length: %d", fixed->length);
	    return (DNS_RETRY);
	}
	if (fixed->length > sizeof(temp))
	    msg_panic("dns_get_rr: length %d > DNS_NAME_LEN",
		      fixed->length);
	memcpy(temp, pos, fixed->length);
	data_len = fixed->length;
	break;
#endif

	/*
	 * We impose the same length limit here as for DNS names. However,
	 * see T_TLSA discussion below.
	 */
    case T_TXT:
	data_len = MIN2(pos[0] + 1, MIN2(fixed->length + 1, sizeof(temp)));
	for (src = pos + 1, dst = (unsigned char *) (temp);
	     dst < (unsigned char *) (temp) + data_len - 1; /* */ ) {
	    ch = *src++;
	    *dst++ = (ISPRINT(ch) ? ch : ' ');
	}
	*dst = 0;
	break;

	/*
	 * For a full certificate, fixed->length may be longer than
	 * sizeof(tmpbuf) == DNS_NAME_LEN.  Since we don't need a decode
	 * buffer, just copy the raw data into the rr.
	 * 
	 * XXX Reject replies with bogus length < 3.
	 * 
	 * XXX What about enforcing a sane upper bound? The RFC 1035 hard
	 * protocol limit is the RRDATA length limit of 65535.
	 */
    case T_TLSA:
	data_len = fixed->length;
	tempbuf = (char *) pos;
	break;

	/*
	 * We use the SOA record TTL to determine the negative reply TTL. We
	 * save the time fields in the SOA record for debugging, but for now
	 * we don't bother saving the source host and mailbox information, as
	 * that would require changes to the DNS_RR structure and APIs. See
	 * also code in dns_strrecord().
	 */
    case T_SOA:
	comp_len = dn_skipname(pos, reply->end);
	if (comp_len < 0)
	    return (DNS_RETRY);
	pos += comp_len;
	comp_len = dn_skipname(pos, reply->end);
	if (comp_len < 0)
	    return (DNS_RETRY);
	pos += comp_len;
	if (reply->end - pos < sizeof(soa_buf)) {
	    msg_warn("extract_answer: bad SOA length: %d", fixed->length);
	    return (DNS_RETRY);
	}
	GETLONG(soa_buf[0], pos);		/* Serial */
	GETLONG(soa_buf[1], pos);		/* Refresh */
	GETLONG(soa_buf[2], pos);		/* Retry */
	GETLONG(soa_buf[3], pos);		/* Expire */
	GETLONG(soa_buf[4], pos);		/* Ncache TTL */
	tempbuf = (char *) soa_buf;
	data_len = sizeof(soa_buf);
	break;
    }
    *list = dns_rr_create(orig_name, rr_name, fixed->type, fixed->class,
			  fixed->ttl, pref, weight, port, tempbuf, data_len);
    return (DNS_OK);
}

/* dns_get_alias - extract CNAME from name server reply */

static int dns_get_alias(DNS_REPLY *reply, unsigned char *pos,
			         DNS_FIXED *fixed, char *cname, int c_len)
{
    if (fixed->type != T_CNAME)
	msg_panic("dns_get_alias: bad type %s", dns_strtype(fixed->type));
    if (dn_expand(reply->buf, reply->end, pos, cname, c_len) < 0)
	return (DNS_RETRY);
    if (!valid_rr_name(cname, "resource data", fixed->type, reply))
	return (DNS_INVAL);
    return (DNS_OK);
}

/* dns_get_answer - extract answers from name server reply */

static int dns_get_answer(const char *orig_name, DNS_REPLY *reply, int type,
	             DNS_RR **rrlist, VSTRING *fqdn, char *cname, int c_len,
			          int *maybe_secure)
{
    char    rr_name[DNS_NAME_LEN];
    unsigned char *pos;
    int     answer_count = reply->answer_count;
    int     len;
    DNS_FIXED fixed;
    DNS_RR *rr;
    int     resource_found = 0;
    int     cname_found = 0;
    int     not_found_status = DNS_NOTFOUND;	/* can't happen */
    int     status;

    /*
     * Initialize. Skip over the name server query if we haven't yet.
     */
    if (reply->answer_start == 0)
	if ((status = dns_skip_query(reply)) < 0)
	    return (status);
    pos = reply->answer_start;

    /*
     * Either this, or use a GOTO for emergency exits. The purpose is to
     * prevent incomplete answers from being passed back to the caller.
     */
#define CORRUPT(status) { \
	if (rrlist && *rrlist) { \
	    dns_rr_free(*rrlist); \
	    *rrlist = 0; \
	} \
	return (status); \
    }

    /*
     * Iterate over all answers.
     */
    while (answer_count-- > 0) {

	/*
	 * Optionally extract the fully-qualified domain name.
	 */
	if (pos >= reply->end)
	    CORRUPT(DNS_RETRY);
	len = dn_expand(reply->buf, reply->end, pos, rr_name, DNS_NAME_LEN);
	if (len < 0)
	    CORRUPT(DNS_RETRY);
	pos += len;

	/*
	 * Extract the fixed reply data: type, class, ttl, length.
	 */
	if (pos + RRFIXEDSZ > reply->end)
	    CORRUPT(DNS_RETRY);
	if ((status = dns_get_fixed(pos, &fixed)) != DNS_OK)
	    CORRUPT(status);
	if (strcmp(orig_name, ".") == 0 && *rr_name == 0)
	     /* Allow empty response name for root queries. */ ;
	else if (!valid_rr_name(rr_name, "resource name", fixed.type, reply))
	    CORRUPT(DNS_INVAL);
	if (fqdn)
	    vstring_strcpy(fqdn, rr_name);
	if (msg_verbose)
	    msg_info("dns_get_answer: type %s for %s",
		     dns_strtype(fixed.type), rr_name);
	pos += RRFIXEDSZ;

	/*
	 * Optionally extract the requested resource or CNAME data.
	 */
	if (pos + fixed.length > reply->end)
	    CORRUPT(DNS_RETRY);
	if (type == fixed.type || type == T_ANY) {	/* requested type */
	    if (rrlist) {
		if ((status = dns_get_rr(&rr, orig_name, reply, pos, rr_name,
					 &fixed)) == DNS_OK) {
		    resource_found++;
		    rr->dnssec_valid = *maybe_secure ? reply->dnssec_ad : 0;
		    *rrlist = dns_rr_append(*rrlist, rr);
		} else if (status == DNS_NULLMX || status == DNS_NULLSRV) {
		    CORRUPT(status);		/* TODO: use better name */
		} else if (not_found_status != DNS_RETRY)
		    not_found_status = status;
	    } else
		resource_found++;
	} else if (fixed.type == T_CNAME) {	/* cname resource */
	    cname_found++;
	    if (cname && c_len > 0)
		if ((status = dns_get_alias(reply, pos, &fixed, cname, c_len)) != DNS_OK)
		    CORRUPT(status);
	    if (!reply->dnssec_ad)
		*maybe_secure = 0;
	}
	pos += fixed.length;
    }

    /*
     * See what answer we came up with. Report success when the requested
     * information was found. Otherwise, when a CNAME was found, report that
     * more recursion is needed. Otherwise report failure.
     */
    if (resource_found)
	return (DNS_OK);
    if (cname_found)
	return (DNS_RECURSE);
    return (not_found_status);
}

/* dns_lookup_x - DNS lookup user interface */

int     dns_lookup_x(const char *name, unsigned type, unsigned flags,
		             DNS_RR **rrlist, VSTRING *fqdn, VSTRING *why,
		             int *rcode, unsigned lflags)
{
    char    cname[DNS_NAME_LEN];
    int     c_len = sizeof(cname);
    static DNS_REPLY reply;
    int     count;
    int     status;
    int     maybe_secure = 1;		/* Query name presumed secure */
    const char *orig_name = name;

    /*
     * Reset results early. DNS_OK is not the only status that returns
     * resource records; DNS_NOTFOUND will do that too, if requested.
     */
    if (rrlist)
	*rrlist = 0;

    /*
     * DJBDNS produces a bogus A record when given a numerical hostname.
     */
    if (valid_hostaddr(name, DONT_GRIPE)) {
	if (why)
	    vstring_sprintf(why,
		   "Name service error for %s: invalid host or domain name",
			    name);
	if (rcode)
	    *rcode = NXDOMAIN;
	DNS_SET_H_ERRNO(&dns_res_state, HOST_NOT_FOUND);
	return (DNS_NOTFOUND);
    }

    /*
     * The Linux resolver misbehaves when given an invalid domain name.
     */
    if (strcmp(name, ".") && !valid_hostname(name, DONT_GRIPE | DO_WILDCARD)) {
	if (why)
	    vstring_sprintf(why,
		   "Name service error for %s: invalid host or domain name",
			    name);
	if (rcode)
	    *rcode = NXDOMAIN;
	DNS_SET_H_ERRNO(&dns_res_state, HOST_NOT_FOUND);
	return (DNS_NOTFOUND);
    }

    /*
     * Perform the lookup. Follow CNAME chains, but only up to a
     * pre-determined maximum.
     */
    for (count = 0; count < 10; count++) {

	/*
	 * Perform the DNS lookup, and pre-parse the name server reply.
	 */
	status = dns_query(name, type, flags, &reply, why, lflags);
	if (rcode)
	    *rcode = reply.rcode;
	if (status != DNS_OK) {

	    /*
	     * If the record does not exist, and we have a copy of the server
	     * response, try to extract the negative caching TTL for the SOA
	     * record in the authority section. DO NOT return an error if an
	     * SOA record is malformed.
	     */
	    if (status == DNS_NOTFOUND && TEST_HAVE_DNS_REPLY_PACKET(&reply)
		&& reply.auth_count > 0) {
		reply.answer_count = reply.auth_count;	/* XXX TODO: Fix API */
		(void) dns_get_answer(orig_name, &reply, T_SOA, rrlist, fqdn,
				      cname, c_len, &maybe_secure);
	    }
	    if (DNS_WANT_DNSSEC_VALIDATION(flags)
		&& !DNS_SEC_STATS_TEST(DNS_SEC_FLAG_AVAILABLE | \
				       DNS_SEC_FLAG_DONT_PROBE))
		dns_sec_probe(flags);		/* XXX Clobbers 'reply' */
	    return (status);
	}

	/*
	 * Extract resource records of the requested type. Pick up CNAME
	 * information just in case the requested data is not found.
	 */
	status = dns_get_answer(orig_name, &reply, type, rrlist, fqdn,
				cname, c_len, &maybe_secure);
	if (DNS_WANT_DNSSEC_VALIDATION(flags)
	    && !DNS_SEC_STATS_TEST(DNS_SEC_FLAG_AVAILABLE | \
				   DNS_SEC_FLAG_DONT_PROBE))
	    dns_sec_probe(flags);		/* XXX Clobbers 'reply' */
	switch (status) {
	default:
	    if (why)
		vstring_sprintf(why, "Name service error for name=%s type=%s: "
				"Malformed or unexpected name server reply",
				name, dns_strtype(type));
	    return (status);
	case DNS_NULLMX:
	    if (why)
		vstring_sprintf(why, "Domain %s does not accept mail (nullMX)",
				name);
	    DNS_SET_H_ERRNO(&dns_res_state, NO_DATA);
	    return (status);
	case DNS_NULLSRV:
	    if (why)
		vstring_sprintf(why, "Domain %s does not support SRV requests",
				name);
	    DNS_SET_H_ERRNO(&dns_res_state, NO_DATA);
	    return (status);
	case DNS_OK:
	    if (rrlist && dns_rr_filter_maps) {
		if (dns_rr_filter_execute(rrlist) < 0) {
		    if (why)
			vstring_sprintf(why,
					"Error looking up name=%s type=%s: "
					"Invalid DNS reply filter syntax",
					name, dns_strtype(type));
		    dns_rr_free(*rrlist);
		    *rrlist = 0;
		    status = DNS_RETRY;
		} else if (*rrlist == 0) {
		    if (why)
			vstring_sprintf(why,
					"Error looking up name=%s type=%s: "
					"DNS reply filter drops all results",
					name, dns_strtype(type));
		    status = DNS_POLICY;
		}
	    }
	    return (status);
	case DNS_RECURSE:
	    if (msg_verbose)
		msg_info("dns_lookup: %s aliased to %s", name, cname);
#if RES_USE_DNSSEC

	    /*
	     * Once an intermediate CNAME reply is not validated, all
	     * consequent RRs are deemed not validated, so we don't ask for
	     * further DNSSEC replies.
	     */
	    if (maybe_secure == 0)
		flags &= ~RES_USE_DNSSEC;
#endif
	    name = cname;
	}
    }
    if (why)
	vstring_sprintf(why, "Name server loop for %s", name);
    msg_warn("dns_lookup: Name server loop for %s", name);
    return (DNS_NOTFOUND);
}

/* dns_lookup_rl - DNS lookup interface with types list */

int     dns_lookup_rl(const char *name, unsigned flags, DNS_RR **rrlist,
		              VSTRING *fqdn, VSTRING *why, int *rcode,
		              int lflags,...)
{
    va_list ap;
    unsigned type, next;
    int     status = DNS_NOTFOUND;
    int     hpref_status = INT_MIN;
    VSTRING *hpref_rtext = 0;
    int     hpref_rcode;
    int     hpref_h_errno;
    DNS_RR *rr;

    /* Save intermediate highest-priority result. */
#define SAVE_HPREF_STATUS() do { \
	hpref_status = status; \
	if (rcode) \
	    hpref_rcode = *rcode; \
	if (why && status != DNS_OK) \
	    vstring_strcpy(hpref_rtext ? hpref_rtext : \
			   (hpref_rtext = vstring_alloc(VSTRING_LEN(why))), \
			   vstring_str(why)); \
	hpref_h_errno = DNS_GET_H_ERRNO(&dns_res_state); \
    } while (0)

    /* Restore intermediate highest-priority result. */
#define RESTORE_HPREF_STATUS() do { \
	status = hpref_status; \
	if (rcode) \
	    *rcode = hpref_rcode; \
	if (why && status != DNS_OK) \
	    vstring_strcpy(why, vstring_str(hpref_rtext)); \
	DNS_SET_H_ERRNO(&dns_res_state, hpref_h_errno); \
    } while (0)

    if (rrlist)
	*rrlist = 0;
    va_start(ap, lflags);
    for (type = va_arg(ap, unsigned); type != 0; type = next) {
	next = va_arg(ap, unsigned);
	if (msg_verbose)
	    msg_info("lookup %s type %s flags %s",
		     name, dns_strtype(type), dns_str_resflags(flags));
	status = dns_lookup_x(name, type, flags, rrlist ? &rr : (DNS_RR **) 0,
			      fqdn, why, rcode, lflags);
	if (rrlist && rr)
	    *rrlist = dns_rr_append(*rrlist, rr);
	if (status == DNS_OK) {
	    if (lflags & DNS_REQ_FLAG_STOP_OK)
		break;
	} else if (status == DNS_INVAL) {
	    if (lflags & DNS_REQ_FLAG_STOP_INVAL)
		break;
	} else if (status == DNS_POLICY) {
	    if (type == T_MX && (lflags & DNS_REQ_FLAG_STOP_MX_POLICY))
		break;
	} else if (status == DNS_NULLMX) {
	    if (lflags & DNS_REQ_FLAG_STOP_NULLMX)
		break;
	}
	/* XXX Stop after NXDOMAIN error. */
	if (next == 0)
	    break;
	if (status >= hpref_status)
	    SAVE_HPREF_STATUS();		/* save last info */
    }
    va_end(ap);
    if (status < hpref_status)
	RESTORE_HPREF_STATUS();			/* else report last info */
    if (hpref_rtext)
	vstring_free(hpref_rtext);
    return (status);
}

/* dns_lookup_rv - DNS lookup interface with types vector */

int     dns_lookup_rv(const char *name, unsigned flags, DNS_RR **rrlist,
		              VSTRING *fqdn, VSTRING *why, int *rcode,
		              int lflags, unsigned *types)
{
    unsigned type, next;
    int     status = DNS_NOTFOUND;
    int     hpref_status = INT_MIN;
    VSTRING *hpref_rtext = 0;
    int     hpref_rcode;
    int     hpref_h_errno;
    DNS_RR *rr;

    if (rrlist)
	*rrlist = 0;
    for (type = *types++; type != 0; type = next) {
	next = *types++;
	if (msg_verbose)
	    msg_info("lookup %s type %s flags %s",
		     name, dns_strtype(type), dns_str_resflags(flags));
	status = dns_lookup_x(name, type, flags, rrlist ? &rr : (DNS_RR **) 0,
			      fqdn, why, rcode, lflags);
	if (rrlist && rr)
	    *rrlist = dns_rr_append(*rrlist, rr);
	if (status == DNS_OK) {
	    if (lflags & DNS_REQ_FLAG_STOP_OK)
		break;
	} else if (status == DNS_INVAL) {
	    if (lflags & DNS_REQ_FLAG_STOP_INVAL)
		break;
	} else if (status == DNS_POLICY) {
	    if (type == T_MX && (lflags & DNS_REQ_FLAG_STOP_MX_POLICY))
		break;
	} else if (status == DNS_NULLMX) {
	    if (lflags & DNS_REQ_FLAG_STOP_NULLMX)
		break;
	}
	/* XXX Stop after NXDOMAIN error. */
	if (next == 0)
	    break;
	if (status >= hpref_status)
	    SAVE_HPREF_STATUS();		/* save last info */
    }
    if (status < hpref_status)
	RESTORE_HPREF_STATUS();			/* else report last info */
    if (hpref_rtext)
	vstring_free(hpref_rtext);
    return (status);
}

/* dns_get_h_errno - get the last lookup status */

int     dns_get_h_errno(void)
{
    return (DNS_GET_H_ERRNO(&dns_res_state));
}
