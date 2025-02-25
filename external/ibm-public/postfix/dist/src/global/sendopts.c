/*	$NetBSD: sendopts.c,v 1.2 2025/02/25 19:15:46 christos Exp $	*/

/*++
/* NAME
/*	sendopts 3
/* SUMMARY
/*	Support for SMTPUTF8, REQUIRETLS, etc.
/* SYNOPSIS
/*	#include <sendopts.h>
/*
/*	const char *sendopts_strflags(
/*	int	flags,
/*	int	delim)
/* DESCRIPTION
/*	Postfix queue files and IPC messages contain a sendopts field
/*	with flags that control SMTPUTF8, REQUIRETLS, etc. support. The
/*	flags are documented in sendopts(3h), and are based on information
/*	received with ESMTP requests or with message content.
/*
/*	The SMTPUTF8 flags life cycle is documented in smtputf8(3h).
/*
/*	sendopts_strflags() maps a sendopts flag value to printable
/*	string. The result is overwritten upon each call.
/*
/*	Arguments:
/* .IP flags
/*	A bitmask that is to be converted to text.
/* .IP delim
/*	The character to separate output words with: one of ' ,|'.
/* DIAGNOSTICS
/*	Panic: invalid delimiter. Fatal error: invalid flag.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <vstring.h>
#include <name_mask.h>

 /*
  * Global library.
  */
#include <sendopts.h>

 /*
  * Mapping from flags to printable string.
  */
static NAME_MASK sendopts_flag_map[] = {
    "smtputf8_requested", SOPT_SMTPUTF8_REQUESTED,
    "smtputf8_header", SOPT_SMTPUTF8_HEADER,
    "smtputf8_sender", SOPT_SMTPUTF8_SENDER,
    "smtputf8_recipient", SOPT_SMTPUTF8_RECIPIENT,
    "requiretls_header", SOPT_REQUIRETLS_HEADER,
    "requiretls_esmtp", SOPT_REQUIRETLS_ESMTP,
    0,
};

/* sendopts_strflags - map flags code to printable string */

const char *sendopts_strflags(unsigned flags, int delim)
{
    const char myname[] = "sendopts_strflags";
    static const char delims[] = " ,|";
    static const int dflags[] = {0, NAME_MASK_COMMA, NAME_MASK_PIPE};
    static VSTRING *result;
    const char *cp;

    if (flags == 0)
	return ("none");

    if (result == 0)
	result = vstring_alloc(20);
    else
	VSTRING_RESET(result);

    if ((cp = strchr(delims, delim)) == 0)
	msg_panic("%s: bad delimiter: '%c'", myname, delim);

    return (str_name_mask_opt(result, "sendopts_strflags", sendopts_flag_map,
			      flags, NAME_MASK_FATAL | dflags[cp - delims]));
}

