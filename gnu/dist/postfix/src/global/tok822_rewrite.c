/*++
/* NAME
/*	tok822_rewrite 3
/* SUMMARY
/*	address rewriting, client interface
/* SYNOPSIS
/*	#include <tok822.h>
/*
/*	TOK822	*tok822_rewrite(addr, how)
/*	TOK822	*addr;
/*	char	*how;
/* DESCRIPTION
/*	tok822_rewrite() takes an address token tree and transforms
/*	it according to the rule set specified via \fIhow\fR. The
/*	result is the \fIaddr\fR argument.
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

/* Utility library. */

#include <vstring.h>
#include <msg.h>

/* Global library. */

#include "rewrite_clnt.h"
#include "tok822.h"

/* tok822_rewrite - address rewriting interface */

TOK822 *tok822_rewrite(TOK822 *addr, const char *how)
{
    VSTRING *input_ext_form = vstring_alloc(100);
    VSTRING *canon_ext_form = vstring_alloc(100);

    if (addr->type != TOK822_ADDR)
	msg_panic("tok822_rewrite: non-address token type: %d", addr->type);

    /*
     * Externalize the token tree, ship it to the rewrite service, and parse
     * the result. Shipping external form is much simpler than shipping parse
     * trees.
     */
    tok822_externalize(input_ext_form, addr->head, TOK822_STR_DEFL);
    if (msg_verbose)
	msg_info("tok822_rewrite: input: %s", vstring_str(input_ext_form));
    rewrite_clnt(how, vstring_str(input_ext_form), canon_ext_form);
    if (msg_verbose)
	msg_info("tok822_rewrite: result: %s", vstring_str(canon_ext_form));
    tok822_free_tree(addr->head);
    addr->head = tok822_scan(vstring_str(canon_ext_form), &addr->tail);

    vstring_free(input_ext_form);
    vstring_free(canon_ext_form);
    return (addr);
}
