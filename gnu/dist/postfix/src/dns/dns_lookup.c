/*++
/* NAME
/*	dns_lookup 3
/* SUMMARY
/*	domain name service lookup
/* SYNOPSIS
/*	#include <dns.h>
/*
/*	int	dns_lookup(name, type, flags, list, fqdn, why)
/*	const char *name;
/*	unsigned type;
/*	unsigned flags;
/*	DNS_RR	**list;
/*	VSTRING *fqdn;
/*	VSTRING *why;
/*
/*	int	dns_lookup_types(name, flags, list, fqdn, why, type, ...)
/*	const char *name;
/*	unsigned flags;
/*	DNS_RR	**list;
/*	VSTRING *fqdn;
/*	VSTRING *why;
/*	unsigned type;
/* DESCRIPTION
/*	dns_lookup() looks up DNS resource records. When requested to
/*	look up data other than type CNAME, it will follow a limited
/*	number of CNAME indirections. All result names (including
/*	null terminator) will fit a buffer of size DNS_NAME_LEN.
/*	All name results are validated by \fIvalid_hostname\fR();
/*	an invalid name is reported as a transient error.
/*
/*	dns_lookup_types() allows the user to specify a null-terminated
/*	list of resource types. This function calls dns_lookup() for each
/*	listed type in the specified order, until the list is exhausted or
/*	until the search result becomes not equal to DNS_NOTFOUND.
/* INPUTS
/* .ad
/* .fi
/* .IP name
/*	The name to be looked up in the domain name system.
/* .IP type
/*	The resource record type to be looked up (T_A, T_MX etc.).
/* .IP flags
/*	A bitwise OR of:
/* .RS
/* .IP RES_DEBUG
/*	Print debugging information.
/* .IP RES_DNSRCH
/*	Search local domain and parent domains.
/* .IP RES_DEFNAMES
/*	Append local domain to unqualified names.
/* .RE
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
/* DIAGNOSTICS
/*	dns_lookup() returns one of the following codes and sets the
/*	\fIwhy\fR argument accordingly:
/* .IP DNS_OK
/*	The DNS query succeeded.
/* .IP DNS_NOTFOUND
/*	The DNS query succeeded; the requested information was not found.
/* .IP DNS_RETRY
/*	The query failed; the problem is transient.
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
#include <stdlib.h>			/* BSDI stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

/* Utility library. */

#include <mymalloc.h>
#include <vstring.h>
#include <msg.h>
#include <valid_hostname.h>
#include <stringops.h>
#include <valid_hostname.h>

/* DNS library. */

#include "dns.h"

/* Local stuff. */

 /*
  * Structure to keep track of things while decoding a name server reply.
  */
#define DNS_REPLY_SIZE	4096		/* in case we're using TCP */

typedef struct DNS_REPLY {
    unsigned char buf[DNS_REPLY_SIZE];	/* raw reply data */
    int     query_count;		/* number of queries */
    int     answer_count;		/* number of answers */
    unsigned char *query_start;		/* start of query data */
    unsigned char *answer_start;	/* start of answer data */
    unsigned char *end;			/* first byte past reply */
} DNS_REPLY;

#define INET_ADDR_LEN	4		/* XXX */
#define INET6_ADDR_LEN	16		/* XXX */

/* dns_query - query name server and pre-parse the reply */

static int dns_query(const char *name, int type, int flags,
		             DNS_REPLY *reply, VSTRING *why)
{
    HEADER *reply_header;
    int     len;
    unsigned long saved_options = _res.options;

    /*
     * Initialize the name service.
     */
    if ((_res.options & RES_INIT) == 0 && res_init() < 0) {
	if (why)
	    vstring_strcpy(why, "Name service initialization failure");
	return (DNS_FAIL);
    }

    /*
     * Set search options: debugging, parent domain search, append local
     * domain. Do not allow the user to control other features.
     */
#define USER_FLAGS (RES_DEBUG | RES_DNSRCH | RES_DEFNAMES)

    if ((flags & USER_FLAGS) != flags)
	msg_panic("dns_query: bad flags: %d", flags);
    _res.options &= ~(USER_FLAGS);
    _res.options |= flags;

    /*
     * Perform the lookup. Claim that the information cannot be found if and
     * only if the name server told us so.
     */
    len = res_search((char *) name, C_IN, type, reply->buf, sizeof(reply->buf));
    _res.options = saved_options;
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

    /*
     * Paranoia.
     */
    if (len > sizeof(reply->buf)) {
	msg_warn("reply length %d > buffer length %d for name=%s type=%s",
		 len, sizeof(reply->buf), name, dns_strtype(type));
	len = sizeof(reply->buf);
    }

    /*
     * Initialize the reply structure. Some structure members are filled on
     * the fly while the reply is being parsed.
     */
    if ((reply->end = reply->buf + len) > reply->buf + sizeof(reply->buf))
	reply->end = reply->buf + sizeof(reply->buf);
    reply_header = (HEADER *) reply->buf;
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

static DNS_RR *dns_get_rr(DNS_REPLY *reply, unsigned char *pos,
			          char *rr_name, DNS_FIXED *fixed)
{
    char    temp[DNS_NAME_LEN];
    int     data_len;
    unsigned pref = 0;
    unsigned char *src;
    unsigned char *dst;
    int     ch;

#define MIN2(a, b)	((unsigned)(a) < (unsigned)(b) ? (a) : (b))

    if (pos + fixed->length > reply->end)
	return (0);

    switch (fixed->type) {
    default:
	msg_panic("dns_get_rr: don't know how to extract resource type %s",
		  dns_strtype(fixed->type));
    case T_CNAME:
    case T_MB:
    case T_MG:
    case T_MR:
    case T_NS:
    case T_PTR:
	if (dn_expand(reply->buf, reply->end, pos, temp, sizeof(temp)) < 0)
	    return (0);
	if (!valid_rr_name(temp, "resource data", fixed->type, reply))
	    return (0);
	data_len = strlen(temp) + 1;
	break;
    case T_MX:
	GETSHORT(pref, pos);
	if (dn_expand(reply->buf, reply->end, pos, temp, sizeof(temp)) < 0)
	    return (0);
	if (!valid_rr_name(temp, "resource data", fixed->type, reply))
	    return (0);
	data_len = strlen(temp) + 1;
	break;
    case T_A:
	if (fixed->length != INET_ADDR_LEN) {
	    msg_warn("extract_answer: bad address length: %d", fixed->length);
	    return (0);
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
	    return (0);
	}
	if (fixed->length > sizeof(temp))
	    msg_panic("dns_get_rr: length %d > DNS_NAME_LEN",
		      fixed->length);
	memcpy(temp, pos, fixed->length);
	data_len = fixed->length;
	break;
#endif
    case T_TXT:
	data_len = MIN2(pos[0] + 1, MIN2(fixed->length + 1, sizeof(temp)));
	for (src = pos + 1, dst = (unsigned char *) (temp);
	     dst < (unsigned char *) (temp) + data_len - 1; /* */ ) {
	    ch = *src++;
	    *dst++ = (ISPRINT(ch) ? ch : ' ');
	}
	*dst = 0;
	break;
    }
    return (dns_rr_create(rr_name, fixed, pref, temp, data_len));
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
	return (DNS_RETRY);
    return (DNS_OK);
}

/* dns_get_answer - extract answers from name server reply */

static int dns_get_answer(DNS_REPLY *reply, int type,
	             DNS_RR **rrlist, VSTRING *fqdn, char *cname, int c_len)
{
    char    rr_name[DNS_NAME_LEN];
    unsigned char *pos;
    int     answer_count = reply->answer_count;
    int     len;
    DNS_FIXED fixed;
    DNS_RR *rr;
    int     resource_found = 0;
    int     cname_found = 0;
    int     not_found_status = DNS_NOTFOUND;

    /*
     * Initialize. Skip over the name server query if we haven't yet.
     */
    if (reply->answer_start == 0)
	if (dns_skip_query(reply) < 0)
	    return (DNS_RETRY);
    pos = reply->answer_start;
    if (rrlist)
	*rrlist = 0;

    /*
     * Either this, or use a GOTO for emergency exits. The purpose is to
     * prevent incomplete answers from being passed back to the caller.
     */
#define CORRUPT { \
	if (rrlist && *rrlist) { \
	    dns_rr_free(*rrlist); \
	    *rrlist = 0; \
	} \
	return (DNS_RETRY); \
    }

    /*
     * Iterate over all answers.
     */
    while (answer_count-- > 0) {

	/*
	 * Optionally extract the fully-qualified domain name.
	 */
	if (pos >= reply->end)
	    CORRUPT;
	len = dn_expand(reply->buf, reply->end, pos, rr_name, DNS_NAME_LEN);
	if (len < 0)
	    CORRUPT;
	pos += len;

	/*
	 * Extract the fixed reply data: type, class, ttl, length.
	 */
	if (pos + RRFIXEDSZ > reply->end)
	    CORRUPT;
	if (dns_get_fixed(pos, &fixed) != DNS_OK)
	    CORRUPT;
	if (!valid_rr_name(rr_name, "resource name", fixed.type, reply))
	    CORRUPT;
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
	    CORRUPT;
	if (type == fixed.type || type == T_ANY) {	/* requested type */
	    if (rrlist) {
		if ((rr = dns_get_rr(reply, pos, rr_name, &fixed)) != 0) {
		    resource_found++;
		    *rrlist = dns_rr_append(*rrlist, rr);
		} else
		    not_found_status = DNS_RETRY;
	    } else
		resource_found++;
	} else if (fixed.type == T_CNAME) {	/* cname resource */
	    cname_found++;
	    if (cname && c_len > 0)
		if (dns_get_alias(reply, pos, &fixed, cname, c_len) != DNS_OK)
		    CORRUPT;
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

/* dns_lookup - DNS lookup user interface */

int     dns_lookup(const char *name, unsigned type, unsigned flags,
		           DNS_RR **rrlist, VSTRING *fqdn, VSTRING *why)
{
    char    cname[DNS_NAME_LEN];
    int     c_len = sizeof(cname);
    DNS_REPLY reply;
    int     count;
    int     status;

    /*
     * The Linux resolver misbehaves when given an invalid domain name.
     */
    if (!valid_hostname(name, DONT_GRIPE)) {
	if (why)
	    vstring_sprintf(why,
		   "Name service error for %s: invalid host or domain name",
			    name);
	h_errno = HOST_NOT_FOUND;
	return (DNS_NOTFOUND);
    }

    /*
     * DJBDNS produces a bogus A record when given a numerical hostname.
     */
    if (valid_hostaddr(name, DONT_GRIPE)) {
	if (why)
	    vstring_sprintf(why,
		   "Name service error for %s: invalid host or domain name",
			    name);
	h_errno = HOST_NOT_FOUND;
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
	if ((status = dns_query(name, type, flags, &reply, why)) != DNS_OK)
	    return (status);

	/*
	 * Extract resource records of the requested type. Pick up CNAME
	 * information just in case the requested data is not found.
	 */
	status = dns_get_answer(&reply, type, rrlist, fqdn, cname, c_len);
	switch (status) {
	default:
	    if (why)
		vstring_sprintf(why, "Name service error for name=%s type=%s: "
				"Malformed name server reply",
				name, dns_strtype(type));
	case DNS_OK:
	case DNS_NOTFOUND:
	    return (status);
	case DNS_RECURSE:
	    if (msg_verbose)
		msg_info("dns_lookup: %s aliased to %s", name, cname);
	    name = cname;
	}
    }
    if (why)
	vstring_sprintf(why, "Name server loop for %s", name);
    msg_warn("dns_lookup: Name server loop for %s", name);
    return (DNS_NOTFOUND);
}

/* dns_lookup_types - DNS lookup interface with multiple types */

int     dns_lookup_types(const char *name, unsigned flags, DNS_RR **rrlist,
			         VSTRING *fqdn, VSTRING *why,...)
{
    va_list ap;
    unsigned type;
    int     status = DNS_NOTFOUND;
    int     soft_err = 0;

    va_start(ap, why);
    while ((type = va_arg(ap, unsigned)) != 0) {
	if (msg_verbose)
	    msg_info("lookup %s type %d flags %d", name, type, flags);
	status = dns_lookup(name, type, flags, rrlist, fqdn, why);
	if (status == DNS_OK)
	    break;
	if (status == DNS_RETRY)
	    soft_err = 1;
    }
    va_end(ap);
    return ((status == DNS_OK || soft_err == 0) ? status : DNS_RETRY);
}
