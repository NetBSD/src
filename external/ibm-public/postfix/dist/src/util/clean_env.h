/*	$NetBSD: clean_env.h,v 1.2 2020/03/18 19:05:21 christos Exp $	*/

#ifndef _CLEAN_ENV_H_INCLUDED_
#define _CLEAN_ENV_H_INCLUDED_

/*++
/* NAME
/*	clean_env 3h
/* SUMMARY
/*	clean up the environment
/* SYNOPSIS
/*	#include <clean_env.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern void clean_env(char **);
extern void update_env(char **);

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

#endif
