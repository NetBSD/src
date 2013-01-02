/*	$NetBSD: match_ops.c,v 1.1.1.2 2013/01/02 18:59:13 tron Exp $	*/

/*++
/* NAME
/*	match_ops 3
/* SUMMARY
/*	simple string or host pattern matching
/* SYNOPSIS
/*	#include <match_list.h>
/*
/*	int	match_string(list, string, pattern)
/*	MATCH_LIST *list;
/*	const char *string;
/*	const char *pattern;
/*
/*	int	match_hostname(list, name, pattern)
/*	MATCH_LIST *list;
/*	const char *name;
/*	const char *pattern;
/*
/*	int	match_hostaddr(list, addr, pattern)
/*	MATCH_LIST *list;
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
/* .IP MATCH_FLAG_RETURN
/*	Log a warning, return "not found", and set list->error to
/*	a non-zero dictionary error code, instead of raising a fatal
/*	run-time error.
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

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <split_at.h>
#include <dict.h>
#include <match_list.h>
#include <stringops.h>
#include <cidr_match.h>

#define MATCH_DICTIONARY(pattern) \
    ((pattern)[0] != '[' && strchr((pattern), ':') != 0)

/* match_error - return or raise fatal error */

static int match_error(MATCH_LIST *list, const char *fmt,...)
{
    VSTRING *buf = vstring_alloc(100);
    va_list ap;

    /*
     * Report, and maybe return.
     */
    va_start(ap, fmt);
    vstring_vsprintf(buf, fmt, ap);
    va_end(ap);
    if (list->flags & MATCH_FLAG_RETURN) {
	msg_warn("%s", vstring_str(buf));
    } else {
	msg_fatal("%s", vstring_str(buf));
    }
    vstring_free(buf);
    return (0);
}

/* match_string - match a string literal */

int     match_string(MATCH_LIST *list, const char *string, const char *pattern)
{
    const char *myname = "match_string";
    DICT   *dict;

    if (msg_verbose)
	msg_info("%s: %s ~? %s", myname, string, pattern);

    /*
     * Try dictionary lookup: exact match.
     */
    if (MATCH_DICTIONARY(pattern)) {
	if ((dict = dict_handle(pattern)) == 0)
	    msg_panic("%s: unknown dictionary: %s", myname, pattern);
	if (dict_get(dict, string) != 0)
	    return (1);
	if ((list->error = dict->error) != 0)
	    return (match_error(list, "%s:%s: table lookup problem",
				dict->type, dict->name));
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

int     match_hostname(MATCH_LIST *list, const char *name, const char *pattern)
{
    const char *myname = "match_hostname";
    const char *pd;
    const char *entry;
    const char *next;
    int     match;
    DICT   *dict;

    if (msg_verbose)
	msg_info("%s: %s ~? %s", myname, name, pattern);

    /*
     * Try dictionary lookup: exact match and parent domains.
     * 
     * Don't look up parent domain substrings with regexp maps etc.
     */
    if (MATCH_DICTIONARY(pattern)) {
	if ((dict = dict_handle(pattern)) == 0)
	    msg_panic("%s: unknown dictionary: %s", myname, pattern);
	match = 0;
	for (entry = name; *entry != 0; entry = next) {
	    if (entry == name || (dict->flags & DICT_FLAG_FIXED)) {
		match = (dict_get(dict, entry) != 0);
		if (msg_verbose > 1)
		    msg_info("%s: lookup %s:%s %s: %s",
			     myname, dict->type, dict->name, entry,
			     match ? "found" : "notfound");
		if (match != 0)
		    break;
		if ((list->error = dict->error) != 0)
		    return (match_error(list, "%s:%s: table lookup problem",
					dict->type, dict->name));
	    }
	    if ((next = strchr(entry + 1, '.')) == 0)
		break;
	    if (list->flags & MATCH_FLAG_PARENT)
		next += 1;
	}
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
	if (list->flags & MATCH_FLAG_PARENT) {
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

/* match_hostaddr - match host by address */

int     match_hostaddr(MATCH_LIST *list, const char *addr, const char *pattern)
{
    const char *myname = "match_hostaddr";
    char   *saved_patt;
    CIDR_MATCH match_info;
    DICT   *dict;
    VSTRING *err;
    int     rc;

    if (msg_verbose)
	msg_info("%s: %s ~? %s", myname, addr, pattern);

#define V4_ADDR_STRING_CHARS	"01234567890."
#define V6_ADDR_STRING_CHARS	V4_ADDR_STRING_CHARS "abcdefABCDEF:"

    if (addr[strspn(addr, V6_ADDR_STRING_CHARS)] != 0)
	return (0);

    /*
     * Try dictionary lookup. This can be case insensitive.
     */
    if (MATCH_DICTIONARY(pattern)) {
	if ((dict = dict_handle(pattern)) == 0)
	    msg_panic("%s: unknown dictionary: %s", myname, pattern);
	if (dict_get(dict, addr) != 0)
	    return (1);
	if ((list->error = dict->error) != 0)
	    return (match_error(list, "%s:%s: table lookup problem",
				dict->type, dict->name));
	return (0);
    }

    /*
     * Try an exact match with the host address.
     */
    if (pattern[0] != '[') {
	if (strcasecmp(addr, pattern) == 0)
	    return (1);
    } else {
	size_t  addr_len = strlen(addr);

	if (strncasecmp(addr, pattern + 1, addr_len) == 0
	    && strcmp(pattern + 1 + addr_len, "]") == 0)
	    return (1);
    }

    /*
     * Light-weight tests before we get into expensive operations.
     * 
     * - Don't bother matching IPv4 against IPv6. Postfix transforms
     * IPv4-in-IPv6 to native IPv4 form when IPv4 support is enabled in
     * Postfix; if not, then Postfix has no business dealing with IPv4
     * addresses anyway.
     * 
     * - Don't bother unless the pattern is either an IPv6 address or net/mask.
     * 
     * We can safely skip IPv4 address patterns because their form is
     * unambiguous and they did not match in the strcasecmp() calls above.
     * 
     * XXX We MUST skip (parent) domain names, which may appear in NAMADR_LIST
     * input, to avoid triggering false cidr_match_parse() errors.
     * 
     * The last two conditions below are for backwards compatibility with
     * earlier Postfix versions: don't abort with fatal errors on junk that
     * was silently ignored (principle of least astonishment).
     */
    if (!strchr(addr, ':') != !strchr(pattern, ':')
	|| pattern[strcspn(pattern, ":/")] == 0
	|| pattern[strspn(pattern, V4_ADDR_STRING_CHARS)] == 0
	|| pattern[strspn(pattern, V6_ADDR_STRING_CHARS "[]/")] != 0)
	return (0);

    /*
     * No escape from expensive operations: either we have a net/mask
     * pattern, or we have an address that can have multiple valid
     * representations (e.g., 0:0:0:0:0:0:0:1 versus ::1, etc.). The only way
     * to find out if the address matches the pattern is to transform
     * everything into to binary form, and to do the comparison there.
     */
    saved_patt = mystrdup(pattern);
    err = cidr_match_parse(&match_info, saved_patt, (VSTRING *) 0);
    myfree(saved_patt);
    if (err != 0) {
	list->error = DICT_ERR_RETRY;
	rc = match_error(list, "%s", vstring_str(err));
	vstring_free(err);
	return (rc);
    }
    return (cidr_match_execute(&match_info, addr) != 0);
}
