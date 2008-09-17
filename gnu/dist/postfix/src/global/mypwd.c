/*++
/* NAME
/*	mypwd 3
/* SUMMARY
/*	caching getpwnam()/getpwuid()
/* SYNOPSIS
/*	#include <mypwd.h>
/*
/*	struct mypasswd *mypwuid(uid)
/*	uid_t	uid;
/*
/*	struct mypasswd *mypwnam(name)
/*	const char *name;
/*
/*	void	mypwfree(pwd)
/*	struct mypasswd *pwd;
/* DESCRIPTION
/*	This module maintains a reference-counted cache of password
/*	database lookup results. The idea is to avoid surprises by
/*	getpwnam() or getpwuid() overwriting a previous result, while
/*	at the same time avoiding duplicate copies of password
/*	information in memory, and to avoid making repeated getpwxxx()
/*	calls for the same information.
/*
/*	mypwnam() and mypwuid() are wrappers that cache a private copy
/*	of results from the getpwnam() and getpwuid() library routines.
/*	Results are shared between calls with the same \fIname\fR
/*	or \fIuid\fR argument, so changing results is verboten.
/*
/*	mypwfree() cleans up the result of mypwnam() and mypwuid().
/* BUGS
/*	This module is security sensitive and complex at the same
/*	time, which is bad.
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

/* System library. */

#include <sys_defs.h>
#include <pwd.h>
#include <string.h>
#ifdef USE_PATHS_H
#include <paths.h>
#endif

/* Utility library. */

#include <mymalloc.h>
#include <htable.h>
#include <binhash.h>
#include <msg.h>

/* Global library. */

#include "mypwd.h"

 /*
  * The private cache. One for lookups by name, one for lookups by uid, and
  * one for the last looked up result.
  */
static HTABLE *mypwcache_name = 0;
static BINHASH *mypwcache_uid = 0;
static struct mypasswd *last_pwd;

/* mypwenter - enter password info into cache */

static struct mypasswd *mypwenter(struct passwd * pwd)
{
    struct mypasswd *mypwd;

    /*
     * Initialize on the fly.
     */
    if (mypwcache_name == 0) {
	mypwcache_name = htable_create(0);
	mypwcache_uid = binhash_create(0);
    }
    mypwd = (struct mypasswd *) mymalloc(sizeof(*mypwd));
    mypwd->refcount = 0;
    mypwd->pw_name = mystrdup(pwd->pw_name);
    mypwd->pw_passwd = mystrdup(pwd->pw_passwd);
    mypwd->pw_uid = pwd->pw_uid;
    mypwd->pw_gid = pwd->pw_gid;
    mypwd->pw_gecos = mystrdup(pwd->pw_gecos);
    mypwd->pw_dir = mystrdup(pwd->pw_dir);
    mypwd->pw_shell = mystrdup(*pwd->pw_shell ? pwd->pw_shell : _PATH_BSHELL);
    htable_enter(mypwcache_name, mypwd->pw_name, (char *) mypwd);
    binhash_enter(mypwcache_uid, (char *) &mypwd->pw_uid,
		  sizeof(mypwd->pw_uid), (char *) mypwd);
    return (mypwd);
}

/* mypwuid - caching getpwuid() */

struct mypasswd *mypwuid(uid_t uid)
{
    struct passwd *pwd;
    struct mypasswd *mypwd;

    /*
     * See if this is the same user as last time.
     */
    if (last_pwd != 0) {
	if (last_pwd->pw_uid != uid) {
	    mypwfree(last_pwd);
	    last_pwd = 0;
	} else {
	    mypwd = last_pwd;
	    mypwd->refcount++;
	    return (mypwd);
	}
    }

    /*
     * Find the info in the cache or in the password database. Make a copy,
     * so that repeated getpwnam() calls will not clobber our result.
     */
    if ((mypwd = (struct mypasswd *)
	 binhash_find(mypwcache_uid, (char *) &uid, sizeof(uid))) == 0) {
	if ((pwd = getpwuid(uid)) == 0)
	    return (0);
	mypwd = mypwenter(pwd);
    }
    last_pwd = mypwd;
    mypwd->refcount += 2;
    return (mypwd);
}

/* mypwnam - caching getpwnam() */

struct mypasswd *mypwnam(const char *name)
{
    struct passwd *pwd;
    struct mypasswd *mypwd;

    /*
     * See if this is the same user as last time.
     */
    if (last_pwd != 0) {
	if (strcmp(last_pwd->pw_name, name) != 0) {
	    mypwfree(last_pwd);
	    last_pwd = 0;
	} else {
	    mypwd = last_pwd;
	    mypwd->refcount++;
	    return (mypwd);
	}
    }

    /*
     * Find the info in the cache or in the password database. Make a copy,
     * so that repeated getpwnam() calls will not clobber our result.
     */
    if ((mypwd = (struct mypasswd *) htable_find(mypwcache_name, name)) == 0) {
	if ((pwd = getpwnam(name)) == 0)
	    return (0);
	mypwd = mypwenter(pwd);
    }
    last_pwd = mypwd;
    mypwd->refcount += 2;
    return (mypwd);
}

/* mypwfree - destroy password info */

void    mypwfree(struct mypasswd * mypwd)
{
    if (mypwd->refcount < 1)
	msg_panic("mypwfree: refcount %d", mypwd->refcount);

    if (--mypwd->refcount == 0) {
	htable_delete(mypwcache_name, mypwd->pw_name, (void (*) (char *)) 0);
	binhash_delete(mypwcache_uid, (char *) &mypwd->pw_uid,
		       sizeof(mypwd->pw_uid), (void (*) (char *)) 0);
	myfree(mypwd->pw_name);
	myfree(mypwd->pw_passwd);
	myfree(mypwd->pw_gecos);
	myfree(mypwd->pw_dir);
	myfree(mypwd->pw_shell);
	myfree((char *) mypwd);
    }
}

#ifdef TEST

 /*
  * Test program. Look up a couple users and/or uid values and see if the
  * results will be properly free()d.
  */
#include <stdlib.h>
#include <ctype.h>
#include <vstream.h>
#include <msg_vstream.h>

int     main(int argc, char **argv)
{
    struct mypasswd **mypwd;
    int     i;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    if (argc == 1)
	msg_fatal("usage: %s name or uid ...", argv[0]);

    mypwd = (struct mypasswd **) mymalloc((argc + 1) * sizeof(*mypwd));

    for (i = 1; i < argc; i++) {
	if (ISDIGIT(argv[i][0]))
	    mypwd[i] = mypwuid(atoi(argv[i]));
	else
	    mypwd[i] = mypwnam(argv[i]);
	if (mypwd[i] == 0)
	    msg_fatal("%s: not found", argv[i]);
	msg_info("+ %s link=%d used=%d used=%d", argv[i], mypwd[i]->refcount,
		 mypwcache_name->used, mypwcache_uid->used);
    }
    for (i = 1; i < argc; i++) {
	msg_info("- %s link=%d used=%d used=%d", argv[i], mypwd[i]->refcount,
		 mypwcache_name->used, mypwcache_uid->used);
	mypwfree(mypwd[i]);
    }

    myfree((char *) mypwd);
    return (0);
}

#endif
