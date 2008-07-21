/* $NetBSD: rwall.c,v 1.17 2008/07/21 14:19:25 lukem Exp $ */

/*
 * Copyright (c) 1988, 1990 Regents of the University of California.
 * All rights reserved.
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

/*
 * Copyright (c) 1993 Christopher G. Demetriou
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
__COPYRIGHT("@(#) Copyright (c) 1988\
 Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "from: @(#)wall.c	5.14 (Berkeley) 3/2/91";
#else
__RCSID("$NetBSD: rwall.c,v 1.17 2008/07/21 14:19:25 lukem Exp $");
#endif
#endif /* not lint */

/*
 * This program is not related to David Wall, whose Stanford Ph.D. thesis
 * is entitled "Mechanisms for Broadcast and Selective Broadcast".
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <err.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpcsvc/rwall.h>

static struct timeval timeout = { 25, 0 };

static char *makemsg(const char *);

int
main(int argc, char **argv)
{
	char *wallhost, res;
	CLIENT *cl;
	char *mbuf;

	setprogname(*argv);

	if ((argc < 2) || (argc > 3)) {
		(void)fprintf(stderr,
		    "Usage: %s hostname <file>\n", getprogname());
		return 1;
	}

	wallhost = argv[1];

	/*
	 * Create client "handle" used for calling MESSAGEPROG on the
	 * server designated on the command line. We tell the rpc package
	 * to use the "tcp" protocol when contacting the server.
	*/
	cl = clnt_create(wallhost, WALLPROG, WALLVERS, "udp");
	if (cl == NULL) {
		/*
		 * Couldn't establish connection with server.
		 * Print error message and die.
		 */
		clnt_pcreateerror(wallhost);
		return 1;
	}

	mbuf = makemsg(argv[2]);

	if (clnt_call(cl, WALLPROC_WALL, xdr_wrapstring, (void *)&mbuf,
	    xdr_void, &res, timeout) != RPC_SUCCESS) {
		free(mbuf);
		/*
		 * An error occurred while calling the server. 
		 * Print error message and die.
		 */
		clnt_perror(cl, wallhost);
		return 1;
	}
	free(mbuf);
	return 0;
}

static char *
makemsg(const char *fname)
{
	struct tm *lt;
	struct passwd *pw;
	struct stat sbuf;
	time_t now;
	size_t mbufsize;
	char *mbuf;
	FILE *fp;
	int fd;
	const char *whom;
	const char *tty;
	char tmpname[MAXPATHLEN], lbuf[BUFSIZ], hostname[MAXHOSTNAMELEN + 1];

	(void)snprintf(tmpname, sizeof(tmpname), "%s/wall.XXXXXX", _PATH_TMP);
	if ((fd = mkstemp(tmpname)) == -1 || (fp = fdopen(fd, "r+")) == NULL)
		err(1, "Can't open temporary file");
	(void)unlink(tmpname);

	if (!(whom = getlogin()))
		whom = (pw = getpwuid(getuid())) ? pw->pw_name : "???";
	(void)gethostname(hostname, sizeof(hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	(void)time(&now);
	lt = localtime(&now);

	/*
	 * all this stuff is to blank out a square for the message;
	 * we wrap message lines at column 79, not 80, because some
	 * terminals wrap after 79, some do not, and we can't tell.
	 * Which means that we may leave a non-blank character
	 * in column 80, but that can't be helped.
	 */
	(void)fprintf(fp, "Remote Broadcast Message from %s@%s\n",
	    whom, hostname);
	tty = ttyname(STDERR_FILENO);
	if (tty == NULL)
		tty = "??";
	(void)fprintf(fp, "        (%s) at %d:%02d ...\n", tty, lt->tm_hour,
	    lt->tm_min);

	(void)putc('\n', fp);

	if (fname && !(freopen(fname, "r", stdin)))
		err(1, "Can't open `%s'", fname);
	while (fgets(lbuf, sizeof(lbuf), stdin))
		(void)fputs(lbuf, fp);
	rewind(fp);

	if (fstat(fd, &sbuf) == -1)
		err(1, "Can't stat temporary file.");
	mbufsize = (size_t)sbuf.st_size;
	if (!(mbuf = malloc((u_int)mbufsize)))
		err(1, "malloc");
	if (fread(mbuf, sizeof(*mbuf), mbufsize, fp) != mbufsize)
		err(1, "Can't read temporary file.");
	(void)close(fd);
	return mbuf;
}
