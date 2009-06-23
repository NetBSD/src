/*	$NetBSD: msg_vstream.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef _MSG_VSTREAM_H_INCLUDED_
#define _MSG_VSTREAM_H_INCLUDED_

/*++
/* NAME
/*	msg_vstream 3h
/* SUMMARY
/*	direct diagnostics to VSTREAM
/* SYNOPSIS
/*	#include <msg_vstream.h>
/* DESCRIPTION

 /*
  * Utility library.
  */
#include <vstream.h>

 /*
  * External interface.
  */
extern void msg_vstream_init(const char *, VSTREAM *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*      IBM T.J. Watson Research
/*      P.O. Box 704
/*      Yorktown Heights, NY 10598, USA
/*--*/

#endif
