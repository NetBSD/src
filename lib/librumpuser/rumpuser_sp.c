/*      $NetBSD: rumpuser_sp.c,v 1.7 2010/11/19 15:25:49 pooka Exp $	*/

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
 * Sysproxy routines.  This provides system RPC support over host sockets.
 * The most notable limitation is that the client and server must share
 * the same ABI.  This does not mean that they have to be the same
 * machine or that they need to run the same version of the host OS,
 * just that they must agree on the data structures.  This even *might*
 * work correctly from one hardware architecture to another.
 *
 * Not finished yet, i.e. don't use in production.  Lacks locking plus
 * handling of multiple clients and unexpected connection closes.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: rumpuser_sp.c,v 1.7 2010/11/19 15:25:49 pooka Exp $");

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

#include <rump/rumpuser.h>

#include "sp_common.c"

#define MAXCLI 4

static struct pollfd pfdlist[MAXCLI];
static struct spclient spclist[MAXCLI];
static unsigned int nfds, maxidx;
static pthread_key_t spclient_tls;

static struct rumpuser_sp_ops spops;

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
lwproc_newproc(void)
{
	int rv;

	spops.spop_schedule();
	rv = spops.spop_lwproc_newproc();
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
copyin_req(struct spclient *spc, const void *remaddr, size_t dlen, void **resp)
{
	struct rsp_hdr rhdr;
	struct rsp_copydata copydata;
	struct respwait rw;
	int rv;

	DPRINTF(("copyin_req: %zu bytes from %p\n", dlen, remaddr));

	rhdr.rsp_len = sizeof(rhdr) + sizeof(copydata);
	rhdr.rsp_class = RUMPSP_REQ;
	rhdr.rsp_type = RUMPSP_COPYIN;
	rhdr.rsp_sysnum = 0;

	copydata.rcp_addr = __UNCONST(remaddr);
	copydata.rcp_len = dlen;

	putwait(spc, &rw, &rhdr);

	sendlock(spc);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	rv = dosend(spc, &copydata, sizeof(copydata));
	sendunlock(spc);
	if (rv)
		return rv; /* XXX: unputwait */

	rv = waitresp(spc, &rw);

	DPRINTF(("copyin: response %d\n", rv));

	*resp = rw.rw_data;
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

	sendlock(spc);
	rv = dosend(spc, &rhdr, sizeof(rhdr));
	rv = dosend(spc, &howmuch, sizeof(howmuch));
	sendunlock(spc);
	if (rv)
		return rv; /* XXX: unputwait */

	rv = waitresp(spc, &rw);
	*resp = rw.rw_data;

	DPRINTF(("anonmmap: mapped at %p\n", **(void ***)resp));

	return rv;
}

static void
serv_handledisco(unsigned int idx)
{
	struct spclient *spc = &spclist[idx];
	int fd = spc->spc_fd;

	DPRINTF(("rump_sp: disconnecting [%u]\n", idx));

	lwproc_switch(spc->spc_lwp);
	lwproc_release();

	pthread_mutex_destroy(&spc->spc_mtx);
	pthread_cond_destroy(&spc->spc_cv);
	free(spc->spc_buf);
	memset(spc, 0, sizeof(*spc));
	close(fd);
	pfdlist[idx].fd = -1;
	nfds--;

	if (idx == maxidx) {
		while (idx--) {
			if (pfdlist[idx].fd != -1) {
				maxidx = idx;
				break;
			}
			assert(idx != 0);
		}
		DPRINTF(("rump_sp: set maxidx to [%u]\n", maxidx));
	}
}

static int
serv_handleconn(int fd, connecthook_fn connhook)
{
	struct sockaddr_storage ss;
	socklen_t sl = sizeof(ss);
	int newfd, flags, error;
	unsigned i;

	/*LINTED: cast ok */
	newfd = accept(fd, (struct sockaddr *)&ss, &sl);
	if (newfd == -1)
		return errno;

	/* XXX: should do some sort of handshake too */

	if (nfds == MAXCLI) {
		close(newfd); /* EBUSY */
		return EBUSY;
	}

	flags = fcntl(newfd, F_GETFL, 0);
	if (fcntl(newfd, F_SETFL, flags | O_NONBLOCK) == -1) {
		close(newfd);
		return errno;
	}
	flags = 1;

	if ((error = connhook(newfd)) != 0) {
		close(newfd);
		return error;
	}

	if ((error = lwproc_newproc()) != 0) {
		close(newfd);
		return error;
	}

	/* find empty slot the simple way */
	for (i = 0; i < MAXCLI; i++) {
		if (pfdlist[i].fd == -1)
			break;
	}

	assert(i < MAXCLI);
	nfds++;

	pfdlist[i].fd = newfd;
	spclist[i].spc_fd = newfd;
	spclist[i].spc_lwp = lwproc_curlwp();
	spclist[i].spc_istatus = SPCSTATUS_BUSY; /* dedicated receiver */

	TAILQ_INIT(&spclist[i].spc_respwait);
	pthread_mutex_init(&spclist[i].spc_mtx, NULL);
	pthread_cond_init(&spclist[i].spc_cv, NULL);

	if (maxidx < i)
		maxidx = i;

	DPRINTF(("rump_sp: added new connection at idx %u, pid %d\n",
	    i, 9)); /* XXX: getpid not spop */

	lwproc_switch(NULL);

	return 0;
}

static void
serv_handlesyscall(struct spclient *spc, struct rsp_hdr *rhdr, uint8_t *data)
{
	register_t retval[2] = {0, 0};
	int rv, sysnum;

	sysnum = (int)rhdr->rsp_sysnum;
	DPRINTF(("rump_sp: handling syscall %d from client %d\n",
	    sysnum, 0));

	pthread_setspecific(spclient_tls, spc);
	lwproc_switch(spc->spc_lwp);
	rv = rumpsyscall(sysnum, data, retval);
	lwproc_switch(NULL);
	pthread_setspecific(spclient_tls, NULL);
	free(data);

	DPRINTF(("rump_sp: got return value %d & %d/%d\n",
	    rv, retval[0], retval[1]));

	send_syscall_resp(spc, rhdr->rsp_reqno, rv, retval);
}

struct sysbouncearg {
	struct spclient *sba_spc;
	struct rsp_hdr sba_hdr;
	uint8_t *sba_data;
};
static void *
serv_syscallbouncer(void *arg)
{
	struct sysbouncearg *barg = arg;

	serv_handlesyscall(barg->sba_spc, &barg->sba_hdr, barg->sba_data);
	free(arg);
	return NULL;
}

int
rumpuser_sp_copyin(const void *uaddr, void *kaddr, size_t len)
{
	struct spclient *spc;
	void *rdata;

	spc = pthread_getspecific(spclient_tls);
	if (!spc)
		return EFAULT;

	copyin_req(spc, uaddr, len, &rdata);

	memcpy(kaddr, rdata, len);
	free(rdata);

	return 0;
}

int
rumpuser_sp_copyout(const void *kaddr, void *uaddr, size_t dlen)
{
	struct spclient *spc;

	spc = pthread_getspecific(spclient_tls);
	if (!spc) {
		DPRINTF(("rump_sp: copyout curlwp not found\n"));
		return EFAULT;
	}

	if (send_copyout_req(spc, uaddr, kaddr, dlen) != 0)
		return EFAULT;
	return 0;
}

int
rumpuser_sp_anonmmap(size_t howmuch, void **addr)
{
	struct spclient *spc;
	void *resp, *rdata;
	int rv;

	spc = pthread_getspecific(spclient_tls);
	if (!spc)
		return EFAULT;

	rv = anonmmap_req(spc, howmuch, &rdata);
	if (rv)
		return rv;

	resp = *(void **)rdata;
	free(rdata);

	if (resp == NULL) {
		return ENOMEM;
	}

	*addr = resp;
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

static void
handlereq(struct spclient *spc)
{
	struct sysbouncearg *sba;
	pthread_attr_t pattr;
	pthread_t pt;
	int rv;

	/* XXX: check that it's a syscall */

	sba = malloc(sizeof(*sba));
	if (sba == NULL) {
		/* panic */
		abort();
	}

	sba->sba_spc = spc;
	sba->sba_hdr = spc->spc_hdr;
	sba->sba_data = spc->spc_buf;

	spc->spc_buf = NULL;
	spc->spc_off = 0;

	pthread_attr_init(&pattr);
	pthread_attr_setdetachstate(&pattr, 1);

	if ((rv = pthread_create(&pt, &pattr, serv_syscallbouncer, sba)) != 0) {
		/* panic */
		abort();
	}
}

static void *
spserver(void *arg)
{
	struct spservarg *sarg = arg;
	unsigned idx;
	int seen;
	int rv;

	for (idx = 1; idx < MAXCLI; idx++) {
		pfdlist[idx].fd = -1;
		pfdlist[idx].events = POLLIN;
	}
	pfdlist[0].fd = sarg->sps_sock;
	pfdlist[0].events = POLLIN;
	nfds = 1;
	maxidx = 0;

	DPRINTF(("rump_sp: server mainloop\n"));

	for (;;) {
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

		for (idx = 0; seen < rv; idx++) {
			assert(idx < MAXCLI);

			if ((pfdlist[idx].revents & POLLIN) == 0)
				continue;

			seen++;
			DPRINTF(("rump_sp: activity at [%u] %d/%d\n",
			    idx, seen, rv));
			if (idx > 0) {
				struct spclient *spc = &spclist[idx];

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
						printf("PANIC\n");
						abort();
						break;
					}
					break;
				}
			} else {
				DPRINTF(("rump_sp: mainloop new connection\n"));
				serv_handleconn(pfdlist[0].fd,
				    sarg->sps_connhook);
			}
		}
	}

	return NULL;
}

int
rumpuser_sp_init(const struct rumpuser_sp_ops *spopsp, const char *url)
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

	/* sloppy error recovery */

	error = pthread_key_create(&spclient_tls, NULL);
	if (error) {
		fprintf(stderr, "rump_sp: tls create failed\n");
		return error;
	}

	/*LINTED*/
	if (bind(s, sap, sap->sa_len) == -1) {
		fprintf(stderr, "rump_sp: server bind failed\n");
		return errno;
	}
	if (listen(s, 20) == -1) {
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
