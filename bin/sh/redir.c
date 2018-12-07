/*	$NetBSD: redir.c,v 1.57.2.2 2018/12/07 13:12:02 martin Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
static char sccsid[] = "@(#)redir.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: redir.c,v 1.57.2.2 2018/12/07 13:12:02 martin Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>	/* PIPE_BUF */
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * Code for dealing with input/output redirection.
 */

#include "main.h"
#include "builtins.h"
#include "shell.h"
#include "nodes.h"
#include "jobs.h"
#include "options.h"
#include "expand.h"
#include "redir.h"
#include "output.h"
#include "memalloc.h"
#include "mystring.h"
#include "error.h"


#define EMPTY -2		/* marks an unused slot in redirtab */
#define CLOSED -1		/* fd was not open before redir */
#ifndef PIPE_BUF
# define PIPESIZE 4096		/* amount of buffering in a pipe */
#else
# define PIPESIZE PIPE_BUF
#endif


MKINIT
struct renamelist {
	struct renamelist *next;
	int orig;
	int into;
};

MKINIT
struct redirtab {
	struct redirtab *next;
	struct renamelist *renamed;
};


MKINIT struct redirtab *redirlist;

/*
 * We keep track of whether or not fd0 has been redirected.  This is for
 * background commands, where we want to redirect fd0 to /dev/null only
 * if it hasn't already been redirected.
 */
STATIC int fd0_redirected = 0;

/*
 * And also where to put internal use fds that should be out of the
 * way of user defined fds (normally)
 */
STATIC int big_sh_fd = 0;

STATIC const struct renamelist *is_renamed(const struct renamelist *, int);
STATIC void fd_rename(struct redirtab *, int, int);
STATIC void free_rl(struct redirtab *, int);
STATIC void openredirect(union node *, char[10], int);
STATIC int openhere(const union node *);
STATIC int copyfd(int, int, int);
STATIC void find_big_fd(void);


struct shell_fds {		/* keep track of internal shell fds */
	struct shell_fds *nxt;
	void (*cb)(int, int);
	int fd;
};

STATIC struct shell_fds *sh_fd_list;

STATIC void renumber_sh_fd(struct shell_fds *);
STATIC struct shell_fds *sh_fd(int);

STATIC const struct renamelist *
is_renamed(const struct renamelist *rl, int fd)
{
	while (rl != NULL) {
		if (rl->orig == fd)
			return rl;
		rl = rl->next;
	}
	return NULL;
}

STATIC void
free_rl(struct redirtab *rt, int reset)
{
	struct renamelist *rl, *rn = rt->renamed;

	while ((rl = rn) != NULL) {
		rn = rl->next;
		if (rl->orig == 0)
			fd0_redirected--;
		if (reset) {
			if (rl->into < 0)
				close(rl->orig);
			else
				movefd(rl->into, rl->orig);
		}
		ckfree(rl);
	}
	rt->renamed = NULL;
}

STATIC void
fd_rename(struct redirtab *rt, int from, int to)
{
	struct renamelist *rl = ckmalloc(sizeof(struct renamelist));

	rl->next = rt->renamed;
	rt->renamed = rl;

	rl->orig = from;
	rl->into = to;
}

/*
 * Process a list of redirection commands.  If the REDIR_PUSH flag is set,
 * old file descriptors are stashed away so that the redirection can be
 * undone by calling popredir.  If the REDIR_BACKQ flag is set, then the
 * standard output, and the standard error if it becomes a duplicate of
 * stdout, is saved in memory.
 */

void
redirect(union node *redir, int flags)
{
	union node *n;
	struct redirtab *sv = NULL;
	int i;
	int fd;
	char memory[10];	/* file descriptors to write to memory */

	for (i = 10 ; --i >= 0 ; )
		memory[i] = 0;
	memory[1] = flags & REDIR_BACKQ;
	if (flags & REDIR_PUSH) {
		/* We don't have to worry about REDIR_VFORK here, as
		 * flags & REDIR_PUSH is never true if REDIR_VFORK is set.
		 */
		sv = ckmalloc(sizeof (struct redirtab));
		sv->renamed = NULL;
		sv->next = redirlist;
		redirlist = sv;
	}
	for (n = redir ; n ; n = n->nfile.next) {
		fd = n->nfile.fd;
		if (fd > max_user_fd)
			max_user_fd = fd;
		renumber_sh_fd(sh_fd(fd));
		if ((n->nfile.type == NTOFD || n->nfile.type == NFROMFD) &&
		    n->ndup.dupfd == fd) {
			/* redirect from/to same file descriptor */
			/* make sure it stays open */
			if (fcntl(fd, F_SETFD, 0) < 0)
				error("fd %d: %s", fd, strerror(errno));
			continue;
		}

		if ((flags & REDIR_PUSH) && !is_renamed(sv->renamed, fd)) {
			INTOFF;
			if (big_sh_fd < 10)
				find_big_fd();
			if ((i = fcntl(fd, F_DUPFD, big_sh_fd)) == -1) {
				switch (errno) {
				case EBADF:
					i = CLOSED;
					break;
				case EMFILE:
				case EINVAL:
					find_big_fd();
					i = fcntl(fd, F_DUPFD, big_sh_fd);
					if (i >= 0)
						break;
					/* FALLTHRU */
				default:
					i = errno;
					INTON;    /* XXX not needed here ? */
					error("%d: %s", fd, strerror(i));
					/* NOTREACHED */
				}
			}
			if (i >= 0)
				(void)fcntl(i, F_SETFD, FD_CLOEXEC);
			fd_rename(sv, fd, i);
			INTON;
		}
		if (fd == 0)
			fd0_redirected++;
		openredirect(n, memory, flags);
	}
	if (memory[1])
		out1 = &memout;
	if (memory[2])
		out2 = &memout;
}


STATIC void
openredirect(union node *redir, char memory[10], int flags)
{
	struct stat sb;
	int fd = redir->nfile.fd;
	char *fname;
	int f;
	int eflags, cloexec;

	/*
	 * We suppress interrupts so that we won't leave open file
	 * descriptors around.  This may not be such a good idea because
	 * an open of a device or a fifo can block indefinitely.
	 */
	INTOFF;
	if (fd < 10)
		memory[fd] = 0;
	switch (redir->nfile.type) {
	case NFROM:
		fname = redir->nfile.expfname;
		if (flags & REDIR_VFORK)
			eflags = O_NONBLOCK;
		else
			eflags = 0;
		if ((f = open(fname, O_RDONLY|eflags)) < 0)
			goto eopen;
		if (eflags)
			(void)fcntl(f, F_SETFL, fcntl(f, F_GETFL, 0) & ~eflags);
		break;
	case NFROMTO:
		fname = redir->nfile.expfname;
		if ((f = open(fname, O_RDWR|O_CREAT, 0666)) < 0)
			goto ecreate;
		break;
	case NTO:
		if (Cflag) {
			fname = redir->nfile.expfname;
			if ((f = open(fname, O_WRONLY)) == -1) {
				if ((f = open(fname, O_WRONLY|O_CREAT|O_EXCL,
				    0666)) < 0)
					goto ecreate;
			} else if (fstat(f, &sb) == -1) {
				int serrno = errno;
				close(f);
				errno = serrno;
				goto ecreate;
			} else if (S_ISREG(sb.st_mode)) {
				close(f);
				errno = EEXIST;
				goto ecreate;
			}
			break;
		}
		/* FALLTHROUGH */
	case NCLOBBER:
		fname = redir->nfile.expfname;
		if ((f = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)
			goto ecreate;
		break;
	case NAPPEND:
		fname = redir->nfile.expfname;
		if ((f = open(fname, O_WRONLY|O_CREAT|O_APPEND, 0666)) < 0)
			goto ecreate;
		break;
	case NTOFD:
	case NFROMFD:
		if (redir->ndup.dupfd >= 0) {	/* if not ">&-" */
			if (fd < 10 && redir->ndup.dupfd < 10 &&
			    memory[redir->ndup.dupfd])
				memory[fd] = 1;
			else if (copyfd(redir->ndup.dupfd, fd,
			    (flags & REDIR_KEEP) == 0) < 0)
				error("Redirect (from %d to %d) failed: %s",
				    redir->ndup.dupfd, fd, strerror(errno));
		} else
			(void) close(fd);
		INTON;
		return;
	case NHERE:
	case NXHERE:
		f = openhere(redir);
		break;
	default:
		abort();
	}

	cloexec = fd > 2 && (flags & REDIR_KEEP) == 0 && !posix;
	if (f != fd) {
		if (copyfd(f, fd, cloexec) < 0) {
			int e = errno;

			close(f);
			error("redirect reassignment (fd %d) failed: %s", fd,
			    strerror(e));
		}
		close(f);
	} else if (cloexec)
		(void)fcntl(f, F_SETFD, FD_CLOEXEC);

	INTON;
	return;
ecreate:
	exerrno = 1;
	error("cannot create %s: %s", fname, errmsg(errno, E_CREAT));
eopen:
	exerrno = 1;
	error("cannot open %s: %s", fname, errmsg(errno, E_OPEN));
}


/*
 * Handle here documents.  Normally we fork off a process to write the
 * data to a pipe.  If the document is short, we can stuff the data in
 * the pipe without forking.
 */

STATIC int
openhere(const union node *redir)
{
	int pip[2];
	int len = 0;

	if (pipe(pip) < 0)
		error("Pipe call failed");
	if (redir->type == NHERE) {
		len = strlen(redir->nhere.doc->narg.text);
		if (len <= PIPESIZE) {
			xwrite(pip[1], redir->nhere.doc->narg.text, len);
			goto out;
		}
	}
	if (forkshell(NULL, NULL, FORK_NOJOB) == 0) {
		close(pip[0]);
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
#ifdef SIGTSTP
		signal(SIGTSTP, SIG_IGN);
#endif
		signal(SIGPIPE, SIG_DFL);
		if (redir->type == NHERE)
			xwrite(pip[1], redir->nhere.doc->narg.text, len);
		else
			expandhere(redir->nhere.doc, pip[1]);
		_exit(0);
	}
out:
	close(pip[1]);
	return pip[0];
}



/*
 * Undo the effects of the last redirection.
 */

void
popredir(void)
{
	struct redirtab *rp = redirlist;

	INTOFF;
	free_rl(rp, 1);
	redirlist = rp->next;
	ckfree(rp);
	INTON;
}

/*
 * Undo all redirections.  Called on error or interrupt.
 */

#ifdef mkinit

INCLUDE "redir.h"

RESET {
	while (redirlist)
		popredir();
}

SHELLPROC {
	clearredir(0);
}

#endif

/* Return true if fd 0 has already been redirected at least once.  */
int
fd0_redirected_p(void)
{
	return fd0_redirected != 0;
}

/*
 * Discard all saved file descriptors.
 */

void
clearredir(int vforked)
{
	struct redirtab *rp;
	struct renamelist *rl;

	for (rp = redirlist ; rp ; rp = rp->next) {
		if (!vforked)
			free_rl(rp, 0);
		else for (rl = rp->renamed; rl; rl = rl->next)
			if (rl->into >= 0)
				close(rl->into);
	}
}



/*
 * Copy a file descriptor to be == to.
 * cloexec indicates if we want close-on-exec or not.
 * Returns -1 if any error occurs.
 */

STATIC int
copyfd(int from, int to, int cloexec)
{
	int newfd;

	if (cloexec && to > 2)
		newfd = dup3(from, to, O_CLOEXEC);
	else
		newfd = dup2(from, to);

	return newfd;
}

/*
 * rename fd from to be fd to (closing from).
 * close-on-exec is never set on 'to' (unless
 * from==to and it was set on from) - ie: a no-op
 * returns to (or errors() if an error occurs).  
 *
 * This is mostly used for rearranging the
 * results from pipe().
 */
int
movefd(int from, int to)
{
	if (from == to)
		return to;

	(void) close(to);
	if (copyfd(from, to, 0) != to) {
		int e = errno;

		(void) close(from);
		error("Unable to make fd %d: %s", to, strerror(e));
	}
	(void) close(from);

	return to;
}

STATIC void
find_big_fd(void)
{
	int i, fd;
	static int last_start = 3; /* aim to keep sh fd's under 20 */

	if (last_start < 10)
		last_start++;

	for (i = (1 << last_start); i >= 10; i >>= 1) {
		if ((fd = fcntl(0, F_DUPFD, i - 1)) >= 0) {
			close(fd);
			break;
		}
	}

	fd = (i / 5) * 4;
	if (fd < 10)
		fd = 10;

	big_sh_fd = fd;
}

/*
 * If possible, move file descriptor fd out of the way
 * of expected user fd values.   Returns the new fd
 * (which may be the input fd if things do not go well.)
 * Always set close-on-exec on the result, and close
 * the input fd unless it is to be our result.
 */
int
to_upper_fd(int fd)
{
	int i;

	if (big_sh_fd < 10)
		find_big_fd();
	do {
		i = fcntl(fd, F_DUPFD_CLOEXEC, big_sh_fd);
		if (i >= 0) {
			if (fd != i)
				close(fd);
			return i;
		}
		if (errno != EMFILE && errno != EINVAL)
			break;
		find_big_fd();
	} while (big_sh_fd > 10);

	/*
	 * If we wanted to move this fd to some random high number
	 * we certainly do not intend to pass it through exec, even
	 * if the reassignment failed.
	 */
	(void)fcntl(fd, F_SETFD, FD_CLOEXEC);
	return fd;
}

void
register_sh_fd(int fd, void (*cb)(int, int))
{
	struct shell_fds *fp;

	fp = ckmalloc(sizeof (struct shell_fds));
	if (fp != NULL) {
		fp->nxt = sh_fd_list;
		sh_fd_list = fp;

		fp->fd = fd;
		fp->cb = cb;
	}
}

void
sh_close(int fd)
{
	struct shell_fds **fpp, *fp;

	fpp = &sh_fd_list;
	while ((fp = *fpp) != NULL) {
		if (fp->fd == fd) {
			*fpp = fp->nxt;
			ckfree(fp);
			break;
		}
		fpp = &fp->nxt;
	}
	(void)close(fd);
}

STATIC struct shell_fds *
sh_fd(int fd)
{
	struct shell_fds *fp;

	for (fp = sh_fd_list; fp != NULL; fp = fp->nxt)
		if (fp->fd == fd)
			return fp;
	return NULL;
}

STATIC void
renumber_sh_fd(struct shell_fds *fp)
{
	int to;

	if (fp == NULL)
		return;

#ifndef	F_DUPFD_CLOEXEC
#define	F_DUPFD_CLOEXEC	F_DUPFD
#define	CLOEXEC(fd)	(fcntl((fd), F_SETFD, fcntl((fd),F_GETFD) | FD_CLOEXEC))
#else
#define	CLOEXEC(fd)
#endif

	/*
	 * if we have had a collision, and the sh fd was a "big" one
	 * try moving the sh fd base to a higher number (if possible)
	 * so future sh fds are less likely to be in the user's sights
	 * (incl this one when moved)
	 */
	if (fp->fd >= big_sh_fd)
		find_big_fd();

	to = fcntl(fp->fd, F_DUPFD_CLOEXEC, big_sh_fd);
	if (to == -1)
		to = fcntl(fp->fd, F_DUPFD_CLOEXEC, big_sh_fd/2);
	if (to == -1)
		to = fcntl(fp->fd, F_DUPFD_CLOEXEC, fp->fd + 1);
	if (to == -1)
		to = fcntl(fp->fd, F_DUPFD_CLOEXEC, 10);
	if (to == -1)
		to = fcntl(fp->fd, F_DUPFD_CLOEXEC, 3);
	if (to == -1)
		error("insufficient file descriptors available");
	CLOEXEC(to);

	if (fp->fd == to)	/* impossible? */
		return;

	(*fp->cb)(fp->fd, to);
	(void)close(fp->fd);
	fp->fd = to;
}

static const struct flgnames {
	const char *name;
	uint16_t minch;
	uint32_t value;
} nv[] = {
#ifdef O_APPEND
	{ "append",	2,	O_APPEND 	},
#endif
#ifdef O_ASYNC
	{ "async",	2,	O_ASYNC		},
#endif
#ifdef O_SYNC
	{ "sync",	2,	O_SYNC		},
#endif
#ifdef O_NONBLOCK
	{ "nonblock",	3,	O_NONBLOCK	},
#endif
#ifdef O_FSYNC
	{ "fsync",	2,	O_FSYNC		},
#endif
#ifdef O_DSYNC
	{ "dsync",	2,	O_DSYNC		},
#endif
#ifdef O_RSYNC
	{ "rsync",	2,	O_RSYNC		},
#endif
#ifdef O_ALTIO
	{ "altio",	2,	O_ALT_IO	},
#endif
#ifdef O_DIRECT
	{ "direct",	2,	O_DIRECT	},
#endif
#ifdef O_NOSIGPIPE
	{ "nosigpipe",	3,	O_NOSIGPIPE	},
#endif
#ifdef O_CLOEXEC
	{ "cloexec",	2,	O_CLOEXEC	},
#endif
	{ 0, 0, 0 }
};
#define ALLFLAGS (O_APPEND|O_ASYNC|O_SYNC|O_NONBLOCK|O_DSYNC|O_RSYNC|\
    O_ALT_IO|O_DIRECT|O_NOSIGPIPE|O_CLOEXEC)

static int
getflags(int fd, int p)
{
	int c, f;

	if (sh_fd(fd) != NULL) {
		if (!p)
			return -1;
		error("Can't get status for fd=%d (%s)", fd,
		    "Bad file descriptor");			/*XXX*/
	}

	if ((c = fcntl(fd, F_GETFD)) == -1) {
		if (!p)
			return -1;
		error("Can't get status for fd=%d (%s)", fd, strerror(errno));
	}
	if ((f = fcntl(fd, F_GETFL)) == -1) {
		if (!p)
			return -1;
		error("Can't get flags for fd=%d (%s)", fd, strerror(errno));
	}
	if (c & FD_CLOEXEC)
		f |= O_CLOEXEC;
	return f & ALLFLAGS;
}

static void
printone(int fd, int p, int verbose, int pfd)
{
	int f = getflags(fd, p);
	const struct flgnames *fn;

	if (f == -1)
		return;

	if (pfd)
		outfmt(out1, "%d: ", fd);
	for (fn = nv; fn->name; fn++) {
		if (f & fn->value) {
			outfmt(out1, "%s%s", verbose ? "+" : "", fn->name);
			f &= ~fn->value;
		} else if (verbose)
			outfmt(out1, "-%s", fn->name);
		else
			continue;
		if (f || (verbose && fn[1].name))
			outfmt(out1, ",");
	}
	if (verbose && f)		/* f should be normally be 0 */
		outfmt(out1, " +%#x", f);
	outfmt(out1, "\n");
}

static void
parseflags(char *s, int *p, int *n)
{
	int *v, *w;
	const struct flgnames *fn;
	size_t len;

	*p = 0;
	*n = 0;
	for (s = strtok(s, ","); s; s = strtok(NULL, ",")) {
		switch (*s++) {
		case '+':
			v = p;
			w = n;
			break;
		case '-':
			v = n;
			w = p;
			break;
		default:
			error("Missing +/- indicator before flag %s", s-1);
		}
			
		len = strlen(s);
		for (fn = nv; fn->name; fn++)
			if (len >= fn->minch && strncmp(s,fn->name,len) == 0) {
				*v |= fn->value;
				*w &=~ fn->value;
				break;
			}
		if (fn->name == 0)
			error("Bad flag `%s'", s);
	}
}

static void
setone(int fd, int pos, int neg, int verbose)
{
	int f = getflags(fd, 1);
	int n, cloexec;

	if (f == -1)
		return;

	cloexec = -1;
	if ((pos & O_CLOEXEC) && !(f & O_CLOEXEC))
		cloexec = FD_CLOEXEC;
	if ((neg & O_CLOEXEC) && (f & O_CLOEXEC))
		cloexec = 0;

	if (cloexec != -1 && fcntl(fd, F_SETFD, cloexec) == -1)
		error("Can't set status for fd=%d (%s)", fd, strerror(errno));

	pos &= ~O_CLOEXEC;
	neg &= ~O_CLOEXEC;
	f &= ~O_CLOEXEC;
	n = f;
	n |= pos;
	n &= ~neg;
	if (n != f && fcntl(fd, F_SETFL, n) == -1)
		error("Can't set flags for fd=%d (%s)", fd, strerror(errno));
	if (verbose)
		printone(fd, 1, verbose, 1);
}

int
fdflagscmd(int argc, char *argv[])
{
	char *num;
	int verbose = 0, ch, pos = 0, neg = 0;
	char *setflags = NULL;

	optreset = 1; optind = 1; /* initialize getopt */
	while ((ch = getopt(argc, argv, ":vs:")) != -1)
		switch ((char)ch) {
		case 'v':
			verbose = 1;
			break;
		case 's':
			if (setflags)
				goto msg;
			setflags = optarg;
			break;
		case '?':
		default:
		msg:
			error("Usage: fdflags [-v] [-s <flags> fd] [fd...]");
			/* NOTREACHED */
		}

	argc -= optind, argv += optind;

	if (setflags)
		parseflags(setflags, &pos, &neg);

	if (argc == 0) {
		int i;

		if (setflags)
			goto msg;

		for (i = 0; i <= max_user_fd; i++)
			printone(i, 0, verbose, 1);
		return 0;
	}

	while ((num = *argv++) != NULL) {
		int fd = number(num);

		while (num[0] == '0' && num[1] != '\0')		/* skip 0's */
			num++;
		if (strlen(num) > 5)
			error("%s too big to be a file descriptor", num);

		if (setflags)
			setone(fd, pos, neg, verbose);
		else
			printone(fd, 1, verbose, argc > 1);
	}
	return 0;
}

#undef MAX		/* in case we inherited them from somewhere */
#undef MIN

#define	MIN(a,b)	(/*CONSTCOND*/((a)<=(b)) ? (a) : (b))
#define	MAX(a,b)	(/*CONSTCOND*/((a)>=(b)) ? (a) : (b))

		/* now make the compiler work for us... */
#define	MIN_REDIR	MIN(MIN(MIN(MIN(NTO,NFROM), MIN(NTOFD,NFROMFD)), \
		   MIN(MIN(NCLOBBER,NAPPEND), MIN(NHERE,NXHERE))), NFROMTO)
#define	MAX_REDIR	MAX(MAX(MAX(MAX(NTO,NFROM), MAX(NTOFD,NFROMFD)), \
		   MAX(MAX(NCLOBBER,NAPPEND), MAX(NHERE,NXHERE))), NFROMTO)

static const char *redir_sym[MAX_REDIR - MIN_REDIR + 1] = {
	[NTO      - MIN_REDIR]=	">",
	[NFROM    - MIN_REDIR]=	"<",
	[NTOFD    - MIN_REDIR]=	">&",
	[NFROMFD  - MIN_REDIR]=	"<&",
	[NCLOBBER - MIN_REDIR]=	">|",
	[NAPPEND  - MIN_REDIR]=	">>",
	[NHERE    - MIN_REDIR]=	"<<",
	[NXHERE   - MIN_REDIR]=	"<<",
	[NFROMTO  - MIN_REDIR]=	"<>",
};

int
outredir(struct output *out, union node *n, int sep)
{
	if (n == NULL)
		return 0;
	if (n->type < MIN_REDIR || n->type > MAX_REDIR ||
	    redir_sym[n->type - MIN_REDIR] == NULL)
		return 0;

	if (sep)
		outc(sep, out);

	/*
	 * ugly, but all redir node types have "fd" in same slot...
	 *	(and code other places assumes it as well)
	 */
	if ((redir_sym[n->type - MIN_REDIR][0] == '<' && n->nfile.fd != 0) ||
	    (redir_sym[n->type - MIN_REDIR][0] == '>' && n->nfile.fd != 1))
		outfmt(out, "%d", n->nfile.fd);

	outstr(redir_sym[n->type - MIN_REDIR], out);

	switch (n->type) {
	case NHERE:
		outstr("'...'", out);
		break;
	case NXHERE:
		outstr("...", out);
		break;
	case NTOFD:
	case NFROMFD:
		if (n->ndup.dupfd < 0)
			outc('-', out);
		else
			outfmt(out, "%d", n->ndup.dupfd);
		break;
	default:
		outstr(n->nfile.expfname, out);
		break;
	}
	return 1;
}
