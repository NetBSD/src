/*	$NetBSD: master_watch.c,v 1.1.1.1.2.2 2009/09/15 06:02:58 snj Exp $	*/

/*++
/* NAME
/*	master_watch 3
/* SUMMARY
/*	Postfix master - monitor main.cf changes
/* SYNOPSIS
/*	#include "master.h"
/*
/*	void	master_str_watch(str_watch_table)
/*	const MASTER_STR_WATCH *str_watch_table;
/*
/*	void	master_int_watch(int_watch_table)
/*	MASTER_INT_WATCH *int_watch_table;
/* DESCRIPTION
/*	The Postfix master daemon is a long-running process. After
/*	main.cf is changed, some parameter changes may require that
/*	master data structures be recomputed.
/*
/*	Unfortunately, some main.cf changes cannot be applied
/*	on-the-fly, either because they require killing off existing
/*	child processes and thus disrupt service, or because the
/*	necessary support for on-the-fly data structure update has
/*	not yet been implemented.  Such main.cf changes trigger a
/*	warning that they require that Postfix be stopped and
/*	restarted.
/*
/*	This module provides functions that monitor selected main.cf
/*	parameters for change. The operation of these functions is
/*	controlled by tables that specify the parameter name, the
/*	current parameter value, a historical parameter value,
/*	optional flags, and an optional notify call-back function.
/*
/*	master_str_watch() monitors string-valued parameters for
/*	change, and master_int_watch() does the same for integer-valued
/*	parameters. Note that master_int_watch() needs read-write
/*	access to its argument table, while master_str_watch() needs
/*	read-only access only.
/*
/*	The functions log a warning when a parameter value has
/*	changed after re-reading main.cf, but the parameter is not
/*	flagged in the MASTER_*_WATCH table as "updatable" with
/*	MASTER_WATCH_FLAG_UPDATABLE.
/*
/*	If the parameter has a notify call-back function, then the
/*	function is called after main.cf is read for the first time.
/*	If the parameter is flagged as "updatable", then the function
/*	is also called when the parameter value changes after
/*	re-reading main.cf.
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
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>

/* Application-specific. */

#include "master.h"

/* master_str_watch - watch string-valued parameters for change */

void    master_str_watch(const MASTER_STR_WATCH *str_watch_table)
{
    const MASTER_STR_WATCH *wp;

    for (wp = str_watch_table; wp->name != 0; wp++) {

	/*
	 * Detect changes to monitored parameter values. If a change is
	 * supported, we discard the backed up value and update it to the
	 * current value later. Otherwise we complain.
	 */
	if (wp->backup[0] != 0
	    && strcmp(wp->backup[0], wp->value[0]) != 0) {
	    if ((wp->flags & MASTER_WATCH_FLAG_UPDATABLE) == 0) {
		msg_warn("ignoring %s parameter value change", wp->name);
		msg_warn("old value: \"%s\", new value: \"%s\"",
			 wp->backup[0], wp->value[0]);
		msg_warn("to change %s, stop and start Postfix", wp->name);
	    } else {
		myfree(wp->backup[0]);
		wp->backup[0] = 0;
	    }
	}

	/*
	 * Initialize the backed up parameter value, or update it if this
	 * parameter supports updates after initialization. Optionally 
	 * notify the application that this parameter has changed.
	 */
	if (wp->backup[0] == 0) {
	    if (wp->notify != 0)
		wp->notify();
	    wp->backup[0] = mystrdup(wp->value[0]);
	}
    }
}

/* master_int_watch - watch integer-valued parameters for change */

void    master_int_watch(MASTER_INT_WATCH *int_watch_table)
{
    MASTER_INT_WATCH *wp;

    for (wp = int_watch_table; wp->name != 0; wp++) {

	/*
	 * Detect changes to monitored parameter values. If a change is
	 * supported, we discard the backed up value and update it to the
	 * current value later. Otherwise we complain.
	 */
	if ((wp->flags & MASTER_WATCH_FLAG_ISSET) != 0
	    && wp->backup != wp->value[0]) {
	    if ((wp->flags & MASTER_WATCH_FLAG_UPDATABLE) == 0) {
		msg_warn("ignoring %s parameter value change", wp->name);
		msg_warn("old value: \"%d\", new value: \"%d\"",
			 wp->backup, wp->value[0]);
		msg_warn("to change %s, stop and start Postfix", wp->name);
	    } else {
		wp->flags &= ~MASTER_WATCH_FLAG_ISSET;
	    }
	}

	/*
	 * Initialize the backed up parameter value, or update if it this
	 * parameter supports updates after initialization. Optionally 
	 * notify the application that this parameter has changed.
	 */
	if ((wp->flags & MASTER_WATCH_FLAG_ISSET) == 0) {
	    if (wp->notify != 0)
		wp->notify();
	    wp->flags |= MASTER_WATCH_FLAG_ISSET;
	    wp->backup = wp->value[0];
	}
    }
}
