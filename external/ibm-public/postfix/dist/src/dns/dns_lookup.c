/*	$NetBSD: dns_lookup.c,v 1.2.8.1 2014/08/10 07:12:48 tls Exp $	*/

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
/* AUXILIARY FUNCTIONS
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
/* DESCRIPTION
/*	dns_lookup() looks up DNS resource records. When requested to
/*	look up data other than type CNAME, it will follow a limited
/*	number of CNAME indirections. All result names (including
/*	null terminator) will fit a buffer of size DNS_NAME_LEN.
/*	All name results are validated by \fIvalid_hostname\fR();
/*	an invalid name is reported as a DNS_INVAL result, while
/*	malformed replies are reported as transient errors.
/*
/*	dns_lookup_l() and dns_lookup_v() allow the user to specify
/*	a list of resource types.
/*
/*	dns_lookup_r(), dns_lookup_rl() and dns_lookup_rv() provide
/*	additional information.
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
/* .RE
/* .IP lflags
/*	Multi-type request control for dns_lookup_l() and dns_lookup_v().
/*	For convenience, DNS_REQ_FLAG_NONE requests no special
/*	processing. Invoke dns_lookup() for all specified resource
/*	record types in the specified order, and merge their results.
/*	Otherwise, specify one or more of the following:
/* .RS
/* .IP DNS_REQ_FLAG_STOP_INVAL
/*	Invoke dns_lookup() for the resource types in the order as
/*	specified, and return when dns_lookup() returns DNS_INVAL.
/* .IP DNS_REQ_FLAG_STOP_OK
/*	Invoke dns_lookup() for the resource types in the order as
/*	specified, and return when dns_lookup() returns DNS_OK.
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
/*	dns_lookup() returns one of the following codes and sets the
/*	\fIwhy\fR argument accordingly:
/* .IP DNS_OK
/*	The DNS query succeeded.
/* .IP DNS_NOTFOUND
/*	The DNS query succeeded; the requested information was not found.
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

/* DNS library. */

#include "dns.h"

/* Local stuff. */

 /*
  * Structure to keep track of things while decoding a name server reply.
  */
#define DEF_DNS_REPLY_SIZE	4096	/* in case we're using TCP */
#define MAX_DNS_REPLY_SIZE	65536	/* in case we're using TCP */

typedef struct DNS_REPLY {
    unsigned char *buf;			/* raw reply data */
    size_t  buf_len;			/* reply buffer length */
    int     rcode;			/* unfiltered reply code */
    int     dnssec_ad;			/* DNSSEC AD bit */
    int     query_count;		/* number of queries */
    int     answer_count;		/* number of answers */
    unsigned char *query_start;		/* start of query data */
    unsigned char *answer_start;	/* start of answer data */
    unsigned char *end;			/* first byte past reply */
} DNS_REPLY;

#define INET_ADDR_LEN	4		/* XXX */
#define INET6_ADDR_LEN	16		/* XXX */

/* dns_query - query name server and pre-parse the reply */

#if __RES < 20030124

static int
res_ninit(res_state res)
{
	int error;

	if ((error = res_init()) < 0)
		return error;

	*res = _res;
	return error;
}

static int
res_nsearch(res_state statp, const char *dname, int class, int type,
    u_char *answer, int anslen)
{
	return res_search(dname, class, type, answer, anslen);
}

#endif

static int dns_query(const char *name, int type, int flags,
		             DNS_REPLY *reply, VSTRING *why)
{
    HEADER *reply_header;
    int     len;
    unsigned long saved_options;
    /* For efficiency, we are not called from multiple threads */
    static struct __res_state res;

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
    if ((res.options & RES_INIT) == 0 && res_ninit(&res) < 0) {
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
#define XTRA_FLAGS (RES_USE_EDNS0)

    if (flags & RES_USE_DNSSEC)
	flags |= RES_USE_EDNS0;

    /*
     * Save and restore resolver options that we overwrite, to avoid
     * surprising behavior in other code that also invokes the resolver.
     */
#define SAVE_FLAGS (USER_FLAGS | XTRA_FLAGS)

    saved_options = (res.options & SAVE_FLAGS);

    /*
     * Perform the lookup. Claim that the information cannot be found if and
     * only if the name server told us so.
     */
    for (;;) {
	res.options &= ~saved_options;
	res.options |= flags;
	len = res_nsearch(&res, name, C_IN, type, reply->buf, reply->buf_len);
	res.options &= ~flags;
	res.options |= saved_options;
	reply_header = (HEADER *) reply->buf;
	reply->rcode = reply_header->rcode;
	if (len < 0) {
	    if (why)
		vstring_sprintf(why, "Host or domain name not found. "
				"Name service error for name=%s type=%s: %s",
			    name, dns_strtype(type), dns_strerror(h_errno));
	    if (msg_verbose)
		msg_info("dns_query: %s (%s): %s",
			 name, dns_strtype(type), dns_strerror(h_errno));
	    switch (h_errno) {
	    case NO_RECOVERY:
		return (DNS_FAIL);
	    case HOST_NOT_FOUND:
	    case NO_DATA:
		return (DNS_NOTFOUND);
	    default:
		return (DNS_RETRY);
	    }
	}
	if (msg_verbose)
	    msg_info("dns_query: %s (%s): OK", name, dns_strtype(type));

	if (reply_header->tc == 0 || reply->buf_len >= MAX_DNS_REPLY_SIZE)
	    break;
	reply->buf = (unsigned char *)
	    myrealloc((char *) reply->buf, 2 * reply->buf_len);
	reply->buf_len *= 2;
    }

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
     * the fly while the reply is being parsed.  Coerce AD bit to boolean.
     */
#if RES_USE_DNSSEC != 0
    reply->dnssec_ad = (flags & RES_USE_DNSSEC) ? !!reply_header->ad : 0;
#else
    reply->dnssec_ad = 0;
#endif
    reply->end = reply->buf + len;
    reply->query_start = reply->buf + sizeof(HEADER);
    reply->answer_start = 0;
    reply->query_count = ntohs(reply_header->qdcount);
    reply->answer_count = ntohs(reply_header->ancount);
    return (DNS_OK);
}

/* dns_skip_query - skip query data in name server reply */

static int dns_skip_query(DNS_REPLY *reply)
{
    int     query_count = reply->query_count;
    unsigned char *pos = reply->query_start;
    char    temp[DNS_NAME_LEN];
    int     len;

    /*
     * For each query, skip over the domain name and over the fixed query
     * data.
     */
    while (query_count-- > 0) {
	if (pos >= reply->end)
	    return DNS_RETRY;
	len = dn_expand(reply->buf, reply->end, pos, temp, DNS_NAME_LEN);
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
    } else if (!valid_hostname(name, DO_GRIPE)) {
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
    ssize_t data_len;
    unsigned pref = 0;
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
    case T_MX:
	GETSHORT(pref, pos);
	if (dn_expand(reply->buf, reply->end, pos, temp, sizeof(temp)) < 0)
	    return (DNS_RETRY);
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
    }
    *list = dns_rr_create(orig_name, rr_name, fixed->type, fixed->class,
			  fixed->ttl, pref, tempbuf, data_len);
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
    if (rrlist)
	*rrlist = 0;

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
	if (!valid_rr_name(rr_name, "resource name", fixed.type, reply))
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

/* dns_lookup_r - DNS lookup user interface */

int     dns_lookup_r(const char *name, unsigned type, unsigned flags,
		             DNS_RR **rrlist, VSTRING *fqdn, VSTRING *why,
		             int *rcode)
{
    char    cname[DNS_NAME_LEN];
    int     c_len = sizeof(cname);
    static DNS_REPLY reply;
    int     count;
    int     status;
    int     maybe_secure = 1;		/* Query name presumed secure */
    const char *orig_name = name;

    /*
     * DJBDNS produces a bogus A record when given a numerical hostname.
     */
    if (valid_hostaddr(name, DONT_GRIPE)) {
	if (why)
	    vstring_sprintf(why,
		   "Name service error for %s: invalid host or domain name",
			    name);
	SET_H_ERRNO(HOST_NOT_FOUND);
	return (DNS_NOTFOUND);
    }

    /*
     * The Linux resolver misbehaves when given an invalid domain name.
     */
    if (!valid_hostname(name, DONT_GRIPE)) {
	if (why)
	    vstring_sprintf(why,
		   "Name service error for %s: invalid host or domain name",
			    name);
	SET_H_ERRNO(HOST_NOT_FOUND);
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
	status = dns_query(name, type, flags, &reply, why);
	if (rcode)
	    *rcode = reply.rcode;
	if (status != DNS_OK)
	    return (status);

	/*
	 * Extract resource records of the requested type. Pick up CNAME
	 * information just in case the requested data is not found.
	 */
	status = dns_get_answer(orig_name, &reply, type, rrlist, fqdn,
				cname, c_len, &maybe_secure);
	switch (status) {
	default:
	    if (why)
		vstring_sprintf(why, "Name service error for name=%s type=%s: "
				"Malformed or unexpected name server reply",
				name, dns_strtype(type));
	    /* FALLTHROUGH */
	case DNS_OK:
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
    unsigned type;
    int     status = DNS_NOTFOUND;
    DNS_RR *rr;
    int     non_err = 0;
    int     soft_err = 0;

    if (rrlist)
	*rrlist = 0;
    va_start(ap, lflags);
    while ((type = va_arg(ap, unsigned)) != 0) {
	if (msg_verbose)
	    msg_info("lookup %s type %s flags %d",
		     name, dns_strtype(type), flags);
	status = dns_lookup_r(name, type, flags, rrlist ? &rr : (DNS_RR **) 0,
			      fqdn, why, rcode);
	if (status == DNS_OK) {
	    non_err = 1;
	    if (rrlist)
		*rrlist = dns_rr_append(*rrlist, rr);
	    if (lflags & DNS_REQ_FLAG_STOP_OK)
		break;
	} else if (status == DNS_INVAL) {
	    if (lflags & DNS_REQ_FLAG_STOP_INVAL)
		break;
	} else if (status == DNS_RETRY) {
	    soft_err = 1;
	}
	/* XXX Stop after NXDOMAIN error. */
    }
    va_end(ap);
    return (non_err ? DNS_OK : soft_err ? DNS_RETRY : status);
}

/* dns_lookup_rv - DNS lookup interface with types vector */

int     dns_lookup_rv(const char *name, unsigned flags, DNS_RR **rrlist,
		              VSTRING *fqdn, VSTRING *why, int *rcode,
		              int lflags, unsigned *types)
{
    unsigned type;
    int     status = DNS_NOTFOUND;
    DNS_RR *rr;
    int     non_err = 0;
    int     soft_err = 0;

    if (rrlist)
	*rrlist = 0;
    while ((type = *types++) != 0) {
	if (msg_verbose)
	    msg_info("lookup %s type %s flags %d",
		     name, dns_strtype(type), flags);
	status = dns_lookup_r(name, type, flags, rrlist ? &rr : (DNS_RR **) 0,
			      fqdn, why, rcode);
	if (status == DNS_OK) {
	    non_err = 1;
	    if (rrlist)
		*rrlist = dns_rr_append(*rrlist, rr);
	    if (lflags & DNS_REQ_FLAG_STOP_OK)
		break;
	} else if (status == DNS_INVAL) {
	    if (lflags & DNS_REQ_FLAG_STOP_INVAL)
		break;
	} else if (status == DNS_RETRY) {
	    soft_err = 1;
	}
	/* XXX Stop after NXDOMAIN error. */
    }
    return (non_err ? DNS_OK : soft_err ? DNS_RETRY : status);
}
