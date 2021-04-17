/*	$NetBSD: shlock.c,v 1.15 2021/04/17 00:02:19 christos Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Erik E. Fair.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
** Program to produce reliable locks for shell scripts.
** Algorithm suggested by Peter Honeyman, January 1984,
** in connection with HoneyDanBer UUCP.
**
** I tried extending this to handle shared locks in November 1987,
** and ran into to some fundamental problems:
**
**	Neither 4.3 BSD nor System V have an open(2) with locking,
**	so that you can open a file and have it locked as soon as
**	it's real; you have to make two system calls, and there's
**	a race...
**
**	When removing dead process id's from a list in a file,
**	you need to truncate the file (you don't want to create a
**	new one; see above); unfortunately for the portability of
**	this program, only 4.3 BSD has ftruncate(2).
**
** Erik E. Fair <fair@ucbarpa.berkeley.edu>, November 8, 1987
**
** Extensions for UUCP style locks (i.e. pid is an int in the file,
** rather than an ASCII string). Also fix long standing bug with
** full file systems and temporary files.
**
** Erik E. Fair <fair@apple.com>, November 12, 1989
**
** ANSIfy the code somewhat to make gcc -Wall happy with the code.
** Submit to NetBSD
**
** Erik E. Fair <fair@clock.org>, May 20, 1997
*/

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: shlock.c,v 1.15 2021/04/17 00:02:19 christos Exp $");
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <err.h>
#include <fcntl.h>			/* Needed on hpux */
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define	LOCK_SET	0
#define	LOCK_FAIL	1

#define	LOCK_GOOD	0
#define	LOCK_BAD	1

#define	FAIL		(-1)

#define	TRUE	1
#define	FALSE	0

static int	Debug = FALSE;
static char	*Pname;
static const char USAGE[] = "%s: USAGE: %s [-du] [-p PID] -f file\n";
static const char E_unlk[] = "unlink(%s)";
static const char E_link[] = "link(%s, %s)";
static const char E_open[] = "open(%s)";
static const char E_stat[] = "stat(%s)";

#define	dprintf	if (Debug) printf

/*
** Prototypes to make the ANSI compilers happy
** Didn't lint used to do type and argument checking?
** (and wasn't that sufficient?)
*/

/* the following is in case you need to make the prototypes go away. */
static char	*xtmpfile(const char *, pid_t, int);
static int	p_exists(pid_t);
static int	cklock(const char *, struct stat *, int);
static int	mklock(const char *, pid_t, int);
__dead static void	bad_usage(void);

/*
** Create a temporary file, all ready to lock with.
** The file arg is so we get the filename right, if he
** gave us a full path, instead of using the current directory
** which might not be in the same filesystem.
*/
static char *
xtmpfile(const char *file, pid_t pid, int uucpstyle)
{
	int	fd;
	size_t	len;
	char	*cp, buf[BUFSIZ];
	static char	tempname[BUFSIZ];

	snprintf(buf, sizeof(buf), "shlock%jd", (intmax_t)getpid());
	if ((cp = strrchr(strcpy(tempname, file), '/')) != NULL) {
		*++cp = '\0';
		(void) strcat(tempname, buf);
	} else
		(void) strcpy(tempname, buf);
	dprintf("%s: temporary filename: %s\n", Pname, tempname);

	snprintf(buf, sizeof(buf), "%jd\n", (intmax_t)pid);
	len = strlen(buf);
	while ((fd = open(tempname, O_RDWR|O_CREAT|O_TRUNC|O_SYNC|O_EXCL, 0644))
	    == -1)
	{
		switch(errno) {
		case EEXIST:
			if (unlink(tempname) == -1) {
				warn(E_unlk, tempname);
				return NULL;
			}
			break;
		default:
			warn(E_open, tempname);
			return NULL;
		}
	}

	/*
	** Write the PID into the temporary file before attempting to link
	** to the actual lock file. That way we have a valid lock the instant
	** the link succeeds.
	*/
	if (uucpstyle ?
		(write(fd, &pid, sizeof(pid)) != sizeof(pid)) :
		(write(fd, buf, len) != (ssize_t)len))
	{
		warn("write(%s,%jd)", tempname, (intmax_t)pid);
		(void) close(fd);
		if (unlink(tempname) == -1) {
			warn(E_unlk, tempname);
		}
		return NULL;
	}
	(void) close(fd);
	return tempname;
}

/*
** Does the PID exist?
** Send null signal to find out.
*/
static int
p_exists(pid_t pid)
{
	dprintf("%s: process %jd is ", Pname, (intmax_t)pid);
	if (pid <= 0) {
		dprintf("invalid\n");
		return FALSE;
	}
	if (kill(pid, 0) == -1) {
		switch(errno) {
		case ESRCH:
			dprintf("dead %jd\n", (intmax_t)pid);
			return FALSE;	/* pid does not exist */
		case EPERM:
			dprintf("alive\n");
			return TRUE;	/* pid exists */
		default:
			dprintf("state unknown: %s\n", strerror(errno));
			return TRUE;	/* be conservative */
		}
	}
	dprintf("alive\n");
	return TRUE;	/* pid exists */
}

/*
** Check the validity of an existing lock file.
**
**	Read the PID out of the lock
**	Send a null signal to determine whether that PID still exists
**	Existence (or not) determines the validity of the lock.
**
**	Two bigs wins to this algorithm:
**
**	o	Locks do not survive crashes of either the system or the
**			application by any appreciable period of time.
**
**	o	No clean up to do if the system or application crashes.
**
*/
static int
cklock(const char *file, struct stat *st, int uucpstyle)
{
	int	fd = open(file, O_RDONLY);
	ssize_t len;
	pid_t	pid;
	char	buf[BUFSIZ];

	dprintf("%s: checking extant lock <%s>\n", Pname, file);
	if (fd == -1) {
		if (errno != ENOENT)
			warn(E_open, file);
		return TRUE;	/* might or might not; conservatism */
	}

	if (st != NULL && fstat(fd, st) == -1) {
		warn(E_stat, file);
		close(fd);
		return TRUE;
	}

	if (uucpstyle ?
		((len = read(fd, &pid, sizeof(pid))) != sizeof(pid)) :
		((len = read(fd, buf, sizeof(buf))) <= 0))
	{
		close(fd);
		dprintf("%s: lock file format error\n", Pname);
		return FALSE;
	}
	close(fd);
	buf[len + 1] = '\0';
	return p_exists(uucpstyle ? pid : atoi(buf));
}

static int
mklock(const char *file, pid_t pid, int uucpstyle)
{
	char	*tmp, tmp2[BUFSIZ + 2];
	int	retcode = FALSE;
	struct stat stlock, sttmp, stlock2;

	dprintf("%s: trying lock <%s> for process %jd\n", Pname, file,
	    (intmax_t)pid);
	if ((tmp = xtmpfile(file, pid, uucpstyle)) == NULL)
		return FALSE;

	while (link(tmp, file) == -1) {
		switch(errno) {
		case EEXIST:
			dprintf("%s: lock <%s> already exists\n", Pname, file);
			if (cklock(file, &stlock, uucpstyle)) {
				dprintf("%s: extant lock is valid\n", Pname);
				goto out;
			}
			if (stat(tmp, &sttmp) == -1) {
				warn(E_stat, tmp);
				goto out;
			}
			/*
			 * We need to take another lock against the lock
			 * file to protect it against multiple removals
			 */
			snprintf(tmp2, sizeof(tmp2), "%s-2", tmp);
			if (unlink(tmp2) == -1 && errno != ENOENT) {
				warn(E_unlk, tmp2);
				goto out;
			}
			if (stlock.st_ctime >= sttmp.st_ctime) {
				dprintf("%s: lock time changed %jd >= %jd\n",
				    Pname, (intmax_t)stlock.st_ctime,
				    (intmax_t)stlock2.st_ctime);
				goto out;
			}
			if (stlock.st_nlink != 1) {
				dprintf("%s: someone else linked to it %d\n",
				    Pname, stlock.st_nlink);
				goto out;
			}
			if (link(file, tmp2) == -1) {
				/* someone took our temp name! */
				warn(E_link, file, tmp2);
				goto out2;
			}
			if (stat(file, &stlock2) == -1) {
				warn(E_stat, file);
				goto out2;
			}
			if (stlock.st_ino != stlock2.st_ino) {
				dprintf("%s: lock inode changed %jd != %jd\n",
				    Pname, (intmax_t)stlock.st_ino,
				    (intmax_t)stlock2.st_ino);
				goto out2;
			}
			if (stlock2.st_nlink != 2) {
				dprintf("%s: someone else linked to it %d\n",
				    Pname, stlock2.st_nlink);
				goto out2;
			}
				
			dprintf("%s: lock is invalid, removing\n", Pname);
			if (unlink(file) == -1) {
				warn(E_unlk, file);
				goto out2;
			}
			if (unlink(tmp2) == -1)
				goto out2;
			break;
		default:
			warn(E_link, tmp, file);
			goto out;
		}
	}

	if (stat(tmp, &sttmp) == -1) {
		warn(E_stat, tmp);
		goto out;
	}

	if (sttmp.st_nlink == 2) {
		dprintf("%s: got lock <%s>\n", Pname, file);
		retcode = TRUE;
		goto out;

	}

out2:
	if (unlink(tmp2) == -1) {
		warn(E_unlk, tmp2);
	}
out:
	if (unlink(tmp) == -1) {
		warn(E_unlk, tmp);
	}

	return retcode;
}

static void
bad_usage(void)
{
	fprintf(stderr, USAGE, Pname, Pname);
	exit(LOCK_FAIL);
}

int
main(int ac, char **av)
{
	int	x;
	char	*file = NULL;
	pid_t	pid = 0;
	int	uucpstyle = FALSE;	/* indicating UUCP style locks */
	int	only_check = TRUE;	/* don't make a lock */

	Pname = ((Pname = strrchr(av[0], '/')) ? Pname + 1 : av[0]);

	for(x = 1; x < ac; x++) {
		if (av[x][0] == '-') {
			switch(av[x][1]) {
			case 'u':
				uucpstyle = TRUE;
				break;
			case 'd':
				Debug = TRUE;
				break;
			case 'p':
				if (strlen(av[x]) > 2) {
					pid = atoi(&av[x][2]);
				} else {
					if (++x >= ac) {
						bad_usage();
					}
					pid = atoi(av[x]);
				}
				only_check = FALSE;	/* wants one */
				break;
			case 'f':
				if (strlen(av[x]) > 2) {
					file = &av[x][2];
				} else {
					if (++x >= ac) {
						bad_usage();
					}
					file = av[x];
				}
				break;
			default:
				bad_usage();
			}
		}
	}

	if (file == NULL || (!only_check && pid <= 0)) {
		bad_usage();
	}

	if (only_check) {
		exit(cklock(file, NULL, uucpstyle) ? LOCK_GOOD : LOCK_BAD);
	}

	exit(mklock(file, pid, uucpstyle) ? LOCK_SET : LOCK_FAIL);
}
