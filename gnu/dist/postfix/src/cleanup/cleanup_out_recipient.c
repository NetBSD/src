/*++
/* NAME
/*	cleanup_out_recipient 3
/* SUMMARY
/*	envelope recipient output filter
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	void	cleanup_out_recipient(state, recipient)
/*	CLEANUP_STATE *state;
/*	char	*recipient;
/* DESCRIPTION
/*	This module implements an envelope recipient output filter.
/*
/*	cleanup_out_recipient() performs virtual table expansion
/*	and recipient duplicate filtering, and appends the
/*	resulting recipients to the output stream.
/* CONFIGURATION
/* .ad
/* .fi
/* .IP local_duplicate_filter_limit
/*	Upper bound to the size of the recipient duplicate filter.
/*	Zero means no limit; this may cause the mail system to
/*	become stuck.
/* .IP virtual_maps
/*	list of virtual address lookup tables.
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

#include <argv.h>

/* Global library. */

#include <been_here.h>
#include <mail_params.h>
#include <rec_type.h>
#include <ext_prop.h>

/* Application-specific. */

#include "cleanup.h"

/* cleanup_out_recipient - envelope recipient output filter */

void    cleanup_out_recipient(CLEANUP_STATE *state, char *recip)
{
    ARGV   *argv;
    char  **cpp;

    if (cleanup_virtual_maps == 0) {
	if (been_here_fixed(state->dups, recip) == 0)
	    cleanup_out_string(state, REC_TYPE_RCPT, recip), state->rcpt_count++;
    } else {
	argv = cleanup_map1n_internal(state, recip, cleanup_virtual_maps,
				  cleanup_ext_prop_mask & EXT_PROP_VIRTUAL);
	for (cpp = argv->argv; *cpp; cpp++)
	    if (been_here_fixed(state->dups, *cpp) == 0)
		cleanup_out_string(state, REC_TYPE_RCPT, *cpp), state->rcpt_count++;
	argv_free(argv);
    }
}
