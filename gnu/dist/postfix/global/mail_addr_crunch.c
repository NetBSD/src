/*++
/* NAME
/*	mail_addr_crunch 3
/* SUMMARY
/*	parse and canonicalize addresses, apply address extension
/* SYNOPSIS
/*	#include <mail_addr_crunch.h>
/*
/*	ARGV	*mail_addr_crunch(string, extension)
/*	const char *string;
/*	const char *extension;
/* DESCRIPTION
/*	mail_addr_crunch() parses a string with zero or more addresses,
/*	rewrites each address to canonical form, and optionally applies
/*	an address extension to each resulting address. Input and result
/*	are in external (quoted) format. The caller is expected to pass
/*	the result to argv_free().
/*
/*	Arguments:
/* .IP string
/*	A string with zero or more addresses in RFC 822 (external) format.
/* .IP extension
/*	A null pointer, or an address extension (including the recipient
/*	address delimiter) that is propagated to all result addresses.
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
#include <mail_addr_crunch.h>

/* mail_addr_crunch - break string into addresses, optionally add extension */

ARGV   *mail_addr_crunch(const char *string, const char *extension)
{
    VSTRING *extern_addr = vstring_alloc(100);
    VSTRING *canon_addr = vstring_alloc(100);
    ARGV   *argv = argv_alloc(1);
    TOK822 *tree;
    TOK822 **addr_list;
    TOK822 **tpp;
    char   *ratsign;
    int     extlen;

    if (extension)
	extlen = strlen(extension);

#define STR(x) vstring_str(x)

    /*
     * Parse the string, rewrite each address to canonical form, and convert
     * the result to external (quoted) form. Optionally apply the extension
     * to each address found.
     */
    tree = tok822_parse(string);
    addr_list = tok822_grep(tree, TOK822_ADDR);
    for (tpp = addr_list; *tpp; tpp++) {
	tok822_externalize(extern_addr, tpp[0]->head, TOK822_STR_DEFL);
	canon_addr_external(canon_addr, STR(extern_addr));
	if (extension) {
	    if ((ratsign = strrchr(STR(canon_addr), '@')) == 0) {
		vstring_strcat(canon_addr, extension);
	    } else {
		VSTRING_SPACE(canon_addr, extlen + 1);
		ratsign = strrchr(STR(canon_addr), '@');
		memmove(ratsign + extlen, ratsign, strlen(ratsign) + 1);
		memcpy(ratsign, extension, extlen);
		VSTRING_SKIP(canon_addr);
	    }
	}
	argv_add(argv, STR(canon_addr), ARGV_END);
    }
    argv_terminate(argv);
    myfree((char *) addr_list);
    tok822_free_tree(tree);
    vstring_free(canon_addr);
    vstring_free(extern_addr);
    return (argv);
}

#ifdef TEST

 /*
  * Stand-alone test program, sort of interactive.
  */
#include <unistd.h>
#include <msg.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <mail_conf.h>
#include <mail_params.h>

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *extension = vstring_alloc(1);
    VSTRING *buf = vstring_alloc(1);
    ARGV   *argv;
    char  **cpp;

    mail_conf_read();
    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir %s: %m", var_queue_dir);

    vstream_printf("extension: (CR for none): ");
    vstream_fflush(VSTREAM_OUT);
    if (vstring_get_nonl(extension, VSTREAM_IN) == VSTREAM_EOF)
	exit(0);

    vstream_printf("print strings to be translated, one per line\n");
    vstream_fflush(VSTREAM_OUT);
    while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	argv = mail_addr_crunch(STR(buf), VSTRING_LEN(extension) ? STR(extension) : 0);
	for (cpp = argv->argv; *cpp; cpp++)
	    vstream_printf("	%s\n", *cpp);
	vstream_fflush(VSTREAM_OUT);
    }
}

#endif
