/*	$NetBSD: sysproxy_socket.c,v 1.9 2010/09/01 19:57:52 pooka Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by The Nokia Foundation.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sysproxy_socket.c,v 1.9 2010/09/01 19:57:52 pooka Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/queue.h>
#include <sys/syscall.h>
#include <sys/atomic.h>

#include <rump/rump.h>
#include <rump/rumpuser.h>

#include "rump_private.h"

/*
 * This is a very simple RPC mechanism.  It defines:
 *
 *   1) system call
 *   2) copyin
 *   3) copyout
 *
 * Currently all the data is in host format, so this can't really be
 * used to make cross-site calls.  Extending to something like XMLRPC
 * is possible, but takes some amount of programming effort, since all
 * the kernel data structures and types are defined in C.
 *
 * XXX: yes, this is overall implemented in a very lazy fashion
 * currently.  Hopefully it will get better.  And hopefully the
 * global "sock" will go away, it's quite disgusting.
 */

enum rumprpc { RUMPRPC_LWP_CREATE, RUMPRPC_LWP_EXIT,
	       RUMPRPC_SYSCALL, RUMPRPC_SYSCALL_RESP,
	       RUMPRPC_COPYIN, RUMPRPC_COPYIN_RESP,
	       RUMPRPC_COPYOUT, RUMPRPC_COPYOUT_RESP };

struct rumprpc_head {
	uint64_t rpch_flen;
	uint32_t rpch_reqno;
	int rpch_type;
};

struct rumprpc_sysreq {
	struct rumprpc_head rpc_head;
	int rpc_sysnum;
	uint8_t rpc_data[0];
};

struct rumprpc_sysresp {
	struct rumprpc_head rpc_head;
	int rpc_error;
	register_t rpc_retval;
};

struct rumprpc_copydata {
	struct rumprpc_head rpc_head;
	void *rpc_addr;
	size_t rpc_len;
	uint8_t rpc_data[0];
};

struct sysproxy_qent {
	uint32_t reqno;
	struct rumprpc_head *rpch_resp;

	kcondvar_t cv;
	TAILQ_ENTRY(sysproxy_qent) entries;
};

static TAILQ_HEAD(, sysproxy_qent) sendq = TAILQ_HEAD_INITIALIZER(sendq);
static TAILQ_HEAD(, sysproxy_qent) recvq = TAILQ_HEAD_INITIALIZER(recvq);
static bool sendavail = true;
static kmutex_t sendmtx, recvmtx;
static unsigned reqno;

static struct sysproxy_qent *
get_qent(void)
{
	struct sysproxy_qent *qent;
	unsigned myreq = atomic_inc_uint_nv(&reqno);

	qent = kmem_alloc(sizeof(*qent), KM_SLEEP);

	qent->reqno = myreq;
	qent->rpch_resp = NULL;
	cv_init(&qent->cv, "sproxyq");

	return qent;
}

static void
put_qent(struct sysproxy_qent *qent)
{

	cv_destroy(&qent->cv);
	kmem_free(qent, sizeof(*qent));
}

static int
write_n(int s, uint8_t *data, size_t dlen)
{
	ssize_t n;
	size_t done;
	int error;

	error = 0;
	for (done = 0; done < dlen; done += n) {
		n = rumpuser_write(s, data + done, dlen - done, &error);
		if (n <= 0) {
			if (n == -1)
				return error;
			return ECONNRESET;
		}
	}

	return error;
}

static int
read_n(int s, uint8_t *data, size_t dlen)
{
	ssize_t n;
	size_t done;
	int error;

	error = 0;
	for (done = 0; done < dlen; done += n) {
		n = rumpuser_read(s, data + done, dlen - done, &error);
		if (n <= 0) {
			if (n == -1)
				return error;
			return ECONNRESET;
		}
	}

	return error;
}

static void
dosend(int s, struct sysproxy_qent *qent, uint8_t *data, size_t len, bool rq)
{
	int error;

	/*
	 * Send.  If the sendq is empty, we send in current thread context.
	 * Otherwise we enqueue ourselves and block until we are able to
	 * send in current thread context, i.e. we are the first in the queue.
	 */
	mutex_enter(&sendmtx);
	if (!sendavail) {
		TAILQ_INSERT_TAIL(&sendq, qent, entries);
		while (qent != TAILQ_FIRST(&sendq))
			cv_wait(&qent->cv, &sendmtx);
		KASSERT(qent == TAILQ_FIRST(&sendq));
		TAILQ_REMOVE(&sendq, qent, entries);
	}
	sendavail = false;
	mutex_exit(&sendmtx);

	/*
	 * Put ourselves onto the receive queue already now in case
	 * the response arrives "immediately" after sending.
	 */
	if (rq) {
		mutex_enter(&recvmtx);
		TAILQ_INSERT_TAIL(&recvq, qent, entries);
		mutex_exit(&recvmtx);
	}

	/* XXX: need better error recovery */
	if ((error = write_n(s, data, len)) != 0)
		panic("unrecoverable error %d (sloppy)", error);
}

struct wrk {
	void (*wfn)(void *arg);
	void *arg;
	kcondvar_t wrkcv;
	LIST_ENTRY(wrk) entries;
};

static LIST_HEAD(, wrk) idlewrker = LIST_HEAD_INITIALIZER(idlewrker);

#define NIDLE_MAX 5

static kmutex_t wrkmtx;
static unsigned nwrk, nidle;

/*
 * workers for handling requests.  comes with simple little pooling
 * for threads.
 */
static void
wrkthread(void *arg)
{
	struct wrk *wrk = arg;

	mutex_enter(&wrkmtx);
	nidle++;
	for (;;) {
		while (wrk->wfn == NULL)
			cv_wait(&wrk->wrkcv, &wrkmtx);
		nidle--;
		mutex_exit(&wrkmtx);

		wrk->wfn(wrk->arg);
		wrk->wfn = NULL;
		wrk->arg = NULL;

		mutex_enter(&wrkmtx);
		if (++nidle > NIDLE_MAX) {
			nidle--;
			break;
		}
		LIST_INSERT_HEAD(&idlewrker, wrk, entries);
	}
	nwrk--;
	mutex_exit(&wrkmtx);

	cv_destroy(&wrk->wrkcv);
	kmem_free(wrk, sizeof(*wrk));
	kthread_exit(0);
}

/*
 * Enqueue work into a separate thread context.  Will create a new
 * thread if there are none available.
 */
static void
wrkenqueue(void (*wfn)(void *), void *arg)
{
	struct wrk *wrk;
	int error;

	mutex_enter(&wrkmtx);
	if (nidle == 0) {
		nwrk++;
		if (nwrk > 30)
			printf("syscall proxy warning: over 30 workers\n");
		mutex_exit(&wrkmtx);
		wrk = kmem_zalloc(sizeof(*wrk), KM_SLEEP);
		cv_init(&wrk->wrkcv, "sproxywk");
 retry:
		error = kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
		    wrkthread, wrk, NULL, "spw_%d", nwrk);
		if (error) {
			printf("kthread_create failed: %d.  retry.\n", error);
			kpause("eagain", false, hz, NULL);
			goto retry;
		}
	} else {
		wrk = LIST_FIRST(&idlewrker);
		LIST_REMOVE(wrk, entries);
		mutex_exit(&wrkmtx);
	}

	wrk->wfn = wfn;
	wrk->arg = arg;

	mutex_enter(&wrkmtx);
	cv_signal(&wrk->wrkcv);
	mutex_exit(&wrkmtx);
}

static int sock; /* XXXXXX */

int
rump_sysproxy_copyout(const void *kaddr, void *uaddr, size_t len)
{
	struct rumprpc_copydata *req;
	struct sysproxy_qent *qent = get_qent();
	size_t totlen = sizeof(*req) + len;

	req = kmem_alloc(totlen, KM_SLEEP);

	req->rpc_head.rpch_flen = totlen;
	req->rpc_head.rpch_reqno = qent->reqno;
	req->rpc_head.rpch_type = RUMPRPC_COPYOUT;
	req->rpc_addr = uaddr;
	req->rpc_len = len;
	memcpy(req->rpc_data, kaddr, len);

	/* XXX: handle async? */
	dosend(sock, qent, (uint8_t *)req, totlen, false);
	kmem_free(req, totlen);
	put_qent(qent);

	return 0;
}

int
rump_sysproxy_copyin(const void *uaddr, void *kaddr, size_t len)
{
	struct sysproxy_qent *qent = get_qent();
	struct rumprpc_copydata req, *resp;

	/* build request */
	req.rpc_head.rpch_flen = sizeof(req);
	req.rpc_head.rpch_reqno = qent->reqno;
	req.rpc_head.rpch_type = RUMPRPC_COPYIN;

	req.rpc_addr = __UNCONST(uaddr);
	req.rpc_len = len;

	dosend(sock, qent, (uint8_t *)&req, sizeof(req), true);

	/*
	 * Wake up next sender or just toggle availability
	 */
	mutex_enter(&sendmtx);
	if (TAILQ_EMPTY(&sendq)) {
		sendavail = true;
	} else {
		cv_signal(&TAILQ_FIRST(&sendq)->cv);
	}
	mutex_exit(&sendmtx);

	/* Wait for response */
	mutex_enter(&recvmtx);
	while (qent->rpch_resp == NULL)
		cv_wait(&qent->cv, &recvmtx);
	mutex_exit(&recvmtx);

	resp = (struct rumprpc_copydata *)qent->rpch_resp;
	/* we trust our kernel */
	KASSERT(resp->rpc_head.rpch_type == RUMPRPC_COPYIN_RESP);

	memcpy(kaddr, resp->rpc_data, len);
	kmem_free(resp, resp->rpc_head.rpch_flen);
	put_qent(qent);

	return 0;
}

struct vmspace rump_sysproxy_vmspace;

static void
handle_syscall(void *arg)
{
	struct rumprpc_sysreq *req = arg;
	struct sysproxy_qent *qent = get_qent();
	struct rumprpc_sysresp resp;
	struct sysent *callp;
	struct lwp *mylwp;

	resp.rpc_head.rpch_flen = sizeof(resp);
	resp.rpc_head.rpch_reqno = req->rpc_head.rpch_reqno;
	resp.rpc_head.rpch_type = RUMPRPC_SYSCALL_RESP;

	if (__predict_false(req->rpc_sysnum >= SYS_NSYSENT)) {
		resp.rpc_error = ENOSYS;
		dosend(sock, qent, (uint8_t *)&resp, sizeof(resp), false);
		kmem_free(req, req->rpc_head.rpch_flen);
		put_qent(qent);
		return;
	}

	callp = rump_sysent + req->rpc_sysnum;
	mylwp = curlwp;
	rump_lwproc_newproc();
	rump_set_vmspace(&rump_sysproxy_vmspace);

	resp.rpc_retval = 0; /* default */
	resp.rpc_error = callp->sy_call(curlwp, (void *)req->rpc_data,
	    &resp.rpc_retval);
	rump_lwproc_releaselwp();
	rump_lwproc_switch(mylwp);
	kmem_free(req, req->rpc_head.rpch_flen);

	dosend(sock, qent, (uint8_t *)&resp, sizeof(resp), false);
	put_qent(qent);
}

static void
handle_copyin(void *arg)
{
	struct rumprpc_copydata *req = arg;
	struct sysproxy_qent *qent = get_qent();
	struct rumprpc_copydata *resp;
	size_t totlen = sizeof(*resp) + req->rpc_len;

	resp = kmem_alloc(totlen, KM_SLEEP);
	resp->rpc_head.rpch_flen = totlen;
	resp->rpc_head.rpch_reqno = req->rpc_head.rpch_reqno;
	resp->rpc_head.rpch_type = RUMPRPC_COPYIN_RESP;
	memcpy(resp->rpc_data, req->rpc_addr, req->rpc_len);
	kmem_free(req, req->rpc_head.rpch_flen);

	dosend(sock, qent, (uint8_t *)resp, totlen, false);
	kmem_free(resp, totlen);
	put_qent(qent);
}

/*
 * Client side.  We can either get the results of an earlier syscall or
 * get a request for a copyin/out.
 */
static void
recvthread(void *arg)
{
	struct rumprpc_head rpch;
	uint8_t *rpc;
	int s = (uintptr_t)arg;
	int error;

	for (;;) {
		error = read_n(s, (uint8_t *)&rpch, sizeof(rpch));
		if (error)
			panic("%d", error);

		rpc = kmem_alloc(rpch.rpch_flen , KM_SLEEP);
		error = read_n(s, rpc + sizeof(struct rumprpc_head),
		    rpch.rpch_flen - sizeof(struct rumprpc_head));
		if (error)
			panic("%d", error);
		memcpy(rpc, &rpch, sizeof(rpch));

		switch (rpch.rpch_type) {
		case RUMPRPC_SYSCALL:
			/* assert server */
			wrkenqueue(handle_syscall, rpc);
			break;

		case RUMPRPC_SYSCALL_RESP:
			/* assert client */
		{
			struct sysproxy_qent *qent;

			mutex_enter(&recvmtx);
			TAILQ_FOREACH(qent, &recvq, entries)
				if (qent->reqno == rpch.rpch_reqno)
					break;
			if (!qent) {
				mutex_exit(&recvmtx);
				kmem_free(rpc, rpch.rpch_flen);
				break;
			}
			TAILQ_REMOVE(&recvq, qent, entries);
			qent->rpch_resp = (void *)rpc;
			cv_signal(&qent->cv);
			mutex_exit(&recvmtx);

			break;
		}
		case RUMPRPC_COPYIN:
			/* assert client */
			wrkenqueue(handle_copyin, rpc);
			break;

		case RUMPRPC_COPYIN_RESP:
			/* assert server */
		{
			struct sysproxy_qent *qent;

			mutex_enter(&recvmtx);
			TAILQ_FOREACH(qent, &recvq, entries)
				if (qent->reqno == rpch.rpch_reqno)
					break;
			if (!qent) {
				mutex_exit(&recvmtx);
				kmem_free(rpc, rpch.rpch_flen);
				break;
			}
			TAILQ_REMOVE(&recvq, qent, entries);
			qent->rpch_resp = (void *)rpc;
			cv_signal(&qent->cv);
			mutex_exit(&recvmtx);

			break;
		}

		case RUMPRPC_COPYOUT:
		{
			struct rumprpc_copydata *req = (void *)rpc;

			memcpy(req->rpc_addr, req->rpc_data, req->rpc_len);
			kmem_free(req, req->rpc_head.rpch_flen);
			break;
		}

		default:
			printf("invalid type %d\n", rpch.rpch_type);
		};
	}
}

/*
 * Make a syscall to a remote site over a socket.  I'm not really sure
 * if this should use kernel or user networking.  Currently it uses
 * user networking, but could be changed.
 */
static int
rump_sysproxy_socket(int num, void *arg, uint8_t *data, size_t dlen,
	register_t *retval)
{
	struct sysproxy_qent *qent;
	struct rumprpc_sysreq *call;
	struct rumprpc_sysresp *resp;
	size_t totlen = sizeof(*call) + dlen;
	int s = (uintptr_t)arg;
	int error;

	qent = get_qent();

	/* build request */
	/* should we prefer multiple writes if dlen > magic_constant? */
	call = kmem_alloc(totlen, KM_SLEEP);
	call->rpc_head.rpch_flen = totlen;
	call->rpc_head.rpch_reqno = qent->reqno;
	call->rpc_head.rpch_type = RUMPRPC_SYSCALL;
	call->rpc_sysnum = num;
	memcpy(call->rpc_data, data, dlen);

	dosend(s, qent, (uint8_t *)call, totlen, true);
	kmem_free(call, totlen);

	/*
	 * Wake up next sender or just toggle availability
	 */
	mutex_enter(&sendmtx);
	if (TAILQ_EMPTY(&sendq)) {
		sendavail = true;
	} else {
		cv_signal(&TAILQ_FIRST(&sendq)->cv);
	}
	mutex_exit(&sendmtx);

	/* Wait for response */
	mutex_enter(&recvmtx);
	while (qent->rpch_resp == NULL)
		cv_wait(&qent->cv, &recvmtx);
	mutex_exit(&recvmtx);

	resp = (struct rumprpc_sysresp *)qent->rpch_resp;
	/* we trust our kernel */
	KASSERT(resp->rpc_head.rpch_type == RUMPRPC_SYSCALL_RESP);

	*retval = resp->rpc_retval;
	error = resp->rpc_error;

	kmem_free(resp, resp->rpc_head.rpch_flen);
	put_qent(qent);

	return error;
}

int
rump_sysproxy_socket_setup_client(int s)
{
	int error;

	error = kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
	    recvthread, (void *)(uintptr_t)s, NULL, "sysproxy_recv");
	if (error)
		return error;

	mutex_init(&wrkmtx, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sendmtx, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&recvmtx, MUTEX_DEFAULT, IPL_NONE);
	error = rump_sysproxy_set(rump_sysproxy_socket,
	    (void *)(uintptr_t)s);
	/* XXX: handle */

	sock = s;

	return error;
}

int
rump_sysproxy_socket_setup_server(int s)
{
	int error;

	error = kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
	    recvthread, (void *)(uintptr_t)s, NULL, "sysproxy_recv");
	if (error)
		return error;

	mutex_init(&wrkmtx, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&recvmtx, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sendmtx, MUTEX_DEFAULT, IPL_NONE);

	sock = s;

	return 0;
}
