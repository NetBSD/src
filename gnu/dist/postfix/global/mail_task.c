/*++
/* NAME
/*	mail_task 3
/* SUMMARY
/*	set task name for logging purposes
/* SYNOPSIS
/*	#include <mail_task.h>
/*
/*	const char *mail_task(argv0)
/*	const char *argv0;
/* DESCRIPTION
/*	mail_task() enforces consistent naming of mailer processes.
/*	It strips pathname information from the process name, and
/*	prepends the name of the mail system so that logfile entries
/*	are easier to recognize.
/*
/*	The result is volatile.  Make a copy of the result if it is
/*	to be used for any appreciable amount of time.
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

#include "mail_params.h"
#include "mail_task.h"

#define MAIL_TASK_FORMAT	"postfix/%s"

/* mail_task - clean up and decorate the process name */

const char *mail_task(const char *argv0)
{
    static VSTRING *canon_name;
    const char *slash;

    if (canon_name == 0)
	canon_name = vstring_alloc(10);
    if ((slash = strrchr(argv0, '/')) != 0)
	argv0 = slash + 1;
    vstring_sprintf(canon_name, MAIL_TASK_FORMAT, argv0);
    return (vstring_str(canon_name));
}
