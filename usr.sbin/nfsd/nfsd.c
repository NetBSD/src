/*	$NetBSD: nfsd.c,v 1.51 2007/07/11 04:59:19 yamt Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1989, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)nfsd.c	8.9 (Berkeley) 3/29/95";
#else
__RCSID("$NetBSD: nfsd.c,v 1.51 2007/07/11 04:59:19 yamt Exp $");
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

#ifdef ISO
#include <netiso/iso.h>
#endif
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
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
#define	syslog(e, s)	fprintf(stderr,(s))
int	debug = 1;
#else
int	debug = 0;
#endif

int	main __P((int, char **));
void	nonfs __P((int));
void	usage __P((void));

static void *
child(void *dummy)
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
 *	-c - support iso cltp clients
 *	-r - reregister with portmapper
 *	-t - support tcp nfs clients
 *	-u - support udp nfs clients
 * followed by "n" which is the number of nfsd threads to create
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct nfsd_args nfsdargs;
	struct addrinfo *ai_udp, *ai_tcp, *ai_udp6, *ai_tcp6, hints;
	struct netconfig *nconf_udp, *nconf_tcp, *nconf_udp6, *nconf_tcp6;
	struct netbuf nb_udp, nb_tcp, nb_udp6, nb_tcp6;
	struct sockaddr_in inetpeer;
	struct sockaddr_in6 inet6peer;
#ifdef ISO
	struct sockaddr_iso isoaddr, isopeer;
#endif
	struct pollfd set[4];
	socklen_t len;
	int ch, cltpflag, connect_type_cnt, i, maxsock, msgsock;
	int nfsdcnt, on = 1, reregister, sock, tcpflag, tcpsock;
	int tcp6sock, ip6flag;
	int tp4cnt, tp4flag, tpipcnt, tpipflag, udpflag, ecode, s;

#define	MAXNFSDCNT	1024
#define	DEFNFSDCNT	 4
	nfsdcnt = DEFNFSDCNT;
	cltpflag = reregister = tcpflag = tp4cnt = tp4flag = tpipcnt = 0;
	tpipflag = udpflag = ip6flag = 0;
	nconf_udp = nconf_tcp = nconf_udp6 = nconf_tcp6 = NULL;
	maxsock = 0;
	tcpsock = tcp6sock = -1;
#ifdef ISO
#define	GETOPT	"6cn:rtu"
#define	USAGE	"[-crtu] [-n num_servers]"
#else
#define	GETOPT	"6n:rtu"
#define	USAGE	"[-rtu] [-n num_servers]"
#endif
	while ((ch = getopt(argc, argv, GETOPT)) != -1) {
		switch (ch) {
		case '6':
			ip6flag = 1;
			s = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
			if (s < 0 && (errno == EPROTONOSUPPORT ||
			    errno == EPFNOSUPPORT || errno == EAFNOSUPPORT))
				ip6flag = 0;
			else
				close(s);
			break;
		case 'n':
			nfsdcnt = atoi(optarg);
			if (nfsdcnt < 1 || nfsdcnt > MAXNFSDCNT) {
				warnx("nfsd count %d; reset to %d", nfsdcnt, DEFNFSDCNT);
				nfsdcnt = DEFNFSDCNT;
			}
			break;
		case 'r':
			reregister = 1;
			break;
		case 't':
			tcpflag = 1;
			break;
		case 'u':
			udpflag = 1;
			break;
#ifdef ISO
		case 'c':
			cltpflag = 1;
			break;
#ifdef notyet
		case 'i':
			tp4cnt = 1;
			break;
		case 'p':
			tpipcnt = 1;
			break;
#endif /* notyet */
#endif /* ISO */
		default:
		case '?':
			usage();
		};
	}
	argv += optind;
	argc -= optind;

	/*
	 * XXX
	 * Backward compatibility, trailing number is the count of daemons.
	 */
	if (argc > 1)
		usage();
	if (argc == 1) {
		nfsdcnt = atoi(argv[0]);
		if (nfsdcnt < 1 || nfsdcnt > MAXNFSDCNT) {
			warnx("nfsd count %d; reset to %d", nfsdcnt, DEFNFSDCNT);
			nfsdcnt = DEFNFSDCNT;
		}
	}

	/*
	 * If none of TCP or UDP are specified, default to UDP only.
	 */
	if (tcpflag == 0 && udpflag == 0)
		udpflag = 1;

	if (debug == 0) {
		daemon(0, 0);
		(void)signal(SIGHUP, SIG_IGN);
		(void)signal(SIGINT, SIG_IGN);
		(void)signal(SIGQUIT, SIG_IGN);
		(void)signal(SIGSYS, nonfs);
	}

	if (udpflag) {
		memset(&hints, 0, sizeof hints);
		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = PF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;

		ecode = getaddrinfo(NULL, "nfs", &hints, &ai_udp);
		if (ecode != 0) {
			syslog(LOG_ERR, "getaddrinfo udp: %s",
			    gai_strerror(ecode));
			exit(1);
		}

		nconf_udp = getnetconfigent("udp");

		if (nconf_udp == NULL)
			err(1, "getnetconfigent udp failed");

		nb_udp.buf = ai_udp->ai_addr;
		nb_udp.len = nb_udp.maxlen = ai_udp->ai_addrlen;
		if (reregister)
			if (!rpcb_set(RPCPROG_NFS, 2, nconf_udp, &nb_udp))
				err(1, "rpcb_set udp failed");
	}

	if (tcpflag) {
		memset(&hints, 0, sizeof hints);
		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = PF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		ecode = getaddrinfo(NULL, "nfs", &hints, &ai_tcp);
		if (ecode != 0) {
			syslog(LOG_ERR, "getaddrinfo tcp: %s",
			    gai_strerror(ecode));
			exit(1);
		}

		nconf_tcp = getnetconfigent("tcp");

		if (nconf_tcp == NULL)
			err(1, "getnetconfigent tcp failed");

		nb_tcp.buf = ai_tcp->ai_addr;
		nb_tcp.len = nb_tcp.maxlen = ai_tcp->ai_addrlen;
		if (reregister)
			if (!rpcb_set(RPCPROG_NFS, 2, nconf_tcp, &nb_tcp))
				err(1, "rpcb_set tcp failed");
	}

	if (udpflag && ip6flag) {
		memset(&hints, 0, sizeof hints);
		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = PF_INET6;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;

		ecode = getaddrinfo(NULL, "nfs", &hints, &ai_udp6);
		if (ecode != 0) {
			syslog(LOG_ERR, "getaddrinfo udp: %s",
			    gai_strerror(ecode));
			exit(1);
		}

		nconf_udp6 = getnetconfigent("udp6");

		if (nconf_udp6 == NULL)
			err(1, "getnetconfigent udp6 failed");

		nb_udp6.buf = ai_udp6->ai_addr;
		nb_udp6.len = nb_udp6.maxlen = ai_udp6->ai_addrlen;
		if (reregister)
			if (!rpcb_set(RPCPROG_NFS, 2, nconf_udp6, &nb_udp6))
				err(1, "rpcb_set udp6 failed");
	}

	if (tcpflag && ip6flag) {
		memset(&hints, 0, sizeof hints);
		hints.ai_flags = AI_PASSIVE;
		hints.ai_family = PF_INET6;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		ecode = getaddrinfo(NULL, "nfs", &hints, &ai_tcp6);
		if (ecode != 0) {
			syslog(LOG_ERR, "getaddrinfo tcp: %s",
			    gai_strerror(ecode));
			exit(1);
		}

		nconf_tcp6 = getnetconfigent("tcp6");

		if (nconf_tcp6 == NULL)
			err(1, "getnetconfigent tcp6 failed");

		nb_tcp6.buf = ai_tcp6->ai_addr;
		nb_tcp6.len = nb_tcp6.maxlen = ai_tcp6->ai_addrlen;
		if (reregister)
			if (!rpcb_set(RPCPROG_NFS, 2, nconf_tcp6, &nb_tcp6))
				err(1, "rpcb_set tcp6 failed");
	}

	openlog("nfsd", LOG_PID, LOG_DAEMON);

	for (i = 0; i < nfsdcnt; i++) {
		pthread_t t;
		int error;

		error = pthread_create(&t, NULL, child, NULL);
		if (error) {
			errno = error;
			syslog(LOG_ERR, "pthread_create: %m");
			exit (1);
		}
	}

	/* If we are serving udp, set up the socket. */
	if (udpflag) {
		if ((sock = socket(ai_udp->ai_family, ai_udp->ai_socktype,
		    ai_udp->ai_protocol)) < 0) {
			syslog(LOG_ERR, "can't create udp socket");
			exit(1);
		}
		if (bind(sock, ai_udp->ai_addr, ai_udp->ai_addrlen) < 0) {
			syslog(LOG_ERR, "can't bind udp addr");
			exit(1);
		}
		if (!rpcb_set(RPCPROG_NFS, 2, nconf_udp, &nb_udp) ||
		    !rpcb_set(RPCPROG_NFS, 3, nconf_udp, &nb_udp)) {
			syslog(LOG_ERR, "can't register with udp portmap");
			exit(1);
		}
		nfsdargs.sock = sock;
		nfsdargs.name = NULL;
		nfsdargs.namelen = 0;
		if (nfssvc(NFSSVC_ADDSOCK, &nfsdargs) < 0) {
			syslog(LOG_ERR, "can't add UDP socket");
			exit(1);
		}
		(void)close(sock);
	}

	if (udpflag &&ip6flag) {
		if ((sock = socket(ai_udp6->ai_family, ai_udp6->ai_socktype,
		    ai_udp6->ai_protocol)) < 0) {
			syslog(LOG_ERR, "can't create udp socket");
			exit(1);
		}
		if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
		    &on, sizeof on) < 0) {
			syslog(LOG_ERR, "can't set v6-only binding for udp6 "
					"socket: %m");
			exit(1);
		}
		if (bind(sock, ai_udp6->ai_addr, ai_udp6->ai_addrlen) < 0) {
			syslog(LOG_ERR, "can't bind udp addr");
			exit(1);
		}
		if (!rpcb_set(RPCPROG_NFS, 2, nconf_udp6, &nb_udp6) ||
		    !rpcb_set(RPCPROG_NFS, 3, nconf_udp6, &nb_udp6)) {
			syslog(LOG_ERR, "can't register with udp portmap");
			exit(1);
		}
		nfsdargs.sock = sock;
		nfsdargs.name = NULL;
		nfsdargs.namelen = 0;
		if (nfssvc(NFSSVC_ADDSOCK, &nfsdargs) < 0) {
			syslog(LOG_ERR, "can't add UDP6 socket");
			exit(1);
		}
		(void)close(sock);
	}

#ifdef ISO
	/* If we are serving cltp, set up the socket. */
	if (cltpflag) {
		if ((sock = socket(AF_ISO, SOCK_DGRAM, 0)) < 0) {
			syslog(LOG_ERR, "can't create cltp socket");
			exit(1);
		}
		memset(&isoaddr, 0, sizeof(isoaddr));
		isoaddr.siso_family = AF_ISO;
		isoaddr.siso_tlen = 2;
		cp = TSEL(&isoaddr);
		*cp++ = (NFS_PORT >> 8);
		*cp = (NFS_PORT & 0xff);
		isoaddr.siso_len = sizeof(isoaddr);
		if (bind(sock,
		    (struct sockaddr *)&isoaddr, sizeof(isoaddr)) < 0) {
			syslog(LOG_ERR, "can't bind cltp addr");
			exit(1);
		}
#ifdef notyet
		/*
		 * XXX
		 * Someday this should probably use "rpcbind", the son of
		 * portmap.
		 */
		if (!pmap_set(RPCPROG_NFS, NFS_VER2, IPPROTO_UDP, NFS_PORT)) {
			syslog(LOG_ERR, "can't register with udp portmap");
			exit(1);
		}
#endif /* notyet */
		nfsdargs.sock = sock;
		nfsdargs.name = NULL;
		nfsdargs.namelen = 0;
		if (nfssvc(NFSSVC_ADDSOCK, &nfsdargs) < 0) {
			syslog(LOG_ERR, "can't add UDP socket");
			exit(1);
		}
		close(sock);
	}
#endif /* ISO */

	/* Now set up the master server socket waiting for tcp connections. */
	on = 1;
	connect_type_cnt = 0;
	if (tcpflag) {
		if ((tcpsock = socket(ai_tcp->ai_family, ai_tcp->ai_socktype,
		    ai_tcp->ai_protocol)) < 0) {
			syslog(LOG_ERR, "can't create tcp socket");
			exit(1);
		}
		if (setsockopt(tcpsock,
		    SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
			syslog(LOG_ERR, "setsockopt SO_REUSEADDR: %m");
		if (bind(tcpsock, ai_tcp->ai_addr, ai_tcp->ai_addrlen) < 0) {
			syslog(LOG_ERR, "can't bind tcp addr");
			exit(1);
		}
		if (listen(tcpsock, 5) < 0) {
			syslog(LOG_ERR, "listen failed");
			exit(1);
		}
		if (!rpcb_set(RPCPROG_NFS, 2, nconf_tcp, &nb_tcp) ||
		    !rpcb_set(RPCPROG_NFS, 3, nconf_tcp, &nb_tcp)) {
			syslog(LOG_ERR, "can't register tcp with rpcbind");
			exit(1);
		}
		set[0].fd = tcpsock;
		set[0].events = POLLIN;
		connect_type_cnt++;
	} else
		set[0].fd = -1;

	if (tcpflag && ip6flag) {
		if ((tcp6sock = socket(ai_tcp6->ai_family, ai_tcp6->ai_socktype,
		    ai_tcp6->ai_protocol)) < 0) {
			syslog(LOG_ERR, "can't create tcp socket");
			exit(1);
		}
		if (setsockopt(tcp6sock,
		    SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
			syslog(LOG_ERR, "setsockopt SO_REUSEADDR: %m");
		if (setsockopt(tcp6sock, IPPROTO_IPV6, IPV6_V6ONLY,
		    &on, sizeof on) < 0) {
			syslog(LOG_ERR, "can't set v6-only binding for tcp6 "
					"socket: %m");
			exit(1);
		}
		if (bind(tcp6sock, ai_tcp6->ai_addr, ai_tcp6->ai_addrlen) < 0) {
			syslog(LOG_ERR, "can't bind tcp6 addr");
			exit(1);
		}
		if (listen(tcp6sock, 5) < 0) {
			syslog(LOG_ERR, "listen failed");
			exit(1);
		}
		if (!rpcb_set(RPCPROG_NFS, 2, nconf_tcp6, &nb_tcp6) ||
		    !rpcb_set(RPCPROG_NFS, 3, nconf_tcp6, &nb_tcp6)) {
			syslog(LOG_ERR, "can't register tcp6 with rpcbind");
			exit(1);
		}
		set[1].fd = tcp6sock;
		set[1].events = POLLIN;
		connect_type_cnt++;
	} else
		set[1].fd = -1;

#ifdef notyet
	/* Now set up the master server socket waiting for tp4 connections. */
	if (tp4flag) {
		if ((tp4sock = socket(AF_ISO, SOCK_SEQPACKET, 0)) < 0) {
			syslog(LOG_ERR, "can't create tp4 socket");
			exit(1);
		}
		if (setsockopt(tp4sock,
		    SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
			syslog(LOG_ERR, "setsockopt SO_REUSEADDR: %m");
		memset(&isoaddr, 0, sizeof(isoaddr));
		isoaddr.siso_family = AF_ISO;
		isoaddr.siso_tlen = 2;
		cp = TSEL(&isoaddr);
		*cp++ = (NFS_PORT >> 8);
		*cp = (NFS_PORT & 0xff);
		isoaddr.siso_len = sizeof(isoaddr);
		if (bind(tp4sock,
		    (struct sockaddr *)&isoaddr, sizeof(isoaddr)) < 0) {
			syslog(LOG_ERR, "can't bind tp4 addr");
			exit(1);
		}
		if (listen(tp4sock, 5) < 0) {
			syslog(LOG_ERR, "listen failed");
			exit(1);
		}
		/*
		 * XXX
		 * Someday this should probably use "rpcbind", the son of
		 * portmap.
		 */
		if (!pmap_set(RPCPROG_NFS, NFS_VER2, IPPROTO_TCP, NFS_PORT)) {
			syslog(LOG_ERR, "can't register tcp with portmap");
			exit(1);
		}
		set[2].fd = tp4sock;
		set[2].events = POLLIN;
		connect_type_cnt++;
	} else
		set[2].fd = -1;

	/* Now set up the master server socket waiting for tpip connections. */
	if (tpipflag) {
		if ((tpipsock = socket(AF_INET, SOCK_SEQPACKET, 0)) < 0) {
			syslog(LOG_ERR, "can't create tpip socket");
			exit(1);
		}
		if (setsockopt(tpipsock,
		    SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
			syslog(LOG_ERR, "setsockopt SO_REUSEADDR: %m");
		inetaddr.sin_family = AF_INET;
		inetaddr.sin_addr.s_addr = INADDR_ANY;
		inetaddr.sin_port = htons(NFS_PORT);
		inetaddr.sin_len = sizeof(inetaddr);
		memset(inetaddr.sin_zero, 0, sizeof(inetaddr.sin_zero));
		if (bind(tpipsock,
		    (struct sockaddr *)&inetaddr, sizeof (inetaddr)) < 0) {
			syslog(LOG_ERR, "can't bind tcp addr");
			exit(1);
		}
		if (listen(tpipsock, 5) < 0) {
			syslog(LOG_ERR, "listen failed");
			exit(1);
		}
		/*
		 * XXX
		 * Someday this should probably use "rpcbind", the son of
		 * portmap.
		 */
		if (!pmap_set(RPCPROG_NFS, NFS_VER2, IPPROTO_TCP, NFS_PORT)) {
			syslog(LOG_ERR, "can't register tcp with portmap");
			exit(1);
		}
		set[3].fd = tpipsock;
		set[3].events = POLLIN;
		connect_type_cnt++;
	} else
		set[3].fd = -1;
#else
	set[2].fd = -1;
	set[3].fd = -1;
#endif /* notyet */

	if (connect_type_cnt == 0)
		exit(0);

	pthread_setname_np(pthread_self(), "master", NULL);

	/*
	 * Loop forever accepting connections and passing the sockets
	 * into the kernel for the mounts.
	 */
	for (;;) {
		if (poll(set, 4, INFTIM) < 1) {
			syslog(LOG_ERR, "poll failed: %m");
			exit(1);
		}

		if (set[0].revents & POLLIN) {
			len = sizeof(inetpeer);
			if ((msgsock = accept(tcpsock,
			    (struct sockaddr *)&inetpeer, &len)) < 0) {
				syslog(LOG_ERR, "accept failed: %m");
				exit(1);
			}
			memset(inetpeer.sin_zero, 0, sizeof(inetpeer.sin_zero));
			if (setsockopt(msgsock, SOL_SOCKET,
			    SO_KEEPALIVE, (char *)&on, sizeof(on)) < 0)
				syslog(LOG_ERR,
				    "setsockopt SO_KEEPALIVE: %m");
			nfsdargs.sock = msgsock;
			nfsdargs.name = (caddr_t)&inetpeer;
			nfsdargs.namelen = sizeof(inetpeer);
			nfssvc(NFSSVC_ADDSOCK, &nfsdargs);
			(void)close(msgsock);
		}

		if (set[1].revents & POLLIN) {
			len = sizeof(inet6peer);
			if ((msgsock = accept(tcp6sock,
			    (struct sockaddr *)&inet6peer, &len)) < 0) {
				syslog(LOG_ERR, "accept failed: %m");
				exit(1);
			}
			if (setsockopt(msgsock, SOL_SOCKET,
			    SO_KEEPALIVE, (char *)&on, sizeof(on)) < 0)
				syslog(LOG_ERR,
				    "setsockopt SO_KEEPALIVE: %m");
			nfsdargs.sock = msgsock;
			nfsdargs.name = (caddr_t)&inet6peer;
			nfsdargs.namelen = sizeof(inet6peer);
			nfssvc(NFSSVC_ADDSOCK, &nfsdargs);
			(void)close(msgsock);
		}

#ifdef notyet
		if (set[2].revents & POLLIN) {
			len = sizeof(isopeer);
			if ((msgsock = accept(tp4sock,
			    (struct sockaddr *)&isopeer, &len)) < 0) {
				syslog(LOG_ERR, "accept failed: %m");
				exit(1);
			}
			if (setsockopt(msgsock, SOL_SOCKET,
			    SO_KEEPALIVE, (char *)&on, sizeof(on)) < 0)
				syslog(LOG_ERR,
				    "setsockopt SO_KEEPALIVE: %m");
			nfsdargs.sock = msgsock;
			nfsdargs.name = (caddr_t)&isopeer;
			nfsdargs.namelen = len;
			nfssvc(NFSSVC_ADDSOCK, &nfsdargs);
			(void)close(msgsock);
		}

		if (set[3].revents & POLLIN) {
			len = sizeof(inetpeer);
			if ((msgsock = accept(tpipsock,
			    (struct sockaddr *)&inetpeer, &len)) < 0) {
				syslog(LOG_ERR, "accept failed: %m");
				exit(1);
			}
			if (setsockopt(msgsock, SOL_SOCKET,
			    SO_KEEPALIVE, (char *)&on, sizeof(on)) < 0)
				syslog(LOG_ERR, "setsockopt SO_KEEPALIVE: %m");
			nfsdargs.sock = msgsock;
			nfsdargs.name = (caddr_t)&inetpeer;
			nfsdargs.namelen = len;
			nfssvc(NFSSVC_ADDSOCK, &nfsdargs);
			(void)close(msgsock);
		}
#endif /* notyet */
	}
}

void
usage()
{
	(void)fprintf(stderr, "usage: nfsd %s\n", USAGE);
	exit(1);
}

void
nonfs(signo)
	int signo;
{
	syslog(LOG_ERR, "missing system call: NFS not available.");
}
