/*	$NetBSD: dict_regexp.c,v 1.1.1.1.10.1 2013/01/23 00:05:16 yamt Exp $	*/

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
/*	regular expressions. The result object can be used to match strings
/*	against the table.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/*	regexp_table(5) format of Postfix regular expression tables
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
/*	Heavily rewritten by Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include "sys_defs.h"

#ifdef HAS_POSIX_REGEXP

#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <regex.h>
#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
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
#include "dict_regexp.h"
#include "mac_parse.h"
#include "warn_stat.h"

 /*
  * Support for IF/ENDIF based on an idea by Bert Driehuis.
  */
#define DICT_REGEXP_OP_MATCH	1	/* Match this regexp */
#define DICT_REGEXP_OP_IF	2	/* Increase if/endif nesting on match */
#define DICT_REGEXP_OP_ENDIF	3	/* Decrease if/endif nesting on match */

 /*
  * Regular expression before compiling.
  */
typedef struct {
    char   *regexp;			/* regular expression */
    int     options;			/* regcomp() options */
    int     match;			/* positive or negative match */
} DICT_REGEXP_PATTERN;

 /*
  * Compiled generic rule, and subclasses that derive from it.
  */
typedef struct DICT_REGEXP_RULE {
    int     op;				/* DICT_REGEXP_OP_MATCH/IF/ENDIF */
    int     nesting;			/* Level of search nesting */
    int     lineno;			/* source file line number */
    struct DICT_REGEXP_RULE *next;	/* next rule in dict */
} DICT_REGEXP_RULE;

typedef struct {
    DICT_REGEXP_RULE rule;		/* generic part */
    regex_t *first_exp;			/* compiled primary pattern */
    int     first_match;		/* positive or negative match */
    regex_t *second_exp;		/* compiled secondary pattern */
    int     second_match;		/* positive or negative match */
    char   *replacement;		/* replacement text */
    size_t  max_sub;			/* largest $number in replacement */
} DICT_REGEXP_MATCH_RULE;

typedef struct {
    DICT_REGEXP_RULE rule;		/* generic members */
    regex_t *expr;			/* the condition */
    int     match;			/* positive or negative match */
} DICT_REGEXP_IF_RULE;

 /*
  * Regexp map.
  */
typedef struct {
    DICT    dict;			/* generic members */
    regmatch_t *pmatch;			/* matched substring info */
    DICT_REGEXP_RULE *head;		/* first rule */
    VSTRING *expansion_buf;		/* lookup result */
} DICT_REGEXP;

 /*
  * Macros to make dense code more readable.
  */
#define NULL_SUBSTITUTIONS	(0)
#define NULL_MATCH_RESULT	((regmatch_t *) 0)

 /*
  * Context for $number expansion callback.
  */
typedef struct {
    DICT_REGEXP *dict_regexp;		/* the dictionary handle */
    DICT_REGEXP_MATCH_RULE *match_rule;	/* the rule we matched */
    const char *lookup_string;		/* matched text */
} DICT_REGEXP_EXPAND_CONTEXT;

 /*
  * Context for $number pre-scan callback.
  */
typedef struct {
    const char *mapname;		/* name of regexp map */
    int     lineno;			/* where in file */
    size_t  max_sub;			/* largest $number seen */
    char   *literal;			/* constant result, $$ -> $ */
} DICT_REGEXP_PRESCAN_CONTEXT;

 /*
  * Compatibility.
  */
#ifndef MAC_PARSE_OK
#define MAC_PARSE_OK 0
#endif

/* dict_regexp_expand - replace $number with substring from matched text */

static int dict_regexp_expand(int type, VSTRING *buf, char *ptr)
{
    DICT_REGEXP_EXPAND_CONTEXT *ctxt = (DICT_REGEXP_EXPAND_CONTEXT *) ptr;
    DICT_REGEXP_MATCH_RULE *match_rule = ctxt->match_rule;
    DICT_REGEXP *dict_regexp = ctxt->dict_regexp;
    regmatch_t *pmatch;
    size_t  n;

    /*
     * Replace $number by the corresponding substring from the matched text.
     * We pre-scanned the replacement text at compile time, so any out of
     * range $number means that something impossible has happened.
     */
    if (type == MAC_PARSE_VARNAME) {
	n = atoi(vstring_str(buf));
	if (n < 1 || n > match_rule->max_sub)
	    msg_panic("regexp map %s, line %d: out of range replacement index \"%s\"",
		      dict_regexp->dict.name, match_rule->rule.lineno,
		      vstring_str(buf));
	pmatch = dict_regexp->pmatch + n;
	if (pmatch->rm_so < 0 || pmatch->rm_so == pmatch->rm_eo)
	    return (MAC_PARSE_UNDEF);		/* empty or not matched */
	vstring_strncat(dict_regexp->expansion_buf,
			ctxt->lookup_string + pmatch->rm_so,
			pmatch->rm_eo - pmatch->rm_so);
	return (MAC_PARSE_OK);
    }

    /*
     * Straight text - duplicate with no substitution.
     */
    else {
	vstring_strcat(dict_regexp->expansion_buf, vstring_str(buf));
	return (MAC_PARSE_OK);
    }
}

/* dict_regexp_regerror - report regexp compile/execute error */

static void dict_regexp_regerror(const char *mapname, int lineno, int error,
				         const regex_t *expr)
{
    char    errbuf[256];

    (void) regerror(error, expr, errbuf, sizeof(errbuf));
    msg_warn("regexp map %s, line %d: %s", mapname, lineno, errbuf);
}

 /*
  * Inlined to reduce function call overhead in the time-critical loop.
  */
#define DICT_REGEXP_REGEXEC(err, map, line, expr, match, str, nsub, pmatch) \
    ((err) = regexec((expr), (str), (nsub), (pmatch), 0), \
     ((err) == REG_NOMATCH ? !(match) : \
      (err) == 0 ? (match) : \
      (dict_regexp_regerror((map), (line), (err), (expr)), 0)))

/* dict_regexp_lookup - match string and perform optional substitution */

static const char *dict_regexp_lookup(DICT *dict, const char *lookup_string)
{
    DICT_REGEXP *dict_regexp = (DICT_REGEXP *) dict;
    DICT_REGEXP_RULE *rule;
    DICT_REGEXP_IF_RULE *if_rule;
    DICT_REGEXP_MATCH_RULE *match_rule;
    DICT_REGEXP_EXPAND_CONTEXT expand_context;
    int     error;
    int     nesting = 0;

    dict->error = 0;

    if (msg_verbose)
	msg_info("dict_regexp_lookup: %s: %s", dict->name, lookup_string);

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_MUL) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, lookup_string);
	lookup_string = lowercase(vstring_str(dict->fold_buf));
    }
    for (rule = dict_regexp->head; rule; rule = rule->next) {

	/*
	 * Skip rules inside failed IF/ENDIF.
	 */
	if (nesting < rule->nesting)
	    continue;

	switch (rule->op) {

	    /*
	     * Search for the first matching primary expression. Limit the
	     * overhead for substring substitution to the bare minimum.
	     */
	case DICT_REGEXP_OP_MATCH:
	    match_rule = (DICT_REGEXP_MATCH_RULE *) rule;
	    if (!DICT_REGEXP_REGEXEC(error, dict->name, rule->lineno,
				     match_rule->first_exp,
				     match_rule->first_match,
				     lookup_string,
				     match_rule->max_sub > 0 ?
				     match_rule->max_sub + 1 : 0,
				     dict_regexp->pmatch))
		continue;
	    if (match_rule->second_exp
		&& !DICT_REGEXP_REGEXEC(error, dict->name, rule->lineno,
					match_rule->second_exp,
					match_rule->second_match,
					lookup_string,
					NULL_SUBSTITUTIONS,
					NULL_MATCH_RESULT))
		continue;

	    /*
	     * Skip $number substitutions when the replacement text contains
	     * no $number strings, as learned during the compile time
	     * pre-scan. The pre-scan already replaced $$ by $.
	     */
	    if (match_rule->max_sub == 0)
		return (match_rule->replacement);

	    /*
	     * Perform $number substitutions on the replacement text. We
	     * pre-scanned the replacement text at compile time. Any macro
	     * expansion errors at this point mean something impossible has
	     * happened.
	     */
	    if (!dict_regexp->expansion_buf)
		dict_regexp->expansion_buf = vstring_alloc(10);
	    VSTRING_RESET(dict_regexp->expansion_buf);
	    expand_context.lookup_string = lookup_string;
	    expand_context.match_rule = match_rule;
	    expand_context.dict_regexp = dict_regexp;

	    if (mac_parse(match_rule->replacement, dict_regexp_expand,
			  (char *) &expand_context) & MAC_PARSE_ERROR)
		msg_panic("regexp map %s, line %d: bad replacement syntax",
			  dict->name, rule->lineno);
	    VSTRING_TERMINATE(dict_regexp->expansion_buf);
	    return (vstring_str(dict_regexp->expansion_buf));

	    /*
	     * Conditional.
	     */
	case DICT_REGEXP_OP_IF:
	    if_rule = (DICT_REGEXP_IF_RULE *) rule;
	    if (DICT_REGEXP_REGEXEC(error, dict->name, rule->lineno,
			       if_rule->expr, if_rule->match, lookup_string,
				    NULL_SUBSTITUTIONS, NULL_MATCH_RESULT))
		nesting++;
	    continue;

	    /*
	     * ENDIF after successful IF.
	     */
	case DICT_REGEXP_OP_ENDIF:
	    nesting--;
	    continue;

	default:
	    msg_panic("dict_regexp_lookup: impossible operation %d", rule->op);
	}
    }
    return (0);
}

/* dict_regexp_close - close regexp dictionary */

static void dict_regexp_close(DICT *dict)
{
    DICT_REGEXP *dict_regexp = (DICT_REGEXP *) dict;
    DICT_REGEXP_RULE *rule;
    DICT_REGEXP_RULE *next;
    DICT_REGEXP_MATCH_RULE *match_rule;
    DICT_REGEXP_IF_RULE *if_rule;

    for (rule = dict_regexp->head; rule; rule = next) {
	next = rule->next;
	switch (rule->op) {
	case DICT_REGEXP_OP_MATCH:
	    match_rule = (DICT_REGEXP_MATCH_RULE *) rule;
	    if (match_rule->first_exp) {
		regfree(match_rule->first_exp);
		myfree((char *) match_rule->first_exp);
	    }
	    if (match_rule->second_exp) {
		regfree(match_rule->second_exp);
		myfree((char *) match_rule->second_exp);
	    }
	    if (match_rule->replacement)
		myfree((char *) match_rule->replacement);
	    break;
	case DICT_REGEXP_OP_IF:
	    if_rule = (DICT_REGEXP_IF_RULE *) rule;
	    if (if_rule->expr) {
		regfree(if_rule->expr);
		myfree((char *) if_rule->expr);
	    }
	    break;
	case DICT_REGEXP_OP_ENDIF:
	    break;
	default:
	    msg_panic("dict_regexp_close: unknown operation %d", rule->op);
	}
	myfree((char *) rule);
    }
    if (dict_regexp->pmatch)
	myfree((char *) dict_regexp->pmatch);
    if (dict_regexp->expansion_buf)
	vstring_free(dict_regexp->expansion_buf);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_regexp_get_pat - extract one pattern with options from rule */

static int dict_regexp_get_pat(const char *mapname, int lineno, char **bufp,
			               DICT_REGEXP_PATTERN *pat)
{
    char   *p = *bufp;
    char    re_delim;

    /*
     * Process negation operators.
     */
    pat->match = 1;
    while (*p == '!') {
	pat->match = !pat->match;
	p++;
    }

    /*
     * Grr...aceful handling of whitespace after '!'.
     */
    while (*p && ISSPACE(*p))
	p++;
    if (*p == 0) {
	msg_warn("regexp map %s, line %d: no regexp: skipping this rule",
		 mapname, lineno);
	return (0);
    }

    /*
     * Search for the closing delimiter, handling backslash escape.
     */
    re_delim = *p++;
    pat->regexp = p;
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
	msg_warn("regexp map %s, line %d: no closing regexp delimiter \"%c\": "
		 "skipping this rule", mapname, lineno, re_delim);
	return (0);
    }
    *p++ = 0;					/* null terminate */

    /*
     * Search for options.
     */
    pat->options = REG_EXTENDED | REG_ICASE;
    while (*p && !ISSPACE(*p) && *p != '!') {
	switch (*p) {
	case 'i':
	    pat->options ^= REG_ICASE;
	    break;
	case 'm':
	    pat->options ^= REG_NEWLINE;
	    break;
	case 'x':
	    pat->options ^= REG_EXTENDED;
	    break;
	default:
	    msg_warn("regexp map %s, line %d: unknown regexp option \"%c\": "
		     "skipping this rule", mapname, lineno, *p);
	    return (0);
	}
	++p;
    }
    *bufp = p;
    return (1);
}

/* dict_regexp_get_pats - get the primary and second patterns and flags */

static int dict_regexp_get_pats(const char *mapname, int lineno, char **p,
				        DICT_REGEXP_PATTERN *first_pat,
				        DICT_REGEXP_PATTERN *second_pat)
{

    /*
     * Get the primary and optional secondary patterns and their flags.
     */
    if (dict_regexp_get_pat(mapname, lineno, p, first_pat) == 0)
	return (0);
    if (**p == '!') {
#if 0
	static int bitrot_warned = 0;

	if (bitrot_warned == 0) {
	    msg_warn("regexp file %s, line %d: /pattern1/!/pattern2/ goes away,"
		 " use \"if !/pattern2/ ... /pattern1/ ... endif\" instead",
		     mapname, lineno);
	    bitrot_warned = 1;
	}
#endif
	if (dict_regexp_get_pat(mapname, lineno, p, second_pat) == 0)
	    return (0);
    } else {
	second_pat->regexp = 0;
    }
    return (1);
}

/* dict_regexp_prescan - find largest $number in replacement text */

static int dict_regexp_prescan(int type, VSTRING *buf, char *context)
{
    DICT_REGEXP_PRESCAN_CONTEXT *ctxt = (DICT_REGEXP_PRESCAN_CONTEXT *) context;
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
	    msg_warn("regexp map %s, line %d: non-numeric replacement index \"%s\"",
		     ctxt->mapname, ctxt->lineno, vstring_str(buf));
	    return (MAC_PARSE_ERROR);
	}
	n = atoi(vstring_str(buf));
	if (n < 1) {
	    msg_warn("regexp map %s, line %d: out-of-range replacement index \"%s\"",
		     ctxt->mapname, ctxt->lineno, vstring_str(buf));
	    return (MAC_PARSE_ERROR);
	}
	if (n > ctxt->max_sub)
	    ctxt->max_sub = n;
    } else if (type == MAC_PARSE_LITERAL && ctxt->max_sub == 0) {
	if (ctxt->literal)
	    msg_panic("regexp map %s, line %d: multiple literals but no $number",
		      ctxt->mapname, ctxt->lineno);
	ctxt->literal = mystrdup(vstring_str(buf));
    }
    return (MAC_PARSE_OK);
}

/* dict_regexp_compile_pat - compile one pattern */

static regex_t *dict_regexp_compile_pat(const char *mapname, int lineno,
					        DICT_REGEXP_PATTERN *pat)
{
    int     error;
    regex_t *expr;

    expr = (regex_t *) mymalloc(sizeof(*expr));
    error = regcomp(expr, pat->regexp, pat->options);
    if (error != 0) {
	dict_regexp_regerror(mapname, lineno, error, expr);
	myfree((char *) expr);
	return (0);
    }
    return (expr);
}

/* dict_regexp_rule_alloc - fill in a generic rule structure */

static DICT_REGEXP_RULE *dict_regexp_rule_alloc(int op, int nesting,
						        int lineno,
						        size_t size)
{
    DICT_REGEXP_RULE *rule;

    rule = (DICT_REGEXP_RULE *) mymalloc(size);
    rule->op = op;
    rule->nesting = nesting;
    rule->lineno = lineno;
    rule->next = 0;

    return (rule);
}

/* dict_regexp_parseline - parse one rule */

static DICT_REGEXP_RULE *dict_regexp_parseline(const char *mapname, int lineno,
					            char *line, int nesting,
					               int dict_flags)
{
    char   *p;

    p = line;

    /*
     * An ordinary rule takes one or two patterns and replacement text.
     */
    if (!ISALNUM(*p)) {
	DICT_REGEXP_PATTERN first_pat;
	DICT_REGEXP_PATTERN second_pat;
	DICT_REGEXP_PRESCAN_CONTEXT prescan_context;
	regex_t *first_exp = 0;
	regex_t *second_exp;
	DICT_REGEXP_MATCH_RULE *match_rule;

	/*
	 * Get the primary and the optional secondary patterns.
	 */
	if (!dict_regexp_get_pats(mapname, lineno, &p, &first_pat, &second_pat))
	    return (0);

	/*
	 * Get the replacement text.
	 */
	while (*p && ISSPACE(*p))
	    ++p;
	if (!*p) {
	    msg_warn("regexp map %s, line %d: using empty replacement string",
		     mapname, lineno);
	}

	/*
	 * Find the highest-numbered $number in the replacement text. We can
	 * speed up pattern matching 1) by passing hints to the regexp
	 * compiler, setting the REG_NOSUB flag when the replacement text
	 * contains no $number string; 2) by passing hints to the regexp
	 * execution code, limiting the amount of text that is made available
	 * for substitution.
	 */
	prescan_context.mapname = mapname;
	prescan_context.lineno = lineno;
	prescan_context.max_sub = 0;
	prescan_context.literal = 0;

	/*
	 * The optimizer will eliminate code duplication and/or dead code.
	 */
#define CREATE_MATCHOP_ERROR_RETURN(rval) do { \
	if (first_exp) { \
	    regfree(first_exp); \
	    myfree((char *) first_exp); \
	} \
	if (prescan_context.literal) \
	    myfree(prescan_context.literal); \
	return (rval); \
    } while (0)

	if (mac_parse(p, dict_regexp_prescan, (char *) &prescan_context)
	    & MAC_PARSE_ERROR) {
	    msg_warn("regexp map %s, line %d: bad replacement syntax: "
		     "skipping this rule", mapname, lineno);
	    CREATE_MATCHOP_ERROR_RETURN(0);
	}

	/*
	 * Compile the primary and the optional secondary pattern. Speed up
	 * execution when no matched text needs to be substituted into the
	 * result string, or when the highest numbered substring is less than
	 * the total number of () subpatterns.
	 */
	if (prescan_context.max_sub == 0)
	    first_pat.options |= REG_NOSUB;
	if (prescan_context.max_sub > 0 && first_pat.match == 0) {
	    msg_warn("regexp map %s, line %d: $number found in negative match "
		   "replacement text: skipping this rule", mapname, lineno);
	    CREATE_MATCHOP_ERROR_RETURN(0);
	}
	if (prescan_context.max_sub > 0 && (dict_flags & DICT_FLAG_NO_REGSUB)) {
	    msg_warn("regexp map %s, line %d: "
		     "regular expression substitution is not allowed: "
		     "skipping this rule", mapname, lineno);
	    CREATE_MATCHOP_ERROR_RETURN(0);
	}
	if ((first_exp = dict_regexp_compile_pat(mapname, lineno,
						 &first_pat)) == 0)
	    CREATE_MATCHOP_ERROR_RETURN(0);
	if (prescan_context.max_sub > first_exp->re_nsub) {
	    msg_warn("regexp map %s, line %d: out of range replacement index \"%d\": "
		     "skipping this rule", mapname, lineno,
		     (int) prescan_context.max_sub);
	    CREATE_MATCHOP_ERROR_RETURN(0);
	}
	if (second_pat.regexp != 0) {
	    second_pat.options |= REG_NOSUB;
	    if ((second_exp = dict_regexp_compile_pat(mapname, lineno,
						      &second_pat)) == 0)
		CREATE_MATCHOP_ERROR_RETURN(0);
	} else {
	    second_exp = 0;
	}
	match_rule = (DICT_REGEXP_MATCH_RULE *)
	    dict_regexp_rule_alloc(DICT_REGEXP_OP_MATCH, nesting, lineno,
				   sizeof(DICT_REGEXP_MATCH_RULE));
	match_rule->first_exp = first_exp;
	match_rule->first_match = first_pat.match;
	match_rule->max_sub = prescan_context.max_sub;
	match_rule->second_exp = second_exp;
	match_rule->second_match = second_pat.match;
	if (prescan_context.literal)
	    match_rule->replacement = prescan_context.literal;
	else
	    match_rule->replacement = mystrdup(p);
	return ((DICT_REGEXP_RULE *) match_rule);
    }

    /*
     * The IF operator takes one pattern but no replacement text.
     */
    else if (strncasecmp(p, "IF", 2) == 0 && !ISALNUM(p[2])) {
	DICT_REGEXP_PATTERN pattern;
	regex_t *expr;
	DICT_REGEXP_IF_RULE *if_rule;

	p += 2;
	while (*p && ISSPACE(*p))
	    p++;
	if (!dict_regexp_get_pat(mapname, lineno, &p, &pattern))
	    return (0);
	while (*p && ISSPACE(*p))
	    ++p;
	if (*p) {
	    msg_warn("regexp map %s, line %d: ignoring extra text after"
		     " IF statement: \"%s\"", mapname, lineno, p);
	    msg_warn("regexp map %s, line %d: do not prepend whitespace"
		     " to statements between IF and ENDIF", mapname, lineno);
	}
	if ((expr = dict_regexp_compile_pat(mapname, lineno, &pattern)) == 0)
	    return (0);
	if_rule = (DICT_REGEXP_IF_RULE *)
	    dict_regexp_rule_alloc(DICT_REGEXP_OP_IF, nesting, lineno,
				   sizeof(DICT_REGEXP_IF_RULE));
	if_rule->expr = expr;
	if_rule->match = pattern.match;
	return ((DICT_REGEXP_RULE *) if_rule);
    }

    /*
     * The ENDIF operator takes no patterns and no replacement text.
     */
    else if (strncasecmp(p, "ENDIF", 5) == 0 && !ISALNUM(p[5])) {
	DICT_REGEXP_RULE *rule;

	p += 5;
	if (nesting == 0) {
	    msg_warn("regexp map %s, line %d: ignoring ENDIF without matching IF",
		     mapname, lineno);
	    return (0);
	}
	while (*p && ISSPACE(*p))
	    ++p;
	if (*p)
	    msg_warn("regexp map %s, line %d: ignoring extra text after ENDIF",
		     mapname, lineno);
	rule = dict_regexp_rule_alloc(DICT_REGEXP_OP_ENDIF, nesting, lineno,
				      sizeof(DICT_REGEXP_RULE));
	return (rule);
    }

    /*
     * Unrecognized input.
     */
    else {
	msg_warn("regexp map %s, line %d: ignoring unrecognized request",
		 mapname, lineno);
	return (0);
    }
}

/* dict_regexp_open - load and compile a file containing regular expressions */

DICT   *dict_regexp_open(const char *mapname, int open_flags, int dict_flags)
{
    DICT_REGEXP *dict_regexp;
    VSTREAM *map_fp;
    struct stat st;
    VSTRING *line_buffer;
    DICT_REGEXP_RULE *rule;
    DICT_REGEXP_RULE *last_rule = 0;
    int     lineno = 0;
    size_t  max_sub = 0;
    int     nesting = 0;
    char   *p;

    /*
     * Sanity checks.
     */
    if (open_flags != O_RDONLY)
	return (dict_surrogate(DICT_TYPE_REGEXP, mapname, open_flags, dict_flags,
			       "%s:%s map requires O_RDONLY access mode",
			       DICT_TYPE_REGEXP, mapname));

    /*
     * Open the configuration file.
     */
    if ((map_fp = vstream_fopen(mapname, O_RDONLY, 0)) == 0)
	return (dict_surrogate(DICT_TYPE_REGEXP, mapname, open_flags, dict_flags,
			       "open %s: %m", mapname));
    if (fstat(vstream_fileno(map_fp), &st) < 0)
	msg_fatal("fstat %s: %m", mapname);

    line_buffer = vstring_alloc(100);

    dict_regexp = (DICT_REGEXP *) dict_alloc(DICT_TYPE_REGEXP, mapname,
					     sizeof(*dict_regexp));
    dict_regexp->dict.lookup = dict_regexp_lookup;
    dict_regexp->dict.close = dict_regexp_close;
    dict_regexp->dict.flags = dict_flags | DICT_FLAG_PATTERN;
    if (dict_flags & DICT_FLAG_FOLD_MUL)
	dict_regexp->dict.fold_buf = vstring_alloc(10);
    dict_regexp->head = 0;
    dict_regexp->pmatch = 0;
    dict_regexp->expansion_buf = 0;
    dict_regexp->dict.owner.uid = st.st_uid;
    dict_regexp->dict.owner.status = (st.st_uid != 0);

    /*
     * Parse the regexp table.
     */
    while (readlline(line_buffer, map_fp, &lineno)) {
	p = vstring_str(line_buffer);
	trimblanks(p, 0)[0] = 0;
	if (*p == 0)
	    continue;
	rule = dict_regexp_parseline(mapname, lineno, p, nesting, dict_flags);
	if (rule == 0)
	    continue;
	if (rule->op == DICT_REGEXP_OP_MATCH) {
	    if (((DICT_REGEXP_MATCH_RULE *) rule)->max_sub > max_sub)
		max_sub = ((DICT_REGEXP_MATCH_RULE *) rule)->max_sub;
	} else if (rule->op == DICT_REGEXP_OP_IF) {
	    nesting++;
	} else if (rule->op == DICT_REGEXP_OP_ENDIF) {
	    nesting--;
	}
	if (last_rule == 0)
	    dict_regexp->head = rule;
	else
	    last_rule->next = rule;
	last_rule = rule;
    }

    if (nesting)
	msg_warn("regexp map %s, line %d: more IFs than ENDIFs",
		 mapname, lineno);

    /*
     * Allocate space for only as many matched substrings as used in the
     * replacement text.
     */
    if (max_sub > 0)
	dict_regexp->pmatch =
	    (regmatch_t *) mymalloc(sizeof(regmatch_t) * (max_sub + 1));

    /*
     * Clean up.
     */
    vstring_free(line_buffer);
    vstream_fclose(map_fp);

    return (DICT_DEBUG (&dict_regexp->dict));
}

#endif
