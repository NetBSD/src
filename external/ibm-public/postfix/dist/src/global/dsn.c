/*	$NetBSD: dsn.c,v 1.1.1.1.2.2 2009/09/15 06:02:41 snj Exp $	*/

/*++
/* NAME
/*	dsn
/* SUMMARY
/*	RFC-compliant delivery status information
/* SYNOPSIS
/*	#include <dsn.h>
/*
/*	typedef struct {
/* .in +4
/*		const char *status;	/* RFC 3463 status */
/*		const char *action;	/* null or RFC 3464 action */
/*		const char *reason;	/* human-readable text */
/*		const char *dtype;	/* null or diagnostic type */
/*		const char *dtext;	/* null or diagnostic code */
/*		const char *mtype;	/* null or MTA type */
/*		const char *mname;	/* null or remote MTA */
/* .in -4
/*	} DSN;
/*
/*	DSN	*create(status, action, reason, dtype, dtext, mtype, mname)
/*	const char *status;
/*	const char *action;
/*	const char *reason;
/*	const char *dtype;
/*	const char *dtext;
/*	const char *mtype;
/*	const char *mname;
/*
/*	DSN	*DSN_COPY(dsn)
/*	DSN	*dsn;
/*
/*	void	dsn_free(dsn)
/*	DSN	*dsn;
/*
/*	DSN	*DSN_ASSIGN(dsn, status, action, reason, dtype, dtext, 
/*						mtype, mname)
/*	DSN	*dsn;
/*	const char *status;
/*	const char *action;
/*	const char *reason;
/*	const char *dtype;
/*	const char *dtext;
/*	const char *mtype;
/*	const char *mname;
/*
/*	DSN	*DSN_SIMPLE(dsn, status, action, reason)
/*	DSN	*dsn;
/*	const char *status;
/*	const char *action;
/*	const char *reason;
/* DESCRIPTION
/*	This module maintains delivery error information. For a
/*	description of structure field members see "Arguments"
/*	below.  Function-like names spelled in upper case are macros.
/*	These may evaluate some arguments more than once.
/*
/*	dsn_create() creates a DSN structure and copies its arguments.
/*	The DSN structure should be destroyed with dsn_free().
/*
/*	DSN_COPY() creates a deep copy of its argument.
/*
/*	dsn_free() destroys a DSN structure and makes its storage
/*	available for reuse.
/*
/*	DSN_ASSIGN() updates a DSN structure and DOES NOT copy
/*	arguments or free memory.  The result DSN structure must
/*	NOT be passed to dsn_free(). DSN_ASSIGN() is typically used
/*	for stack-based short-lived storage.
/*
/*	DSN_SIMPLE() takes the minimally required subset of all the
/*	attributes and sets the rest to empty strings.
/*	This is a wrapper around the DSN_ASSIGN() macro.
/*
/*	Arguments:
/* .IP reason
/*	Human-readable text, used for logging purposes, and for
/*	updating the message-specific \fBbounce\fR or \fIdefer\fR
/*	logfile.
/* .IP status
/*	Enhanced status code as specified in RFC 3463.
/* .IP action
/*	DSN_NO_ACTION, empty string, or action as defined in RFC 3464.
/*	If no action is specified, a default action is chosen.
/* .IP dtype
/*	DSN_NO_DTYPE, empty string, or diagnostic code type as
/*	specified in RFC 3464.
/* .IP dtext
/*	DSN_NO_DTEXT, empty string, or diagnostic code as specified
/*	in RFC 3464.
/* .IP mtype
/*	DSN_NO_MTYPE, empty string, DSN_MTYPE_DNS or DSN_MTYPE_UNIX.
/* .IP mname
/*	DSN_NO_MNAME, empty string, or remote MTA as specified in
/*	RFC 3464.
/* DIAGNOSTICS
/*	Panic: null or empty status or reason.
/*	Fatal: out of memory.
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
#include <mymalloc.h>

/* Global library. */

#include <dsn.h>

/* dsn_create - create DSN structure */

DSN    *dsn_create(const char *status, const char *action, const char *reason,
		           const char *dtype, const char *dtext,
		           const char *mtype, const char *mname)
{
    const char *myname = "dsn_create";
    DSN    *dsn;

    dsn = (DSN *) mymalloc(sizeof(*dsn));

    /*
     * Status and reason must not be empty. Other members may be empty
     * strings.
     * 
     * Early implementations represented unavailable information with null
     * pointers. This resulted in code that was difficult to maintain. We now
     * use empty strings instead. For safety sake we keep the null pointer
     * test for input, but we always convert to empty string on output.
     */
#define NULL_OR_EMPTY(s) ((s) == 0 || *(s) == 0)

    if (NULL_OR_EMPTY(status))
	msg_panic("%s: null dsn status", myname);
    else
	dsn->status = mystrdup(status);

    if (NULL_OR_EMPTY(action))
	dsn->action = mystrdup("");
    else
	dsn->action = mystrdup(action);

    if (NULL_OR_EMPTY(reason))
	msg_panic("%s: null dsn reason", myname);
    else
	dsn->reason = mystrdup(reason);

    if (NULL_OR_EMPTY(dtype) || NULL_OR_EMPTY(dtext)) {
	dsn->dtype = mystrdup("");
	dsn->dtext = mystrdup("");
    } else {
	dsn->dtype = mystrdup(dtype);
	dsn->dtext = mystrdup(dtext);
    }
    if (NULL_OR_EMPTY(mtype) || NULL_OR_EMPTY(mname)) {
	dsn->mtype = mystrdup("");
	dsn->mname = mystrdup("");
    } else {
	dsn->mtype = mystrdup(mtype);
	dsn->mname = mystrdup(mname);
    }
    return (dsn);
}

/* dsn_free - destroy DSN structure */

void    dsn_free(DSN *dsn)
{
    myfree((char *) dsn->status);
    myfree((char *) dsn->action);
    myfree((char *) dsn->reason);
    myfree((char *) dsn->dtype);
    myfree((char *) dsn->dtext);
    myfree((char *) dsn->mtype);
    myfree((char *) dsn->mname);
    myfree((char *) dsn);
}
