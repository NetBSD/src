/*++
/* NAME
/*	cleanup_rewrite 3
/* SUMMARY
/*	address canonicalization
/* SYNOPSIS
/*	#include <cleanup.h>
/*
/*	int	cleanup_rewrite_external(context_name, result, addr)
/*	const char *context;
/*	VSTRING	*result;
/*	const char *addr;
/*
/*	int	cleanup_rewrite_internal(context_name, result, addr)
/*	const char *context;
/*	VSTRING	*result;
/*	const char *addr;
/*
/*	int	cleanup_rewrite_tree(context_name, tree)
/*	const char *context;
/*	TOK822	*tree;
/* DESCRIPTION
/*	This module rewrites addresses to canonical form, adding missing
/*	domains and stripping source routes etc., and performs
/*	\fIcanonical\fR map lookups to map addresses to official form.
/*	These functions return non-zero when the address was changed.
/*
/*	cleanup_rewrite_init() performs one-time initialization.
/*
/*	cleanup_rewrite_external() rewrites the external (quoted) string
/*	form of an address.
/*
/*	cleanup_rewrite_internal() is a wrapper around the
/*	cleanup_rewrite_external() routine that transforms from
/*	internal (quoted) string form to external form and back.
/*
/*	cleanup_rewrite_tree() is a wrapper around the
/*	cleanup_rewrite_external() routine that transforms from
/*	internal parse tree form to external form and back.
/*
/*	Arguments:
/* .IP context_name
/*	The name of an address rewriting context that supplies
/*	the equivalents of myorigin and mydomain.
/* .IP result
/*	Result buffer.
/* .IP addr
/*	Input buffer.
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

/* Utility library. */

#include <msg.h>
#include <vstring.h>

/* Global library. */

#include <tok822.h>
#include <rewrite_clnt.h>
#include <quote_822_local.h>

/* Application-specific. */

#include "cleanup.h"

#define STR		vstring_str

/* cleanup_rewrite_external - rewrite address external form */

int     cleanup_rewrite_external(const char *context_name, VSTRING *result,
				         const char *addr)
{
    rewrite_clnt(context_name, addr, result);
    return (strcmp(STR(result), addr) != 0);
}

/* cleanup_rewrite_tree - rewrite address node */

int    cleanup_rewrite_tree(const char *context_name, TOK822 *tree)
{
    VSTRING *dst = vstring_alloc(100);
    VSTRING *src = vstring_alloc(100);
    int     did_rewrite;

    tok822_externalize(src, tree->head, TOK822_STR_DEFL);
    did_rewrite = cleanup_rewrite_external(context_name, dst, STR(src));
    tok822_free_tree(tree->head);
    tree->head = tok822_scan(STR(dst), &tree->tail);
    vstring_free(dst);
    vstring_free(src);
    return (did_rewrite);
}

/* cleanup_rewrite_internal - rewrite address internal form */

int     cleanup_rewrite_internal(const char *context_name,
				         VSTRING *result, const char *addr)
{
    VSTRING *dst = vstring_alloc(100);
    VSTRING *src = vstring_alloc(100);
    int     did_rewrite;

    quote_822_local(src, addr);
    did_rewrite = cleanup_rewrite_external(context_name, dst, STR(src));
    unquote_822_local(result, STR(dst));
    vstring_free(dst);
    vstring_free(src);
    return (did_rewrite);
}
