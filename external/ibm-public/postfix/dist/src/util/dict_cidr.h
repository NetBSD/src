/*	$NetBSD: dict_cidr.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _DICT_CIDR_H_INCLUDED_
#define _DICT_CIDR_H_INCLUDED_

/*++
/* NAME
/*	dict_cidr 3h
/* SUMMARY
/*	Dictionary manager interface to handle cidr data.
/* SYNOPSIS
/*	#include <dict_cidr.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
extern DICT *dict_cidr_open(const char *, int, int);

#define DICT_TYPE_CIDR		"cidr"

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
/*	Jozsef Kadlecsik
/*	kadlec@blackhole.kfki.hu
/*	KFKI Research Institute for Particle and Nuclear Physics
/*	POB. 49
/*	1525 Budapest 114, Hungary
/*--*/

#endif
