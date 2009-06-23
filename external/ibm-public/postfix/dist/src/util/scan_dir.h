/*	$NetBSD: scan_dir.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef _SCAN_DIR_H_INCLUDED_
#define _SCAN_DIR_H_INCLUDED_

/*++
/* NAME
/*	scan_dir 3h
/* SUMMARY
/*	directory scanner
/* SYNOPSIS
/*	#include <scan_dir.h>
/* DESCRIPTION
/* .nf

 /*
  * The directory scanner interface.
  */
typedef struct SCAN_DIR SCAN_DIR;

extern SCAN_DIR *scan_dir_open(const char *);
extern char *scan_dir_next(SCAN_DIR *);
extern char *scan_dir_path(SCAN_DIR *);
extern void scan_dir_push(SCAN_DIR *, const char *);
extern SCAN_DIR *scan_dir_pop(SCAN_DIR *);
extern SCAN_DIR *scan_dir_close(SCAN_DIR *);

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
