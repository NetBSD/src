/*	$NetBSD: mypwd.h,v 1.1.1.2 2013/01/02 18:58:59 tron Exp $	*/

#ifndef _MYPWNAM_H_INCLUDED_
#define _MYPWNAM_H_INCLUDED_

/*++
/* NAME
/*	mypwnam 3h
/* SUMMARY
/*	caching getpwnam_r()/getpwuid_r()
/* SYNOPSIS
/*	#include <mypwd.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
struct mypasswd {
    int     refcount;
    char   *pw_name;
    char   *pw_passwd;
    uid_t   pw_uid;
    gid_t   pw_gid;
    char   *pw_gecos;
    char   *pw_dir;
    char   *pw_shell;
};

extern int mypwnam_err(const char *, struct mypasswd **);
extern int mypwuid_err(uid_t, struct mypasswd **);
extern struct mypasswd *mypwnam(const char *);
extern struct mypasswd *mypwuid(uid_t);
extern void mypwfree(struct mypasswd *);

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
