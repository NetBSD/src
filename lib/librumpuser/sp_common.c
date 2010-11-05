/*      $NetBSD: sp_common.c,v 1.2 2010/11/05 14:23:45 pooka Exp $	*/

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
 * Common client/server sysproxy routines.  #included.
 */

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
	struct lwp *spc_lwp;

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

typedef int (*addrparse_fn)(const char *, struct sockaddr **, int);
typedef int (*connecthook_fn)(int);

static uint64_t nextreq;

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

static int
tcp_parse(const char *addr, struct sockaddr **sa, int allow_wildcard)
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

	if (!allow_wildcard && sin.sin_addr.s_addr == INADDR_ANY) {
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

static int
unix_parse(const char *addr, struct sockaddr **sa, int allow_wildcard)
{
	struct sockaddr_un sun;

	if (strlen(addr) > sizeof(sun.sun_path))
		return ENAMETOOLONG;

	/*
	 * The pathname can be all kinds of spaghetti elementals,
	 * so meek and obidient we accept everything.
	 */
	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_LOCAL;
	strlcpy(sun.sun_path, addr, sizeof(sun.sun_path));
	sun.sun_len = SUN_LEN(&sun);

	*sa = malloc(sun.sun_len);
	if (*sa == NULL)
		return errno;
	memcpy(*sa, &sun, sun.sun_len);

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
	{ "unix", PF_LOCAL, unix_parse, (connecthook_fn)success },
	{ "tcp6", PF_INET6, (addrparse_fn)notsupp, (connecthook_fn)success },
};
#define NPARSE (sizeof(parsetab)/sizeof(parsetab[0]))

static int
parseurl(const char *url, struct sockaddr **sap, unsigned *idxp,
	int allow_wildcard)
{
	char id[16];
	const char *p, *p2;
	size_t l;
	unsigned i;
	int error;

	/*
	 * Parse the url
	 */

	p = url;
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
			error = parsetab[i].ap(p2, sap, allow_wildcard);
			if (error)
				return error;
			break;
		}
	}
	if (i == NPARSE) {
		fprintf(stderr, "rump_sp: invalid identifier ``%s''\n", p);
		return EINVAL;
	}

	*idxp = i;
	return 0;
}
