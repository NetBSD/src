/*++
/* NAME
/*	match_ops 3
/* SUMMARY
/*	simple string or host pattern matching
/* SYNOPSIS
/*	#include <match_ops.h>
/*
/*	int	match_string(flags, string, pattern)
/*	int	flags;
/*	const char *string;
/*	const char *pattern;
/*
/*	int	match_hostname(flags, name, pattern)
/*	int	flags;
/*	const char *name;
/*	const char *pattern;
/*
/*	int	match_hostaddr(flags, addr, pattern)
/*	int	flags;
/*	const char *addr;
/*	const char *pattern;
/* DESCRIPTION
/*	This module implements simple string and host name or address
/*	matching. The matching process is case insensitive. If a pattern
/*	has the form type:name, table lookup is used instead of string
/*	or address comparison.
/*
/*	match_string() matches the string against the pattern, requiring
/*	an exact (case-insensitive) match. The flags argument is not used.
/*
/*	match_hostname() matches the host name when the hostname matches
/*	the pattern exactly, or when the pattern matches a parent domain
/*	of the named host. The flags argument specifies the bit-wise OR
/*	of zero or more of the following:
/* .IP MATCH_FLAG_PARENT
/*	The hostname pattern foo.com matches itself and any name below
/*	the domain foo.com. If this flag is cleared, foo.com matches itself
/*	only, and .foo.com matches any name below the domain foo.com.
/* .RE
/*	Specify MATCH_FLAG_NONE to request none of the above.
/*
/*	match_hostaddr() matches a host address when the pattern is
/*	identical to the host address, or when the pattern is a net/mask
/*	that contains the address. The mask specifies the number of
/*	bits in the network part of the pattern. The flags argument is
/*	not used.
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <split_at.h>
#include <dict.h>
#include <match_ops.h>
#include <stringops.h>

/* match_string - match a string literal */

int     match_string(int unused_flags, const char *string, const char *pattern)
{
    char   *myname = "match_string";
    int     match;
    char   *key;

    if (msg_verbose)
	msg_info("%s: %s ~? %s", myname, string, pattern);

    /*
     * Try dictionary lookup: exact match.
     */
    if (strchr(pattern, ':') != 0) {
	key = lowercase(mystrdup(string));
	match = (dict_lookup(pattern, key) != 0);
	myfree(key);
	if (match != 0)
	    return (1);
	if (dict_errno != 0)
	    msg_fatal("%s: table lookup problem", pattern);
	return (0);
    }

    /*
     * Try an exact string match.
     */
    if (strcasecmp(string, pattern) == 0) {
	return (1);
    }

    /*
     * No match found.
     */
    return (0);
}

/* match_hostname - match a host by name */

int     match_hostname(int flags, const char *name, const char *pattern)
{
    char   *myname = "match_hostname";
    const char *pd;
    const char *entry;
    char   *next;
    char   *temp;
    int     match;

    if (msg_verbose)
	msg_info("%s: %s ~? %s", myname, name, pattern);

    /*
     * Try dictionary lookup: exact match and parent domains.
     */
    if (strchr(pattern, ':') != 0) {
	temp = lowercase(mystrdup(name));
	match = 0;
	for (entry = temp; *entry != 0; entry = next) {
	    if ((match = (dict_lookup(pattern, entry) != 0)) != 0)
		break;
	    if (dict_errno != 0)
		msg_fatal("%s: table lookup problem", pattern);
	    if ((next = strchr(entry + 1, '.')) == 0)
		break;
	    if (flags & MATCH_FLAG_PARENT)
		next += 1;
	}
	myfree(temp);
	return (match);
    }

    /*
     * Try an exact match with the host name.
     */
    if (strcasecmp(name, pattern) == 0) {
	return (1);
    }

    /*
     * See if the pattern is a parent domain of the hostname.
     */
    else {
	if (flags & MATCH_FLAG_PARENT) {
	    pd = name + strlen(name) - strlen(pattern);
	    if (pd > name && pd[-1] == '.' && strcasecmp(pd, pattern) == 0)
		return (1);
	} else if (pattern[0] == '.') {
	    pd = name + strlen(name) - strlen(pattern);
	    if (pd > name && strcasecmp(pd, pattern) == 0)
		return (1);
	}
    }
    return (0);
}

/* match_parse_mask - parse net/mask pattern */

static int match_parse_mask(const char *pattern, unsigned long *net_bits,
			            unsigned int *mask_shift)
{
    char   *saved_pattern;
    char   *mask;

#define BITS_PER_ADDR	32

    saved_pattern = mystrdup(pattern);
    if ((mask = split_at(saved_pattern, '/')) != 0) {
	if (!alldig(mask) || (*mask_shift = atoi(mask)) > BITS_PER_ADDR
	    || (*net_bits = inet_addr(saved_pattern)) == INADDR_NONE) {
	    msg_fatal("bad net/mask pattern: %s", pattern);
	}
    }
    myfree(saved_pattern);
    return (mask != 0);
}

/* match_hostaddr - match host by address */

int     match_hostaddr(int unused_flags, const char *addr, const char *pattern)
{
    char   *myname = "match_hostaddr";
    unsigned int mask_shift;
    unsigned long mask_bits;
    unsigned long net_bits;
    unsigned long addr_bits;
    struct in_addr net_addr;

    if (msg_verbose)
	msg_info("%s: %s ~? %s", myname, addr, pattern);

    if (addr[strspn(addr, "01234567890./:")] != 0)
	return (0);

    /*
     * Try dictionary lookup. This can be case insensitive. XXX Probably
     * should also try again after stripping least significant octets.
     */
    if (strchr(pattern, ':') != 0) {
	if (dict_lookup(pattern, addr) != 0)
	    return (1);
	if (dict_errno != 0)
	    msg_fatal("%s: table lookup problem", pattern);
	return (0);
    }

    /*
     * Try an exact match with the host address.
     */
    if (strcasecmp(addr, pattern) == 0) {
	return (1);
    }

    /*
     * In a net/mask pattern, the mask is specified as the number of bits of
     * the network part.
     */
    if (match_parse_mask(pattern, &net_bits, &mask_shift)) {
	addr_bits = inet_addr(addr);
	if (addr_bits == INADDR_NONE)
	    msg_fatal("%s: bad address argument: %s", myname, addr);
	mask_bits = mask_shift > 0 ?
	    htonl((0xffffffff) << (BITS_PER_ADDR - mask_shift)) : 0;
	if ((addr_bits & mask_bits) == net_bits)
	    return (1);
	if (net_bits & ~mask_bits) {
	    net_addr.s_addr = (net_bits & mask_bits);
	    msg_fatal("net/mask pattern %s has a non-null host portion; "
		      "specify %s/%d if this is really what you want",
		      pattern, inet_ntoa(net_addr), mask_shift);
	}
    }
    return (0);
}
