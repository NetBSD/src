/*	$NetBSD: netbsd32_ioctl.c,v 1.4.2.2 2000/12/08 09:08:34 bouyer Exp $	*/

/*
 * Copyright (c) 1998 Matthew R. Green
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
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * handle ioctl conversions from netbsd32 -> sparc64
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/socketvar.h>
#include <sys/audioio.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/sockio.h>
#include <sys/socket.h>
#include <sys/ttycom.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <machine/fbio.h>
#include <machine/openpromio.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/igmp.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_mroute.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_ioctl.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

/* prototypes for the converters */
static __inline void
netbsd32_to_fbcmap(struct netbsd32_fbcmap *, struct fbcmap *, u_long);
static __inline void
netbsd32_to_fbcursor(struct netbsd32_fbcursor *, struct fbcursor *, u_long);
static __inline void
netbsd32_to_opiocdesc(struct netbsd32_opiocdesc *, struct opiocdesc *, u_long);
static __inline void
netbsd32_to_partinfo(struct netbsd32_partinfo *, struct partinfo *, u_long);
static __inline void
netbsd32_to_format_op(struct netbsd32_format_op *, struct format_op *, u_long);
static __inline void
netbsd32_to_ifconf(struct netbsd32_ifconf *, struct ifconf *, u_long);
static __inline void
netbsd32_to_ifmediareq(struct netbsd32_ifmediareq *, struct ifmediareq *, u_long);
static __inline void
netbsd32_to_ifdrv(struct netbsd32_ifdrv *, struct ifdrv *, u_long);
static __inline void
netbsd32_to_sioc_vif_req(struct netbsd32_sioc_vif_req *, struct sioc_vif_req *, u_long);
static __inline void
netbsd32_to_sioc_sg_req(struct netbsd32_sioc_sg_req *, struct sioc_sg_req *, u_long);

static __inline void
netbsd32_from_fbcmap(struct fbcmap *, struct netbsd32_fbcmap *);
static __inline void
netbsd32_from_fbcursor(struct fbcursor *, struct netbsd32_fbcursor *);
static __inline void
netbsd32_from_opiocdesc(struct opiocdesc *, struct netbsd32_opiocdesc *);
static __inline void
netbsd32_from_partinfo(struct partinfo *, struct netbsd32_partinfo *);
static __inline void
netbsd32_from_format_op(struct format_op *, struct netbsd32_format_op *);
static __inline void
netbsd32_from_ifconf(struct ifconf *, struct netbsd32_ifconf *);
static __inline void
netbsd32_from_ifmediareq(struct ifmediareq *, struct netbsd32_ifmediareq *);
static __inline void
netbsd32_from_ifdrv(struct ifdrv *, struct netbsd32_ifdrv *);
static __inline void
netbsd32_from_sioc_vif_req(struct sioc_vif_req *, struct netbsd32_sioc_vif_req *);
static __inline void
netbsd32_from_sioc_sg_req(struct sioc_sg_req *, struct netbsd32_sioc_sg_req *);

/* convert to/from different structures */

static __inline void
netbsd32_to_fbcmap(s32p, p, cmd)
	struct netbsd32_fbcmap *s32p;
	struct fbcmap *p;
	u_long cmd;
{

	p->index = s32p->index;
	p->count = s32p->count;
	p->red = (u_char *)(u_long)s32p->red;
	p->green = (u_char *)(u_long)s32p->green;
	p->blue = (u_char *)(u_long)s32p->blue;
}

static __inline void
netbsd32_to_fbcursor(s32p, p, cmd)
	struct netbsd32_fbcursor *s32p;
	struct fbcursor *p;
	u_long cmd;
{

	p->set = s32p->set;
	p->enable = s32p->enable;
	p->pos = s32p->pos;
	p->hot = s32p->hot;
	netbsd32_to_fbcmap(&s32p->cmap, &p->cmap, cmd);
	p->size = s32p->size;
	p->image = (char *)(u_long)s32p->image;
	p->mask = (char *)(u_long)s32p->mask;
}

static __inline void
netbsd32_to_opiocdesc(s32p, p, cmd)
	struct netbsd32_opiocdesc *s32p;
	struct opiocdesc *p;
	u_long cmd;
{

	p->op_nodeid = s32p->op_nodeid;
	p->op_namelen = s32p->op_namelen;
	p->op_name = (char *)(u_long)s32p->op_name;
	p->op_buflen = s32p->op_buflen;
	p->op_buf = (char *)(u_long)s32p->op_buf;
}

static __inline void
netbsd32_to_partinfo(s32p, p, cmd)
	struct netbsd32_partinfo *s32p;
	struct partinfo *p;
	u_long cmd;
{

	p->disklab = (struct disklabel *)(u_long)s32p->disklab;
	p->part = (struct partition *)(u_long)s32p->part;
}

static __inline void
netbsd32_to_format_op(s32p, p, cmd)
	struct netbsd32_format_op *s32p;
	struct format_op *p;
	u_long cmd;
{

	p->df_buf = (char *)(u_long)s32p->df_buf;
	p->df_count = s32p->df_count;
	p->df_startblk = s32p->df_startblk;
	memcpy(p->df_reg, s32p->df_reg, sizeof(s32p->df_reg));
}

#if 0 /* XXX see below */
static __inline void
netbsd32_to_ifreq(s32p, p, cmd)
	struct netbsd32_ifreq *s32p;
	struct ifreq *p;
	u_long cmd;	/* XXX unused yet */
{

	/*
	 * XXX
	 * struct ifreq says the same, but sometimes the ifr_data
	 * union member needs to be converted to 64 bits... this
	 * is very driver specific and so we ignore it for now..
	 */
	memcpy(p, s32p, sizeof *s32p);
}
#endif

static __inline void
netbsd32_to_ifconf(s32p, p, cmd)
	struct netbsd32_ifconf *s32p;
	struct ifconf *p;
	u_long cmd;
{

	p->ifc_len = s32p->ifc_len;
	/* ifc_buf & ifc_req are the same size so this works */
	p->ifc_buf = (caddr_t)(u_long)s32p->ifc_buf;
}

static __inline void
netbsd32_to_ifmediareq(s32p, p, cmd)
	struct netbsd32_ifmediareq *s32p;
	struct ifmediareq *p;
	u_long cmd;
{

	memcpy(p, s32p, sizeof *s32p);
	p->ifm_ulist = (int *)(u_long)s32p->ifm_ulist;
}

static __inline void
netbsd32_to_ifdrv(s32p, p, cmd)
	struct netbsd32_ifdrv *s32p;
	struct ifdrv *p;
	u_long cmd;
{

	memcpy(p, s32p, sizeof *s32p);
	p->ifd_data = (void *)(u_long)s32p->ifd_data;
}

static __inline void
netbsd32_to_sioc_vif_req(s32p, p, cmd)
	struct netbsd32_sioc_vif_req *s32p;
	struct sioc_vif_req *p;
	u_long cmd;
{

	p->vifi = s32p->vifi;
	p->icount = (u_long)s32p->icount;
	p->ocount = (u_long)s32p->ocount;
	p->ibytes = (u_long)s32p->ibytes;
	p->obytes = (u_long)s32p->obytes;
}

static __inline void
netbsd32_to_sioc_sg_req(s32p, p, cmd)
	struct netbsd32_sioc_sg_req *s32p;
	struct sioc_sg_req *p;
	u_long cmd;
{

	p->src = s32p->src;
	p->grp = s32p->grp;
	p->pktcnt = (u_long)s32p->pktcnt;
	p->bytecnt = (u_long)s32p->bytecnt;
	p->wrong_if = (u_long)s32p->wrong_if;
}

/*
 * handle ioctl conversions from sparc64 -> netbsd32
 */

static __inline void
netbsd32_from_fbcmap(p, s32p)
	struct fbcmap *p;
	struct netbsd32_fbcmap *s32p;
{

	s32p->index = p->index;
	s32p->count = p->count;
/* filled in */
#if 0
	s32p->red = (netbsd32_u_charp)p->red;
	s32p->green = (netbsd32_u_charp)p->green;
	s32p->blue = (netbsd32_u_charp)p->blue;
#endif
}

static __inline void
netbsd32_from_fbcursor(p, s32p)
	struct fbcursor *p;
	struct netbsd32_fbcursor *s32p;
{

	s32p->set = p->set;
	s32p->enable = p->enable;
	s32p->pos = p->pos;
	s32p->hot = p->hot;
	netbsd32_from_fbcmap(&p->cmap, &s32p->cmap);
	s32p->size = p->size;
/* filled in */
#if 0
	s32p->image = (netbsd32_charp)p->image;
	s32p->mask = (netbsd32_charp)p->mask;
#endif
}

static __inline void
netbsd32_from_opiocdesc(p, s32p)
	struct opiocdesc *p;
	struct netbsd32_opiocdesc *s32p;
{

	s32p->op_nodeid = p->op_nodeid;
	s32p->op_namelen = p->op_namelen;
	s32p->op_name = (netbsd32_charp)(u_long)p->op_name;
	s32p->op_buflen = p->op_buflen;
	s32p->op_buf = (netbsd32_charp)(u_long)p->op_buf;
}

static __inline void
netbsd32_from_partinfo(p, s32p)
	struct partinfo *p;
	struct netbsd32_partinfo *s32p;
{

	s32p->disklab = (netbsd32_disklabel_tp_t)(u_long)p->disklab;
	s32p->part = s32p->part;
}

static __inline void
netbsd32_from_format_op(p, s32p)
	struct format_op *p;
	struct netbsd32_format_op *s32p;
{

/* filled in */
#if 0
	s32p->df_buf = (netbsd32_charp)p->df_buf;
#endif
	s32p->df_count = p->df_count;
	s32p->df_startblk = p->df_startblk;
	memcpy(s32p->df_reg, p->df_reg, sizeof(p->df_reg));
}

#if 0 /* XXX see below */
static __inline void
netbsd32_from_ifreq(p, s32p, cmd)
	struct ifreq *p;
	struct netbsd32_ifreq *s32p;
	u_long cmd;	/* XXX unused yet */
{

	/*
	 * XXX
	 * struct ifreq says the same, but sometimes the ifr_data
	 * union member needs to be converted to 64 bits... this
	 * is very driver specific and so we ignore it for now..
	 */
	*s32p = *p;
}
#endif

static __inline void
netbsd32_from_ifconf(p, s32p)
	struct ifconf *p;
	struct netbsd32_ifconf *s32p;
{

	s32p->ifc_len = p->ifc_len;
	/* ifc_buf & ifc_req are the same size so this works */
	s32p->ifc_buf = (netbsd32_caddr_t)(u_long)p->ifc_buf;
}

static __inline void
netbsd32_from_ifmediareq(p, s32p)
	struct ifmediareq *p;
	struct netbsd32_ifmediareq *s32p;
{

	memcpy(s32p, p, sizeof *p);
/* filled in? */
#if 0
	s32p->ifm_ulist = (netbsd32_intp_t)p->ifm_ulist;
#endif
}

static __inline void
netbsd32_from_ifdrv(p, s32p)
	struct ifdrv *p;
	struct netbsd32_ifdrv *s32p;
{

	memcpy(s32p, p, sizeof *p);
/* filled in? */
#if 0
	s32p->ifm_data = (netbsd32_u_longp_t)p->ifm_data;
#endif
}

static __inline void
netbsd32_from_sioc_vif_req(p, s32p)
	struct sioc_vif_req *p;
	struct netbsd32_sioc_vif_req *s32p;
{

	s32p->vifi = p->vifi;
	s32p->icount = (netbsd32_u_long)p->icount;
	s32p->ocount = (netbsd32_u_long)p->ocount;
	s32p->ibytes = (netbsd32_u_long)p->ibytes;
	s32p->obytes = (netbsd32_u_long)p->obytes;
}

static __inline void
netbsd32_from_sioc_sg_req(p, s32p)
	struct sioc_sg_req *p;
	struct netbsd32_sioc_sg_req *s32p;
{

	s32p->src = p->src;
	s32p->grp = p->grp;
	s32p->pktcnt = (netbsd32_u_long)p->pktcnt;
	s32p->bytecnt = (netbsd32_u_long)p->bytecnt;
	s32p->wrong_if = (netbsd32_u_long)p->wrong_if;
}


/*
 * main ioctl syscall.
 *
 * ok, here we are in the biggy.  we have to do fix ups depending
 * on the ioctl command before and afterwards.
 */
int
netbsd32_ioctl(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_u_long) com;
		syscallarg(netbsd32_voidp) data;
	} */ *uap = v;
	struct file *fp;
	struct filedesc *fdp;
	u_long com;
	int error = 0;
	u_int size, size32;
	caddr_t data, memp = NULL;
	caddr_t data32, memp32 = NULL;
	int tmp;
#define STK_PARAMS	128
	u_long stkbuf[STK_PARAMS/sizeof(u_long)];
	u_long stkbuf32[STK_PARAMS/sizeof(u_long)];

	/*
	 * we need to translate some commands (_IOW) before calling sys_ioctl,
	 * some after (_IOR), and some both (_IOWR). 
	 */
#if 0
	{
char *dirs[8] = { "NONE!", "VOID", "OUT", "VOID|OUT!", "IN", "VOID|IN!",
		"INOUT", "VOID|IN|OUT!" };

printf("netbsd32_ioctl(%d, %x, %x): %s group %c base %d len %d\n",
       SCARG(uap, fd), SCARG(uap, com), SCARG(uap, data),
       dirs[((SCARG(uap, com) & IOC_DIRMASK)>>29)],
       IOCGROUP(SCARG(uap, com)), IOCBASECMD(SCARG(uap, com)),
       IOCPARM_LEN(SCARG(uap, com)));
	}
#endif

	fdp = p->p_fd;
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
		return (EBADF);

	FILE_USE(fp);

	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		error = EBADF;
		goto out;
	}

	switch (com = SCARG(uap, com)) {
	case FIONCLEX:
		fdp->fd_ofileflags[SCARG(uap, fd)] &= ~UF_EXCLOSE;
		goto out;

	case FIOCLEX:
		fdp->fd_ofileflags[SCARG(uap, fd)] |= UF_EXCLOSE;
		goto out;
	}

	/*
	 * Interpret high order word to find amount of data to be
	 * copied to/from the user's address space.
	 */
	size32 = IOCPARM_LEN(com);
	if (size32 > IOCPARM_MAX) {
		error = ENOTTY;
		goto out;
	}
	memp = NULL;
	if (size32 > sizeof(stkbuf)) {
		memp32 = (caddr_t)malloc((u_long)size32, M_IOCTLOPS, M_WAITOK);
		data32 = memp32;
	} else
		data32 = (caddr_t)stkbuf32;
	if (com&IOC_IN) {
		if (size32) {
			error = copyin((caddr_t)(u_long)SCARG(uap, data), 
				       data32, size32);
			if (error) {
				if (memp32)
					free(memp32, M_IOCTLOPS);
				goto out;
			}
		} else
			*(caddr_t *)data32 = (caddr_t)(u_long)SCARG(uap, data);
	} else if ((com&IOC_OUT) && size32)
		/*
		 * Zero the buffer so the user always
		 * gets back something deterministic.
		 */
		memset(data32, 0, size32);
	else if (com&IOC_VOID)
		*(caddr_t *)data32 = (caddr_t)(u_long)SCARG(uap, data);

/* we define some handy macros here... */
#define IOCTL_STRUCT_CONV_TO(cmd, type)	\
		com = cmd; \
		size = IOCPARM_LEN(com); \
		if (size > sizeof(stkbuf)) \
			data = memp = malloc(size, M_IOCTLOPS, M_WAITOK); \
		else \
			data = (caddr_t)stkbuf; \
		__CONCAT(netbsd32_to_, type)((struct __CONCAT(netbsd32_, type) *) \
			data32, (struct type *)data, com); \
		error = (*fp->f_ops->fo_ioctl)(fp, com, data, p); \
		__CONCAT(netbsd32_from_, type)((struct type *)data, \
			(struct __CONCAT(netbsd32_, type) *)data32); \
		break

	/*
	 * convert various structures, pointers, and other objects that
	 * change size from 32 bit -> 64 bit, for all ioctl commands.
	 */
	switch (SCARG(uap, com)) {
	case FIONBIO:
		if ((tmp = *(int *)data32) != 0)
			fp->f_flag |= FNONBLOCK;
		else
			fp->f_flag &= ~FNONBLOCK;
		error = (*fp->f_ops->fo_ioctl)(fp, FIONBIO, (caddr_t)&tmp, p);
		break;

	case FIOASYNC:
		if ((tmp = *(int *)data32) != 0)
			fp->f_flag |= FASYNC;
		else
			fp->f_flag &= ~FASYNC;
		error = (*fp->f_ops->fo_ioctl)(fp, FIOASYNC, (caddr_t)&tmp, p);
		break;

	case FIOSETOWN:
		tmp = *(int *)data32;
		if (fp->f_type == DTYPE_SOCKET) {
			((struct socket *)fp->f_data)->so_pgid = tmp;
			error = 0;
			break;
		}
		if (tmp <= 0) {
			tmp = -tmp;
		} else {
			struct proc *p1 = pfind(tmp);
			if (p1 == 0) {
				error = ESRCH;
				break;
			}
			tmp = p1->p_pgrp->pg_id;
		}
		error = (*fp->f_ops->fo_ioctl)
			(fp, TIOCSPGRP, (caddr_t)&tmp, p);
		break;

	case FIOGETOWN:
		if (fp->f_type == DTYPE_SOCKET) {
			error = 0;
			*(int *)data32 = ((struct socket *)fp->f_data)->so_pgid;
			break;
		}
		error = (*fp->f_ops->fo_ioctl)(fp, TIOCGPGRP, data32, p);
		*(int *)data32 = -*(int *)data32;
		break;

/*
 * Here are calls that need explicit conversion.
 */
	case FBIOPUTCMAP32:
		IOCTL_STRUCT_CONV_TO(FBIOPUTCMAP, fbcmap);
	case FBIOGETCMAP32:
		IOCTL_STRUCT_CONV_TO(FBIOGETCMAP, fbcmap);

	case FBIOSCURSOR32:
		IOCTL_STRUCT_CONV_TO(FBIOSCURSOR, fbcursor);
	case FBIOGCURSOR32:
		IOCTL_STRUCT_CONV_TO(FBIOGCURSOR, fbcursor);

	case OPIOCGET32:
		IOCTL_STRUCT_CONV_TO(OPIOCGET, opiocdesc);
	case OPIOCSET32:
		IOCTL_STRUCT_CONV_TO(OPIOCSET, opiocdesc);
	case OPIOCNEXTPROP32:
		IOCTL_STRUCT_CONV_TO(OPIOCNEXTPROP, opiocdesc);

	case DIOCGPART32:
		IOCTL_STRUCT_CONV_TO(DIOCGPART, partinfo);

	case DIOCRFORMAT32:
		IOCTL_STRUCT_CONV_TO(DIOCRFORMAT, format_op);
	case DIOCWFORMAT32:
		IOCTL_STRUCT_CONV_TO(DIOCWFORMAT, format_op);

/*
 * only a few ifreq syscalls need conversion and those are
 * all driver specific... XXX
 */
#if 0
	case SIOCGADDRROM3232:
		IOCTL_STRUCT_CONV_TO(SIOCGADDRROM32, ifreq);
	case SIOCGCHIPID32:
		IOCTL_STRUCT_CONV_TO(SIOCGCHIPID, ifreq);
	case SIOCSIFADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFADDR, ifreq);
	case OSIOCGIFADDR32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFADDR, ifreq);
	case SIOCGIFADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFADDR, ifreq);
	case SIOCSIFDSTADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFDSTADDR, ifreq);
	case OSIOCGIFDSTADDR32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFDSTADDR, ifreq);
	case SIOCGIFDSTADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFDSTADDR, ifreq);
	case SIOCSIFFLAGS32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFFLAGS, ifreq);
	case SIOCGIFFLAGS32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFFLAGS, ifreq);
	case OSIOCGIFBRDADDR32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFBRDADDR, ifreq);
	case SIOCGIFBRDADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFBRDADDR, ifreq);
	case SIOCSIFBRDADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFBRDADDR, ifreq);
	case OSIOCGIFNETMASK32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFNETMASK, ifreq);
	case SIOCGIFNETMASK32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFNETMASK, ifreq);
	case SIOCSIFNETMASK32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFNETMASK, ifreq);
	case SIOCGIFMETRIC32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFMETRIC, ifreq);
	case SIOCSIFMETRIC32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFMETRIC, ifreq);
	case SIOCDIFADDR32:
		IOCTL_STRUCT_CONV_TO(SIOCDIFADDR, ifreq);
	case SIOCADDMULTI32:
		IOCTL_STRUCT_CONV_TO(SIOCADDMULTI, ifreq);
	case SIOCDELMULTI32:
		IOCTL_STRUCT_CONV_TO(SIOCDELMULTI, ifreq);
	case SIOCSIFMEDIA32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFMEDIA, ifreq);
	case SIOCSIFMTU32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFMTU, ifreq);
	case SIOCGIFMTU32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFMTU, ifreq);
	case SIOCSIFASYNCMAP32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFASYNCMAP, ifreq);
	case SIOCGIFASYNCMAP32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFASYNCMAP, ifreq);
/*		IOCTL_STRUCT_CONV_TO(BIOCGETIF, ifreq); READ ONLY */
	case BIOCSETIF32:
		IOCTL_STRUCT_CONV_TO(BIOCSETIF, ifreq);
	case SIOCPHASE132:
		IOCTL_STRUCT_CONV_TO(SIOCPHASE1, ifreq);
	case SIOCPHASE232:
		IOCTL_STRUCT_CONV_TO(SIOCPHASE2, ifreq);
#endif

	case OSIOCGIFCONF32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFCONF, ifconf);
	case SIOCGIFCONF32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFCONF, ifconf);

	case SIOCGIFMEDIA32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFMEDIA, ifmediareq);

	case SIOCSDRVSPEC32:
		IOCTL_STRUCT_CONV_TO(SIOCSDRVSPEC, ifdrv);

	case SIOCGETVIFCNT32:
		IOCTL_STRUCT_CONV_TO(SIOCGETVIFCNT, sioc_vif_req);

	case SIOCGETSGCNT32:
		IOCTL_STRUCT_CONV_TO(SIOCGETSGCNT, sioc_sg_req);

	default:
		error = (*fp->f_ops->fo_ioctl)(fp, com, data32, p);
		break;
	}

	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (error == 0 && (com&IOC_OUT) && size32)
		error = copyout(data32, (caddr_t)(u_long)SCARG(uap, data), size32);

	/* if we malloced data, free it here */
	if (memp32)
		free(memp32, M_IOCTLOPS);
	if (memp)
		free(memp, M_IOCTLOPS);

 out:
	FILE_UNUSE(fp, p);
	return (error);
}
