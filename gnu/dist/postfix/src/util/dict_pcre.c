/*++
/* NAME
/*	dict_pcre 3
/* SUMMARY
/*	dictionary manager interface to PCRE regular expression library
/* SYNOPSIS
/*	#include <dict_pcre.h>
/*
/*	DICT	*dict_pcre_open(name, dummy, dict_flags)
/*	const char *name;
/*	int	dummy;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_pcre_open() opens the named file and compiles the contained
/*	regular expressions.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* AUTHOR(S)
/*	Andrew McNamara
/*	andrewm@connect.com.au
/*	connect.com.au Pty. Ltd.
/*	Level 3, 213 Miller St
/*	North Sydney, NSW, Australia
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#include "sys_defs.h"

#ifdef HAS_PCRE

/* System library. */

#include <stdio.h>			/* sprintf() prototype */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "safe.h"
#include "vstream.h"
#include "vstring.h"
#include "stringops.h"
#include "readlline.h"
#include "dict.h"
#include "dict_pcre.h"
#include "mac_parse.h"

/* PCRE library */

#include "pcre.h"

#define PCRE_MAX_CAPTURE	99	/* Max strings captured by regexp - */
 /* essentially the max number of (..) */

struct dict_pcre_list {
    pcre   *pattern;			/* The compiled pattern */
    pcre_extra *hints;			/* Hints to speed pattern execution */
    char   *replace;			/* Replacement string */
    int     lineno;			/* Source file line number */
    struct dict_pcre_list *next;	/* Next regexp in dict */
};

typedef struct {
    DICT    dict;			/* generic members */
    struct dict_pcre_list *head;
} DICT_PCRE;

static  dict_pcre_init = 0;		/* flag need to init pcre library */

/*
 * Context for macro expansion callback.
 */
struct dict_pcre_context {
    const char *dict_name;		/* source dict name */
    int     lineno;			/* source file line number */
    VSTRING *buf;			/* target string buffer */
    const char *subject;		/* str against which we match */
    int     offsets[PCRE_MAX_CAPTURE * 3];	/* Cut substrings */
    int     matches;			/* Count of cuts */
};

/*
 * Macro expansion callback - replace $0-${99} with strings cut from
 * matched string.
 */
static int dict_pcre_action(int type, VSTRING *buf, char *ptr)
{
    struct dict_pcre_context *ctxt = (struct dict_pcre_context *) ptr;
    const char *pp;
    int     n,
            ret;

    if (type == MAC_PARSE_VARNAME) {
	n = atoi(vstring_str(buf));
	ret = pcre_get_substring(ctxt->subject, ctxt->offsets, ctxt->matches,
				 n, &pp);
	if (ret < 0) {
	    if (ret == PCRE_ERROR_NOSUBSTRING)
		msg_fatal("regexp %s, line %d: replace index out of range",
			  ctxt->dict_name, ctxt->lineno);
	    else
		msg_fatal("regexp %s, line %d: pcre_get_substring error: %d",
			  ctxt->dict_name, ctxt->lineno, ret);
	}
	if (*pp == 0) {
	    myfree((char *) pp);
	    return (MAC_PARSE_UNDEF);
	}
	vstring_strcat(ctxt->buf, pp);
	myfree((char *) pp);
	return (0);
    } else
	/* Straight text - duplicate with no substitution */
	vstring_strcat(ctxt->buf, vstring_str(buf));

    return (0);
}

/*
 * Look up regexp dict and perform string substitution on matched
 * strings.
 */
static const char *dict_pcre_lookup(DICT *dict, const char *name)
{
    DICT_PCRE *dict_pcre = (DICT_PCRE *) dict;
    struct dict_pcre_list *pcre_list;
    int     name_len = strlen(name);
    struct dict_pcre_context ctxt;
    static VSTRING *buf;

    dict_errno = 0;

    if (msg_verbose)
	msg_info("dict_pcre_lookup: %s: %s", dict_pcre->dict.name, name);

    /* Search for a matching expression */
    ctxt.matches = 0;
    for (pcre_list = dict_pcre->head; pcre_list; pcre_list = pcre_list->next) {
	if (pcre_list->pattern) {
	    ctxt.matches = pcre_exec(pcre_list->pattern, pcre_list->hints,
		  name, name_len, 0, 0, ctxt.offsets, PCRE_MAX_CAPTURE * 3);
	    if (ctxt.matches != PCRE_ERROR_NOMATCH) {
		if (ctxt.matches > 0)
		    break;			/* Got a match! */
		else {
		    /* An error */
		    switch (ctxt.matches) {
		    case 0:
			msg_warn("pcre map %s, line %d: too many (...)",
				 dict_pcre->dict.name, pcre_list->lineno);
			break;
		    case PCRE_ERROR_NULL:
		    case PCRE_ERROR_BADOPTION:
			msg_fatal("pcre map %s, line %d: bad args to re_exec",
				  dict_pcre->dict.name, pcre_list->lineno);
			break;
		    case PCRE_ERROR_BADMAGIC:
		    case PCRE_ERROR_UNKNOWN_NODE:
			msg_fatal("pcre map %s, line %d: corrupt compiled regexp",
				  dict_pcre->dict.name, pcre_list->lineno);
			break;
		    default:
			msg_fatal("pcre map %s, line %d: unknown re_exec error: %d",
				  dict_pcre->dict.name, pcre_list->lineno, ctxt.matches);
			break;
		    }
		    return ((char *) 0);
		}
	    }
	}
    }

    /* If we've got a match, */
    if (ctxt.matches > 0) {
	/* Then perform substitution on replacement string */
	if (buf == 0)
	    buf = vstring_alloc(10);
	VSTRING_RESET(buf);
	ctxt.buf = buf;
	ctxt.subject = name;
	ctxt.dict_name = dict_pcre->dict.name;
	ctxt.lineno = pcre_list->lineno;

	if (mac_parse(pcre_list->replace, dict_pcre_action, (char *) &ctxt) & MAC_PARSE_ERROR)
	    msg_fatal("pcre map %s, line %d: bad replacement syntax",
		      dict_pcre->dict.name, pcre_list->lineno);

	VSTRING_TERMINATE(buf);
	return (vstring_str(buf));
    }
    return ((char *) 0);
}

/* dict_pcre_close - close pcre dictionary */

static void dict_pcre_close(DICT *dict)
{
    DICT_PCRE *dict_pcre = (DICT_PCRE *) dict;
    struct dict_pcre_list *pcre_list;
    struct dict_pcre_list *next;

    for (pcre_list = dict_pcre->head; pcre_list; pcre_list = next) {
	next = pcre_list->next;
	if (pcre_list->pattern)
	    myfree((char *) pcre_list->pattern);
	if (pcre_list->hints)
	    myfree((char *) pcre_list->hints);
	if (pcre_list->replace)
	    myfree((char *) pcre_list->replace);
	myfree((char *) pcre_list);
    }
    dict_free(dict);
}

/*
 * dict_pcre_open - load and compile a file containing regular expressions
 */
DICT   *dict_pcre_open(const char *map, int unused_flags, int dict_flags)
{
    DICT_PCRE *dict_pcre;
    VSTREAM *map_fp;
    VSTRING *line_buffer;
    struct dict_pcre_list *pcre_list = NULL,
           *pl;
    int     lineno = 0;
    char   *regexp,
           *p,
            re_delimiter;
    int     re_options;
    pcre   *pattern;
    pcre_extra *hints;
    const char *error;
    int     errptr;

    line_buffer = vstring_alloc(100);

    dict_pcre = (DICT_PCRE *) dict_alloc(DICT_TYPE_PCRE, map,
					 sizeof(*dict_pcre));
    dict_pcre->dict.lookup = dict_pcre_lookup;
    dict_pcre->dict.close = dict_pcre_close;
    dict_pcre->dict.flags = dict_flags | DICT_FLAG_PATTERN;
    dict_pcre->head = NULL;

    if (dict_pcre_init == 0) {
	pcre_malloc = (void *(*) (size_t)) mymalloc;
	pcre_free = (void (*) (void *)) myfree;
	dict_pcre_init = 1;
    }
    if ((map_fp = vstream_fopen(map, O_RDONLY, 0)) == 0) {
	msg_fatal("open %s: %m", map);
    }
    while (readlline(line_buffer, map_fp, &lineno, READLL_STRIPNL)) {

	if (*vstring_str(line_buffer) == '#')	/* Skip comments */
	    continue;

	if (*vstring_str(line_buffer) == 0)	/* Skip blank lines */
	    continue;

	p = vstring_str(line_buffer);
	trimblanks(p, 0)[0] = 0;		/* Trim space at end */
	re_delimiter = *p++;
	regexp = p;

	/* Search for second delimiter, handling backslash escape */
	while (*p) {
	    if (*p == '\\') {
		++p;
		if (*p == 0)
		    break;
	    } else if (*p == re_delimiter)
		break;
	    ++p;
	}

	if (!*p) {
	    msg_warn("%s, line %d: no closing regexp delimiter: %c",
		     VSTREAM_PATH(map_fp), lineno, re_delimiter);
	    continue;
	}
	*p++ = '\0';				/* Null term the regexp */

	/* Now parse any regexp options */
	re_options = PCRE_CASELESS;
	while (*p && !ISSPACE(*p)) {
	    switch (*p) {
	    case 'i':
		re_options ^= PCRE_CASELESS;
		break;
	    case 'm':
		re_options ^= PCRE_MULTILINE;
		break;
	    case 's':
		re_options ^= PCRE_DOTALL;
		break;
	    case 'x':
		re_options ^= PCRE_EXTENDED;
		break;
	    case 'A':
		re_options ^= PCRE_ANCHORED;
		break;
	    case 'E':
		re_options ^= PCRE_DOLLAR_ENDONLY;
		break;
	    case 'U':
		re_options ^= PCRE_UNGREEDY;
		break;
	    case 'X':
		re_options ^= PCRE_EXTRA;
		break;
	    default:
		msg_warn("%s, line %d: unknown regexp option '%c'",
			 VSTREAM_PATH(map_fp), lineno, *p);
	    }
	    ++p;
	}

	while (*p && ISSPACE(*p))
	    ++p;

	if (!*p) {
	    msg_warn("%s, line %d: no replacement text",
		     VSTREAM_PATH(map_fp), lineno);
	    p = "";
	}
	/* Compile the patern */
	pattern = pcre_compile(regexp, re_options, &error, &errptr, NULL);
	if (pattern == NULL) {
	    msg_warn("%s, line %d: error in regex at offset %d: %s",
		     VSTREAM_PATH(map_fp), lineno, errptr, error);
	    continue;
	}
	hints = pcre_study(pattern, 0, &error);
	if (error != NULL) {
	    msg_warn("%s, line %d: error while studying regex: %s",
		     VSTREAM_PATH(map_fp), lineno, error);
	    myfree((char *) pattern);
	    continue;
	}
	/* Add it to the list */
	pl = (struct dict_pcre_list *) mymalloc(sizeof(struct dict_pcre_list));

	/* Save the replacement string (if any) */
	pl->replace = mystrdup(p);
	pl->pattern = pattern;
	pl->hints = hints;
	pl->next = NULL;
	pl->lineno = lineno;

	if (pcre_list == NULL)
	    dict_pcre->head = pl;
	else
	    pcre_list->next = pl;
	pcre_list = pl;
    }

    vstring_free(line_buffer);
    vstream_fclose(map_fp);

    return (DICT_DEBUG(&dict_pcre->dict));
}

#endif					/* HAS_PCRE */
