/*	$NetBSD: mail_addr_find.c,v 1.4 2022/10/08 16:12:45 christos Exp $	*/

/*++
/* NAME
/*	mail_addr_find 3
/* SUMMARY
/*	generic address-based lookup
/* SYNOPSIS
/*	#include <mail_addr_find.h>
/*
/*	const char *mail_addr_find_int_to_ext(maps, address, extension)
/*	MAPS	*maps;
/*	const char *address;
/*	char	**extension;
/*
/*	const char *mail_addr_find_opt(maps, address, extension, in_form,
/*					query_form, out_form, strategy)
/*	MAPS	*maps;
/*	const char *address;
/*	char	**extension;
/*	int	in_form;
/*	int	in_form;
/*	int	out_form;
/*	int	strategy;
/* LEGACY SUPPORT
/*	const char *mail_addr_find(maps, address, extension)
/*	MAPS	*maps;
/*	const char *address;
/*	char	**extension;
/*
/*	const char *mail_addr_find_to_internal(maps, address, extension)
/*	MAPS	*maps;
/*	const char *address;
/*	char	**extension;
/*
/*	const char *mail_addr_find_strategy(maps, address, extension)
/*	MAPS	*maps;
/*	const char *address;
/*	char	**extension;
/*	int	strategy;
/* DESCRIPTION
/*	mail_addr_find*() searches the specified maps for an entry with as
/*	key the specified address, and derivations from that address.
/*	It is up to the caller to specify its case sensitivity
/*	preferences when it opens the maps.
/*	The result is overwritten upon each call.
/*
/*	In the lookup table, the key is expected to be in external
/*	form (as produced with the postmap command) and the value is
/*	expected to be in external (quoted) form if it is an email
/*	address. Override these assumptions with the query_form
/*	and out_form arguments.
/*
/*	With mail_addr_find_int_to_ext(), the specified address is in
/*	internal (unquoted) form, the query is made in external (quoted)
/*	form, and the result is in the form found in the table (it is
/*	not necessarily an email address). This version minimizes
/*	internal/external (unquoted/quoted) conversions of the input,
/*	query, extension, or result.
/*
/*	mail_addr_find_opt() gives more control, at the cost of
/*	additional conversions between internal and external forms.
/*	In particular, output conversion to internal form assumes
/*	that the lookup result is an email address.
/*
/*	mail_addr_find() is used by legacy code that historically searched
/*	with internal-form queries. The input is in internal form. It
/*	searches with external-form queries first, and falls back to
/*	internal-form queries if no result was found and the external
/*	and internal forms differ. The result is external form (i.e. no
/*	conversion).
/*
/*	mail_addr_find_to_internal() is like mail_addr_find() but assumes
/*	that the lookup result is one external-form email address,
/*	and converts it to internal form.
/*
/*	mail_addr_find_strategy() is like mail_addr_find() but overrides
/*	the default search strategy for full and partial addresses.
/*
/*	Arguments:
/* .IP maps
/*	Dictionary search path (see maps(3)).
/* .IP address
/*	The address to be looked up.
/* .IP extension
/*	A null pointer, or the address of a pointer that is set to
/*	the address of a dynamic memory copy of the address extension
/*	that had to be chopped off in order to match the lookup tables.
/*	The copy includes the recipient address delimiter.
/*	The copy is in internal (unquoted) form.
/*	The caller is expected to pass the copy to myfree().
/* .IP query_form
/*	The address form to use for database queries: one of
/*	MA_FORM_INTERNAL (unquoted form), MA_FORM_EXTERNAL (quoted form),
/*	MA_FORM_EXTERNAL_FIRST (external form, then internal form if the
/*	external and internal forms differ), or MA_FORM_INTERNAL_FIRST
/*	(internal form, then external form if the internal and external
/*	forms differ).
/* .IP in_form
/* .IP out_form
/*	Input and output address forms, one of MA_FORM_INTERNAL (unquoted
/*	form), or MA_FORM_EXTERNAL (quoted form).
/* .IP strategy
/*	The lookup strategy for full and partial addresses, specified
/*	as the binary OR of one or more of the following. These lookups
/*	are implemented in the order as listed below.
/* .RS
/* .IP MA_FIND_DEFAULT
/*	A convenience alias for (MA_FIND_FULL |
/*	MA_FIND_NOEXT | MA_FIND_LOCALPART_IF_LOCAL |
/*	MA_FIND_AT_DOMAIN).
/* .IP MA_FIND_FULL
/*	Look up the full email address.
/* .IP MA_FIND_NOEXT
/*	If no match was found, and the address has a localpart extension,
/*	look up the address after removing the extension.
/* .IP MA_FIND_LOCALPART_IF_LOCAL
/*	If no match was found, and the domain matches myorigin,
/*	mydestination, or any inet_interfaces or proxy_interfaces IP
/*	address, look up the localpart.  If no match was found, and the
/*	address has a localpart extension, repeat the same query after
/*	removing the extension unless MA_FIND_NOEXT is specified.
/* .IP MA_FIND_LOCALPART_AT_IF_LOCAL
/*	As above, but using the localpart@ instead.
/* .IP MA_FIND_AT_DOMAIN
/*	If no match was found, look up the @domain without localpart.
/* .IP MA_FIND_DOMAIN
/*	If no match was found, look up the domain without localpart.
/* .IP MA_FIND_PDMS
/*	When used with MA_FIND_DOMAIN, the domain also matches subdomains.
/* .IP MA_FIND_PDDMDS
/*	When used with MA_FIND_DOMAIN, dot-domain also matches
/*	dot-subdomains.
/* .IP MA_FIND_LOCALPART_AT
/*	If no match was found, look up the localpart@, regardless of
/*	the domain content.
/* .RE
/* DIAGNOSTICS
/*	The maps->error value is non-zero when the lookup failed due to
/*	a non-permanent error.
/* SEE ALSO
/*	maps(3), multi-dictionary search resolve_local(3), recognize
/*	local system
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
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <name_mask.h>
#include <dict.h>
#include <stringops.h>
#include <mymalloc.h>
#include <vstring.h>

/* Global library. */

#include <mail_params.h>
#include <strip_addr.h>
#include <mail_addr_find.h>
#include <resolve_local.h>
#include <quote_822_local.h>

/* Application-specific. */

#define STR	vstring_str

#ifdef TEST

static const NAME_MASK strategy_table[] = {
    "full", MA_FIND_FULL,
    "noext", MA_FIND_NOEXT,
    "localpart_if_local", MA_FIND_LOCALPART_IF_LOCAL,
    "localpart_at_if_local", MA_FIND_LOCALPART_AT_IF_LOCAL,
    "at_domain", MA_FIND_AT_DOMAIN,
    "domain", MA_FIND_DOMAIN,
    "pdms", MA_FIND_PDMS,
    "pddms", MA_FIND_PDDMDS,
    "localpart_at", MA_FIND_LOCALPART_AT,
    "default", MA_FIND_DEFAULT,
    0, -1,
};

/* strategy_from_string - symbolic strategy flags to internal form */

static int strategy_from_string(const char *strategy_string)
{
    return (name_mask_delim_opt("strategy_from_string", strategy_table,
				strategy_string, "|",
				NAME_MASK_WARN | NAME_MASK_ANY_CASE));
}

/* strategy_to_string - internal form to symbolic strategy flags */

static const char *strategy_to_string(VSTRING *res_buf, int strategy_mask)
{
    static VSTRING *my_buf;

    if (res_buf == 0 && (res_buf = my_buf) == 0)
	res_buf = my_buf = vstring_alloc(20);
    return (str_name_mask_opt(res_buf, "strategy_to_string",
			      strategy_table, strategy_mask,
			      NAME_MASK_WARN | NAME_MASK_PIPE));
}

#endif

 /*
  * Specify what keys are partial or full, to avoid matching partial
  * addresses with regular expressions.
  */
#define FULL	0
#define PARTIAL	DICT_FLAG_FIXED

/* find_addr - helper to search maps with the right query form */

static const char *find_addr(MAPS *path, const char *address, int flags,
	             int with_domain, int query_form, VSTRING *ext_addr_buf)
{
    const char *result;

#define SANS_DOMAIN	0
#define WITH_DOMAIN	1

    switch (query_form) {

	/*
	 * Query with external-form (quoted) address. The code looks a bit
	 * unusual to emphasize the symmetry with the other cases.
	 */
    case MA_FORM_EXTERNAL:
    case MA_FORM_EXTERNAL_FIRST:
	quote_822_local_flags(ext_addr_buf, address,
			      with_domain ? QUOTE_FLAG_DEFAULT :
			    QUOTE_FLAG_DEFAULT | QUOTE_FLAG_BARE_LOCALPART);
	result = maps_find(path, STR(ext_addr_buf), flags);
	if (result != 0 || path->error != 0
	    || query_form != MA_FORM_EXTERNAL_FIRST
	    || strcmp(address, STR(ext_addr_buf)) == 0)
	    break;
	result = maps_find(path, address, flags);
	break;

	/*
	 * Query with internal-form (unquoted) address. The code looks a bit
	 * unusual to emphasize the symmetry with the other cases.
	 */
    case MA_FORM_INTERNAL:
    case MA_FORM_INTERNAL_FIRST:
	result = maps_find(path, address, flags);
	if (result != 0 || path->error != 0
	    || query_form != MA_FORM_INTERNAL_FIRST)
	    break;
	quote_822_local_flags(ext_addr_buf, address,
			      with_domain ? QUOTE_FLAG_DEFAULT :
			    QUOTE_FLAG_DEFAULT | QUOTE_FLAG_BARE_LOCALPART);
	if (strcmp(address, STR(ext_addr_buf)) == 0)
	    break;
	result = maps_find(path, STR(ext_addr_buf), flags);
	break;

	/*
	 * Can't happen.
	 */
    default:
	msg_panic("mail_addr_find: bad query_form: %d", query_form);
    }
    return (result);
}

/* find_local - search on localpart info */

static const char *find_local(MAPS *path, char *ratsign, int rats_offs,
			              char *int_full_key, char *int_bare_key,
		              int query_form, char **extp, char **saved_ext,
			              VSTRING *ext_addr_buf)
{
    const char *myname = "mail_addr_find";
    const char *result;
    int     with_domain;
    int     saved_ch;

    /*
     * This code was ripped from the middle of a function so that it can be
     * reused multiple times, that's why the interface makes little sense.
     */
    with_domain = rats_offs ? WITH_DOMAIN : SANS_DOMAIN;

    saved_ch = *(unsigned char *) (ratsign + rats_offs);
    *(ratsign + rats_offs) = 0;
    result = find_addr(path, int_full_key, PARTIAL, with_domain,
		       query_form, ext_addr_buf);
    *(ratsign + rats_offs) = saved_ch;
    if (result == 0 && path->error == 0 && int_bare_key != 0) {
	if ((ratsign = strrchr(int_bare_key, '@')) == 0)
	    msg_panic("%s: bare key botch", myname);
	saved_ch = *(unsigned char *) (ratsign + rats_offs);
	*(ratsign + rats_offs) = 0;
	if ((result = find_addr(path, int_bare_key, PARTIAL, with_domain,
				query_form, ext_addr_buf)) != 0
	    && extp != 0) {
	    *extp = *saved_ext;
	    *saved_ext = 0;
	}
	*(ratsign + rats_offs) = saved_ch;
    }
    return result;
}

/* mail_addr_find_opt - map a canonical address */

const char *mail_addr_find_opt(MAPS *path, const char *address, char **extp,
			               int in_form, int query_form,
			               int out_form, int strategy)
{
    const char *myname = "mail_addr_find";
    VSTRING *ext_addr_buf = 0;
    VSTRING *int_addr_buf = 0;
    const char *int_addr;
    static VSTRING *int_result = 0;
    const char *result;
    char   *ratsign = 0;
    char   *int_full_key;
    char   *int_bare_key;
    char   *saved_ext;
    int     rc = 0;

    /*
     * Optionally convert the address from external form.
     */
    if (in_form == MA_FORM_EXTERNAL) {
	int_addr_buf = vstring_alloc(100);
	unquote_822_local(int_addr_buf, address);
	int_addr = STR(int_addr_buf);
    } else {
	int_addr = address;
    }
    if (query_form == MA_FORM_EXTERNAL_FIRST
	|| query_form == MA_FORM_EXTERNAL)
	ext_addr_buf = vstring_alloc(100);

    /*
     * Initialize.
     */
    int_full_key = mystrdup(int_addr);
    if (*var_rcpt_delim == 0 || (strategy & MA_FIND_NOEXT) == 0) {
	int_bare_key = saved_ext = 0;
    } else {
	/* XXX This could be done after user+foo@domain fails. */
	int_bare_key =
	    strip_addr_internal(int_full_key, &saved_ext, var_rcpt_delim);
    }

    /*
     * Try user+foo@domain and user@domain.
     */
    if ((strategy & MA_FIND_FULL) != 0) {
	result = find_addr(path, int_full_key, FULL, WITH_DOMAIN,
			   query_form, ext_addr_buf);
    } else {
	result = 0;
	path->error = 0;
    }

    if (result == 0 && path->error == 0 && int_bare_key != 0
	&& (result = find_addr(path, int_bare_key, PARTIAL, WITH_DOMAIN,
			       query_form, ext_addr_buf)) != 0
	&& extp != 0) {
	*extp = saved_ext;
	saved_ext = 0;
    }

    /*
     * Try user+foo if the domain matches user+foo@$myorigin,
     * user+foo@$mydestination or user+foo@[${proxy,inet}_interfaces]. Then
     * try with +foo stripped off.
     */
    if (result == 0 && path->error == 0
	&& (ratsign = strrchr(int_full_key, '@')) != 0
	&& (strategy & (MA_FIND_LOCALPART_IF_LOCAL
			| MA_FIND_LOCALPART_AT_IF_LOCAL)) != 0) {
	if (strcasecmp_utf8(ratsign + 1, var_myorigin) == 0
	    || (rc = resolve_local(ratsign + 1)) > 0) {
	    if ((strategy & MA_FIND_LOCALPART_IF_LOCAL) != 0)
		result = find_local(path, ratsign, 0, int_full_key,
				 int_bare_key, query_form, extp, &saved_ext,
				    ext_addr_buf);
	    if (result == 0 && path->error == 0
		&& (strategy & MA_FIND_LOCALPART_AT_IF_LOCAL) != 0)
		result = find_local(path, ratsign, 1, int_full_key,
				 int_bare_key, query_form, extp, &saved_ext,
				    ext_addr_buf);
	} else if (rc < 0)
	    path->error = rc;
    }

    /*
     * Try @domain.
     */
    if (result == 0 && path->error == 0 && ratsign != 0
	&& (strategy & MA_FIND_AT_DOMAIN) != 0)
	result = maps_find(path, ratsign, PARTIAL);

    /*
     * Try domain (optionally, subdomains).
     */
    if (result == 0 && path->error == 0 && ratsign != 0
	&& (strategy & MA_FIND_DOMAIN) != 0) {
	const char *name;
	const char *next;

	if ((strategy & MA_FIND_PDMS) && (strategy & MA_FIND_PDDMDS))
	    msg_warn("mail_addr_find_opt: do not specify both "
		     "MA_FIND_PDMS and MA_FIND_PDDMDS");
	for (name = ratsign + 1; *name != 0; name = next) {
	    if ((result = maps_find(path, name, PARTIAL)) != 0
		|| path->error != 0
		|| (strategy & (MA_FIND_PDMS | MA_FIND_PDDMDS)) == 0
		|| (next = strchr(name + 1, '.')) == 0)
		break;
	    if ((strategy & MA_FIND_PDDMDS) == 0)
		next++;
	}
    }

    /*
     * Try localpart@ even if the domain is not local.
     */
    if ((strategy & MA_FIND_LOCALPART_AT) != 0 \
	&&result == 0 && path->error == 0)
	result = find_local(path, ratsign, 1, int_full_key,
			    int_bare_key, query_form, extp, &saved_ext,
			    ext_addr_buf);

    /*
     * Optionally convert the result to internal form. The lookup result is
     * supposed to be one external-form email address.
     */
    if (result != 0 && out_form == MA_FORM_INTERNAL) {
	if (int_result == 0)
	    int_result = vstring_alloc(100);
	unquote_822_local(int_result, result);
	result = STR(int_result);
    }

    /*
     * Clean up.
     */
    if (msg_verbose)
	msg_info("%s: %s -> %s", myname, address,
		 result ? result :
		 path->error ? "(try again)" :
		 "(not found)");
    myfree(int_full_key);
    if (int_bare_key)
	myfree(int_bare_key);
    if (saved_ext)
	myfree(saved_ext);
    if (int_addr_buf)
	vstring_free(int_addr_buf);
    if (ext_addr_buf)
	vstring_free(ext_addr_buf);
    return (result);
}

#ifdef TEST

 /*
  * Proof-of-concept test program. Read an address and expected results from
  * stdin, and warn about any discrepancies.
  */
#include <ctype.h>
#include <stdlib.h>

#include <vstream.h>
#include <vstring_vstream.h>
#include <mail_params.h>

static NORETURN usage(const char *progname)
{
    msg_fatal("usage: %s [-v]", progname);
}

int     main(int argc, char **argv)
{
    VSTRING *buffer = vstring_alloc(100);
    char   *bp;
    MAPS   *path = 0;
    const char *result;
    char   *extent;
    char   *cmd;
    char   *in_field;
    char   *query_field;
    char   *out_field;
    char   *strategy_field;
    char   *key_field;
    char   *expect_res;
    char   *expect_ext;
    int     in_form;
    int     query_form;
    int     out_form;
    int     strategy_flags;
    int     ch;
    int     errs = 0;

    /*
     * Parse JCL.
     */
    while ((ch = GETOPT(argc, argv, "v")) > 0) {
	switch (ch) {
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    usage(argv[0]);
	}
    }
    if (argc != optind)
	usage(argv[0]);

    /*
     * Initialize.
     */
#define UPDATE(var, val) do { myfree(var); var = mystrdup(val); } while (0)

    mail_params_init();

    /*
     * TODO: move these assignments into the read/eval loop.
     */
    UPDATE(var_rcpt_delim, "+");
    UPDATE(var_mydomain, "localdomain");
    UPDATE(var_myorigin, "localdomain");
    UPDATE(var_mydest, "localhost.localdomain");
    while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	bp = STR(buffer);
	if (msg_verbose)
	    msg_info("> %s", bp);
	if ((cmd = mystrtok(&bp, CHARS_SPACE)) == 0 || *cmd == '#')
	    continue;
	while (ISSPACE(*bp))
	    bp++;

	/*
	 * Visible comment.
	 */
	if (strcmp(cmd, "echo") == 0) {
	    vstream_printf("%s\n", bp);
	}

	/*
	 * Open maps.
	 */
	else if (strcmp(cmd, "maps") == 0) {
	    if (path)
		maps_free(path);
	    path = maps_create(argv[0], bp, DICT_FLAG_LOCK
			     | DICT_FLAG_FOLD_FIX | DICT_FLAG_UTF8_REQUEST);
	    vstream_printf("%s\n", bp);
	    continue;
	}

	/*
	 * Lookup and verify.
	 */
	else if (path && strcmp(cmd, "test") == 0) {

	    /*
	     * Parse the input and expectations.
	     */
	    /* internal, external. */
	    if ((in_field = mystrtok(&bp, ":")) == 0)
		msg_fatal("no input form");
	    if ((in_form = mail_addr_form_from_string(in_field)) < 0)
		msg_fatal("bad input form: '%s'", in_field);
	    if ((query_field = mystrtok(&bp, ":")) == 0)
		msg_fatal("no query form");
	    /* internal, external, external-first. */
	    if ((query_form = mail_addr_form_from_string(query_field)) < 0)
		msg_fatal("bad query form: '%s'", query_field);
	    if ((out_field = mystrtok(&bp, ":")) == 0)
		msg_fatal("no output form");
	    /* internal, external. */
	    if ((out_form = mail_addr_form_from_string(out_field)) < 0)
		msg_fatal("bad output form: '%s'", out_field);
	    if ((strategy_field = mystrtok(&bp, ":")) == 0)
		msg_fatal("no strategy field");
	    if ((strategy_flags = strategy_from_string(strategy_field)) < 0)
		msg_fatal("bad strategy field: '%s'", strategy_field);
	    if ((key_field = mystrtok(&bp, ":")) == 0)
		msg_fatal("no search key");
	    expect_res = mystrtok(&bp, ":");
	    expect_ext = mystrtok(&bp, ":");
	    if (mystrtok(&bp, ":") != 0)
		msg_fatal("garbage after extension field");

	    /*
	     * Lookups.
	     */
	    extent = 0;
	    result = mail_addr_find_opt(path, key_field, &extent,
					in_form, query_form, out_form,
					strategy_flags);
	    vstream_printf("%s:%s -%s-> %s:%s (%s)\n",
	      in_field, key_field, query_field, out_field, result ? result :
			   path->error ? "(try again)" :
			 "(not found)", extent ? extent : "null extension");
	    vstream_fflush(VSTREAM_OUT);

	    /*
	     * Enforce expectations.
	     */
	    if (expect_res && result) {
		if (strcmp(expect_res, result) != 0) {
		    msg_warn("expect result '%s' but got '%s'", expect_res, result);
		    errs = 1;
		    if (expect_ext && extent) {
			if (strcmp(expect_ext, extent) != 0)
			    msg_warn("expect extension '%s' but got '%s'",
				     expect_ext, extent);
			errs = 1;
		    } else if (expect_ext && !extent) {
			msg_warn("expect extension '%s' but got none", expect_ext);
			errs = 1;
		    } else if (!expect_ext && extent) {
			msg_warn("expect no extension but got '%s'", extent);
			errs = 1;
		    }
		}
	    } else if (expect_res && !result) {
		msg_warn("expect result '%s' but got none", expect_res);
		errs = 1;
	    } else if (!expect_res && result) {
		msg_warn("expected no result but got '%s'", result);
		errs = 1;
	    }
	    vstream_fflush(VSTREAM_OUT);
	    if (extent)
		myfree(extent);
	}

	/*
	 * Unknown request.
	 */
	else {
	    msg_warn("bad request: %s", cmd);
	}
    }
    vstring_free(buffer);

    maps_free(path);
    return (errs != 0);
}

#endif
