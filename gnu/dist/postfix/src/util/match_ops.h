#ifndef _MATCH_OPS_H_INCLUDED_
#define _MATCH_OPS_H_INCLUDED_

/*++
/* NAME
/*	match_ops 3h
/* SUMMARY
/*	simple string or host pattern matching
/* SYNOPSIS
/*	#include <match_ops.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int match_string(const char *, const char *);
extern int match_hostname(const char *, const char *);
extern int match_hostaddr(const char *, const char *);

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
