/*++
/* NAME
/*	cleanup_map1n 3
/* SUMMARY
/*	one-to-many address mapping
/* SYNOPSIS
/*	#include <cleanup.h>
/*
/*	ARGV	*cleanup_map1n_internal(addr)
/*	char	*addr;
/* DESCRIPTION
/*	This module implements one-to-many table mapping via table lookup.
/*	The process is recursive. The recursion terminates when the
/*	left-hand side appears in its own expansion, or when a maximal
/*	nesting level is reached.
/*
/*	cleanup_map1n_internal() is the interface for addresses in
/*	internal (unquoted) form.
/* DIAGNOSTICS
/*	Recoverable errors: the global \fIcleanup_errs\fR flag is updated.
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
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <mymalloc.h>
#include <msg.h>
#include <argv.h>
#include <vstring.h>
#include <dict.h>

/* Global library. */

#include <mail_addr_map.h>
#include <cleanup_user.h>
#include <quote_822_local.h>
#include <been_here.h>

/* Application-specific. */

#include "cleanup.h"

/* cleanup_map1n_internal - one-to-many table lookups */

ARGV   *cleanup_map1n_internal(CLEANUP_STATE *state, char *addr,
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
#define UPDATE(ptr,new)	{ myfree(ptr); ptr = mystrdup(new); }
#define MAX_RECURSION 1000
#define MAX_EXPANSION 1000
#define STR	vstring_str
#define RETURN(x) { been_here_free(been_here); return (x); }

    for (arg = 0; arg < argv->argc; arg++) {
	if (argv->argc > MAX_EXPANSION) {
	    msg_warn("%s: unreasonable %s map expansion size for %s",
		     state->queue_id, maps->title, addr);
	    break;
	}
	for (count = 0; /* void */ ; count++) {
	    if (been_here_fixed(been_here, argv->argv[arg]) != 0)
		break;
	    if (count >= MAX_RECURSION) {
		msg_warn("%s: unreasonable %s map nesting for %s",
			 state->queue_id, maps->title, addr);
		break;
	    }
	    if ((lookup = mail_addr_map(maps, argv->argv[arg], propagate)) != 0) {
		saved_lhs = mystrdup(argv->argv[arg]);
		for (i = 0; i < lookup->argc; i++) {
		    unquote_822_local(state->temp1, lookup->argv[i]);
		    if (i == 0) {
			UPDATE(argv->argv[arg], STR(state->temp1));
		    } else {
			argv_add(argv, STR(state->temp1), ARGV_END);
			argv_terminate(argv);
		    }
		}
		myfree(saved_lhs);
		argv_free(lookup);
	    } else if (dict_errno != 0) {
		msg_warn("%s: %s map lookup problem for %s",
			 state->queue_id, maps->title, addr);
		state->errs |= CLEANUP_STAT_WRITE;
		RETURN(argv);
	    } else {
		break;
	    }
	}
    }
    RETURN(argv);
}
