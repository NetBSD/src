#ifndef _MBOX_OPEN_H_INCLUDED_
#define _MBOX_OPEN_H_INCLUDED_

/*++
/* NAME
/*	mbox_open 3h
/* SUMMARY
/*	mailbox access
/* SYNOPSIS
/*	#include <mbox_open.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>
#include <safe_open.h>

 /*
  * External interface.
  */
typedef struct {
    char    *path;			/* saved path, for dot_unlock */
    VSTREAM *fp;			/* open stream or null */
    int     locked;			/* what locks were set */
} MBOX;
extern MBOX *mbox_open(const char *, int, int, struct stat *, uid_t, gid_t, int, VSTRING *);
extern void mbox_release(MBOX *);

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
