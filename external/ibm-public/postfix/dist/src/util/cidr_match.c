/*	$NetBSD: cidr_match.c,v 1.4 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	cidr_match 3
/* SUMMARY
/*	CIDR-style pattern matching
/* SYNOPSIS
/*	#include <cidr_match.h>
/*
/*	VSTRING *cidr_match_parse(info, pattern, match, why)
/*	CIDR_MATCH *info;
/*	char	*pattern;
/*	VSTRING	*why;
/*
/*	int	cidr_match_execute(info, address)
/*	CIDR_MATCH *info;
/*	const char *address;
/* AUXILIARY FUNCTIONS
/*	VSTRING *cidr_match_parse_if(info, pattern, match, why)
/*	CIDR_MATCH *info;
/*	char	*pattern;
/*	VSTRING	*why;
/*
/*	void	cidr_match_endif(info)
/*	CIDR_MATCH *info;
/* DESCRIPTION
/*	This module parses address or address/length patterns and
/*	provides simple address matching. The implementation is
/*	such that parsing and execution can be done without dynamic
/*	memory allocation. The purpose is to minimize overhead when
/*	called by functions that parse and execute on the fly, such
/*	as match_hostaddr().
/*
/*	cidr_match_parse() parses an address or address/mask
/*	expression and stores the result into the info argument.
/*	A non-zero (or zero) match argument requests a positive (or
/*	negative) match. The symbolic constants CIDR_MATCH_TRUE and
/*	CIDR_MATCH_FALSE may help to improve code readability.
/*	The result is non-zero in case of problems: either the
/*	value of the why argument, or a newly allocated VSTRING
/*	(the caller should give the latter to vstring_free()).
/*	The pattern argument is destroyed.
/*
/*	cidr_match_parse_if() parses the address that follows an IF
/*	token, and stores the result into the info argument.
/*	The arguments are the same as for cidr_match_parse().
/*
/*	cidr_match_endif() handles the occurrence of an ENDIF token,
/*	and updates the info argument.
/*
/*	cidr_match_execute() matches the specified address against
/*	a list of parsed expressions, and returns the matching
/*	expression's data structure.
/* SEE ALSO
/*	dict_cidr(3) CIDR-style lookup table
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
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <stringops.h>
#include <split_at.h>
#include <myaddrinfo.h>
#include <mask_addr.h>
#include <cidr_match.h>

/* Application-specific. */

 /*
  * This is how we figure out the address family, address bit count and
  * address byte count for a CIDR_MATCH entry.
  */
#ifdef HAS_IPV6
#define CIDR_MATCH_ADDR_FAMILY(a) (strchr((a), ':') ? AF_INET6 : AF_INET)
#define CIDR_MATCH_ADDR_BIT_COUNT(f) \
    ((f) == AF_INET6 ? MAI_V6ADDR_BITS : \
     (f) == AF_INET ? MAI_V4ADDR_BITS : \
     (msg_panic("%s: bad address family %d", myname, (f)), 0))
#define CIDR_MATCH_ADDR_BYTE_COUNT(f) \
    ((f) == AF_INET6 ? MAI_V6ADDR_BYTES : \
     (f) == AF_INET ? MAI_V4ADDR_BYTES : \
     (msg_panic("%s: bad address family %d", myname, (f)), 0))
#else
#define CIDR_MATCH_ADDR_FAMILY(a) (AF_INET)
#define CIDR_MATCH_ADDR_BIT_COUNT(f) \
    ((f) == AF_INET ? MAI_V4ADDR_BITS : \
     (msg_panic("%s: bad address family %d", myname, (f)), 0))
#define CIDR_MATCH_ADDR_BYTE_COUNT(f) \
    ((f) == AF_INET ? MAI_V4ADDR_BYTES : \
     (msg_panic("%s: bad address family %d", myname, (f)), 0))
#endif

/* cidr_match_entry - match one entry */

static inline int cidr_match_entry(CIDR_MATCH *entry,
				           unsigned char *addr_bytes)
{
    unsigned char *mp;
    unsigned char *np;
    unsigned char *ap;

    /* Unoptimized case: netmask with some or all bits zero. */
    if (entry->mask_shift < entry->addr_bit_count) {
	for (np = entry->net_bytes, mp = entry->mask_bytes,
	     ap = addr_bytes; /* void */ ; np++, mp++, ap++) {
	    if (ap >= addr_bytes + entry->addr_byte_count)
		return (entry->match);
	    if ((*ap & *mp) != *np)
		break;
	}
    }
    /* Optimized case: all 1 netmask (i.e. no netmask specified). */
    else {
	for (np = entry->net_bytes,
	     ap = addr_bytes; /* void */ ; np++, ap++) {
	    if (ap >= addr_bytes + entry->addr_byte_count)
		return (entry->match);
	    if (*ap != *np)
		break;
	}
    }
    return (!entry->match);
}

/* cidr_match_execute - match address against compiled CIDR pattern list */

CIDR_MATCH *cidr_match_execute(CIDR_MATCH *list, const char *addr)
{
    unsigned char addr_bytes[CIDR_MATCH_ABYTES];
    unsigned addr_family;
    CIDR_MATCH *entry;

    addr_family = CIDR_MATCH_ADDR_FAMILY(addr);
    if (inet_pton(addr_family, addr, addr_bytes) != 1)
	return (0);

    for (entry = list; entry; entry = entry->next) {

	switch (entry->op) {

	case CIDR_MATCH_OP_MATCH:
	    if (entry->addr_family == addr_family)
		if (cidr_match_entry(entry, addr_bytes))
		    return (entry);
	    break;

	case CIDR_MATCH_OP_IF:
	    if (entry->addr_family == addr_family)
		if (cidr_match_entry(entry, addr_bytes))
		    continue;
	    /* An IF without matching ENDIF has no end-of block entry. */
	    if ((entry = entry->block_end) == 0)
		return (0);
	    /* FALLTHROUGH */

	case CIDR_MATCH_OP_ENDIF:
	    continue;
	}
    }
    return (0);
}

/* cidr_match_parse - parse CIDR pattern */

VSTRING *cidr_match_parse(CIDR_MATCH *ip, char *pattern, int match,
			          VSTRING *why)
{
    const char *myname = "cidr_match_parse";
    char   *mask_search;
    char   *mask;
    MAI_HOSTADDR_STR hostaddr;
    unsigned char *np;
    unsigned char *mp;

    /*
     * Strip [] from [addr/len] or [addr]/len, destroying the pattern. CIDR
     * maps don't need [] to eliminate syntax ambiguity, but matchlists need
     * it. While stripping [], figure out where we should start looking for
     * /mask information.
     */
    if (*pattern == '[') {
	pattern++;
	if ((mask_search = split_at(pattern, ']')) == 0) {
	    vstring_sprintf(why ? why : (why = vstring_alloc(20)),
			    "missing ']' character after \"[%s\"", pattern);
	    return (why);
	} else if (*mask_search != '/') {
	    if (*mask_search != 0) {
		vstring_sprintf(why ? why : (why = vstring_alloc(20)),
				"garbage after \"[%s]\"", pattern);
		return (why);
	    }
	    mask_search = pattern;
	}
    } else
	mask_search = pattern;

    /*
     * Parse the pattern into network and mask, destroying the pattern.
     */
    if ((mask = split_at(mask_search, '/')) != 0) {
	const char *parse_error;

	ip->addr_family = CIDR_MATCH_ADDR_FAMILY(pattern);
	ip->addr_bit_count = CIDR_MATCH_ADDR_BIT_COUNT(ip->addr_family);
	ip->addr_byte_count = CIDR_MATCH_ADDR_BYTE_COUNT(ip->addr_family);
	if (!alldig(mask)) {
	    parse_error = "bad mask value";
	} else if ((ip->mask_shift = atoi(mask)) > ip->addr_bit_count) {
	    parse_error = "bad mask length";
	} else if (inet_pton(ip->addr_family, pattern, ip->net_bytes) != 1) {
	    parse_error = "bad network value";
	} else {
	    parse_error = 0;
	}
	if (parse_error != 0) {
	    vstring_sprintf(why ? why : (why = vstring_alloc(20)),
			    "%s in \"%s/%s\"", parse_error, pattern, mask);
	    return (why);
	}
	if (ip->mask_shift > 0) {
	    /* Allow for bytes > 8. */
	    memset(ip->mask_bytes, ~0U, ip->addr_byte_count);
	    mask_addr(ip->mask_bytes, ip->addr_byte_count, ip->mask_shift);
	} else
	    memset(ip->mask_bytes, 0, ip->addr_byte_count);

	/*
	 * Sanity check: all host address bits must be zero.
	 */
	for (np = ip->net_bytes, mp = ip->mask_bytes;
	     np < ip->net_bytes + ip->addr_byte_count; np++, mp++) {
	    if (*np & ~(*mp)) {
		mask_addr(ip->net_bytes, ip->addr_byte_count, ip->mask_shift);
		if (inet_ntop(ip->addr_family, ip->net_bytes, hostaddr.buf,
			      sizeof(hostaddr.buf)) == 0)
		    msg_fatal("inet_ntop: %m");
		vstring_sprintf(why ? why : (why = vstring_alloc(20)),
				"non-null host address bits in \"%s/%s\", "
				"perhaps you should use \"%s/%d\" instead",
				pattern, mask, hostaddr.buf, ip->mask_shift);
		return (why);
	    }
	}
    }

    /*
     * No /mask specified. Treat a bare network address as /allbits.
     */
    else {
	ip->addr_family = CIDR_MATCH_ADDR_FAMILY(pattern);
	ip->addr_bit_count = CIDR_MATCH_ADDR_BIT_COUNT(ip->addr_family);
	ip->addr_byte_count = CIDR_MATCH_ADDR_BYTE_COUNT(ip->addr_family);
	if (inet_pton(ip->addr_family, pattern, ip->net_bytes) != 1) {
	    vstring_sprintf(why ? why : (why = vstring_alloc(20)),
			    "bad address pattern: \"%s\"", pattern);
	    return (why);
	}
	ip->mask_shift = ip->addr_bit_count;
	/* Allow for bytes > 8. */
	memset(ip->mask_bytes, ~0U, ip->addr_byte_count);
    }

    /*
     * Wrap up the result.
     */
    ip->op = CIDR_MATCH_OP_MATCH;
    ip->match = match;
    ip->next = 0;
    ip->block_end = 0;

    return (0);
}

/* cidr_match_parse_if - parse CIDR pattern after IF */

VSTRING *cidr_match_parse_if(CIDR_MATCH *ip, char *pattern, int match,
			             VSTRING *why)
{
    VSTRING *ret;

    if ((ret = cidr_match_parse(ip, pattern, match, why)) == 0)
	ip->op = CIDR_MATCH_OP_IF;
    return (ret);
}

/* cidr_match_endif - handle ENDIF pattern */

void    cidr_match_endif(CIDR_MATCH *ip)
{
    memset(ip, 0, sizeof(*ip));
    ip->op = CIDR_MATCH_OP_ENDIF;
    ip->next = 0;				/* maybe not all bits 0 */
    ip->block_end = 0;
}
