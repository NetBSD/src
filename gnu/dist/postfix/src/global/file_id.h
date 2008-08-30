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

 /* External interface. */

extern const char *get_file_id(int);
extern int check_file_id(int, const char *);

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
