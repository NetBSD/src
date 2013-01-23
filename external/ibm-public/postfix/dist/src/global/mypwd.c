/*	$NetBSD: mypwd.c,v 1.1.1.1.10.1 2013/01/23 00:05:03 yamt Exp $	*/

/*++
/* NAME
/*	mypwd 3
/* SUMMARY
/*	caching getpwnam_r()/getpwuid_r()
/* SYNOPSIS
/*	#include <mypwd.h>
/*
/*	int	mypwuid_err(uid, pwd)
/*	uid_t	uid;
/*	struct mypasswd **pwd;
/*
/*	int	mypwnam_err(name, pwd)
/*	const char *name;
/*	struct mypasswd **pwd;
/*
/*	void	mypwfree(pwd)
/*	struct mypasswd *pwd;
/* BACKWARDS COMPATIBILITY
/*	struct mypasswd *mypwuid(uid)
/*	uid_t	uid;
/*
/*	struct mypasswd *mypwnam(name)
/*	const char *name;
/* DESCRIPTION
/*	This module maintains a reference-counted cache of password
/*	database lookup results. The idea is to avoid making repeated
/*	getpw*() calls for the same information.
/*
/*	mypwnam_err() and mypwuid_err() are wrappers that cache a
/*	private copy of results from the getpwnam_r() and getpwuid_r()
/*	library routines (on legacy systems: from getpwnam() and
/*	getpwuid(). Note: cache updates are not protected by mutex.
/*
/*	Results are shared between calls with the same \fIname\fR
/*	or \fIuid\fR argument, so changing results is verboten.
/*
/*	mypwnam() and mypwuid() are binary-compatibility wrappers
/*	for legacy applications.
/*
/*	mypwfree() cleans up the result of mypwnam*() and mypwuid*().
/* BUGS
/*	This module is security sensitive and complex at the same
/*	time, which is bad.
/* DIAGNOSTICS
/*	mypwnam_err() and mypwuid_err() return a non-zero system
/*	error code when the lookup could not be performed. They 
/*	return zero, plus a null struct mypasswd pointer, when the
/*	requested information was not found.
/*
/*	Fatal error: out of memory.
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
#include <unistd.h>
#include <string.h>
#ifdef USE_PATHS_H
#include <paths.h>
#endif
#include <errno.h>

/* Utility library. */

#include <mymalloc.h>
#include <htable.h>
#include <binhash.h>
#include <msg.h>

/* Global library. */

#include "mypwd.h"

 /*
  * Workaround: Solaris >= 2.5.1 provides two getpwnam_r() and getpwuid_r()
  * implementations. The default variant is compatible with historical
  * Solaris implementations. The non-default variant is POSIX-compliant.
  * 
  * To get the POSIX-compliant variant, we include the file <pwd.h> after
  * defining _POSIX_PTHREAD_SEMANTICS. We do this after all other includes,
  * so that we won't unexpectedly affect any other APIs.
  * 
  * This happens to work because nothing above this includes <pwd.h>, and
  * because of the specific way that Solaris redefines getpwnam_r() and
  * getpwuid_r() for POSIX compliance. We know the latter from peeking under
  * the hood. What we do is only marginally better than directly invoking
  * __posix_getpwnam_r() and __posix_getpwuid_r().
  */
#ifdef GETPW_R_NEEDS_POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS
#endif
#include <pwd.h>

 /*
  * The private cache. One for lookups by name, one for lookups by uid, and
  * one for the last looked up result. There is a minor problem: multiple
  * cache entries may have the same uid value, but the cache that is indexed
  * by uid can store only one entry per uid value.
  */
static HTABLE *mypwcache_name = 0;
static BINHASH *mypwcache_uid = 0;
static struct mypasswd *last_pwd;

 /*
  * XXX Solaris promises that we can determine the getpw*_r() buffer size by
  * calling sysconf(_SC_GETPW_R_SIZE_MAX). Many systems promise that they
  * will return an ERANGE error when the buffer is too small. However, not
  * all systems make such promises. Therefore, we settle for the dumbest
  * option: a large buffer. This is acceptable because the buffer is used
  * only for short-term storage.
  */
#ifdef HAVE_POSIX_GETPW_R
#define GETPW_R_BUFSIZ		1024
#endif
#define MYPWD_ERROR_DELAY	(30)

/* mypwenter - enter password info into cache */

static struct mypasswd *mypwenter(const struct passwd * pwd)
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

    /*
     * Avoid mypwcache_uid memory leak when multiple names have the same UID.
     * This makes the lookup result dependent on program history. But, it was
     * already history-dependent before we added this extra check.
     */
    htable_enter(mypwcache_name, mypwd->pw_name, (char *) mypwd);
    if (binhash_locate(mypwcache_uid, (char *) &mypwd->pw_uid,
		       sizeof(mypwd->pw_uid)) == 0)
	binhash_enter(mypwcache_uid, (char *) &mypwd->pw_uid,
		      sizeof(mypwd->pw_uid), (char *) mypwd);
    return (mypwd);
}

/* mypwuid - caching getpwuid() */

struct mypasswd *mypwuid(uid_t uid)
{
    struct mypasswd *mypwd;

    while ((errno = mypwuid_err(uid, &mypwd)) != 0) {
	msg_warn("getpwuid_r: %m");
	sleep(MYPWD_ERROR_DELAY);
    }
    return (mypwd);
}

/* mypwuid_err - caching getpwuid_r(), minus thread safety */

int     mypwuid_err(uid_t uid, struct mypasswd ** result)
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
	    *result = mypwd = last_pwd;
	    mypwd->refcount++;
	    return (0);
	}
    }

    /*
     * Find the info in the cache or in the password database.
     */
    if ((mypwd = (struct mypasswd *)
	 binhash_find(mypwcache_uid, (char *) &uid, sizeof(uid))) == 0) {
#ifdef HAVE_POSIX_GETPW_R
	char    pwstore[GETPW_R_BUFSIZ];
	struct passwd pwbuf;
	int     err;

	err = getpwuid_r(uid, &pwbuf, pwstore, sizeof(pwstore), &pwd);
	if (err != 0)
	    return (err);
	if (pwd == 0) {
	    *result = 0;
	    return (0);
	}
#else
	if ((pwd = getpwuid(uid)) == 0) {
	    *result = 0;
	    return (0);
	}
#endif
	mypwd = mypwenter(pwd);
    }
    *result = last_pwd = mypwd;
    mypwd->refcount += 2;
    return (0);
}

/* mypwnam - caching getpwnam() */

struct mypasswd *mypwnam(const char *name)
{
    struct mypasswd *mypwd;

    while ((errno = mypwnam_err(name, &mypwd)) != 0) {
	msg_warn("getpwnam_r: %m");
	sleep(MYPWD_ERROR_DELAY);
    }
    return (mypwd);
}

/* mypwnam_err - caching getpwnam_r(), minus thread safety */

int     mypwnam_err(const char *name, struct mypasswd ** result)
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
	    *result = mypwd = last_pwd;
	    mypwd->refcount++;
	    return (0);
	}
    }

    /*
     * Find the info in the cache or in the password database.
     */
    if ((mypwd = (struct mypasswd *) htable_find(mypwcache_name, name)) == 0) {
#ifdef HAVE_POSIX_GETPW_R
	char    pwstore[GETPW_R_BUFSIZ];
	struct passwd pwbuf;
	int     err;

	err = getpwnam_r(name, &pwbuf, pwstore, sizeof(pwstore), &pwd);
	if (err != 0)
	    return (err);
	if (pwd == 0) {
	    *result = 0;
	    return (0);
	}
#else
	if ((pwd = getpwnam(name)) == 0) {
	    *result = 0;
	    return (0);
	}
#endif
	mypwd = mypwenter(pwd);
    }
    *result = last_pwd = mypwd;
    mypwd->refcount += 2;
    return (0);
}

/* mypwfree - destroy password info */

void    mypwfree(struct mypasswd * mypwd)
{
    if (mypwd->refcount < 1)
	msg_panic("mypwfree: refcount %d", mypwd->refcount);

    /*
     * See mypwenter() for the reason behind the binhash_locate() test.
     */
    if (--mypwd->refcount == 0) {
	htable_delete(mypwcache_name, mypwd->pw_name, (void (*) (char *)) 0);
	if (binhash_locate(mypwcache_uid, (char *) &mypwd->pw_uid,
			   sizeof(mypwd->pw_uid)))
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

    mypwd = (struct mypasswd **) mymalloc((argc + 2) * sizeof(*mypwd));

    /*
     * Do a sequence of lookups.
     */
    for (i = 1; i < argc; i++) {
	if (ISDIGIT(argv[i][0]))
	    mypwd[i] = mypwuid(atoi(argv[i]));
	else
	    mypwd[i] = mypwnam(argv[i]);
	if (mypwd[i] == 0)
	    msg_fatal("%s: not found", argv[i]);
	msg_info("lookup %s %s/%d refcount=%d name_cache=%d uid_cache=%d",
		 argv[i], mypwd[i]->pw_name, mypwd[i]->pw_uid,
	     mypwd[i]->refcount, mypwcache_name->used, mypwcache_uid->used);
    }
    mypwd[argc] = last_pwd;

    /*
     * The following should free all entries.
     */
    for (i = 1; i < argc + 1; i++) {
	msg_info("free %s/%d refcount=%d name_cache=%d uid_cache=%d",
		 mypwd[i]->pw_name, mypwd[i]->pw_uid, mypwd[i]->refcount,
		 mypwcache_name->used, mypwcache_uid->used);
	mypwfree(mypwd[i]);
    }
    msg_info("name_cache=%d uid_cache=%d",
	     mypwcache_name->used, mypwcache_uid->used);

    myfree((char *) mypwd);
    return (0);
}

#endif
