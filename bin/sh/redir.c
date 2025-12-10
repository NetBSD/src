/*	$NetBSD: redir.c,v 1.77 2025/12/10 21:33:02 andvar Exp $	*/

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
__RCSID("$NetBSD: redir.c,v 1.77 2025/12/10 21:33:02 andvar Exp $");
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
#include "show.h"


#define CLOSED -1		/* fd was not open before redir */

#ifndef PIPE_BUF
# define PIPESIZE 4096		/* amount of buffering in a pipe */
#else
# define PIPESIZE PIPE_BUF
#endif

#ifndef FD_CLOEXEC
# define FD_CLOEXEC	1	/* well known from before there was a name */
#endif

#ifndef F_DUPFD_CLOEXEC
#define F_DUPFD_CLOEXEC	F_DUPFD
#define CLOEXEC(fd)	(fcntl((fd), F_SETFD,				\
			     (fcntl_int)(fcntl((fd), F_GETFD) | FD_CLOEXEC)))
#else
#define CLOEXEC(fd)	__nothing
#endif

/* yes, this is correct, bizarre parens and all -- used only as a cast */
#define fcntl_int	void *)(intptr_t
#undef  fcntl_int
#define fcntl_int	int


MKINIT
struct renamelist {
	struct renamelist *next;
	int orig;
	int into;
	int cloexec;	/* orig had FD_CLOEXEC set (into always does) */
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
STATIC int biggest_sh_fd = 2;

STATIC const struct renamelist *is_renamed(const struct renamelist *, int);
STATIC void fd_rename(struct redirtab *, int, int, int);
STATIC int * saved_redirected_fd(int);
STATIC void free_rl(struct redirtab *, int);
STATIC void openredirect(union node *, char[10], int);
STATIC int openhere(const union node *);
STATIC int copyfd(int, int, int, int);
STATIC void find_big_fd(void);


struct shell_fds {		/* keep track of internal shell fds */
	struct shell_fds *nxt;
	void (*cb)(int, int);
	int fd;
};

STATIC struct shell_fds *sh_fd_list;

STATIC int pick_new_fd(int);
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

STATIC int *
saved_redirected_fd(int fd)
{
	struct redirtab *rt;
	struct renamelist *rl;

	for (rt = redirlist; rt != NULL; rt = rt->next) {
		for (rl =  rt->renamed; rl != NULL; rl = rl->next) {
			if (rl->into == fd)
				return &rl->into;
		}
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

		VTRACE(DBG_REDIR, ("popredir %d%s: %s",
		    rl->orig, rl->orig==0 ? " (STDIN)" : "",
		    reset == POPREDIR_UNDO ? "" :
		    reset == POPREDIR_PERMANENT ? "make permanent" :
					     "no reset\n"));

		switch (reset) {
		case POPREDIR_UNDO:
			if (rl->into < 0) {
				VTRACE(DBG_REDIR, (" closed\n"));
				close(rl->orig);
			} else {
				VTRACE(DBG_REDIR,
				    (" from %d%s\n", rl->into,
				       rl->cloexec ? " (colexec)" : ""));
				copyfd(rl->into, rl->orig, rl->cloexec, 1);
			}
			break;

		case POPREDIR_PERMANENT:
			if (rl->into < 0) {
				VTRACE(DBG_REDIR, (" was closed\n"));
				/* nothing to do */
			} else {
				VTRACE(DBG_REDIR,
				    (" close savefd %d\n", rl->into));
				close(rl->into);
			}
			break;

		default:
			/* nothing to do */
			break;
		}
		ckfree(rl);
	}
	rt->renamed = NULL;
}

STATIC void
fd_rename(struct redirtab *rt, int from, int to, int cloexec)
{
	/* XXX someday keep a short list (8..10) of freed renamelists XXX */
	struct renamelist *rl = ckmalloc(sizeof(struct renamelist));

	/*
	 * Note this list is operated as LIFO so saved fd's are
	 * undone in the opposite order to that they were saved
	 * (needed to ensure correct results)
	 */
	rl->next = rt->renamed;
	rt->renamed = rl;

	rl->orig = from;
	rl->into = to;
	rl->cloexec = cloexec;
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

	CTRACE(DBG_REDIR, ("redirect(F=0x%x):%s\n", flags, redir?"":" NONE"));

	for (i = 10 ; --i >= 0 ; )
		memory[i] = 0;
	memory[1] = flags & REDIR_BACKQ;
	if (flags & REDIR_PUSH) {
		/*
		 * We don't have to worry about REDIR_VFORK here, as
		 * flags & REDIR_PUSH is never true if REDIR_VFORK is set.
		 */
		sv = ckmalloc(sizeof (struct redirtab));
		sv->renamed = NULL;
		sv->next = redirlist;
		redirlist = sv;
	}
	for (n = redir ; n ; n = n->nfile.next) {
		int *renamed;

		fd = n->nfile.fd;
		VTRACE(DBG_REDIR, ("redir %d (max=%d limit=%ld) ",
		    fd, max_user_fd, user_fd_limit));
		if (fd < user_fd_limit && fd > max_user_fd)
			max_user_fd = fd;
		if ((n->nfile.type == NTOFD || n->nfile.type == NFROMFD) &&
		    n->ndup.dupfd == fd) {
			VTRACE(DBG_REDIR, ("!cloexec\n"));
			if (sh_fd(fd) != NULL ||
			    saved_redirected_fd(fd) != NULL)
				error("fd %d: %s", fd, strerror(EBADF));
			/* redirect from/to same file descriptor */
			/* make sure it stays open */
			if (fcntl(fd, F_SETFD, (fcntl_int)0) < 0)
				error("fd %d: %s", fd, strerror(errno));
			continue;
		}
		if ((renamed = saved_redirected_fd(fd)) != NULL) {
			int to = pick_new_fd(fd);

			VTRACE(DBG_REDIR,
			    ("redirect: moved holding fd %d to %d\n", fd, to));
			*renamed = to;
			if (to != fd)	/* always... */
				(void)close(fd);
		}
		renumber_sh_fd(sh_fd(fd));

		if ((flags & REDIR_PUSH) && !is_renamed(sv->renamed, fd)) {
			int bigfd;
			int cloexec;

			cloexec = fcntl(fd, F_GETFD);
			if (cloexec >= 0)
				cloexec &= FD_CLOEXEC;
			else
				cloexec = 0;

			INTOFF;
			if (big_sh_fd < 10)
				find_big_fd();
			if ((bigfd = big_sh_fd) < max_user_fd)
				bigfd = max_user_fd;
			if ((i = fcntl(fd, F_DUPFD,
			    (fcntl_int)(bigfd + 1))) == -1) {
				switch (errno) {
				case EBADF:
					i = CLOSED;
					break;
				case EMFILE:
				case EINVAL:
					find_big_fd();
					i = fcntl(fd, F_DUPFD, 
					          (fcntl_int) big_sh_fd);
					if (i >= 0)
						break;
					if (errno == EMFILE || errno == EINVAL)
						i = fcntl(fd, F_DUPFD,
							  (fcntl_int) 3);
					if (i >= 0)
						break;
					/* FALLTHRU */
				default:
					error("%d: %s", fd, strerror(errno));
					/* NOTREACHED */
				}
			}
			if (i > biggest_sh_fd)
				biggest_sh_fd = i;
			if (i >= 0)
				(void)fcntl(i, F_SETFD, (fcntl_int) FD_CLOEXEC);
			fd_rename(sv, fd, i, cloexec);
			VTRACE(DBG_REDIR, ("fd %d saved as %d%s ", fd, i,
			    cloexec ? "+" : ""));
			INTON;
		}
		VTRACE(DBG_REDIR, ("%s\n", fd == 0 ? "STDIN" : ""));
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
		VTRACE(DBG_REDIR, ("openredirect(< '%s') -> %d [%#x]",
		    fname, f, eflags));
		if (eflags)
			(void)fcntl(f, F_SETFL,
				  (fcntl_int)(fcntl(f, F_GETFL) & ~eflags));
		break;
	case NFROMTO:
		fname = redir->nfile.expfname;
		if ((f = open(fname, O_RDWR|O_CREAT, 0666)) < 0)
			goto ecreate;
		VTRACE(DBG_REDIR, ("openredirect(<> '%s') -> %d", fname, f));
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
			VTRACE(DBG_REDIR, ("openredirect(>| '%s') -> %d",
			    fname, f));
			break;
		}
		/* FALLTHROUGH */
	case NCLOBBER:
		fname = redir->nfile.expfname;
		if ((f = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0666)) < 0)
			goto ecreate;
		VTRACE(DBG_REDIR, ("openredirect(> '%s') -> %d", fname, f));
		break;
	case NAPPEND:
		fname = redir->nfile.expfname;
		if ((f = open(fname, O_WRONLY|O_CREAT|O_APPEND, 0666)) < 0)
			goto ecreate;
		VTRACE(DBG_REDIR, ("openredirect(>> '%s') -> %d", fname, f));
		break;
	case NTOFD:
	case NFROMFD:
		if (redir->ndup.dupfd >= 0) {	/* if not ">&-" */
			if (sh_fd(redir->ndup.dupfd) != NULL ||
			    saved_redirected_fd(redir->ndup.dupfd) != NULL)
				error("Redirect (from %d to %d) failed: %s",
				    redir->ndup.dupfd, fd, strerror(EBADF));
			if (fd < 10 && redir->ndup.dupfd < 10 &&
			    memory[redir->ndup.dupfd])
				memory[fd] = 1;
			else if (copyfd(redir->ndup.dupfd, fd,
			    (flags & REDIR_KEEP) == 0, 0) < 0)
				error("Redirect (from %d to %d) failed: %s",
				    redir->ndup.dupfd, fd, strerror(errno));
			VTRACE(DBG_REDIR, ("openredirect: %d%c&%d\n", fd,
			    "<>"[redir->nfile.type==NTOFD], redir->ndup.dupfd));
		} else {
			(void) close(fd);
			VTRACE(DBG_REDIR, ("openredirect: %d%c&-\n", fd,
			    "<>"[redir->nfile.type==NTOFD]));
		}
		INTON;
		return;
	case NHERE:
	case NXHERE:
		VTRACE(DBG_REDIR, ("openredirect: %d<<...", fd));
		f = openhere(redir);
		break;
	default:
		abort();
	}

	if (f > biggest_sh_fd)
		biggest_sh_fd = f;

	cloexec = fd > 2 && (flags & REDIR_KEEP) == 0 && !posix;
	if (f != fd) {
		VTRACE(DBG_REDIR, (" -> %d", fd));
		if (copyfd(f, fd, cloexec, 1) < 0) {
			int e = errno;

			VTRACE(DBG_REDIR, (" failed: %s\n", strerror(e)));
			error("redirect reassignment (fd %d) failed: %s", fd,
			    strerror(e));
		}
	} else if (cloexec)
		(void)fcntl(f, F_SETFD, (fcntl_int) FD_CLOEXEC);
	VTRACE(DBG_REDIR, ("%s\n", cloexec ? " cloexec" : ""));

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
	if (pip[1] > biggest_sh_fd)
		biggest_sh_fd = pip[1];
	len = strlen(redir->nhere.text);
	VTRACE(DBG_REDIR, ("openhere(%p) [%d] \"%.*s\"%s\n", redir, len,
	    (len < 40 ? len : 40), redir->nhere.text, (len < 40 ? "" : "...")));
	if (len <= PIPESIZE) {		/* XXX eventually copy FreeBSD method */
		xwrite(pip[1], redir->nhere.text, len);
		goto out;
	}
	VTRACE(DBG_REDIR, (" forking [%d,%d]\n", pip[0], pip[1]));
	if (forkshell(NULL, NULL, FORK_NOJOB) == 0) {
		close(pip[0]);
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
#ifdef SIGTSTP
		signal(SIGTSTP, SIG_IGN);
#endif
		signal(SIGPIPE, SIG_DFL);
		xwrite(pip[1], redir->nhere.text, len);
		VTRACE(DBG_PROCS|DBG_REDIR, ("wrote here doc.  exiting(0)\n"));
		_exit(0);
	}
	VTRACE(DBG_REDIR, ("openhere (closing %d)", pip[1]));
 out:;
	close(pip[1]);
	VTRACE(DBG_REDIR, (" (pipe fd=%d)", pip[0]));
	return pip[0];
}



/*
 * if (reset == POPREDIR_UNDO)
 *	Undo the effects of the last redirection.
 * else if (reset == POPREDIR_PERMANENT)
 *	Make the last redirection permanent
 * else / * reset == POPREDIR_DISCARD * /
 *	Just throw away the redirection
 */

void
popredir(int reset)
{
	struct redirtab *rp = redirlist;

	INTOFF;
	free_rl(rp, reset);
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
		popredir(POPREDIR_UNDO);
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

	/*
	 * This happens only in a child process attempting to
	 * exec a script which failed ENOEXEC.   Any redirections
	 * that have been made are for that script, after it
	 * is done, we exit, there is nothing  to restore, so
	 * just make the redirections permanent.
	 *
	 * free_rl() does that does that for us, unless we're
	 * a child of a vfork() in which case that is unsafe to
	 * call - that should never happen, but just in case, fale
	 * it here
	 */

	for (rp = redirlist ; rp ; rp = rp->next) {
		if (!vforked)
			free_rl(rp, POPREDIR_PERMANENT);
		else for (rl = rp->renamed; rl; rl = rl->next)
			if (rl->into >= 0)
				close(rl->into);
	}
}



/*
 * Copy a file descriptor (from) to (also) be 'to'..
 * cloexec indicates if we want close-on-exec or not.
 * move indicates if "from" should be closed on success (yes if set).
 * Returns -1 if any error occurs.
 */

STATIC int
copyfd(int from, int to, int cloexec, int move)
{
	int newfd;

	if (cloexec && to > 2) {
#ifdef O_CLOEXEC
		newfd = dup3(from, to, O_CLOEXEC);
#else
		newfd = dup2(from, to);
		if (newfd >= 0)
			fcntl(newfd, F_SETFD,
			    (fcntl_int)(fcntl(newfd, F_GETFD) | FD_CLOEXEC));
#endif
	} else
		newfd = dup2(from, to);

	if (move && newfd == to && from != to)
		close(from);

	if (newfd != to) {
		if (newfd < 0)
			error("Unable to dup(%d->%d): %s", from, to,
			    strerror(errno));
		else {
			close(newfd);
			error("Moving fd %d to %d made %d!", from, to, newfd);
		}
	}

	if (newfd > biggest_sh_fd)
		biggest_sh_fd = newfd;

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
	if (from > biggest_sh_fd)
		biggest_sh_fd = from;
	if (to > biggest_sh_fd)
		biggest_sh_fd = to;

	if (from == to)
		return to;

	(void)close(to);
	if (copyfd(from, to, 0, 1) != to) {
		int e = errno;

		(void) close(from);
		error("Unable to make fd %d: %s", to, strerror(e));
	}

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
		if ((fd = fcntl(0, F_DUPFD, (fcntl_int)(i - 1))) >= 0) {
			if (fd > biggest_sh_fd)
				biggest_sh_fd = fd;
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

	VTRACE(DBG_REDIR|DBG_OUTPUT, ("to_upper_fd(%d)", fd));
	if (big_sh_fd < 10 || big_sh_fd >= user_fd_limit)
		find_big_fd();
	do {
		i = fcntl(fd, F_DUPFD_CLOEXEC, (fcntl_int) big_sh_fd);
		if (i >= 0) {
			if (i > biggest_sh_fd)
				biggest_sh_fd = i;
			if (fd != i)
				close(fd);
			VTRACE(DBG_REDIR|DBG_OUTPUT, ("-> %d\n", i));
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
	(void)fcntl(fd, F_SETFD, (fcntl_int) FD_CLOEXEC);
	VTRACE(DBG_REDIR|DBG_OUTPUT, (" fails ->%d\n", fd));
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

STATIC int
pick_new_fd(int fd)
{
	int to;

	to = fcntl(fd, F_DUPFD_CLOEXEC, (fcntl_int) big_sh_fd);
	if (to == -1 && big_sh_fd >= 22)
		to = fcntl(fd, F_DUPFD_CLOEXEC, (fcntl_int) (big_sh_fd / 2));
	if (to == -1)
		to = fcntl(fd, F_DUPFD_CLOEXEC, (fcntl_int) (fd + 1));
	if (to == -1)
		to = fcntl(fd, F_DUPFD_CLOEXEC, (fcntl_int) 10);
	if (to == -1)
		to = fcntl(fd, F_DUPFD_CLOEXEC, (fcntl_int)  3);
	if (to == -1)
		error("insufficient file descriptors available");
	CLOEXEC(to);
	if (to > biggest_sh_fd)
		biggest_sh_fd = to;
	return to;
}

STATIC void
renumber_sh_fd(struct shell_fds *fp)
{
	int to;

	if (fp == NULL)
		return;

	/*
	 * if we have had a collision, and the sh fd was a "big" one
	 * try moving the sh fd base to a higher number (if possible)
	 * so future sh fds are less likely to be in the user's sights
	 * (incl this one when moved)
	 */
	if (fp->fd >= big_sh_fd)
		find_big_fd();

	to = pick_new_fd(fp->fd);

	if (fp->fd == to)	/* impossible? */
		return;

	VTRACE(DBG_REDIR, ("renumber_sh_fd: moved shell fd %d to %d\n",
	    fp->fd, to));

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
#else
# define O_APPEND 0
#endif
#ifdef O_ASYNC
	{ "async",	2,	O_ASYNC		},
#else
# define O_ASYNC 0
#endif
#ifdef O_SYNC
	{ "sync",	2,	O_SYNC		},
#else
# define O_SYNC 0
#endif
#ifdef O_NONBLOCK
	{ "nonblock",	3,	O_NONBLOCK	},
#else
# define O_NONBLOCK 0
#endif
#ifdef O_FSYNC
	{ "fsync",	2,	O_FSYNC		},
#else
# define O_FSYNC 0
#endif
#ifdef O_DSYNC
	{ "dsync",	2,	O_DSYNC		},
#else
# define O_DSYNC 0
#endif
#ifdef O_RSYNC
	{ "rsync",	2,	O_RSYNC		},
#else
# define O_RSYNC 0
#endif
#ifdef O_ALT_IO
	{ "altio",	2,	O_ALT_IO	},
#else
# define O_ALT_IO 0
#endif
#ifdef O_DIRECT
	{ "direct",	2,	O_DIRECT	},
#else
# define O_DIRECT 0
#endif
#ifdef O_NOSIGPIPE
	{ "nosigpipe",	3,	O_NOSIGPIPE	},
#else
# define O_NOSIGPIPE 0
#endif

#define ALLFLAGS (O_APPEND|O_ASYNC|O_SYNC|O_NONBLOCK|O_DSYNC|O_RSYNC|\
    O_ALT_IO|O_DIRECT|O_NOSIGPIPE)

#ifndef	O_CLOEXEC
# define O_CLOEXEC	((~ALLFLAGS) ^ ((~ALLFLAGS) & ((~ALLFLAGS) - 1)))
#endif

	/* for any system we support, close on exec is always defined */
	{ "cloexec",	2,	O_CLOEXEC	},
	{ 0, 0, 0 }
};

#ifndef O_ACCMODE
# define O_ACCMODE	0
#endif
#ifndef O_RDONLY
# define O_RDONLY	0
#endif
#ifndef O_WRONLY
# define O_WRONLY	0
#endif
#ifndef O_RWDR
# define O_RWDR		0
#endif
#ifndef O_SHLOCK
# define O_SHLOCK	0
#endif
#ifndef O_EXLOCK
# define O_EXLOCK	0
#endif
#ifndef O_NOFOLLOW
# define O_NOFOLLOW	0
#endif
#ifndef O_CREAT
# define O_CREAT	0
#endif
#ifndef O_TRUNC
# define O_TRUNC	0
#endif
#ifndef O_EXCL
# define O_EXCL		0
#endif
#ifndef O_NOCTTY
# define O_NOCTTY	0
#endif
#ifndef O_DIRECTORY
# define O_DIRECTORY	0
#endif
#ifndef O_REGULAR
# define O_REGULAR	0
#endif
/*
 * flags that F_GETFL might return that we want to ignore
 *
 * F_GETFL should not actually return these, they're all just open()
 * modifiers, rather than state, but just in case...
 */
#define IGNFLAGS (O_ACCMODE|O_RDONLY|O_WRONLY|O_RDWR|O_SHLOCK|O_EXLOCK| \
    O_NOFOLLOW|O_CREAT|O_TRUNC|O_EXCL|O_NOCTTY|O_DIRECTORY|O_REGULAR)

static int
getflags(int fd, int p, int all)
{
	int c, f;

	if (!all && (sh_fd(fd) != NULL || saved_redirected_fd(fd) != NULL)) {
		if (!p)
			return -1;
		error("Can't get status for fd=%d (%s)", fd, strerror(EBADF));
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
	f &= ~IGNFLAGS;		/* clear anything we know about, but ignore */
	if (c & FD_CLOEXEC)
		f |= O_CLOEXEC;
	return f;
}

static void
printone(int fd, int p, int verbose, int pfd)
{
	int f = getflags(fd, p, verbose > 1);
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
	int f = getflags(fd, 1, 0);
	int n, cloexec;

	if (f == -1)
		return;

	cloexec = -1;
	if ((pos & O_CLOEXEC) && !(f & O_CLOEXEC))
		cloexec = FD_CLOEXEC;
	if ((neg & O_CLOEXEC) && (f & O_CLOEXEC))
		cloexec = 0;

	/* Don't allow O_CLOEXEC on stdin, stdout, or stderr */
	if ((cloexec > 0 && fd <= 2 && (errno = EINVAL)) ||
	    (cloexec != -1 && fcntl(fd, F_SETFD, (fcntl_int) cloexec) == -1))
		error("Can't set status for fd=%d (%s)", fd, strerror(errno));

	pos &= ~O_CLOEXEC;
	neg &= ~O_CLOEXEC;
	f &= ~O_CLOEXEC;
	n = f;
	n |= pos;
	n &= ~neg;
	if (n != f && fcntl(fd, F_SETFL, (fcntl_int)n) == -1)
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
	while ((ch = getopt(argc, argv, ":vs:"
#ifdef DEBUG
					     "V"
#endif
						)) != -1)
		switch ((char)ch) {
#ifdef DEBUG
		case 'V':
			verbose = 2;
			break;
#endif
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
		if (verbose > 1)
			while (i <= biggest_sh_fd)
				printone(i++, 0, verbose, 1);

	} else while ((num = *argv++) != NULL) {
		int fd = number(num);

		while (num[0] == '0' && num[1] != '\0')		/* skip 0's */
			num++;
		if (strlen(num) > 5 ||
		    (fd >= user_fd_limit && fd > max_user_fd))
			error("%s: too big to be a file descriptor", num);

		if (setflags)
			setone(fd, pos, neg, verbose);
		else
			printone(fd, 1, verbose, argc > 1);
	}
	flushout(out1);
	if (io_err(out1)) {
		out2str("fdflags: I/O error\n");
		return 1;
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
