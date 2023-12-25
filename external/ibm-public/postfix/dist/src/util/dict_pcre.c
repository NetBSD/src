/*	$NetBSD: dict_pcre.c,v 1.4.2.1 2023/12/25 12:43:37 martin Exp $	*/

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
/*	regular expressions. The result object can be used to match strings
/*	against the table.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/*	pcre_table(5) PCRE table configuration
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#include "sys_defs.h"

#ifdef HAS_PCRE

/* System library. */

#include <sys/stat.h>
#include <stdio.h>			/* sprintf() prototype */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

#if HAS_PCRE == 1
#include <pcre.h>
#elif HAS_PCRE == 2
#define PCRE2_CODE_UNIT_WIDTH	8
#include <pcre2.h>
#else
#error "define HAS_PCRE=2 or HAS_PCRE=1"
#endif

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
#include "warn_stat.h"
#include "mvect.h"

 /*
  * Backwards compatibility.
  */
#if HAS_PCRE == 1
 /* PCRE Legacy JIT supprt. */
#ifdef PCRE_STUDY_JIT_COMPILE
#define DICT_PCRE_FREE_STUDY(x)	pcre_free_study(x)
#else
#define DICT_PCRE_FREE_STUDY(x)	pcre_free((char *) (x))
#endif

 /* PCRE Compiled pattern. */
#define DICT_PCRE_CODE		pcre
#define DICT_PCRE_CODE_FREE(x)	myfree((void *) (x))

 /* Old-style hints versus new-style match_data. */
#define DICT_PCRE_MATCH_HINT_TYPE pcre_extra *
#define DICT_PCRE_MATCH_HINT_NAME hints
#define DICT_PCRE_MATCH_HINT(x) ((x)->DICT_PCRE_MATCH_HINT_NAME)
#define DICT_PCRE_MATCH_HINT_FREE(x) do { \
	if (DICT_PCRE_MATCH_HINT(x)) \
	    DICT_PCRE_FREE_STUDY(DICT_PCRE_MATCH_HINT(x)); \
    } while (0)

 /* PCRE Pattern options. */
#define DICT_PCRE_CASELESS	PCRE_CASELESS
#define DICT_PCRE_MULTILINE	PCRE_MULTILINE
#define DICT_PCRE_DOTALL	PCRE_DOTALL
#define DICT_PCRE_EXTENDED	PCRE_EXTENDED
#define DICT_PCRE_ANCHORED	PCRE_ANCHORED
#define DICT_PCRE_DOLLAR_ENDONLY PCRE_DOLLAR_ENDONLY
#define DICT_PCRE_UNGREEDY	PCRE_UNGREEDY
#define DICT_PCRE_EXTRA		PCRE_EXTRA

 /* PCRE Number of captures in pattern. */
#ifdef PCRE_INFO_CAPTURECOUNT
#define DICT_PCRE_CAPTURECOUNT_T int
#endif

#else					/* HAS_PCRE */

 /* PCRE2 Compiled pattern. */
#define DICT_PCRE_CODE		pcre2_code
#define DICT_PCRE_CODE_FREE(x)	pcre2_code_free(x)

 /* PCRE2 Old-style hints versus new-style match_data. */
#define DICT_PCRE_MATCH_HINT_TYPE pcre2_match_data *
#define DICT_PCRE_MATCH_HINT_NAME match_data
#define DICT_PCRE_MATCH_HINT(x)	((x)->DICT_PCRE_MATCH_HINT_NAME)
#define DICT_PCRE_MATCH_HINT_FREE(x) \
	pcre2_match_data_free(DICT_PCRE_MATCH_HINT(x))

 /* PCRE2 Pattern options. */
#define DICT_PCRE_CASELESS	PCRE2_CASELESS
#define DICT_PCRE_MULTILINE	PCRE2_MULTILINE
#define DICT_PCRE_DOTALL	PCRE2_DOTALL
#define DICT_PCRE_EXTENDED	PCRE2_EXTENDED
#define DICT_PCRE_ANCHORED	PCRE2_ANCHORED
#define DICT_PCRE_DOLLAR_ENDONLY PCRE2_DOLLAR_ENDONLY
#define DICT_PCRE_UNGREEDY	PCRE2_UNGREEDY
#define DICT_PCRE_EXTRA		0

 /* PCRE2 Number of captures in pattern. */
#define	DICT_PCRE_CAPTURECOUNT_T uint32_t

#endif					/* HAS_PCRE */

 /*
  * Support for IF/ENDIF based on an idea by Bert Driehuis.
  */
#define DICT_PCRE_OP_MATCH    1		/* Match this regexp */
#define DICT_PCRE_OP_IF       2		/* Increase if/endif nesting on match */
#define DICT_PCRE_OP_ENDIF    3		/* Decrease if/endif nesting on match */

 /*
  * Max strings captured by regexp - essentially the max number of (..)
  */
#if HAS_PCRE == 1
#define PCRE_MAX_CAPTURE	99
#endif

 /*
  * Regular expression before and after compilation.
  */
typedef struct {
    char   *regexp;			/* regular expression */
    int     options;			/* options */
    int     match;			/* positive or negative match */
} DICT_PCRE_REGEXP;

typedef struct {
    DICT_PCRE_CODE *pattern;		/* the compiled pattern */
    DICT_PCRE_MATCH_HINT_TYPE DICT_PCRE_MATCH_HINT_NAME;
} DICT_PCRE_ENGINE;

 /*
  * Compiled generic rule, and subclasses that derive from it.
  */
typedef struct DICT_PCRE_RULE {
    int     op;				/* DICT_PCRE_OP_MATCH/IF/ENDIF */
    int     lineno;			/* source file line number */
    struct DICT_PCRE_RULE *next;	/* next rule in dict */
} DICT_PCRE_RULE;

typedef struct {
    DICT_PCRE_RULE rule;		/* generic part */
    DICT_PCRE_CODE *pattern;		/* compiled pattern */
    DICT_PCRE_MATCH_HINT_TYPE DICT_PCRE_MATCH_HINT_NAME;
    char   *replacement;		/* replacement string */
    int     match;			/* positive or negative match */
    size_t  max_sub;			/* largest $number in replacement */
} DICT_PCRE_MATCH_RULE;

typedef struct {
    DICT_PCRE_RULE rule;		/* generic members */
    DICT_PCRE_CODE *pattern;		/* compiled pattern */
    DICT_PCRE_MATCH_HINT_TYPE DICT_PCRE_MATCH_HINT_NAME;
    int     match;			/* positive or negative match */
    struct DICT_PCRE_RULE *endif_rule;	/* matching endif rule */
} DICT_PCRE_IF_RULE;

 /*
  * PCRE map.
  */
typedef struct {
    DICT    dict;			/* generic members */
    DICT_PCRE_RULE *head;
    VSTRING *expansion_buf;		/* lookup result */
} DICT_PCRE;

#if HAS_PCRE == 1
static int dict_pcre_init = 0;		/* flag need to init pcre library */

#endif

/*
 * Context for $number expansion callback.
 */
typedef struct {
    DICT_PCRE *dict_pcre;		/* the dictionary handle */
#if HAS_PCRE == 1
    DICT_PCRE_MATCH_RULE *match_rule;	/* the rule we matched */
#endif
    const char *lookup_string;		/* string against which we match */
#if HAS_PCRE == 1
    int     offsets[PCRE_MAX_CAPTURE * 3];	/* Cut substrings */
#else					/* HAS_PCRE */
    PCRE2_SIZE *ovector;		/* matched string offsets */
#endif					/* HAS_PCRE */
    int     matches;			/* Count of cuts */
} DICT_PCRE_EXPAND_CONTEXT;

 /*
  * Context for $number pre-scan callback.
  */
typedef struct {
    const char *mapname;		/* name of regexp map */
    int     lineno;			/* where in file */
    size_t  max_sub;			/* Largest $n seen */
    char   *literal;			/* constant result, $$ -> $ */
} DICT_PCRE_PRESCAN_CONTEXT;

 /*
  * Compatibility.
  */
#ifndef MAC_PARSE_OK
#define MAC_PARSE_OK 0
#endif

 /*
  * Macros to make dense code more accessible.
  */
#define NULL_STARTOFFSET	(0)
#define NULL_EXEC_OPTIONS 	(0)

/* dict_pcre_expand - replace $number with matched text */

static int dict_pcre_expand(int type, VSTRING *buf, void *ptr)
{
    DICT_PCRE_EXPAND_CONTEXT *ctxt = (DICT_PCRE_EXPAND_CONTEXT *) ptr;
    DICT_PCRE *dict_pcre = ctxt->dict_pcre;
    int     n;

#if HAS_PCRE == 1
    DICT_PCRE_MATCH_RULE *match_rule = ctxt->match_rule;
    const char *pp;
    int     ret;

#else
    PCRE2_SPTR start;
    PCRE2_SIZE length;

#endif

    /*
     * Replace $0-${99} with strings cut from matched text.
     */
    if (type == MAC_PARSE_VARNAME) {
	n = atoi(vstring_str(buf));
#if HAS_PCRE == 1
	ret = pcre_get_substring(ctxt->lookup_string, ctxt->offsets,
				 ctxt->matches, n, &pp);
	if (ret < 0) {
	    if (ret == PCRE_ERROR_NOSUBSTRING)
		return (MAC_PARSE_UNDEF);
	    else
		msg_fatal("pcre map %s, line %d: pcre_get_substring error: %d",
			dict_pcre->dict.name, match_rule->rule.lineno, ret);
	}
	if (*pp == 0) {
	    myfree((void *) pp);
	    return (MAC_PARSE_UNDEF);
	}
	vstring_strcat(dict_pcre->expansion_buf, pp);
	myfree((void *) pp);
	return (MAC_PARSE_OK);
#else
	start = (unsigned char *) ctxt->lookup_string + ctxt->ovector[2 * n];
	length = ctxt->ovector[2 * n + 1] - ctxt->ovector[2 * n];
	if (length == 0)
	    return (MAC_PARSE_UNDEF);
	vstring_strncat(dict_pcre->expansion_buf, (char *) start, length);
	return (MAC_PARSE_OK);
#endif
    }

    /*
     * Straight text - duplicate with no substitution.
     */
    else {
	vstring_strcat(dict_pcre->expansion_buf, vstring_str(buf));
	return (MAC_PARSE_OK);
    }
}

#if HAS_PCRE == 2

#define DICT_PCRE_GET_ERROR_BUF_LEN	256

/* dict_pcre_get_error - convert PCRE2 error number or text */

static char *dict_pcre_get_error(VSTRING *buf, int errval)
{
    ssize_t len;

    VSTRING_SPACE(buf, DICT_PCRE_GET_ERROR_BUF_LEN);
    if ((len = pcre2_get_error_message(errval,
				       (unsigned char *) vstring_str(buf),
				       DICT_PCRE_GET_ERROR_BUF_LEN)) > 0) {
	vstring_set_payload_size(buf, len);
    } else
	vstring_sprintf(buf, "unexpected pcre2 error code %d", errval);
    return (vstring_str(buf));
}

#endif					/* HAS_PCRE == 2 */

/* dict_pcre_exec_error - report matching error */

static void dict_pcre_exec_error(const char *mapname, int lineno, int errval)
{
#if HAS_PCRE == 1
    switch (errval) {
	case 0:
	msg_warn("pcre map %s, line %d: too many (...)",
		 mapname, lineno);
	return;
    case PCRE_ERROR_NULL:
    case PCRE_ERROR_BADOPTION:
	msg_warn("pcre map %s, line %d: bad args to re_exec",
		 mapname, lineno);
	return;
    case PCRE_ERROR_BADMAGIC:
    case PCRE_ERROR_UNKNOWN_NODE:
	msg_warn("pcre map %s, line %d: corrupt compiled regexp",
		 mapname, lineno);
	return;
#ifdef PCRE_ERROR_NOMEMORY
    case PCRE_ERROR_NOMEMORY:
	msg_warn("pcre map %s, line %d: out of memory",
		 mapname, lineno);
	return;
#endif
#ifdef PCRE_ERROR_MATCHLIMIT
    case PCRE_ERROR_MATCHLIMIT:
	msg_warn("pcre map %s, line %d: backtracking limit exceeded",
		 mapname, lineno);
	return;
#endif
#ifdef PCRE_ERROR_BADUTF8
    case PCRE_ERROR_BADUTF8:
	msg_warn("pcre map %s, line %d: bad UTF-8 sequence in search string",
		 mapname, lineno);
	return;
#endif
#ifdef PCRE_ERROR_BADUTF8_OFFSET
    case PCRE_ERROR_BADUTF8_OFFSET:
	msg_warn("pcre map %s, line %d: bad UTF-8 start offset in search string",
		 mapname, lineno);
	return;
#endif
    default:
	msg_warn("pcre map %s, line %d: unknown pcre_exec error: %d",
		 mapname, lineno, errval);
	return;
    }
#else					/* HAS_PCRE */
    VSTRING *buf = vstring_alloc(DICT_PCRE_GET_ERROR_BUF_LEN);

    msg_warn("pcre map %s, line %d: %s", mapname, lineno,
	     dict_pcre_get_error(buf, errval));
    vstring_free(buf);
#endif						/* HAS_PCRE */
}

 /*
  * Inlined to reduce function call overhead in the time-critical loop.
  */
#if HAS_PCRE == 1
#define DICT_PCRE_EXEC(ctxt, map, line, pattern, hints, match, str, len) \
    ((ctxt).matches = pcre_exec((pattern), (hints), (str), (len), \
				NULL_STARTOFFSET, NULL_EXEC_OPTIONS, \
				(ctxt).offsets, PCRE_MAX_CAPTURE * 3), \
     (ctxt).matches > 0 ? (match) : \
     (ctxt).matches == PCRE_ERROR_NOMATCH ? !(match) : \
     (dict_pcre_exec_error((map), (line), (ctxt).matches), 0))
#else
#define DICT_PCRE_EXEC(ctxt, map, line, pattern, match_data, match, str, len) \
    ((ctxt).matches = pcre2_match((pattern), (unsigned char *) (str), (len), \
				NULL_STARTOFFSET, NULL_EXEC_OPTIONS, \
				(match_data), (pcre2_match_context *) 0), \
     (ctxt).matches > 0 ? (match) : \
     (ctxt).matches == PCRE2_ERROR_NOMATCH ? !(match) : \
     (dict_pcre_exec_error((map), (line), (ctxt).matches), 0))
#endif

/* dict_pcre_lookup - match string and perform optional substitution */

static const char *dict_pcre_lookup(DICT *dict, const char *lookup_string)
{
    DICT_PCRE *dict_pcre = (DICT_PCRE *) dict;
    DICT_PCRE_RULE *rule;
    DICT_PCRE_IF_RULE *if_rule;
    DICT_PCRE_MATCH_RULE *match_rule;
    int     lookup_len = strlen(lookup_string);
    DICT_PCRE_EXPAND_CONTEXT ctxt;

    dict->error = 0;

    if (msg_verbose)
	msg_info("dict_pcre_lookup: %s: %s", dict->name, lookup_string);

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_MUL) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, lookup_string);
	lookup_string = lowercase(vstring_str(dict->fold_buf));
    }
    for (rule = dict_pcre->head; rule; rule = rule->next) {

	switch (rule->op) {

	    /*
	     * Search for a matching expression.
	     */
	case DICT_PCRE_OP_MATCH:
	    match_rule = (DICT_PCRE_MATCH_RULE *) rule;
	    if (!DICT_PCRE_EXEC(ctxt, dict->name, rule->lineno,
				match_rule->pattern,
				DICT_PCRE_MATCH_HINT(match_rule),
			      match_rule->match, lookup_string, lookup_len))
		continue;

	    /*
	     * Skip $number substitutions when the replacement text contains
	     * no $number strings, as learned during the compile time
	     * pre-scan. The pre-scan already replaced $$ by $.
	     */
	    if (match_rule->max_sub == 0)
		return match_rule->replacement;

	    /*
	     * We've got a match. Perform substitution on replacement string.
	     */
	    if (dict_pcre->expansion_buf == 0)
		dict_pcre->expansion_buf = vstring_alloc(10);
	    VSTRING_RESET(dict_pcre->expansion_buf);
	    ctxt.dict_pcre = dict_pcre;
#if HAS_PCRE == 1
	    ctxt.match_rule = match_rule;
#else
	    ctxt.ovector = pcre2_get_ovector_pointer(match_rule->match_data);
#endif
	    ctxt.lookup_string = lookup_string;

	    if (mac_parse(match_rule->replacement, dict_pcre_expand,
			  (void *) &ctxt) & MAC_PARSE_ERROR)
		msg_fatal("pcre map %s, line %d: bad replacement syntax",
			  dict->name, rule->lineno);

	    VSTRING_TERMINATE(dict_pcre->expansion_buf);
	    return (vstring_str(dict_pcre->expansion_buf));

	    /*
	     * Conditional. XXX We provide space for matched substring info
	     * because PCRE uses part of it as workspace for backtracking.
	     * PCRE will allocate memory if it runs out of backtracking
	     * storage.
	     */
	case DICT_PCRE_OP_IF:
	    if_rule = (DICT_PCRE_IF_RULE *) rule;
	    if (DICT_PCRE_EXEC(ctxt, dict->name, rule->lineno,
			       if_rule->pattern,
			       DICT_PCRE_MATCH_HINT(if_rule),
			       if_rule->match, lookup_string, lookup_len))
		continue;
	    /* An IF without matching ENDIF has no "endif" rule. */
	    if ((rule = if_rule->endif_rule) == 0)
		return (0);
	    /* FALLTHROUGH */

	    /*
	     * ENDIF after IF.
	     */
	case DICT_PCRE_OP_ENDIF:
	    continue;

	default:
	    msg_panic("dict_pcre_lookup: impossible operation %d", rule->op);
	}
    }
    return (0);
}

/* dict_pcre_close - close pcre dictionary */

static void dict_pcre_close(DICT *dict)
{
    DICT_PCRE *dict_pcre = (DICT_PCRE *) dict;
    DICT_PCRE_RULE *rule;
    DICT_PCRE_RULE *next;
    DICT_PCRE_MATCH_RULE *match_rule;
    DICT_PCRE_IF_RULE *if_rule;

    for (rule = dict_pcre->head; rule; rule = next) {
	next = rule->next;
	switch (rule->op) {
	case DICT_PCRE_OP_MATCH:
	    match_rule = (DICT_PCRE_MATCH_RULE *) rule;
	    if (match_rule->pattern)
		DICT_PCRE_CODE_FREE(match_rule->pattern);
	    DICT_PCRE_MATCH_HINT_FREE(match_rule);
	    if (match_rule->replacement)
		myfree((void *) match_rule->replacement);
	    break;
	case DICT_PCRE_OP_IF:
	    if_rule = (DICT_PCRE_IF_RULE *) rule;
	    if (if_rule->pattern)
		DICT_PCRE_CODE_FREE(if_rule->pattern);
	    DICT_PCRE_MATCH_HINT_FREE(if_rule);
	    break;
	case DICT_PCRE_OP_ENDIF:
	    break;
	default:
	    msg_panic("dict_pcre_close: unknown operation %d", rule->op);
	}
	myfree((void *) rule);
    }
    if (dict_pcre->expansion_buf)
	vstring_free(dict_pcre->expansion_buf);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_pcre_get_pattern - extract pattern from rule */

static int dict_pcre_get_pattern(const char *mapname, int lineno, char **bufp,
				         DICT_PCRE_REGEXP *pattern)
{
    char   *p = *bufp;
    char    re_delimiter;

    /*
     * Process negation operators.
     */
    pattern->match = 1;
    for (;;) {
	if (*p == '!')
	    pattern->match = !pattern->match;
	else if (!ISSPACE(*p))
	    break;
	p++;
    }
    if (*p == 0) {
	msg_warn("pcre map %s, line %d: no regexp: skipping this rule",
		 mapname, lineno);
	return (0);
    }
    re_delimiter = *p++;
    pattern->regexp = p;

    /*
     * Search for second delimiter, handling backslash escape.
     */
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
	msg_warn("pcre map %s, line %d: no closing regexp delimiter \"%c\": "
		 "ignoring this rule", mapname, lineno, re_delimiter);
	return (0);
    }
    *p++ = 0;					/* Null term the regexp */

    /*
     * Parse any regexp options.
     */
    pattern->options = DICT_PCRE_CASELESS | DICT_PCRE_DOTALL;
    while (*p && !ISSPACE(*p)) {
	switch (*p) {
	case 'i':
	    pattern->options ^= DICT_PCRE_CASELESS;
	    break;
	case 'm':
	    pattern->options ^= DICT_PCRE_MULTILINE;
	    break;
	case 's':
	    pattern->options ^= DICT_PCRE_DOTALL;
	    break;
	case 'x':
	    pattern->options ^= DICT_PCRE_EXTENDED;
	    break;
	case 'A':
	    pattern->options ^= DICT_PCRE_ANCHORED;
	    break;
	case 'E':
	    pattern->options ^= DICT_PCRE_DOLLAR_ENDONLY;
	    break;
	case 'U':
	    pattern->options ^= DICT_PCRE_UNGREEDY;
	    break;
	case 'X':
#if DICT_PCRE_EXTRA != 0
	    pattern->options ^= DICT_PCRE_EXTRA;
#else
	    msg_warn("pcre map %s, line %d: ignoring obsolete regexp "
		     "option \"%c\"", mapname, lineno, *p);
#endif
	    break;
	default:
	    msg_warn("pcre map %s, line %d: unknown regexp option \"%c\": "
		     "skipping this rule", mapname, lineno, *p);
	    return (0);
	}
	++p;
    }
    *bufp = p;
    return (1);
}

/* dict_pcre_prescan - sanity check $number instances in replacement text */

static int dict_pcre_prescan(int type, VSTRING *buf, void *context)
{
    DICT_PCRE_PRESCAN_CONTEXT *ctxt = (DICT_PCRE_PRESCAN_CONTEXT *) context;
    size_t  n;

    /*
     * Keep a copy of literal text (with $$ already replaced by $) if and
     * only if the replacement text contains no $number expression. This way
     * we can avoid having to scan the replacement text at lookup time.
     */
    if (type == MAC_PARSE_VARNAME) {
	if (ctxt->literal) {
	    myfree(ctxt->literal);
	    ctxt->literal = 0;
	}
	if (!alldig(vstring_str(buf))) {
	    msg_warn("pcre map %s, line %d: non-numeric replacement index \"%s\"",
		     ctxt->mapname, ctxt->lineno, vstring_str(buf));
	    return (MAC_PARSE_ERROR);
	}
	n = atoi(vstring_str(buf));
	if (n < 1) {
	    msg_warn("pcre map %s, line %d: out of range replacement index \"%s\"",
		     ctxt->mapname, ctxt->lineno, vstring_str(buf));
	    return (MAC_PARSE_ERROR);
	}
	if (n > ctxt->max_sub)
	    ctxt->max_sub = n;
    } else if (type == MAC_PARSE_LITERAL && ctxt->max_sub == 0) {
	if (ctxt->literal)
	    msg_panic("pcre map %s, line %d: multiple literals but no $number",
		      ctxt->mapname, ctxt->lineno);
	ctxt->literal = mystrdup(vstring_str(buf));
    }
    return (MAC_PARSE_OK);
}

/* dict_pcre_compile - compile pattern */

static int dict_pcre_compile(const char *mapname, int lineno,
			             DICT_PCRE_REGEXP *pattern,
			             DICT_PCRE_ENGINE *engine)
{
#if HAS_PCRE == 1
    const char *error;
    int     errptr;

    engine->pattern = pcre_compile(pattern->regexp, pattern->options,
				   &error, &errptr, NULL);
    if (engine->pattern == 0) {
	msg_warn("pcre map %s, line %d: error in regex at offset %d: %s",
		 mapname, lineno, errptr, error);
	return (0);
    }
    engine->hints = pcre_study(engine->pattern, 0, &error);
    if (error != 0) {
	msg_warn("pcre map %s, line %d: error while studying regex: %s",
		 mapname, lineno, error);
	DICT_PCRE_CODE_FREE(engine->pattern);
	return (0);
    }
#else
    int     error;
    size_t  errptr;

    engine->pattern = pcre2_compile((unsigned char *) pattern->regexp,
				    PCRE2_ZERO_TERMINATED,
				    pattern->options, &error, &errptr, NULL);
    if (engine->pattern == 0) {
	VSTRING *buf = vstring_alloc(DICT_PCRE_GET_ERROR_BUF_LEN);

	msg_warn("pcre map %s, line %d: error in regex at offset %lu: %s",
		 mapname, lineno, (unsigned long) errptr,
		 dict_pcre_get_error(buf, error));
	vstring_free(buf);
	return (0);
    }
    engine->match_data = pcre2_match_data_create_from_pattern(
					       engine->pattern, (void *) 0);
#endif
    return (1);
}

/* dict_pcre_rule_alloc - fill in a generic rule structure */

static DICT_PCRE_RULE *dict_pcre_rule_alloc(int op, int lineno, size_t size)
{
    DICT_PCRE_RULE *rule;

    rule = (DICT_PCRE_RULE *) mymalloc(size);
    rule->op = op;
    rule->lineno = lineno;
    rule->next = 0;

    return (rule);
}

/* dict_pcre_parse_rule - parse and compile one rule */

static DICT_PCRE_RULE *dict_pcre_parse_rule(DICT *dict, const char *mapname,
					            int lineno, char *line,
					            int nesting)
{
    char   *p;

#ifdef DICT_PCRE_CAPTURECOUNT_T
    DICT_PCRE_CAPTURECOUNT_T actual_sub;

#endif
#if 0
    uint32_t namecount;

#endif

    p = line;

    /*
     * An ordinary match rule takes one pattern and replacement text.
     */
    if (!ISALNUM(*p)) {
	DICT_PCRE_REGEXP regexp;
	DICT_PCRE_ENGINE engine;
	DICT_PCRE_PRESCAN_CONTEXT prescan_context;
	DICT_PCRE_MATCH_RULE *match_rule;

	/*
	 * Get the pattern string and options.
	 */
	if (dict_pcre_get_pattern(mapname, lineno, &p, &regexp) == 0)
	    return (0);

	/*
	 * Get the replacement text.
	 */
	while (*p && ISSPACE(*p))
	    ++p;
	if (!*p)
	    msg_warn("pcre map %s, line %d: no replacement text: "
		     "using empty string", mapname, lineno);

	/*
	 * Sanity check the $number instances in the replacement text.
	 */
	prescan_context.mapname = mapname;
	prescan_context.lineno = lineno;
	prescan_context.max_sub = 0;
	prescan_context.literal = 0;

	/*
	 * The optimizer will eliminate code duplication and/or dead code.
	 */
#define CREATE_MATCHOP_ERROR_RETURN(rval) do { \
	if (prescan_context.literal) \
	    myfree(prescan_context.literal); \
	return (rval); \
    } while (0)

	if (dict->flags & DICT_FLAG_SRC_RHS_IS_FILE) {
	    VSTRING *base64_buf;
	    char   *err;

	    if ((base64_buf = dict_file_to_b64(dict, p)) == 0) {
		err = dict_file_get_error(dict);
		msg_warn("pcre map %s, line %d: %s: skipping this rule",
			 mapname, lineno, err);
		myfree(err);
		CREATE_MATCHOP_ERROR_RETURN(0);
	    }
	    p = vstring_str(base64_buf);
	}
	if (mac_parse(p, dict_pcre_prescan, (void *) &prescan_context)
	    & MAC_PARSE_ERROR) {
	    msg_warn("pcre map %s, line %d: bad replacement syntax: "
		     "skipping this rule", mapname, lineno);
	    CREATE_MATCHOP_ERROR_RETURN(0);
	}

	/*
	 * Substring replacement not possible with negative regexps.
	 */
	if (prescan_context.max_sub > 0 && regexp.match == 0) {
	    msg_warn("pcre map %s, line %d: $number found in negative match "
		   "replacement text: skipping this rule", mapname, lineno);
	    CREATE_MATCHOP_ERROR_RETURN(0);
	}
	if (prescan_context.max_sub > 0 && (dict->flags & DICT_FLAG_NO_REGSUB)) {
	    msg_warn("pcre map %s, line %d: "
		     "regular expression substitution is not allowed: "
		     "skipping this rule", mapname, lineno);
	    CREATE_MATCHOP_ERROR_RETURN(0);
	}

	/*
	 * Compile the pattern.
	 */
	if (dict_pcre_compile(mapname, lineno, &regexp, &engine) == 0)
	    CREATE_MATCHOP_ERROR_RETURN(0);
#ifdef DICT_PCRE_CAPTURECOUNT_T
#if HAS_PCRE == 1
	if (pcre_fullinfo(engine.pattern, engine.hints,
			  PCRE_INFO_CAPTURECOUNT,
			  (void *) &actual_sub) != 0)
	    msg_panic("pcre map %s, line %d: pcre_fullinfo failed",
		      mapname, lineno);
#else						/* HAS_PCRE */
#if 0
	if (pcre2_pattern_info(
		     engine.pattern, PCRE2_INFO_NAMECOUNT, &namecount) != 0)
	    msg_panic("pcre map %s, line %d: pcre2_pattern_info failed",
		      mapname, lineno);
	if (namecount > 0) {
	    msg_warn("pcre map %s, line %d: named substrings are not supported",
		     mapname, lineno);
	    if (engine.pattern)
		DICT_PCRE_CODE_FREE(engine.pattern);
	    DICT_PCRE_MATCH_HINT_FREE(&engine);
	    CREATE_MATCHOP_ERROR_RETURN(0);
	}
#endif
	if (pcre2_pattern_info(engine.pattern, PCRE2_INFO_CAPTURECOUNT,
			       (void *) &actual_sub) != 0)
	    msg_panic("pcre map %s, line %d: pcre2_pattern_info failed",
		      mapname, lineno);
#endif						/* HAS_PCRE */
	if (prescan_context.max_sub > actual_sub) {
	    msg_warn("pcre map %s, line %d: out of range replacement index \"%d\": "
		     "skipping this rule", mapname, lineno,
		     (int) prescan_context.max_sub);
	    if (engine.pattern)
		DICT_PCRE_CODE_FREE(engine.pattern);
	    DICT_PCRE_MATCH_HINT_FREE(&engine);
	    CREATE_MATCHOP_ERROR_RETURN(0);
	}
#endif						/* DICT_PCRE_CAPTURECOUNT_T */

	/*
	 * Save the result.
	 */
	match_rule = (DICT_PCRE_MATCH_RULE *)
	    dict_pcre_rule_alloc(DICT_PCRE_OP_MATCH, lineno,
				 sizeof(DICT_PCRE_MATCH_RULE));
	match_rule->match = regexp.match;
	match_rule->max_sub = prescan_context.max_sub;
	if (prescan_context.literal)
	    match_rule->replacement = prescan_context.literal;
	else
	    match_rule->replacement = mystrdup(p);
	match_rule->pattern = engine.pattern;
	DICT_PCRE_MATCH_HINT(match_rule) = DICT_PCRE_MATCH_HINT(&engine);
	return ((DICT_PCRE_RULE *) match_rule);
    }

    /*
     * The IF operator takes one pattern but no replacement text.
     */
    else if (strncasecmp(p, "IF", 2) == 0 && !ISALNUM(p[2])) {
	DICT_PCRE_REGEXP regexp;
	DICT_PCRE_ENGINE engine;
	DICT_PCRE_IF_RULE *if_rule;

	p += 2;

	/*
	 * Get the pattern.
	 */
	while (*p && ISSPACE(*p))
	    p++;
	if (!dict_pcre_get_pattern(mapname, lineno, &p, &regexp))
	    return (0);

	/*
	 * Warn about out-of-place text.
	 */
	while (*p && ISSPACE(*p))
	    ++p;
	if (*p) {
	    msg_warn("pcre map %s, line %d: ignoring extra text after "
		     "IF statement: \"%s\"", mapname, lineno, p);
	    msg_warn("pcre map %s, line %d: do not prepend whitespace"
		     " to statements between IF and ENDIF", mapname, lineno);
	}

	/*
	 * Compile the pattern.
	 */
	if (dict_pcre_compile(mapname, lineno, &regexp, &engine) == 0)
	    return (0);

	/*
	 * Save the result.
	 */
	if_rule = (DICT_PCRE_IF_RULE *)
	    dict_pcre_rule_alloc(DICT_PCRE_OP_IF, lineno,
				 sizeof(DICT_PCRE_IF_RULE));
	if_rule->match = regexp.match;
	if_rule->pattern = engine.pattern;
	DICT_PCRE_MATCH_HINT(if_rule) = DICT_PCRE_MATCH_HINT(&engine);
	if_rule->endif_rule = 0;
	return ((DICT_PCRE_RULE *) if_rule);
    }

    /*
     * The ENDIF operator takes no patterns and no replacement text.
     */
    else if (strncasecmp(p, "ENDIF", 5) == 0 && !ISALNUM(p[5])) {
	DICT_PCRE_RULE *rule;

	p += 5;

	/*
	 * Warn about out-of-place ENDIFs.
	 */
	if (nesting == 0) {
	    msg_warn("pcre map %s, line %d: ignoring ENDIF without matching IF",
		     mapname, lineno);
	    return (0);
	}

	/*
	 * Warn about out-of-place text.
	 */
	while (*p && ISSPACE(*p))
	    ++p;
	if (*p)
	    msg_warn("pcre map %s, line %d: ignoring extra text after ENDIF",
		     mapname, lineno);

	/*
	 * Save the result.
	 */
	rule = dict_pcre_rule_alloc(DICT_PCRE_OP_ENDIF, lineno,
				    sizeof(DICT_PCRE_RULE));
	return (rule);
    }

    /*
     * Unrecognized input.
     */
    else {
	msg_warn("pcre map %s, line %d: ignoring unrecognized request",
		 mapname, lineno);
	return (0);
    }
}

/* dict_pcre_open - load and compile a file containing regular expressions */

DICT   *dict_pcre_open(const char *mapname, int open_flags, int dict_flags)
{
    const char myname[] = "dict_pcre_open";
    DICT_PCRE *dict_pcre;
    VSTREAM *map_fp = 0;
    struct stat st;
    VSTRING *why = 0;
    VSTRING *line_buffer = 0;
    DICT_PCRE_RULE *last_rule = 0;
    DICT_PCRE_RULE *rule;
    int     last_line = 0;
    int     lineno;
    int     nesting = 0;
    char   *p;
    DICT_PCRE_RULE **rule_stack = 0;
    MVECT   mvect;

    /*
     * Let the optimizer worry about eliminating redundant code.
     */
#define DICT_PCRE_OPEN_RETURN(d) do { \
	DICT *__d = (d); \
	if (map_fp != 0) \
	    vstream_fclose(map_fp); \
	if (line_buffer != 0) \
	    vstring_free(line_buffer); \
	if (why != 0) \
	   vstring_free(why); \
	return (__d); \
    } while (0)

    /*
     * Sanity checks.
     */
    if (open_flags != O_RDONLY)
	DICT_PCRE_OPEN_RETURN(dict_surrogate(DICT_TYPE_PCRE, mapname,
					     open_flags, dict_flags,
				  "%s:%s map requires O_RDONLY access mode",
					     DICT_TYPE_PCRE, mapname));

    /*
     * Open the configuration file.
     */
    if ((map_fp = dict_stream_open(DICT_TYPE_PCRE, mapname, O_RDONLY,
				   dict_flags, &st, &why)) == 0)
	DICT_PCRE_OPEN_RETURN(dict_surrogate(DICT_TYPE_PCRE, mapname,
					     open_flags, dict_flags,
					     "%s", vstring_str(why)));
    line_buffer = vstring_alloc(100);

    dict_pcre = (DICT_PCRE *) dict_alloc(DICT_TYPE_PCRE, mapname,
					 sizeof(*dict_pcre));
    dict_pcre->dict.lookup = dict_pcre_lookup;
    dict_pcre->dict.close = dict_pcre_close;
    dict_pcre->dict.flags = dict_flags | DICT_FLAG_PATTERN;
    if (dict_flags & DICT_FLAG_FOLD_MUL)
	dict_pcre->dict.fold_buf = vstring_alloc(10);
    dict_pcre->head = 0;
    dict_pcre->expansion_buf = 0;

#if HAS_PCRE == 1
    if (dict_pcre_init == 0) {
	pcre_malloc = (void *(*) (size_t)) mymalloc;
	pcre_free = (void (*) (void *)) myfree;
	dict_pcre_init = 1;
    }
#endif
    dict_pcre->dict.owner.uid = st.st_uid;
    dict_pcre->dict.owner.status = (st.st_uid != 0);

    /*
     * Parse the pcre table.
     */
    while (readllines(line_buffer, map_fp, &last_line, &lineno)) {
	p = vstring_str(line_buffer);
	trimblanks(p, 0)[0] = 0;		/* Trim space at end */
	if (*p == 0)
	    continue;
	rule = dict_pcre_parse_rule(&dict_pcre->dict, mapname, lineno,
				    p, nesting);
	if (rule == 0)
	    continue;
	if (rule->op == DICT_PCRE_OP_IF) {
	    if (rule_stack == 0)
		rule_stack = (DICT_PCRE_RULE **) mvect_alloc(&mvect,
					   sizeof(*rule_stack), nesting + 1,
						(MVECT_FN) 0, (MVECT_FN) 0);
	    else
		rule_stack =
		    (DICT_PCRE_RULE **) mvect_realloc(&mvect, nesting + 1);
	    rule_stack[nesting] = rule;
	    nesting++;
	} else if (rule->op == DICT_PCRE_OP_ENDIF) {
	    DICT_PCRE_IF_RULE *if_rule;

	    if (nesting-- <= 0)
		/* Already handled in dict_pcre_parse_rule(). */
		msg_panic("%s: ENDIF without IF", myname);
	    if (rule_stack[nesting]->op != DICT_PCRE_OP_IF)
		msg_panic("%s: unexpected rule stack element type %d",
			  myname, rule_stack[nesting]->op);
	    if_rule = (DICT_PCRE_IF_RULE *) rule_stack[nesting];
	    if_rule->endif_rule = rule;
	}
	if (last_rule == 0)
	    dict_pcre->head = rule;
	else
	    last_rule->next = rule;
	last_rule = rule;
    }

    while (nesting-- > 0)
	msg_warn("pcre map %s, line %d: IF has no matching ENDIF",
		 mapname, rule_stack[nesting]->lineno);

    if (rule_stack)
	(void) mvect_free(&mvect);

    dict_file_purge_buffers(&dict_pcre->dict);
    DICT_PCRE_OPEN_RETURN(DICT_DEBUG (&dict_pcre->dict));
}

#endif					/* HAS_PCRE */
