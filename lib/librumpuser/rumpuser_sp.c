/*      $NetBSD: rumpuser_sp.c,v 1.1 2010/10/27 20:44:50 pooka Exp $	*/

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
__RCSID("$NetBSD: rumpuser_sp.c,v 1.1 2010/10/27 20:44:50 pooka Exp $");

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

#include <rump/rump.h>
#include <rump/rumpuser.h>

//#define DEBUG
#ifdef DEBUG
#define DPRINTF(x) mydprintf x
static void
mydprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}
#else
#define DPRINTF(x)
#endif

/*
 * Bah, I hate writing on-off-wire conversions in C
 */

enum {
	RUMPSP_SYSCALL_REQ,	RUMPSP_SYSCALL_RESP,
	RUMPSP_COPYIN_REQ,	RUMPSP_COPYIN_RESP,
	RUMPSP_COPYOUT_REQ,	/* no copyout resp */
	RUMPSP_ANONMMAP_REQ,	RUMPSP_ANONMMAP_RESP
};

struct rsp_hdr {
	uint64_t rsp_len;
	uint64_t rsp_reqno;
	uint32_t rsp_type;
	/*
	 * We want this structure 64bit-aligned for typecast fun,
	 * so might as well use the following for something.
	 */
	uint32_t rsp_sysnum;
};
#define HDRSZ sizeof(struct rsp_hdr)

/*
 * Data follows the header.  We have two types of structured data.
 */

/* copyin/copyout */
struct rsp_copydata {
	size_t rcp_len;
	void *rcp_addr;
	uint8_t rcp_data[0];
};

/* syscall response */
struct rsp_sysresp {
	int rsys_error;
	register_t rsys_retval[2];
};


struct spclient {
	int spc_fd;

	/* incoming */
	struct rsp_hdr spc_hdr;
	uint8_t *spc_buf;
	size_t spc_off;

#if 0
	/* outgoing */
	int spc_obusy;
	pthread_mutex_t spc_omtx;
	pthread_cond_t spc_cv;
#endif
};

typedef int (*addrparse_fn)(const char *, int, struct sockaddr **);
typedef int (*connecthook_fn)(int);

#define MAXCLI 4

static struct pollfd pfdlist[MAXCLI];
static struct spclient spclist[MAXCLI];
static unsigned int nfds, maxidx;
static uint64_t nextreq;
static pthread_key_t spclient_tls;

static struct spclient clispc;


static int
dosend(struct spclient *spc, const void *data, size_t dlen)
{
	struct pollfd pfd;
	const uint8_t *sdata = data;
	ssize_t n;
	size_t sent;
	int fd = spc->spc_fd;

	pfd.fd = fd;
	pfd.events = POLLOUT;

	for (sent = 0, n = 0; sent < dlen; ) {
		if (n) {
			if (poll(&pfd, 1, INFTIM) == -1) {
				if (errno == EINTR)
					continue;
				return errno;
			}
		}

		n = write(fd, sdata + sent, dlen - sent);
		if (n == 0) {
			return EFAULT;
		}
		if (n == -1 && errno != EAGAIN) {
			return EFAULT;
		}
		sent += n;
	}

	return 0;
}

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
send_syscall_resp(struct spclient *spc, uint64_t reqno, int error,
	register_t retval[2])
{
	struct rsp_hdr rhdr;
	struct rsp_sysresp sysresp;

	rhdr.rsp_len = sizeof(rhdr) + sizeof(sysresp);
	rhdr.rsp_reqno = reqno;
	rhdr.rsp_type = RUMPSP_SYSCALL_RESP;
	rhdr.rsp_sysnum = 0;

	sysresp.rsys_error = error;
	memcpy(sysresp.rsys_retval, retval, sizeof(retval));

	dosend(spc, &rhdr, sizeof(rhdr));
	dosend(spc, &sysresp, sizeof(sysresp));

	return 0;
}

static int
send_copyin_req(struct spclient *spc, const void *remaddr, size_t dlen)
{
	struct rsp_hdr rhdr;
	struct rsp_copydata copydata;

	rhdr.rsp_len = sizeof(rhdr) + sizeof(copydata);
	rhdr.rsp_reqno = nextreq++;
	rhdr.rsp_type = RUMPSP_COPYIN_REQ;
	rhdr.rsp_sysnum = 0;

	copydata.rcp_addr = __UNCONST(remaddr);
	copydata.rcp_len = dlen;

	dosend(spc, &rhdr, sizeof(rhdr));
	dosend(spc, &copydata, sizeof(copydata));

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
send_copyout_req(struct spclient *spc, const void *remaddr,
	const void *data, size_t dlen)
{
	struct rsp_hdr rhdr;
	struct rsp_copydata copydata;

	rhdr.rsp_len = sizeof(rhdr) + sizeof(copydata) + dlen;
	rhdr.rsp_reqno = nextreq++;
	rhdr.rsp_type = RUMPSP_COPYOUT_REQ;
	rhdr.rsp_sysnum = 0;

	copydata.rcp_addr = __UNCONST(remaddr);
	copydata.rcp_len = dlen;

	dosend(spc, &rhdr, sizeof(rhdr));
	dosend(spc, &copydata, sizeof(copydata));
	dosend(spc, data, dlen);

	return 0;
}

static int
send_anonmmap_req(struct spclient *spc, size_t howmuch)
{
	struct rsp_hdr rhdr;

	rhdr.rsp_len = sizeof(rhdr) + sizeof(howmuch);
	rhdr.rsp_reqno = nextreq++;
	rhdr.rsp_type = RUMPSP_ANONMMAP_REQ;
	rhdr.rsp_sysnum = 0;

	dosend(spc, &rhdr, sizeof(rhdr));
	dosend(spc, &howmuch, sizeof(howmuch));

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

static void
serv_handle_disco(unsigned int idx)
{
	struct spclient *spc = &spclist[idx];
	int fd = spc->spc_fd;

	DPRINTF(("rump_sp: disconnecting [%u]\n", idx));

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

	/* find empty slot the simple way */
	for (i = 0; i < MAXCLI; i++) {
		if (pfdlist[i].fd == -1)
		break;
	}

	assert(i < MAXCLI);
	nfds++;

	pfdlist[i].fd = newfd;
	spclist[i].spc_fd = newfd;
	if (maxidx < i)
		maxidx = i;

	DPRINTF(("rump_sp: added new connection at idx %u\n", i));

	return 0;
}

static void
serv_handlesyscall(struct spclient *spc, struct rsp_hdr *rhdr, uint8_t *data)
{
	register_t retval[2];
	int rv, sysnum;

	sysnum = (int)rhdr->rsp_sysnum;
	DPRINTF(("rump_sp: handling syscall %d from client %d\n",
	    sysnum, 0));

	pthread_setspecific(spclient_tls, spc);
	rv = rump_pub_syscall(sysnum, data, retval);
	pthread_setspecific(spclient_tls, NULL);

	send_syscall_resp(spc, rhdr->rsp_reqno, rv, retval);
}

static int
readframe(struct spclient *spc)
{
	int fd = spc->spc_fd;
	size_t left;
	size_t framelen;
	ssize_t n;

	/* still reading header? */
	if (spc->spc_off < HDRSZ) {
		DPRINTF(("rump_sp: readframe getting header at offset %zu\n",
		    spc->spc_off));

		left = HDRSZ - spc->spc_off;
		/*LINTED: cast ok */
		n = read(fd, (uint8_t *)&spc->spc_hdr + spc->spc_off, left);
		if (n == 0) {
			return -1;
		}
		if (n == -1) {
			if (errno == EAGAIN)
				return 0;
			return -1;
		}

		spc->spc_off += n;
		if (spc->spc_off < HDRSZ)
			return -1;

		/*LINTED*/
		framelen = spc->spc_hdr.rsp_len;

		if (framelen < HDRSZ) {
			return -1;
		} else if (framelen == HDRSZ) {
			return 1;
		}

		spc->spc_buf = malloc(framelen - HDRSZ);
		if (spc->spc_buf == NULL) {
			return -1;
		}
		memset(spc->spc_buf, 0, framelen - HDRSZ);

		/* "fallthrough" */
	} else {
		/*LINTED*/
		framelen = spc->spc_hdr.rsp_len;
	}

	left = framelen - spc->spc_off;

	DPRINTF(("rump_sp: readframe getting body at offset %zu, left %zu\n",
	    spc->spc_off, left));

	if (left == 0)
		return 1;
	n = read(fd, spc->spc_buf + (spc->spc_off - HDRSZ), left);
	if (n == 0) {
		return -1;
	}
	if (n == -1) {
		if (errno == EAGAIN)
			return 0;
		return -1;
	}
	spc->spc_off += n;
	left -= n;

	/* got everything? */
	if (left == 0)
		return 1;
	else
		return 0;
}

int
rumpuser_sp_copyin(const void *uaddr, void *kaddr, size_t len)
{
	struct spclient *spc;
	struct pollfd pfd;

	spc = pthread_getspecific(spclient_tls);
	if (!spc)
		return EFAULT;

	send_copyin_req(spc, uaddr, len);

	pfd.fd = spc->spc_fd;
	pfd.events = POLLIN;
	do {
		poll(&pfd, 1, INFTIM);
	} while (readframe(spc) < 1);

	if (spc->spc_hdr.rsp_type != RUMPSP_COPYIN_RESP) {
		abort();
	}

	memcpy(kaddr, spc->spc_buf, len);
	free(spc->spc_buf);
	spc->spc_off = 0;

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

	send_copyout_req(spc, uaddr, kaddr, dlen);

	return 0;
}

int
rumpuser_sp_anonmmap(size_t howmuch, void **addr)
{
	struct spclient *spc;
	struct pollfd pfd;
	void *resp;

	spc = pthread_getspecific(spclient_tls);
	if (!spc)
		return EFAULT;

	send_anonmmap_req(spc, howmuch);

	pfd.fd = spc->spc_fd;
	pfd.events = POLLIN;
	do {
		poll(&pfd, 1, INFTIM);
	} while (readframe(spc) < 1);

	if (spc->spc_hdr.rsp_type != RUMPSP_ANONMMAP_RESP) {
		abort();
	}

	/*LINTED*/
	resp = *(void **)spc->spc_buf;
	spc->spc_off = 0;

	if (resp == NULL)
		return ENOMEM;

	*addr = resp;
	return 0;
}

int
rumpuser_sp_syscall(int sysnum, const void *data, size_t dlen,
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

/*
 *
 * Startup routines and mainloop for server.
 *
 */

static int
tcp_parse(const char *addr, int type, struct sockaddr **sa)
{
	struct sockaddr_in sin;
	char buf[64];
	const char *p;
	size_t l;
	int port;

	memset(&sin, 0, sizeof(sin));
	sin.sin_len = sizeof(sin);
	sin.sin_family = AF_INET;

	p = strchr(addr, ':');
	if (!p) {
		fprintf(stderr, "rump_sp_tcp: missing port specifier\n");
		return EINVAL;
	}

	l = p - addr;
	if (l > sizeof(buf)-1) {
		fprintf(stderr, "rump_sp_tcp: address too long\n");
		return EINVAL;
	}
	strncpy(buf, addr, l);
	buf[l] = '\0';

	/* special INADDR_ANY treatment */
	if (strcmp(buf, "*") == 0 || strcmp(buf, "0") == 0) {
		sin.sin_addr.s_addr = INADDR_ANY;
	} else {
		switch (inet_pton(AF_INET, buf, &sin.sin_addr)) {
		case 1:
			break;
		case 0:
			fprintf(stderr, "rump_sp_tcp: cannot parse %s\n", buf);
			return EINVAL;
		case -1:
			fprintf(stderr, "rump_sp_tcp: inet_pton failed\n");
			return errno;
		default:
			assert(/*CONSTCOND*/0);
			return EINVAL;
		}
	}

	if (type == RUMP_SP_CLIENT && sin.sin_addr.s_addr == INADDR_ANY) {
		fprintf(stderr, "rump_sp_tcp: client needs !INADDR_ANY\n");
		return EINVAL;
	}

	/* advance to port number & parse */
	p++;
	l = strspn(p, "0123456789");
	if (l == 0) {
		fprintf(stderr, "rump_sp_tcp: port now found: %s\n", p);
		return EINVAL;
	}
	strncpy(buf, p, l);
	buf[l] = '\0';

	if (*(p+l) != '/' && *(p+l) != '\0') {
		fprintf(stderr, "rump_sp_tcp: junk at end of port: %s\n", addr);
		return EINVAL;
	}

	port = atoi(buf);
	if (port < 0 || port >= (1<<(8*sizeof(in_port_t)))) {
		fprintf(stderr, "rump_sp_tcp: port %d out of range\n", port);
		return ERANGE;
	}
	sin.sin_port = htons(port);

	*sa = malloc(sizeof(sin));
	if (*sa == NULL)
		return errno;
	memcpy(*sa, &sin, sizeof(sin));
	return 0;
}

static int
tcp_connecthook(int s)
{
	int x;

	x = 1;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &x, sizeof(x));

	return 0;
}

/*ARGSUSED*/
static int
notsupp(void)
{

	fprintf(stderr, "rump_sp: support not yet implemented\n");
	return EOPNOTSUPP;
}

static int
success(void)
{

	return 0;
}

struct {
	const char *id;
	int domain;
	addrparse_fn ap;
	connecthook_fn connhook;
} parsetab[] = {
	{ "tcp", PF_INET, tcp_parse, tcp_connecthook },
	{ "unix", PF_LOCAL, (addrparse_fn)notsupp, (connecthook_fn)success },
	{ "tcp6", PF_INET6, (addrparse_fn)notsupp, (connecthook_fn)success },
};
#define NPARSE (sizeof(parsetab)/sizeof(parsetab[0]))

struct spservarg {
	int sps_sock;
	connecthook_fn sps_connhook;
};

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
					serv_handle_disco(idx);
					break;
				default:
					spc->spc_off = 0;
					serv_handlesyscall(spc,
					    &spc->spc_hdr, spc->spc_buf);
					spc->spc_buf = NULL;
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
rumpuser_sp_init(int *typep)
{
	struct spservarg *sarg;
	struct sockaddr *sap;
	char id[16];
	char *p, *p2;
	size_t l;
	unsigned i;
	int error, type, s;

	p = NULL;
	error = 0;

	type = RUMP_SP_NONE;
	if ((p = getenv("RUMP_SP_SERVER")) != NULL) {
		type = RUMP_SP_SERVER;
	}
	if ((p2 = getenv("RUMP_SP_CLIENT")) != NULL) {
		if (type != RUMP_SP_NONE)
			return EEXIST;
		type = RUMP_SP_CLIENT;
		p = p2;
	}

	/*
	 * Parse the url
	 */

	if (type == RUMP_SP_NONE)
		return RUMP_SP_NONE;

	p2 = strstr(p, "://");
	if (!p2) {
		fprintf(stderr, "rump_sp: invalid locator ``%s''\n", p);
		return EINVAL;
	}
	l = p2-p;
	if (l > sizeof(id)-1) {
		fprintf(stderr, "rump_sp: identifier too long in ``%s''\n", p);
		return EINVAL;
	}

	strncpy(id, p, l);
	id[l] = '\0';
	p2 += 3; /* beginning of address */

	for (i = 0; i < NPARSE; i++) {
		if (strcmp(id, parsetab[i].id) == 0) {
			error = parsetab[i].ap(p2, type, &sap);
			if (error)
				return error;
			break;
		}
	}
	if (i == NPARSE) {
		fprintf(stderr, "rump_sp: invalid identifier ``%s''\n", p);
		return EINVAL;
	}

	s = socket(parsetab[i].domain, SOCK_STREAM, 0);
	if (s == -1)
		return errno;

	if (type == RUMP_SP_CLIENT) {
		/*LINTED*/
		if (connect(s, sap, sap->sa_len) == -1) {
			fprintf(stderr, "rump_sp: client connect failed\n");
			return errno;
		}
		if ((error = parsetab[i].connhook(s)) != 0) {
			fprintf(stderr, "rump_sp: connect hook failed\n");
			return error;
		}

		clispc.spc_fd = s;
	} else {
		pthread_t pt;

		sarg = malloc(sizeof(*sarg));
		if (sarg == NULL) {
			close(s);
			return ENOMEM;
		}

		sarg->sps_sock = s;
		sarg->sps_connhook = parsetab[i].connhook;

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
	}

	*typep = type;
	return 0;
}
