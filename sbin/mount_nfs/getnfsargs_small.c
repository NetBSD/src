/*	$NetBSD: getnfsargs_small.c,v 1.4 2006/05/20 08:06:48 yamt Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by David Laight.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

/*-
 *  Copyright (c) 1993 John Brezak
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1992, 1993, 1994
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
__RCSID("$NetBSD: getnfsargs_small.c,v 1.4 2006/05/20 08:06:48 yamt Exp $");

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <syslog.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nqnfs.h>
#include <nfs/nfsmount.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "mount_nfs.h"

#include "iodesc.h"
typedef int32_t n_long;
#include "rpc.h"

#define RPC_HEADER_WORDS 28	/* more than enough */
#define FNAME_SIZE 512

struct nfhret {
	long		fhsize;
	u_char		nfh[NFSX_V3FHMAX];
};

/* Ripped from src/sys/arch/i386/stand/libsa/nfs.c */
static int
nfs_getrootfh(struct iodesc *d, const char *path, int mntvers, struct nfhret *nfhret)
{
	size_t len;
	struct args {
		uint32_t len;
		char	path[FNAME_SIZE];
	} *args;
	struct repl {
		uint32_t errval;
		u_char	fh[NFSX_V3FHMAX];
	} *repl;
	struct {
		uint32_t h[RPC_HEADER_WORDS];
		struct args d;
	} sdata;
	struct {
		uint32_t h[RPC_HEADER_WORDS];
		struct repl d;
	} rdata;
	ssize_t cc;

	args = &sdata.d;
	repl = &rdata.d;

	memset(args, 0, sizeof(*args));
	len = strlen(path);
	if (len > sizeof(args->path))
		len = sizeof(args->path);
	args->len = htonl(len);
	memcpy(args->path, path, len);
	len = 4 + roundup(len, 4);

	cc = rpc_call(d, RPCPROG_MNT, mntvers, RPCMNT_MOUNT,
	    args, len, repl, sizeof(*repl));
	if (cc == -1) {
		/* errno was set by rpc_call */
		return errno;
	}
	if (cc < 4)
		return EBADRPC;
	if (repl->errval)
		return ntohl(repl->errval);
	nfhret->fhsize = cc;
	memcpy(nfhret->nfh, repl->fh, sizeof(repl->fh));
	return 0;
}

int
getnfsargs(char *spec, struct nfs_args *nfsargsp)
{
	struct addrinfo hints, *ai_nfs;
	int ecode;
	int nfsvers, mntvers;
	char *hostp, *delimp;
	static struct nfhret nfhret;
	static char nam[MNAMELEN + 1];
	struct iodesc d;
	int nfs_port;

	strncpy(nam, spec, MNAMELEN);
	nam[MNAMELEN] = '\0';
	if ((delimp = strchr(spec, '@')) != NULL) {
		hostp = delimp + 1;
	} else if ((delimp = strrchr(spec, ':')) != NULL) {
		hostp = spec;
		spec = delimp + 1;
	} else {
		warnx("no <host>:<dirpath> or <dirpath>@<host> spec");
		return 0;
	}
	*delimp = '\0';

	memset(&hints, 0, sizeof hints);
	hints.ai_socktype = nfsargsp->sotype;

	if ((ecode = getaddrinfo(hostp, "nfs", &hints, &ai_nfs)) != 0) {
		warnx("can't get net id for host \"%s\": %s", hostp,
		    gai_strerror(ecode));
		return 0;
	}

	if ((nfsargsp->flags & NFSMNT_NFSV3) != 0) {
		nfsvers = NFS_VER3;
		mntvers = RPCMNT_VER3;
	} else {
		nfsvers = NFS_VER2;
		mntvers = RPCMNT_VER1;
	}

	d.socket = -1;
	for (d.ai = ai_nfs; ; d.ai = d.ai->ai_next) {
		if (d.ai == NULL) {
			if (nfsvers == NFS_VER3 && !force3) {
				nfsvers = NFS_VER2;
				mntvers = RPCMNT_VER1;
				d.ai = ai_nfs;
				continue;
			}
			return 0;
		}
		nfs_port = rpc_getport(&d, RPCPROG_NFS, nfsvers);
		if (nfs_port == -1)
			continue;
		if (nfs_getrootfh(&d, spec, mntvers, &nfhret) == 0)
			break;
	}

	if (port != 0)
		nfs_port = port;
	set_port(d.ai->ai_addr, htons(nfs_port));

	nfsargsp->hostname = nam;
	nfsargsp->addr = d.ai->ai_addr;
	nfsargsp->addrlen = d.ai->ai_addrlen;

	nfsargsp->fh = nfhret.nfh;
	if (nfsvers == NFS_VER3) {
		nfsargsp->fhsize = ntohl(*(uint32_t *)nfhret.nfh);
		nfsargsp->fh += 4;
	} else {
		nfsargsp->fhsize = NFSX_V2FH;
		nfsargsp->flags &= ~NFSMNT_NFSV3;
	}
	return 1;
}
