/*	$NetBSD: svr4_net.c,v 1.59.2.1 2014/08/10 06:54:34 tls Exp $	*/

/*-
 * Copyright (c) 1994, 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

/*
 * Emulate /dev/{udp,tcp,...}
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: svr4_net.c,v 1.59.2.1 2014/08/10 06:54:34 tls Exp $");

#define COMPAT_SVR4 1

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/mount.h>

#include <sys/syscallargs.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_ioctl.h>
#include <compat/svr4/svr4_stropts.h>
#include <compat/svr4/svr4_socket.h>

dev_type_open(svr4_netopen);

const struct cdevsw svr4_net_cdevsw = {
	.d_open = svr4_netopen,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER,
};

/*
 * Device minor numbers
 */
enum {
	dev_arp			= 26,
	dev_icmp		= 27,
	dev_ip			= 28,
	dev_tcp			= 35,
	dev_udp			= 36,
	dev_rawip		= 37,
	dev_unix_dgram		= 38,
	dev_unix_stream		= 39,
	dev_unix_ord_stream	= 40
};

int svr4_netattach(int);

int svr4_soo_close(file_t *);

static const struct fileops svr4_netops = {
	.fo_read = soo_read,
	.fo_write = soo_write,
	.fo_ioctl = soo_ioctl,
	.fo_fcntl = soo_fcntl,
	.fo_poll = soo_poll,
	.fo_stat = soo_stat,
	.fo_close = svr4_soo_close,
	.fo_kqfilter = soo_kqfilter,
	.fo_restart = soo_restart,
};


/*
 * Used by new config, but we don't need it.
 */
int
svr4_netattach(int n)
{
	return 0;
}


int
svr4_netopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int type, protocol;
	int fd;
	file_t *fp;
	struct socket *so;
	int error;
	int family;

	DPRINTF(("netopen("));

	if (curlwp->l_dupfd >= 0)	/* XXX */
		return ENODEV;

	switch (minor(dev)) {
	case dev_udp:
		family = AF_INET;
		type = SOCK_DGRAM;
		protocol = IPPROTO_UDP;
		DPRINTF(("udp, "));
		break;

	case dev_tcp:
		family = AF_INET;
		type = SOCK_STREAM;
		protocol = IPPROTO_TCP;
		DPRINTF(("tcp, "));
		break;

	case dev_ip:
	case dev_rawip:
		family = AF_INET;
		type = SOCK_RAW;
		protocol = IPPROTO_IP;
		DPRINTF(("ip, "));
		break;

	case dev_icmp:
		family = AF_INET;
		type = SOCK_RAW;
		protocol = IPPROTO_ICMP;
		DPRINTF(("icmp, "));
		break;

	case dev_unix_dgram:
		family = AF_LOCAL;
		type = SOCK_DGRAM;
		protocol = 0;
		DPRINTF(("unix-dgram, "));
		break;

	case dev_unix_stream:
	case dev_unix_ord_stream:
		family = AF_LOCAL;
		type = SOCK_STREAM;
		protocol = 0;
		DPRINTF(("unix-stream, "));
		break;

	default:
		DPRINTF(("%"PRId32");\n", minor(dev)));
		return EOPNOTSUPP;
	}

	if ((error = fd_allocfile(&fp, &fd)) != 0)
		return error;

	if ((error = socreate(family, &so, type, protocol, l, NULL)) != 0) {
		DPRINTF(("socreate error %d\n", error));
		fd_abort(curproc, fp, fd);
		return error;
	}

	error = fd_clone(fp, fd, flag, &svr4_netops, so);
	fp->f_type = DTYPE_SOCKET;
	(void)svr4_stream_get(fp);

	DPRINTF(("ok);\n"));
	return error;
}


int
svr4_soo_close(file_t *fp)
{
	struct socket *so = fp->f_data;

	svr4_delete_socket(curproc, fp);
	free(so->so_internal, M_NETADDR);
	return soo_close(fp);
}


struct svr4_strm *
svr4_stream_get(file_t *fp)
{
	struct socket *so;
	struct svr4_strm *st;

	if (fp == NULL || fp->f_type != DTYPE_SOCKET)
		return NULL;

	so = fp->f_data;

	if (so->so_internal)
		return so->so_internal;

	/* Allocate a new one. */
	fp->f_ops = &svr4_netops;
	st = malloc(sizeof(struct svr4_strm), M_NETADDR, M_WAITOK);
	st->s_family = so->so_proto->pr_domain->dom_family;
	st->s_cmd = ~0;
	st->s_afd = -1;
	st->s_eventmask = 0;
	so->so_internal = st;

	return st;
}
