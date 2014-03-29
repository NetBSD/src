/*	$NetBSD: faketalk.c,v 1.19 2014/03/29 19:03:21 dholland Exp $	*/
/*
 * Copyright (c) 1983-2003, Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * + Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * + Neither the name of the University of California, San Francisco nor 
 *   the names of its contributors may be used to endorse or promote 
 *   products derived from this software without specific prior written 
 *   permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: faketalk.c,v 1.19 2014/03/29 19:03:21 dholland Exp $");
#endif /* not lint */

#include "bsd.h"
#include "hunt.h"

#if defined(TALK_43) || defined(TALK_42)

#include <sys/time.h>
#include <sys/wait.h>
#include <ctype.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "talk_ctl.h"

#define TRUE		1
#define FALSE		0

/* defines for fake talk message to announce start of game */
#ifdef TALK_43
#define MASQUERADE	"\"Hunt Game\""
#else
#define MASQUERADE	"HuntGame"
#endif
#define RENDEZVOUS	"hunt-players"
#define ARGV0		"HUNT-ANNOUNCE"

extern char *my_machine_name;
extern char *First_arg, *Last_arg;
extern char **environ;

static void do_announce(char *);
void exorcise(int);

/*
 * exorcise - disspell zombies
 */

void
exorcise(int dummy __unused)
{
	(void) wait(0);
}

/*
 * query the local SMTP daemon to expand the RENDEZVOUS mailing list
 * and fake a talk request to each address thus found.
 */

void
faketalk(void)
{
	struct servent *sp;
	char buf[BUFSIZ];
	FILE *f;
	int service;		/* socket of service */
	struct sockaddr_in des;	/* address of destination */
	char *a, *b;

	(void) signal(SIGCHLD, exorcise);

	if (fork() != 0)
		return;

	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGPIPE, SIG_IGN);

	/*
	 * change argv so that a ps shows ARGV0
	 */
	*environ = NULL;
	for (a = First_arg, b = ARGV0; a < Last_arg; a++) {
		if (*b)
			*a = *b++;
		else
			*a = ' ';
	}

	/*
	 *	initialize "talk"
	 */
	get_local_name(MASQUERADE);
	open_ctl();

	/*
	 *	start fetching addresses
	 */

	if ((sp = getservbyname("smtp", NULL)) == NULL) {
#ifdef LOG
		syslog(LOG_ERR, "faketalk: smtp protocol not supported\n");
#else
		warn("faketalk: smtp protocol not supported");
#endif
		_exit(1);
	}

	memset(&des, 0, sizeof (des));
	des.sin_family = AF_INET;
	des.sin_addr = my_machine_addr;
	des.sin_port = sp->s_port;

	if ((service = socket(des.sin_family, SOCK_STREAM, 0)) < 0) {
#ifdef LOG
		syslog(LOG_ERR, "falktalk:  socket");
#else
		warn("falktalk:  socket");
#endif
		_exit(1);
	}

	if (connect(service, (struct sockaddr *) &des, sizeof(des)) != 0) {
#ifdef LOG
		syslog(LOG_ERR, "faketalk:  connect");
#else
		warn("faketalk:  connect");
#endif
		_exit(1);
	}
	if ((f = fdopen(service, "r")) == NULL) {
#ifdef LOG
		syslog(LOG_ERR, "fdopen failed\n");
#else
		warn("faketalk:  fdopen");
#endif
		_exit(2);
	}

	(void) fgets(buf, BUFSIZ, f);
	(void) snprintf(buf, sizeof(buf),
			"HELO HuntGame@%s\r\n", my_machine_name);
	(void) write(service, buf, strlen(buf));
	(void) fgets(buf, BUFSIZ, f);
	(void) snprintf(buf, sizeof(buf),
			"EXPN %s@%s\r\n", RENDEZVOUS, my_machine_name);
	(void) write(service, buf, strlen(buf));
	while (fgets(buf, BUFSIZ, f) != NULL) {
		char *s, *t;

		if (buf[0] != '2' || buf[1] != '5' || buf[2] != '0')
			break;
		if ((s = strchr(buf + 4, '<')) == NULL)
			s = buf + 4, t = buf + strlen(buf) - 1;
		else {
			s += 1;
			if ((t = strrchr(s, '>')) == NULL)
				t = s + strlen(s) - 1;
			else
				t -= 1;
		}
		while (isspace(*s))
			s += 1;
		if (*s == '\\')
			s += 1;
		while (isspace(*t))
			t -= 1;
		*(t + 1) = '\0';
		do_announce(s);		/* construct and send talk request */
		if (buf[3] == ' ')
			break;
	}
	(void) shutdown(service, 2);
	(void) close(service);
	_exit(0);
}

/*
 * The msg.id's for the invitations on the local and remote machines.
 * These are used to delete the invitations.
 */

static void
do_announce(char *s)
{
	CTL_RESPONSE response;

	get_remote_name(s);	/* setup his_machine_addr, msg.r_name */

#ifdef TALK_43
	msg.ctl_addr = *(struct osockaddr *) &ctl_addr;
	msg.ctl_addr.sa_family = htons(msg.ctl_addr.sa_family);
#else
	msg.ctl_addr = ctl_addr;
	msg.ctl_addr.sin_family = htons(msg.ctl_addr.sin_family);
#endif
	msg.id_num = (int) htonl((uint32_t) -1);	/* an impossible id_num */
	ctl_transact(his_machine_addr, msg, ANNOUNCE, &response);
	if (response.answer != SUCCESS)
		return;

	/*
	 * Have the daemons delete the invitations now that we
	 * have announced.
	 */

	/* we don't care if cleanup doesn't make it. */
	msg.type = DELETE;
	msg.id_num = (int) htonl(response.id_num);
	daemon_addr.sin_addr = his_machine_addr;
	if (sendto(ctl_sockt, &msg, sizeof (msg), 0,
			(struct sockaddr *) &daemon_addr, sizeof(daemon_addr))
			!= sizeof(msg))
		p_error("send delete remote");
}

#else

void
faketalk(void)
{
	return;
}

#endif
