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

#ifdef INET6
/*
 *		$Id: match_ops.c,v 1.2 2001/03/13 18:34:22 itojun Exp $
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 * 
 * Modifications:
 *		Artur Frysiak <wiget@pld.org.pl>
 *		Arkadiusz Mi¶kiewicz <misiek@pld.org.pl>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <resolv.h>

#ifndef	AF_DECnet
#define	AF_DECnet	12
#endif

#ifndef	PF_PACKET
#define	PF_PACKET	17
#endif

typedef struct
{
	unsigned char family;
	unsigned char bytelen;
	signed short  bitlen;
	unsigned int data[4];
} inet_prefix;

/* prototypes */
int masked_match(char *, char *, char *);
int get_integer(int *, char *, int);
int get_addr_1(inet_prefix *, char *, int);
int get_prefix_1(inet_prefix *, char *, int);
int get_addr(inet_prefix *, char *, int);
int get_prefix(inet_prefix *, char *, int);
unsigned int get_addr32(char *);
int matches(char *, char *);
int inet_addr_match(inet_prefix *, inet_prefix *, int);
int mask_match(char *, char *, char *);
	
int get_integer(int *val, char *arg, int base)
{
	long res;
	char *ptr;

	if (!arg || !*arg)
		return -1;
	res = strtol(arg, &ptr, base);
	if (!ptr || ptr == arg || *ptr || res > INT_MAX || res < INT_MIN)
		return -1;
	*val = res;
	return 0;
}

int get_addr_1(inet_prefix *addr, char *name, int family)
{
	char *cp;
	unsigned char *ap = (unsigned char*)addr->data;
	int i;

	memset(addr, 0, sizeof(*addr));

	if (strcmp(name, "default") == 0 || strcmp(name, "any") == 0) {
		if (family == AF_DECnet)
			return -1;
		addr->family = family;
		addr->bytelen = (family == AF_INET6 ? 16 : 4);
		addr->bitlen = -1;
		return 0;
	}

	if (strchr(name, ':')) {
		addr->family = AF_INET6;
		if (family != AF_UNSPEC && family != AF_INET6)
			return -1;
		if (inet_pton(AF_INET6, name, addr->data) <= 0)
			return -1;
		addr->bytelen = 16;
		addr->bitlen = -1;
		return 0;
	}
	addr->family = AF_INET;
	if (family != AF_UNSPEC && family != AF_INET)
		return -1;
	addr->bytelen = 4;
	addr->bitlen = -1;
	for (cp = name, i = 0; *cp; cp++) {
		if (*cp <= '9' && *cp >= '0') {
			ap[i] = 10*ap[i] + (*cp-'0');
			continue;
		}
		if (*cp == '.' && ++i <= 3)
			continue;
		return -1;
	}
	return 0;
}

int get_prefix_1(inet_prefix *dst, char *arg, int family)
{
	int err;
	unsigned plen;
	char *slash;

	memset(dst, 0, sizeof(*dst));

	if (strcmp(arg, "default") == 0 || strcmp(arg, "any") == 0) {
		if (family == AF_DECnet)
			return -1;
		dst->family = family;
		dst->bytelen = 0;
		dst->bitlen = 0;
		return 0;
	}

	slash = strchr(arg, '/');
	if (slash)
		*slash = 0;
	err = get_addr_1(dst, arg, family);
	if (err == 0) {
		switch(dst->family) {
			case AF_INET6:
				dst->bitlen = 128;
				break;
			case AF_DECnet:
				dst->bitlen = 16;
				break;
			default:
			case AF_INET:
				dst->bitlen = 32;
		}
		if (slash) {
			if (get_integer(&plen, slash+1, 0) || plen > dst->bitlen) {
				err = -1;
				goto done;
			}
			dst->bitlen = plen;
		}
	}
done:
	if (slash)
		*slash = '/';
	return err;
}

int get_addr(inet_prefix *dst, char *arg, int family)
{
#ifdef AF_PACKET
	if (family == AF_PACKET)
		return -1;
#endif
	if (get_addr_1(dst, arg, family))
		return -1;
	return 0;
}

int get_prefix(inet_prefix *dst, char *arg, int family)
{
#ifdef AF_PACKET
	if (family == AF_PACKET)
		return -1;
#endif
	if (get_prefix_1(dst, arg, family))
		return -1;
	return 0;
}

unsigned int get_addr32(char *name)
{
	inet_prefix addr;
	if (get_addr_1(&addr, name, AF_INET))
		return -1;
	return addr.data[0];
}

int matches(char *cmd, char *pattern)
{
	int len = strlen(cmd);
	if (len > strlen(pattern))
		return -1;
	return memcmp(pattern, cmd, len);
}

int inet_addr_match(inet_prefix *a, inet_prefix *b, int bits)
{
	unsigned int *a1 = a->data;
	unsigned int *a2 = b->data;
	int words = bits >> 0x05;

	bits &= 0x1f;

	if (words)
		if (memcmp(a1, a2, words << 2))
			return -1;

	if (bits) {
		unsigned int w1, w2;
		unsigned int mask;

		w1 = a1[words];
		w2 = a2[words];

		mask = htonl((0xffffffff) << (0x20 - bits));

		if ((w1 ^ w2) & mask)
			return 1;
	}

	return 0;
}

/* zero if matches */
int mask_match(char *network, char *cprefix, char *address)
{
 	inet_prefix *inetwork;
	inet_prefix *iaddress;
	int ret, prefix;

	if (!(network && address && cprefix))
		return -1;
	prefix = strtol(cprefix, (char **)NULL, 10);
	if ((prefix < 0) || (prefix > 128))
		return -1;
	if ((strlen(network) == 0) || (strlen(address) == 0))
		return -1;

	inetwork = malloc(sizeof(inet_prefix));
	iaddress = malloc(sizeof(inet_prefix));

	if ((get_addr(iaddress, address, AF_UNSPEC) >= 0)
			&& (get_addr(inetwork, network, AF_UNSPEC) >= 0))
		ret = inet_addr_match(inetwork, iaddress, prefix);
	else
		ret = -1;
	free(inetwork);
	free(iaddress);

	/* 1 if matches */
	/* return (!ret); */
	/* 0 if matches */
	return ret;
}

/*
 * masked_match() - universal for IPv4 and IPv6
 */
int masked_match(net_tok, mask_tok, string)
char	*net_tok;
char	*mask_tok;
char	*string;
{
#ifdef INET6
	struct in6_addr in6[2];
	char v4addr[2][INET_ADDRSTRLEN];
	char newmask[6];
	int plen;
#endif

	/* Check for NULL */
	if (!(net_tok && mask_tok && string))
		return 1;

	/* If IPv6 mapped convert to native-IPv4 */
#ifdef INET6
	if (inet_pton(AF_INET6, net_tok, &in6[0]) == 1 &&
	    inet_pton(AF_INET6, string, &in6[1]) == 1 &&
	    IN6_IS_ADDR_V4MAPPED(&in6[0]) && IN6_IS_ADDR_V4MAPPED(&in6[1])) {
		plen = atoi(mask_tok);
		if (32 < plen && plen < 129) {
			sprintf(newmask, "%d", plen - 96);
			mask_tok = newmask;
		}

		(void)inet_ntop(AF_INET, &in6[0].s6_addr[12], v4addr[0],
		    sizeof(v4addr[0]));
		net_tok = v4addr[0];
		(void)inet_ntop(AF_INET, &in6[1].s6_addr[12], v4addr[1],
		    sizeof(v4addr[1]));
		string = v4addr[1];
	}
#endif
	return (!mask_match(net_tok, mask_tok, string));
}
#endif

/* match_string - match a string literal */

int     match_string(const char *string, const char *pattern)
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

int     match_hostname(const char *name, const char *pattern)
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

#ifndef INET6
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
#endif

/* match_hostaddr - match host by address */

int     match_hostaddr(const char *addr, const char *pattern)
{
    char   *myname = "match_hostaddr";
#ifdef INET6
    char *network, *mask, *escl, *escr, *patternx;
    struct in6_addr in6;
    char v4addr[INET_ADDRSTRLEN];
#else
    int     mask_shift;
    unsigned long mask_bits;
    unsigned long net_bits;
    unsigned long addr_bits;
#endif

    if (msg_verbose)
	msg_info("%s: %s ~? %s", myname, addr, pattern);

#ifdef INET6
    if (addr[strspn(addr, "01234567890./:abcdef")] != 0)
#else
    if (addr[strspn(addr, "01234567890./:")] != 0)
#endif
	return (0);

#ifdef INET6
    patternx = mystrdup(pattern);
    escl = strchr(patternx,'[');
    escr = strrchr(patternx,']');
    if (escl && escr) {
	*escr = 0;
	sprintf(patternx, "%s%s", escl + 1, escr + 1);
	pattern = patternx;
    }
#endif
    
    /*
     * Try dictionary lookup. This can be case insensitive. XXX Probably
     * should also try again after stripping least significant octets.
     */
#ifdef INET6
    if (!(escl && escr) && strchr(pattern, ':') != 0)
#else
    if (strchr(pattern, ':') != 0)
#endif
    {
	if (dict_lookup(pattern, addr) != 0)
	    return (1);
	if (dict_errno != 0)
	    msg_fatal("%s: table lookup problem", pattern);
	return (0);
    }

    /*
     * Try an exact match with the host address.
     */
#ifdef INET6
    if (inet_pton(AF_INET6, addr, &in6) == 1 && IN6_IS_ADDR_V4MAPPED(&in6)) {
	(void)inet_ntop(AF_INET, &in6.s6_addr[12], v4addr, sizeof(v4addr));
	addr = v4addr;
    }
#endif
    if (strcasecmp(addr, pattern) == 0) {
	return (1);
    }

    /*
     * In a net/mask pattern, the mask is specified as the number of bits of
     * the network part.
     */
#ifdef INET6
    network = mystrdup(patternx);
    mask = split_at(network, '/');

    if (masked_match(network, mask, (char *)addr)) {
	myfree(network);
	myfree(patternx);
	return (1);
    } else {
	myfree(network);
	myfree(patternx);
    }
#else
	    
    if (match_parse_mask(pattern, &net_bits, &mask_shift)) {
	addr_bits = inet_addr(addr);
	if (addr_bits == INADDR_NONE)
	    msg_fatal("%s: bad address argument: %s", myname, addr);
	mask_bits = htonl((0xffffffff) << (BITS_PER_ADDR - mask_shift));
	return ((addr_bits & mask_bits) == (net_bits & mask_bits));
    }
#endif
    return (0);
}
