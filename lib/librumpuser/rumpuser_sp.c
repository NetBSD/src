/*      $NetBSD: rumpuser_sp.c,v 1.31 2011/01/06 06:57:14 pooka Exp $	*/

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
 * Sysproxy routines.  This provides system RPC support over host sockets.
 * The most notable limitation is that the client and server must share
 * the same ABI.  This does not mean that they have to be the same
 * machine or that they need to run the same version of the host OS,
 * just that they must agree on the data structures.  This even *might*
 * work correctly from one hardware architecture to another.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: rumpuser_sp.c,v 1.31 2011/01/06 06:57:14 pooka Exp $");

#include <sys/types.h>
#include <sys/atomic.h>
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

#include <rump/rump.h> /* XXX: for rfork flags */
#include <rump/rumpuser.h>
#include "rumpuser_int.h"

#include "sp_common.c"

#ifndef MAXCLI
#define MAXCLI 256
#endif
#ifndef MAXWORKER
#define MAXWORKER 128
#endif
#ifndef IDLEWORKER
#define IDLEWORKER 16
#endif
int rumpsp_maxworker = MAXWORKER;
int rumpsp_idleworker = IDLEWORKER;

static struct pollfd pfdlist[MAXCLI];
static struct spclient spclist[MAXCLI];
static unsigned int disco;
static volatile int spfini;

static struct rumpuser_sp_ops spops;

static char banner[MAXBANNER];

#define PROTOMAJOR 0
#define PROTOMINOR 1

struct prefork {
	uint32_t pf_auth[AUTHLEN];
	struct lwp *pf_lwp;

	LIST_ENTRY(prefork) pf_entries;		/* global list */
	LIST_ENTRY(prefork) pf_spcentries;	/* linked from forking spc */
};
static LIST_HEAD(, prefork) preforks = LIST_HEAD_INITIALIZER(preforks);
static pthread_mutex_t pfmtx;

/*
 * This version is for the server.  It's optimized for multiple threads
 * and is *NOT* reentrant wrt to signals
 */
static int
waitresp(struct spclient *spc, struct respwait *rw)
{
	struct pollfd pfd;
	int rv = 0;

	sendunlockl(spc);

	rw->rw_error = 0;
	while (rw->rw_data == NULL && rw->rw_error == 0
	    && spc->spc_state != SPCSTATE_DYING){
		/* are we free to receive? */
		if (spc->spc_istatus == SPCSTATUS_FREE) {
			int gotresp;

			spc->spc_istatus = SPCSTATUS_BUSY;
			pthread_mutex_unlock(&spc->spc_mtx);

			pfd.fd = spc->spc_fd;
			pfd.events = POLLIN;

			for (gotresp = 0; !gotresp; ) {
				switch (readframe(spc)) {
				case 0:
					poll(&pfd, 1, INFTIM);
					continue;
				case -1:
					rv = errno;
					spc->spc_state = SPCSTATE_DYING;
					goto cleanup;
				default:
					break;
				}

				switch (spc->spc_hdr.rsp_class) {
				case RUMPSP_RESP:
				case RUMPSP_ERROR:
					kickwaiter(spc);
					gotresp = spc->spc_hdr.rsp_reqno ==
					    rw->rw_reqno;
					break;
				case RUMPSP_REQ:
					handlereq(spc);
					break;
				default:
					/* panic */
					break;
				}
			}
 cleanup:
			pthread_mutex_lock(&spc->spc_mtx);
			if (spc->spc_istatus == SPCSTATUS_WANTED)
				kickall(spc);
			spc->spc_istatus = SPCSTATUS_FREE;
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

/*
 * Manual wrappers, since librump does not have access to the
 * user namespace wrapped interfaces.
 */

static void
lwproc_switch(struct lwp *l)
{

	spops.spop_schedule();
	spops.spop_lwproc_switch(l);
	spops.spop_unschedule();
}

static void
lwproc_release(void)
{

	spops.spop_schedule();
	spops.spop_lwproc_release();
	spops.spop_unschedule();
}

static int
lwproc_rfork(struct spclient *spc, int flags)
{
	int rv;

	spops.spop_schedule();
	rv = spops.spop_lwproc_rfork(spc, flags);
	spops.spop_unschedule();

	return rv;
}

static int
lwproc_newlwp(pid_t pid)
{
	int rv;

	spops.spop_schedule();
	rv = spops.spop_lwproc_newlwp(pid);
	spops.spop_unschedule();

	return rv;
}

static struct lwp *
lwproc_curlwp(void)
{
	struct lwp *l;

	spops.spop_schedule();
	l = spops.spop_lwproc_curlwp();
	spops.spop_unschedule();

	return l;
}

static pid_t
lwproc_getpid(void)
{
	pid_t p;

	spops.spop_schedule();
	p = spops.spop_getpid();
	spops.spop_unschedule();

	return p;
}

static int
rumpsyscall(int sysnum, void *data, register_t *retval)
{
	int rv;

	spops.spop_schedule();
	rv = spops.spop_syscall(sysnum, data, retval);
	spops.spop_unschedule();

	return rv;
}

static uint64_t
nextreq(struct spclient *spc)
{
	uint64_t nw;

	pthread_mutex_lock(&spc->spc_mtx);
	nw = spc->spc_nextreq++;
	pthread_mutex_unlock(&spc->spc_mtx);

	return nw;
}

static void
send_error_resp(struct spclient *spc, uint64_t reqno, int error)
{
	struct rsp_hdr rhdr;

	rhdr.rsp_len = sizeof(rhdr);
	rhdr.rsp_reqno = reqno;
	rhdr.rsp_class = RUMPSP_ERROR;
	rhdr.rsp_type = 0;
	rhdr.rsp_error = error;

	sendlock(spc);
	(void)dosend(spc, &rhdr, sizeof(rhdr));
	sendunlock(spc);
}

static int
send_handshake_resp(struct spclient *spc, uint64_t reqno, int error)
{
	struct rsp_hdr rhdr;
	int rv;

	rhdr.rsp_len = sizeof(rhdr) + sizeof(error);
	rhdr.rsp_reqno = reqno;
	rhdr.rsp_class = RUMPSP_RESP;
	rhdr.rsp_type = RUMPSP_HANDSHAKE;
	rhdr.rsp_error = 0;

	sendlock(spc);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	rv = dosend(spc, &error, sizeof(error));
	sendunlock(spc);

	return rv;
}

static int
send_syscall_resp(struct spclient *spc, uint64_t reqno, int error,
	register_t *retval)
{
	struct rsp_hdr rhdr;
	struct rsp_sysresp sysresp;
	int rv;

	rhdr.rsp_len = sizeof(rhdr) + sizeof(sysresp);
	rhdr.rsp_reqno = reqno;
	rhdr.rsp_class = RUMPSP_RESP;
	rhdr.rsp_type = RUMPSP_SYSCALL;
	rhdr.rsp_sysnum = 0;

	sysresp.rsys_error = error;
	memcpy(sysresp.rsys_retval, retval, sizeof(sysresp.rsys_retval));

	sendlock(spc);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	rv = dosend(spc, &sysresp, sizeof(sysresp));
	sendunlock(spc);

	return rv;
}

static int
send_prefork_resp(struct spclient *spc, uint64_t reqno, uint32_t *auth)
{
	struct rsp_hdr rhdr;
	int rv;

	rhdr.rsp_len = sizeof(rhdr) + AUTHLEN*sizeof(*auth);
	rhdr.rsp_reqno = reqno;
	rhdr.rsp_class = RUMPSP_RESP;
	rhdr.rsp_type = RUMPSP_PREFORK;
	rhdr.rsp_sysnum = 0;

	sendlock(spc);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	rv = dosend(spc, auth, AUTHLEN*sizeof(*auth));
	sendunlock(spc);

	return rv;
}

static int
copyin_req(struct spclient *spc, const void *remaddr, size_t *dlen,
	int wantstr, void **resp)
{
	struct rsp_hdr rhdr;
	struct rsp_copydata copydata;
	struct respwait rw;
	int rv;

	DPRINTF(("copyin_req: %zu bytes from %p\n", *dlen, remaddr));

	rhdr.rsp_len = sizeof(rhdr) + sizeof(copydata);
	rhdr.rsp_class = RUMPSP_REQ;
	if (wantstr)
		rhdr.rsp_type = RUMPSP_COPYINSTR;
	else
		rhdr.rsp_type = RUMPSP_COPYIN;
	rhdr.rsp_sysnum = 0;

	copydata.rcp_addr = __UNCONST(remaddr);
	copydata.rcp_len = *dlen;

	putwait(spc, &rw, &rhdr);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	rv = dosend(spc, &copydata, sizeof(copydata));
	if (rv) {
		unputwait(spc, &rw);
		return rv;
	}

	rv = waitresp(spc, &rw);

	DPRINTF(("copyin: response %d\n", rv));

	*resp = rw.rw_data;
	if (wantstr)
		*dlen = rw.rw_dlen;

	return rv;

}

static int
send_copyout_req(struct spclient *spc, const void *remaddr,
	const void *data, size_t dlen)
{
	struct rsp_hdr rhdr;
	struct rsp_copydata copydata;
	int rv;

	DPRINTF(("copyout_req (async): %zu bytes to %p\n", dlen, remaddr));

	rhdr.rsp_len = sizeof(rhdr) + sizeof(copydata) + dlen;
	rhdr.rsp_reqno = nextreq(spc);
	rhdr.rsp_class = RUMPSP_REQ;
	rhdr.rsp_type = RUMPSP_COPYOUT;
	rhdr.rsp_sysnum = 0;

	copydata.rcp_addr = __UNCONST(remaddr);
	copydata.rcp_len = dlen;

	sendlock(spc);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	rv = dosend(spc, &copydata, sizeof(copydata));
	rv = dosend(spc, data, dlen);
	sendunlock(spc);

	return rv;
}

static int
anonmmap_req(struct spclient *spc, size_t howmuch, void **resp)
{
	struct rsp_hdr rhdr;
	struct respwait rw;
	int rv;

	DPRINTF(("anonmmap_req: %zu bytes\n", howmuch));

	rhdr.rsp_len = sizeof(rhdr) + sizeof(howmuch);
	rhdr.rsp_class = RUMPSP_REQ;
	rhdr.rsp_type = RUMPSP_ANONMMAP;
	rhdr.rsp_sysnum = 0;

	putwait(spc, &rw, &rhdr);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	rv = dosend(spc, &howmuch, sizeof(howmuch));
	if (rv) {
		unputwait(spc, &rw);
		return rv;
	}

	rv = waitresp(spc, &rw);

	*resp = rw.rw_data;

	DPRINTF(("anonmmap: mapped at %p\n", **(void ***)resp));

	return rv;
}

static void
spcref(struct spclient *spc)
{

	pthread_mutex_lock(&spc->spc_mtx);
	spc->spc_refcnt++;
	pthread_mutex_unlock(&spc->spc_mtx);
}

static void
spcrelease(struct spclient *spc)
{
	int ref;

	pthread_mutex_lock(&spc->spc_mtx);
	ref = --spc->spc_refcnt;
	pthread_mutex_unlock(&spc->spc_mtx);

	if (ref > 0)
		return;

	DPRINTF(("rump_sp: spcrelease: spc %p fd %d\n", spc, spc->spc_fd));

	_DIAGASSERT(TAILQ_EMPTY(&spc->spc_respwait));
	_DIAGASSERT(spc->spc_buf == NULL);

	if (spc->spc_mainlwp) {
		lwproc_switch(spc->spc_mainlwp);
		lwproc_release();
	}
	spc->spc_mainlwp = NULL;

	close(spc->spc_fd);
	spc->spc_fd = -1;
	spc->spc_state = SPCSTATE_NEW;

	atomic_inc_uint(&disco);
}

static void
serv_handledisco(unsigned int idx)
{
	struct spclient *spc = &spclist[idx];

	DPRINTF(("rump_sp: disconnecting [%u]\n", idx));

	pfdlist[idx].fd = -1;
	pfdlist[idx].revents = 0;
	pthread_mutex_lock(&spc->spc_mtx);
	spc->spc_state = SPCSTATE_DYING;
	kickall(spc);
	sendunlockl(spc);
	pthread_mutex_unlock(&spc->spc_mtx);

	/*
	 * Nobody's going to attempt to send/receive anymore,
	 * so reinit info relevant to that.
	 */
	/*LINTED:pointer casts may be ok*/
	memset((char *)spc + SPC_ZEROFF, 0, sizeof(*spc) - SPC_ZEROFF);

	spcrelease(spc);
}

static void
serv_shutdown(void)
{
	struct spclient *spc;
	unsigned int i;

	for (i = 1; i < MAXCLI; i++) {
		spc = &spclist[i];
		if (spc->spc_fd == -1)
			continue;

		shutdown(spc->spc_fd, SHUT_RDWR);
		serv_handledisco(i);

		spcrelease(spc);
	}
}

static unsigned
serv_handleconn(int fd, connecthook_fn connhook, int busy)
{
	struct sockaddr_storage ss;
	socklen_t sl = sizeof(ss);
	int newfd, flags;
	unsigned i;

	/*LINTED: cast ok */
	newfd = accept(fd, (struct sockaddr *)&ss, &sl);
	if (newfd == -1)
		return 0;

	if (busy) {
		close(newfd); /* EBUSY */
		return 0;
	}

	flags = fcntl(newfd, F_GETFL, 0);
	if (fcntl(newfd, F_SETFL, flags | O_NONBLOCK) == -1) {
		close(newfd);
		return 0;
	}

	if (connhook(newfd) != 0) {
		close(newfd);
		return 0;
	}

	/* write out a banner for the client */
	if (write(newfd, banner, strlen(banner)) != (ssize_t)strlen(banner)) {
		close(newfd);
		return 0;
	}

	/* find empty slot the simple way */
	for (i = 0; i < MAXCLI; i++) {
		if (pfdlist[i].fd == -1 && spclist[i].spc_state == SPCSTATE_NEW)
			break;
	}

	assert(i < MAXCLI);

	pfdlist[i].fd = newfd;
	spclist[i].spc_fd = newfd;
	spclist[i].spc_istatus = SPCSTATUS_BUSY; /* dedicated receiver */
	spclist[i].spc_refcnt = 1;

	TAILQ_INIT(&spclist[i].spc_respwait);

	DPRINTF(("rump_sp: added new connection fd %d at idx %u\n", newfd, i));

	return i;
}

static void
serv_handlesyscall(struct spclient *spc, struct rsp_hdr *rhdr, uint8_t *data)
{
	register_t retval[2] = {0, 0};
	int rv, sysnum;

	sysnum = (int)rhdr->rsp_sysnum;
	DPRINTF(("rump_sp: handling syscall %d from client %d\n",
	    sysnum, spc->spc_pid));

	lwproc_newlwp(spc->spc_pid);
	rv = rumpsyscall(sysnum, data, retval);
	lwproc_release();

	DPRINTF(("rump_sp: got return value %d & %d/%d\n",
	    rv, retval[0], retval[1]));

	send_syscall_resp(spc, rhdr->rsp_reqno, rv, retval);
}

struct sysbouncearg {
	struct spclient *sba_spc;
	struct rsp_hdr sba_hdr;
	uint8_t *sba_data;

	TAILQ_ENTRY(sysbouncearg) sba_entries;
};
static pthread_mutex_t sbamtx;
static pthread_cond_t sbacv;
static int nworker, idleworker;
static TAILQ_HEAD(, sysbouncearg) syslist = TAILQ_HEAD_INITIALIZER(syslist);

/*ARGSUSED*/
static void *
serv_syscallbouncer(void *arg)
{
	struct sysbouncearg *sba;

	for (;;) {
		pthread_mutex_lock(&sbamtx);
		if (idleworker >= rumpsp_idleworker) {
			nworker--;
			pthread_mutex_unlock(&sbamtx);
			break;
		}
		idleworker++;
		while (TAILQ_EMPTY(&syslist)) {
			pthread_cond_wait(&sbacv, &sbamtx);
		}

		sba = TAILQ_FIRST(&syslist);
		TAILQ_REMOVE(&syslist, sba, sba_entries);
		idleworker--;
		pthread_mutex_unlock(&sbamtx);

		serv_handlesyscall(sba->sba_spc,
		    &sba->sba_hdr, sba->sba_data);
		spcrelease(sba->sba_spc);
		free(sba->sba_data);
		free(sba);
	}

	return NULL;
}

static int
sp_copyin(void *arg, const void *raddr, void *laddr, size_t *len, int wantstr)
{
	struct spclient *spc = arg;
	void *rdata = NULL; /* XXXuninit */
	int rv, nlocks;

	rumpuser__kunlock(0, &nlocks, NULL);

	rv = copyin_req(spc, raddr, len, wantstr, &rdata);
	if (rv)
		goto out;

	memcpy(laddr, rdata, *len);
	free(rdata);

 out:
	rumpuser__klock(nlocks, NULL);
	if (rv)
		return EFAULT;
	return 0;
}

int
rumpuser_sp_copyin(void *arg, const void *raddr, void *laddr, size_t len)
{

	return sp_copyin(arg, raddr, laddr, &len, 0);
}

int
rumpuser_sp_copyinstr(void *arg, const void *raddr, void *laddr, size_t *len)
{

	return sp_copyin(arg, raddr, laddr, len, 1);
}

static int
sp_copyout(void *arg, const void *laddr, void *raddr, size_t dlen)
{
	struct spclient *spc = arg;
	int nlocks, rv;

	rumpuser__kunlock(0, &nlocks, NULL);
	rv = send_copyout_req(spc, raddr, laddr, dlen);
	rumpuser__klock(nlocks, NULL);

	if (rv)
		return EFAULT;
	return 0;
}

int
rumpuser_sp_copyout(void *arg, const void *laddr, void *raddr, size_t dlen)
{

	return sp_copyout(arg, laddr, raddr, dlen);
}

int
rumpuser_sp_copyoutstr(void *arg, const void *laddr, void *raddr, size_t *dlen)
{

	return sp_copyout(arg, laddr, raddr, *dlen);
}

int
rumpuser_sp_anonmmap(void *arg, size_t howmuch, void **addr)
{
	struct spclient *spc = arg;
	void *resp, *rdata;
	int nlocks, rv;

	rumpuser__kunlock(0, &nlocks, NULL);

	rv = anonmmap_req(spc, howmuch, &rdata);
	if (rv) {
		rv = EFAULT;
		goto out;
	}

	resp = *(void **)rdata;
	free(rdata);

	if (resp == NULL) {
		rv = ENOMEM;
	}

	*addr = resp;

 out:
	rumpuser__klock(nlocks, NULL);

	if (rv)
		return rv;
	return 0;
}

/*
 *
 * Startup routines and mainloop for server.
 *
 */

struct spservarg {
	int sps_sock;
	connecthook_fn sps_connhook;
};

static pthread_attr_t pattr_detached;
static void
handlereq(struct spclient *spc)
{
	struct sysbouncearg *sba;
	pthread_t pt;
	int retries, error, i;

	if (__predict_false(spc->spc_state == SPCSTATE_NEW)) {
		if (spc->spc_hdr.rsp_type != RUMPSP_HANDSHAKE) {
			send_error_resp(spc, spc->spc_hdr.rsp_reqno, EAUTH);
			shutdown(spc->spc_fd, SHUT_RDWR);
			spcfreebuf(spc);
			return;
		}

		if (spc->spc_hdr.rsp_handshake == HANDSHAKE_GUEST) {
			if ((error = lwproc_rfork(spc, RUMP_RFCFDG)) != 0) {
				shutdown(spc->spc_fd, SHUT_RDWR);
			}

			spcfreebuf(spc);
			if (error)
				return;

			spc->spc_mainlwp = lwproc_curlwp();

			send_handshake_resp(spc, spc->spc_hdr.rsp_reqno, 0);
		} else if (spc->spc_hdr.rsp_handshake == HANDSHAKE_FORK) {
			struct lwp *tmpmain;
			struct prefork *pf;
			struct handshake_fork *rfp;
			uint64_t reqno;
			int cancel;

			reqno = spc->spc_hdr.rsp_reqno;
			if (spc->spc_off-HDRSZ != sizeof(*rfp)) {
				send_error_resp(spc, reqno, EINVAL);
				shutdown(spc->spc_fd, SHUT_RDWR);
				spcfreebuf(spc);
				return;
			}

			/*LINTED*/
			rfp = (void *)spc->spc_buf;
			cancel = rfp->rf_cancel;

			pthread_mutex_lock(&pfmtx);
			LIST_FOREACH(pf, &preforks, pf_entries) {
				if (memcmp(rfp->rf_auth, pf->pf_auth,
				    sizeof(rfp->rf_auth)) == 0) {
					LIST_REMOVE(pf, pf_entries);
					LIST_REMOVE(pf, pf_spcentries);
					break;
				}
			}
			pthread_mutex_lock(&pfmtx);
			spcfreebuf(spc);

			if (!pf) {
				send_error_resp(spc, reqno, ESRCH);
				shutdown(spc->spc_fd, SHUT_RDWR);
				return;
			}

			tmpmain = pf->pf_lwp;
			free(pf);
			lwproc_switch(tmpmain);
			if (cancel) {
				lwproc_release();
				shutdown(spc->spc_fd, SHUT_RDWR);
				return;
			}

			/*
			 * So, we forked already during "prefork" to save
			 * the file descriptors from a parent exit
			 * race condition.  But now we need to fork
			 * a second time since the initial fork has
			 * the wrong spc pointer.  (yea, optimize
			 * interfaces some day if anyone cares)
			 */
			if ((error = lwproc_rfork(spc, 0)) != 0) {
				send_error_resp(spc, reqno, error);
				shutdown(spc->spc_fd, SHUT_RDWR);
				lwproc_release();
				return;
			}
			spc->spc_mainlwp = lwproc_curlwp();
			lwproc_switch(tmpmain);
			lwproc_release();
			lwproc_switch(spc->spc_mainlwp);

			send_handshake_resp(spc, reqno, 0);
		}

		spc->spc_pid = lwproc_getpid();

		DPRINTF(("rump_sp: handshake for client %p complete, pid %d\n",
		    spc, spc->spc_pid));
		    
		lwproc_switch(NULL);
		spc->spc_state = SPCSTATE_RUNNING;
		return;
	}

	if (__predict_false(spc->spc_hdr.rsp_type == RUMPSP_PREFORK)) {
		struct prefork *pf;
		uint64_t reqno;
		uint32_t auth[AUTHLEN];

		DPRINTF(("rump_sp: prefork handler executing for %p\n", spc));
		reqno = spc->spc_hdr.rsp_reqno;
		spcfreebuf(spc);

		pf = malloc(sizeof(*pf));
		if (pf == NULL) {
			send_error_resp(spc, reqno, ENOMEM);
			return;
		}

		/*
		 * Use client main lwp to fork.  this is never used by
		 * worker threads (except if spc refcount goes to 0),
		 * so we can safely use it here.
		 */
		lwproc_switch(spc->spc_mainlwp);
		if ((error = lwproc_rfork(spc, RUMP_RFFDG)) != 0) {
			DPRINTF(("rump_sp: fork failed: %d (%p)\n",error, spc));
			send_error_resp(spc, reqno, error);
			lwproc_switch(NULL);
			free(pf);
			return;
		}

		/* Ok, we have a new process context and a new curlwp */
		for (i = 0; i < AUTHLEN; i++) {
			pf->pf_auth[i] = auth[i] = arc4random();
		}
		pf->pf_lwp = lwproc_curlwp();
		lwproc_switch(NULL);

		pthread_mutex_lock(&pfmtx);
		LIST_INSERT_HEAD(&preforks, pf, pf_entries);
		LIST_INSERT_HEAD(&spc->spc_pflist, pf, pf_spcentries);
		pthread_mutex_unlock(&pfmtx);

		DPRINTF(("rump_sp: prefork handler success %p\n", spc));

		send_prefork_resp(spc, reqno, auth);
		return;
	}

	if (__predict_false(spc->spc_hdr.rsp_type != RUMPSP_SYSCALL)) {
		send_error_resp(spc, spc->spc_hdr.rsp_reqno, EINVAL);
		spcfreebuf(spc);
		return;
	}

	retries = 0;
	while ((sba = malloc(sizeof(*sba))) == NULL) {
		if (nworker == 0 || retries > 10) {
			send_error_resp(spc, spc->spc_hdr.rsp_reqno, EAGAIN);
			spcfreebuf(spc);
			return;
		}
		/* slim chance of more memory? */
		usleep(10000);
	}

	sba->sba_spc = spc;
	sba->sba_hdr = spc->spc_hdr;
	sba->sba_data = spc->spc_buf;
	spcresetbuf(spc);

	spcref(spc);

	pthread_mutex_lock(&sbamtx);
	TAILQ_INSERT_TAIL(&syslist, sba, sba_entries);
	if (idleworker > 0) {
		/* do we have a daemon's tool (i.e. idle threads)? */
		pthread_cond_signal(&sbacv);
	} else if (nworker < rumpsp_maxworker) {
		/*
		 * Else, need to create one
		 * (if we can, otherwise just expect another
		 * worker to pick up the syscall)
		 */
		if (pthread_create(&pt, &pattr_detached,
		    serv_syscallbouncer, NULL) == 0)
			nworker++;
	}
	pthread_mutex_unlock(&sbamtx);
}

static void *
spserver(void *arg)
{
	struct spservarg *sarg = arg;
	struct spclient *spc;
	unsigned idx;
	int seen;
	int rv;
	unsigned int nfds, maxidx;

	for (idx = 0; idx < MAXCLI; idx++) {
		pfdlist[idx].fd = -1;
		pfdlist[idx].events = POLLIN;

		spc = &spclist[idx];
		pthread_mutex_init(&spc->spc_mtx, NULL);
		pthread_cond_init(&spc->spc_cv, NULL);
		spc->spc_fd = -1;
	}
	pfdlist[0].fd = spclist[0].spc_fd = sarg->sps_sock;
	pfdlist[0].events = POLLIN;
	nfds = 1;
	maxidx = 0;

	pthread_attr_init(&pattr_detached);
	pthread_attr_setdetachstate(&pattr_detached, PTHREAD_CREATE_DETACHED);
	/* XXX: doesn't stacksize currently work on NetBSD */
	pthread_attr_setstacksize(&pattr_detached, 32*1024);

	pthread_mutex_init(&sbamtx, NULL);
	pthread_cond_init(&sbacv, NULL);

	DPRINTF(("rump_sp: server mainloop\n"));

	for (;;) {
		int discoed;

		/* g/c hangarounds (eventually) */
		discoed = atomic_swap_uint(&disco, 0);
		while (discoed--) {
			nfds--;
			idx = maxidx;
			while (idx) {
				if (pfdlist[idx].fd != -1) {
					maxidx = idx;
					break;
				}
				idx--;
			}
			DPRINTF(("rump_sp: set maxidx to [%u]\n",
			    maxidx));
		}

		DPRINTF(("rump_sp: loop nfd %d\n", maxidx+1));
		seen = 0;
		rv = poll(pfdlist, maxidx+1, INFTIM);
		assert(maxidx+1 <= MAXCLI);
		assert(rv != 0);
		if (rv == -1) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "rump_spserver: poll returned %d\n",
			    errno);
			break;
		}

		for (idx = 0; seen < rv && idx < MAXCLI; idx++) {
			if ((pfdlist[idx].revents & POLLIN) == 0)
				continue;

			seen++;
			DPRINTF(("rump_sp: activity at [%u] %d/%d\n",
			    idx, seen, rv));
			if (idx > 0) {
				spc = &spclist[idx];
				DPRINTF(("rump_sp: mainloop read [%u]\n", idx));
				switch (readframe(spc)) {
				case 0:
					break;
				case -1:
					serv_handledisco(idx);
					break;
				default:
					switch (spc->spc_hdr.rsp_class) {
					case RUMPSP_RESP:
						kickwaiter(spc);
						break;
					case RUMPSP_REQ:
						handlereq(spc);
						break;
					default:
						send_error_resp(spc,
						    spc->spc_hdr.rsp_reqno,
						    ENOENT);
						spcfreebuf(spc);
						break;
					}
					break;
				}

			} else {
				DPRINTF(("rump_sp: mainloop new connection\n"));

				if (__predict_false(spfini)) {
					close(spclist[0].spc_fd);
					serv_shutdown();
					goto out;
				}

				idx = serv_handleconn(pfdlist[0].fd,
				    sarg->sps_connhook, nfds == MAXCLI);
				if (idx)
					nfds++;
				if (idx > maxidx)
					maxidx = idx;
				DPRINTF(("rump_sp: maxid now %d\n", maxidx));
			}
		}
	}

 out:
	return NULL;
}

static unsigned cleanupidx;
static struct sockaddr *cleanupsa;
int
rumpuser_sp_init(const char *url, const struct rumpuser_sp_ops *spopsp,
	const char *ostype, const char *osrelease, const char *machine)
{
	pthread_t pt;
	struct spservarg *sarg;
	struct sockaddr *sap;
	char *p;
	unsigned idx;
	int error, s;

	p = strdup(url);
	if (p == NULL)
		return ENOMEM;
	error = parseurl(p, &sap, &idx, 1);
	free(p);
	if (error)
		return error;

	snprintf(banner, sizeof(banner), "RUMPSP-%d.%d-%s-%s/%s\n",
	    PROTOMAJOR, PROTOMINOR, ostype, osrelease, machine);

	s = socket(parsetab[idx].domain, SOCK_STREAM, 0);
	if (s == -1)
		return errno;

	spops = *spopsp;
	sarg = malloc(sizeof(*sarg));
	if (sarg == NULL) {
		close(s);
		return ENOMEM;
	}

	sarg->sps_sock = s;
	sarg->sps_connhook = parsetab[idx].connhook;

	cleanupidx = idx;
	cleanupsa = sap;

	/* sloppy error recovery */

	/*LINTED*/
	if (bind(s, sap, sap->sa_len) == -1) {
		fprintf(stderr, "rump_sp: server bind failed\n");
		return errno;
	}

	if (listen(s, MAXCLI) == -1) {
		fprintf(stderr, "rump_sp: server listen failed\n");
		return errno;
	}

	if ((error = pthread_create(&pt, NULL, spserver, sarg)) != 0) {
		fprintf(stderr, "rump_sp: cannot create wrkr thread\n");
		return errno;
	}
	pthread_detach(pt);

	return 0;
}

void
rumpuser_sp_fini()
{

	if (spclist[0].spc_fd) {
		parsetab[cleanupidx].cleanup(cleanupsa);
		shutdown(spclist[0].spc_fd, SHUT_RDWR);
		spfini = 1;
	}
}
