/*	$NetBSD: rexec.c,v 1.11 1999/07/02 15:16:41 simonb Exp $	*/

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)rexec.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: rexec.c,v 1.11 1999/07/02 15:16:41 simonb Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <err.h>

int	rexecoptions;

int	ruserpass __P((const char *, char **, char **));
int	rexec __P((char **, int, char *, char *, char *, int *));

int
rexec(ahost, rport, name, pass, cmd, fd2p)
	char **ahost;
	int rport;
	char *name, *pass, *cmd;
	int *fd2p;
{
	struct sockaddr_in sin, from;
	socklen_t fromlen;
	struct hostent *hp;
	u_short port;
	size_t len;
	unsigned int timo = 1;
	int s, s3;
	char c;

	hp = gethostbyname(*ahost);
	if (hp == 0) {
		warnx("Error resolving %s (%s)", *ahost, hstrerror(h_errno));
		return -1;
	}
	*ahost = hp->h_name;
	(void)ruserpass(hp->h_name, &name, &pass);
retry:
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		warn("Cannot create socket");
		return (-1);
	}
	sin.sin_family = hp->h_addrtype;
	sin.sin_len = sizeof(sin);
	sin.sin_port = rport;
	/* Avoid data corruption from bogus DNS results */
	if (hp->h_length > sizeof(sin.sin_addr))
		len = sizeof(sin.sin_addr);
	else
		len = hp->h_length;
	memcpy(&sin.sin_addr, hp->h_addr, len);
	if (connect(s, (struct sockaddr *)(void *)&sin, sizeof(sin)) == -1) {
		if (errno == ECONNREFUSED && timo <= 16) {
			(void)close(s);
			sleep(timo);
			timo *= 2;
			goto retry;
		}
		warn("Cannot connect to %s", hp->h_name);
		return -1;
	}
	port = 0;
	if (fd2p == 0) {
		(void)write(s, "", 1);
	} else {
		char num[8];
		int s2;
		struct sockaddr_in sin2;
		socklen_t sin2len;

		s2 = socket(AF_INET, SOCK_STREAM, 0);
		if (s2 == -1) {
			warn("Error creating socket");
			(void)close(s);
			return -1;
		}
		listen(s2, 1);
		sin2len = sizeof(sin2);
		if (getsockname(s2, (struct sockaddr *)(void *)&sin2,
		    &sin2len) == -1 || sin2len != sizeof(sin2)) {
			warn("Failed to get socket name");
			(void)close(s2);
			goto bad;
		}
		port = ntohs((u_short)sin2.sin_port);
		(void)snprintf(num, sizeof(num), "%u", port);
		(void)write(s, num, strlen(num)+1);

		fromlen = sizeof(from);
		s3 = accept(s2, (struct sockaddr *)(void *)&from, &fromlen);
		(void)close(s2);
		if (s3 == -1) {
			warn("Error accepting connection");
			port = 0;
			goto bad;
		}
		*fd2p = s3;
	}
	(void)write(s, name, strlen(name) + 1);
	/* should public key encypt the password here */
	(void)write(s, pass, strlen(pass) + 1);
	(void)write(s, cmd, strlen(cmd) + 1);
	if (read(s, &c, 1) != 1) {
		warn("Error reading from %s", *ahost);
		goto bad;
	}
	if (c != 0) {
		while (read(s, &c, 1) == 1) {
			(void) write(2, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad;
	}
	return s;
bad:
	if (port)
		(void)close(*fd2p);
	(void)close(s);
	return -1;
}
