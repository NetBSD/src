/*
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Portions Copyright(C) 1994, Jason Downs.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 2000\n\
	The NetBSD Foundation, Inc.  All rights reserved.\n\
Copyright (c) 1991, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
__SCCSID("from: @(#)pwd_mkdb.c	8.5 (Berkeley) 4/20/94");
__RCSID("$NetBSD: pwd_mkdb.c,v 1.18 2000/07/07 15:11:46 itojun Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>

#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#define	INSECURE	1
#define	SECURE		2
#define	PERM_INSECURE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define	PERM_SECURE	(S_IRUSR|S_IWUSR)

/* pull this out of the C library. */
extern const char __yp_token[];

HASHINFO openinfo = {
	4096,		/* bsize */
	32,		/* ffactor */
	256,		/* nelem */
	2048 * 1024,	/* cachesize */
	NULL,		/* hash() */
	0		/* lorder */
};

static enum state { FILE_INSECURE, FILE_SECURE, FILE_ORIG } clean;
static struct passwd pwd;			/* password structure */
static char *pname;				/* password file name */
static char prefix[MAXPATHLEN];
static char oldpwdfile[MAX(MAXPATHLEN, LINE_MAX * 2)];
static char pwd_db_tmp[MAX(MAXPATHLEN, LINE_MAX * 2)];
static char pwd_Sdb_tmp[MAX(MAXPATHLEN, LINE_MAX * 2)];

void	cleanup __P((void));
void	error __P((char *));
void	wr_error __P((char *));
int	main __P((int, char **));
void	mv __P((char *, char *));
void	rm __P((char *));
int	scan __P((FILE *, struct passwd *, u_int32_t *));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	DB *dp, *edp;
	DBT data, key;
	FILE *fp, *oldfp;
	sigset_t set;
	int ch, len, makeold, tfd;
	u_int32_t x, cnt, flags;
	char *p;
	const char *t;
	char buf[MAX(MAXPATHLEN, LINE_MAX * 2)], tbuf[1024];
	int hasyp = 0;
	DBT ypdata, ypkey;
	int lorder = BYTE_ORDER;

	oldfp = NULL;
	strcpy(prefix, "/");
	makeold = 0;
	while ((ch = getopt(argc, argv, "d:pvBL")) != -1)
		switch(ch) {
		case 'd':
			strncpy(prefix, optarg, sizeof(prefix));
			prefix[sizeof(prefix)-1] = '\0';
			break;
		case 'p':			/* create V7 "file.orig" */
			makeold = 1;
			break;
		case 'v':			/* backward compatible */
			break;
		case 'B':
			lorder = BIG_ENDIAN;
			break;
		case 'L':
			lorder = LITTLE_ENDIAN;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	/*
	 * This could be changed to allow the user to interrupt.
	 * Probably not worth the effort.
	 */
	sigemptyset(&set);
	sigaddset(&set, SIGTSTP);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGTERM);
	(void)sigprocmask(SIG_BLOCK, &set, (sigset_t *)NULL);

	/* We don't care what the user wants. */
	(void)umask(0);

	pname = *argv;
	/* Open the original password file */
	if (!(fp = fopen(pname, "r")))
		error(pname);

	openinfo.lorder = lorder;

	/* Open the temporary insecure password database. */
	(void)snprintf(pwd_db_tmp, sizeof(pwd_db_tmp), "%s%s.tmp", prefix,
		_PATH_MP_DB);
	dp = dbopen(pwd_db_tmp,
	    O_RDWR|O_CREAT|O_EXCL, PERM_INSECURE, DB_HASH, &openinfo);
	if (dp == NULL)
		error(pwd_db_tmp);
	clean = FILE_INSECURE;

	/*
	 * Open file for old password file.  Minor trickiness -- don't want to
	 * chance the file already existing, since someone (stupidly) might
	 * still be using this for permission checking.  So, open it first and
	 * fdopen the resulting fd.  The resulting file should be readable by
	 * everyone.
	 */
	if (makeold) {
		(void)snprintf(oldpwdfile, sizeof(oldpwdfile), "%s.orig",
			pname);
		if ((tfd = open(oldpwdfile,
		    O_WRONLY|O_CREAT|O_EXCL, PERM_INSECURE)) < 0)
			error(oldpwdfile);
		if ((oldfp = fdopen(tfd, "w")) == NULL)
			error(oldpwdfile);
		clean = FILE_ORIG;
	}

	/*
	 * The databases actually contain three copies of the original data.
	 * Each password file entry is converted into a rough approximation
	 * of a ``struct passwd'', with the strings placed inline.  This
	 * object is then stored as the data for three separate keys.  The
	 * first key * is the pw_name field prepended by the _PW_KEYBYNAME
	 * character.  The second key is the pw_uid field prepended by the
	 * _PW_KEYBYUID character.  The third key is the line number in the
	 * original file prepended by the _PW_KEYBYNUM character.  (The special
	 * characters are prepended to ensure that the keys do not collide.)
	 *
	 * If we see something go by that looks like YP, we save a special
	 * pointer record, which if YP is enabled in the C lib, will speed
	 * things up.
	 */
	data.data = (u_char *)buf;
	key.data = (u_char *)tbuf;
	for (cnt = 1; scan(fp, &pwd, &flags); ++cnt) {
#define	COMPACT(e)	t = e; while ((*p++ = *t++));

		/* look like YP? */
		if((pwd.pw_name[0] == '+') || (pwd.pw_name[0] == '-'))
			hasyp++;

		/*
		 * Warn about potentially unsafe uid/gid overrides.
		 */
		if (pwd.pw_name[0] == '+') {
			if ((flags & _PASSWORD_NOUID) == 0 && pwd.pw_uid == 0)
				warnx(
				"line %d: superuser override in YP inclusion",
				    cnt);
			if ((flags & _PASSWORD_NOGID) == 0 && pwd.pw_gid == 0)
				warnx("line %d: wheel override in YP inclusion",
				    cnt);
		}

		if (lorder != BYTE_ORDER) {
			M_32_SWAP(pwd.pw_uid);
			M_32_SWAP(pwd.pw_gid);
			M_32_SWAP(pwd.pw_change);
			M_32_SWAP(pwd.pw_expire);
			M_32_SWAP(flags);
		}

		/* Create insecure data. */
		p = buf;
		COMPACT(pwd.pw_name);
		COMPACT("*");
		memmove(p, &pwd.pw_uid, sizeof(pwd.pw_uid));
		p += sizeof(pwd.pw_uid);
		memmove(p, &pwd.pw_gid, sizeof(pwd.pw_gid));
		p += sizeof(pwd.pw_gid);
		memmove(p, &pwd.pw_change, sizeof(pwd.pw_change));
		p += sizeof(pwd.pw_change);
		COMPACT(pwd.pw_class);
		COMPACT(pwd.pw_gecos);
		COMPACT(pwd.pw_dir);
		COMPACT(pwd.pw_shell);
		memmove(p, &pwd.pw_expire, sizeof(pwd.pw_expire));
		p += sizeof(pwd.pw_expire);
		memmove(p, &flags, sizeof(flags));
		p += sizeof(flags);
		data.size = p - buf;

		/* Store insecure by name. */
		tbuf[0] = _PW_KEYBYNAME;
		len = strlen(pwd.pw_name);
		memmove(tbuf + 1, pwd.pw_name, len);
		key.size = len + 1;
		if ((dp->put)(dp, &key, &data, R_NOOVERWRITE) == -1)
			wr_error(pwd_db_tmp);

		/* Store insecure by number. */
		tbuf[0] = _PW_KEYBYNUM;
		x = cnt;
		if (lorder != BYTE_ORDER)
			M_32_SWAP(x);
		memmove(tbuf + 1, &x, sizeof(x));
		key.size = sizeof(x) + 1;
		if ((dp->put)(dp, &key, &data, R_NOOVERWRITE) == -1)
			wr_error(pwd_db_tmp);

		/* Store insecure by uid. */
		tbuf[0] = _PW_KEYBYUID;
		memmove(tbuf + 1, &pwd.pw_uid, sizeof(pwd.pw_uid));
		key.size = sizeof(pwd.pw_uid) + 1;
		if ((dp->put)(dp, &key, &data, R_NOOVERWRITE) == -1)
			wr_error(pwd_db_tmp);

		/* Create original format password file entry */
		if (makeold) {
			(void)fprintf(oldfp, "%s:*:%d:%d:%s:%s:%s\n",
			    pwd.pw_name, pwd.pw_uid, pwd.pw_gid, pwd.pw_gecos,
			    pwd.pw_dir, pwd.pw_shell);
			if (ferror(oldfp)) {
				wr_error(oldpwdfile);
			}
		}
	}

	/* Store YP token, if needed. */
	if(hasyp) {
		ypkey.data = (u_char *)__yp_token;
		ypkey.size = strlen(__yp_token);
		ypdata.data = (u_char *)NULL;
		ypdata.size = 0;

		if ((dp->put)(dp, &ypkey, &ypdata, R_NOOVERWRITE) == -1)
			wr_error(pwd_db_tmp);
	}

	if ((dp->close)(dp) < 0) {
		wr_error(pwd_db_tmp);
	}
	if (makeold) {
		if (fflush(oldfp) == EOF) {
			wr_error(oldpwdfile);
		}
		if (fclose(oldfp) == EOF) {
			wr_error(oldpwdfile);
		}
	}

	/* Open the temporary encrypted password database. */
	(void)snprintf(pwd_Sdb_tmp, sizeof(pwd_Sdb_tmp), "%s%s.tmp", prefix,
		_PATH_SMP_DB);
	edp = dbopen(pwd_Sdb_tmp,
	    O_RDWR|O_CREAT|O_EXCL, PERM_SECURE, DB_HASH, &openinfo);
	if (!edp)
		error(pwd_Sdb_tmp);
	clean = FILE_SECURE;

	rewind(fp);
	for (cnt = 1; scan(fp, &pwd, &flags); ++cnt) {

		if (lorder != BYTE_ORDER) {
			M_32_SWAP(pwd.pw_uid);
			M_32_SWAP(pwd.pw_gid);
			M_32_SWAP(pwd.pw_change);
			M_32_SWAP(pwd.pw_expire);
			M_32_SWAP(flags);
		}

		/* Create secure data. */
		p = buf;
		COMPACT(pwd.pw_name);
		COMPACT(pwd.pw_passwd);
		memmove(p, &pwd.pw_uid, sizeof(pwd.pw_uid));
		p += sizeof(pwd.pw_uid);
		memmove(p, &pwd.pw_gid, sizeof(pwd.pw_gid));
		p += sizeof(pwd.pw_gid);
		memmove(p, &pwd.pw_change, sizeof(pwd.pw_change));
		p += sizeof(pwd.pw_change);
		COMPACT(pwd.pw_class);
		COMPACT(pwd.pw_gecos);
		COMPACT(pwd.pw_dir);
		COMPACT(pwd.pw_shell);
		memmove(p, &pwd.pw_expire, sizeof(pwd.pw_expire));
		p += sizeof(pwd.pw_expire);
		memmove(p, &flags, sizeof(flags));
		p += sizeof(flags);
		data.size = p - buf;

		/* Store secure by name. */
		tbuf[0] = _PW_KEYBYNAME;
		len = strlen(pwd.pw_name);
		memmove(tbuf + 1, pwd.pw_name, len);
		key.size = len + 1;
		if ((edp->put)(edp, &key, &data, R_NOOVERWRITE) == -1)
			wr_error(pwd_Sdb_tmp);

		/* Store secure by number. */
		tbuf[0] = _PW_KEYBYNUM;
		x = cnt;
		if (lorder != BYTE_ORDER)
			M_32_SWAP(x);
		memmove(tbuf + 1, &x, sizeof(x));
		key.size = sizeof(x) + 1;
		if ((edp->put)(edp, &key, &data, R_NOOVERWRITE) == -1)
			wr_error(pwd_Sdb_tmp);

		/* Store secure by uid. */
		tbuf[0] = _PW_KEYBYUID;
		memmove(tbuf + 1, &pwd.pw_uid, sizeof(pwd.pw_uid));
		key.size = sizeof(pwd.pw_uid) + 1;
		if ((edp->put)(edp, &key, &data, R_NOOVERWRITE) == -1)
			wr_error(pwd_Sdb_tmp);
	}

	/* Store YP token, if needed. */
	if(hasyp) {
		ypkey.data = (u_char *)__yp_token;
		ypkey.size = strlen(__yp_token);
		ypdata.data = (u_char *)NULL;
		ypdata.size = 0;

		if((edp->put)(edp, &ypkey, &ypdata, R_NOOVERWRITE) == -1)
			wr_error(pwd_Sdb_tmp);
	}

	if ((edp->close)(edp) < 0) {
		wr_error(_PATH_SMP_DB);
	}

	/* Set master.passwd permissions, in case caller forgot. */
	(void)fchmod(fileno(fp), S_IRUSR|S_IWUSR);
	if (fclose(fp) == EOF) {
		wr_error(pname);
	}

	/* Install as the real password files. */
	{
		char	destination[MAXPATHLEN];

		(void)snprintf(destination, sizeof(destination),
			"%s%s", prefix, _PATH_MP_DB);
		mv(pwd_db_tmp, destination);

		(void)snprintf(destination, sizeof(destination),
			"%s%s", prefix, _PATH_SMP_DB);
		mv(pwd_Sdb_tmp, destination);

		if (makeold) {
			(void)snprintf(destination, sizeof(destination),
				"%s%s", prefix, _PATH_PASSWD);
			mv(oldpwdfile, destination);
		}

		/*
		 * Move the master password LAST -- chpass(1),
		 * passwd(1) and vipw(8) all use flock(2) on it to
		 * block other incarnations of themselves. The rename
		 * means that everything is unlocked, as the original
		 * file can no longer be accessed.
		 */
		(void)snprintf(destination, sizeof(destination),
			"%s%s", prefix, _PATH_MASTERPASSWD);
		mv(pname, destination);
	}
	exit(0);
}

int
scan(fp, pw, flags)
	FILE *fp;
	struct passwd *pw;
	u_int32_t *flags;
{
	static int lcnt;
	static char line[LINE_MAX];
	char *p;
	int oflags;

	if (!fgets(line, sizeof(line), fp))
		return (0);
	++lcnt;
	/*
	 * ``... if I swallow anything evil, put your fingers down my
	 * throat...''
	 *	-- The Who
	 */
	if (!(p = strchr(line, '\n'))) {
		warnx("line too long");
		goto fmt;

	}
	*p = '\0';
	if (strcmp(line, "+") == 0)
		strcpy(line, "+:::::::::");	/* pw_scan() can't handle "+" */
	oflags = 0;
	if (!pw_scan(line, pw, &oflags)) {
		warnx("at line #%d", lcnt);
fmt:		errno = EFTYPE;	/* XXX */
		error(pname);
	}
	*flags = oflags;

	return (1);
}

void
mv(from, to)
	char *from, *to;
{
	char buf[MAXPATHLEN];

	if (rename(from, to)) {
		int sverrno = errno;
		(void)snprintf(buf, sizeof(buf), "%s to %s", from, to);
		errno = sverrno;
		error(buf);
	}
}

void
wr_error(name)
	char *name;
{
	char	errbuf[BUFSIZ];
	int	sverrno = errno;

	(void)snprintf(errbuf, sizeof(errbuf),
		"attempt to write %s failed", name);

	errno = sverrno;
	error(errbuf);
}

void
error(name)
	char *name;
{

	warn("%s", name);
	cleanup();
#ifdef think_about_this_a_while_longer
	fputs("NOTE: possible inconsistencies between text files and databases\n", stderr);
	fputs("re-run pwd_mkdb when you have fixed the problem.\n", stderr);
#endif
	exit(1);
}

void
rm(victim)
	char *victim;
{
	if (unlink(victim) < 0) {
		warn("unlink(%s)", victim);
	}
}

void
cleanup()
{
	switch(clean) {
	case FILE_ORIG:
		rm(oldpwdfile);
		/* FALLTHROUGH */
	case FILE_SECURE:
		rm(pwd_Sdb_tmp);
		/* FALLTHROUGH */
	case FILE_INSECURE:
		rm(pwd_db_tmp);
	}
}

void
usage()
{

	(void)fprintf(stderr, "usage: pwd_mkdb [-pBL] [-d directory] file\n");
	exit(1);
}
