/*++
/* NAME
/*	match_ops 3
/* SUMMARY
/*	simple string or host pattern matching
/* SYNOPSIS
/*	#include <match_ops.h>
/*
/*	int	match_string(string, pattern)
/*	const char *string;
/*	const char *pattern;
/*
/*	int	match_hostname(name, pattern)
/*	const char *name;
/*	const char *pattern;
/*
/*	int	match_hostaddr(addr, pattern)
/*	const char *addr;
/*	const char *pattern;
/* DESCRIPTION
/*	This module implements simple string and host name or address
/*	matching. The matching process is case insensitive. If a pattern
/*	has the form type:name, table lookup is used instead of string
/*	or address comparison.
/*
/*	match_string() matches the string against the pattern, requiring
/*	an exact (case-insensitive) match.
/*
/*	match_hostname() matches the host name when the hostname matches
/*	the pattern exactly, or when the pattern matches a parent domain
/*	of the named host.
/*
/*	match_hostaddr() matches a host address when the pattern is
/*	identical to the host address, or when the pattern is a net/mask
/*	that contains the address. The mask specifies the number of
/*	bits in the network part of the pattern.
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
#define INADDR_NONE 0xffffff
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <split_at.h>
#include <dict.h>
#include <match_ops.h>
#include <stringops.h>

/* match_string - match a string literal */

int     match_string(const char *string, const char *pattern)
{
    const char *myname = "match_string";
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

int     match_hostname(const char *name, const char *pattern)
{
    const char *myname = "match_hostname";
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
	for (entry = temp; /* void */ ; entry = next + 1) {
	    if ((match = (dict_lookup(pattern, entry) != 0)) != 0)
		break;
	    if (dict_errno != 0)
		msg_fatal("%s: table lookup problem", pattern);
	    if ((next = strchr(entry, '.')) == 0)
		break;
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
	pd = name + strlen(name) - strlen(pattern);
	if (pd > name && pd[-1] == '.' && strcasecmp(pd, pattern) == 0)
	    return (1);
    }
    return (0);
}

/* match_parse_mask - parse net/mask pattern */

static int match_parse_mask(const char *pattern, unsigned long *net_bits,
			            int *mask_shift)
{
    char   *saved_pattern;
    char   *mask;

#define BITS_PER_ADDR	32

    saved_pattern = mystrdup(pattern);
    if ((mask = split_at(saved_pattern, '/')) != 0) {
	if ((*mask_shift = atoi(mask)) <= 0 || *mask_shift > BITS_PER_ADDR
	    || (*net_bits = inet_addr(saved_pattern)) == INADDR_NONE) {
	    msg_fatal("bad net/mask pattern: %s", pattern);
	}
    }
    myfree(saved_pattern);
    return (mask != 0);
}

/* match_hostaddr - match host by address */

int     match_hostaddr(const char *addr, const char *pattern)
{
    const char *myname = "match_hostaddr";
    int     mask_shift;
    unsigned long mask_bits;
    unsigned long net_bits;
    unsigned long addr_bits;

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
	mask_bits = htonl((0xffffffff) << (BITS_PER_ADDR - mask_shift));
	return ((addr_bits & mask_bits) == (net_bits & mask_bits));
    }
    return (0);
}
