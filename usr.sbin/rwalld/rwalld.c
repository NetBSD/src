/*
 * Copyright (c) 1993 Christopher G. Demetriou
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char rcsid[] = "$Header: /cvsroot/src/usr.sbin/rwalld/Attic/rwalld.c,v 1.2 1993/04/10 17:59:30 cgd Exp $";
#endif

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>
#include <rpc/rpc.h>
#include <rpcsvc/rwall.h>

extern void wallprog_1();

void possess();
void killkids();

main(argc, argv)
	int argc;
	char *argv[];
{
	SVCXPRT *transp;
	int s, salen, nodaemon = 0;
	struct sockaddr_in sa;

	if (argc == 2 && !strcmp(argv[1], "-n"))
		nodaemon = 1;
	if (argc != 1 && !nodaemon) {
		printf("usage: %s [-n]\n", argv[0]);
		exit(1);
	}

	if (geteuid() == 0) {
		struct passwd *pep = getpwnam("nobody");
		if (pep)
			setuid(pep->pw_uid);
		else
			setuid(getuid());
	}

	if (!nodaemon)
		possess();
	signal(SIGCHLD, killkids);

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		exit(1);

	if (bind(s, (struct sockaddr *)&sa, sizeof sa) < 0)
		exit(1);

	salen = sizeof sa;
	if (getsockname(s, (struct sockaddr *)&sa, &salen))
		exit(1);

	(void)pmap_unset(WALLPROG, WALLVERS);
	pmap_set(WALLPROG, WALLVERS, IPPROTO_UDP, ntohs(sa.sin_port));
	if (dup2(s, 0) < 0)
		exit(1);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		(void)fprintf(stderr, "cannot create udp service.\n");
		exit(1);
	}
	(void)pmap_unset(WALLPROG, WALLVERS);
	if (!svc_register(transp, WALLPROG, WALLVERS, wallprog_1, IPPROTO_UDP)) {
		(void)fprintf(stderr, "unable to register (WALLPROG, WALLVERS, udp).\n");
		exit(1);
	}
	svc_run();
	(void)fprintf(stderr, "svc_run returned\n");
	exit(1);

}

void possess()
{
	daemon(0, 0);
}

void killkids()
{
	while(wait4(-1, NULL, WNOHANG, NULL) > 0)
		;
}

void *wallproc_wall_1(s)
	char **s;
{
	/* fork, popen wall with special option, and send the message */
	if (fork() == 0) {
		FILE *pfp;

		pfp = popen("/usr/bin/wall -n", "w");
		if (pfp != NULL) {
			fprintf(pfp, "\007\007%s", *s);
			pclose(pfp);
			exit(0);
		}
	}
}
