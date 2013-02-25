/*	$NetBSD: db_common.c,v 1.1.1.3.10.1 2013/02/25 00:27:17 tls Exp $	*/

/*++
/* NAME
/*	db_common 3
/* SUMMARY
/*	utilities common to network based dictionaries
/* SYNOPSIS
/*	#include "db_common.h"
/*
/*	int	db_common_parse(dict, ctx, format, query)
/*	DICT	*dict;
/*	void	**ctx;
/*	const char *format;
/*	int	query;
/*
/*	void	db_common_free_context(ctx)
/*	void	*ctx;
/*
/*	int	db_common_expand(ctx, format, value, key, buf, quote_func);
/*	void	*ctx;
/*	const char *format;
/*	const char *value;
/*	const char *key;
/*	VSTRING	*buf;
/*	void	(*quote_func)(DICT *, const char *, VSTRING *);
/*
/*	int	db_common_check_domain(domain_list, addr);
/*	STRING_LIST *domain_list;
/*	const char *addr;
/*
/*	void	db_common_sql_build_query(query,parser);
/*	VSTRING	*query;
/*	CFG_PARSER *parser;
/*
/* DESCRIPTION
/*	This module implements utilities common to network based dictionaries.
/*
/*	\fIdb_common_parse\fR parses query and result substitution templates.
/*	It must be called for each template before any calls to
/*	\fIdb_common_expand\fR. The \fIctx\fB argument must be initialized to
/*	a reference to a (void *)0 before the first template is parsed, this
/*	causes memory for the context to be allocated and the new pointer is
/*	stored in *ctx. When the dictionary is closed, this memory must be
/*	freed with a final call to \fBdb_common_free_context\fR.
/*
/*	Calls for additional templates associated with the same map must use the
/*	same ctx argument. The context accumulates run-time lookup key and result
/*	validation information (inapplicable keys or results are skipped) and is
/*	needed later in each call of \fIdb_common_expand\fR. A non-zero return
/*	value indicates that data-depedent '%' expansions were found in the input
/*	template.
/*
/*	db_common_alloc() provides a way to use db_common_parse_domain()
/*	etc. without prior db_common_parse() call.
/*
/*	\fIdb_common_expand\fR expands the specifiers in \fIformat\fR.
/*	When the input data lacks all fields needed for the expansion, zero
/*	is returned and the query or result should be skipped. Otherwise
/*	the expansion is appended to the result buffer (after a comma if the
/*	the result buffer is not empty).
/*
/*	If not NULL, the \fBquote_func\fR callback performs database-specific
/*	quoting of each variable before expansion.
/*	\fBvalue\fR is the lookup key for query expansion and result for result
/*	expansion. \fBkey\fR is NULL for query expansion and the lookup key for
/*	result expansion.
/* .PP
/*	The following '%' expansions are performed on \fBvalue\fR:
/* .IP %%
/*	A literal percent character.
/* .IP %s
/*	The entire lookup key \fIaddr\fR.
/* .IP %u
/*	If \fBaddr\fR is a fully qualified address, the local part of the
/*	address.  Otherwise \fIaddr\fR.
/* .IP %d
/*	If \fIaddr\fR is a fully qualified address, the domain part of the
/*	address.  Otherwise the query against the database is suppressed and
/*	the lookup returns no results.
/*
/*	The following '%' expansions are performed on the lookup \fBkey\fR:
/* .IP %S
/*	The entire lookup key \fIkey\fR.
/* .IP %U
/*	If \fBkey\fR is a fully qualified address, the local part of the
/*	address.  Otherwise \fIkey\fR.
/* .IP %D
/*	If \fIkey\fR is a fully qualified address, the domain part of the
/*	address.  Otherwise the query against the database is suppressed and
/*	the lookup returns no results.
/* .PP
/*	\fIdb_common_check_domain\fR() checks the domain list so
/*	that query optimization can be performed. The result is >0
/*	(match found), 0 (no match), or <0 (dictionary error code).
/*
/* .PP
/*	\fIdb_common_sql_build_query\fR builds the "default"(backwards compatible)
/*	query from the 'table', 'select_field', 'where_field' and
/*	'additional_conditions' parameters, checking for errors.
/*
/* DIAGNOSTICS
/*	Fatal errors: invalid substitution format, invalid string_list pattern,
/*	insufficient parameters.
/* SEE ALSO
/*	dict(3) dictionary manager
/*	string_list(3) string list pattern matching
/*	match_ops(3) simple string or host pattern matching
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Liviu Daia
/*	Institute of Mathematics of the Romanian Academy
/*	P.O. BOX 1-764
/*	RO-014700 Bucharest, ROMANIA
/*
/*	Jose Luis Tallon
/*	G4 J.E. - F.I. - U.P.M.
/*	Campus de Montegancedo, S/N
/*	E-28660 Madrid, SPAIN
/*
/*	Victor Duchovni
/*	Morgan Stanley
/*--*/

 /*
  * System library.
  */
#include "sys_defs.h"
#include <stddef.h>
#include <string.h>

 /*
  * Global library.
  */
#include "cfg_parser.h"

 /*
  * Utility library.
  */
#include <mymalloc.h>
#include <vstring.h>
#include <msg.h>
#include <dict.h>

 /*
  * Application specific
  */
#include "db_common.h"

#define	DB_COMMON_KEY_DOMAIN	(1 << 0)/* Need lookup key domain */
#define	DB_COMMON_KEY_USER	(1 << 1)/* Need lookup key localpart */
#define	DB_COMMON_VALUE_DOMAIN	(1 << 2)/* Need result domain */
#define	DB_COMMON_VALUE_USER	(1 << 3)/* Need result localpart */
#define	DB_COMMON_KEY_PARTIAL	(1 << 4)/* Key uses input substrings */

typedef struct {
    DICT   *dict;
    STRING_LIST *domain;
    int     flags;
    int     nparts;
} DB_COMMON_CTX;

/* db_common_alloc - allocate db_common context */

void   *db_common_alloc(DICT *dict)
{
    DB_COMMON_CTX *ctx;

    ctx = (DB_COMMON_CTX *) mymalloc(sizeof *ctx);
    ctx->dict = dict;
    ctx->domain = 0;
    ctx->flags = 0;
    ctx->nparts = 0;
    return ((void *) ctx);
}

/* db_common_parse - validate query or result template */

int     db_common_parse(DICT *dict, void **ctxPtr, const char *format, int query)
{
    DB_COMMON_CTX *ctx = (DB_COMMON_CTX *) *ctxPtr;
    const char *cp;
    int     dynamic = 0;

    if (ctx == 0)
	ctx = (DB_COMMON_CTX *) (*ctxPtr = db_common_alloc(dict));

    for (cp = format; *cp; ++cp)
	if (*cp == '%')
	    switch (*++cp) {
	    case '%':
		break;
	    case 'u':
		ctx->flags |=
		    query ? DB_COMMON_KEY_USER | DB_COMMON_KEY_PARTIAL
		    : DB_COMMON_VALUE_USER;
		dynamic = 1;
		break;
	    case 'd':
		ctx->flags |=
		    query ? DB_COMMON_KEY_DOMAIN | DB_COMMON_KEY_PARTIAL
		    : DB_COMMON_VALUE_DOMAIN;
		dynamic = 1;
		break;
	    case 's':
	    case 'S':
		dynamic = 1;
		break;
	    case 'U':
		ctx->flags |= DB_COMMON_KEY_PARTIAL | DB_COMMON_KEY_USER;
		dynamic = 1;
		break;
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':

		/*
		 * Find highest %[1-9] index in query template. Input keys
		 * will be constrained to those with at least this many
		 * domain components. This makes the db_common_expand() code
		 * safe from invalid inputs.
		 */
		if (ctx->nparts < *cp - '0')
		    ctx->nparts = *cp - '0';
		/* FALLTHROUGH */
	    case 'D':
		ctx->flags |= DB_COMMON_KEY_PARTIAL | DB_COMMON_KEY_DOMAIN;
		dynamic = 1;
		break;
	    default:
		msg_fatal("db_common_parse: %s: Invalid %s template: %s",
		       ctx->dict->name, query ? "query" : "result", format);
	    }
    return dynamic;
}

/* db_common_parse_domain - parse domain matchlist*/

void    db_common_parse_domain(CFG_PARSER *parser, void *ctxPtr)
{
    DB_COMMON_CTX *ctx = (DB_COMMON_CTX *) ctxPtr;
    char   *domainlist;
    const char *myname = "db_common_parse_domain";

    domainlist = cfg_get_str(parser, "domain", "", 0, 0);
    if (*domainlist) {
	ctx->domain = string_list_init(MATCH_FLAG_RETURN, domainlist);
	if (ctx->domain == 0)

	    /*
	     * The "domain" optimization skips input keys that may in fact
	     * have unwanted matches in the database, so failure to create
	     * the match list is fatal.
	     */
	    msg_fatal("%s: %s: domain match list creation using '%s' failed",
		      myname, parser->name, domainlist);
    }
    myfree(domainlist);
}

/* db_common_dict_partial - Does query use partial lookup keys? */

int     db_common_dict_partial(void *ctxPtr)
{
#if 0					/* Breaks recipient_delimiter */
    DB_COMMON_CTX *ctx = (DB_COMMON_CTX *) ctxPtr;

    return (ctx->domain || ctx->flags & DB_COMMON_KEY_PARTIAL);
#endif
    return (0);
}

/* db_common_free_ctx - free parse context */

void    db_common_free_ctx(void *ctxPtr)
{
    DB_COMMON_CTX *ctx = (DB_COMMON_CTX *) ctxPtr;

    if (ctx->domain)
	string_list_free(ctx->domain);
    myfree((char *) ctxPtr);
}

/* db_common_expand - expand query and result templates */

int     db_common_expand(void *ctxArg, const char *format, const char *value,
			         const char *key, VSTRING *result,
			         db_quote_callback_t quote_func)
{
    const char *myname = "db_common_expand";
    DB_COMMON_CTX *ctx = (DB_COMMON_CTX *) ctxArg;
    const char *vdomain = 0;
    const char *kdomain = 0;
    const char *domain = 0;
    int     dflag = key ? DB_COMMON_VALUE_DOMAIN : DB_COMMON_KEY_DOMAIN;
    char   *vuser = 0;
    char   *kuser = 0;
    ARGV   *parts = 0;
    int     i;
    const char *cp;

    /* Skip NULL values, silently. */
    if (value == 0)
	return (0);

    /* Don't silenty skip empty query string or empty lookup results. */
    if (*value == 0) {
	if (key)
	    msg_warn("table \"%s:%s\": empty lookup result for: \"%s\""
		     " -- ignored", ctx->dict->type, ctx->dict->name, key);
	else
	    msg_warn("table \"%s:%s\": empty query string"
		     " -- ignored", ctx->dict->type, ctx->dict->name);
	return (0);
    }
    if (key) {
	/* This is a result template and the input value is the result */
	if (ctx->flags & (DB_COMMON_VALUE_DOMAIN | DB_COMMON_VALUE_USER))
	    if ((vdomain = strrchr(value, '@')) != 0)
		++vdomain;

	if (((!vdomain || !*vdomain) && (ctx->flags & DB_COMMON_VALUE_DOMAIN) != 0)
	    || (vdomain == value + 1 && (ctx->flags & DB_COMMON_VALUE_USER) != 0))
	    return (0);

	/* The result format may use the local or domain part of the key */
	if (ctx->flags & (DB_COMMON_KEY_DOMAIN | DB_COMMON_KEY_USER))
	    if ((kdomain = strrchr(key, '@')) != 0)
		++kdomain;

	/*
	 * The key should already be checked before the query. No harm if the
	 * query did not get optimized out, so we just issue a warning.
	 */
	if (((!kdomain || !*kdomain) && (ctx->flags & DB_COMMON_KEY_DOMAIN) != 0)
	|| (kdomain == key + 1 && (ctx->flags & DB_COMMON_KEY_USER) != 0)) {
	    msg_warn("%s: %s: lookup key '%s' skipped after query", myname,
		     ctx->dict->name, value);
	    return (0);
	}
    } else {
	/* This is a query template and the input value is the lookup key */
	if (ctx->flags & (DB_COMMON_KEY_DOMAIN | DB_COMMON_KEY_USER))
	    if ((vdomain = strrchr(value, '@')) != 0)
		++vdomain;

	if (((!vdomain || !*vdomain) && (ctx->flags & DB_COMMON_KEY_DOMAIN) != 0)
	|| (vdomain == value + 1 && (ctx->flags & DB_COMMON_KEY_USER) != 0))
	    return (0);
    }

    if (ctx->nparts > 0) {
	parts = argv_split(key ? kdomain : vdomain, ".");

	/*
	 * Filter out input keys whose domains lack enough labels to fill-in
	 * the query template. See below and also db_common_parse() which
	 * initializes ctx->nparts.
	 */
	if (parts->argc < ctx->nparts) {
	    argv_free(parts);
	    return (0);
	}

	/*
	 * Skip domains with leading, consecutive or trailing '.' separators
	 * among the required labels.
	 */
	for (i = 0; i < ctx->nparts; i++)
	    if (*parts->argv[parts->argc - i - 1] == 0) {
		argv_free(parts);
		return (0);
	    }
    }
    if (VSTRING_LEN(result) > 0)
	VSTRING_ADDCH(result, ',');

#define QUOTE_VAL(d, q, v, buf) do { \
	if (q) \
	    q(d, v, buf); \
	else \
	    vstring_strcat(buf, v); \
    } while (0)

    /*
     * Replace all instances of %s with the address to look up. Replace %u
     * with the user portion, and %d with the domain portion. "%%" expands to
     * "%".  lowercase -> addr, uppercase -> key
     */
    for (cp = format; *cp; cp++) {
	if (*cp == '%') {
	    switch (*++cp) {

	    case '%':
		VSTRING_ADDCH(result, '%');
		break;

	    case 's':
		QUOTE_VAL(ctx->dict, quote_func, value, result);
		break;

	    case 'u':
		if (vdomain) {
		    if (vuser == 0)
			vuser = mystrndup(value, vdomain - value - 1);
		    QUOTE_VAL(ctx->dict, quote_func, vuser, result);
		} else
		    QUOTE_VAL(ctx->dict, quote_func, value, result);
		break;

	    case 'd':
		if (!(ctx->flags & dflag))
		    msg_panic("%s: %s: %s: bad query/result template context",
			      myname, ctx->dict->name, format);
		if (!vdomain)
		    msg_panic("%s: %s: %s: expanding domain-less key or value",
			      myname, ctx->dict->name, format);
		QUOTE_VAL(ctx->dict, quote_func, vdomain, result);
		break;

	    case 'S':
		if (key)
		    QUOTE_VAL(ctx->dict, quote_func, key, result);
		else
		    QUOTE_VAL(ctx->dict, quote_func, value, result);
		break;

	    case 'U':
		if (key) {
		    if (kdomain) {
			if (kuser == 0)
			    kuser = mystrndup(key, kdomain - key - 1);
			QUOTE_VAL(ctx->dict, quote_func, kuser, result);
		    } else
			QUOTE_VAL(ctx->dict, quote_func, key, result);
		} else {
		    if (vdomain) {
			if (vuser == 0)
			    vuser = mystrndup(value, vdomain - value - 1);
			QUOTE_VAL(ctx->dict, quote_func, vuser, result);
		    } else
			QUOTE_VAL(ctx->dict, quote_func, value, result);
		}
		break;

	    case 'D':
		if (!(ctx->flags & DB_COMMON_KEY_DOMAIN))
		    msg_panic("%s: %s: %s: bad query/result template context",
			      myname, ctx->dict->name, format);
		if ((domain = key ? kdomain : vdomain) == 0)
		    msg_panic("%s: %s: %s: expanding domain-less key or value",
			      myname, ctx->dict->name, format);
		QUOTE_VAL(ctx->dict, quote_func, domain, result);
		break;

	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':

		/*
		 * Interpolate %[1-9] components into the query string. By
		 * this point db_common_parse() has identified the highest
		 * component index, and (see above) keys with fewer
		 * components have been filtered out. The "parts" ARGV is
		 * guaranteed to be initialized and hold enough elements to
		 * satisfy the query template.
		 */
		if (!(ctx->flags & DB_COMMON_KEY_DOMAIN)
		    || ctx->nparts < *cp - '0')
		    msg_panic("%s: %s: %s: bad query/result template context",
			      myname, ctx->dict->name, format);
		if (!parts || parts->argc < ctx->nparts)
		    msg_panic("%s: %s: %s: key has too few domain labels",
			      myname, ctx->dict->name, format);
		QUOTE_VAL(ctx->dict, quote_func,
			  parts->argv[parts->argc - (*cp - '0')], result);
		break;

	    default:
		msg_fatal("%s: %s: invalid %s template '%s'", myname,
			  ctx->dict->name, key ? "result" : "query",
			  format);
	    }
	} else
	    VSTRING_ADDCH(result, *cp);
    }
    VSTRING_TERMINATE(result);

    if (vuser)
	myfree(vuser);
    if (kuser)
	myfree(kuser);
    if (parts)
	argv_free(parts);

    return (1);
}


/* db_common_check_domain - check domain list */

int     db_common_check_domain(void *ctxPtr, const char *addr)
{
    DB_COMMON_CTX *ctx = (DB_COMMON_CTX *) ctxPtr;
    char   *domain;

    if (ctx->domain) {
	if ((domain = strrchr(addr, '@')) != NULL)
	    ++domain;
	if (domain == NULL || domain == addr + 1)
	    return (0);
	if (match_list_match(ctx->domain, domain) == 0)
	    return (ctx->domain->error);
    }
    return (1);
}

/* db_common_sql_build_query -- build query for SQL maptypes */

void    db_common_sql_build_query(VSTRING *query, CFG_PARSER *parser)
{
    const char *myname = "db_common_sql_build_query";
    char   *table;
    char   *select_field;
    char   *where_field;
    char   *additional_conditions;

    /*
     * Build "old style" query: "select %s from %s where %s"
     */
    if ((table = cfg_get_str(parser, "table", NULL, 1, 0)) == 0)
	msg_fatal("%s: 'table' parameter not defined", myname);

    if ((select_field = cfg_get_str(parser, "select_field", NULL, 1, 0)) == 0)
	msg_fatal("%s: 'select_field' parameter not defined", myname);

    if ((where_field = cfg_get_str(parser, "where_field", NULL, 1, 0)) == 0)
	msg_fatal("%s: 'where_field' parameter not defined", myname);

    additional_conditions = cfg_get_str(parser, "additional_conditions",
					"", 0, 0);

    vstring_sprintf(query, "SELECT %s FROM %s WHERE %s='%%s' %s",
		    select_field, table, where_field,
		    additional_conditions);

    myfree(table);
    myfree(select_field);
    myfree(where_field);
    myfree(additional_conditions);
}
