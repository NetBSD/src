/*	$NetBSD: svr4_stream.c,v 1.1 1994/11/14 06:13:20 christos Exp $	 */

/*
 * Copyright (c) 1994 Christos Zoulas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Pretend that we have streams...
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/device.h>

#include <sys/syscallargs.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_syscallargs.h>
#include <compat/svr4/svr4_stropts.h>
#include <compat/svr4/svr4_timod.h>
#include <compat/svr4/svr4_sockmod.h>
#include <compat/svr4/svr4_ioctl.h>

static int
svr4_sockmod(fp, ioc, p)
	struct file		*fp;
	struct svr4_strioctl	*ioc;
	struct proc		*p;
{
	int error;

	switch (ioc->cmd) {
	case SVR4_SI_GETUDATA:
		{
			struct svr4_si_udata ud;

			DPRINTF(("SI_GETUDATA\n"));

			if (sizeof(ud) != ioc->len) {
				DPRINTF(("Wrong size %d != %d\n", 
					 sizeof(ud), ioc->len));
				return EINVAL;
			}

			if ((error = copyin(ioc->buf, &ud, sizeof(ud))) != 0)
				return error;

			/* I have no idea what these should be! */
			ud.tidusize = 16384;
			ud.addrsize = 16;
			ud.optsize = 128;
			ud.etsdusize = 0;
			ud.servtype = 3;
			ud.so_state = 0;
			ud.so_options = 0;
			return copyout(&ud, ioc->buf, sizeof(ud));
		}

	case SVR4_SI_SHUTDOWN:
		DPRINTF(("SI_SHUTDOWN\n"));
		return 0;

	case SVR4_SI_LISTEN:
		DPRINTF(("SI_LISTEN\n"));
		return 0;

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

	default:
		DPRINTF(("Unknown sockmod ioctl %x\n", ioc->cmd));
		return 0;

	}
}


static int
svr4_timod(fp, ioc, p)
	struct file		*fp;
	struct svr4_strioctl	*ioc;
	struct proc		*p;
{
	switch (ioc->cmd) {
	case SVR4_TI_GETINFO:
		DPRINTF(("TI_GETINFO\n"));
		return 0;

	case SVR4_TI_OPTMGMT:
		DPRINTF(("TI_OPTMGMT\n"));
		return 0;

	case SVR4_TI_BIND:
		DPRINTF(("TI_BIND\n"));
		return 0;

	case SVR4_TI_UNBIND:
		DPRINTF(("TI_UNBIND\n"));
		return 0;

	case SVR4_TI_GETMYNAME:
		DPRINTF(("TI_GETMYNAME\n"));
		return 0;

	case SVR4_TI_GETPEERNAME:
		DPRINTF(("TI_GETPEERNAME\n"));
		return 0;

	case SVR4_TI_SETMYNAME:
		DPRINTF(("TI_SETMYNAME\n"));
		return 0;

	case SVR4_TI_SETPEERNAME:
		DPRINTF(("TI_SETPEERNAME\n"));
		return 0;

	default:
		DPRINTF(("Unknown timod ioctl %x\n", ioc->cmd));
		return 0;
	}
}


int
svr4_streamioctl(fp, cmd, dat, p, retval)
	struct file	*fp;
	u_long		 cmd;
	caddr_t		 dat;
	struct proc	*p;
	register_t	*retval;
{
	struct svr4_strioctl	 ioc;
	int			 error;

	*retval = 0;

	/*
	 * All the following stuff assumes "sockmod" is pushed...
	 */
	switch (cmd) {
	case SVR4_I_NREAD:
		DPRINTF(("I_NREAD\n"));
		return 0;

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
		if ((error = copyin(dat, &ioc, sizeof(ioc))) != 0)
			return error;

#ifdef DEBUG_SVR4
		{
			char *ptr = (char *) malloc(ioc.len, M_TEMP, M_WAITOK);
			int i;

			printf("cmd = %d, timeout = %d, len = %d, buf = %x { ",
			       ioc.cmd, ioc.timeout, ioc.len, ioc.buf);

			if ((error = copyin(ioc.buf, ptr, ioc.len)) != 0) {
				free((char *) ptr, M_TEMP);
				return error;
			}

			for (i = 0; i < ioc.len; i++)
				printf("%x ", (unsigned char) ptr[i]);

			printf("}\n");

			free((char *) ptr, M_TEMP);
		}
#endif


		switch (ioc.cmd & 0xff00) {
		case SVR4_SIMOD:
			return svr4_sockmod(fp, &ioc, p);

		case SVR4_TIMOD:
			return svr4_timod(fp, &ioc, p);

		default:
			DPRINTF(("Unimplemented module %c %d\n",
				 (char) (cmd >> 8), cmd & 0xff));
			return 0;
		}

	case SVR4_I_SETSIG:
		DPRINTF(("I_SETSIG\n"));
		return 0;

	case SVR4_I_GETSIG:
		DPRINTF(("I_GETSIG\n"));
		return 0;

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
		return 0;

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

	default:
		DPRINTF(("unimpl cmd = %x\n", cmd));
		break;
	}

	return 0;
}


#ifdef DEBUG_SVR4


static int
svr4_getstrbuf(str)
	struct svr4_strbuf *str;
{
	int error;
	int i;
	char *ptr = NULL;
	int maxlen = str->maxlen;
	int len = str->len;

	if (maxlen < 0)
		maxlen = 0;

	if (str->len >= str->maxlen || str->len == 0)
		len = maxlen;

	if (len != 0) {
	    ptr = malloc(len, M_TEMP, M_WAITOK);

	    if ((error = copyin(str->buf, ptr, len)) != 0) {
		    free((char *) ptr, M_TEMP);
		    return error;
	}    }

	printf(", { %d, %d, %x=[ ", str->maxlen, str->len, str->buf);
	for (i = 0; i < len; i++)
		printf("%x ", (unsigned char) ptr[i]);
	printf("]}");

	if (ptr)
		free((char *) ptr, M_TEMP);

	return 0;
}


static void
svr4_showmsg(str, fd, ctl, dat, flags)
	const char		*str;
	int			 fd;
	struct svr4_strbuf	*ctl;
	struct svr4_strbuf	*dat;
	int			 flags;
{
	struct svr4_strbuf	buf;
	int error;

	printf("%s(%d", str, fd);
	if (ctl != NULL) {
		if ((error = copyin(ctl, &buf, sizeof(buf))) != 0)
			return;
		svr4_getstrbuf(&buf);
	}
	else 
		printf(", NULL");

	if (dat != NULL) {
		if ((error = copyin(dat, &buf, sizeof(buf))) != 0)
			return;
		svr4_getstrbuf(&buf);
	}
	else 
		printf(", NULL");

	printf(", %x);\n", flags);
}
#endif


int
svr4_putmsg(p, uap, retval)
	register struct proc			*p;
	register struct svr4_putmsg_args	*uap;
	register_t				*retval;
{
	struct filedesc	*fdp = p->p_fd;
	struct file	*fp = fdp->fd_ofiles[SCARG(uap, fd)];
	struct svr4_strbuf dat, ctl;
	struct svr4_sockctl sc;
	int error;
	struct sockaddr_in sa, *sap;
	caddr_t sg;

#ifdef DEBUG_SVR4
	svr4_showmsg(">putmsg", SCARG(uap, fd), SCARG(uap, ctl),
		     SCARG(uap, dat), SCARG(uap, flags));
#endif

	if (SCARG(uap, ctl) != NULL) {
		if ((error = copyin(SCARG(uap, ctl), &ctl, sizeof(ctl))) != 0)
			return error;
	}
	else
		ctl.len = -1;

	if (SCARG(uap, dat) != NULL) {
	    	if ((error = copyin(SCARG(uap, dat), &dat,
				    sizeof(dat))) != 0)
			return error;
	}
	else
		dat.len = -1;

	/*
	 * Only for sockets for now.
	 */
	if (fp == NULL || fp->f_type != DTYPE_SOCKET) {
		DPRINTF(("putmsg: bad file type\n"));
		return EINVAL;
	}


	if (ctl.len != sizeof(struct svr4_sockctl)) {
		DPRINTF(("putmsg: Bad control size %d != %d\n", ctl.len,
			 sizeof(struct svr4_sockctl)));
		return EINVAL;
	}

	if ((error = copyin(ctl.buf, &sc, sizeof(sc))) != 0)
		return error;

	bzero(&sa, sizeof(sa));
	sa.sin_family = sc.family;
	sa.sin_port = sc.port;
	sa.sin_addr.s_addr = sc.addr;

	sg = stackgap_init();
	sap = (struct sockaddr_in *) stackgap_alloc(&sg,
						    sizeof(struct sockaddr_in));


	if ((error = copyout(&sa, sap, sizeof(sa))) != 0)
		return error;

	switch (sc.cmd) {
	case SVR4_SC_CMD_CONNECT:	/* connect 	*/
		{
			struct connect_args co;

			co.s = SCARG(uap, fd);
			co.name = (caddr_t) sap;
			co.namelen = (int) sizeof(sa);
			return connect(p, &co, retval);
		}

	case SVR4_SC_CMD_SENDTO:	/* sendto 	*/
		{
			struct msghdr msg;
			struct iovec aiov;
			msg.msg_name = (caddr_t) sap;
			msg.msg_namelen = sizeof(sa);
			msg.msg_iov = &aiov;
			msg.msg_iovlen = 1;
			msg.msg_control = 0;
#ifdef COMPAT_OLDSOCK
			msg.msg_flags = 0;
#endif
			aiov.iov_base = dat.buf;
			aiov.iov_len = dat.len;
			error = sendit(p, SCARG(uap, fd), &msg,
				       SCARG(uap, flags), retval);

			*retval = 0;
			return error;
		}
	default:
		DPRINTF(("Unimplemented putmsg command %x\n", sc.cmd));
		return ENOSYS;
	}
}


int
svr4_getmsg(p, uap, retval)
	register struct proc			*p;
	register struct svr4_getmsg_args	*uap;
	register_t				*retval;
{
	struct filedesc	*fdp = p->p_fd;
	struct file	*fp = fdp->fd_ofiles[SCARG(uap, fd)];
	struct svr4_strbuf dat, ctl;
	struct svr4_sockctl* sc;
	struct svr4_sockctl1 scr;
	int error;
	struct msghdr msg;
	struct iovec aiov;
	struct sockaddr_in sa, *sap;
	int *flen;
	int fl;
	caddr_t sg;

#ifdef DEBUG_SVR4
	svr4_showmsg(">getmsg", SCARG(uap, fd), SCARG(uap, ctl),
		     SCARG(uap, dat), 0);
#endif
			
	if (SCARG(uap, ctl) != NULL) {
		if ((error = copyin(SCARG(uap, ctl), &ctl, sizeof(ctl))) != 0)
			return error;
	}
	else {
		ctl.len = -1;
		ctl.maxlen = 0;
	}

	if (SCARG(uap, dat) != NULL) {
	    	if ((error = copyin(SCARG(uap, dat),
				    &dat, sizeof(dat))) != 0)
			return error;
	}
	else {
		dat.len = -1;
		dat.maxlen = 0;
	}

	/*
	 * Only for sockets for now.
	 */
	if (fp == NULL || fp->f_type != DTYPE_SOCKET) {
		DPRINTF(("getmsg: bad file type\n"));
		return EINVAL;
	}

	if (ctl.maxlen == -1 || dat.maxlen == -1) {
		DPRINTF(("getmsg: Cannot handle -1 maxlen (yet)\n"));
		return ENOSYS;
	}

	/*
	 * HACK: This is the only case so far that has these arguments
	 */
	if (dat.len == -1 && ctl.len != -1) {
		DPRINTF(("getmsg with no dat\n"));
		ctl.len = 8;
		scr.cmd = 0x13;
		scr.unk1 = 0x0;
		fl = 1;
		goto out;
	}

	sg = stackgap_init();
	sap = (struct sockaddr_in *) stackgap_alloc(&sg,
						    sizeof(struct sockaddr_in));
	flen = (int *) stackgap_alloc(&sg, sizeof(*flen));


	/*
	 * XXX: This is totally wrong we need to keep some state, but we do
	 *      not for now.
	 */
	if (ctl.len != -1 && ctl.maxlen > sizeof(scr)) {
		if ((error = copyin(ctl.buf, &scr, sizeof(scr))) != 0)
			return error;
		/*
		 * HACK: we take advantage that the socket library passes
		 * 	 the same buffer back to us. The previous call
		 *	 we put the 0x13 there, now we are checking for it
		 */
		if (scr.cmd == 0x13) {
			struct getpeername_args ga;
			fl = sizeof(sa);
			if ((error = copyout(&fl, flen, sizeof(fl))) != 0)
				return error;

			SCARG(&ga, fdes) = SCARG(uap, fd);
			SCARG(&ga, asa) = (caddr_t) sap;
			SCARG(&ga, alen) = flen;
			
			if ((error = getpeername(p, &ga, retval)) != 0) {
				DPRINTF(("getpeername failed %d\n", error));
				return error;
			}

			if ((error = copyin(sap, &sa, sizeof(sa))) != 0)
				return error;
			
			scr.cmd = 0xc;
			scr.unk1 = 0x10;
			scr.unk2 = 0x18;
			scr.unk3 = 0x4;
			scr.unk4 = 0x14;
			scr.unk5 = 0x04000402;
			scr.unk6 = 0;
			scr.unk7 = 0;
			scr.family = sa.sin_family;
			scr.port = sa.sin_port;
			scr.addr = sa.sin_addr.s_addr;
			ctl.len = sizeof(scr);
			dat.len = -1;
			fl = 0;
			goto out;
		} else {
		   	sc = (struct svr4_sockctl *) &scr; 
		    
			bzero(&sa, sizeof(sa));
			sa.sin_family = sc->family;
			sa.sin_port = sc->port;
			sa.sin_addr.s_addr = sc->addr;

			if ((error = copyout(&sa, sap, sizeof(sa))) != 0)
				return error;

			msg.msg_name = (caddr_t) sap;
			msg.msg_namelen = sizeof(sa);
			msg.msg_iov = &aiov;
			msg.msg_iovlen = 1;
			msg.msg_control = 0;
			aiov.iov_base = dat.buf;
			aiov.iov_len = dat.maxlen;
			msg.msg_flags = 0;

			if ((error = copyout(&msg.msg_namelen, flen,
					     sizeof(*flen))) != 0)
				return error;

			error = recvit(p, SCARG(uap, fd), &msg, flen, retval);

			if (error) {
				DPRINTF(("recvit failed %d\n", error))
				return error;
			}

			dat.len = *retval;

			if ((error = copyin(msg.msg_name, &sa,
					    sizeof(sa))) != 0)
				return error;

			sc->cmd   = SVR4_SC_CMD_SENDTO;
			sc->unk1  = 0x10;
			sc->unk2  = 0x14;
			sc->unk3 = 0;
			sc->unk4 = 0;
			sc->unk5 = 0;
			sc->unk6 = 0;
			sc->family = sa.sin_family;
			sc->port = sa.sin_port;
			sc->addr = sa.sin_addr.s_addr;
			fl = 0; /* XXX */
			goto out;
		}
	}


out:
	if (SCARG(uap, ctl)) {
		if (ctl.len != -1)
			if ((error = copyout(&scr, ctl.buf, ctl.len)) != 0)
				return error;

		if ((error = copyout(&ctl, SCARG(uap, ctl), sizeof(ctl))) != 0)
			return error;
	}

	if (SCARG(uap, dat)) {
		if ((error = copyout(&dat, SCARG(uap, dat), sizeof(dat))) != 0)
			return error;
	}

	if (SCARG(uap, flags)) { /* XXX: Need translation */
		if ((error = copyout(&fl, SCARG(uap, flags), sizeof(fl))) != 0)
			return error;
	}

	*retval = 0;

#ifdef DEBUG_SVR4
	svr4_showmsg("<getmsg", SCARG(uap, fd), SCARG(uap, ctl),
		     SCARG(uap, dat), fl);
#endif
	return error;
}
