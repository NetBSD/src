/*++
/* NAME
/*	dict_cidr 3
/* SUMMARY
/*	Dictionary interface for CIDR data
/* SYNOPSIS
/*	#include <dict_cidr.h>
/*
/*	DICT	*dict_cidr_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_cidr_open() opens the named file and stores
/*	the key/value pairs where the key must be either a
/*	"naked" IP address or a netblock in CIDR notation.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* AUTHOR(S)
/*	Jozsef Kadlecsik
/*	kadlec@blackhole.kfki.hu
/*	KFKI Research Institute for Particle and Nuclear Physics
/*	POB. 49
/*	1525 Budapest, Hungary
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* Utility library. */

#include <mymalloc.h>
#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <stringops.h>
#include <readlline.h>
#include <dict.h>
#include <dict_cidr.h>
#include <split_at.h>

/* Application-specific. */

 /*
  * Each rule in a CIDR table is parsed and stored in a linked list.
  * Obviously all this is IPV4 specific and needs to be redone for IPV6.
  */
typedef struct DICT_CIDR_ENTRY {
    unsigned long net_bits;		/* network portion of address */
    unsigned long mask_bits;		/* network mask */
    char   *value;			/* lookup result */
    struct DICT_CIDR_ENTRY *next;	/* next entry */
} DICT_CIDR_ENTRY;

typedef struct {
    DICT    dict;			/* generic members */
    DICT_CIDR_ENTRY *head;		/* first entry */
} DICT_CIDR;

#define BITS_PER_ADDR   32

/* dict_cidr_lookup - CIDR table lookup */

static const char *dict_cidr_lookup(DICT *dict, const char *key)
{
    DICT_CIDR *dict_cidr = (DICT_CIDR *) dict;
    DICT_CIDR_ENTRY *entry;
    unsigned long addr;

    if (msg_verbose)
	msg_info("dict_cidr_lookup: %s: %s", dict_cidr->dict.name, key);

    if ((addr = inet_addr(key)) == INADDR_NONE)
	return (0);

    for (entry = dict_cidr->head; entry; entry = entry->next)
	if ((addr & entry->mask_bits) == entry->net_bits)
	    return (entry->value);

    return (0);
}

/* dict_cidr_close - close the CIDR table */

static void dict_cidr_close(DICT *dict)
{
    DICT_CIDR *dict_cidr = (DICT_CIDR *) dict;
    DICT_CIDR_ENTRY *entry;
    DICT_CIDR_ENTRY *next;

    for (entry = dict_cidr->head; entry; entry = next) {
	next = entry->next;
	myfree(entry->value);
	myfree((char *) entry);
    }
    dict_free(dict);
}

/* dict_cidr_parse_rule - parse CIDR table rule into network, mask and value */

static DICT_CIDR_ENTRY *dict_cidr_parse_rule(const char *mapname, int lineno,
					             char *p)
{
    DICT_CIDR_ENTRY *rule;
    char   *key;
    char   *value;
    char   *mask;
    int     mask_shift;
    unsigned long net_bits;
    unsigned long mask_bits;
    struct in_addr net_addr;

    /*
     * Split the rule into key and value. We already eliminated leading
     * whitespace, comments, empty lines or lines with whitespace only. This
     * means a null key can't happen but we will handle this anyway.
     */
    key = p;
    while (*p && !ISSPACE(*p))			/* Skip over key */
	p++;
    if (*p)					/* Terminate key */
	*p++ = 0;
    while (*p && ISSPACE(*p))			/* Skip whitespace */
	p++;
    value = p;
    trimblanks(value, 0)[0] = 0;		/* Trim trailing blanks */
    if (*key == 0) {
	msg_warn("cidr map %s, line %d: no address pattern: skipping this rule",
		 mapname, lineno);
	return (0);
    }
    if (*value == 0) {
	msg_warn("cidr map %s, line %d: no lookup result: skipping this rule",
		 mapname, lineno);
	return (0);
    }

    /*
     * Parse the key into network and mask, and destroy the key. Treat a bare
     * network address as /32.
     * 
     * We need explicit code for /0. The result of << is undefined when the
     * shift is greater or equal to the number of bits in the shifted
     * operand.
     */
    if ((mask = split_at(key, '/')) != 0) {
	if (!alldig(mask) || (mask_shift = atoi(mask)) > BITS_PER_ADDR
	    || (net_bits = inet_addr(key)) == INADDR_NONE) {
	    msg_warn("cidr map %s, line %d: bad net/mask pattern: \"%s/%s\": "
		     "skipping this rule", mapname, lineno, key, mask);
	    return (0);
	}
	mask_bits = mask_shift > 0 ?
	    htonl((0xffffffff) << (BITS_PER_ADDR - mask_shift)) : 0;
	if (net_bits & ~mask_bits) {
	    net_addr.s_addr = (net_bits & mask_bits);
	    msg_warn("cidr map %s, line %d: net/mask pattern \"%s/%s\" with "
		     "non-null host portion: skipping this rule",
		     mapname, lineno, key, mask);
	    msg_warn("specify \"%s/%d\" if this is really what you want",
		     inet_ntoa(net_addr), mask_shift);
	    return (0);
	}
    } else {
	if ((net_bits = inet_addr(key)) == INADDR_NONE) {
	    msg_warn("cidr map %s, line %d: bad address pattern: \"%s\": "
		     "skipping this rule", mapname, lineno, key);
	    return (0);
	}
	mask_shift = 32;
	mask_bits = htonl(0xffffffff);
    }

    /*
     * Bundle up the result.
     */
    rule = (DICT_CIDR_ENTRY *) mymalloc(sizeof(DICT_CIDR_ENTRY));
    rule->net_bits = net_bits;
    rule->mask_bits = mask_bits;
    rule->value = mystrdup(value);
    rule->next = 0;

    if (msg_verbose)
	msg_info("dict_cidr_open: %s: %lu/%d %s",
		 mapname, rule->net_bits, mask_shift, rule->value);

    return (rule);
}

/* dict_cidr_open - parse CIDR table */

DICT   *dict_cidr_open(const char *mapname, int open_flags, int dict_flags)
{
    DICT_CIDR *dict_cidr;
    VSTREAM *map_fp;
    VSTRING *line_buffer = vstring_alloc(100);
    DICT_CIDR_ENTRY *rule;
    DICT_CIDR_ENTRY *last_rule = 0;
    int     lineno = 0;

    /*
     * Sanity checks.
     */
    if (open_flags != O_RDONLY)
	msg_fatal("%s:%s map requires O_RDONLY access mode",
		  DICT_TYPE_CIDR, mapname);

    /*
     * XXX Eliminate unnecessary queries by setting a flag that says "this
     * map matches network addresses only".
     */
    dict_cidr = (DICT_CIDR *) dict_alloc(DICT_TYPE_CIDR, mapname,
					 sizeof(*dict_cidr));
    dict_cidr->dict.lookup = dict_cidr_lookup;
    dict_cidr->dict.close = dict_cidr_close;
    dict_cidr->dict.flags = dict_flags | DICT_FLAG_PATTERN;
    dict_cidr->head = 0;

    if ((map_fp = vstream_fopen(mapname, O_RDONLY, 0)) == 0)
	msg_fatal("open %s: %m", mapname);

    while (readlline(line_buffer, map_fp, &lineno)) {
	rule = dict_cidr_parse_rule(mapname, lineno, vstring_str(line_buffer));
	if (rule == 0)
	    continue;
	if (last_rule == 0)
	    dict_cidr->head = rule;
	else
	    last_rule->next = rule;
	last_rule = rule;
    }

    /*
     * Clean up.
     */
    if (vstream_fclose(map_fp))
	msg_fatal("cidr map %s: read error: %m", mapname);
    vstring_free(line_buffer);

    return (DICT_DEBUG (&dict_cidr->dict));
}
