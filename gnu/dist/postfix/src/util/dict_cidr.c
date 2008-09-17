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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Utility library. */

#include <mymalloc.h>
#include <msg.h>
#include <vstream.h>
#include <vstring.h>
#include <stringops.h>
#include <readlline.h>
#include <dict.h>
#include <myaddrinfo.h>
#include <cidr_match.h>
#include <dict_cidr.h>

/* Application-specific. */

 /*
  * Each rule in a CIDR table is parsed and stored in a linked list.
  */
typedef struct DICT_CIDR_ENTRY {
    CIDR_MATCH cidr_info;		/* must be first */
    char   *value;			/* lookup result */
} DICT_CIDR_ENTRY;

typedef struct {
    DICT    dict;			/* generic members */
    DICT_CIDR_ENTRY *head;		/* first entry */
} DICT_CIDR;

/* dict_cidr_lookup - CIDR table lookup */

static const char *dict_cidr_lookup(DICT *dict, const char *key)
{
    DICT_CIDR *dict_cidr = (DICT_CIDR *) dict;
    DICT_CIDR_ENTRY *entry;

    if (msg_verbose)
	msg_info("dict_cidr_lookup: %s: %s", dict->name, key);

    dict_errno = 0;

    if ((entry = (DICT_CIDR_ENTRY *)
	 cidr_match_execute(&(dict_cidr->head->cidr_info), key)) != 0)
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
	next = (DICT_CIDR_ENTRY *) entry->cidr_info.next;
	myfree(entry->value);
	myfree((char *) entry);
    }
    dict_free(dict);
}

/* dict_cidr_parse_rule - parse CIDR table rule into network, mask and value */

static DICT_CIDR_ENTRY *dict_cidr_parse_rule(char *p, VSTRING *why)
{
    DICT_CIDR_ENTRY *rule;
    char   *pattern;
    char   *value;
    CIDR_MATCH cidr_info;
    MAI_HOSTADDR_STR hostaddr;

    /*
     * Split the rule into key and value. We already eliminated leading
     * whitespace, comments, empty lines or lines with whitespace only. This
     * means a null key can't happen but we will handle this anyway.
     */
    pattern = p;
    while (*p && !ISSPACE(*p))			/* Skip over key */
	p++;
    if (*p)					/* Terminate key */
	*p++ = 0;
    while (*p && ISSPACE(*p))			/* Skip whitespace */
	p++;
    value = p;
    trimblanks(value, 0)[0] = 0;		/* Trim trailing blanks */
    if (*pattern == 0) {
	vstring_sprintf(why, "no address pattern");
	return (0);
    }
    if (*value == 0) {
	vstring_sprintf(why, "no lookup result");
	return (0);
    }

    /*
     * Parse the pattern, destroying it in the process.
     */
    if (cidr_match_parse(&cidr_info, pattern, why) != 0)
	return (0);

    /*
     * Bundle up the result.
     */
    rule = (DICT_CIDR_ENTRY *) mymalloc(sizeof(DICT_CIDR_ENTRY));
    rule->cidr_info = cidr_info;
    rule->value = mystrdup(value);

    if (msg_verbose) {
	if (inet_ntop(cidr_info.addr_family, cidr_info.net_bytes,
		      hostaddr.buf, sizeof(hostaddr.buf)) == 0)
	    msg_fatal("inet_ntop: %m");
	msg_info("dict_cidr_open: add %s/%d %s",
		 hostaddr.buf, cidr_info.mask_shift, rule->value);
    }
    return (rule);
}

/* dict_cidr_open - parse CIDR table */

DICT   *dict_cidr_open(const char *mapname, int open_flags, int dict_flags)
{
    DICT_CIDR *dict_cidr;
    VSTREAM *map_fp;
    VSTRING *line_buffer = vstring_alloc(100);
    VSTRING *why = vstring_alloc(100);
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
	rule = dict_cidr_parse_rule(vstring_str(line_buffer), why);
	if (rule == 0) {
	    msg_warn("cidr map %s, line %d: %s: skipping this rule",
		     mapname, lineno, vstring_str(why));
	    continue;
	}
	if (last_rule == 0)
	    dict_cidr->head = rule;
	else
	    last_rule->cidr_info.next = &(rule->cidr_info);
	last_rule = rule;
    }

    /*
     * Clean up.
     */
    if (vstream_fclose(map_fp))
	msg_fatal("cidr map %s: read error: %m", mapname);
    vstring_free(line_buffer);
    vstring_free(why);

    return (DICT_DEBUG (&dict_cidr->dict));
}
