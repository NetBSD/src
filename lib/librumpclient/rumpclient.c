/*      $NetBSD: rumpclient.c,v 1.7 2010/11/30 14:24:40 pooka Exp $	*/

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

#include <sys/param.h>
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
syscall_req(struct spclient *spc, int sysnum,
	const void *data, size_t dlen, void **resp)
{
	struct rsp_hdr rhdr;
	struct respwait rw;
	int rv;

	rhdr.rsp_len = sizeof(rhdr) + dlen;
	rhdr.rsp_class = RUMPSP_REQ;
	rhdr.rsp_type = RUMPSP_SYSCALL;
	rhdr.rsp_sysnum = sysnum;

	do {
		putwait(spc, &rw, &rhdr);
		rv = dosend(spc, &rhdr, sizeof(rhdr));
		rv = dosend(spc, data, dlen);
		if (rv) {
			unputwait(spc, &rw);
			return rv;
		}

		rv = waitresp(spc, &rw);
	} while (rv == EAGAIN);

	*resp = rw.rw_data;
	return rv;
}

static int
send_copyin_resp(struct spclient *spc, uint64_t reqno, void *data, size_t dlen,
	int wantstr)
{
	struct rsp_hdr rhdr;
	int rv;

	if (wantstr)
		dlen = MIN(dlen, strlen(data)+1);

	rhdr.rsp_len = sizeof(rhdr) + dlen;
	rhdr.rsp_reqno = reqno;
	rhdr.rsp_class = RUMPSP_RESP;
	rhdr.rsp_type = RUMPSP_COPYIN;
	rhdr.rsp_sysnum = 0;

	sendlock(spc);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	rv = dosend(spc, data, dlen);
	sendunlock(spc);

	return rv;
}

static int
send_anonmmap_resp(struct spclient *spc, uint64_t reqno, void *addr)
{
	struct rsp_hdr rhdr;
	int rv;

	rhdr.rsp_len = sizeof(rhdr) + sizeof(addr);
	rhdr.rsp_reqno = reqno;
	rhdr.rsp_class = RUMPSP_RESP;
	rhdr.rsp_type = RUMPSP_ANONMMAP;
	rhdr.rsp_sysnum = 0;

	sendlock(spc);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	rv = dosend(spc, &addr, sizeof(addr));
	sendunlock(spc);

	return rv;
}

int
rumpclient_syscall(int sysnum, const void *data, size_t dlen,
	register_t *retval)
{
	struct rsp_sysresp *resp;
	void *rdata;
	int rv;

	DPRINTF(("rumpsp syscall_req: syscall %d with %p/%zu\n",
	    sysnum, data, dlen));

	rv = syscall_req(&clispc, sysnum, data, dlen, &rdata);
	if (rv)
		return rv;

	resp = rdata;
	DPRINTF(("rumpsp syscall_resp: syscall %d error %d, rv: %d/%d\n",
	    sysnum, rv, resp->rsys_retval[0], resp->rsys_retval[1]));

	memcpy(retval, &resp->rsys_retval, sizeof(resp->rsys_retval));
	rv = resp->rsys_error;
	free(rdata);

	return rv;
}

static void
handlereq(struct spclient *spc)
{
	struct rsp_copydata *copydata;
	void *mapaddr;
	size_t maplen;
	int reqtype = spc->spc_hdr.rsp_type;

	switch (reqtype) {
	case RUMPSP_COPYIN:
	case RUMPSP_COPYINSTR:
		/*LINTED*/
		copydata = (struct rsp_copydata *)spc->spc_buf;
		DPRINTF(("rump_sp handlereq: copyin request: %p/%zu\n",
		    copydata->rcp_addr, copydata->rcp_len));
		send_copyin_resp(spc, spc->spc_hdr.rsp_reqno,
		    copydata->rcp_addr, copydata->rcp_len,
		    reqtype == RUMPSP_COPYINSTR);
		break;
	case RUMPSP_COPYOUT:
	case RUMPSP_COPYOUTSTR:
		/*LINTED*/
		copydata = (struct rsp_copydata *)spc->spc_buf;
		DPRINTF(("rump_sp handlereq: copyout request: %p/%zu\n",
		    copydata->rcp_addr, copydata->rcp_len));
		/*LINTED*/
		memcpy(copydata->rcp_addr, copydata->rcp_data,
		    copydata->rcp_len);
		break;
	case RUMPSP_ANONMMAP:
		/*LINTED*/
		maplen = *(size_t *)spc->spc_buf;
		mapaddr = mmap(NULL, maplen, PROT_READ|PROT_WRITE,
		    MAP_ANON, -1, 0);
		if (mapaddr == MAP_FAILED)
			mapaddr = NULL;
		DPRINTF(("rump_sp handlereq: anonmmap: %p\n", mapaddr));
		send_anonmmap_resp(spc, spc->spc_hdr.rsp_reqno, mapaddr);
		break;
	default:
		printf("PANIC: INVALID TYPE\n");
		abort();
		break;
	}

	spcfreebuf(spc);
}

int
rumpclient_init()
{
	struct sockaddr *sap;
	char *p;
	unsigned idx;
	int error, s;

	if ((p = getenv("RUMP_SERVER")) == NULL) {
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
	TAILQ_INIT(&clispc.spc_respwait);
	pthread_mutex_init(&clispc.spc_mtx, NULL);
	pthread_cond_init(&clispc.spc_cv, NULL);

	return 0;
}
