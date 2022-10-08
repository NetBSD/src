/*	$NetBSD: login_sender_match.h,v 1.2 2022/10/08 16:12:45 christos Exp $	*/

#ifndef _LOGIN_SENDER_MATCH_H_INCLUDED_
#define _LOGIN_SENDER_MATCH_H_INCLUDED_

/*++
/* NAME
/*	login_sender_match 3h
/* SUMMARY
/*	oracle for per-login allowed sender addresses
/* SYNOPSIS
/*	#include <login_sender_match.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
typedef struct LOGIN_SENDER_MATCH LOGIN_SENDER_MATCH;

extern LOGIN_SENDER_MATCH *login_sender_create(const char *title,
					               const char *map_names,
					         const char *ext_delimiters,
					            const char *null_sender,
					               const char *wildcard);
extern void login_sender_free(LOGIN_SENDER_MATCH *lsm);
extern int login_sender_match(LOGIN_SENDER_MATCH *lsm, const char *login_name,
			              const char *sender_addr);

#define LSM_STAT_FOUND		(1)
#define LSM_STAT_NOTFOUND	(0)
#define LSM_STAT_RETRY		(DICT_ERR_RETRY)
#define LSM_STAT_CONFIG		(DICT_ERR_CONFIG)

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

#endif					/* _LOGIN_SENDER_MATCH_H_INCLUDED_ */
