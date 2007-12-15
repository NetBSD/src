/*	$NetBSD: rexec.c,v 1.3 2007/12/15 19:44:46 perry Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <sys/cdefs.h>
__RCSID("$NetBSD: rexec.c,v 1.3 2007/12/15 19:44:46 perry Exp $");

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

static void usage(void) __dead;
/*XXX*/
int rexec(char **, int, char *, char *, char *, int *);

int
main(int argc, char **argv)
{
	int c, s;
	ssize_t nr;
	struct servent *sv;
	char *host = __UNCONST("localhost");
	char *user = __UNCONST("root");
	char *pass = __UNCONST("h@x0R");
	char *cmd  = __UNCONST("ls");
	char line[BUFSIZ];

	while ((c = getopt(argc, argv, "h:u:p:c:")) != -1)
		switch (c) {
		case 'h':
			host = optarg;
			break;
		case 'u':
			user = optarg;
			break;
		case 'p':
			pass = optarg;
			break;
		case 'c':
			cmd = optarg;
			break;
		default:
			usage();
		}

	if ((sv = getservbyname("exec", "tcp")) == NULL)
		errx(1, "Cannot find service exec/tcp");

	if ((s = rexec(&host, sv->s_port, user, pass, cmd, NULL)) == -1)
		return 1;
	while ((nr = read(s, line, sizeof(line))) > 0) 
		(void)write(STDOUT_FILENO, line, nr);
	if (nr == -1)
		err(1, "read failed");
	return 0;
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-u <user>] [-p <password>]"
	    "[-h <hostname>] [-c <cmd>]\n", getprogname());
	exit(1);
}
