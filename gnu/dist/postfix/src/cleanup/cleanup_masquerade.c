/*++
/* NAME
/*	cleanup_masquerade 3
/* SUMMARY
/*	address masquerading
/* SYNOPSIS
/*	#include <cleanup.h>
/*
/*	void	cleanup_masquerade_external(addr, masq_domains)
/*	VSTRING	*addr;
/*	ARGV	*masq_domains;
/*
/*	void	cleanup_masquerade_internal(addr, masq_domains)
/*	VSTRING	*addr;
/*	ARGV	*masq_domains;
/*
/*	void	cleanup_masquerade_tree(tree, masq_domains)
/*	TOK822	*tree;
/*	ARGV	*masq_domains;
/* DESCRIPTION
/*	This module masquerades addresses, that is, it strips subdomains
/*	below domain names that are listed in the masquerade_domains
/*	configuration parameter, except for user names listed in the
/*	masquerade_exceptions configuration parameter.
/*
/*	cleanup_masquerade_external() rewrites the external (quoted) string
/*	form of an address.
/*
/*	cleanup_masquerade_internal() is a wrapper around the
/*	cleanup_masquerade_external() routine that transforms from
/*	internal (quoted) string form to external form and back.
/*
/*	cleanup_masquerade_tree() is a wrapper around the
/*	cleanup_masquerade_external() routine that transforms from
/*	internal parse tree form to external form and back.
/* DIAGNOSTICS
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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <argv.h>
#include <htable.h>
#include <mymalloc.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>
#include <tok822.h>
#include <quote_822_local.h>

/* Application-specific. */

#include "cleanup.h"

#define STR	vstring_str

/* cleanup_masquerade_external - masquerade address external form */

void    cleanup_masquerade_external(VSTRING *addr, ARGV *masq_domains)
{
    char   *domain;
    int     domain_len;
    char  **masqp;
    int     masq_len;
    char   *parent;

    /* Stuff for excluded names. */
    static HTABLE *masq_except_table = 0;
    char   *saved_names;
    char   *name;
    char   *ptr;
    int     excluded;

    /*
     * First time, build a lookup table for excluded names.
     */
    if (*var_masq_exceptions && masq_except_table == 0) {
	masq_except_table = htable_create(5);
	ptr = saved_names = mystrdup(var_masq_exceptions);
	while ((name = mystrtok(&ptr, ", \t\r\n")) != 0)
	    htable_enter(masq_except_table, lowercase(name), (char *) 0);
	myfree(saved_names);
    }

    /*
     * Find the domain part.
     */
    if ((domain = strrchr(STR(addr), '@')) == 0)
	return;
    domain += 1;
    domain_len = strlen(domain);

    /*
     * Don't masquerade excluded names (regardless of domain).
     */
    if (masq_except_table) {
	name = mystrndup(STR(addr), domain - 1 - STR(addr));
	excluded = (htable_locate(masq_except_table, lowercase(name)) != 0);
	myfree(name);
	if (excluded)
	    return;
    }

    /*
     * If any parent domain matches the list of masquerade domains, replace
     * the domain in the address and terminate. If the domain matches a
     * masquerade domain, leave it alone. Order of specification matters.
     */
    for (masqp = masq_domains->argv; *masqp; masqp++) {
	masq_len = strlen(*masqp);
	if (masq_len == domain_len) {
	    if (strcasecmp(*masqp, domain) == 0)
		break;
	} else if (masq_len < domain_len) {
	    parent = domain + domain_len - masq_len;
	    if (parent[-1] == '.' && strcasecmp(*masqp, parent) == 0) {
		if (msg_verbose)
		    msg_info("masquerade: %s -> %s", domain, *masqp);
		vstring_truncate(addr, domain - STR(addr));
		vstring_strcat(addr, *masqp);
		break;
	    }
	}
    }
}

/* cleanup_masquerade_tree - masquerade address node */

void    cleanup_masquerade_tree(TOK822 *tree, ARGV *masq_domains)
{
    VSTRING *temp = vstring_alloc(100);

    tok822_externalize(temp, tree->head, TOK822_STR_DEFL);
    cleanup_masquerade_external(temp, masq_domains);
    tok822_free_tree(tree->head);
    tree->head = tok822_scan(STR(temp), &tree->tail);

    vstring_free(temp);
}

/* cleanup_masquerade_internal - masquerade address internal form */

void    cleanup_masquerade_internal(VSTRING *addr, ARGV *masq_domains)
{
    VSTRING *temp = vstring_alloc(100);

    quote_822_local(temp, STR(addr));
    cleanup_masquerade_external(temp, masq_domains);
    unquote_822_local(addr, STR(temp));

    vstring_free(temp);
}
