/*	$NetBSD: server_acl.h,v 1.1.1.1.6.2 2013/02/25 00:27:19 tls Exp $	*/

#ifndef _SERVER_ACL_INCLUDED_
#define _SERVER_ACL_INCLUDED_

/*++
/* NAME
/*	dict_memcache 3h
/* SUMMARY
/*	dictionary interface to memcache databases
/* SYNOPSIS
/*	#include <dict_memcache.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <argv.h>

 /*
  * External interface.
  */
typedef ARGV SERVER_ACL;
extern void server_acl_pre_jail_init(const char *, const char *);
extern SERVER_ACL *server_acl_parse(const char *, const char *);
extern int server_acl_eval(const char *, SERVER_ACL *, const char *);

#define SERVER_ACL_NAME_WL_MYNETWORKS "permit_mynetworks"
#define SERVER_ACL_NAME_PERMIT	"permit"
#define SERVER_ACL_NAME_DUNNO	"dunno"
#define SERVER_ACL_NAME_REJECT	"reject"
#define SERVER_ACL_NAME_ERROR	"error"

#define SERVER_ACL_ACT_PERMIT	1
#define SERVER_ACL_ACT_DUNNO	0
#define SERVER_ACL_ACT_REJECT	(-1)
#define SERVER_ACL_ACT_ERROR	(-2)

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
