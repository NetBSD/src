/*	$NetBSD: verp_sender.c,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

/*++
/* NAME
/*	verp_sender 3
/* SUMMARY
/*	quote local part of mailbox
/* SYNOPSIS
/*	#include <verp_sender.h>
/*
/*	VSTRING	*verp_sender(dst, delims, sender, recipient)
/*	VSTRING	*dst;
/*	const char *delims;
/*	const char *sender;
/*	const RECIPIENT *recipient;
/*
/*	const char *verp_delims_verify(delims)
/*	const char *delims;
/* DESCRIPTION
/*	verp_sender() encodes the recipient address in the sender
/*	address, using the specified delimiters. For example,
/*	with delims +=, sender \fIprefix@origin\fR, and
/*	recipient \fIuser@domain\fR the result is
/*	\fIprefix+user=domain@origin\fR.
/*
/*	verp_delims_verify() determines if the specified VERP delimiters
/*	have reasonable values. What is reasonable is configured with
/*	the verp_delimiter_filter configuration parameter. The result
/*	is null in case of success, a description of the problem in
/*	case of error.
/*
/*	Arguments:
/* .IP dst
/*	The result. The buffer is null terminated.
/* .IP delims
/*	VERP formatting characters.
/* .IP sender
/*	Sender envelope address.
/* .IP recipient
/*	Recipient envelope address.
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

/* Utility library. */

#include <vstring.h>

/* Global library. */

#include <mail_params.h>
#include <recipient_list.h>
#include <verp_sender.h>

/* verp_sender - encode recipient into envelope sender address */

VSTRING *verp_sender(VSTRING *buf, const char *delimiters,
		             const char *sender, const RECIPIENT *rcpt_info)
{
    ssize_t send_local_len;
    ssize_t rcpt_local_len;
    const char *recipient;
    const char *cp;

    /*
     * Change prefix@origin into prefix+user=domain@origin.
     * 
     * Fix 20090115: Use the Postfix original recipient, because that is what
     * the VERP consumer expects.
     */
    send_local_len = ((cp = strrchr(sender, '@')) != 0 ?
		      cp - sender : strlen(sender));
    recipient = (rcpt_info->orig_addr[0] ?
		 rcpt_info->orig_addr : rcpt_info->address);
    rcpt_local_len = ((cp = strrchr(recipient, '@')) != 0 ?
		      cp - recipient : strlen(recipient));
    vstring_strncpy(buf, sender, send_local_len);
    VSTRING_ADDCH(buf, delimiters[0] & 0xff);
    vstring_strncat(buf, recipient, rcpt_local_len);
    if (recipient[rcpt_local_len] && recipient[rcpt_local_len + 1]) {
	VSTRING_ADDCH(buf, delimiters[1] & 0xff);
	vstring_strcat(buf, recipient + rcpt_local_len + 1);
    }
    if (sender[send_local_len] && sender[send_local_len + 1]) {
	VSTRING_ADDCH(buf, '@');
	vstring_strcat(buf, sender + send_local_len + 1);
    }
    VSTRING_TERMINATE(buf);
    return (buf);
}

/* verp_delims_verify - sanitize VERP delimiters */

const char *verp_delims_verify(const char *delims)
{
    if (strlen(delims) != 2)
	return ("bad VERP delimiter character count");
    if (strchr(var_verp_filter, delims[0]) == 0)
	return ("bad first VERP delimiter character");
    if (strchr(var_verp_filter, delims[1]) == 0)
	return ("bad second VERP delimiter character");
    return (0);
}
