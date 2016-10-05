/*	$NetBSD: svr4_stream.c,v 1.81.2.3 2016/10/05 20:55:39 skrll Exp $	 */

/*-
 * Copyright (c) 1994, 2008 The NetBSD Foundation, Inc.
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
 * Pretend that we have streams...
 * Yes, this is gross.
 *
 * ToDo: The state machine for getmsg needs re-thinking
 *
 * The svr4_32 version is built from this file as well.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: svr4_stream.c,v 1.81.2.3 2016/10/05 20:55:39 skrll Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/namei.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/un.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/vfs_syscalls.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/stat.h>

#include <sys/syscallargs.h>

#ifndef SVR4_32
#define	compat(name) <compat/svr4/svr4_##name>
#else
#define	compat(name) <compat/svr4_32/svr4_32_##name>
#endif

#include compat(types.h)
#include compat(util.h)
#include compat(signal.h)
#include compat(lwp.h)
#include compat(ucontext.h)
#include compat(syscallargs.h)
#include compat(stropts.h)
#include compat(timod.h)
#include <compat/svr4/svr4_sockmod.h>
#include compat(ioctl.h)
#include compat(socket.h)

#undef compat

#ifndef SVR4_32

#define SCARG_PTR(uap, arg)	SCARG(uap, arg)
#define NETBSD32PTR(ptr)	(ptr)

#else

#define SCARG_PTR(uap, arg)	SCARG_P32(uap, arg)
#define NETBSD32PTR(ptr)	NETBSD32PTR64(ptr)

/* Rename stuff that is different for the svr4_32 build */
#define	svr4_infocmd		svr4_32_infocmd
#define	svr4_ino_t		svr4_32_ino_t
#define	svr4_netaddr_in		svr4_32_netaddr_in
#define	svr4_netaddr_un		svr4_32_netaddr_un
#define	svr4_strbuf		svr4_32_strbuf
#define	svr4_stream_get		svr4_32_stream_get
#define	svr4_stream_ioctl	svr4_32_stream_ioctl
#define	svr4_stream_ti_ioctl	svr4_32_stream_ti_ioctl
#define	svr4_strfdinsert	svr4_32_strfdinsert
#define	svr4_strioctl		svr4_32_strioctl
#define	svr4_strmcmd		svr4_32_strmcmd
#define	svr4_sys_getmsg		svr4_32_sys_getmsg
#define	svr4_sys_getmsg_args	svr4_32_sys_getmsg_args
#define	svr4_sys_putmsg		svr4_32_sys_putmsg
#define	svr4_sys_putmsg_args	svr4_32_sys_putmsg_args

#endif /* SVR4_32 */

/* Utils */
static int clean_pipe(struct lwp *, const char *);
static void getparm(file_t *, struct svr4_si_sockparms *);

/* Address Conversions */
static void sockaddr_to_netaddr_in(struct svr4_strmcmd *,
					const struct sockaddr_in *);
static void sockaddr_to_netaddr_un(struct svr4_strmcmd *,
					const struct sockaddr_un *);
static void netaddr_to_sockaddr_in(struct sockaddr_in *,
					const struct svr4_strmcmd *);
static void netaddr_to_sockaddr_un(struct sockaddr_un *,
					const struct svr4_strmcmd *);

/* stream ioctls */
static int i_nread(file_t *, struct lwp *, register_t *, int,
    u_long, void *);
static int i_fdinsert(file_t *, struct lwp *, register_t *, int,
    u_long, void *);
static int i_str(file_t *, struct lwp *, register_t *, int,
    u_long, void *);
static int i_setsig(file_t *, struct lwp *, register_t *, int,
    u_long, void *);
static int i_getsig(file_t *, struct lwp *, register_t *, int,
    u_long, void *);
static int _i_bind_rsvd(file_t *, struct lwp *, register_t *, int,
    u_long, void *);
static int _i_rele_rsvd(file_t *, struct lwp *, register_t *, int,
    u_long, void *);

/* i_str sockmod calls */
static int sockmod(file_t *, int, struct svr4_strioctl *,
			      struct lwp *);
static int si_listen(file_t *, int, struct svr4_strioctl *,
			      struct lwp *);
static int si_ogetudata(file_t *, int, struct svr4_strioctl *,
			      struct lwp *);
static int si_sockparams(file_t *, int, struct svr4_strioctl *,
			      struct lwp *);
static int si_shutdown(file_t *, int, struct svr4_strioctl *,
			      struct lwp *);
static int si_getudata(file_t *, int, struct svr4_strioctl *,
			      struct lwp *);

/* i_str timod calls */
static int timod(file_t *, int, struct svr4_strioctl *,
		              struct lwp *);
static int ti_getinfo(file_t *, int, struct svr4_strioctl *,
			      struct lwp *);
static int ti_bind(file_t *, int, struct svr4_strioctl *,
			      struct lwp *);

#ifdef DEBUG_SVR4
static void bufprint(u_char *, size_t);
static int show_ioc(const char *, struct svr4_strioctl *);
static int show_strbuf(struct svr4_strbuf *);
static void show_msg(const char *, int, struct svr4_strbuf *,
			  struct svr4_strbuf *, int);

static void
bufprint(u_char *buf, size_t len)
{
	size_t i;

	uprintf("\n\t");
	for (i = 0; i < len; i++) {
		uprintf("%x ", buf[i]);
		if (i && (i % 16) == 0)
			uprintf("\n\t");
	}
}

static int
show_ioc(const char *str, struct svr4_strioctl *ioc)
{
	u_char *ptr;
	int error, len;

	len = ioc->len;
	if (len > 1024)
		len = 1024;

	ptr = (u_char *) malloc(len, M_TEMP, M_WAITOK);
	uprintf("%s cmd = %ld, timeout = %d, len = %d, buf = %p { ",
	    str, ioc->cmd, ioc->timeout, ioc->len, ioc->buf);

	if ((error = copyin(ioc->buf, ptr, len)) != 0) {
		free((char *) ptr, M_TEMP);
		return error;
	}

	bufprint(ptr, len);

	uprintf("}\n");

	free((char *) ptr, M_TEMP);
	return 0;
}


static int
show_strbuf(struct svr4_strbuf *str)
{
	int error;
	u_char *ptr = NULL;
	int maxlen = str->maxlen;
	int len = str->len;

	if (maxlen > 8192)
		maxlen = 8192;

	if (maxlen < 0)
		maxlen = 0;

	if (len >= maxlen)
		len = maxlen;

	if (len > 0) {
	    ptr = (u_char *) malloc(len, M_TEMP, M_WAITOK);

	    if ((error = copyin(str->buf, ptr, len)) != 0) {
		    free((char *) ptr, M_TEMP);
		    return error;
	    }
	}

	uprintf(", { %d, %d, %p=[ ", str->maxlen, str->len, str->buf);

	if (ptr)
		bufprint(ptr, len);

	uprintf("]}");

	if (ptr)
		free((char *) ptr, M_TEMP);

	return 0;
}


static void
show_msg(const char *str, int fd, struct svr4_strbuf *ctl, struct svr4_strbuf *dat, int flags)
{
	struct svr4_strbuf	buf;
	int error;

	uprintf("%s(%d", str, fd);
	if (ctl != NULL) {
		if ((error = copyin(ctl, &buf, sizeof(buf))) != 0)
			return;
		show_strbuf(&buf);
	}
	else
		uprintf(", NULL");

	if (dat != NULL) {
		if ((error = copyin(dat, &buf, sizeof(buf))) != 0)
			return;
		show_strbuf(&buf);
	}
	else
		uprintf(", NULL");

	uprintf(", %x);\n", flags);
}

#endif /* DEBUG_SVR4 */


/*
 * We are faced with an interesting situation. On svr4 unix sockets
 * are really pipes. But we really have sockets, and we might as
 * well use them. At the point where svr4 calls TI_BIND, it has
 * already created a named pipe for the socket using mknod(2).
 * We need to create a socket with the same name when we bind,
 * so we need to remove the pipe before, otherwise we'll get address
 * already in use. So we *carefully* remove the pipe, to avoid
 * using this as a random file removal tool.
 */
static int
clean_pipe(struct lwp *l, const char *path)
{
	struct pathbuf *pb;
	struct nameidata nd;
	struct vattr va;
	int error;

	pb = pathbuf_create(path);
	if (pb == NULL) {
		return ENOMEM;
	}

	NDINIT(&nd, DELETE, NOFOLLOW | LOCKPARENT | LOCKLEAF | TRYEMULROOT, pb);
	error = namei(&nd);
	if (error != 0) {
		pathbuf_destroy(pb);
		return error;
	}

	/*
	 * Make sure we are dealing with a mode 0 named pipe.
	 */
	if (nd.ni_vp->v_type != VFIFO)
		goto bad;
        error = VOP_GETATTR(nd.ni_vp, &va, l->l_cred);
	if (error != 0)
		goto bad;
	if ((va.va_mode & ALLPERMS) != 0)
		goto bad;

	error = VOP_REMOVE(nd.ni_dvp, nd.ni_vp, &nd.ni_cnd);
	pathbuf_destroy(pb);
	return error;

    bad:
	if (nd.ni_dvp == nd.ni_vp)
		vrele(nd.ni_dvp);
	else
		vput(nd.ni_dvp);
	vput(nd.ni_vp);
	pathbuf_destroy(pb);
	return error;
}


static void
sockaddr_to_netaddr_in(struct svr4_strmcmd *sc, const struct sockaddr_in *sain)
{
	struct svr4_netaddr_in *na;
	na = SVR4_ADDROF(sc);

	na->family = sain->sin_family;
	na->port = sain->sin_port;
	na->addr = sain->sin_addr.s_addr;
	DPRINTF(("sockaddr_in -> netaddr %d %d %lx\n", na->family, na->port,
		 na->addr));
}


static void
sockaddr_to_netaddr_un(struct svr4_strmcmd *sc, const struct sockaddr_un *saun)
{
	struct svr4_netaddr_un *na;
	char *dst, *edst = ((char *) sc) + sc->offs + sizeof(na->family) + 1  -
	    sizeof(*sc);
	const char *src;

	na = SVR4_ADDROF(sc);
	na->family = saun->sun_family;
	for (src = saun->sun_path, dst = na->path; (*dst++ = *src++) != '\0'; )
		if (dst == edst)
			break;
	DPRINTF(("sockaddr_un -> netaddr %d %s\n", na->family, na->path));
}


static void
netaddr_to_sockaddr_in(struct sockaddr_in *sain, const struct svr4_strmcmd *sc)
{
	const struct svr4_netaddr_in *na;


	na = SVR4_C_ADDROF(sc);
	memset(sain, 0, sizeof(*sain));
	sain->sin_len = sizeof(*sain);
	sain->sin_family = na->family;
	sain->sin_port = na->port;
	sain->sin_addr.s_addr = na->addr;
	DPRINTF(("netaddr -> sockaddr_in %d %d %x\n", sain->sin_family,
		 sain->sin_port, sain->sin_addr.s_addr));
}


static void
netaddr_to_sockaddr_un(struct sockaddr_un *saun, const struct svr4_strmcmd *sc)
{
	const struct svr4_netaddr_un *na;
	char *dst, *edst = &saun->sun_path[sizeof(saun->sun_path) - 1];
	const char *src;

	na = SVR4_C_ADDROF(sc);
	memset(saun, 0, sizeof(*saun));
	saun->sun_family = na->family;
	for (src = na->path, dst = saun->sun_path; (*dst++ = *src++) != '\0'; )
		if (dst == edst)
			break;
	saun->sun_len = dst - saun->sun_path;
	DPRINTF(("netaddr -> sockaddr_un %d %s\n", saun->sun_family,
		 saun->sun_path));
}


static void
getparm(file_t *fp, struct svr4_si_sockparms *pa)
{
	struct svr4_strm *st = svr4_stream_get(fp);
	struct socket *so = fp->f_socket;

	if (st == NULL)
		return;

	pa->family = st->s_family;

	switch (so->so_type) {
	case SOCK_DGRAM:
		pa->type = SVR4_T_CLTS;
		pa->protocol = IPPROTO_UDP;
		DPRINTF(("getparm(dgram)\n"));
		return;

	case SOCK_STREAM:
	        pa->type = SVR4_T_COTS;  /* What about T_COTS_ORD? XXX */
		pa->protocol = IPPROTO_IP;
		DPRINTF(("getparm(stream)\n"));
		return;

	case SOCK_RAW:
		pa->type = SVR4_T_CLTS;
		pa->protocol = IPPROTO_RAW;
		DPRINTF(("getparm(raw)\n"));
		return;

	default:
		pa->type = 0;
		pa->protocol = 0;
		DPRINTF(("getparm(type %d?)\n", so->so_type));
		return;
	}
}


static int
si_ogetudata(file_t *fp, int fd, struct svr4_strioctl *ioc,
    struct lwp *l)
{
	int error;
	struct svr4_si_oudata ud;
	struct svr4_si_sockparms pa;
	(void)memset(&pa, 0, sizeof(pa));	/* XXX: GCC */

	if (ioc->len != sizeof(ud) && ioc->len != sizeof(ud) - sizeof(int)) {
		DPRINTF(("SI_OGETUDATA: Wrong size %ld != %d\n",
		    (unsigned long)sizeof(ud), ioc->len));
		return EINVAL;
	}

	if ((error = copyin(NETBSD32PTR(ioc->buf), &ud, sizeof(ud))) != 0)
		return error;

	getparm(fp, &pa);

	switch (pa.family) {
	case AF_INET:
	    ud.tidusize = 16384;
	    ud.addrsize = sizeof(struct sockaddr_in);
	    if (pa.type == SVR4_SOCK_STREAM)
		    ud.etsdusize = 1;
	    else
		    ud.etsdusize = 0;
	    break;

	case AF_LOCAL:
	    ud.tidusize = 65536;
	    ud.addrsize = 128;
	    ud.etsdusize = 128;
	    break;

	default:
	    DPRINTF(("SI_OGETUDATA: Unsupported address family %d\n",
		     pa.family));
	    return ENOSYS;
	}

	/* I have no idea what these should be! */
	ud.optsize = 128;
	ud.tsdusize = 128;

	ud.servtype = pa.type;

	/* XXX: Fixme */
	ud.so_state = 0;
	ud.so_options = 0;
	return copyout(&ud, NETBSD32PTR(ioc->buf), ioc->len);
}


static int
si_sockparams(file_t *fp, int fd, struct svr4_strioctl *ioc,
    struct lwp *l)
{
	struct svr4_si_sockparms pa;

	getparm(fp, &pa);
	return copyout(&pa, NETBSD32PTR(ioc->buf), sizeof(pa));
}


static int
si_listen(file_t *fp, int fd, struct svr4_strioctl *ioc, struct lwp *l)
{
	int error;
	struct svr4_strm *st = svr4_stream_get(fp);
	register_t retval;
	struct svr4_strmcmd lst;
	struct sys_listen_args la;

	if (st == NULL)
		return EINVAL;

	if (ioc->len > sizeof(lst))
		return EINVAL;

	if ((error = copyin(NETBSD32PTR(ioc->buf), &lst, ioc->len)) != 0)
		return error;

	if (lst.cmd != SVR4_TI_OLD_BIND_REQUEST) {
		DPRINTF(("si_listen: bad request %ld\n", lst.cmd));
		return EINVAL;
	}

	/*
	 * We are making assumptions again...
	 */
	SCARG(&la, s) = fd;
	DPRINTF(("SI_LISTEN: fileno %d backlog = %d\n", fd, 5));
	SCARG(&la, backlog) = 5;

	if ((error = sys_listen(l, &la, &retval)) != 0) {
		DPRINTF(("SI_LISTEN: listen failed %d\n", error));
		return error;
	}

	st->s_cmd = SVR4_TI__ACCEPT_WAIT;
	lst.cmd = SVR4_TI_BIND_REPLY;

	switch (st->s_family) {
	case AF_INET:
		/* XXX: Fill the length here */
		break;

	case AF_LOCAL:
		lst.len = 140;
		lst.pad[28] = 0x00000000;	/* magic again */
		lst.pad[29] = 0x00000800;	/* magic again */
		lst.pad[30] = 0x80001400;	/* magic again */
		break;

	default:
		DPRINTF(("SI_LISTEN: Unsupported address family %d\n",
		    st->s_family));
		return ENOSYS;
	}


	if ((error = copyout(&lst, NETBSD32PTR(ioc->buf), ioc->len)) != 0)
		return error;

	return 0;
}


static int
si_getudata(file_t *fp, int fd, struct svr4_strioctl *ioc,
    struct lwp *l)
{
	int error;
	struct svr4_si_udata ud;

	if (sizeof(ud) != ioc->len) {
		DPRINTF(("SI_GETUDATA: Wrong size %ld != %d\n",
		    (unsigned long)sizeof(ud), ioc->len));
		return EINVAL;
	}

	if ((error = copyin(NETBSD32PTR(ioc->buf), &ud, sizeof(ud))) != 0)
		return error;

	getparm(fp, &ud.sockparms);

	switch (ud.sockparms.family) {
	case AF_INET:
	    ud.tidusize = 16384;
	    ud.tsdusize = 16384;
	    ud.addrsize = sizeof(struct sockaddr_in);
	    if (ud.sockparms.type == SVR4_SOCK_STREAM)
		    ud.etsdusize = 1;
	    else
		    ud.etsdusize = 0;
	    ud.optsize = 0;
	    break;

	case AF_LOCAL:
	    ud.tidusize = 65536;
	    ud.tsdusize = 128;
	    ud.addrsize = 128;
	    ud.etsdusize = 128;
	    ud.optsize = 128;
	    break;

	default:
	    DPRINTF(("SI_GETUDATA: Unsupported address family %d\n",
		     ud.sockparms.family));
	    return ENOSYS;
	}


	ud.servtype = ud.sockparms.type;

	/* XXX: Fixme */
	ud.so_state = 0;
	ud.so_options = 0;
	return copyout(&ud, NETBSD32PTR(ioc->buf), sizeof(ud));
}


static int
si_shutdown(file_t *fp, int fd, struct svr4_strioctl *ioc,
    struct lwp *l)
{
	int error;
	struct sys_shutdown_args ap;
	register_t retval;

	if (ioc->len != sizeof(SCARG(&ap, how))) {
		DPRINTF(("SI_SHUTDOWN: Wrong size %ld != %d\n",
		    (unsigned long)sizeof(SCARG(&ap, how)), ioc->len));
		return EINVAL;
	}

	if ((error = copyin(NETBSD32PTR(ioc->buf), &SCARG(&ap, how), ioc->len)) != 0)
		return error;

	SCARG(&ap, s) = fd;

	return sys_shutdown(l, &ap, &retval);
}


static int
sockmod(file_t *fp, int fd, struct svr4_strioctl *ioc, struct lwp *l)
{
	switch (ioc->cmd) {
	case SVR4_SI_OGETUDATA:
		DPRINTF(("SI_OGETUDATA\n"));
		return si_ogetudata(fp, fd, ioc, l);

	case SVR4_SI_SHUTDOWN:
		DPRINTF(("SI_SHUTDOWN\n"));
		return si_shutdown(fp, fd, ioc, l);

	case SVR4_SI_LISTEN:
		DPRINTF(("SI_LISTEN\n"));
		return si_listen(fp, fd, ioc, l);

	case SVR4_SI_SETMYNAME:
		DPRINTF(("SI_SETMYNAME\n"));
		return 0;

	case SVR4_SI_SETPEERNAME:
		DPRINTF(("SI_SETPEERNAME\n"));
		return 0;

	case SVR4_SI_GETINTRANSIT:
		DPRINTF(("SI_GETINTRANSIT\n"));
		return 0;

	case SVR4_SI_TCL_LINK:
		DPRINTF(("SI_TCL_LINK\n"));
		return 0;

	case SVR4_SI_TCL_UNLINK:
		DPRINTF(("SI_TCL_UNLINK\n"));
		return 0;

	case SVR4_SI_SOCKPARAMS:
		DPRINTF(("SI_SOCKPARAMS\n"));
		return si_sockparams(fp, fd, ioc, l);

	case SVR4_SI_GETUDATA:
		DPRINTF(("SI_GETUDATA\n"));
		return si_getudata(fp, fd, ioc, l);

	default:
		DPRINTF(("Unknown sockmod ioctl %lx\n", ioc->cmd));
		return 0;

	}
}


static int
ti_getinfo(file_t *fp, int fd, struct svr4_strioctl *ioc,
    struct lwp *l)
{
	int error;
	struct svr4_infocmd info;

	memset(&info, 0, sizeof(info));

	if (ioc->len > sizeof(info))
		return EINVAL;

	if ((error = copyin(NETBSD32PTR(ioc->buf), &info, ioc->len)) != 0)
		return error;

	if (info.cmd != SVR4_TI_INFO_REQUEST)
		return EINVAL;

	info.cmd = SVR4_TI_INFO_REPLY;
	info.tsdu = 0;
	info.etsdu = 1;
	info.cdata = -2;
	info.ddata = -2;
	info.addr = 16;
	info.opt = -1;
	info.tidu = 16384;
	info.serv = 2;
	info.current = 0;
	info.provider = 2;

	ioc->len = sizeof(info);
	if ((error = copyout(&info, NETBSD32PTR(ioc->buf), ioc->len)) != 0)
		return error;

	return 0;
}


static int
ti_bind(file_t *fp, int fd, struct svr4_strioctl *ioc, struct lwp *l)
{
	int error;
	struct svr4_strm *st = svr4_stream_get(fp);
	struct sockaddr_in *sain;
	struct sockaddr_un *saun;
	struct sockaddr_big sbig;
	void *sup = NULL;
	int sasize;
	struct svr4_strmcmd bnd;

	if (st == NULL) {
		DPRINTF(("ti_bind: bad file descriptor\n"));
		return EINVAL;
	}

	if (ioc->len > sizeof(bnd))
		return EINVAL;

	if ((error = copyin(NETBSD32PTR(ioc->buf), &bnd, ioc->len)) != 0)
		return error;

	if (bnd.cmd != SVR4_TI_OLD_BIND_REQUEST) {
		DPRINTF(("ti_bind: bad request %ld\n", bnd.cmd));
		return EINVAL;
	}

	switch (st->s_family) {
	case AF_INET:
		sain = (struct sockaddr_in *)&sbig;
		sasize = sizeof(*sain);

		if (bnd.offs == 0)
			goto reply;

		netaddr_to_sockaddr_in(sain, &bnd);

		DPRINTF(("TI_BIND: fam %d, port %d, addr %x\n",
			 sain->sin_family, sain->sin_port,
			 sain->sin_addr.s_addr));
		break;

	case AF_LOCAL:
		saun = (struct sockaddr_un *)&sbig;
		sasize = sizeof(*saun);
		if (bnd.offs == 0)
			goto reply;

		netaddr_to_sockaddr_un(saun, &bnd);

		if (saun->sun_path[0] == '\0')
			goto reply;

		DPRINTF(("TI_BIND: fam %d, path %s\n",
			 saun->sun_family, saun->sun_path));

		if ((error = clean_pipe(l, saun->sun_path)) != 0)
			return error;

		bnd.pad[28] = 0x00001000;	/* magic again */
		break;

	default:
		DPRINTF(("TI_BIND: Unsupported address family %d\n",
			 st->s_family));
		return ENOSYS;
	}

	DPRINTF(("TI_BIND: fileno %d\n", fd));

	error = do_sys_bind(l, fd, (struct sockaddr *)&sbig);
	if (error != 0) {
		DPRINTF(("TI_BIND: bind failed %d\n", error));
		return error;
	}

reply:
	if (sup == NULL) {
		memset(&bnd, 0, sizeof(bnd));
		bnd.len = sasize + 4;
		bnd.offs = 0x10;	/* XXX */
	}

	bnd.cmd = SVR4_TI_BIND_REPLY;

	if ((error = copyout(&bnd, NETBSD32PTR(ioc->buf), ioc->len)) != 0)
		return error;

	return 0;
}


static int
timod(file_t *fp, int fd, struct svr4_strioctl *ioc, struct lwp *l)
{
	switch (ioc->cmd) {
	case SVR4_TI_GETINFO:
		DPRINTF(("TI_GETINFO\n"));
		return ti_getinfo(fp, fd, ioc, l);

	case SVR4_TI_OPTMGMT:
		DPRINTF(("TI_OPTMGMT\n"));
		return 0;

	case SVR4_TI_BIND:
		DPRINTF(("TI_BIND\n"));
		return ti_bind(fp, fd, ioc, l);

	case SVR4_TI_UNBIND:
		DPRINTF(("TI_UNBIND\n"));
		return 0;

	default:
		DPRINTF(("Unknown timod ioctl %lx\n", ioc->cmd));
		return 0;
	}
}


int
svr4_stream_ti_ioctl(file_t *fp, struct lwp *l, register_t *retval, int fd, u_long cmd, void *dat)
{
	struct svr4_strbuf skb, *sub = (struct svr4_strbuf *) dat;
	struct svr4_strm *st = svr4_stream_get(fp);
	int error;
	struct svr4_strmcmd sc;
	struct sockaddr_big sbig;

	if (st == NULL)
		return EINVAL;

	sbig.sb_len = UCHAR_MAX;
	sc.offs = 0x10;

	if ((error = copyin(sub, &skb, sizeof(skb))) != 0) {
		DPRINTF(("ti_ioctl: error copying in strbuf\n"));
		return error;
	}

	switch (cmd) {
	case SVR4_TI_GETMYNAME:
		DPRINTF(("TI_GETMYNAME\n"));
		error = do_sys_getsockname(fd, (struct sockaddr *)&sbig);
		if (error != 0)
			return error;
		break;

	case SVR4_TI_GETPEERNAME:
		DPRINTF(("TI_GETPEERNAME\n"));
		error = do_sys_getpeername(fd, (struct sockaddr *)&sbig);
		if (error != 0)
			return error;
		break;

	case SVR4_TI_SETMYNAME:
		DPRINTF(("TI_SETMYNAME\n"));
		return 0;

	case SVR4_TI_SETPEERNAME:
		DPRINTF(("TI_SETPEERNAME\n"));
		return 0;
	default:
		DPRINTF(("ti_ioctl: Unknown ioctl %lx\n", cmd));
		return ENOSYS;
	}

	switch (st->s_family) {
	case AF_INET:
		sockaddr_to_netaddr_in(&sc, (struct sockaddr_in *)&sbig);
		skb.len = sizeof (struct sockaddr_in);
		break;

	case AF_LOCAL:
		sockaddr_to_netaddr_un(&sc, (struct sockaddr_un *)&sbig);
		/* XXX: the length gets adjusted but the copyout doesn't */
		skb.len = sizeof (struct sockaddr_un) + 4;
		break;

	default:
		DPRINTF(("ti_ioctl: Unsupported address family %d\n",
			 st->s_family));
		return ENOSYS;
	}

	error = copyout(SVR4_ADDROF(&sc), NETBSD32PTR(skb.buf), sbig.sb_len);
	if (error != 0) {
		DPRINTF(("ti_ioctl: error copying out socket data\n"));
		return error;
	}


	if ((error = copyout(&skb, sub, sizeof(skb))) != 0) {
		DPRINTF(("ti_ioctl: error copying out strbuf\n"));
		return error;
	}

	return error;
}




static int
i_nread(file_t *fp, struct lwp *l, register_t *retval, int fd,
    u_long cmd, void *dat)
{
	int error;
	int nread = 0;

	/*
	 * We are supposed to return the message length in nread, and the
	 * number of messages in retval. We don't have the notion of number
	 * of stream messages, so we just find out if we have any bytes waiting
	 * for us, and if we do, then we assume that we have at least one
	 * message waiting for us.
	 */
	if ((error = (*fp->f_ops->fo_ioctl)(fp, FIONREAD, &nread)) != 0)
		return error;

	if (nread != 0)
		*retval = 1;
	else
		*retval = 0;

	return copyout(&nread, dat, sizeof(nread));
}

static int
i_fdinsert(file_t *fp, struct lwp *l, register_t *retval, int fd,
    u_long cmd, void *dat)
{
	/*
	 * Major hack again here. We assume that we are using this to
	 * implement accept(2). If that is the case, we have already
	 * called accept, and we have stored the file descriptor in
	 * afd. We find the file descriptor that the code wants to use
	 * in fd insert, and then we dup2() our accepted file descriptor
	 * to it.
	 */
	int error;
	struct svr4_strm *st = svr4_stream_get(fp);
	struct svr4_strfdinsert fdi;
	struct sys_dup2_args d2p;
	struct sys_close_args clp;

	if (st == NULL) {
		DPRINTF(("fdinsert: bad file type\n"));
		return EINVAL;
	}

	if (st->s_afd == -1) {
		DPRINTF(("fdinsert: accept fd not found\n"));
		return ENOENT;
	}

	if ((error = copyin(dat, &fdi, sizeof(fdi))) != 0) {
		DPRINTF(("fdinsert: copyin failed %d\n", error));
		return error;
	}

	SCARG(&d2p, from) = st->s_afd;
	SCARG(&d2p, to) = fdi.fd;

	if ((error = sys_dup2(l, &d2p, retval)) != 0) {
		DPRINTF(("fdinsert: dup2(%d, %d) failed %d\n",
		    st->s_afd, fdi.fd, error));
		return error;
	}

	SCARG(&clp, fd) = st->s_afd;

	if ((error = sys_close(l, &clp, retval)) != 0) {
		DPRINTF(("fdinsert: close(%d) failed %d\n",
		    st->s_afd, error));
		return error;
	}

	st->s_afd = -1;

	*retval = 0;
	return 0;
}


static int
_i_bind_rsvd(file_t *fp, struct lwp *l, register_t *retval,
    int fd, u_long cmd, void *dat)
{
	struct sys_mkfifo_args ap;

	/*
	 * This is a supposed to be a kernel and library only ioctl.
	 * It gets called before ti_bind, when we have a unix
	 * socket, to physically create the socket transport and
	 * ``reserve'' it. I don't know how this get reserved inside
	 * the kernel, but we are going to create it nevertheless.
	 */
	SCARG(&ap, path) = dat;
	SCARG(&ap, mode) = S_IFIFO;

	return sys_mkfifo(l, &ap, retval);
}

static int
_i_rele_rsvd(file_t *fp, struct lwp *l, register_t *retval,
    int fd, u_long cmd, void *dat)
{
	struct sys_unlink_args ap;

	/*
	 * This is a supposed to be a kernel and library only ioctl.
	 * I guess it is supposed to release the socket.
	 */
	SCARG(&ap, path) = dat;

	return sys_unlink(l, &ap, retval);
}

static int
i_str(file_t *fp, struct lwp *l, register_t *retval, int fd,
    u_long cmd, void *dat)
{
	int			 error;
	struct svr4_strioctl	 ioc;

	/*
	 * Noop on non sockets
	 */
	if (fp->f_type != DTYPE_SOCKET)
		return 0;

	if ((error = copyin(dat, &ioc, sizeof(ioc))) != 0)
		return error;

#ifdef DEBUG_SVR4
	if ((error = show_ioc(">", &ioc)) != 0)
		return error;
#endif /* DEBUG_SVR4 */

	switch (ioc.cmd & 0xff00) {
	case SVR4_SIMOD:
		if ((error = sockmod(fp, fd, &ioc, l)) != 0)
			return error;
		break;

	case SVR4_TIMOD:
		if ((error = timod(fp, fd, &ioc, l)) != 0)
			return error;
		break;

	default:
		DPRINTF(("Unimplemented module %c %ld\n",
			 (char) (cmd >> 8), cmd & 0xff));
		return 0;
	}

#ifdef DEBUG_SVR4
	if ((error = show_ioc("<", &ioc)) != 0)
		return error;
#endif /* DEBUG_SVR4 */
	return copyout(&ioc, dat, sizeof(ioc));
}

static int
i_setsig(file_t *fp, struct lwp *l, register_t *retval, int fd,
    u_long cmd, void *dat)
{
	/*
	 * This is the best we can do for now; we cannot generate
	 * signals only for specific events so the signal mask gets
	 * ignored; we save it just to pass it to a possible I_GETSIG...
	 *
	 * We alse have to fix the O_ASYNC fcntl bit, so the
	 * process will get SIGPOLLs.
	 */
	struct sys_fcntl_args fa;
	int error;
	register_t oflags, flags;
	struct svr4_strm *st = svr4_stream_get(fp);

	if (st == NULL) {
		DPRINTF(("i_setsig: bad file descriptor\n"));
		return EINVAL;
	}
	/* get old status flags */
	SCARG(&fa, fd) = fd;
	SCARG(&fa, cmd) = F_GETFL;

	if ((error = sys_fcntl(l, &fa, &oflags)) != 0)
		return error;

	/* update the flags */
	if (dat != NULL) {
		int mask;

		flags = oflags | O_ASYNC;
		mask = (int)(u_long)dat;
		if (mask & ~SVR4_S_ALLMASK) {
			DPRINTF(("i_setsig: bad eventmask data %x\n", mask));
			return EINVAL;
		}
		st->s_eventmask = mask;
	}
	else {
		flags = oflags & ~O_ASYNC;
		st->s_eventmask = 0;
	}

	/* set the new flags, if changed */
	if (flags != oflags) {
		SCARG(&fa, cmd) = F_SETFL;
		SCARG(&fa, arg) = (void *) flags;
		if ((error = sys_fcntl(l, &fa, &flags)) != 0)
			return error;
	}

	/* set up SIGIO receiver if needed */
	if (dat != NULL) {
		SCARG(&fa, cmd) = F_SETOWN;
		SCARG(&fa, arg) = (void *)(u_long)l->l_proc->p_pid;
		return sys_fcntl(l, &fa, &flags);
	}
	return 0;
}

static int
i_getsig(file_t *fp, struct lwp *l, register_t *retval,
    int fd, u_long cmd, void *dat)
{
	int error;

	if (dat != NULL) {
		struct svr4_strm *st = svr4_stream_get(fp);

		if (st == NULL) {
			DPRINTF(("i_getsig: bad file descriptor\n"));
			return EINVAL;
		}
		if ((error = copyout(&st->s_eventmask, dat,
		    sizeof(st->s_eventmask))) != 0) {
			DPRINTF(("i_getsig: bad eventmask pointer\n"));
			return error;
		}
	}
	return 0;
}

int
svr4_stream_ioctl(file_t *fp, struct lwp *l, register_t *retval, int fd, u_long cmd, void *dat)
{
	*retval = 0;

	/*
	 * All the following stuff assumes "sockmod" is pushed...
	 */
	switch (cmd) {
	case SVR4_I_NREAD:
		DPRINTF(("I_NREAD\n"));
		return i_nread(fp, l, retval, fd, cmd, dat);

	case SVR4_I_PUSH:
		DPRINTF(("I_PUSH\n"));
		return 0;

	case SVR4_I_POP:
		DPRINTF(("I_POP\n"));
		return 0;

	case SVR4_I_LOOK:
		DPRINTF(("I_LOOK\n"));
		return 0;

	case SVR4_I_FLUSH:
		DPRINTF(("I_FLUSH\n"));
		return 0;

	case SVR4_I_SRDOPT:
		DPRINTF(("I_SRDOPT\n"));
		return 0;

	case SVR4_I_GRDOPT:
		DPRINTF(("I_GRDOPT\n"));
		return 0;

	case SVR4_I_STR:
		DPRINTF(("I_STR\n"));
		return i_str(fp, l, retval, fd, cmd, dat);

	case SVR4_I_SETSIG:
		DPRINTF(("I_SETSIG\n"));
		return i_setsig(fp, l, retval, fd, cmd, dat);

	case SVR4_I_GETSIG:
		DPRINTF(("I_GETSIG\n"));
		return i_getsig(fp, l, retval, fd, cmd, dat);

	case SVR4_I_FIND:
		DPRINTF(("I_FIND\n"));
		/*
		 * Here we are not pushing modules really, we just
		 * pretend all are present
		 */
		*retval = 1;
		return 0;

	case SVR4_I_LINK:
		DPRINTF(("I_LINK\n"));
		return 0;

	case SVR4_I_UNLINK:
		DPRINTF(("I_UNLINK\n"));
		return 0;

	case SVR4_I_ERECVFD:
		DPRINTF(("I_ERECVFD\n"));
		return 0;

	case SVR4_I_PEEK:
		DPRINTF(("I_PEEK\n"));
		return 0;

	case SVR4_I_FDINSERT:
		DPRINTF(("I_FDINSERT\n"));
		return i_fdinsert(fp, l, retval, fd, cmd, dat);

	case SVR4_I_SENDFD:
		DPRINTF(("I_SENDFD\n"));
		return 0;

	case SVR4_I_RECVFD:
		DPRINTF(("I_RECVFD\n"));
		return 0;

	case SVR4_I_SWROPT:
		DPRINTF(("I_SWROPT\n"));
		return 0;

	case SVR4_I_GWROPT:
		DPRINTF(("I_GWROPT\n"));
		return 0;

	case SVR4_I_LIST:
		DPRINTF(("I_LIST\n"));
		return 0;

	case SVR4_I_PLINK:
		DPRINTF(("I_PLINK\n"));
		return 0;

	case SVR4_I_PUNLINK:
		DPRINTF(("I_PUNLINK\n"));
		return 0;

	case SVR4_I_SETEV:
		DPRINTF(("I_SETEV\n"));
		return 0;

	case SVR4_I_GETEV:
		DPRINTF(("I_GETEV\n"));
		return 0;

	case SVR4_I_STREV:
		DPRINTF(("I_STREV\n"));
		return 0;

	case SVR4_I_UNSTREV:
		DPRINTF(("I_UNSTREV\n"));
		return 0;

	case SVR4_I_FLUSHBAND:
		DPRINTF(("I_FLUSHBAND\n"));
		return 0;

	case SVR4_I_CKBAND:
		DPRINTF(("I_CKBAND\n"));
		return 0;

	case SVR4_I_GETBAND:
		DPRINTF(("I_GETBANK\n"));
		return 0;

	case SVR4_I_ATMARK:
		DPRINTF(("I_ATMARK\n"));
		return 0;

	case SVR4_I_SETCLTIME:
		DPRINTF(("I_SETCLTIME\n"));
		return 0;

	case SVR4_I_GETCLTIME:
		DPRINTF(("I_GETCLTIME\n"));
		return 0;

	case SVR4_I_CANPUT:
		DPRINTF(("I_CANPUT\n"));
		return 0;

	case SVR4__I_BIND_RSVD:
		DPRINTF(("_I_BIND_RSVD\n"));
		return _i_bind_rsvd(fp, l, retval, fd, cmd, dat);

	case SVR4__I_RELE_RSVD:
		DPRINTF(("_I_RELE_RSVD\n"));
		return _i_rele_rsvd(fp, l, retval, fd, cmd, dat);

	default:
		DPRINTF(("unimpl cmd = %lx\n", cmd));
		break;
	}

	return 0;
}




int
svr4_sys_putmsg(struct lwp *l, const struct svr4_sys_putmsg_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct sockaddr *skp;
	file_t	*fp;
	struct svr4_strbuf dat, ctl;
	struct svr4_strmcmd sc;
	struct sockaddr_in sain;
	struct sockaddr_un saun;
	struct svr4_strm *st;
	int error;
	struct msghdr msg;
	struct iovec aiov;


#ifdef DEBUG_SVR4
	show_msg(">putmsg", SCARG(uap, fd), SCARG(uap, ctl),
		 SCARG(uap, dat), SCARG(uap, flags));
#endif /* DEBUG_SVR4 */

	if ((fp = fd_getfile(SCARG(uap, fd))) == NULL)
		return EBADF;

	KERNEL_LOCK(1, NULL);	/* svr4_find_socket */

	if (SCARG_PTR(uap, ctl) != NULL) {
		if ((error = copyin(SCARG_PTR(uap, ctl),
				    &ctl, sizeof(ctl))) != 0)
			goto out;
	} else
		ctl.len = -1;

	if (SCARG_PTR(uap, dat) != NULL) {
	    	if ((error = copyin(SCARG_PTR(uap, dat),
				    &dat, sizeof(dat))) != 0)
			goto out;
	} else
		dat.len = -1;

	/*
	 * Only for sockets for now.
	 */
	if ((st = svr4_stream_get(fp)) == NULL) {
		DPRINTF(("putmsg: bad file type\n"));
		error = EINVAL;
		goto out;
	}

	if (ctl.len > sizeof(sc)) {
		DPRINTF(("putmsg: Bad control size %ld != %d\n",
		    (unsigned long)sizeof(struct svr4_strmcmd), ctl.len));
		error = EINVAL;
		goto out;
	}

	if ((error = copyin(NETBSD32PTR(ctl.buf), &sc, ctl.len)) != 0)
		goto out;

	switch (st->s_family) {
	case AF_INET:
		if (sc.len != sizeof(sain)) {
#ifdef notyet
		        if (sc.cmd == SVR4_TI_DATA_REQUEST) {
			        struct sys_write_args wa;

				/* Solaris seems to use sc.cmd = 3 to
				 * send "expedited" data.  telnet uses
				 * this for options processing, sending EOF,
				 * etc.  I'm sure other things use it too.
				 * I don't have any documentation
				 * on it, so I'm making a guess that this
				 * is how it works. newton@atdot.dotat.org XXX
				 *
				 * Hmm, expedited data seems to be sc.cmd = 4.
				 * I think 3 is normal data. (christos)
				 */
				DPRINTF(("sending expedited data (?)\n"));
				SCARG(&wa, fd) = SCARG(uap, fd);
				SCARG(&wa, buf) = dat.buf;
				SCARG(&wa, nbyte) = dat.len;
				error = sys_write(l, &wa, retval);
				goto out;
			}
#endif
	                DPRINTF(("putmsg: Invalid inet length %ld\n", sc.len));
			error = EINVAL;
			goto out;
		}
		netaddr_to_sockaddr_in(&sain, &sc);
		skp = (struct sockaddr *)&sain;
		error = sain.sin_family != st->s_family;
		break;

	case AF_LOCAL:
		if (ctl.len == 8) {
			/* We are doing an accept; succeed */
			DPRINTF(("putmsg: Do nothing\n"));
			*retval = 0;
			error = 0;
			goto out;
		}
		else {
			/* Maybe we've been given a device/inode pair */
			dev_t *dev = SVR4_ADDROF(&sc);
			svr4_ino_t *ino = (svr4_ino_t *) &dev[1];
			skp = (struct sockaddr *)svr4_find_socket(
			    p, fp, *dev, *ino);
			if (skp == NULL) {
				skp = (struct sockaddr *)&saun;
				/* I guess we have it by name */
				netaddr_to_sockaddr_un(
				    (struct sockaddr_un *)skp, &sc);
			}
		}
		break;

	default:
		DPRINTF(("putmsg: Unsupported address family %d\n",
			 st->s_family));
		error = ENOSYS;
		goto out;
	}

 	switch (st->s_cmd = sc.cmd) {
	case SVR4_TI_CONNECT_REQUEST:	/* connect 	*/
	 	KERNEL_UNLOCK_ONE(NULL);
		return do_sys_connect(l, SCARG(uap, fd), skp);

	case SVR4_TI_SENDTO_REQUEST:	/* sendto 	*/
	 	KERNEL_UNLOCK_ONE(NULL);
		msg.msg_name = skp;
		msg.msg_namelen = skp->sa_len;
		msg.msg_iov = &aiov;
		msg.msg_iovlen = 1;
		msg.msg_control = NULL;
		msg.msg_flags = 0;
		aiov.iov_base = NETBSD32PTR(dat.buf);
		aiov.iov_len = dat.len;
		error = do_sys_sendmsg(l, SCARG(uap, fd), &msg,
			       SCARG(uap, flags),  NULL, 0, retval);

		*retval = 0;
		return error;
	default:
		DPRINTF(("putmsg: Unimplemented command %lx\n", sc.cmd));
		error = ENOSYS;
		goto out;
	}

 out:
 	KERNEL_UNLOCK_ONE(NULL);
 	fd_putfile(SCARG(uap, fd));
 	return error;
}


int
svr4_sys_getmsg(struct lwp *l, const struct svr4_sys_getmsg_args *uap, register_t *retval)
{
	file_t *fp;
	struct svr4_strbuf dat, ctl;
	struct svr4_strmcmd sc;
	int error = 0;
	struct msghdr msg;
	struct iovec aiov;
	struct svr4_strm *st;
	int fl;
	struct sockaddr_big sbig;
	struct mbuf *name;

	sbig.sb_len = UCHAR_MAX;
	memset(&sc, 0, sizeof(sc));

#ifdef DEBUG_SVR4
	show_msg(">getmsg", SCARG(uap, fd), SCARG(uap, ctl),
		 SCARG(uap, dat), 0);
#endif /* DEBUG_SVR4 */

	if ((fp = fd_getfile(SCARG(uap, fd))) == NULL)
		return EBADF;

	if (SCARG_PTR(uap, ctl) != NULL) {
		if ((error = copyin(SCARG_PTR(uap, ctl), &ctl,
				    sizeof(ctl))) != 0)
			goto out;
	} else {
		ctl.len = -1;
		ctl.maxlen = 0;
	}

	if (SCARG_PTR(uap, dat) != NULL) {
	    	if ((error = copyin(SCARG_PTR(uap, dat), &dat,
				    sizeof(dat))) != 0)
			goto out;
	} else {
		dat.len = -1;
		dat.maxlen = 0;
	}

	/*
	 * Only for sockets for now.
	 */
	if ((st = svr4_stream_get(fp)) == NULL) {
		DPRINTF(("getmsg: bad file type\n"));
		error = EINVAL;
		goto out;
	}

	if (ctl.maxlen == -1 || dat.maxlen == -1) {
		DPRINTF(("getmsg: Cannot handle -1 maxlen (yet)\n"));
		error = ENOSYS;
		goto out;
	}

	switch (st->s_family) {
	case AF_INET:
	case AF_LOCAL:
		break;

	default:
		DPRINTF(("getmsg: Unsupported address family %d\n",
			 st->s_family));
		goto out;
	}

	switch (st->s_cmd) {
	case SVR4_TI_CONNECT_REQUEST:
		DPRINTF(("getmsg: TI_CONNECT_REQUEST\n"));
		/*
		 * We do the connect in one step, so the putmsg should
		 * have gotten the error.
		 */
		sc.cmd = SVR4_TI_OK_REPLY;
		sc.len = 0;

		ctl.len = 8;
		dat.len = -1;
		fl = 1;
		st->s_cmd = sc.cmd;
		break;

	case SVR4_TI_OK_REPLY:
		DPRINTF(("getmsg: TI_OK_REPLY\n"));
		/*
		 * We are immediately after a connect reply, so we send
		 * a connect verification.
		 */

		error = do_sys_getsockname(SCARG(uap, fd),
		    (struct sockaddr *)&sbig);
		if (error != 0) {
			DPRINTF(("getmsg: getsockname failed %d\n", error));
			goto out;
		}

		sc.cmd = SVR4_TI_CONNECT_REPLY;
		sc.pad[0] = 0x4;
		sc.offs = 0x18;
		sc.pad[1] = 0x14;
		sc.pad[2] = 0x04000402;

		switch (st->s_family) {
		case AF_INET:
			sc.len = sizeof (struct sockaddr_in) + 4;
			sockaddr_to_netaddr_in(&sc,
			    (struct sockaddr_in *)&sbig);
			break;

		case AF_LOCAL:
			sc.len = sizeof (struct sockaddr_un) + 4;
			sockaddr_to_netaddr_un(&sc,
			    (struct sockaddr_un *)&sbig);
			break;

		default:
			error = ENOSYS;
			goto out;
		}

		ctl.len = 40;
		dat.len = -1;
		fl = 0;
		st->s_cmd = sc.cmd;
		break;

	case SVR4_TI__ACCEPT_OK:
		DPRINTF(("getmsg: TI__ACCEPT_OK\n"));
		/*
		 * We do the connect in one step, so the putmsg should
		 * have gotten the error.
		 */
		sc.cmd = SVR4_TI_OK_REPLY;
		sc.len = 1;

		ctl.len = 8;
		dat.len = -1;
		fl = 1;
		st->s_cmd = SVR4_TI__ACCEPT_WAIT;
		break;

	case SVR4_TI__ACCEPT_WAIT:
		DPRINTF(("getmsg: TI__ACCEPT_WAIT\n"));
		/*
		 * We are after a listen, so we try to accept...
		 */

		error = do_sys_accept(l, SCARG(uap, fd),
		    (struct sockaddr *)&sbig, retval, NULL, 0, FNONBLOCK);
		if (error != 0) {
			DPRINTF(("getmsg: accept failed %d\n", error));
			goto out;
		}

		st->s_afd = *retval;

		DPRINTF(("getmsg: Accept fd = %d\n", st->s_afd));

		sc.cmd = SVR4_TI_ACCEPT_REPLY;
		sc.offs = 0x18;
		sc.pad[0] = 0x0;

		switch (st->s_family) {
		case AF_INET:
			sc.pad[1] = 0x28;
			sockaddr_to_netaddr_in(&sc,
			    (struct sockaddr_in *)&sbig);
			ctl.len = 40;
			sc.len = sizeof (struct sockaddr_in);
			break;

		case AF_LOCAL:
			sc.pad[1] = 0x00010000;
			sc.pad[2] = 0xf6bcdaa0;	/* I don't know what that is */
			sc.pad[3] = 0x00010000;
			ctl.len = 134;
			sc.len = sizeof (struct sockaddr_un) + 4;
			break;

		default:
			error = ENOSYS;
			goto out;
		}

		dat.len = -1;
		fl = 0;
		st->s_cmd = SVR4_TI__ACCEPT_OK;
		break;

	case SVR4_TI_SENDTO_REQUEST:
		/*
		 * XXX: dsl - I think this means that because we last did a
		 * 'sendto' we'd better do a recvfrom now.
		 */
		DPRINTF(("getmsg: TI_SENDTO_REQUEST\n"));
		if (ctl.maxlen > 36 && ctl.len < 36)
		    ctl.len = 36;
		if (ctl.len > sizeof(sc))
			ctl.len = sizeof(sc);

		if ((error = copyin(NETBSD32PTR(ctl.buf), &sc, ctl.len)) != 0)
			goto out;

		msg.msg_name = NULL;
		msg.msg_namelen = 0;
		msg.msg_iov = &aiov;
		msg.msg_iovlen = 1;
		msg.msg_control = 0;
		aiov.iov_base = NETBSD32PTR(dat.buf);
		aiov.iov_len = dat.maxlen;
		msg.msg_flags = 0;

		error = do_sys_recvmsg(l,  SCARG(uap, fd), &msg, NULL, 0,
		    &name, NULL, retval);

		if (error) {
			DPRINTF(("getmsg: do_sys_recvmsg failed %d\n", error));
			goto out;
		}

		sc.cmd = SVR4_TI_RECVFROM_IND;

		switch (st->s_family) {
		case AF_INET:
			sc.len = sizeof (struct sockaddr_in);
			sockaddr_to_netaddr_in(&sc, mtod(name, void *));
			break;

		case AF_LOCAL:
			sc.len = sizeof (struct sockaddr_un) + 4;
			sockaddr_to_netaddr_un(&sc, mtod(name, void *));
			break;

		default:
			m_free(name);
			error = ENOSYS;
			goto out;
		}
		m_free(name);

		dat.len = *retval;
		fl = 0;
		st->s_cmd = sc.cmd;
		break;

	default:
		st->s_cmd = sc.cmd;
#ifdef notyet
		if (st->s_cmd == SVR4_TI_CONNECT_REQUEST) {
		        struct sys_read_args ra;

			/* More weirdness:  Again, I can't find documentation
			 * to back this up, but when a process does a generic
			 * "getmsg()" call it seems that the command field is
			 * zero and the length of the data area is zero.  I
			 * think processes expect getmsg() to fill in dat.len
			 * after reading at most dat.maxlen octets from the
			 * stream.  Since we're using sockets I can let
			 * read() look after it and frob return values
			 * appropriately (or inappropriately :-)
			 *   -- newton@atdot.dotat.org        XXX
			 */
			SCARG(&ra, fd) = SCARG(uap, fd);
			SCARG(&ra, buf) = dat.buf;
			SCARG(&ra, nbyte) = dat.maxlen;
			if ((error = sys_read(p, &ra, retval)) != 0)
			        goto out;
			dat.len = *retval;
			*retval = 0;
			st->s_cmd = SVR4_TI_SENDTO_REQUEST;
			break;

		}
#endif
		DPRINTF(("getmsg: Unknown state %x\n", st->s_cmd));
		error = EINVAL;
		goto out;
	}

	if (SCARG_PTR(uap, ctl)) {
		if (ctl.len != -1)
			if ((error = copyout(&sc, NETBSD32PTR(ctl.buf),
					     ctl.len)) != 0)
				goto out;

		if ((error = copyout(&ctl, SCARG_PTR(uap, ctl),
				     sizeof(ctl))) != 0)
			goto out;
	}

	if (SCARG_PTR(uap, dat)) {
		if ((error = copyout(&dat, SCARG_PTR(uap, dat),
				     sizeof(dat))) != 0)
			goto out;
	}

	if (SCARG_PTR(uap, flags)) { /* XXX: Need translation */
		if ((error = copyout(&fl, SCARG_PTR(uap, flags),
				     sizeof(fl))) != 0)
			goto out;
	}

	*retval = 0;

#ifdef DEBUG_SVR4
	show_msg("<getmsg", SCARG(uap, fd), SCARG(uap, ctl),
		 SCARG(uap, dat), fl);
#endif /* DEBUG_SVR4 */

 out:
 	fd_putfile(SCARG(uap, fd));
	return error;
}
