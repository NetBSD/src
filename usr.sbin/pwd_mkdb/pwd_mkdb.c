/*	$NetBSD: pwd_mkdb.c,v 1.20 2000/08/08 12:08:17 ad Exp $	*/

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
__RCSID("$NetBSD: pwd_mkdb.c,v 1.20 2000/08/08 12:08:17 ad Exp $");
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
#define	PERM_INSECURE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define	PERM_SECURE	(S_IRUSR | S_IWUSR)

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

static enum { FILE_INSECURE, FILE_SECURE, FILE_ORIG } clean;
static char *pname;				/* password file name */
static char prefix[MAXPATHLEN];
static char oldpwdfile[MAX(MAXPATHLEN, LINE_MAX * 2)];
static char pwd_db_tmp[MAX(MAXPATHLEN, LINE_MAX * 2)];
static char pwd_Sdb_tmp[MAX(MAXPATHLEN, LINE_MAX * 2)];
static int lorder = BYTE_ORDER;

void	cleanup(void);
void	error(const char *);
void	wr_error(const char *);
int	main(int, char **);
void	install(const char *, const char *);
void	rm(const char *);
int	scan(FILE *, struct passwd *, int *, int *);
void	usage(void);
void	putdbent(DB *, struct passwd *, const char *, int, const char *, int);
void	putyptoken(DB *dp, const char *fn);

int
main(int argc, char *argv[])
{
	DB *dp, *edp;
	FILE *fp, *oldfp;
	sigset_t set;
	int ch, makeold, tfd, hasyp, flags, lineno;
	struct passwd pwd;

	hasyp = 0;
	oldfp = NULL;
	strcpy(prefix, "/");
	makeold = 0;
	
	while ((ch = getopt(argc, argv, "d:pvBL")) != -1)
		switch(ch) {
		case 'd':			/* set prefix */
			strncpy(prefix, optarg, sizeof(prefix));
			prefix[sizeof(prefix)-1] = '\0';
			break;
		case 'p':			/* create V7 "file.orig" */
			makeold = 1;
			break;
		case 'v':			/* backward compatible */
			break;
		case 'B':			/* big-endian output */
			lorder = BIG_ENDIAN;
			break;
		case 'L':			/* little-endian output */
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
	if ((fp = fopen(pname, "r")) == NULL)
		error(pname);

	openinfo.lorder = lorder;

	/* Open the temporary insecure password database. */
	(void)snprintf(pwd_db_tmp, sizeof(pwd_db_tmp), "%s%s.tmp", prefix,
	    _PATH_MP_DB);
	dp = dbopen(pwd_db_tmp, O_RDWR | O_CREAT | O_EXCL, PERM_INSECURE,
	    DB_HASH, &openinfo);
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
		if ((tfd = open(oldpwdfile, O_WRONLY | O_CREAT | O_EXCL,
		    PERM_INSECURE)) < 0)
			error(oldpwdfile);
		if ((oldfp = fdopen(tfd, "w")) == NULL)
			error(oldpwdfile);
		clean = FILE_ORIG;
	}

	/*
	 * If we see something go by that looks like YP, we save a special
	 * pointer record, which if YP is enabled in the C lib, will speed
	 * things up.
	 */
	for (lineno = 0; scan(fp, &pwd, &flags, &lineno);) {
		/* look like YP? */
		if (pwd.pw_name[0] == '+' || pwd.pw_name[0] == '-')
			hasyp++;

		/*
		 * Warn about potentially unsafe uid/gid overrides.
		 */
		if (pwd.pw_name[0] == '+') {
			if ((flags & _PASSWORD_NOUID) == 0 && pwd.pw_uid == 0)
				warnx(
				"line %d: superuser override in YP inclusion",
				    lineno);
			if ((flags & _PASSWORD_NOGID) == 0 && pwd.pw_gid == 0)
				warnx(
				    "line %d: wheel override in YP inclusion",
				    lineno);
		}

		putdbent(dp, &pwd, "*", flags, pwd_db_tmp, lineno);

		/* Create original format password file entry */
		if (makeold) {
			(void)fprintf(oldfp, "%s:*:%d:%d:%s:%s:%s\n",
			    pwd.pw_name, pwd.pw_uid, pwd.pw_gid, pwd.pw_gecos,
			    pwd.pw_dir, pwd.pw_shell);
			if (ferror(oldfp))
				wr_error(oldpwdfile);
		}
	}

	/* Store YP token, if needed. */
	if (hasyp)
		putyptoken(dp, pwd_db_tmp);

	if ((dp->close)(dp) < 0)
		wr_error(pwd_db_tmp);
	if (makeold) {
		if (fflush(oldfp) == EOF)
			wr_error(oldpwdfile);
		if (fclose(oldfp) == EOF)
			wr_error(oldpwdfile);
	}

	/* Open the temporary encrypted password database. */
	(void)snprintf(pwd_Sdb_tmp, sizeof(pwd_Sdb_tmp), "%s%s.tmp", prefix,
		_PATH_SMP_DB);
	edp = dbopen(pwd_Sdb_tmp, O_RDWR | O_CREAT | O_EXCL, PERM_SECURE,
	   DB_HASH, &openinfo);
	if (!edp)
		error(pwd_Sdb_tmp);
	clean = FILE_SECURE;

	rewind(fp);
	for (lineno = 0; scan(fp, &pwd, &flags, &lineno);)
		putdbent(edp, &pwd, pwd.pw_passwd, flags, pwd_Sdb_tmp,
		    lineno);

	/* Store YP token, if needed. */
	if (hasyp)
		putyptoken(edp, pwd_Sdb_tmp);

	if ((edp->close)(edp) < 0)
		wr_error(_PATH_SMP_DB);

	/* Set master.passwd permissions, in case caller forgot. */
	(void)fchmod(fileno(fp), S_IRUSR|S_IWUSR);
	if (fclose(fp) == EOF)
		wr_error(pname);

	/* Install as the real password files. */
	install(pwd_db_tmp, _PATH_MP_DB);
	install(pwd_Sdb_tmp, _PATH_SMP_DB);
	if (makeold)
		install(oldpwdfile, _PATH_PASSWD);

	/*
	 * Move the master password LAST -- chpass(1), passwd(1) and vipw(8)
	 * all use flock(2) on it to block other incarnations of themselves.
	 * The rename means that everything is unlocked, as the original
	 * file can no longer be accessed.
	 */
	install(pname, _PATH_MASTERPASSWD);
	exit(EXIT_SUCCESS);
	/* NOTREACHED */
}

int
scan(FILE *fp, struct passwd *pw, int *flags, int *lineno)
{
	static char line[LINE_MAX];
	char *p;
	int oflags;

	if (fgets(line, sizeof(line), fp) == NULL)
		return (0);
	(*lineno)++;

	/*
	 * ``... if I swallow anything evil, put your fingers down my
	 * throat...''
	 *	-- The Who
	 */
	if ((p = strchr(line, '\n')) == NULL) {
		warnx("line too long");
		errno = EFTYPE;	/* XXX */
		error(pname);
	}
	*p = '\0';
	if (strcmp(line, "+") == 0)
		strcpy(line, "+:::::::::");	/* pw_scan() can't handle "+" */
	oflags = 0;
	if (!pw_scan(line, pw, &oflags)) {
		warnx("at line #%d", *lineno);
		errno = EFTYPE;	/* XXX */
		error(pname);
	}
	*flags = oflags;

	return (1);
}

void
install(const char *from, const char *to)
{
	char buf[MAXPATHLEN];
	int sverrno;

	snprintf(buf, sizeof(buf), "%s%s", prefix, to);
	if (rename(from, buf)) {
		sverrno = errno;
		(void)snprintf(buf, sizeof(buf), "%s to %s", from, buf);
		errno = sverrno;
		error(buf);
	}
}

void
wr_error(const char *str)
{
	char errbuf[BUFSIZ];
	int sverrno;

	sverrno = errno;

	(void)snprintf(errbuf, sizeof(errbuf),
		"attempt to write %s failed", str);

	errno = sverrno;
	error(errbuf);
}

void
error(const char *str)
{

	warn("%s", str);
	cleanup();
#ifdef think_about_this_a_while_longer
	fprintf(stderr, "NOTE: possible inconsistencies between "
	    "text files and databases\n"
	    "re-run pwd_mkdb when you have fixed the problem.\n");
#endif
	exit(EXIT_FAILURE);
}

void
rm(const char *victim)
{

	if (unlink(victim) < 0)
		warn("unlink(%s)", victim);
}

void
cleanup(void)
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
		/* FALLTHROUGH */
	}
}

/*
 * Write entries to a database for a single user. 
 *
 * The databases actually contain three copies of the original data.  Each
 * password file entry is converted into a rough approximation of a ``struct
 * passwd'', with the strings placed inline.  This object is then stored as
 * the data for three separate keys.  The first key * is the pw_name field
 * prepended by the _PW_KEYBYNAME character.  The second key is the pw_uid
 * field prepended by the _PW_KEYBYUID character.  The third key is the line
 * number in the original file prepended by the _PW_KEYBYNUM character. 
 * (The special characters are prepended to ensure that the keys do not
 * collide.)
 */
#define	COMPACT(e)	for (t = e; (*p++ = *t++) != '\0';)

void
putdbent(DB *dp, struct passwd *pw, const char *passwd, int flags,
	 const char *fn, int lineno)
{
	struct passwd pwd;
	char buf[MAX(MAXPATHLEN, LINE_MAX * 2)], tbuf[1024], *p;
	DBT data, key;
	const char *t;
	u_int32_t x;
	int len;

	memcpy(&pwd, pw, sizeof(pwd));
	data.data = (u_char *)buf;
	key.data = (u_char *)tbuf;

	if (lorder != BYTE_ORDER) {
		M_32_SWAP(pwd.pw_uid);
		M_32_SWAP(pwd.pw_gid);
		M_32_SWAP(pwd.pw_change);
		M_32_SWAP(pwd.pw_expire);
	}

	/* Create insecure data. */
	p = buf;
	COMPACT(pwd.pw_name);
	COMPACT(passwd);
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
	x = flags;
	if (lorder != BYTE_ORDER)
		M_32_SWAP(x);
	memmove(p, &x, sizeof(x));
	p += sizeof(flags);
	data.size = p - buf;

	/* Store insecure by name. */
	tbuf[0] = _PW_KEYBYNAME;
	len = strlen(pwd.pw_name);
	memmove(tbuf + 1, pwd.pw_name, len);
	key.size = len + 1;
	if ((dp->put)(dp, &key, &data, R_NOOVERWRITE) == -1)
		wr_error(fn);
		
	/* Store insecure by number. */
	tbuf[0] = _PW_KEYBYNUM;
	x = lineno;
	if (lorder != BYTE_ORDER)
		M_32_SWAP(x);
	memmove(tbuf + 1, &x, sizeof(x));
	key.size = sizeof(x) + 1;
	if ((dp->put)(dp, &key, &data, R_NOOVERWRITE) == -1)
		wr_error(fn);

	/* Store insecure by uid. */
	tbuf[0] = _PW_KEYBYUID;
	memmove(tbuf + 1, &pwd.pw_uid, sizeof(pwd.pw_uid));
	key.size = sizeof(pwd.pw_uid) + 1;
	if ((dp->put)(dp, &key, &data, R_NOOVERWRITE) == -1)
		wr_error(fn);
}

void
putyptoken(DB *dp, const char *fn)
{
	DBT data, key;

	key.data = (u_char *)__yp_token;
	key.size = strlen(__yp_token);
	data.data = (u_char *)NULL;
	data.size = 0;

	if ((dp->put)(dp, &key, &data, R_NOOVERWRITE) == -1)
		wr_error(fn);
}

void
usage(void)
{

	(void)fprintf(stderr, "usage: pwd_mkdb [-pBL] [-d directory] file\n");
	exit(EXIT_FAILURE);
}
