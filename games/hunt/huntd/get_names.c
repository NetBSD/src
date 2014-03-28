/*	$NetBSD: get_names.c,v 1.11 2014/03/28 17:49:11 apb Exp $	*/
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
__RCSID("$NetBSD: get_names.c,v 1.11 2014/03/28 17:49:11 apb Exp $");
#endif /* not lint */

#include "bsd.h"

#if defined(TALK_43) || defined(TALK_42)

#include <sys/param.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hunt.h"
#include "talk_ctl.h"

static char hostname[MAXHOSTNAMELEN + 1];
char *my_machine_name;

/*
 * Determine the local user and machine
 */
void
get_local_name(char *my_name)
{
	struct hostent *hp;
	struct servent *sp;

	/* Load these useful values into the standard message header */
	msg.id_num = 0;
	(void) strncpy(msg.l_name, my_name, NAME_SIZE);
	msg.l_name[NAME_SIZE - 1] = '\0';
	msg.r_tty[0] = '\0';
	msg.pid = getpid();
#ifdef TALK_43
	msg.vers = TALK_VERSION;
	msg.addr.sa_family = htons(AF_INET);
	msg.ctl_addr.sa_family = htons(AF_INET);
#else
	msg.addr.sin_family = htons(AF_INET);
	msg.ctl_addr.sin_family = htons(AF_INET);
#endif

	(void)gethostname(hostname, sizeof (hostname));
	hostname[sizeof(hostname) - 1] = '\0';
	my_machine_name = hostname;
	/* look up the address of the local host */
	hp = gethostbyname(my_machine_name);
	if (hp == (struct hostent *) 0) {
#ifdef LOG
		syslog(LOG_ERR,
		    "This machine doesn't exist. Boy, am I confused!");
#else
		perror("This machine doesn't exist. Boy, am I confused!");
#endif
		exit(1);
	}
	memcpy(&my_machine_addr, hp->h_addr, hp->h_length);
	/* find the daemon portal */
#ifdef TALK_43
	sp = getservbyname("ntalk", "udp");
#else
	sp = getservbyname("talk", "udp");
#endif
	if (sp == 0) {
#ifdef LOG
		syslog(LOG_ERR, "This machine doesn't support talk");
#else
		perror("This machine doesn't support talk");
#endif
		exit(1);
	}
	daemon_port = sp->s_port;
}

/*
 * Determine the remote user and machine
 */
int
get_remote_name(char *his_address)
{
	char *his_name;
	char *his_machine_name;
	char *ptr;
	struct hostent *hp;

	/* check for, and strip out, the machine name of the target */
	for (ptr = his_address; *ptr != '\0' && *ptr != '@' && *ptr != ':'
					&& *ptr != '!' && *ptr != '.'; ptr++)
		continue;
	if (*ptr == '\0') {
		/* this is a local to local talk */
		his_name = his_address;
		his_machine_name = my_machine_name;
	} else {
		if (*ptr == '@') {
			/* user@host */
			his_name = his_address;
			his_machine_name = ptr + 1;
		} else {
			/* host.user or host!user or host:user */
			his_name = ptr + 1;
			his_machine_name = his_address;
		}
		*ptr = '\0';
	}
	/* Load these useful values into the standard message header */
	(void) strncpy(msg.r_name, his_name, NAME_SIZE);
	msg.r_name[NAME_SIZE - 1] = '\0';

	/* if he is on the same machine, then simply copy */
	if (memcmp(&his_machine_name, &my_machine_name,
						sizeof(his_machine_name)) == 0)
		memcpy(&his_machine_addr, &my_machine_addr,
						sizeof(his_machine_addr));
	else {
		/* look up the address of the recipient's machine */
		hp = gethostbyname(his_machine_name);
		if (hp == (struct hostent *) 0)
			return 0;			/* unknown host */
		memcpy(&his_machine_addr, hp->h_addr, hp->h_length);
	}
	return 1;
}
#endif
