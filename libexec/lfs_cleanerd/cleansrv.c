/* $NetBSD: cleansrv.c,v 1.2 2006/05/12 19:33:02 perseant Exp $	 */

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifdef USE_CLIENT_SERVER

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/syslog.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ufs/ufs/inode.h>
#include <ufs/lfs/lfs.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs.h"
#include "fdfs.h"
#include "cleaner.h"

#define LFS_CLEANERD_SOCKDIR	"/tmp/.lfs_cleanerd"
#define LFS_CLEANERD_SOCKFILE	LFS_CLEANERD_SOCKDIR "/socket"

int control_socket = -1;
extern int nfss;
extern struct clfs **fsp;

struct lfs_cleanerd_cmd {
	int  cmd;
	int  len;
	char data[PATH_MAX];
};

void
check_control_socket(void)
{
	int c, r;
	struct lfs_cleanerd_cmd cmd;
	struct clfs **nfsp;

	if (control_socket < 0)
		return;

	while(1) {
		ioctl(control_socket, FIONREAD, &c);
		if (c <= 0)
			return;
		read(control_socket, (char *)&cmd, sizeof(cmd));

		switch(cmd.cmd) {
		    case 'C': /* Add filesystem for cleaning */
			++nfss;
			nfsp = (struct clfs **)realloc(fsp,
						       nfss * sizeof(*fsp));
			if (nfsp == NULL) {
				--nfss;
				break;
			}
			fsp = nfsp;

			fsp[nfss - 1] = (struct clfs *)malloc(sizeof(**fsp));
			if (fsp[nfss - 1] == NULL) {
				syslog(LOG_ERR, "%s: couldn't alloc memory: %m"
				       cmd.data);
				--nfsp;
				break;
			}
				
			if ((r = init_fs(fsp[nfss - 1], cmd.data)) < 0) {
				syslog(LOG_ERR, "%s: couldn't init: "
				       "error code %d", cmd.data, r);
				handle_error(fsp, nfss - 1);
			}
			break;
		    default:
			syslog(LOG_NOTICE, "unknown message type %d", cmd.cmd);
			break;
		}
	}
}

static int
send_fss_to_master(int argc, char **argv)
{
	struct sockaddr_un sun;
	struct lfs_cleanerd_cmd cmd;
	int i, r, s;

	strcpy(sun.sun_path, LFS_CLEANERD_SOCKFILE);
	sun.sun_family = AF_LOCAL;
	sun.sun_len = sizeof(sa_family_t) + 1 + strlen(sun.sun_path);

	s = socket(PF_LOCAL, SOCK_DGRAM, 0);
	if (s < 0) {
		syslog(LOG_DEBUG, "open failed: %m");
		return -1;
	}

	cmd.cmd = 'C';
	for (i = 0; i < argc; i++) {
		strncpy(cmd.data, argv[i], PATH_MAX);
		cmd.len = 2 * sizeof(int) + strlen(cmd.data) + 1;
		r = sendto(s, &cmd, sizeof(cmd), 0, (struct sockaddr *)&sun,
			   sizeof(sun));
		if (r < 0) {
			syslog(LOG_DEBUG, "sendto failed: %m");
			return -1;
		}
	}
	return 0;
}

static void
sig_donothing(int sig)
{
	/* Do nothing */
	dlog("caught sigio");
}

static void
cleanup_socket(void)
{
	if (control_socket >= 0) {
		close(control_socket);
		unlink(LFS_CLEANERD_SOCKFILE);
		rmdir(LFS_CLEANERD_SOCKDIR);
	}
}

void
try_to_become_master(int argc, char **argv)
{
	struct sockaddr_un sun;
	int fd;
	int pid;
	int flags;
	char scratch[80];
	
	if (mkdir(LFS_CLEANERD_SOCKDIR, 0700) < 0) {
		if (errno != EEXIST)
			return;
		pid = 0;
		fd = open("/var/run/lfs_cleanerd.pid", O_RDONLY);
		if (fd >= 0) {
			read(fd, scratch, 80);
			scratch[79] = '\0';
			pid = atoi(scratch);
			if (kill(pid, 0) == 0) {
				send_fss_to_master(argc, argv);
				exit(0);
			}
			close(fd);
		}

		/*
		 * Master is no longer present even though directory
		 * exists.  Remove the socket and proceed.  There is
		 * a race condition here which could result in more than
		 * one master daemon.  That would not be a catastrophe.
		 */
		if (unlink(LFS_CLEANERD_SOCKFILE) != 0)
			return;
	}

	/*
	 * Create the socket and bind it in the namespace
	 */
	control_socket = socket(PF_LOCAL, SOCK_DGRAM, 0);
	strcpy(sun.sun_path, LFS_CLEANERD_SOCKFILE);
	sun.sun_family = AF_LOCAL;
	sun.sun_len = sizeof(sa_family_t) + 1 + strlen(sun.sun_path);
	bind(control_socket, (struct sockaddr *)&sun, sizeof(sun));

	/* Clean up when we leave */
	atexit(cleanup_socket);

	/*
	 * Wake us when there is i/o on this socket.  We don't need
	 * to actually do anything when we get the signal, but we
	 * have to install a signal handler so LFCNSEGWAIT will be
	 * interrupted when data comes in on the socket.
	 */
	fcntl(control_socket, F_SETOWN, getpid());
	flags = fcntl(control_socket, F_GETFL, NULL);
	flags |= O_ASYNC;
	fcntl(control_socket, F_SETFL, flags);
	signal(SIGIO, sig_donothing);
	
	/* And finally record our pid */
	pidfile("lfs_cleanerd");
}
#endif /* USE_CLIENT_SERVER */
