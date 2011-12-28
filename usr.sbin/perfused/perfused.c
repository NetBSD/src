/*  $NetBSD: perfused.c,v 1.17 2011/12/28 17:33:53 manu Exp $ */

/*-
 *  Copyright (c) 2010 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <syslog.h>
#include <ctype.h>
#include <paths.h>
#include <stdarg.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <sysexits.h>
#include <signal.h>
#include <puffs.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <machine/vmparam.h>

#include "../../lib/libperfuse/perfuse_if.h"
#include "perfused.h"

/*
 * This is used for trace file. of course it will not work if
 * we ever mount multiple filesystems in a single perfused, 
 * but it is not sure we will ever want to do that.
 */
struct puffs_usermount *perfuse_mount = NULL;
FILE *perfuse_trace = NULL;

static int access_mount(const char *, uid_t, int);
static void new_mount(int, int);
static int parse_debug(char *);
static void siginfo_handler(int);
static void sigusr1_handler(int);
static int parse_options(int, char **);
static void get_mount_info(int, struct perfuse_mount_info *, int);

/*
 * Flags for new_mount()
 */
#define  PMNT_DEVFUSE	0x0	/* We use /dev/fuse */
#define  PMNT_SOCKPAIR	0x1	/* We use socketpair */
#define  PMNT_DGRAM	0x2	/* We use SOCK_DGRAM sockets */


static int
access_mount(const char *mnt, uid_t uid, int ro)
{
	struct stat st;
	mode_t mode;

	if (uid == 0)
		return 0;

	if (stat(mnt, &st) == -1)
		return -1;
	
	if (st.st_uid != uid)
		return -1;

	mode = S_IRUSR;
	if (!ro)
		mode |= S_IWUSR;
	
	if ((st.st_mode & mode) == mode)
		return 0;

	return -1;
}

static void
get_mount_info(int fd, struct perfuse_mount_info *pmi, int sock_type)
{
	struct perfuse_mount_out *pmo;
	struct sockcred cred;
	int opt;
	char *cp;
	char *source = NULL;
	char *target = NULL;
	char *filesystemtype = NULL;
	long mountflags = 0;
	void *data = NULL;
	char *sock = NULL;

	pmo = (struct perfuse_mount_out *)
		perfuse_recv_early(fd, &cred, sizeof(cred));

	if (pmo == NULL) {
		if (shutdown(fd, SHUT_RDWR) != 0)
			DERR(EX_OSERR, "shutdown failed");
		exit(EX_PROTOCOL);
	}

	/*
	 * We do not need peer creds beyond this point
	 */
	opt = 0;
	if (setsockopt(fd, 0, LOCAL_CREDS, &opt, sizeof(opt)) != 0)
		DWARN("%s: setsockopt LOCAL_CREDS failed", __func__);

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_MISC)
		DPRINTF("perfuse lengths: source = %"PRId32", "
			"target = %"PRId32", filesystemtype = %"PRId32", "
			"data = %"PRId32", sock = %"PRId32"\n", 
			pmo->pmo_source_len, pmo->pmo_target_len, 
			pmo->pmo_filesystemtype_len, pmo->pmo_data_len,
			pmo->pmo_sock_len);
#endif
	cp = (char *)(void *)(pmo + 1);
	
	if (pmo->pmo_source_len != 0) {
		source = cp;
		cp += pmo->pmo_source_len;
	}

	if (pmo->pmo_target_len != 0) {
		target = cp;
		cp += pmo->pmo_target_len;
	}

	if (pmo->pmo_filesystemtype_len != 0) {
		filesystemtype = cp;
		cp += pmo->pmo_filesystemtype_len;
	}

	mountflags = pmo->pmo_mountflags;

	if (pmo->pmo_data_len != 0) {
		data = cp;
		cp += pmo->pmo_data_len;
	}

	if (pmo->pmo_sock_len != 0) {
		sock = cp;
		cp += pmo->pmo_sock_len;
	}

#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_MISC)
		DPRINTF("%s(\"%s\", \"%s\", \"%s\", 0x%lx, \"%s\", \"%s\")\n", 
		__func__, source, target, filesystemtype, 
		mountflags, (const char *)data, sock);
#endif
	pmi->pmi_source = source;
	pmi->pmi_target = target;
	pmi->pmi_filesystemtype = filesystemtype;
	pmi->pmi_mountflags = (int)mountflags;
	pmi->pmi_data = data;

	pmi->pmi_uid = cred.sc_euid;

	/*
	 * Connect to the remote socket if provided ans using SOCK_DGRAM
	 */
	if ((sock_type == SOCK_DGRAM) && sock) {
		const struct sockaddr *sa;
		struct sockaddr_un sun;

		sa = (const struct sockaddr *)(void *)&sun;
		sun.sun_len = sizeof(sun);
		sun.sun_family = AF_LOCAL;
		strcpy(sun.sun_path, sock);

		if (connect(fd, sa, sun.sun_len) != 0)
			DERR(EX_OSERR, "connect \"%s\" failed", sun.sun_path);
	}

	return;
}

static void
new_mount(int fd, int pmnt_flags)
{
	struct puffs_usermount *pu;
	struct perfuse_mount_info pmi;
	struct perfuse_callbacks pc;
	int ro_flag;
	pid_t pid;
	int flags;
	int sock_type;

	pid = (perfuse_diagflags & PDF_FOREGROUND) ? 0 : fork();
	switch(pid) {
	case -1:
		DERR(EX_OSERR, "cannot fork");
		break;
	case 0:
		break;
	default:
		return;
		/* NOTREACHED */
		break;
	}

	/*
	 * Mount information (source, target, mount flags...)
	 */
	sock_type = pmnt_flags & PMNT_DGRAM ? SOCK_DGRAM : SOCK_SEQPACKET;
	get_mount_info(fd, &pmi, sock_type);

	/*
	 * Check that peer owns mountpoint and read (and write) on it?
	 */
	ro_flag = pmi.pmi_mountflags & MNT_RDONLY;
	if (access_mount(pmi.pmi_target, pmi.pmi_uid, ro_flag) != 0)
		DERRX(EX_NOPERM, "insuficient privileges to mount on %s", 
		      pmi.pmi_target);


	/*
	 * Initialize libperfuse, which will initialize libpuffs
	 */
	pc.pc_new_msg = perfuse_new_pb;
	pc.pc_xchg_msg = perfuse_xchg_pb;
	pc.pc_destroy_msg = (perfuse_destroy_msg_fn)puffs_framebuf_destroy;
	pc.pc_get_inhdr = perfuse_get_inhdr;
	pc.pc_get_inpayload = perfuse_get_inpayload;
	pc.pc_get_outhdr = perfuse_get_outhdr;
	pc.pc_get_outpayload = perfuse_get_outpayload;
	pc.pc_umount = perfuse_umount;

	pu = perfuse_init(&pc, &pmi);
	
	puffs_framev_init(pu, perfuse_readframe, perfuse_writeframe, 
			  perfuse_cmpframe, perfuse_gotframe, perfuse_fdnotify);

	if (puffs_framev_addfd(pu, fd, PUFFS_FBIO_READ|PUFFS_FBIO_WRITE) == -1)
		DERR(EX_SOFTWARE, "puffs_framev_addfd failed");

	perfuse_setspecific(pu, (void *)(long)fd);

	setproctitle("perfused %s", pmi.pmi_target);
	(void)kill(getpid(), SIGINFO);		/* This is for -s option */

	perfuse_fs_init(pu);

	/*
	 * Non blocking I/O on /dev/fuse
	 * This must be done after perfuse_fs_init
	 */
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		DERR(EX_OSERR, "fcntl failed");
	if (fcntl(fd, F_SETFL, flags|O_NONBLOCK) != 0)
		DERR(EX_OSERR, "fcntl failed");

	/*
	 * Setup trace file facility
	 */
	perfuse_mount = pu;

	if ((perfuse_trace = fopen(_PATH_VAR_RUN_PERFUSE_TRACE, "w")) == NULL)
		DERR(EX_OSFILE, 
		     "could not open \"%s\"",
		     _PATH_VAR_RUN_PERFUSE_TRACE);

	if (signal(SIGUSR1, sigusr1_handler) != 0)
		DERR(EX_OSERR, "signal failed");

	/*
	 * Hand over control to puffs main loop.
	 */
	if (perfuse_mainloop(pu) != 0)
		DERRX(EX_SOFTWARE, "perfuse_mainloop exit");

	/*	
	 * Normal return after unmount
	 */
	return;
}

static int 
parse_debug(char *optstr)
{
	int retval = PDF_SYSLOG;
	char *opt;
	char *lastp;

	for (opt = strtok_r(optstr, ",", &lastp);
	     opt;
	     opt = strtok_r(NULL, ",", &lastp)) {
		if (strcmp(opt, "fuse") == 0)
			retval |= PDF_FUSE;
		else if (strcmp(opt, "puffs") == 0)
			retval |= PDF_PUFFS;
		else if (strcmp(opt, "dump") == 0)
			retval |= PDF_DUMP;
		else if (strcmp(opt, "fh") == 0)
			retval |= PDF_FH;
		else if (strcmp(opt, "readdir") == 0)
			retval |= PDF_READDIR;
		else if (strcmp(opt, "reclaim") == 0)
			retval |= PDF_RECLAIM;
		else if (strcmp(opt, "requeue") == 0)
			retval |= PDF_REQUEUE;
		else if (strcmp(opt, "sync") == 0)
			retval |= PDF_SYNC;
		else if (strcmp(opt, "misc") == 0)
			retval |= PDF_MISC;
		else if (strcmp(opt, "filename") == 0)
			retval |= PDF_FILENAME;
		else if (strcmp(opt, "reize") == 0)
			retval |= PDF_RESIZE;
		else
			DWARNX("unknown debug flag \"%s\"", opt);
	}

	return retval;
}

/* ARGSUSED0 */
static void
siginfo_handler(int sig)
{
	static int old_flags = 0;
	int swap;

	swap = perfuse_diagflags;
	perfuse_diagflags = old_flags;
	old_flags = swap;

	DWARNX("debug %sabled", old_flags == 0 ? "en" : "dis");

	return;
}

/* ARGSUSED0 */
static void
sigusr1_handler(int sig)
{
	return perfuse_trace_dump(perfuse_mount, perfuse_trace);
}

static int
parse_options(int argc, char **argv)
{
	int ch;
	int foreground = 0;
	int retval = -1;

	perfuse_diagflags = PDF_FOREGROUND | PDF_SYSLOG;
#ifdef PERFUSE_DEBUG
	perfuse_diagflags |= PDF_TRACE;
#endif

	while ((ch = getopt(argc, argv, "d:fsi:")) != -1) {
		switch (ch) {
		case 'd':
			perfuse_diagflags |= parse_debug(optarg);
			break;
		case 's':
			if (signal(SIGINFO, siginfo_handler) != 0)
				DERR(EX_OSERR, "signal failed");
			break;
		case 'f':
			foreground = 1;
			perfuse_diagflags |= PDF_MISC;
			break;
		case 'i':
			retval = atoi(optarg);
			foreground = 1;
			break;
		default:
			DERRX(EX_USAGE, "%s [-fs] [-d classes] [-i fd]", argv[0]);
			break;
		}
	}
	
	if (!foreground)
		perfuse_diagflags &= ~PDF_FOREGROUND;

	return retval; 
}

int
main(int argc, char **argv)
{
	int s;
	int sock_type;
	socklen_t len;

	s = parse_options(argc, argv);

	if (perfuse_diagflags & PDF_SYSLOG)
		openlog("perfused", LOG_NDELAY, LOG_DAEMON);

	if (!(perfuse_diagflags & PDF_FOREGROUND))
		if (daemon(0, 0) != 0)
			DERR(EX_OSERR, "daemon failed");

	if (s != -1) {
		new_mount(s, PMNT_SOCKPAIR);
		exit(0);
	}

	s = perfuse_open_sock();
	
#ifdef PERFUSE_DEBUG
	if (perfuse_diagflags & PDF_MISC)
		DPRINTF("perfused ready\n");
#endif
	len = sizeof(sock_type);
	if (getsockopt(s, SOL_SOCKET, SO_TYPE, &sock_type, &len) != 0)
		DERR(EX_OSERR, "getsockopt SO_TYPE failed");

	switch(sock_type) {
	case SOCK_DGRAM:
		new_mount(s, PMNT_DEVFUSE|PMNT_DGRAM);
		exit(0);
		break;
	case SOCK_SEQPACKET:
		if (listen(s, 0) != 0)
			DERR(EX_OSERR, "listen failed");

		do {
			int fd;
			struct sockaddr_un sun;
			struct sockaddr *sa;
		
			len = sizeof(sun);
			sa = (struct sockaddr *)(void *)&sun;
			if ((fd = accept(s, sa, &len)) == -1)
				DERR(EX_OSERR, "accept failed");

			new_mount(fd, PMNT_DEVFUSE);
		} while (1 /* CONSTCOND */);
		break;
	default:
		DERRX(EX_SOFTWARE, "unexpected so_type %d", sock_type);
		break;
	}
		
	/* NOTREACHED */
	return 0;
}
