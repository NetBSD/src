/*	$NetBSD: mail_scan_dir.h,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

#ifndef _MAIL_SCAN_DIR_H_INCLUDED_
#define _MAIL_SCAN_DIR_H_INCLUDED_

/*++
/* NAME
/*	mail_scan_dir 3h
/* SUMMARY
/*	mail directory scanner support
/* SYNOPSIS
/*	#include <mail_scan_dir.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <scan_dir.h>

 /*
  * External interface.
  */
extern char *mail_scan_dir_next(SCAN_DIR *);

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
