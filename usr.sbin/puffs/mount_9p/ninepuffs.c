/*	$NetBSD: ninepuffs.c,v 1.34 2022/02/10 09:29:39 ozaki-r Exp $	*/

/*
 * Copyright (c) 2007  Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * 9puffs: access a 9P file server as a vfs via puffs
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ninepuffs.c,v 1.34 2022/02/10 09:29:39 ozaki-r Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include <netinet/in.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>
#include <puffs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ninepuffs.h"
#include "nineproto.h"

#define DEFPORT_9P "564" /* "9pfs", but don't depend on it being in services */

__dead static void
usage(void)
{

	fprintf(stderr, "usage: %s [-46su] [-o mntopts] [-p port] "
	    "[user@]server[:path] mountpoint\n", getprogname());
	fprintf(stderr, "       %s -c [-su] [-o mntopts] device mountpoint\n",
	    getprogname());
	exit(1);
}

/*
 * TCP connection to 9P file server.
 * Return connected socket or exit with error.
 */
static int
serverconnect(const char *hostname, const char *portname, int family)
{
	int ret;

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;

	if (portname == NULL) {
		portname = DEFPORT_9P;
		hints.ai_flags |= AI_NUMERICSERV;
	}

	struct addrinfo *ai0;
	ret = getaddrinfo(hostname, portname, &hints, &ai0);
	if (ret != 0)
		errx(EXIT_FAILURE, "%s", gai_strerror(ret));

	int s = -1;
	const char *cause = NULL;
	for (struct addrinfo *ai = ai0; ai != NULL; ai = ai->ai_next) {
		s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (s < 0) {
			cause = "socket";
			continue;
		}

		const int opt = 1;
		ret = setsockopt(s, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
		if (ret < 0) {
			cause = "SO_NOSIGPIPE";
			continue;
		}

		ret = connect(s, ai->ai_addr, ai->ai_addrlen);
		if (ret < 0) {
			close(s);
			s = -1;
			cause = "connect";
			continue;
		}
	}

	if (s < 0)
		err(EXIT_FAILURE, "%s", cause);

	freeaddrinfo(ai0);
	return s;
}

static int
open_cdev(const char *path)
{
	int s;

	s = open(path, O_RDWR, 0);
	if (s == -1)
		err(1, "%s", path);
	return s;
}

int
main(int argc, char *argv[])
{
	struct puffs9p p9p;
	struct puffs_usermount *pu;
	struct puffs_ops *pops;
	struct puffs_node *pn_root;
	mntoptparse_t mp;
	int family;
	const char *user, *srvhost, *srvpath;
	char *p;
	const char *port;
	int mntflags, pflags, ch;
	int detach;
	int protover;
	int server;
	bool cachename = false;

	setprogname(argv[0]);

	if (argc < 2)
		usage();

	mntflags = pflags = 0;
	detach = 1;
#ifdef INET6
	family = AF_UNSPEC;
#else
	family = AF_INET;
#endif
	port = NULL;
	protover = P9PROTO_VERSION;
	server = P9P_SERVER_TCP;

	while ((ch = getopt(argc, argv, "46cCo:p:su")) != -1) {
		switch (ch) {
		case '4':
			family = AF_INET;
			break;
		case '6':
#ifdef INET6
			family = AF_INET6;
			break;
#else
			errno = EPFNOSUPPORT;
			err(EXIT_FAILURE, "IPv6");
			/* NOTREACHED */
#endif
		case 'c':
			server = P9P_SERVER_CDEV;
			break;
		case 'C':
			cachename = true;
			break;
		case 'o':
			mp = getmntopts(optarg, puffsmopts, &mntflags, &pflags);
			if (mp == NULL)
				err(1, "getmntopts");
			freemntopts(mp);
			break;
		case 'p':
			port = optarg;
			break;
		case 's':
			detach = 0;
			break;
		case 'u':
			protover = P9PROTO_VERSION_U;
			break;
		default:
			usage();
			/*NOTREACHED*/
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	if (pflags & PUFFS_FLAG_OPDUMP)
		detach = 0;
	pflags |= PUFFS_KFLAG_WTCACHE | PUFFS_KFLAG_IAONDEMAND;

	if (!cachename)
		pflags |= PUFFS_KFLAG_NOCACHE_NAME;

	PUFFSOP_INIT(pops);

	PUFFSOP_SET(pops, puffs9p, fs, unmount);
	PUFFSOP_SETFSNOP(pops, sync);
	PUFFSOP_SETFSNOP(pops, statvfs);

	PUFFSOP_SET(pops, puffs9p, node, lookup);
	PUFFSOP_SET(pops, puffs9p, node, readdir);
	PUFFSOP_SET(pops, puffs9p, node, getattr);
	PUFFSOP_SET(pops, puffs9p, node, setattr);
	PUFFSOP_SET(pops, puffs9p, node, create);
	PUFFSOP_SET(pops, puffs9p, node, open);
	PUFFSOP_SET(pops, puffs9p, node, mkdir);
	PUFFSOP_SET(pops, puffs9p, node, remove);
	PUFFSOP_SET(pops, puffs9p, node, rmdir);
	PUFFSOP_SET(pops, puffs9p, node, read);
	PUFFSOP_SET(pops, puffs9p, node, write);
	PUFFSOP_SET(pops, puffs9p, node, rename);
	PUFFSOP_SET(pops, puffs9p, node, inactive);
	PUFFSOP_SET(pops, puffs9p, node, reclaim);
#if 0
	PUFFSOP_SET(pops, puffs9p, node, mknod);
#endif

	pu = puffs_init(pops, argv[0], "9p", &p9p, pflags);
	if (pu == NULL)
		err(1, "puffs_init");

	memset(&p9p, 0, sizeof(p9p));
	p9p.maxreq = P9P_DEFREQLEN;
	p9p.nextfid = 1;
	p9p.protover = protover;
	p9p.server = server;

	/* user@ */
	if ((p = strchr(argv[0], '@')) != NULL) {
		*p = '\0';
		srvhost = p+1;
		user = argv[0];
	} else {
		struct passwd *pw;

		srvhost = argv[0];
		pw = getpwuid(getuid());
		if (pw == NULL)
			err(1, "getpwuid");
		user = pw->pw_name;
	}

	/* [host] or [host]:/path with square brackets around host */
	if (server == P9P_SERVER_TCP && *srvhost == '[') {
		++srvhost;
		if ((p = strchr(srvhost, ']')) == NULL)
			errx(EXIT_FAILURE, "Missing bracket after the host name");
		*p++ = '\0';
		if (*p == '\0')		/* [host] */
			srvpath = "/";
		else if (*p == ':')	/* [host]:path */
			srvpath = p+1;
		else			/* [foo]bar */
			errx(EXIT_FAILURE, "Invalid brackets in the host name");

	} else { /* host or host:/path without brackets around host */
		if ((p = strchr(srvhost, ':')) != NULL) {
			*p = '\0';
			srvpath = p+1;
		} else {
			srvpath = "/";
		}
	}

	if (*srvpath == '\0')
		errx(1, "Empty path");
	if (*srvpath != '/')
		errx(1, "%s: Not an absolute path", srvpath);


	if (p9p.server == P9P_SERVER_TCP) {
		p9p.servsock = serverconnect(srvhost, port, family);
	} else {
		/* path to a vio9p(4) device, e.g., /dev/vio9p0 */
		p9p.servsock = open_cdev(argv[0]);
	}

	if ((pn_root = p9p_handshake(pu, user, srvpath)) == NULL) {
		close(p9p.servsock);
		puffs_exit(pu, 1);
		exit(1);
	}

	puffs_framev_init(pu, p9pbuf_read, p9pbuf_write, p9pbuf_cmp, NULL,
	    puffs_framev_unmountonclose);
	if (puffs_framev_addfd(pu, p9p.servsock,
	    PUFFS_FBIO_READ | PUFFS_FBIO_WRITE) == -1)
		err(1, "puffs_framebuf_addfd");

	if (detach)
		if (puffs_daemon(pu, 1, 1) == -1)
			err(1, "puffs_daemon");

	if (puffs_mount(pu, argv[1], mntflags, pn_root) == -1)
		err(1, "puffs_mount");
	if (puffs_setblockingmode(pu, PUFFSDEV_NONBLOCK) == -1)
		err(1, "setblockingmode");

	if (puffs_mainloop(pu) == -1)
		err(1, "mainloop");
	close(p9p.servsock);
	puffs_exit(pu, 1);

	return 0;
}
