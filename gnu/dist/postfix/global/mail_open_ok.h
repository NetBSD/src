#ifndef _MAIL_OPEN_OK_H_INCLUDED_
#define _MAIL_OPEN_OK_H_INCLUDED_

/*++
/* NAME
/*	mail_open_ok 3h
/* SUMMARY
/*	scrutinize mail queue file
/* SYNOPSIS
/*	#include <mail_open_ok.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int mail_open_ok(const char *, const char *, struct stat *,
			        const char **);

#define MAIL_OPEN_YES	1
#define MAIL_OPEN_NO	2

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
