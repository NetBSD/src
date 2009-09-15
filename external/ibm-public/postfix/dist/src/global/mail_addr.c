/*	$NetBSD: mail_addr.c,v 1.1.1.1.2.2 2009/09/15 06:02:44 snj Exp $	*/

/*++
/* NAME
/*	mail_addr 3
/* SUMMARY
/*	pre-defined mail addresses
/* SYNOPSIS
/*	#include <mail_addr.h>
/*
/*	const char *mail_addr_double_bounce()
/*
/*	const char *mail_addr_postmaster()
/*
/*	const char *mail_addr_mail_daemon()
/* DESCRIPTION
/*	This module predefines the following addresses:
/* .IP MAIL_ADDR_POSTMASTER
/*	The postmaster handle. Typically used for sending mail to.
/* .IP MAIL_ADDR_MAIL_DAEMON
/*	The mailer-daemon handle. Typically used to bring bad news.
/* .IP MAIL_ADDR_EMPTY
/*	The empty mail address. This refers to the postmaster on the
/*	local machine.
/* .PP
/*	mail_addr_double_bounce() produces the fully-qualified version
/*	of the local double bounce address.
/*
/*	mail_addr_postmaster() produces the fully-qualified version
/*	of the local postmaster address.
/*
/*	mail_addr_mail_daemon() produces the fully-qualified version
/*	of the local mailer-daemon address.
/* CONFIGURATION PARAMETERS
/*	double_bounce_sender, the double bounce pseudo account.
/*	myhostname, the local machine hostname.
/* BUGS
/*	Addresses are constructed by string concatenation, instead of
/*	passing them to the rewriting service.
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

#include <stringops.h>

/* Global library. */

#include "mail_params.h"
#include "mail_addr.h"

/* mail_addr_double_bounce - construct the local double-bounce address */

const char *mail_addr_double_bounce(void)
{
    static char *addr;

    if (addr == 0)
	addr = concatenate(var_double_bounce_sender, "@",
			   var_myhostname, (char *) 0);
    return (addr);
}

/* mail_addr_postmaster - construct the local postmaster address */

const char *mail_addr_postmaster(void)
{
    static char *addr;

    if (addr == 0)
	addr = concatenate(MAIL_ADDR_POSTMASTER, "@",
			   var_myhostname, (char *) 0);
    return (addr);
}

/* mail_addr_mail_daemon - construct the local mailer-daemon address */

const char *mail_addr_mail_daemon(void)
{
    static char *addr;

    if (addr == 0)
	addr = concatenate(MAIL_ADDR_MAIL_DAEMON, "@",
			   var_myhostname, (char *) 0);
    return (addr);
}
