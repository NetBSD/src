/*      $NetBSD: rumpclient.c,v 1.2 2010/11/05 13:50:48 pooka Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
 * Client side routines for rump syscall proxy.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD");

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rumpclient.h>

#include "sp_common.c"

static struct spclient clispc;

static int
send_syscall_req(struct spclient *spc, int sysnum,
	const void *data, size_t dlen)
{
	struct rsp_hdr rhdr;

	rhdr.rsp_len = sizeof(rhdr) + dlen;
	rhdr.rsp_reqno = nextreq++;
	rhdr.rsp_type = RUMPSP_SYSCALL_REQ;
	rhdr.rsp_sysnum = sysnum;

	dosend(spc, &rhdr, sizeof(rhdr));
	dosend(spc, data, dlen);

	return 0;
}

static int
send_copyin_resp(struct spclient *spc, uint64_t reqno, void *data, size_t dlen)
{
	struct rsp_hdr rhdr;

	rhdr.rsp_len = sizeof(rhdr) + dlen;
	rhdr.rsp_reqno = reqno;
	rhdr.rsp_type = RUMPSP_COPYIN_RESP;
	rhdr.rsp_sysnum = 0;

	dosend(spc, &rhdr, sizeof(rhdr));
	dosend(spc, data, dlen);

	return 0;
}

static int
send_anonmmap_resp(struct spclient *spc, uint64_t reqno, void *addr)
{
	struct rsp_hdr rhdr;

	rhdr.rsp_len = sizeof(rhdr) + sizeof(addr);
	rhdr.rsp_reqno = reqno;
	rhdr.rsp_type = RUMPSP_ANONMMAP_RESP;
	rhdr.rsp_sysnum = 0;

	dosend(spc, &rhdr, sizeof(rhdr));
	dosend(spc, &addr, sizeof(addr));

	return 0;
}

int
rumpclient_syscall(int sysnum, const void *data, size_t dlen,
	register_t *retval)
{
	struct rsp_sysresp *resp;
	struct rsp_copydata *copydata;
	struct pollfd pfd;
	size_t maplen;
	void *mapaddr;
	int gotresp;

	DPRINTF(("rump_sp_syscall: executing syscall %d\n", sysnum));

	send_syscall_req(&clispc, sysnum, data, dlen);

	DPRINTF(("rump_sp_syscall: syscall %d request sent.  "
	    "waiting for response\n", sysnum));

	pfd.fd = clispc.spc_fd;
	pfd.events = POLLIN;

	gotresp = 0;
	while (!gotresp) {
		while (readframe(&clispc) < 1)
			poll(&pfd, 1, INFTIM);

		switch (clispc.spc_hdr.rsp_type) {
		case RUMPSP_COPYIN_REQ:
			/*LINTED*/
			copydata = (struct rsp_copydata *)clispc.spc_buf;
			DPRINTF(("rump_sp_syscall: copyin request: %p/%zu\n",
			    copydata->rcp_addr, copydata->rcp_len));
			send_copyin_resp(&clispc, clispc.spc_hdr.rsp_reqno,
			    copydata->rcp_addr, copydata->rcp_len);
			clispc.spc_off = 0;
			break;
		case RUMPSP_COPYOUT_REQ:
			/*LINTED*/
			copydata = (struct rsp_copydata *)clispc.spc_buf;
			DPRINTF(("rump_sp_syscall: copyout request: %p/%zu\n",
			    copydata->rcp_addr, copydata->rcp_len));
			/*LINTED*/
			memcpy(copydata->rcp_addr, copydata->rcp_data,
			    copydata->rcp_len);
			clispc.spc_off = 0;
			break;
		case RUMPSP_ANONMMAP_REQ:
			/*LINTED*/
			maplen = *(size_t *)clispc.spc_buf;
			mapaddr = mmap(NULL, maplen, PROT_READ|PROT_WRITE,
			    MAP_ANON, -1, 0);
			if (mapaddr == MAP_FAILED)
				mapaddr = NULL;
			send_anonmmap_resp(&clispc,
			    clispc.spc_hdr.rsp_reqno, mapaddr);
			clispc.spc_off = 0;
			break;
		case RUMPSP_SYSCALL_RESP:
			DPRINTF(("rump_sp_syscall: got response \n"));
			gotresp = 1;
			break;
		}
	}

	/*LINTED*/
	resp = (struct rsp_sysresp *)clispc.spc_buf;
	memcpy(retval, &resp->rsys_retval, sizeof(resp->rsys_retval));
	clispc.spc_off = 0;

	return resp->rsys_error;
}

int
rumpclient_init()
{
	struct sockaddr *sap;
	char *p;
	unsigned idx;
	int error, s;

	if ((p = getenv("RUMP_SP_CLIENT")) == NULL) {
		errno = ENOENT;
		return -1;
	}

	if ((error = parseurl(p, &sap, &idx, 0)) != 0) {
		errno = error;
		return -1;
	}

	s = socket(parsetab[idx].domain, SOCK_STREAM, 0);
	if (s == -1)
		return -1;

	if (connect(s, sap, sap->sa_len) == -1) {
		error = errno;
		fprintf(stderr, "rump_sp: client connect failed\n");
		errno = error;
		return -1;
	}
	if ((error = parsetab[idx].connhook(s)) != 0) {
		error = errno;
		fprintf(stderr, "rump_sp: connect hook failed\n");
		errno = error;
		return -1;
	}
	clispc.spc_fd = s;

	return 0;
}
