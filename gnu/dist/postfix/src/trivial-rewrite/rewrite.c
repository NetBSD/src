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
/*	void	rewrite_addr(rule, addr, result)
/*	char	*rule;
/*	char	*addr;
/*	VSTRING *result;
/*
/*	void	rewrite_tree(rule, tree)
/*	char	*rule;
/*	TOK822	*tree;
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

static VSTRING *ruleset;
static VSTRING *address;
static VSTRING *result;

/* rewrite_tree - rewrite address according to rule set */

void    rewrite_tree(char *unused_ruleset, TOK822 *tree)
{
    TOK822 *colon;
    TOK822 *domain;
    TOK822 *bang;
    TOK822 *local;

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
	else if (var_append_at_myorigin != 0) {
	    domain = tok822_sub_append(tree, tok822_alloc('@', (char *) 0));
	    tok822_sub_append(tree, tok822_scan(var_myorigin, (TOK822 **) 0));
	}
    }

    /*
     * Append missing .domain
     */
    if (var_append_dot_mydomain != 0
	&& (domain = tok822_rfind_type(tree->tail, '@')) != 0
	&& tok822_find_type(domain, TOK822_DOMLIT) == 0
	&& tok822_find_type(domain, '.') == 0) {
	tok822_sub_append(tree, tok822_alloc('.', (char *) 0));
	tok822_sub_append(tree, tok822_scan(var_mydomain, (TOK822 **) 0));
    }

    /*
     * Strip trailing dot.
     */
    if (tree->tail->type == '.')
	tok822_free_tree(tok822_sub_keep_before(tree, tree->tail));
}

/* rewrite_addr - rewrite address according to rule set */

void    rewrite_addr(char *ruleset, char *addr, VSTRING *result)
{
    TOK822 *tree;

    /*
     * Sanity check. An address is supposed to be in externalized form.
     */
    if (*addr == 0) {
	msg_warn("rewrite_addr: null address, ruleset \"%s\"", ruleset);
	vstring_strcpy(result, addr);
	return;
    }

    /*
     * Convert the address from externalized (quoted) form to token list,
     * rewrite it, and convert back.
     */
    tree = tok822_scan_addr(addr);
    rewrite_tree(ruleset, tree);
    tok822_externalize(result, tree, TOK822_STR_DEFL);
    tok822_free_tree(tree);
}

/* rewrite_proto - read request and send reply */

int     rewrite_proto(VSTREAM *stream)
{
    if (attr_scan(stream, ATTR_FLAG_STRICT,
		  ATTR_TYPE_STR, MAIL_ATTR_RULE, ruleset,
		  ATTR_TYPE_STR, MAIL_ATTR_ADDR, address,
		  ATTR_TYPE_END) != 2)
	return (-1);

    rewrite_addr(vstring_str(ruleset), vstring_str(address), result);

    if (msg_verbose)
	msg_info("`%s' `%s' -> `%s'", vstring_str(ruleset),
		 vstring_str(address), vstring_str(result));

    attr_print(stream, ATTR_FLAG_NONE,
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
