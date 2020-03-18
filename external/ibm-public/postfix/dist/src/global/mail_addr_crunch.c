/*	$NetBSD: mail_addr_crunch.c,v 1.3 2020/03/18 19:05:16 christos Exp $	*/

/*++
/* NAME
/*	mail_addr_crunch 3
/* SUMMARY
/*	parse and canonicalize addresses, apply address extension
/* SYNOPSIS
/*	#include <mail_addr_crunch.h>
/*
/*	ARGV	*mail_addr_crunch_ext_to_int(string, extension)
/*	const char *string;
/*	const char *extension;
/*
/*	ARGV	*mail_addr_crunch_opt(string, extension, in_form, out_form)
/*	const char *string;
/*	const char *extension;
/*	int	in_form;
/*	int	out_form;
/* DESCRIPTION
/*	mail_addr_crunch_*() parses a string with zero or more addresses,
/*	rewrites each address to canonical form, and optionally applies
/*	an address extension to each resulting address. The caller is
/*	expected to pass the result to argv_free().
/*
/*	With mail_addr_crunch_ext_to_int(), the string is in external
/*	form, and the result is in internal form. This API minimizes
/*	the number of conversions between internal and external forms.
/*
/*	mail_addr_crunch_opt() gives more control, at the cost of
/*	additional conversions between internal and external forms.
/*
/*	Arguments:
/* .IP string
/*	A string with zero or more addresses in external (quoted)
/*	form, or in the form specified with the in_form argument.
/* .IP extension
/*	A null pointer, or an address extension (including the recipient
/*	address delimiter) that is propagated to all result addresses.
/*	This is in internal (unquoted) form.
/* .IP in_form
/* .IP out_form
/*	Input and output address forms, either MA_FORM_INTERNAL
/*	(unquoted form) or MA_FORM_EXTERNAL (quoted form).
/* DIAGNOSTICS
/*	Fatal error: out of memory.
/* SEE ALSO
/*	tok822_parse(3), address parser
/*	canon_addr(3), address canonicalization
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

#include <mymalloc.h>
#include <argv.h>
#include <vstring.h>

/* Global library. */

#include <tok822.h>
#include <canon_addr.h>
#include <quote_822_local.h>
#include <mail_addr_crunch.h>

/* mail_addr_crunch - break string into addresses, optionally add extension */

ARGV   *mail_addr_crunch_opt(const char *string, const char *extension,
			             int in_form, int out_form)
{
    VSTRING *intern_addr = vstring_alloc(100);
    VSTRING *extern_addr = vstring_alloc(100);
    VSTRING *canon_addr = vstring_alloc(100);
    ARGV   *argv = argv_alloc(1);
    TOK822 *tree;
    TOK822 **addr_list;
    TOK822 **tpp;
    char   *ratsign;
    ssize_t extlen;

    if (extension)
	extlen = strlen(extension);

#define STR(x) vstring_str(x)

    /*
     * Optionally convert input from internal form.
     */
    if (in_form == MA_FORM_INTERNAL) {
	quote_822_local(extern_addr, string);
	string = STR(extern_addr);
    }

    /*
     * Parse the string, rewrite each address to canonical form, and convert
     * the result to external (quoted) form. Optionally apply the extension
     * to each address found.
     * 
     * XXX Workaround for the null address. This works for envelopes but
     * produces ugly results for message headers.
     */
    if (*string == 0 || strcmp(string, "<>") == 0)
	string = "\"\"";
    tree = tok822_parse(string);
    /* string->extern_addr would be invalidated by tok822_externalize() */
    string = 0;
    addr_list = tok822_grep(tree, TOK822_ADDR);
    for (tpp = addr_list; *tpp; tpp++) {
	tok822_externalize(extern_addr, tpp[0]->head, TOK822_STR_DEFL);
	canon_addr_external(canon_addr, STR(extern_addr));
	unquote_822_local(intern_addr, STR(canon_addr));
	if (extension) {
	    VSTRING_SPACE(intern_addr, extlen + 1);
	    if ((ratsign = strrchr(STR(intern_addr), '@')) == 0) {
		vstring_strcat(intern_addr, extension);
	    } else {
		memmove(ratsign + extlen, ratsign, strlen(ratsign) + 1);
		memcpy(ratsign, extension, extlen);
		VSTRING_SKIP(intern_addr);
	    }
	}
	/* Optionally convert output to external form. */
	if (out_form == MA_FORM_EXTERNAL) {
	    quote_822_local(extern_addr, STR(intern_addr));
	    argv_add(argv, STR(extern_addr), ARGV_END);
	} else {
	    argv_add(argv, STR(intern_addr), ARGV_END);
	}
    }
    argv_terminate(argv);
    myfree((void *) addr_list);
    tok822_free_tree(tree);
    vstring_free(canon_addr);
    vstring_free(extern_addr);
    vstring_free(intern_addr);
    return (argv);
}

#ifdef TEST

 /*
  * Stand-alone test program, sort of interactive.
  */
#include <stdlib.h>
#include <unistd.h>
#include <msg.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <mail_conf.h>
#include <mail_params.h>

/* canon_addr_external - surrogate to avoid trivial-rewrite dependency */

VSTRING *canon_addr_external(VSTRING *result, const char *addr)
{
    return (vstring_strcpy(result, addr));
}

static int get_addr_form(const char *prompt, VSTRING *buf)
{
    int     addr_form;

    if (prompt) {
	vstream_printf("%s: ", prompt);
	vstream_fflush(VSTREAM_OUT);
    }
    if (vstring_get_nonl(buf, VSTREAM_IN) == VSTREAM_EOF)
	exit(0);
    if ((addr_form = mail_addr_form_from_string(STR(buf))) < 0)
	msg_fatal("bad address form: %s", STR(buf));
    return (addr_form);
}

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *extension = vstring_alloc(1);
    VSTRING *buf = vstring_alloc(1);
    ARGV   *argv;
    char  **cpp;
    int     do_prompt = isatty(0);
    int     in_form;
    int     out_form;

    mail_conf_read();
    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir %s: %m", var_queue_dir);

    in_form = get_addr_form(do_prompt ? "input form" : 0, buf);
    out_form = get_addr_form(do_prompt ? "output form" : 0, buf);
    if (do_prompt) {
	vstream_printf("extension: (CR for none): ");
	vstream_fflush(VSTREAM_OUT);
    }
    if (vstring_get_nonl(extension, VSTREAM_IN) == VSTREAM_EOF)
	exit(0);

    if (do_prompt) {
	vstream_printf("print strings to be translated, one per line\n");
	vstream_fflush(VSTREAM_OUT);
    }
    while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	argv = mail_addr_crunch_opt(STR(buf), (VSTRING_LEN(extension) ?
					       STR(extension) : 0),
				    in_form, out_form);
	for (cpp = argv->argv; *cpp; cpp++)
	    vstream_printf("|%s|\n", *cpp);
	vstream_fflush(VSTREAM_OUT);
	argv_free(argv);
    }
    vstring_free(extension);
    vstring_free(buf);
    return (0);
}

#endif
