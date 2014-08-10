/*	$NetBSD: timecmp.h,v 1.1.1.1.2.2 2014/08/10 07:12:50 tls Exp $	*/

#ifndef _TIMECMP_H_INCLUDED_
#define _TIMECMP_H_INCLUDED_

#include <time.h>

/*++
/* NAME
/*	timecmp 3h
/* SUMMARY
/*	compare two time_t values
/* SYNOPSIS
/*	#include <timecmp.h>
/*
/*	int	timecmp(t1, t2)
/*	time_t	t1;
/*	time_t	t2;
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int timecmp(time_t, time_t);

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
/*	Viktor Dukhovni
/*--*/

#endif
