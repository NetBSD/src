#ifndef _SAFE_OPEN_H_INCLUDED_
#define _SAFE_OPEN_H_INCLUDED_

/*++
/* NAME
/*	safe_open 3h
/* SUMMARY
/*	safely open or create regular file
/* SYNOPSIS
/*	#include <safe_open.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys/stat.h>
#include <fcntl.h>

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>

 /*
  * External interface.
  */
extern VSTREAM *safe_open(const char *, int, int, struct stat *, uid_t, gid_t, VSTRING *);

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
