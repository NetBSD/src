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
/* .IP "no_address_mapping (INPUT_TRANSP_ADDRESS_MAPPING)
/*	Disable canonical address mapping, virtual alias map expansion,
/*	address masquerading, and automatic BCC recipients.
/* .IP "no_header_body_checkss (INPUT_TRANSP_HEADER_BODY)
/*	Disable header/body_checks.
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

/* Global library. */

#include <mail_params.h>
#include <input_transp.h>

/* input_transp_mask - compute mail receive transparency mask */

int     input_transp_mask(const char *param_name, const char *pattern)
{
    static NAME_MASK table[] = {
	"no_unknown_recipient_checks", INPUT_TRANSP_UNKNOWN_RCPT,
	"no_address_mappings", INPUT_TRANSP_ADDRESS_MAPPING,
 	"no_header_body_checks", INPUT_TRANSP_HEADER_BODY,
	0,
    };

    return (name_mask(param_name, table, pattern));
}
