/*	$NetBSD: dotlock.c,v 1.9.14.1 2009/05/13 19:19:56 jym Exp $	*/

/*
 * Copyright (c) 1996 Christos Zoulas.  All rights reserved.
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
 *	This product includes software developed by Christos Zoulas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: dotlock.c,v 1.9.14.1 2009/05/13 19:19:56 jym Exp $");
#endif

#include "rcv.h"
#include "extern.h"
#include "sig.h"

#ifndef O_SYNC
#define O_SYNC	0
#endif

static int create_exclusive(const char *);
/*
 * Create a unique file. O_EXCL does not really work over NFS so we follow
 * the following trick: [Inspired by  S.R. van den Berg]
 *
 * - make a mostly unique filename and try to create it.
 * - link the unique filename to our target
 * - get the link count of the target
 * - unlink the mostly unique filename
 * - if the link count was 2, then we are ok; else we've failed.
 */
static int
create_exclusive(const char *fname)
{
	char path[MAXPATHLEN], hostname[MAXHOSTNAMELEN + 1];
	const char *ptr;
	struct timeval tv;
	pid_t pid;
	size_t ntries, cookie;
	int fd, serrno;
	struct stat st;

	(void)gettimeofday(&tv, NULL);
	(void)gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	pid = getpid();

	cookie = pid ^ tv.tv_usec;

	/*
	 * We generate a semi-unique filename, from hostname.(pid ^ usec)
	 */
	if ((ptr = strrchr(fname, '/')) == NULL)
		ptr = fname;
	else
		ptr++;

	(void)snprintf(path, sizeof(path), "%.*s.%s.%lx",
	    (int)(ptr - fname), fname, hostname, (u_long)cookie);

	/*
	 * We try to create the unique filename.
	 */
	for (ntries = 0; ntries < 5; ntries++) {
		fd = open(path, O_WRONLY|O_CREAT|O_TRUNC|O_EXCL|O_SYNC, 0);
		if (fd != -1) {
			(void)close(fd);
			break;
		}
		else if (errno == EEXIST)
			continue;
		else
			return -1;
	}

	/*
	 * We link the path to the name
	 */
	if (link(path, fname) == -1)
		goto bad;

	/*
	 * Note that we stat our own exclusively created name, not the
	 * destination, since the destination can be affected by others.
	 */
	if (stat(path, &st) == -1)
		goto bad;

	(void)unlink(path);

	/*
	 * If the number of links was two (one for the unique file and one
	 * for the lock), we've won the race
	 */
	if (st.st_nlink != 2) {
		errno = EEXIST;
		return -1;
	}
	return 0;

bad:
	serrno = errno;
	(void)unlink(path);
	errno = serrno;
	return -1;
}

/*
 * fname -- Pathname to lock
 * pollinterval -- Interval to check for lock, -1 return
 * fp -- File to print message
 * msg -- Message to print
 */
PUBLIC int
dot_lock(const char *fname, int pollinterval, FILE *fp, const char *msg)
{
	char path[MAXPATHLEN];
	sigset_t nset, oset;
	int retval;

	(void)sigemptyset(&nset);
	(void)sigaddset(&nset, SIGHUP);
	(void)sigaddset(&nset, SIGINT);
	(void)sigaddset(&nset, SIGQUIT);
	(void)sigaddset(&nset, SIGTERM);
	(void)sigaddset(&nset, SIGTTIN);
	(void)sigaddset(&nset, SIGTTOU);
	(void)sigaddset(&nset, SIGTSTP);
	(void)sigaddset(&nset, SIGCHLD);

	(void)snprintf(path, sizeof(path), "%s.lock", fname);

	retval = -1;
	for (;;) {
		sig_check();
		(void)sigprocmask(SIG_BLOCK, &nset, &oset);
		if (create_exclusive(path) != -1) {
			(void)sigprocmask(SIG_SETMASK, &oset, NULL);
			retval = 0;
			break;
		}
		else
			(void)sigprocmask(SIG_SETMASK, &oset, NULL);

		if (errno != EEXIST)
			break;

		if (fp && msg)
		    (void)fputs(msg, fp);

		if (pollinterval) {
			if (pollinterval == -1) {
				errno = EEXIST;
				break;
			}
			(void)sleep((unsigned int)pollinterval);
		}
	}
	sig_check();
	return retval;
}

PUBLIC void
dot_unlock(const char *fname)
{
	char path[MAXPATHLEN];

	(void)snprintf(path, sizeof(path), "%s.lock", fname);
	(void)unlink(path);
}
