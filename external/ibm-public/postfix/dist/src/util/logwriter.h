/*	$NetBSD: logwriter.h,v 1.2 2020/03/18 19:05:21 christos Exp $	*/

#ifndef _LOGWRITER_H_INCLUDED_
#define _LOGWRITER_H_INCLUDED_

/*++
/* NAME
/*	logwriter 3h
/* SUMMARY
/*	logfile writer
/* SYNOPSIS
/*	#include <logwriter.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>

 /*
  * External interface.
  */
extern VSTREAM *logwriter_open_or_die(const char *);
extern int logwriter_write(VSTREAM *, const char *, ssize_t);
extern int logwriter_close(VSTREAM *);
extern int logwriter_one_shot(const char *, const char *, ssize_t);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
