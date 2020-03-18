/*	$NetBSD: smtp_map11.c,v 1.3 2020/03/18 19:05:20 christos Exp $	*/

/*++
/* NAME
/*	smtp_map11 3
/* SUMMARY
/*	one-to-one address mapping
/* SYNOPSIS
/*	#include <smtp.h>
/*
/*	int	smtp_map11_internal(addr, maps, propagate)
/*	VSTRING	*addr;
/*	MAPS	*maps;
/*	int	propagate;
/*
/*	int	smtp_map11_external(addr, maps, propagate)
/*	VSTRING	*addr;
/*	MAPS	*maps;
/*	int	propagate;
/*
/*	int	smtp_map11_tree(tree, maps, propagate)
/*	TOK822	*tree;
/*	MAPS	*maps;
/*	int	propagate;
/* DESCRIPTION
/*	This module performs non-recursive one-to-one address mapping.
/*	An unmatched address extension is propagated when
/*	\fIpropagate\fR is non-zero.
/*
/*	smtp_map11_internal() looks up the RFC 822 internal (unquoted)
/*	string form of an address in the maps specified via the
/*	\fImaps\fR argument.
/*
/*	smtp_map11_external() is a wrapper around the smtp_map11_internal()
/*	routine that transforms from external (quoted) string form
/*	to internal form and back.
/*
/*	smtp_map11_tree() is a wrapper around the smtp_map11_internal()
/*	routine that transforms from internal parse tree form to
/*	internal form and back.
/* DIAGNOSTICS
/*	Table lookup errors are fatal.
/* SEE ALSO
/*	mail_addr_map(3) address mappings
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
#include <vstring.h>
#include <dict.h>
#include <argv.h>
#include <tok822.h>

/* Global library. */

#include <mail_addr_map.h>
#include <quote_822_local.h>

/* Application-specific. */

#include <smtp.h>

/* smtp_map11_internal - one-to-one table lookups */

int     smtp_map11_internal(VSTRING *addr, MAPS *maps, int propagate)
{
    const char *myname = "smtp_map11_internal";
    ARGV   *new_addr;
    const char *result;

    if ((new_addr = mail_addr_map_internal(maps, STR(addr), propagate)) != 0) {
	if (new_addr->argc > 1)
	    msg_warn("multi-valued %s result for %s", maps->title, STR(addr));
	result = new_addr->argv[0];
	if (msg_verbose)
	    msg_info("%s: %s -> %s", myname, STR(addr), result);
	vstring_strcpy(addr, result);
	argv_free(new_addr);
	return (1);
    } else {
	if (maps->error != 0)
	    msg_fatal("%s map lookup problem for %s", maps->title, STR(addr));
	if (msg_verbose)
	    msg_info("%s: %s not found", myname, STR(addr));
	return (0);
    }
}

/* smtp_map11_tree - rewrite address node */

int     smtp_map11_tree(TOK822 *tree, MAPS *maps, int propagate)
{
    VSTRING *int_buf = vstring_alloc(100);
    VSTRING *ext_buf = vstring_alloc(100);
    int     ret;

    tok822_internalize(int_buf, tree->head, TOK822_STR_DEFL);
    ret = smtp_map11_internal(int_buf, maps, propagate);
    tok822_free_tree(tree->head);
    quote_822_local(ext_buf, STR(int_buf));
    tree->head = tok822_scan(STR(ext_buf), &tree->tail);
    vstring_free(int_buf);
    vstring_free(ext_buf);
    return (ret);
}

/* smtp_map11_external - rewrite address external form */

int     smtp_map11_external(VSTRING *addr, MAPS *maps, int propagate)
{
    VSTRING *temp = vstring_alloc(100);
    int     ret;

    unquote_822_local(temp, STR(addr));
    ret = smtp_map11_internal(temp, maps, propagate);
    quote_822_local(addr, STR(temp));
    vstring_free(temp);
    return (ret);
}

#ifdef TEST
#include <ctype.h>

#include <msg_vstream.h>
#include <readlline.h>
#include <stringops.h>
#include <vstring_vstream.h>

#include <canon_addr.h>
#include <mail_params.h>

/* canon_addr_external - surrogate to avoid trivial-rewrite dependency */

VSTRING *canon_addr_external(VSTRING *result, const char *addr)
{
    char   *at;

    vstring_strcpy(result, addr);
    if ((at = strrchr(addr, '@')) == 0
	|| (at + 1)[strcspn(at + 1, "\"\\")] != 0)
	vstring_sprintf_append(result, "@%s", var_myorigin);
    return (result);
}

static NORETURN usage(const char *progname)
{
    msg_fatal("usage: %s [-v]", progname);
}

int     main(int argc, char **argv)
{
    VSTRING *read_buf = vstring_alloc(100);
    MAPS   *maps = 0;
    int     lineno;
    int     first_line;
    char   *bp;
    char   *cmd;
    VSTRING *addr_buf = vstring_alloc(100);
    char   *addr_field;
    char   *res_field;
    int     ch;
    int     errs = 0;

    /*
     * Initialize.
     */
    msg_vstream_init(basename(argv[0]), VSTREAM_ERR);

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

    util_utf8_enable = 1;
    mail_params_init();

    /*
     * TODO: Data-driven parameter settings.
     */
#define UPDATE(var, val) do { myfree(var); var = mystrdup(val); } while (0)

    UPDATE(var_myhostname, "localhost.localdomain");
    UPDATE(var_mydomain, "localdomain");
    UPDATE(var_myorigin, "localdomain");
    UPDATE(var_mydest, "localhost.localdomain");
    UPDATE(var_rcpt_delim, "+");

    /*
     * The read-eval-print loop.
     */
    while (readllines(read_buf, VSTREAM_IN, &lineno, &first_line)) {
	bp = STR(read_buf);
	if (msg_verbose)
	    msg_info("> %s", bp);
	if ((cmd = mystrtok(&bp, CHARS_SPACE)) == 0 || *cmd == '#')
	    continue;
	while (ISSPACE(*bp))
	    bp++;
	if (*bp == 0)
	    msg_fatal("missing parameter for command %s", cmd);

	/*
	 * Open maps.
	 */
	if (strcmp(cmd, "maps") == 0) {
	    if (maps)
		maps_free(maps);
	    maps = maps_create(bp, bp, DICT_FLAG_FOLD_FIX
			       | DICT_FLAG_UTF8_REQUEST);
	    vstream_printf("%s\n", bp);
	    continue;
	}

	/*
	 * Lookup and verify.
	 */
	else if (maps != 0 && (strcmp(cmd, "external") == 0
			       || strcmp(cmd, "internal") == 0
			       || strcmp(cmd, "tree") == 0)) {
	    int     have_result = 0;


	    /*
	     * Parse the input and expectations.
	     */
	    if ((addr_field = mystrtok(&bp, ":")) == 0)
		msg_fatal("missing address field");
	    res_field = mystrtok(&bp, ":");
	    if (mystrtok(&bp, ":") != 0)
		msg_fatal("garbage after result field");

	    /*
	     * Perform the mapping.
	     */
	    if (strcmp(cmd, "external") == 0) {
		vstring_strcpy(addr_buf, addr_field);
		have_result = smtp_map11_external(addr_buf, maps, 1);
	    } else if (maps && strcmp(cmd, "internal") == 0) {
		vstring_strcpy(addr_buf, addr_field);
		have_result = smtp_map11_internal(addr_buf, maps, 1);
	    } else if (maps && strcmp(cmd, "tree") == 0) {
		TOK822 *tree;
		TOK822 **addr_list;
		TOK822 **tpp;

		tree = tok822_parse(addr_field);
		addr_list = tok822_grep(tree, TOK822_ADDR);
		for (tpp = addr_list; *tpp; tpp++)
		    have_result |= smtp_map11_tree(tpp[0], maps, 1);
		myfree((void *) addr_list);
		if (have_result)
		    tok822_externalize(addr_buf, tree, TOK822_STR_DEFL);
		tok822_free_tree(tree);
	    }

	    /*
	     * Summarize.
	     */
	    vstream_printf("%s:%s -> %s\n",
			   cmd, addr_field, have_result ?
			   STR(addr_buf) : maps->error ?
			   "(error)" : "(no match)");
	    vstream_fflush(VSTREAM_OUT);

	    /*
	     * Enforce expectations.
	     */
	    if (res_field && have_result) {
		if (strcmp(res_field, STR(addr_buf)) != 0) {
		    msg_warn("expect result '%s' but got '%s'",
			     res_field, STR(addr_buf));
		    errs = 1;
		}
	    } else if (res_field && !have_result) {
		msg_warn("expect result '%s' but got none", res_field);
		errs = 1;
	    } else if (!res_field && have_result) {
		msg_warn("expected no result but got '%s'", STR(addr_buf));
		errs = 1;
	    }
	    vstream_fflush(VSTREAM_OUT);
	}

	/*
	 * Unknown request.
	 */
	else {
	    msg_fatal("bad request: %s", cmd);
	}
    }
    vstring_free(addr_buf);
    vstring_free(read_buf);
    maps_free(maps);
    return (errs != 0);
}

#endif
