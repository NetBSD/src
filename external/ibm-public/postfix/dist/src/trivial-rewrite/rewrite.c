/*	$NetBSD: rewrite.c,v 1.1.1.1 2009/06/23 10:08:58 tron Exp $	*/

/*++
/* NAME
/*	rewrite 3
/* SUMMARY
/*	mail address rewriter
/* SYNOPSIS
/*	#include "trivial-rewrite.h"
/*
/*	void	rewrite_init(void)
/*
/*	void	rewrite_proto(stream)
/*	VSTREAM	*stream;
/*
/*	void	rewrite_addr(context, addr, result)
/*	RWR_CONTEXT *context;
/*	char	*addr;
/*	VSTRING *result;
/*
/*	void	rewrite_tree(context, tree)
/*	RWR_CONTEXT *context;
/*	TOK822	*tree;
/*
/*	RWR_CONTEXT local_context;
/*	RWR_CONTEXT remote_context;
/* DESCRIPTION
/*	This module implements the trivial address rewriting engine.
/*
/*	rewrite_init() initializes data structures that are private
/*	to this module. It should be called once before using the
/*	actual rewriting routines.
/*
/*	rewrite_proto() implements the client-server protocol: read
/*	one rule set name and one address in external (quoted) form,
/*	reply with the rewritten address in external form.
/*
/*	rewrite_addr() rewrites an address string to another string.
/*	Both input and output are in external (quoted) form.
/*
/*	rewrite_tree() rewrites a parse tree with a single address to
/*	another tree.  A tree is a dummy node on top of a token list.
/*
/*	local_context and remote_context provide domain names for
/*	completing incomplete address forms.
/* STANDARDS
/* DIAGNOSTICS
/*	Problems and transactions are logged to the syslog daemon.
/* BUGS
/* SEE ALSO
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
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <split_at.h>

/* Global library. */

#include <mail_params.h>
#include <mail_proto.h>
#include <resolve_local.h>
#include <tok822.h>
#include <mail_conf.h>

/* Application-specific. */

#include "trivial-rewrite.h"

RWR_CONTEXT local_context = {
    VAR_MYORIGIN, &var_myorigin,
    VAR_MYDOMAIN, &var_mydomain,
};

RWR_CONTEXT remote_context = {
    VAR_REM_RWR_DOMAIN, &var_remote_rwr_domain,
    VAR_REM_RWR_DOMAIN, &var_remote_rwr_domain,
};

static VSTRING *ruleset;
static VSTRING *address;
static VSTRING *result;

/* rewrite_tree - rewrite address according to rule set */

void    rewrite_tree(RWR_CONTEXT *context, TOK822 *tree)
{
    TOK822 *colon;
    TOK822 *domain;
    TOK822 *bang;
    TOK822 *local;

    /*
     * XXX If you change this module, quote_822_local.c, or tok822_parse.c,
     * be sure to re-run the tests under "make rewrite_clnt_test" and "make
     * resolve_clnt_test" in the global directory.
     */

    /*
     * Sanity check.
     */
    if (tree->head == 0)
	msg_panic("rewrite_tree: empty tree");

    /*
     * An empty address is a special case.
     */
    if (tree->head == tree->tail
	&& tree->tail->type == TOK822_QSTRING
	&& VSTRING_LEN(tree->tail->vstr) == 0)
	return;

    /*
     * Treat a lone @ as if it were an empty address.
     */
    if (tree->head == tree->tail
	&& tree->tail->type == '@') {
	tok822_free_tree(tok822_sub_keep_before(tree, tree->tail));
	tok822_sub_append(tree, tok822_alloc(TOK822_QSTRING, ""));
	return;
    }

    /*
     * Strip source route.
     */
    if (tree->head->type == '@'
	&& (colon = tok822_find_type(tree->head, ':')) != 0
	&& colon != tree->tail)
	tok822_free_tree(tok822_sub_keep_after(tree, colon));

    /*
     * Optionally, transform address forms without @.
     */
    if ((domain = tok822_rfind_type(tree->tail, '@')) == 0) {

	/*
	 * Swap domain!user to user@domain.
	 */
	if (var_swap_bangpath != 0
	    && (bang = tok822_find_type(tree->head, '!')) != 0) {
	    tok822_sub_keep_before(tree, bang);
	    local = tok822_cut_after(bang);
	    tok822_free(bang);
	    tok822_sub_prepend(tree, tok822_alloc('@', (char *) 0));
	    if (local)
		tok822_sub_prepend(tree, local);
	}

	/*
	 * Promote user%domain to user@domain.
	 */
	else if (var_percent_hack != 0
		 && (domain = tok822_rfind_type(tree->tail, '%')) != 0) {
	    domain->type = '@';
	}

	/*
	 * Append missing @origin
	 */
	else if (var_append_at_myorigin != 0
		 && REW_PARAM_VALUE(context->origin) != 0
		 && REW_PARAM_VALUE(context->origin)[0] != 0) {
	    domain = tok822_sub_append(tree, tok822_alloc('@', (char *) 0));
	    tok822_sub_append(tree, tok822_scan(REW_PARAM_VALUE(context->origin),
						(TOK822 **) 0));
	}
    }

    /*
     * Append missing .domain, but leave broken forms ending in @ alone. This
     * merely makes diagnostics more accurate by leaving bogus addresses
     * alone.
     */
    if (var_append_dot_mydomain != 0
	&& REW_PARAM_VALUE(context->domain) != 0
	&& REW_PARAM_VALUE(context->domain)[0] != 0
	&& (domain = tok822_rfind_type(tree->tail, '@')) != 0
	&& domain != tree->tail
	&& tok822_find_type(domain, TOK822_DOMLIT) == 0
	&& tok822_find_type(domain, '.') == 0) {
	tok822_sub_append(tree, tok822_alloc('.', (char *) 0));
	tok822_sub_append(tree, tok822_scan(REW_PARAM_VALUE(context->domain),
					    (TOK822 **) 0));
    }

    /*
     * Strip trailing dot at end of domain, but not dot-dot or @-dot. This
     * merely makes diagnostics more accurate by leaving bogus addresses
     * alone.
     */
    if (tree->tail->type == '.'
	&& tree->tail->prev
	&& tree->tail->prev->type != '.'
	&& tree->tail->prev->type != '@')
	tok822_free_tree(tok822_sub_keep_before(tree, tree->tail));
}

/* rewrite_proto - read request and send reply */

int     rewrite_proto(VSTREAM *stream)
{
    RWR_CONTEXT *context;
    TOK822 *tree;

    if (attr_scan(stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_RULE, ruleset,
		  ATTR_TYPE_STR, MAIL_ATTR_ADDR, address,
		  ATTR_TYPE_END) != 2)
	return (-1);

    if (strcmp(vstring_str(ruleset), MAIL_ATTR_RWR_LOCAL) == 0)
	context = &local_context;
    else if (strcmp(vstring_str(ruleset), MAIL_ATTR_RWR_REMOTE) == 0)
	context = &remote_context;
    else {
	msg_warn("unknown context: %s", vstring_str(ruleset));
	return (-1);
    }

    /*
     * Sanity check. An address is supposed to be in externalized form.
     */
    if (*vstring_str(address) == 0) {
	msg_warn("rewrite_addr: null address");
	vstring_strcpy(result, vstring_str(address));
    }

    /*
     * Convert the address from externalized (quoted) form to token list,
     * rewrite it, and convert back.
     */
    else {
	tree = tok822_scan_addr(vstring_str(address));
	rewrite_tree(context, tree);
	tok822_externalize(result, tree, TOK822_STR_DEFL);
	tok822_free_tree(tree);
    }
    if (msg_verbose)
	msg_info("`%s' `%s' -> `%s'", vstring_str(ruleset),
		 vstring_str(address), vstring_str(result));

    attr_print(stream, ATTR_FLAG_NONE,
	       ATTR_TYPE_INT, MAIL_ATTR_FLAGS, server_flags,
	       ATTR_TYPE_STR, MAIL_ATTR_ADDR, vstring_str(result),
	       ATTR_TYPE_END);

    if (vstream_fflush(stream) != 0) {
	msg_warn("write rewrite reply: %m");
	return (-1);
    }
    return (0);
}

/* rewrite_init - module initializations */

void    rewrite_init(void)
{
    ruleset = vstring_alloc(100);
    address = vstring_alloc(100);
    result = vstring_alloc(100);
}
