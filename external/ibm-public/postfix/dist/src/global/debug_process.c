/*	$NetBSD: debug_process.c,v 1.1.1.1.2.2 2009/09/15 06:02:39 snj Exp $	*/

/*++
/* NAME
/*	debug_process 3
/* SUMMARY
/*	run an external debugger
/* SYNOPSIS
/*	#include <debug_process.h>
/*
/*	char	*debug_process()
/* DESCRIPTION
/*	debug_process() runs a debugger, as specified in the
/*	\fIdebugger_command\fR configuration variable.
/*
/*	Examples of non-interactive debuggers are call tracing tools
/*	such as: trace, strace or truss.
/*
/*	Examples of interactive debuggers are xxgdb, xxdbx, and so on.
/*	In order to use an X-based debugger, the process must have a
/*	properly set up XAUTHORITY environment variable.
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
#include <stdlib.h>
#include <unistd.h>

/* Utility library. */

#include <msg.h>

/* Global library. */

#include "mail_params.h"
#include "mail_conf.h"
#include "debug_process.h"

/* debug_process - run a debugger on this process */

void    debug_process(void)
{
    const char *command;

    /*
     * Expand $debugger_command then run it.
     */
    command = mail_conf_lookup_eval(VAR_DEBUG_COMMAND);
    if (command == 0 || *command == 0)
	msg_fatal("no %s variable set up", VAR_DEBUG_COMMAND);
    msg_info("running: %s", command);
    system(command);
}
