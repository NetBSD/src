/*	$NetBSD: postqueue.h,v 1.2.4.2 2017/04/21 16:52:50 bouyer Exp $	*/

/*++
/* NAME
/*	postqueue 5h
/* SUMMARY
/*	postqueue internal interfaces
/* SYNOPSIS
/*	#include <postqueue.h>
/* DESCRIPTION
/* .nf

 /*
  * showq_compat.c
  */
extern void showq_compat(VSTREAM *);

 /*
  * showq_json.c
  */
extern void showq_json(VSTREAM *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/
