/*	$NetBSD: int_filt.c,v 1.1.1.1.36.1 2017/04/21 16:52:48 bouyer Exp $	*/

/*++
/* NAME
/*	int_filt 3
/* SUMMARY
/*	internal mail filter control
/* SYNOPSIS
/*	#include <int_filt.h>
/*
/*	int	int_filt_flags(class)
/*	int	class;
/* DESCRIPTION
/*	int_filt_flags() determines the appropriate mail filtering
/*	flags for the cleanup server, depending on the setting of
/*	the internal_mail_filter_classes configuration parameter.
/*
/*	Specify one of the following:
/* .IP MAIL_SRC_MASK_NOTIFY
/*	Postmaster notifications from the smtpd(8) and smtp(8)
/*	protocol adapters.
/* .IP MAIL_SRC_MASK_BOUNCE
/*	Delivery status notifications from the bounce(8) server.
/* .PP
/*	Other MAIL_SRC_MASK_XXX arguments are permited but will
/*	have no effect.
/* DIAGNOSTICS
/*	Fatal: invalid mail category name.
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
#include <int_filt.h>

/* int_filt_flags - map mail class to submission flags */

int     int_filt_flags(int class)
{
    static const NAME_MASK table[] = {
	MAIL_SRC_NAME_NOTIFY, MAIL_SRC_MASK_NOTIFY,
	MAIL_SRC_NAME_BOUNCE, MAIL_SRC_MASK_BOUNCE,
	MAIL_SRC_NAME_SENDMAIL, 0,
	MAIL_SRC_NAME_SMTPD, 0,
	MAIL_SRC_NAME_QMQPD, 0,
	MAIL_SRC_NAME_FORWARD, 0,
	MAIL_SRC_NAME_VERIFY, 0,
	0,
    };
    int     filtered_classes = 0;

    if (class && *var_int_filt_classes) {
	filtered_classes =
	    name_mask(VAR_INT_FILT_CLASSES, table, var_int_filt_classes);
	if (filtered_classes == 0)
	    msg_warn("%s: bad input: %s", VAR_INT_FILT_CLASSES,
		     var_int_filt_classes);
	if (filtered_classes & class)
	    return (CLEANUP_FLAG_FILTER | CLEANUP_FLAG_MILTER);
    }
    return (0);
}
