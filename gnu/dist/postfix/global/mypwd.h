#ifndef _MYPWNAM_H_INCLUDED_
#define _MYPWNAM_H_INCLUDED_

/*++
/* NAME
/*	mypwnam 3h
/* SUMMARY
/*	caching getpwnam()/getpwuid()
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
