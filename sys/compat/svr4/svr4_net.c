/*	$NetBSD: svr4_net.c,v 1.24.2.2 2001/04/09 01:55:49 nathanw Exp $	 */

/*-
 * Copyright (c) 1994 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
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

/*
 * Emulate /dev/{udp,tcp,...}
 */

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

/*
 * Device minor numbers
 */
enum {
	dev_ptm			= 10,
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

int svr4_netattach __P((int));

int svr4_soo_close __P((struct file *, struct proc *));
int svr4_ptm_alloc __P((struct proc *));

static struct fileops svr4_netops = {
	soo_read, soo_write, soo_ioctl, soo_fcntl, soo_poll,
	soo_stat, svr4_soo_close
};


/*
 * Used by new config, but we don't need it.
 */
int
svr4_netattach(n)
	int n;
{
	return 0;
}


int
svr4_netopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int type, protocol;
	int fd;
	struct file *fp;
	struct socket *so;
	int error;
	int family;

	DPRINTF(("netopen("));

	if (p->p_dupfd >= 0)
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

	case dev_ptm:
		DPRINTF(("ptm);\n"));
		return svr4_ptm_alloc(p);

	default:
		DPRINTF(("%d);\n", minor(dev)));
		return EOPNOTSUPP;
	}

	/* falloc() will use the descriptor for us */
	if ((error = falloc(p, &fp, &fd)) != 0)
		return error;

	if ((error = socreate(family, &so, type, protocol)) != 0) {
		DPRINTF(("socreate error %d\n", error));
		fdremove(p->p_fd, fd);
		FILE_UNUSE(fp, NULL);
		ffree(fp);
		return error;
	}

	fp->f_flag = FREAD|FWRITE;
	fp->f_type = DTYPE_SOCKET;
	fp->f_ops = &svr4_netops;

	fp->f_data = (caddr_t)so;
	(void) svr4_stream_get(fp);

	DPRINTF(("ok);\n"));

	p->p_dupfd = fd;
	FILE_UNUSE(fp, p);
	return ENXIO;
}


int
svr4_soo_close(fp, p)
	struct file *fp;
	struct proc *p;
{
	struct socket *so = (struct socket *) fp->f_data;

	svr4_delete_socket(p, fp);
	free(so->so_internal, M_NETADDR);
	return soo_close(fp, p);
}


int
svr4_ptm_alloc(p)
	struct proc *p;
{
	/*
	 * XXX this is very, very ugly.  But I can't find a better
	 * way that won't duplicate a big amount of code from
	 * sys_open().  Ho hum...
	 *
	 * Fortunately for us, Solaris (at least 2.5.1) makes the
	 * /dev/ptmx open automatically just open a pty, that (after
	 * STREAMS I_PUSHes), is just a plain pty.  fstat() is used
	 * to get the minor device number to map to a tty.
	 * 
	 * Cycle through the names. If sys_open() returns ENOENT (or
	 * ENXIO), short circuit the cycle and exit.
	 */
	static char ptyname[] = "/dev/ptyXX";
	static const char ttyletters[] = "pqrstuvwxyzPQRST";
	static const char ttynumbers[] = "0123456789abcdef";
	caddr_t sg = stackgap_init(p->p_emul);
	char *path = stackgap_alloc(&sg, sizeof(ptyname));
	struct sys_open_args oa;
	int l = 0, n = 0;
	register_t fd = -1;
	int error;

	SCARG(&oa, path) = path;
	SCARG(&oa, flags) = O_RDWR;
	SCARG(&oa, mode) = 0;

	while (fd == -1) {
		ptyname[8] = ttyletters[l];
		ptyname[9] = ttynumbers[n];

		if ((error = copyout(ptyname, path, sizeof(ptyname))) != 0)
			return error;

		switch (error = sys_open(curproc, &oa, &fd)) { /* XXX NJWLWP */
		case ENOENT:
		case ENXIO:
			return error;
		case 0:
			p->p_dupfd = fd;
			return ENXIO;
		default:
			if (ttynumbers[++n] == '\0') {
				if (ttyletters[++l] == '\0')
					break;
				n = 0;
			}
		}
	}
	return ENOENT;
}


struct svr4_strm *
svr4_stream_get(fp)
	struct file *fp;
{
	struct socket *so;
	struct svr4_strm *st;

	if (fp == NULL || fp->f_type != DTYPE_SOCKET)
		return NULL;

	so = (struct socket *) fp->f_data;

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
