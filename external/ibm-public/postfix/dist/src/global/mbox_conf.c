/*	$NetBSD: mbox_conf.c,v 1.1.1.1.2.2 2009/09/15 06:02:47 snj Exp $	*/

/*++
/* NAME
/*	mbox_conf 3
/* SUMMARY
/*	mailbox lock configuration
/* SYNOPSIS
/*	#include <mbox_conf.h>
/*
/*	int	mbox_lock_mask(string)
/*	const char *string;
/*
/*	ARGV	*mbox_lock_names()
/* DESCRIPTION
/*	The functions in this module translate between external
/*	mailbox locking method names and internal representations.
/*
/*	mbox_lock_mask() translates a string with locking method names
/*	into a bit mask. Names are separated by comma or whitespace.
/*	The following gives the method names and corresponding bit
/*	mask value:
/* .IP "flock (MBOX_FLOCK_LOCK)"
/*	Use flock() style lock after opening the file. This is the mailbox
/*	locking method traditionally used on BSD-ish systems (including
/*	Ultrix and SunOS). It is not suitable for remote file systems.
/* .IP "fcntl (MBOX_FCNTL_LOCK)"
/*	Use fcntl() style lock after opening the file. This is the mailbox
/*	locking method on System-V-ish systems (Solaris, AIX, IRIX, HP-UX).
/*	This method is supposed to work for remote systems, but often
/*	has problems.
/* .IP "dotlock (MBOX_DOT_LOCK)"
/*	Create a lock file with the name \fIfilename\fB.lock\fR. This
/*	method pre-dates kernel locks. This works with remote file systems,
/*	modulo cache coherency problems.
/* .PP
/*	mbox_lock_names() returns an array with the names of available
/*	mailbox locking methods. The result should be given to argv_free().
/* DIAGNOSTICS
/*	Fatal errors: undefined locking method name.
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
#include <argv.h>

/* Global library. */

#include <mail_params.h>
#include <mbox_conf.h>

 /*
  * The table with available mailbox locking methods. Some systems have
  * flock() locks; all POSIX-compatible systems have fcntl() locks. Even
  * though some systems do not use dotlock files by default (4.4BSD), such
  * locks can be necessary when accessing mailbox files over NFS.
  */
static const NAME_MASK mbox_mask[] = {
#ifdef HAS_FLOCK_LOCK
    "flock", MBOX_FLOCK_LOCK,
#endif
#ifdef HAS_FCNTL_LOCK
    "fcntl", MBOX_FCNTL_LOCK,
#endif
    "dotlock", MBOX_DOT_LOCK,
    0,
};

/* mbox_lock_mask - translate mailbox lock names to bit mask */

int     mbox_lock_mask(const char *string)
{
    return (name_mask(VAR_MAILBOX_LOCK, mbox_mask, string));
}

/* mbox_lock_names - return available mailbox lock method names */

ARGV   *mbox_lock_names(void)
{
    const NAME_MASK *np;
    ARGV   *argv;

    argv = argv_alloc(2);
    for (np = mbox_mask; np->name != 0; np++)
	argv_add(argv, np->name, ARGV_END);
    argv_terminate(argv);
    return (argv);
}
