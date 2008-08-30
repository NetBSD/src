/*++
/* NAME
/*	input_transp 3
/* SUMMARY
/*	receive transparency control
/* SYNOPSIS
/*	#include <input_transp.h>
/*
/*	int	input_transp_mask(param_name, pattern)
/*	const char *param_name;
/*	const char *pattern;
/*
/*	int	input_transp_cleanup(cleanup_flags, transp_mask)
/*	int	cleanup_flags;
/*	int	transp_mask;
/* DESCRIPTION
/*	This module controls how much processing happens before mail is
/*	written to the Postfix queue. Each transparency option is either
/*	implemented by a client of the cleanup service, or is passed
/*	along in a client request to the cleanup service. This eliminates
/*	the need to configure multiple cleanup service instances.
/*
/*	input_transp_mask() takes a comma-separated list of names and
/*	computes the corresponding mask. The following names are
/*	recognized in \fBpattern\fR, with the corresponding bit mask
/*	given in parentheses:
/* .IP "no_unknown_recipient_checks (INPUT_TRANSP_UNKNOWN_RCPT)"
/*	Do not try to reject unknown recipients.
/* .IP "no_address_mappings (INPUT_TRANSP_ADDRESS_MAPPING)"
/*	Disable canonical address mapping, virtual alias map expansion,
/*	address masquerading, and automatic BCC recipients.
/* .IP "no_header_body_checks (INPUT_TRANSP_HEADER_BODY)"
/*	Disable header/body_checks.
/* .IP "no_milters (INPUT_TRANSP_MILTER)"
/*	Disable Milter applications.
/*
/*	input_transp_cleanup() takes a bunch of cleanup processing
/*	flags and updates them according to the settings in the
/*	specified input transparency mask.
/* DIAGNOSTICS
/*	Panic: inappropriate use.
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

#include <name_mask.h>
#include <msg.h>

/* Global library. */

#include <mail_params.h>
#include <cleanup_user.h>
#include <input_transp.h>

/* input_transp_mask - compute mail receive transparency mask */

int     input_transp_mask(const char *param_name, const char *pattern)
{
    static const NAME_MASK table[] = {
	"no_unknown_recipient_checks", INPUT_TRANSP_UNKNOWN_RCPT,
	"no_address_mappings", INPUT_TRANSP_ADDRESS_MAPPING,
	"no_header_body_checks", INPUT_TRANSP_HEADER_BODY,
	"no_milters", INPUT_TRANSP_MILTER,
	0,
    };

    return (name_mask(param_name, table, pattern));
}

/* input_transp_cleanup - adjust cleanup options */

int     input_transp_cleanup(int cleanup_flags, int transp_mask)
{
    const char *myname = "input_transp_cleanup";

    if (msg_verbose)
	msg_info("before %s: cleanup flags = %s",
		 myname, cleanup_strflags(cleanup_flags));
    if (transp_mask & INPUT_TRANSP_ADDRESS_MAPPING)
	cleanup_flags &= ~(CLEANUP_FLAG_BCC_OK | CLEANUP_FLAG_MAP_OK);
    if (transp_mask & INPUT_TRANSP_HEADER_BODY)
	cleanup_flags &= ~CLEANUP_FLAG_FILTER;
    if (transp_mask & INPUT_TRANSP_MILTER)
	cleanup_flags &= ~CLEANUP_FLAG_MILTER;
    if (msg_verbose)
	msg_info("after %s: cleanup flags = %s",
		 myname, cleanup_strflags(cleanup_flags));
    return (cleanup_flags);
}
