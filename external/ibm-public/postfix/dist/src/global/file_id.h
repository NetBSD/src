/*	$NetBSD: file_id.h,v 1.1.1.2 2013/01/02 18:58:58 tron Exp $	*/

#ifndef _FILE_ID_H_INCLUDED_
#define _FILE_ID_H_INCLUDED_

/*++
/* NAME
/*	file_id 3h
/* SUMMARY
/*	file ID printable representation
/* SYNOPSIS
/*	#include <file_id.h>
/* DESCRIPTION
/* .nf

 /*
System library.
*/
#include <sys/stat.h>

 /* External interface. */

extern const char *get_file_id_fd(int, int);
extern const char *get_file_id_st(struct stat *, int);

 /* Legacy interface. */
extern const char *get_file_id(int);

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
