/*	$NetBSD: function.c,v 1.38 2001/09/21 07:11:33 enami Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Cimarron D. Taylor of the University of California, Berkeley.
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
#if 0
static char sccsid[] = "from: @(#)function.c	8.10 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: function.c,v 1.38 2001/09/21 07:11:33 enami Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mount.h>

#include <err.h>
#include <errno.h>
#include <fnmatch.h>
#include <fts.h>
#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tzfile.h>
#include <unistd.h>

#include "find.h"
#include "stat_flags.h"

#define	COMPARE(a, b) {							\
	switch (plan->flags) {						\
	case F_EQUAL:							\
		return (a == b);					\
	case F_LESSTHAN:						\
		return (a < b);						\
	case F_GREATER:							\
		return (a > b);						\
	default:							\
		abort();						\
	}								\
}

static	int64_t	find_parsenum __P((PLAN *, char *, char *, char *));
	int	f_always_true __P((PLAN *, FTSENT *));
	int	f_amin __P((PLAN *, FTSENT *));
	int	f_atime __P((PLAN *, FTSENT *));
	int	f_cmin __P((PLAN *, FTSENT *));
	int	f_ctime __P((PLAN *, FTSENT *));
	int	f_exec __P((PLAN *, FTSENT *));
	int	f_flags __P((PLAN *, FTSENT *));
	int	f_fstype __P((PLAN *, FTSENT *));
	int	f_group __P((PLAN *, FTSENT *));
	int	f_inum __P((PLAN *, FTSENT *));
	int	f_links __P((PLAN *, FTSENT *));
	int	f_ls __P((PLAN *, FTSENT *));
	int	f_mmin __P((PLAN *, FTSENT *));
	int	f_mtime __P((PLAN *, FTSENT *));
	int	f_name __P((PLAN *, FTSENT *));
	int	f_newer __P((PLAN *, FTSENT *));
	int	f_nogroup __P((PLAN *, FTSENT *));
	int	f_nouser __P((PLAN *, FTSENT *));
	int	f_path __P((PLAN *, FTSENT *));
	int	f_perm __P((PLAN *, FTSENT *));
	int	f_print __P((PLAN *, FTSENT *));
	int	f_print0 __P((PLAN *, FTSENT *));
	int	f_printx __P((PLAN *, FTSENT *));
	int	f_prune __P((PLAN *, FTSENT *));
	int	f_regex __P((PLAN *, FTSENT *));
	int	f_size __P((PLAN *, FTSENT *));
	int	f_type __P((PLAN *, FTSENT *));
	int	f_user __P((PLAN *, FTSENT *));
	int	f_not __P((PLAN *, FTSENT *));
	int	f_or __P((PLAN *, FTSENT *));
static	PLAN   *c_regex_common __P((char ***, int, enum ntype, int));
static	PLAN   *palloc __P((enum ntype, int (*) __P((PLAN *, FTSENT *))));

extern int dotfd;
extern FTS *tree;
extern time_t now;

/*
 * find_parsenum --
 *	Parse a string of the form [+-]# and return the value.
 */
static int64_t
find_parsenum(plan, option, vp, endch)
	PLAN *plan;
	char *option, *vp, *endch;
{
	int64_t value;
	char *endchar, *str;	/* Pointer to character ending conversion. */
    
	/* Determine comparison from leading + or -. */
	str = vp;
	switch (*str) {
	case '+':
		++str;
		plan->flags = F_GREATER;
		break;
	case '-':
		++str;
		plan->flags = F_LESSTHAN;
		break;
	default:
		plan->flags = F_EQUAL;
		break;
	}
    
	/*
	 * Convert the string with strtol().  Note, if strtol() returns zero
	 * and endchar points to the beginning of the string we know we have
	 * a syntax error.
	 */
	value = strtoq(str, &endchar, 10);
	if (value == 0 && endchar == str)
		errx(1, "%s: %s: illegal numeric value", option, vp);
	if (endchar[0] && (endch == NULL || endchar[0] != *endch))
		errx(1, "%s: %s: illegal trailing character", option, vp);
	if (endch)
		*endch = endchar[0];
	return (value);
}

/*
 * The value of n for the inode times (atime, ctime, and mtime) is a range,
 * i.e. n matches from (n - 1) to n 24 hour periods.  This interacts with
 * -n, such that "-mtime -1" would be less than 0 days, which isn't what the
 * user wanted.  Correct so that -1 is "less than 1".
 */
#define	TIME_CORRECT(p, ttype)						\
	if ((p)->type == ttype && (p)->flags == F_LESSTHAN)		\
		++((p)->t_data);

/*
 * -amin n functions --
 *
 *	True if the difference between the file access time and the
 *	current time is n 1 minute periods.
 */
int
f_amin(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	COMPARE((now - entry->fts_statp->st_atime +
	    SECSPERMIN - 1) / SECSPERMIN, plan->t_data);
}
 
PLAN *
c_amin(argvp, isok)
	char ***argvp;
	int isok;
{
	char *arg = **argvp;
	PLAN *new;

	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	new = palloc(N_AMIN, f_amin);
	new->t_data = find_parsenum(new, "-amin", arg, NULL);
	TIME_CORRECT(new, N_AMIN);
	return (new);
}
/*
 * -atime n functions --
 *
 *	True if the difference between the file access time and the
 *	current time is n 24 hour periods.
 */
int
f_atime(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	COMPARE((now - entry->fts_statp->st_atime +
	    SECSPERDAY - 1) / SECSPERDAY, plan->t_data);
}
 
PLAN *
c_atime(argvp, isok)
	char ***argvp;
	int isok;
{
	char *arg = **argvp;
	PLAN *new;

	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	new = palloc(N_ATIME, f_atime);
	new->t_data = find_parsenum(new, "-atime", arg, NULL);
	TIME_CORRECT(new, N_ATIME);
	return (new);
}
/*
 * -cmin n functions --
 *
 *	True if the difference between the last change of file
 *	status information and the current time is n 24 hour periods.
 */
int
f_cmin(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	COMPARE((now - entry->fts_statp->st_ctime +
	    SECSPERMIN - 1) / SECSPERMIN, plan->t_data);
}
 
PLAN *
c_cmin(argvp, isok)
	char ***argvp;
	int isok;
{
	char *arg = **argvp;
	PLAN *new;

	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	new = palloc(N_CMIN, f_cmin);
	new->t_data = find_parsenum(new, "-cmin", arg, NULL);
	TIME_CORRECT(new, N_CMIN);
	return (new);
}
/*
 * -ctime n functions --
 *
 *	True if the difference between the last change of file
 *	status information and the current time is n 24 hour periods.
 */
int
f_ctime(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	COMPARE((now - entry->fts_statp->st_ctime +
	    SECSPERDAY - 1) / SECSPERDAY, plan->t_data);
}
 
PLAN *
c_ctime(argvp, isok)
	char ***argvp;
	int isok;
{
	char *arg = **argvp;
	PLAN *new;

	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	new = palloc(N_CTIME, f_ctime);
	new->t_data = find_parsenum(new, "-ctime", arg, NULL);
	TIME_CORRECT(new, N_CTIME);
	return (new);
}

/*
 * -depth functions --
 *
 *	Always true, causes descent of the directory hierarchy to be done
 *	so that all entries in a directory are acted on before the directory
 *	itself.
 */
int
f_always_true(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	return (1);
}
 
PLAN *
c_depth(argvp, isok)
	char ***argvp;
	int isok;
{
	isdepth = 1;

	return (palloc(N_DEPTH, f_always_true));
}
 
/*
 * [-exec | -ok] utility [arg ... ] ; functions --
 *
 *	True if the executed utility returns a zero value as exit status.
 *	The end of the primary expression is delimited by a semicolon.  If
 *	"{}" occurs anywhere, it gets replaced by the current pathname.
 *	The current directory for the execution of utility is the same as
 *	the current directory when the find utility was started.
 *
 *	The primary -ok is different in that it requests affirmation of the
 *	user before executing the utility.
 */
int
f_exec(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	int cnt;
	pid_t pid;
	int status;

	for (cnt = 0; plan->e_argv[cnt]; ++cnt)
		if (plan->e_len[cnt])
			brace_subst(plan->e_orig[cnt], &plan->e_argv[cnt],
			    entry->fts_path, &plan->e_len[cnt]);

	if (plan->flags == F_NEEDOK && !queryuser(plan->e_argv))
		return (0);

	/* don't mix output of command with find output */
	fflush(stdout);
	fflush(stderr);

	switch (pid = vfork()) {
	case -1:
		err(1, "vfork");
		/* NOTREACHED */
	case 0:
		if (fchdir(dotfd)) {
			warn("chdir");
			_exit(1);
		}
		execvp(plan->e_argv[0], plan->e_argv);
		warn("%s", plan->e_argv[0]);
		_exit(1);
	}
	pid = waitpid(pid, &status, 0);
	return (pid != -1 && WIFEXITED(status) && !WEXITSTATUS(status));
}
 
/*
 * c_exec --
 *	build three parallel arrays, one with pointers to the strings passed
 *	on the command line, one with (possibly duplicated) pointers to the
 *	argv array, and one with integer values that are lengths of the
 *	strings, but also flags meaning that the string has to be massaged.
 */
PLAN *
c_exec(argvp, isok)
	char ***argvp;
	int isok;
{
	PLAN *new;			/* node returned */
	int cnt;
	char **argv, **ap, *p;

	isoutput = 1;
    
	new = palloc(N_EXEC, f_exec);
	if (isok)
		new->flags = F_NEEDOK;

	for (ap = argv = *argvp;; ++ap) {
		if (!*ap)
			errx(1,
			    "%s: no terminating \";\"", isok ? "-ok" : "-exec");
		if (**ap == ';')
			break;
	}

	cnt = ap - *argvp + 1;
	new->e_argv = (char **)emalloc((u_int)cnt * sizeof(char *));
	new->e_orig = (char **)emalloc((u_int)cnt * sizeof(char *));
	new->e_len = (int *)emalloc((u_int)cnt * sizeof(int));

	for (argv = *argvp, cnt = 0; argv < ap; ++argv, ++cnt) {
		new->e_orig[cnt] = *argv;
		for (p = *argv; *p; ++p)
			if (p[0] == '{' && p[1] == '}') {
				new->e_argv[cnt] = emalloc((u_int)MAXPATHLEN);
				new->e_len[cnt] = MAXPATHLEN;
				break;
			}
		if (!*p) {
			new->e_argv[cnt] = *argv;
			new->e_len[cnt] = 0;
		}
	}
	new->e_argv[cnt] = new->e_orig[cnt] = NULL;

	*argvp = argv + 1;
	return (new);
}
 
/*
 * -flags [-]flags functions --
 */
int
f_flags(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	u_int32_t flags;

	flags = entry->fts_statp->st_flags;
	if (plan->flags == F_ATLEAST)
		return ((plan->f_data | flags) == flags);
	else
		return (flags == plan->f_data);
	/* NOTREACHED */
}
 
PLAN *
c_flags(argvp, isok)
	char ***argvp;
	int isok;
{
	char *flags = **argvp;
	PLAN *new;
	u_long flagset;

	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	new = palloc(N_FLAGS, f_flags);

	if (*flags == '-') {
		new->flags = F_ATLEAST;
		++flags;
	}

	flagset = 0;
	if ((strcmp(flags, "none") != 0) &&
	    (string_to_flags(&flags, &flagset, NULL) != 0))
		errx(1, "-flags: %s: illegal flags string", flags);
	new->f_data = flagset;
	return (new);
}

 
/*
 * -follow functions --
 *
 *	Always true, causes symbolic links to be followed on a global
 *	basis.
 */
PLAN *
c_follow(argvp, isok)
	char ***argvp;
	int isok;
{
	ftsoptions &= ~FTS_PHYSICAL;
	ftsoptions |= FTS_LOGICAL;

	return (palloc(N_FOLLOW, f_always_true));
}
 
/*
 * -fstype functions --
 *
 *	True if the file is of a certain type.
 */
int
f_fstype(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	static dev_t curdev;	/* need a guaranteed illegal dev value */
	static int first = 1;
	struct statfs sb;
	static short val;
	static char fstype[MFSNAMELEN];
	char *p, save[2];

	/* Only check when we cross mount point. */
	if (first || curdev != entry->fts_statp->st_dev) {
		curdev = entry->fts_statp->st_dev;

		/*
		 * Statfs follows symlinks; find wants the link's file system,
		 * not where it points.
		 */
		if (entry->fts_info == FTS_SL ||
		    entry->fts_info == FTS_SLNONE) {
			if ((p = strrchr(entry->fts_accpath, '/')) != NULL)
				++p;
			else
				p = entry->fts_accpath;
			save[0] = p[0];
			p[0] = '.';
			save[1] = p[1];
			p[1] = '\0';
			
		} else 
			p = NULL;

		if (statfs(entry->fts_accpath, &sb))
			err(1, "%s", entry->fts_accpath);

		if (p) {
			p[0] = save[0];
			p[1] = save[1];
		}

		first = 0;

		/*
		 * Further tests may need both of these values, so
		 * always copy both of them.
		 */
		val = sb.f_flags;
		strncpy(fstype, sb.f_fstypename, MFSNAMELEN);
	}
	switch (plan->flags) {
	case F_MTFLAG:
		return (val & plan->mt_data);	
	case F_MTTYPE:
		return (strncmp(fstype, plan->c_data, MFSNAMELEN) == 0);
	default:
		abort();
	}
}
 
PLAN *
c_fstype(argvp, isok)
	char ***argvp;
	int isok;
{
	char *arg = **argvp;
	PLAN *new;
    
	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;
    
	new = palloc(N_FSTYPE, f_fstype);

	switch (*arg) {
	case 'l':
		if (!strcmp(arg, "local")) {
			new->flags = F_MTFLAG;
			new->mt_data = MNT_LOCAL;
			return (new);
		}
		break;
	case 'r':
		if (!strcmp(arg, "rdonly")) {
			new->flags = F_MTFLAG;
			new->mt_data = MNT_RDONLY;
			return (new);
		}
		break;
	}

	new->flags = F_MTTYPE;
	new->c_data = arg;
	return (new);
}
 
/*
 * -group gname functions --
 *
 *	True if the file belongs to the group gname.  If gname is numeric and
 *	an equivalent of the getgrnam() function does not return a valid group
 *	name, gname is taken as a group ID.
 */
int
f_group(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	return (entry->fts_statp->st_gid == plan->g_data);
}
 
PLAN *
c_group(argvp, isok)
	char ***argvp;
	int isok;
{
	char *gname = **argvp;
	PLAN *new;
	struct group *g;
	gid_t gid;
    
	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	g = getgrnam(gname);
	if (g == NULL) {
		gid = atoi(gname);
		if (gid == 0 && gname[0] != '0')
			errx(1, "-group: %s: no such group", gname);
	} else
		gid = g->gr_gid;
    
	new = palloc(N_GROUP, f_group);
	new->g_data = gid;
	return (new);
}

/*
 * -inum n functions --
 *
 *	True if the file has inode # n.
 */
int
f_inum(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	COMPARE(entry->fts_statp->st_ino, plan->i_data);
}
 
PLAN *
c_inum(argvp, isok)
	char ***argvp;
	int isok;
{
	char *arg = **argvp;
	PLAN *new;
    
	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;
    
	new = palloc(N_INUM, f_inum);
	new->i_data = find_parsenum(new, "-inum", arg, NULL);
	return (new);
}
 
/*
 * -links n functions --
 *
 *	True if the file has n links.
 */
int
f_links(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	COMPARE(entry->fts_statp->st_nlink, plan->l_data);
}
 
PLAN *
c_links(argvp, isok)
	char ***argvp;
	int isok;
{
	char *arg = **argvp;
	PLAN *new;
    
	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;
    
	new = palloc(N_LINKS, f_links);
	new->l_data = (nlink_t)find_parsenum(new, "-links", arg, NULL);
	return (new);
}
 
/*
 * -ls functions --
 *
 *	Always true - prints the current entry to stdout in "ls" format.
 */
int
f_ls(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	printlong(entry->fts_path, entry->fts_accpath, entry->fts_statp);
	return (1);
}
 
PLAN *
c_ls(argvp, isok)
	char ***argvp;
	int isok;
{

	ftsoptions &= ~FTS_NOSTAT;
	isoutput = 1;
    
	return (palloc(N_LS, f_ls));
}

/*
 * -mmin n functions --
 *
 *	True if the difference between the file modification time and the
 *	current time is n 24 hour periods.
 */
int
f_mmin(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	COMPARE((now - entry->fts_statp->st_mtime + SECSPERMIN - 1) /
	    SECSPERMIN, plan->t_data);
}
 
PLAN *
c_mmin(argvp, isok)
	char ***argvp;
	int isok;
{
	char *arg = **argvp;
	PLAN *new;

	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	new = palloc(N_MMIN, f_mmin);
	new->t_data = find_parsenum(new, "-mmin", arg, NULL);
	TIME_CORRECT(new, N_MMIN);
	return (new);
}
/*
 * -mtime n functions --
 *
 *	True if the difference between the file modification time and the
 *	current time is n 24 hour periods.
 */
int
f_mtime(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	COMPARE((now - entry->fts_statp->st_mtime + SECSPERDAY - 1) /
	    SECSPERDAY, plan->t_data);
}
 
PLAN *
c_mtime(argvp, isok)
	char ***argvp;
	int isok;
{
	char *arg = **argvp;
	PLAN *new;

	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	new = palloc(N_MTIME, f_mtime);
	new->t_data = find_parsenum(new, "-mtime", arg, NULL);
	TIME_CORRECT(new, N_MTIME);
	return (new);
}

/*
 * -name functions --
 *
 *	True if the basename of the filename being examined
 *	matches pattern using Pattern Matching Notation S3.14
 */
int
f_name(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	return (!fnmatch(plan->c_data, entry->fts_name, 0));
}
 
PLAN *
c_name(argvp, isok)
	char ***argvp;
	int isok;
{
	char *pattern = **argvp;
	PLAN *new;

	(*argvp)++;
	new = palloc(N_NAME, f_name);
	new->c_data = pattern;
	return (new);
}
 
/*
 * -newer file functions --
 *
 *	True if the current file has been modified more recently
 *	then the modification time of the file named by the pathname
 *	file.
 */
int
f_newer(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	return (entry->fts_statp->st_mtime > plan->t_data);
}
 
PLAN *
c_newer(argvp, isok)
	char ***argvp;
	int isok;
{
	char *filename = **argvp;
	PLAN *new;
	struct stat sb;
    
	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	if (stat(filename, &sb))
		err(1, "%s", filename);
	new = palloc(N_NEWER, f_newer);
	new->t_data = sb.st_mtime;
	return (new);
}
 
/*
 * -nogroup functions --
 *
 *	True if file belongs to a user ID for which the equivalent
 *	of the getgrnam() 9.2.1 [POSIX.1] function returns NULL.
 */
int
f_nogroup(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	return (group_from_gid(entry->fts_statp->st_gid, 1) ? 0 : 1);
}
 
PLAN *
c_nogroup(argvp, isok)
	char ***argvp;
	int isok;
{
	ftsoptions &= ~FTS_NOSTAT;

	return (palloc(N_NOGROUP, f_nogroup));
}
 
/*
 * -nouser functions --
 *
 *	True if file belongs to a user ID for which the equivalent
 *	of the getpwuid() 9.2.2 [POSIX.1] function returns NULL.
 */
int
f_nouser(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	return (user_from_uid(entry->fts_statp->st_uid, 1) ? 0 : 1);
}
 
PLAN *
c_nouser(argvp, isok)
	char ***argvp;
	int isok;
{
	ftsoptions &= ~FTS_NOSTAT;

	return (palloc(N_NOUSER, f_nouser));
}
 
/*
 * -path functions --
 *
 *	True if the path of the filename being examined
 *	matches pattern using Pattern Matching Notation S3.14
 */
int
f_path(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	return (!fnmatch(plan->c_data, entry->fts_path, 0));
}
 
PLAN *
c_path(argvp, isok)
	char ***argvp;
	int isok;
{
	char *pattern = **argvp;
	PLAN *new;

	(*argvp)++;
	new = palloc(N_NAME, f_path);
	new->c_data = pattern;
	return (new);
}
 
/*
 * -perm functions --
 *
 *	The mode argument is used to represent file mode bits.  If it starts
 *	with a leading digit, it's treated as an octal mode, otherwise as a
 *	symbolic mode.
 */
int
f_perm(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	mode_t mode;

	mode = entry->fts_statp->st_mode &
	    (S_ISUID|S_ISGID|S_ISTXT|S_IRWXU|S_IRWXG|S_IRWXO);
	if (plan->flags == F_ATLEAST)
		return ((plan->m_data | mode) == mode);
	else
		return (mode == plan->m_data);
	/* NOTREACHED */
}
 
PLAN *
c_perm(argvp, isok)
	char ***argvp;
	int isok;
{
	char *perm = **argvp;
	PLAN *new;
	mode_t *set;

	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	new = palloc(N_PERM, f_perm);

	if (*perm == '-') {
		new->flags = F_ATLEAST;
		++perm;
	}

	if ((set = setmode(perm)) == NULL)
		err(1, "-perm: %s: illegal mode string", perm);

	new->m_data = getmode(set, 0);
	free(set);
	return (new);
}
 
/*
 * -print functions --
 *
 *	Always true, causes the current pathame to be written to
 *	standard output.
 */
int
f_print(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	(void)printf("%s\n", entry->fts_path);
	return (1);
}

int
f_print0(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	(void)fputs(entry->fts_path, stdout);
	(void)fputc('\0', stdout);
	return (1);
}

int
f_printx(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	char *cp;

	for (cp = entry->fts_path; *cp; cp++) {
		if (*cp == '\'' || *cp == '\"' || *cp == ' ' ||
		    *cp == '\t' || *cp == '\n' || *cp == '\\')
			fputc('\\', stdout);

		fputc(*cp, stdout);
	}

	fputc('\n', stdout);
	return (1);
}
 
PLAN *
c_print(argvp, isok)
	char ***argvp;
	int isok;
{

	isoutput = 1;

	return (palloc(N_PRINT, f_print));
}

PLAN *
c_print0(argvp, isok)
	char ***argvp;
	int isok;
{

	isoutput = 1;

	return (palloc(N_PRINT0, f_print0));
}

PLAN *
c_printx(argvp, isok)
	char ***argvp;
	int isok;
{

	isoutput = 1;

	return (palloc(N_PRINTX, f_printx));
}
 
/*
 * -prune functions --
 *
 *	Prune a portion of the hierarchy.
 */
int
f_prune(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	if (fts_set(tree, entry, FTS_SKIP))
		err(1, "%s", entry->fts_path);
	return (1);
}
 
PLAN *
c_prune(argvp, isok)
	char ***argvp;
	int isok;
{

	return (palloc(N_PRUNE, f_prune));
}

/*
 * -regex regexp (and related) functions --
 *
 *	True if the complete file path matches the regular expression regexp.
 *	For -regex, regexp is a case-sensitive (basic) regular expression.
 *	For -iregex, regexp is a case-insensitive (basic) regular expression.
 */
int
f_regex(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	return (regexec(&plan->regexp_data, entry->fts_path, 0, NULL, 0) == 0);
}
 
static PLAN *
c_regex_common(argvp, isok, type, regcomp_flags)
	char ***argvp;
	int isok, regcomp_flags;
	enum ntype type;
{
	char errbuf[LINE_MAX];
	regex_t reg;
	char *regexp = **argvp;
	char *lineregexp;
	PLAN *new;
	int rv;

	(*argvp)++;

	lineregexp = alloca(strlen(regexp) + 1 + 6);	/* max needed */
	sprintf(lineregexp, "^%s(%s%s)$",
	    (regcomp_flags & REG_EXTENDED) ? "" : "\\", regexp,
	    (regcomp_flags & REG_EXTENDED) ? "" : "\\");
	rv = regcomp(&reg, lineregexp, REG_NOSUB|regcomp_flags);
	if (rv != 0) {
		regerror(rv, &reg, errbuf, sizeof errbuf);
		errx(1, "regexp %s: %s", regexp, errbuf);
	}
	
	new = palloc(type, f_regex);
	new->regexp_data = reg;
	return (new);
}

PLAN *
c_regex(argvp, isok)
	char ***argvp;
	int isok;
{

	return (c_regex_common(argvp, isok, N_REGEX, REG_BASIC));
}

PLAN *
c_iregex(argvp, isok)
	char ***argvp;
	int isok;
{

	return (c_regex_common(argvp, isok, N_IREGEX, REG_BASIC|REG_ICASE));
}

/*
 * -size n[c] functions --
 *
 *	True if the file size in bytes, divided by an implementation defined
 *	value and rounded up to the next integer, is n.  If n is followed by
 *	a c, the size is in bytes.
 */
#define	FIND_SIZE	512
static int divsize = 1;

int
f_size(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	off_t size;

	size = divsize ? (entry->fts_statp->st_size + FIND_SIZE - 1) /
	    FIND_SIZE : entry->fts_statp->st_size;
	COMPARE(size, plan->o_data);
}
 
PLAN *
c_size(argvp, isok)
	char ***argvp;
	int isok;
{
	char *arg = **argvp;
	PLAN *new;
	char endch;
    
	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	new = palloc(N_SIZE, f_size);
	endch = 'c';
	new->o_data = find_parsenum(new, "-size", arg, &endch);
	if (endch == 'c')
		divsize = 0;
	return (new);
}
 
/*
 * -type c functions --
 *
 *	True if the type of the file is c, where c is b, c, d, p, f or w
 *	for block special file, character special file, directory, FIFO,
 *	regular file or whiteout respectively.
 */
int
f_type(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	return ((entry->fts_statp->st_mode & S_IFMT) == plan->m_data);
}
 
PLAN *
c_type(argvp, isok)
	char ***argvp;
	int isok;
{
	char *typestring = **argvp;
	PLAN *new;
	mode_t  mask = (mode_t)0;
    
	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	switch (typestring[0]) {
#ifdef S_IFWHT
      case 'W':
#ifdef FTS_WHITEOUT
	      ftsoptions |= FTS_WHITEOUT;
#endif
              mask = S_IFWHT;
              break;
#endif
	case 'b':
		mask = S_IFBLK;
		break;
	case 'c':
		mask = S_IFCHR;
		break;
	case 'd':
		mask = S_IFDIR;
		break;
	case 'f':
		mask = S_IFREG;
		break;
	case 'l':
		mask = S_IFLNK;
		break;
	case 'p':
		mask = S_IFIFO;
		break;
	case 's':
		mask = S_IFSOCK;
		break;
#ifdef FTS_WHITEOUT
	case 'w':
		mask = S_IFWHT;
		ftsoptions |= FTS_WHITEOUT;
		break;
#endif /* FTS_WHITEOUT */
	default:
		errx(1, "-type: %s: unknown type", typestring);
	}
    
	new = palloc(N_TYPE, f_type);
	new->m_data = mask;
	return (new);
}
 
/*
 * -user uname functions --
 *
 *	True if the file belongs to the user uname.  If uname is numeric and
 *	an equivalent of the getpwnam() S9.2.2 [POSIX.1] function does not
 *	return a valid user name, uname is taken as a user ID.
 */
int
f_user(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{

	return (entry->fts_statp->st_uid == plan->u_data);
}
 
PLAN *
c_user(argvp, isok)
	char ***argvp;
	int isok;
{
	char *username = **argvp;
	PLAN *new;
	struct passwd *p;
	uid_t uid;
    
	(*argvp)++;
	ftsoptions &= ~FTS_NOSTAT;

	p = getpwnam(username);
	if (p == NULL) {
		uid = atoi(username);
		if (uid == 0 && username[0] != '0')
			errx(1, "-user: %s: no such user", username);
	} else
		uid = p->pw_uid;

	new = palloc(N_USER, f_user);
	new->u_data = uid;
	return (new);
}
 
/*
 * -xdev functions --
 *
 *	Always true, causes find not to decend past directories that have a
 *	different device ID (st_dev, see stat() S5.6.2 [POSIX.1])
 */
PLAN *
c_xdev(argvp, isok)
	char ***argvp;
	int isok;
{
	ftsoptions |= FTS_XDEV;

	return (palloc(N_XDEV, f_always_true));
}

/*
 * ( expression ) functions --
 *
 *	True if expression is true.
 */
int
f_expr(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	PLAN *p;
	int state;

	state = 0;
	for (p = plan->p_data[0];
	    p && (state = (p->eval)(p, entry)); p = p->next);
	return (state);
}
 
/*
 * N_OPENPAREN and N_CLOSEPAREN nodes are temporary place markers.  They are
 * eliminated during phase 2 of find_formplan() --- the '(' node is converted
 * to a N_EXPR node containing the expression and the ')' node is discarded.
 */
PLAN *
c_openparen(argvp, isok)
	char ***argvp;
	int isok;
{

	return (palloc(N_OPENPAREN, (int (*) __P((PLAN *, FTSENT *)))-1));
}
 
PLAN *
c_closeparen(argvp, isok)
	char ***argvp;
	int isok;
{

	return (palloc(N_CLOSEPAREN, (int (*) __P((PLAN *, FTSENT *)))-1));
}
 
/*
 * ! expression functions --
 *
 *	Negation of a primary; the unary NOT operator.
 */
int
f_not(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	PLAN *p;
	int state;

	state = 0;
	for (p = plan->p_data[0];
	    p && (state = (p->eval)(p, entry)); p = p->next);
	return (!state);
}
 
PLAN *
c_not(argvp, isok)
	char ***argvp;
	int isok;
{

	return (palloc(N_NOT, f_not));
}
 
/*
 * expression -o expression functions --
 *
 *	Alternation of primaries; the OR operator.  The second expression is
 * not evaluated if the first expression is true.
 */
int
f_or(plan, entry)
	PLAN *plan;
	FTSENT *entry;
{
	PLAN *p;
	int state;

	state = 0;
	for (p = plan->p_data[0];
	    p && (state = (p->eval)(p, entry)); p = p->next);

	if (state)
		return (1);

	for (p = plan->p_data[1];
	    p && (state = (p->eval)(p, entry)); p = p->next);
	return (state);
}

PLAN *
c_or(argvp, isok)
	char ***argvp;
	int isok;
{

	return (palloc(N_OR, f_or));
}

PLAN *
c_null(argvp, isok)
	char ***argvp;
	int isok;
{

	return (NULL);
}

static PLAN *
palloc(t, f)
	enum ntype t;
	int (*f) __P((PLAN *, FTSENT *));
{
	PLAN *new;

	if ((new = malloc(sizeof(PLAN))) == NULL)
		err(1, NULL);
	new->type = t;
	new->eval = f;
	new->flags = 0;
	new->next = NULL;
	return (new);
}
