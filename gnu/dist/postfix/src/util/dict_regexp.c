/*++
/* NAME
/*	dict_regexp 3
/* SUMMARY
/*	dictionary manager interface to REGEXP regular expression library
/* SYNOPSIS
/*	#include <dict_regexp.h>
/*
/*	DICT	*dict_regexp_open(name, dummy, dict_flags)
/*	const char *name;
/*	int	dummy;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_regexp_open() opens the named file and compiles the contained
/*	regular expressions.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* AUTHOR(S)
/*	LaMont Jones
/*	lamont@hp.com
/*
/*	Based on PCRE dictionary contributed by Andrew McNamara
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

/* System library. */

#include "sys_defs.h"

#ifdef HAS_POSIX_REGEXP

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "safe.h"
#include "vstream.h"
#include "vstring.h"
#include "stringops.h"
#include "readlline.h"
#include "dict.h"
#include "dict_regexp.h"
#include "mac_parse.h"

typedef struct dict_regexp_list {
    struct dict_regexp_list *next;	/* Next regexp in dict */
    regex_t *expr[2];			/* The compiled pattern(s) */
    char   *replace;			/* Replacement string */
    int     lineno;			/* Source file line number */
} DICT_REGEXP_RULE;

typedef struct {
    DICT    dict;			/* generic members */
    char   *map;			/* map name */
    int     flags;			/* unused at the moment */
    regmatch_t *pmatch;			/* Cut substrings */
    int     nmatch;			/* number of elements in pmatch */
    DICT_REGEXP_RULE *head;		/* first rule */
} DICT_REGEXP;

/*
 * Context for macro expansion callback.
 */
struct dict_regexp_context {
    DICT_REGEXP *dict;			/* the dictionary entry */
    DICT_REGEXP_RULE *rule;		/* the rule we matched */
    VSTRING *buf;			/* target string buffer */
    const char *subject;		/* str against which we match */
};

/*
 * Macro expansion callback - replace $0-${99} with strings cut from
 * matched string.
 */
static int dict_regexp_action(int type, VSTRING *buf, char *ptr)
{
    struct dict_regexp_context *ctxt = (struct dict_regexp_context *) ptr;
    DICT_REGEXP_RULE *rule = ctxt->rule;
    DICT_REGEXP *dict = ctxt->dict;
    int     n;

    if (type == MAC_PARSE_VARNAME) {
	n = atoi(vstring_str(buf));
	if (n >= dict->nmatch)
	    msg_fatal("regexp %s, line %d: replacement index out of range",
		      dict->dict.name, rule->lineno);
	if (dict->pmatch[n].rm_so < 0 ||
	    dict->pmatch[n].rm_so == dict->pmatch[n].rm_eo) {
	    return (MAC_PARSE_UNDEF);		/* empty string or not
						 * matched */
	}
	vstring_strncat(ctxt->buf, ctxt->subject + dict->pmatch[n].rm_so,
			dict->pmatch[n].rm_eo - dict->pmatch[n].rm_so);
    } else
	/* Straight text - duplicate with no substitution */
	vstring_strcat(ctxt->buf, vstring_str(buf));
    return (0);
}

/*
 * Look up regexp dict and perform string substitution on matched
 * strings.
 */
static const char *dict_regexp_lookup(DICT *dict, const char *name)
{
    DICT_REGEXP *dict_regexp = (DICT_REGEXP *) dict;
    DICT_REGEXP_RULE *rule;
    struct dict_regexp_context ctxt;
    static VSTRING *buf;
    int     error;

    dict_errno = 0;

    if (msg_verbose)
	msg_info("dict_regexp_lookup: %s: %s", dict_regexp->dict.name, name);

    /* Search for a matching expression */
    for (rule = dict_regexp->head; rule; rule = rule->next) {
	error = regexec(rule->expr[0], name, rule->expr[0]->re_nsub + 1,
			dict_regexp->pmatch, 0);
	if (!error) {
	    if (rule->expr[1]) {
		error = regexec(rule->expr[1], name, rule->expr[1]->re_nsub + 1,
		       dict_regexp->pmatch + rule->expr[0]->re_nsub + 1, 0);
		if (!error) {
		    continue;
		} else if (error != REG_NOMATCH) {
		    char    errbuf[256];

		    (void) regerror(error, rule->expr[1], errbuf, sizeof(errbuf));
		    msg_fatal("regexp map %s, line %d: %s.",
			      dict_regexp->dict.name, rule->lineno, errbuf);
		}
	    }

	    /*
	     * We found a match. Do some final initialization on the
	     * subexpression fields, and perform substitution on replacement
	     * string
	     */
	    if (!buf)
		buf = vstring_alloc(10);
	    VSTRING_RESET(buf);
	    ctxt.buf = buf;
	    ctxt.subject = name;
	    ctxt.rule = rule;
	    ctxt.dict = dict_regexp;

	    if (mac_parse(rule->replace, dict_regexp_action, (char *) &ctxt) & MAC_PARSE_ERROR)
		msg_fatal("regexp map %s, line %d: bad replacement syntax.",
			  dict_regexp->dict.name, rule->lineno);

	    VSTRING_TERMINATE(buf);
	    return (vstring_str(buf));
	} else if (error && error != REG_NOMATCH) {
	    char    errbuf[256];

	    (void) regerror(error, rule->expr[0], errbuf, sizeof(errbuf));
	    msg_fatal("regexp map %s, line %d: %s.",
		      dict_regexp->dict.name, rule->lineno, errbuf);
	    return ((char *) 0);
	}
    }
    return ((char *) 0);
}

/* dict_regexp_close - close regexp dictionary */

static void dict_regexp_close(DICT *dict)
{
    DICT_REGEXP *dict_regexp = (DICT_REGEXP *) dict;
    DICT_REGEXP_RULE *rule,
           *next;
    int     i;

    for (rule = dict_regexp->head; rule; rule = next) {
	next = rule->next;
	for (i = 0; i < 2; i++) {
	    if (rule->expr[i]) {
		regfree(rule->expr[i]);
		myfree((char *) rule->expr[i]);
	    }
	}
	myfree((char *) rule->replace);
	myfree((char *) rule);
    }
    if (dict_regexp->pmatch)
	myfree((char *) dict_regexp->pmatch);
    dict_free(dict);
}

static regex_t *dict_regexp_get_expr(int lineno, char **bufp, VSTREAM *map_fp)
{
    char   *p = *bufp,
           *regexp,
            re_delim;
    int     re_options,
            error;
    regex_t *expr;

    re_delim = *p++;
    regexp = p;

    /* Search for second delimiter, handling backslash escape */
    while (*p) {
	if (*p == '\\') {
	    if (p[1])
		p++;
	    else
		break;
	} else if (*p == re_delim) {
	    break;
	}
	++p;
    }
    if (!*p) {
	msg_warn("%s, line %d: no closing regexp delimiter: %c",
		 VSTREAM_PATH(map_fp), lineno, re_delim);
	return NULL;
    }
    *p++ = '\0';				/* Null term the regexp */

    re_options = REG_EXTENDED | REG_ICASE;
    while (*p) {
	if (!*p || ISSPACE(*p) || (*p == '!' && p[1] == re_delim)) {
	    /* end of the regexp */
	    expr = (regex_t *) mymalloc(sizeof(*expr));
	    error = regcomp(expr, regexp, re_options);
	    if (error != 0) {
		char    errbuf[256];

		(void) regerror(error, expr, errbuf, sizeof(errbuf));
		msg_warn("%s, line %d: error in regexp: %s.",
			 VSTREAM_PATH(map_fp), lineno, errbuf);
		myfree((char *) expr);
		return NULL;
	    }
	    *bufp = p;
	    return expr;
	} else {
	    switch (*p) {
	    case 'i':
		re_options ^= REG_ICASE;
		break;
	    case 'm':
		re_options ^= REG_NEWLINE;
		break;
	    case 'x':
		re_options ^= REG_EXTENDED;
		break;
	    default:
		msg_warn("%s, line %d: unknown regexp option '%c'",
			 VSTREAM_PATH(map_fp), lineno, *p);
	    }
	    ++p;
	}
    }
    return NULL;
}

static DICT_REGEXP_RULE *dict_regexp_parseline(int lineno, char *line, int *nsub, VSTREAM *map_fp)
{
    DICT_REGEXP_RULE *rule;
    char   *p,
            re_delim;
    regex_t *expr1,
           *expr2;
    int     total_nsub;

    p = line;
    re_delim = *p;

    expr1 = dict_regexp_get_expr(lineno, &p, map_fp);
    if (!expr1) {
	return NULL;
    } else if (*p == '!' && p[1] == re_delim) {
	p++;
	expr2 = dict_regexp_get_expr(lineno, &p, map_fp);
	if (!expr2) {
	    myfree((char *) expr1);
	    return NULL;
	}
	total_nsub = expr1->re_nsub + expr2->re_nsub + 2;
    } else {
	expr2 = NULL;
	total_nsub = expr1->re_nsub + 1;
    }
    if (nsub)
	*nsub = total_nsub;

    if (!ISSPACE(*p)) {
	msg_warn("%s, line %d: Too many expressions.",
		 VSTREAM_PATH(map_fp), lineno);
	myfree((char *) expr1);
	if (expr2)
	    myfree((char *) expr2);
	return NULL;
    }
    rule = (DICT_REGEXP_RULE *) mymalloc(sizeof(DICT_REGEXP_RULE));

    while (*p && ISSPACE(*p))
	++p;

    if (!*p) {
	msg_warn("%s, line %d: no replacement text: using empty string",
		 VSTREAM_PATH(map_fp), lineno);
	p = "";
    }
    rule->expr[0] = expr1;
    rule->expr[1] = expr2;
    rule->replace = mystrdup(p);
    rule->lineno = lineno;
    rule->next = NULL;
    return rule;
}

/*
 * dict_regexp_open - load and compile a file containing regular expressions
 */
DICT   *dict_regexp_open(const char *map, int unused_flags, int dict_flags)
{
    DICT_REGEXP *dict_regexp;
    VSTREAM *map_fp;
    VSTRING *line_buffer;
    DICT_REGEXP_RULE *rule,
           *last_rule = NULL;
    int     lineno = 0;
    int     max_nsub = 0;
    int     nsub;
    char   *p;

    line_buffer = vstring_alloc(100);

    dict_regexp = (DICT_REGEXP *) dict_alloc(DICT_TYPE_REGEXP, map,
					     sizeof(*dict_regexp));
    dict_regexp->dict.lookup = dict_regexp_lookup;
    dict_regexp->dict.close = dict_regexp_close;
    dict_regexp->dict.flags = dict_flags | DICT_FLAG_PATTERN;
    dict_regexp->head = 0;
    dict_regexp->pmatch = 0;

    if ((map_fp = vstream_fopen(map, O_RDONLY, 0)) == 0) {
	msg_fatal("open %s: %m", map);
    }
    while (readlline(line_buffer, map_fp, &lineno, READLL_STRIPNL)) {
	p = vstring_str(line_buffer);

	if (*p == '#')				/* Skip comments */
	    continue;

	if (*p == 0)				/* Skip blank lines */
	    continue;

	rule = dict_regexp_parseline(lineno, p, &nsub, map_fp);
	if (rule) {
	    if (nsub > max_nsub)
		max_nsub = nsub;

	    if (last_rule == NULL)
		dict_regexp->head = rule;
	    else
		last_rule->next = rule;
	    last_rule = rule;
	}
    }

    if (max_nsub > 0)
	dict_regexp->pmatch =
	    (regmatch_t *) mymalloc(sizeof(regmatch_t) * max_nsub);
    dict_regexp->nmatch = max_nsub;

    vstring_free(line_buffer);
    vstream_fclose(map_fp);

    return (DICT_DEBUG(&dict_regexp->dict));
}

#endif
