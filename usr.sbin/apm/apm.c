/*	$NetBSD: apm.c,v 1.12 2001/09/13 11:05:58 enami Exp $ */

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl.
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

#include <machine/apmvar.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathnames.h"
#include "apm-proto.h"

#define	FALSE 0
#define	TRUE 1

void	usage(void);
void	zzusage(void);
int	do_zzz(const char *, enum apm_action);
int	open_socket(const char *);
int	send_command(int, struct apm_command *, struct apm_reply *);

void
usage(void)
{

	fprintf(stderr,"usage: %s [-v] [-z | -S] [-slmba] [-f socket]\n",
	    getprogname());
	exit(1);
}

void
zzusage(void)
{

	fprintf(stderr,"usage: %s [-z | -S] [-f socket]\n",
	    getprogname());
	exit(1);
}

int
send_command(int fd,
    struct apm_command *cmd,
    struct apm_reply *reply)
{

	/* send a command to the apm daemon */
	cmd->vno = APMD_VNO;

	if (send(fd, cmd, sizeof(*cmd), 0) == sizeof(*cmd)) {
		if (recv(fd, reply, sizeof(*reply), 0) != sizeof(*reply)) {
			warn("invalid reply from APM daemon\n");
			return (1);
		}
	} else {
		warn("invalid send to APM daemon");
		return (1);
	}
	return (0);
}

int
do_zzz(const char *pn, enum apm_action action)
{
	struct apm_command command;
	struct apm_reply reply;
	int fd;

	switch (action) {
	case NONE:
	case SUSPEND:
		command.action = SUSPEND;
		break;
	case STANDBY:
		command.action = STANDBY;
		break;
	default:
		zzusage();
	}

	fd = open_socket(pn);
	if (fd == -1)
		err(1, "cannot open connection to APM daemon");
	printf("Suspending system...\n");
	exit(send_command(fd, &command, &reply));
}

int
open_socket(const char *sockname)
{
	struct sockaddr_un s_un;
	int sock, errr;

	sock = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (sock == -1)
		err(1, "cannot create local socket");

	s_un.sun_family = AF_LOCAL;
	strncpy(s_un.sun_path, sockname, sizeof(s_un.sun_path));
	s_un.sun_len = SUN_LEN(&s_un);
	if (connect(sock, (struct sockaddr *)&s_un, s_un.sun_len) == -1) {
		errr = errno;
		close(sock);
		errno = errr;
		return (-1);
	}
	return (sock);
}

int
main(int argc, char *argv[])
{
	struct apm_command command;
	struct apm_reply reply;
	struct apm_power_info *api = &reply.batterystate;
	char *sockname = _PATH_APM_SOCKET;
	enum apm_action action = NONE;
	int ch, doac, dobstate, domin, dopct, dostatus, fd, nodaemon,
	    rval, verbose;

	doac = dobstate = domin = dopct = dostatus = nodaemon =
	    verbose = FALSE;
	while ((ch = getopt(argc, argv, "lmbvadsSzf:d")) != -1)
		switch (ch) {
		case 'v':
			verbose = TRUE;
			break;
		case 'f':
			sockname = optarg;
			break;
		case 'z':
			if (action != NONE)
				usage();
			action = SUSPEND;
			break;
		case 'S':
			if (action != NONE)
				usage();
			action = STANDBY;
			break;
		case 's':
			if (action != NONE && action != GETSTATUS)
				usage();
			dostatus = TRUE;
			action = GETSTATUS;
			break;
		case 'b':
			if (action != NONE && action != GETSTATUS)
				usage();
			dobstate = TRUE;
			action = GETSTATUS;
			break;
		case 'l':
			if (action != NONE && action != GETSTATUS)
				usage();
			dopct = TRUE;
			action = GETSTATUS;
			break;
		case 'm':
			if (action != NONE && action != GETSTATUS)
				usage();
			domin = TRUE;
			action = GETSTATUS;
			break;
		case 'a':
			if (action != NONE && action != GETSTATUS)
				usage();
			doac = TRUE;
			action = GETSTATUS;
			break;
		case 'd':
			nodaemon = TRUE;
			break;
		case '?':
		default:
			usage();
		}

	if (strcmp(getprogname(), "zzz") == 0)
		exit(do_zzz(sockname, action));

	if (nodaemon)
		fd = -1;
	else
		fd = open_socket(sockname);

	switch (action) {
	case NONE:
		verbose = doac = dopct = domin = dobstate = dostatus = TRUE;
		action = GETSTATUS;
		/* FALLTHROUGH */
	case GETSTATUS:
		if (fd == -1) {
			/* open the device directly and get status */
			fd = open(_PATH_APM_NORMAL, O_RDONLY);
			if (fd == -1) {
				err(1, "cannot contact APM daemon and "
				    "cannot open "
				    _PATH_APM_NORMAL);
			}
			memset(&reply, 0, sizeof(reply));
			if (ioctl(fd, APM_IOC_GETPOWER,
			    &reply.batterystate) == -1)
				err(1, "ioctl(APM_IOC_GETPOWER)");
			goto printval;
		}
		/* FALLTHROUGH */
	case SUSPEND:
	case STANDBY:
		if (nodaemon && fd == -1) {
			fd = open(_PATH_APM_CTLDEV, O_RDWR);
			if (fd == -1)
				err(1, "cannot open APM control device "
				    _PATH_APM_CTLDEV);
			sync();
			sync();
			sleep(1);
			if (ioctl(fd, action == SUSPEND ?
			    APM_IOC_SUSPEND : APM_IOC_STANDBY, 0) == -1)
				err(1, "cannot enter requested power state");
			printf("System will enter %s in a moment.\n",
			    action == SUSPEND ? "suspend mode" :
			    "standby mode");
			exit(0);
		} else if (fd == -1)
			err(1, "cannot contact APM daemon at socket "
			    _PATH_APM_SOCKET);
		command.action = action;
		break;
	default:
		usage();
	}
    
	if ((rval = send_command(fd, &command, &reply)) == 0) {
		switch (action) {
		case GETSTATUS:
printval:
			if (verbose) {
				if (dobstate)
					printf("Battery charge state: %s\n",
					    battstate(api->battery_state));
				if (dopct || domin) {
					printf("Battery remaining: ");
					if (dopct)
						printf("%d percent",
						    api->battery_life);
					if (dopct && domin)
						printf(" (");
					if (domin)
						printf("%d minutes",
						    api->minutes_left);
					if (dopct && domin)
						printf(")");
					printf("\n");
				}
				if (doac)
					printf("A/C adapter state: %s\n",
					    ac_state(api->ac_state));
				if (dostatus)
					printf("Power management enabled\n");
				if (api->nbattery) {
					printf("Number of batteries: %u\n",
					    api->nbattery);
				}
			} else {
				if (dobstate)
					printf("%d\n", api->battery_state);
				if (dopct)
					printf("%d\n", api->battery_life);
				if (domin)
					printf("%d\n", api->minutes_left);
				if (doac)
					printf("%d\n", api->ac_state);
				if (dostatus)
					printf("1\n");
			}
			break;
		default:
			break;
		}
		switch (reply.newstate) {
		case SUSPEND:
			printf("System will enter suspend mode "
			    "in a moment.\n");
			break;
		case STANDBY:
			printf("System will enter standby mode "
			    "in a moment.\n");
			break;
		default:
			break;
		}
	} else
		errx(rval, "cannot get reply from APM daemon\n");

	exit(0);
}
