/*	$NetBSD: cleanup_map1n.c,v 1.3.6.1 2023/12/25 12:43:31 martin Exp $	*/

/*++
/* NAME
/*	cleanup_map1n 3
/* SUMMARY
/*	one-to-many address mapping
/* SYNOPSIS
/*	#include <cleanup.h>
/*
/*	ARGV	*cleanup_map1n_internal(state, addr, maps, propagate)
/*	CLEANUP_STATE *state;
/*	const char *addr;
/*	MAPS	*maps;
/*	int	propagate;
/* DESCRIPTION
/*	This module implements one-to-many table mapping via table lookup.
/*	Table lookups are done with quoted (externalized) address forms.
/*	The process is recursive. The recursion terminates when the
/*	left-hand side appears in its own expansion.
/*
/*	cleanup_map1n_internal() is the interface for addresses in
/*	internal (unquoted) form.
/* DIAGNOSTICS
/*	When the maximal expansion or recursion limit is reached,
/*	the alias is not expanded and the CLEANUP_STAT_DEFER error
/*	is raised with reason "4.6.0 Alias expansion error".
/*
/*	When table lookup fails, the alias is not expanded and the
/*	CLEANUP_STAT_WRITE error is raised with reason "4.6.0 Alias
/*	expansion error".
/* SEE ALSO
/*	mail_addr_map(3) address mappings
/*	mail_addr_find(3) address lookups
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
#include <msg.h>
#include <argv.h>
#include <vstring.h>
#include <dict.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>
#include <mail_addr_map.h>
#include <cleanup_user.h>
#include <quote_822_local.h>
#include <been_here.h>

/* Application-specific. */

#include "cleanup.h"

/* cleanup_map1n_internal - one-to-many table lookups */

ARGV   *cleanup_map1n_internal(CLEANUP_STATE *state, const char *addr,
			               MAPS *maps, int propagate)
{
    ARGV   *argv;
    ARGV   *lookup;
    int     count;
    int     i;
    int     arg;
    BH_TABLE *been_here;
    char   *saved_lhs;

    /*
     * Initialize.
     */
    argv = argv_alloc(1);
    argv_add(argv, addr, ARGV_END);
    argv_terminate(argv);
    been_here = been_here_init(0, BH_FLAG_FOLD);

    /*
     * Rewrite the address vector in place. With each map lookup result,
     * split it into separate addresses, then rewrite and flatten each
     * address, and repeat the process. Beware: argv is being changed, so we
     * must index the array explicitly, instead of running along it with a
     * pointer.
     */
#define UPDATE(ptr,new)	do { \
	if (ptr) { myfree(ptr); } ptr = mystrdup(new); \
    } while (0)
#define STR	vstring_str
#define RETURN(x) do { \
	been_here_free(been_here); return (x); \
    } while (0)
#define UNEXPAND(argv, addr) do { \
	argv_truncate((argv), 0); argv_add((argv), (addr), (char *) 0); \
    } while (0)

    for (arg = 0; arg < argv->argc; arg++) {
	if (argv->argc > var_virt_expan_limit) {
	    msg_warn("%s: unreasonable %s map expansion size for %s -- "
		     "message not accepted, try again later",
		     state->queue_id, maps->title, addr);
	    state->errs |= CLEANUP_STAT_DEFER;
	    UPDATE(state->reason, "4.6.0 Alias expansion error");
	    UNEXPAND(argv, addr);
	    RETURN(argv);
	}
	for (count = 0; /* void */ ; count++) {

	    /*
	     * Don't expand an address that already expanded into itself.
	     */
	    if (been_here_check_fixed(been_here, argv->argv[arg]) != 0)
		break;
	    if (count >= var_virt_recur_limit) {
		msg_warn("%s: unreasonable %s map nesting for %s -- "
			 "message not accepted, try again later",
			 state->queue_id, maps->title, addr);
		state->errs |= CLEANUP_STAT_DEFER;
		UPDATE(state->reason, "4.6.0 Alias expansion error");
		UNEXPAND(argv, addr);
		RETURN(argv);
	    }
	    if ((lookup = mail_addr_map_internal(maps, argv->argv[arg],
						 propagate)) != 0) {
		saved_lhs = mystrdup(argv->argv[arg]);
		for (i = 0; i < lookup->argc; i++) {
		    if (strlen(lookup->argv[i]) > var_virt_addrlen_limit) {
			msg_warn("%s: unreasonable %s result %.300s... -- "
				 "message not accepted, try again later",
			     state->queue_id, maps->title, lookup->argv[i]);
			state->errs |= CLEANUP_STAT_DEFER;
			UPDATE(state->reason, "4.6.0 Alias expansion error");
			UNEXPAND(argv, addr);
			RETURN(argv);
		    }
		    if (i == 0) {
			UPDATE(argv->argv[arg], lookup->argv[i]);
		    } else {
			argv_add(argv, lookup->argv[i], ARGV_END);
			argv_terminate(argv);
		    }

		    /*
		     * Allow an address to expand into itself once.
		     */
		    if (strcasecmp_utf8(saved_lhs, lookup->argv[i]) == 0)
			been_here_fixed(been_here, saved_lhs);
		}
		myfree(saved_lhs);
		argv_free(lookup);
	    } else if (maps->error != 0) {
		msg_warn("%s: %s map lookup problem for %s -- "
			 "message not accepted, try again later",
			 state->queue_id, maps->title, addr);
		state->errs |= CLEANUP_STAT_WRITE;
		UPDATE(state->reason, "4.6.0 Alias expansion error");
		UNEXPAND(argv, addr);
		RETURN(argv);
	    } else {
		break;
	    }
	}
    }
    RETURN(argv);
}
