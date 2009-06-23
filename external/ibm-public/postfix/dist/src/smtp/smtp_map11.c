/*	$NetBSD: smtp_map11.c,v 1.1.1.1 2009/06/23 10:08:54 tron Exp $	*/

/*++
/* NAME
/*	smtp_map11 3
/* SUMMARY
/*	one-to-one address mapping
/* SYNOPSIS
/*	#include <smtp.h>
/*
/*	int	smtp_map11_external(addr, maps, propagate)
/*	VSTRING	*addr;
/*	MAPS	*maps;
/*	int	propagate;
/*
/*	int	smtp_map11_internal(addr, maps, propagate)
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
/*	smtp_map11_external() looks up the RFC 822 external (quoted) string
/*	form of an address in the maps specified via the \fImaps\fR argument.
/*
/*	smtp_map11_internal() is a wrapper around the
/*	smtp_map11_external() routine that transforms from
/*	internal (quoted) string form to external form and back.
/*
/*	smtp_map11_tree() is a wrapper around the
/*	smtp_map11_external() routine that transforms from
/*	internal parse tree form to external form and back.
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
#include <valid_hostname.h>

/* Global library. */

#include <mail_addr_map.h>
#include <quote_822_local.h>

/* Application-specific. */

#include <smtp.h>

/* smtp_map11_external - one-to-one table lookups */

int     smtp_map11_external(VSTRING *addr, MAPS *maps, int propagate)
{
    const char *myname = "smtp_map11_external";
    ARGV   *new_addr;
    const char *result;

    if ((new_addr = mail_addr_map(maps, STR(addr), propagate)) != 0) {
	if (new_addr->argc > 1)
	    msg_warn("multi-valued %s result for %s", maps->title, STR(addr));
	result = new_addr->argv[0];
	if (msg_verbose)
	    msg_info("%s: %s -> %s", myname, STR(addr), result);
	vstring_strcpy(addr, result);
	argv_free(new_addr);
	return (1);
    } else {
	if (dict_errno != 0)
	    msg_fatal("%s map lookup problem for %s", maps->title, STR(addr));
	if (msg_verbose)
	    msg_info("%s: %s not found", myname, STR(addr));
	return (0);
    }
}

/* smtp_map11_tree - rewrite address node */

int     smtp_map11_tree(TOK822 *tree, MAPS *maps, int propagate)
{
    VSTRING *temp = vstring_alloc(100);
    int     ret;

    tok822_externalize(temp, tree->head, TOK822_STR_DEFL);
    ret = smtp_map11_external(temp, maps, propagate);
    tok822_free_tree(tree->head);
    tree->head = tok822_scan(STR(temp), &tree->tail);
    vstring_free(temp);
    return (ret);
}

/* smtp_map11_internal - rewrite address internal form */

int     smtp_map11_internal(VSTRING *addr, MAPS *maps, int propagate)
{
    VSTRING *temp = vstring_alloc(100);
    int     ret;

    quote_822_local(temp, STR(addr));
    ret = smtp_map11_external(temp, maps, propagate);
    unquote_822_local(addr, STR(temp));
    vstring_free(temp);
    return (ret);
}

#ifdef TEST

#include <msg_vstream.h>
#include <stringops.h>
#include <mail_params.h>

int     main(int argc, char **argv)
{
    VSTRING *buf = vstring_alloc(100);
    MAPS   *maps;

    msg_vstream_init(basename(argv[0]), VSTREAM_ERR);
    if (argc < 3)
	msg_fatal("usage: %s maptype:mapname address...", argv[0]);

    maps = maps_create(argv[1], argv[1], DICT_FLAG_FOLD_FIX);
    mail_params_init();
    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir(%s): %m", var_queue_dir);
    argv += 1;

    msg_verbose = 1;
    while (--argc && *++argv) {
	msg_info("-- start %s --", *argv);
	smtp_map11_external(vstring_strcpy(buf, *argv), maps, 1);
	msg_info("-- end %s --", *argv);
    }
    vstring_free(buf);
    maps_free(maps);
    return (0);
}

#endif
