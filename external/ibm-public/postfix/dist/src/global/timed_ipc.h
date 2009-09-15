/*	$NetBSD: timed_ipc.h,v 1.1.1.1.2.2 2009/09/15 06:02:54 snj Exp $	*/

#ifndef _TIMED_IPC_H_INCLUDED_
#define _TIMED_IPC_H_INCLUDED_

/*++
/* NAME
/*	timed_ipc 3h
/* SUMMARY
/*	enforce IPC timeout on stream
/* SYNOPSIS
/*	#include <timed_ipc.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>

 /*
  * External interface.
  */
extern void timed_ipc_setup(VSTREAM *);

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
