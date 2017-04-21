/*	$NetBSD: smtputf8.c,v 1.2.4.2 2017/04/21 16:52:48 bouyer Exp $	*/

/*++
/* NAME
/*	smtputf8 3
/* SUMMARY
/*	SMTPUTF8 support
/* SYNOPSIS
/*	#include <smtputf8.h>
/*
/*	int	smtputf8_autodetect(class)
/*	int	class;
/* DESCRIPTION
/*	smtputf8_autodetect() determines whether the cleanup server
/*	should perform SMTPUTF8 detection, depending on the declared
/*	source class and the setting of the smtputf8_autodetect_classes
/*	configuration parameter.
/*
/*	Specify one of the following:
/* .IP MAIL_SRC_MASK_SENDMAIL
/*	Submission with the Postfix sendmail(1) command.
/* .IP MAIL_SRC_MASK_SMTPD
/*	Mail received with the smtpd(8) daemon.
/* .IP MAIL_SRC_MASK_QMQPD
/*	Mail received with the qmqpd(8) daemon.
/* .IP MAIL_SRC_MASK_FORWARD
/*	Local forwarding or aliasing.
/* .IP MAIL_SRC_MASK_BOUNCE
/*	Submission by the bounce(8) daemon. 
/* .IP MAIL_SRC_MASK_NOTIFY
/*	Postmaster notification from the smtp(8) or smtpd(8) daemon.
/* .IP MAIL_SRC_MASK_VERIFY
/*	Address verification probe.
/* DIAGNOSTICS
/*	Panic: no valid class argument.
/*
/*	Specify one of the following:
/*	Warning: the smtputf8_autodetect_classes parameter specifies
/*	an invalid source category name.
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
#include <mail_proto.h>
#include <smtputf8.h>

/* smtputf8_autodetect - enable SMTPUTF8 autodetection */

int     smtputf8_autodetect(int class)
{
    static const char myname[] = "smtputf8_autodetect";
    static const NAME_MASK table[] = {
	MAIL_SRC_NAME_SENDMAIL, MAIL_SRC_MASK_SENDMAIL,
	MAIL_SRC_NAME_SMTPD, MAIL_SRC_MASK_SMTPD,
	MAIL_SRC_NAME_QMQPD, MAIL_SRC_MASK_QMQPD,
	MAIL_SRC_NAME_FORWARD, MAIL_SRC_MASK_FORWARD,
	MAIL_SRC_NAME_BOUNCE, MAIL_SRC_MASK_BOUNCE,
	MAIL_SRC_NAME_NOTIFY, MAIL_SRC_MASK_NOTIFY,
	MAIL_SRC_NAME_VERIFY, MAIL_SRC_MASK_VERIFY,
	MAIL_SRC_NAME_ALL, MAIL_SRC_MASK_ALL,
	0,
    };
    int     autodetect_classes = 0;

    if (class == 0 || (class & ~MAIL_SRC_MASK_ALL) != 0)
	msg_panic("%s: bad source class: %d", myname, class);
    if (*var_smtputf8_autoclass) {
	autodetect_classes =
	    name_mask(VAR_SMTPUTF8_AUTOCLASS, table, var_smtputf8_autoclass);
	if (autodetect_classes == 0)
	    msg_warn("%s: bad input: %s", VAR_SMTPUTF8_AUTOCLASS,
		     var_smtputf8_autoclass);
	if (autodetect_classes & class)
	    return (CLEANUP_FLAG_AUTOUTF8);
    }
    return (0);
}
