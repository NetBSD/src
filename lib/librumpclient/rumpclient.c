/*      $NetBSD: rumpclient.c,v 1.12 2011/01/06 06:57:14 pooka Exp $	*/

/*
 * Copyright (c) 2010, 2011 Antti Kantee.  All Rights Reserved.
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
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rump/rumpclient.h>

#include "sp_common.c"

static struct spclient clispc = {
	.spc_fd = -1,
};

/*
 * This version of waitresp is optimized for single-threaded clients
 * and is required by signal-safe clientside rump syscalls.
 */

static void
releasercvlock(struct spclient *spc)
{

	pthread_mutex_lock(&spc->spc_mtx);
	if (spc->spc_istatus == SPCSTATUS_WANTED)
		kickall(spc);
	spc->spc_istatus = SPCSTATUS_FREE;
}

static sigset_t fullset;
static int
waitresp(struct spclient *spc, struct respwait *rw, sigset_t *mask)
{
	struct pollfd pfd;
	int rv = 0;

	sendunlockl(spc);

	rw->rw_error = 0;
	while (rw->rw_data == NULL && rw->rw_error == 0
	    && spc->spc_state != SPCSTATE_DYING){
		/* are we free to receive? */
		if (spc->spc_istatus == SPCSTATUS_FREE) {
			spc->spc_istatus = SPCSTATUS_BUSY;
			pthread_mutex_unlock(&spc->spc_mtx);

			pfd.fd = spc->spc_fd;
			pfd.events = POLLIN;

			switch (readframe(spc)) {
			case 0:
				releasercvlock(spc);
				pthread_mutex_unlock(&spc->spc_mtx);
				pollts(&pfd, 1, NULL, mask);
				pthread_mutex_lock(&spc->spc_mtx);
				continue;
			case -1:
				releasercvlock(spc);
				rv = errno;
				spc->spc_state = SPCSTATE_DYING;
				continue;
			default:
				break;
			}

			switch (spc->spc_hdr.rsp_class) {
				case RUMPSP_RESP:
				case RUMPSP_ERROR:
					kickwaiter(spc);
					break;
				case RUMPSP_REQ:
					handlereq(spc);
					break;
				default:
					/* panic */
					break;
			}

			releasercvlock(spc);
		} else {
			spc->spc_istatus = SPCSTATUS_WANTED;
			pthread_cond_wait(&rw->rw_cv, &spc->spc_mtx);
		}
	}
	TAILQ_REMOVE(&spc->spc_respwait, rw, rw_entries);
	pthread_mutex_unlock(&spc->spc_mtx);
	pthread_cond_destroy(&rw->rw_cv);

	if (rv)
		return rv;
	if (spc->spc_state == SPCSTATE_DYING)
		return ENOTCONN;
	return rw->rw_error;
}


static int
syscall_req(struct spclient *spc, int sysnum,
	const void *data, size_t dlen, void **resp)
{
	struct rsp_hdr rhdr;
	struct respwait rw;
	sigset_t omask;
	int rv;

	rhdr.rsp_len = sizeof(rhdr) + dlen;
	rhdr.rsp_class = RUMPSP_REQ;
	rhdr.rsp_type = RUMPSP_SYSCALL;
	rhdr.rsp_sysnum = sysnum;

	pthread_sigmask(SIG_SETMASK, &fullset, &omask);
	do {

		putwait(spc, &rw, &rhdr);
		rv = dosend(spc, &rhdr, sizeof(rhdr));
		rv = dosend(spc, data, dlen);
		if (rv) {
			unputwait(spc, &rw);
			pthread_sigmask(SIG_SETMASK, &omask, NULL);
			return rv;
		}

		rv = waitresp(spc, &rw, &omask);
	} while (rv == EAGAIN);
	pthread_sigmask(SIG_SETMASK, &omask, NULL);

	*resp = rw.rw_data;
	return rv;
}

static int
handshake_req(struct spclient *spc, uint32_t *auth, int cancel)
{
	struct handshake_fork rf;
	struct rsp_hdr rhdr;
	struct respwait rw;
	sigset_t omask;
	int rv;

	/* performs server handshake */
	rhdr.rsp_len = sizeof(rhdr) + (auth ? sizeof(rf) : 0);
	rhdr.rsp_class = RUMPSP_REQ;
	rhdr.rsp_type = RUMPSP_HANDSHAKE;
	if (auth)
		rhdr.rsp_handshake = HANDSHAKE_FORK;
	else
		rhdr.rsp_handshake = HANDSHAKE_GUEST;

	pthread_sigmask(SIG_SETMASK, &fullset, &omask);
	putwait(spc, &rw, &rhdr);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	if (auth) {
		memcpy(rf.rf_auth, auth, AUTHLEN*sizeof(*auth));
		rf.rf_cancel = cancel;
		rv = dosend(spc, &rf, sizeof(rf));
	}
	if (rv != 0 || cancel) {
		unputwait(spc, &rw);
		pthread_sigmask(SIG_SETMASK, &omask, NULL);
		return rv;
	}

	rv = waitresp(spc, &rw, &omask);
	pthread_sigmask(SIG_SETMASK, &omask, NULL);
	if (rv)
		return rv;

	rv = *(int *)rw.rw_data;
	free(rw.rw_data);

	return rv;
}

static int
prefork_req(struct spclient *spc, void **resp)
{
	struct rsp_hdr rhdr;
	struct respwait rw;
	sigset_t omask;
	int rv;

	rhdr.rsp_len = sizeof(rhdr);
	rhdr.rsp_class = RUMPSP_REQ;
	rhdr.rsp_type = RUMPSP_PREFORK;
	rhdr.rsp_error = 0;

	pthread_sigmask(SIG_SETMASK, &fullset, &omask);
	putwait(spc, &rw, &rhdr);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	if (rv != 0) {
		unputwait(spc, &rw);
		pthread_sigmask(SIG_SETMASK, &omask, NULL);
		return rv;
	}

	rv = waitresp(spc, &rw, &omask);
	pthread_sigmask(SIG_SETMASK, &omask, NULL);
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
		printf("PANIC: INVALID TYPE %d\n", reqtype);
		abort();
		break;
	}

	spcfreebuf(spc);
}

static unsigned ptab_idx;
static struct sockaddr *serv_sa;

static int
doconnect(void)
{
	char banner[MAXBANNER];
	int s, error;
	ssize_t n;

	s = socket(parsetab[ptab_idx].domain, SOCK_STREAM, 0);
	if (s == -1)
		return -1;

	if (connect(s, serv_sa, (socklen_t)serv_sa->sa_len) == -1) {
		error = errno;
		fprintf(stderr, "rump_sp: client connect failed\n");
		errno = error;
		return -1;
	}

	if ((error = parsetab[ptab_idx].connhook(s)) != 0) {
		error = errno;
		fprintf(stderr, "rump_sp: connect hook failed\n");
		errno = error;
		return -1;
	}

	if ((n = read(s, banner, sizeof(banner)-1)) < 0) {
		error = errno;
		fprintf(stderr, "rump_sp: failed to read banner\n");
		errno = error;
		return -1;
	}

	if (banner[n-1] != '\n') {
		fprintf(stderr, "rump_sp: invalid banner\n");
		errno = EINVAL;
		return -1;
	}
	banner[n] = '\0';

	/* parse the banner some day */

	clispc.spc_fd = s;
	TAILQ_INIT(&clispc.spc_respwait);
	pthread_mutex_init(&clispc.spc_mtx, NULL);
	pthread_cond_init(&clispc.spc_cv, NULL);

	return 0;
}

int
rumpclient_init()
{
	char *p;
	int error;

	if ((p = getenv("RUMP_SERVER")) == NULL) {
		errno = ENOENT;
		return -1;
	}

	if ((error = parseurl(p, &serv_sa, &ptab_idx, 0)) != 0) {
		errno = error;
		return -1;
	}

	if (doconnect() == -1)
		return -1;

	error = handshake_req(&clispc, NULL, 0);
	if (error) {
		pthread_mutex_destroy(&clispc.spc_mtx);
		pthread_cond_destroy(&clispc.spc_cv);
		close(clispc.spc_fd);
		errno = error;
		return -1;
	}

	sigfillset(&fullset);
	return 0;
}

struct rumpclient_fork {
	uint32_t fork_auth[AUTHLEN];
};

struct rumpclient_fork *
rumpclient_prefork(void)
{
	struct rumpclient_fork *rpf;
	void *resp;
	int rv;

	rpf = malloc(sizeof(*rpf));
	if (rpf == NULL)
		return NULL;

	if ((rv = prefork_req(&clispc, &resp)) != 0) {
		free(rpf);
		errno = rv;
		return NULL;
	}

	memcpy(rpf->fork_auth, resp, sizeof(rpf->fork_auth));
	free(resp);

	return rpf;
}

int
rumpclient_fork_init(struct rumpclient_fork *rpf)
{
	int error;

	close(clispc.spc_fd);
	memset(&clispc, 0, sizeof(clispc));
	clispc.spc_fd = -1;

	if (doconnect() == -1)
		return -1;

	error = handshake_req(&clispc, rpf->fork_auth, 0);
	if (error) {
		pthread_mutex_destroy(&clispc.spc_mtx);
		pthread_cond_destroy(&clispc.spc_cv);
		errno = error;
		return -1;
	}

	return 0;
}
