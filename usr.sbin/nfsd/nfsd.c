/*	$NetBSD: nfsd.c,v 1.61.2.1 2014/08/20 00:05:10 tls Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1989, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)nfsd.c	8.9 (Berkeley) 3/29/95";
#else
__RCSID("$NetBSD: nfsd.c,v 1.61.2.1 2014/08/20 00:05:10 tls Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <poll.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <paths.h>
#include <pwd.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <netdb.h>

/* Global defs */
#ifdef DEBUG
#define	syslog(e, s, args...)						\
do {									\
    fprintf(stderr,(s), ## args);					\
    fprintf(stderr, "\n");						\
} while (/*CONSTCOND*/0)
static int	debug = 1;
#else
static int	debug = 0;
#endif

static void	nonfs(int);
__dead static void	usage(void);

static void *
worker(void *dummy)
{
	struct	nfsd_srvargs nsd;
	int nfssvc_flag;

	pthread_setname_np(pthread_self(), "slave", NULL);
	nfssvc_flag = NFSSVC_NFSD;
	memset(&nsd, 0, sizeof(nsd));
	while (nfssvc(nfssvc_flag, &nsd) < 0) {
		if (errno != ENEEDAUTH) {
			syslog(LOG_ERR, "nfssvc: %m");
			exit(1);
		}
		nfssvc_flag = NFSSVC_NFSD | NFSSVC_AUTHINFAIL;
	}

	return NULL;
}

struct conf {
	struct addrinfo *ai;
	struct netconfig *nc;
	struct netbuf nb;
	struct pollfd pfd;
};

#define NFS_UDP4	0
#define NFS_TCP4	1
#define NFS_UDP6	2
#define NFS_TCP6	3

static int cfg_family[] = { PF_INET, PF_INET, PF_INET6, PF_INET6 };
static const char *cfg_netconf[] = { "udp", "tcp", "udp6", "tcp6" };
static int cfg_socktype[] = {
    SOCK_DGRAM, SOCK_STREAM, SOCK_DGRAM, SOCK_STREAM };
static int cfg_protocol[] = {
    IPPROTO_UDP, IPPROTO_TCP, IPPROTO_UDP, IPPROTO_TCP };

static int
tryconf(struct conf *cfg, int t, int reregister)
{
	struct addrinfo hints;
	int ecode;

	memset(cfg, 0, sizeof(*cfg));
	memset(&hints, 0, sizeof hints);
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = cfg_family[t];
	hints.ai_socktype = cfg_socktype[t];
	hints.ai_protocol = cfg_protocol[t];

	ecode = getaddrinfo(NULL, "nfs", &hints, &cfg->ai);
	if (ecode != 0) {
		syslog(LOG_ERR, "getaddrinfo %s: %s", cfg_netconf[t],
		    gai_strerror(ecode));
		return -1;
	}

	cfg->nc = getnetconfigent(cfg_netconf[t]);

	if (cfg->nc == NULL) {
		syslog(LOG_ERR, "getnetconfigent %s failed: %m",
		    cfg_netconf[t]);
		goto out;
	}

	cfg->nb.buf = cfg->ai->ai_addr;
	cfg->nb.len = cfg->nb.maxlen = cfg->ai->ai_addrlen;
	if (reregister)
		if (!rpcb_set(RPCPROG_NFS, 2, cfg->nc, &cfg->nb)) {
			syslog(LOG_ERR, "rpcb_set %s failed", cfg_netconf[t]);
			goto out1;
		}
	return 0;
out1:
	freenetconfigent(cfg->nc);
	cfg->nc = NULL;
out:
	freeaddrinfo(cfg->ai);
	cfg->ai = NULL;
	return -1;
}

static int
setupsock(struct conf *cfg, struct pollfd *set, int p)
{
	int sock;
	struct nfsd_args nfsdargs;
	struct addrinfo *ai = cfg->ai;
	int on = 1;

	sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

	if (sock == -1) {
		syslog(LOG_ERR, "can't create %s socket: %m", cfg_netconf[p]);
		return -1;
	}
	if (cfg_family[p] == PF_INET6) {
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &on,
		    sizeof(on)) == -1) {
			syslog(LOG_ERR, "can't set v6-only binding for %s "
			    "socket: %m", cfg_netconf[p]);
			goto out;
		}
	}

	if (cfg_protocol[p] == IPPROTO_TCP) {
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on,
		    sizeof(on)) == -1) {
			syslog(LOG_ERR, "setsockopt SO_REUSEADDR for %s: %m",
			    cfg_netconf[p]);
			goto out;
		}
	}

	if (bind(sock, ai->ai_addr, ai->ai_addrlen) == -1) {
		syslog(LOG_ERR, "can't bind %s addr: %m", cfg_netconf[p]);
		goto out;
	}

	if (cfg_protocol[p] == IPPROTO_TCP) {
		if (listen(sock, 5) == -1) {
			syslog(LOG_ERR, "listen failed");
			goto out;
		}
	}

	if (!rpcb_set(RPCPROG_NFS, 2, cfg->nc, &cfg->nb) ||
	    !rpcb_set(RPCPROG_NFS, 3, cfg->nc, &cfg->nb)) {
		syslog(LOG_ERR, "can't register with %s portmap",
		    cfg_netconf[p]);
		goto out;
	}


	if (cfg_protocol[p] == IPPROTO_TCP)
		set->fd = sock;
	else {
		nfsdargs.sock = sock;
		nfsdargs.name = NULL;
		nfsdargs.namelen = 0;
		if (nfssvc(NFSSVC_ADDSOCK, &nfsdargs) < 0) {
			syslog(LOG_ERR, "can't add %s socket", cfg_netconf[p]);
			goto out;
		}
		(void)close(sock);
	}
	return 0;
out:
	(void)close(sock);
	return -1;
}

/*
 * The functions daemon2_fork() and daemon2_detach() below provide
 * functionality similar to daemon(3) but split into two phases.
 * daemon2_fork() is called early, before creating resources that
 * cannot be inherited across a fork, such as threads or kqueues.
 * When the daemon is ready to provide service, daemon2_detach()
 * is called to complete the daemonization and signal the parent
 * process to exit.
 *
 * These functions could potentially be moved to a library and 
 * shared by other daemons.
 *
 * The return value from daemon2_fork() is a file descriptor to
 * be passed as the first argument to daemon2_detach().
 */

static int
daemon2_fork(void)
{
	 int i;
	 int fd;
	 int r;
	 pid_t pid;
	 int detach_msg_pipe[2];

	 /*
	  * Set up a pipe for singalling the parent, making sure the
	  * write end does not get allocated one of the file
	  * descriptors that may be closed in daemon2_detach().  The
	  * read end does not need such protection.
	  */
	 for (i = 0; i < 3; i++) {
		 r = pipe2(detach_msg_pipe, O_CLOEXEC|O_NOSIGPIPE);
		 if (r < 0)
			 return -1;
		 if (detach_msg_pipe[1] <= STDERR_FILENO &&
		     (fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
			 (void)dup2(fd, detach_msg_pipe[0]);
			 (void)dup2(fd, detach_msg_pipe[1]);
			 if (fd > STDERR_FILENO)
				 (void)close(fd);
			 continue;
		 }
		 break;
	 }

	 pid = fork();
	 switch (pid) {
	 case -1:
		 return -1;
	 case 0:
		 /* child */
		 (void)close(detach_msg_pipe[0]);
		 return detach_msg_pipe[1];
	 default:
		 break;
	}

	/* Parent */
	(void)close(detach_msg_pipe[1]);

	for (;;) {
		ssize_t nread;
		char dummy;
		nread = read(detach_msg_pipe[0], &dummy, 1);
		if (nread < 0) {
			if (errno == EINTR)
				continue;
			_exit(1);
		} else if (nread == 0) {
			_exit(1);
		} else { /* nread > 0 */
			_exit(0);
		}
	}
}

static int
daemon2_detach(int parentfd, int nochdir, int noclose)
{
	int fd;

	if (setsid() == -1)
		return -1;

	if (!nochdir)
		(void)chdir("/");

	if (!noclose && (fd = open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
		(void)dup2(fd, STDIN_FILENO);
		(void)dup2(fd, STDOUT_FILENO);
		(void)dup2(fd, STDERR_FILENO);
		if (fd > STDERR_FILENO)
			(void)close(fd);
	}

	while (1) {
		ssize_t r = write(parentfd, "", 1);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			else if (errno == EPIPE)
				break;
			else
				return -1;
		} else if (r == 0) {
			/* Should not happen */
			return -1;
		} else {
			break;
		}
	}

	(void)close(parentfd);

	return 0;
}

/*
 * Nfs server daemon mostly just a user context for nfssvc()
 *
 * 1 - do file descriptor and signal cleanup
 * 2 - create the nfsd thread(s)
 * 3 - create server socket(s)
 * 4 - register socket with portmap
 *
 * For connectionless protocols, just pass the socket into the kernel via
 * nfssvc().
 * For connection based sockets, loop doing accepts. When you get a new
 * socket from accept, pass the msgsock into the kernel via nfssvc().
 * The arguments are:
 *	-r - reregister with portmapper
 *	-t - support only tcp nfs clients
 *	-u - support only udp nfs clients
 *	-n num how many threads to create.
 *	-4 - use only ipv4
 *	-6 - use only ipv6
 */
int
main(int argc, char *argv[])
{
	struct conf cfg[4];
	struct pollfd set[__arraycount(cfg)];
	int ch, connect_type_cnt;
	size_t i, nfsdcnt;
	int reregister;
	int tcpflag, udpflag;
	int ip6flag, ip4flag;
	int s, compat;
	int parent_fd = -1;

#define	DEFNFSDCNT	 4
	nfsdcnt = DEFNFSDCNT;
	compat = reregister = 0;
	tcpflag = udpflag = 1;
	ip6flag = ip4flag = 1;
#define	GETOPT	"46n:rtu"
#define	USAGE	"[-46rtu] [-n num_servers]"
	while ((ch = getopt(argc, argv, GETOPT)) != -1) {
		switch (ch) {
		case '6':
			ip6flag = 1;
			ip4flag = 0;
			s = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
			if (s < 0 && (errno == EPROTONOSUPPORT ||
			    errno == EPFNOSUPPORT || errno == EAFNOSUPPORT))
				ip6flag = 0;
			else
				close(s);
			break;
		case '4':
			ip6flag = 0;
			ip4flag = 1;
			s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
			if (s < 0 && (errno == EPROTONOSUPPORT ||
			    errno == EPFNOSUPPORT || errno == EAFNOSUPPORT))
				ip4flag = 0;
			else
				close(s);
			break;
		case 'n':
			nfsdcnt = atoi(optarg);
			if (nfsdcnt < 1) {
				warnx("nfsd count %zu; reset to %d", nfsdcnt,
				    DEFNFSDCNT);
				nfsdcnt = DEFNFSDCNT;
			}
			break;
		case 'r':
			reregister = 1;
			break;
		case 't':
			compat |= 2;
			tcpflag = 1;
			udpflag = 0;
			break;
		case 'u':
			compat |= 1;
			tcpflag = 0;
			udpflag = 1;
			break;
		default:
		case '?':
			usage();
		}
	}
	argv += optind;
	argc -= optind;

	if (compat == 3) {
		warnx("Old -tu options detected; enabling both udp and tcp.");
		warnx("This is the default behavior now and you can remove");
		warnx("all options.");
		tcpflag = udpflag = 1;
		if (ip6flag == 1 && ip4flag == 0)
			ip4flag = 1;
	}

	if (debug == 0) {
		parent_fd = daemon2_fork();
	}

	openlog("nfsd", LOG_PID, LOG_DAEMON);

	for (i = 0; i < __arraycount(cfg); i++) {
		if (ip4flag == 0 && cfg_family[i] == PF_INET)
			continue;
		if (ip6flag == 0 && cfg_family[i] == PF_INET6)
			continue;
		if (tcpflag == 0 && cfg_protocol[i] == IPPROTO_TCP)
			continue;
		if (udpflag == 0 && cfg_protocol[i] == IPPROTO_UDP)
			continue;
		tryconf(&cfg[i], i, reregister);
	}

	for (i = 0; i < nfsdcnt; i++) {
		pthread_t t;
		int error;

		error = pthread_create(&t, NULL, worker, NULL);
		if (error) {
			errno = error;
			syslog(LOG_ERR, "pthread_create: %m");
			exit(1);
		}
	}

	connect_type_cnt = 0;
	for (i = 0; i < __arraycount(cfg); i++) {
		set[i].fd = -1;
		set[i].events = POLLIN;
		set[i].revents = 0;

		if (cfg[i].nc == NULL)
			continue;

		setupsock(&cfg[i], &set[i], i);
		if (set[i].fd != -1)
			connect_type_cnt++;

	}

	if (connect_type_cnt == 0)
		exit(0);

	pthread_setname_np(pthread_self(), "master", NULL);

	if (debug == 0) {
		daemon2_detach(parent_fd, 0, 0);
		(void)signal(SIGHUP, SIG_IGN);
		(void)signal(SIGINT, SIG_IGN);
		(void)signal(SIGQUIT, SIG_IGN);
		(void)signal(SIGSYS, nonfs);
	}

	/*
	 * Loop forever accepting connections and passing the sockets
	 * into the kernel for the mounts.
	 */
	for (;;) {
		if (poll(set, __arraycount(set), INFTIM) == -1) {
			syslog(LOG_ERR, "poll failed: %m");
			exit(1);
		}

		for (i = 0; i < __arraycount(set); i++) {
			struct nfsd_args nfsdargs;
			struct sockaddr_storage ss;
			socklen_t len;
			int msgsock;
			int on = 1;

			if ((set[i].revents & POLLIN) == 0)
				continue;
			len = sizeof(ss);
			if ((msgsock = accept(set[i].fd,
			    (struct sockaddr *)&ss, &len)) == -1) {
				int serrno = errno;
				syslog(LOG_ERR, "accept failed: %m");
				if (serrno == EINTR || serrno == ECONNABORTED)
					continue;
				exit(1);
			}
			if (setsockopt(msgsock, SOL_SOCKET, SO_KEEPALIVE, &on,
			    sizeof(on)) == -1)
				syslog(LOG_ERR, "setsockopt SO_KEEPALIVE: %m");
			nfsdargs.sock = msgsock;
			nfsdargs.name = (void *)&ss;
			nfsdargs.namelen = len;
			nfssvc(NFSSVC_ADDSOCK, &nfsdargs);
			(void)close(msgsock);
		}
	}
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s %s\n", getprogname(), USAGE);
	exit(1);
}

static void
nonfs(int signo)
{
	syslog(LOG_ERR, "missing system call: NFS not available.");
}
