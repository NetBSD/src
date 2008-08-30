#ifndef _MAIL_RUN_H_INCLUDED_
#define _MAIL_RUN_H_INCLUDED_

/*++
/* NAME
/*	mail_run 3h
/* SUMMARY
/*	run mail component program
/* SYNOPSIS
/*	#include <mail_run.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int mail_run_foreground(const char *, char **);
extern int mail_run_background(const char *, char **);
extern NORETURN mail_run_replace(const char *, char **);

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

#endif
