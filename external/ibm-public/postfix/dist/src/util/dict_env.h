/*	$NetBSD: dict_env.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _DICT_ENV_H_INCLUDED_
#define _DICT_ENV_H_INCLUDED_

/*++
/* NAME
/*	dict_env 3h
/* SUMMARY
/*	dictionary manager interface to environment variables
/* SYNOPSIS
/*	#include <dict_env.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
#define DICT_TYPE_ENVIRON	"environ"

extern DICT *dict_env_open(const char *, int, int);

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
