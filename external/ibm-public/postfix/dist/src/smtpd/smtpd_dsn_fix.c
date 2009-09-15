/*	$NetBSD: smtpd_dsn_fix.c,v 1.1.1.1.2.2 2009/09/15 06:03:34 snj Exp $	*/

/*++
/* NAME
/*	smtpd_dsn_fix 3
/* SUMMARY
/*	fix DSN status
/* SYNOPSIS
/*	#include <smtpd_dsn_fix.h>
/*
/*	const char *smtpd_dsn_fix(status, reply_class)
/*	const char *status;
/*	const char *reply_class;
/* DESCRIPTION
/*	smtpd_dsn_fix() transforms DSN status codes according to the
/*	status information that is actually being reported. The
/*	following transformations are implemented:
/* .IP \(bu
/*	Transform a recipient address DSN into a sender address DSN
/*	when reporting sender address status information, and vice
/*	versa. This transformation may be needed because some Postfix
/*	access control features don't know whether the address being
/*	rejected is a sender or recipient. Examples are smtpd access
/*	tables, rbl reply templates, and the error mailer.
/* .IP \(bu
/*	Transform a sender or recipient address DSN into a non-address
/*	DSN when reporting non-address status information. For
/*	example, if something rejects HELO with DSN status 4.1.1
/*	(unknown recipient address), then we send the more neutral
/*	4.0.0 DSN instead. This transformation is needed when the
/*	same smtpd access map entry or rbl reply template is used
/*	for both address and non-address information.
/* .PP
/*	A non-address DSN is not transformed
/*	when reporting sender or recipient address status information,
/*	as there are many legitimate instances of such usage.
/*
/*	It is left up to the caller to update the initial DSN digit
/*	appropriately; in Postfix this is done as late as possible,
/*	because hard rejects may be changed into soft rejects for
/*	all kinds of reasons.
/*
/*	Arguments:
/* .IP status
/*	A DSN status as per RFC 3463.
/* .IP reply_class
/*	SMTPD_NAME_SENDER, SMTPD_NAME_RECIPIENT or some other
/*	null-terminated string.
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
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include <msg.h>

/* Global library. */

/* Application-specific. */

#include <smtpd_dsn_fix.h>

struct dsn_map {
    const char *micro_code;		/* Final digits in mailbox D.S.N. */
    const char *sender_dsn;		/* Replacement sender D.S.N. */
    const char *rcpt_dsn;		/* Replacement recipient D.S.N. */
};

static struct dsn_map dsn_map[] = {
    /* - Sender - Recipient */
    "1", SND_DSN, "4.1.1",		/* 4.1.1: Bad dest mbox addr */
    "2", "4.1.8", "4.1.2",		/* 4.1.2: Bad dest system addr */
    "3", "4.1.7", "4.1.3",		/* 4.1.3: Bad dest mbox addr syntax */
    "4", SND_DSN, "4.1.4",		/* 4.1.4: Dest mbox addr ambiguous */
    "5", "4.1.0", "4.1.5",		/* 4.1.5: Dest mbox addr valid */
    "6", SND_DSN, "4.1.6",		/* 4.1.6: Mailbox has moved */
    "7", "4.1.7", "4.1.3",		/* 4.1.7: Bad sender mbox addr syntax */
    "8", "4.1.8", "4.1.2",		/* 4.1.8: Bad sender system addr */
    0, "4.1.0", "4.1.0",		/* Default mapping */
};

/* smtpd_dsn_fix - fix DSN status */

const char *smtpd_dsn_fix(const char *status, const char *reply_class)
{
    struct dsn_map *dp;
    const char *result = status;

    /*
     * Update an address-specific DSN according to what is being rejected.
     */
    if (ISDIGIT(status[0]) && strncmp(status + 1, ".1.", 3) == 0) {

	/*
	 * Fix recipient address DSN while rejecting a sender address. Don't
	 * let future recipient-specific DSN codes slip past us.
	 */
	if (strcmp(reply_class, SMTPD_NAME_SENDER) == 0) {
	    for (dp = dsn_map; dp->micro_code != 0; dp++)
		if (strcmp(status + 4, dp->micro_code) == 0)
		    break;
	    result = dp->sender_dsn;
	}

	/*
	 * Fix sender address DSN while rejecting a recipient address. Don't
	 * let future sender-specific DSN codes slip past us.
	 */
	else if (strcmp(reply_class, SMTPD_NAME_RECIPIENT) == 0) {
	    for (dp = dsn_map; dp->micro_code != 0; dp++)
		if (strcmp(status + 4, dp->micro_code) == 0)
		    break;
	    result = dp->rcpt_dsn;
	}

	/*
	 * Fix address-specific DSN while rejecting a non-address.
	 */
	else {
	    result = "4.0.0";
	}

	/*
	 * Give them a clue of what is going on.
	 */
	if (strcmp(status + 2, result + 2) != 0)
	    msg_info("mapping DSN status %s into %s status %c%s",
		     status, reply_class, status[0], result + 1);
	return (result);
    }

    /*
     * Don't update a non-address DSN. There are many legitimate uses for
     * these while rejecting address or non-address information.
     */
    else {
	return (status);
    }
}
