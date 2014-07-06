/*	$NetBSD: smtp_reuse.h,v 1.1.1.2 2014/07/06 19:27:56 tron Exp $	*/

/*++
/* NAME
/*	smtp_reuse 3h
/* SUMMARY
/*	SMTP session cache glue
/* SYNOPSIS
/*	#include <smtp_reuse.h>
/* DESCRIPTION
/* .nf

 /*
  * Internal interfaces.
  */
extern void smtp_save_session(SMTP_STATE *, int, int);
extern SMTP_SESSION *smtp_reuse_nexthop(SMTP_STATE *, int);
extern SMTP_SESSION *smtp_reuse_addr(SMTP_STATE *, int);

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
