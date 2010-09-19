/*	$NetBSD: netbsd32_ioctl.c,v 1.50 2010/09/19 10:33:31 mrg Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 * handle ioctl conversions from netbsd32 -> 64-bit kernel
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_ioctl.c,v 1.50 2010/09/19 10:33:31 mrg Exp $");

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
#include <sys/sockio.h>
#include <sys/socket.h>
#include <sys/ttycom.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/ktrace.h>
#include <sys/kmem.h>

#ifdef __sparc__
#include <dev/sun/fbio.h>
#include <machine/openpromio.h>
#endif

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/igmp.h>
#include <netinet/igmp_var.h>
#include <netinet/ip_mroute.h>

#include <compat/sys/sockio.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_ioctl.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <dev/vndvar.h>

/* prototypes for the converters */
static inline void netbsd32_to_partinfo(struct netbsd32_partinfo *,
					  struct partinfo *, u_long);
#if 0
static inline void netbsd32_to_format_op(struct netbsd32_format_op *,
					   struct format_op *, u_long);
#endif
static inline void netbsd32_to_oifreq(struct netbsd32_oifreq *, struct oifreq *,
				       u_long cmd);
static inline void netbsd32_to_ifreq(struct netbsd32_ifreq *, struct ifreq *,
				       u_long cmd);
static inline void netbsd32_to_ifconf(struct netbsd32_ifconf *,
					struct ifconf *, u_long);
static inline void netbsd32_to_ifmediareq(struct netbsd32_ifmediareq *,
					    struct ifmediareq *, u_long);
static inline void netbsd32_to_ifdrv(struct netbsd32_ifdrv *, struct ifdrv *,
				       u_long);
static inline void netbsd32_to_sioc_vif_req(struct netbsd32_sioc_vif_req *,
					      struct sioc_vif_req *, u_long);
static inline void netbsd32_to_sioc_sg_req(struct netbsd32_sioc_sg_req *,
					     struct sioc_sg_req *, u_long);
static inline void netbsd32_from_partinfo(struct partinfo *,
					    struct netbsd32_partinfo *, u_long);
#if 0
static inline void netbsd32_from_format_op(struct format_op *,
					     struct netbsd32_format_op *,
					     u_long);
#endif
static inline void netbsd32_from_ifreq(struct ifreq *,
                                         struct netbsd32_ifreq *, u_long);
static inline void netbsd32_from_oifreq(struct oifreq *,
                                         struct netbsd32_oifreq *, u_long);
static inline void netbsd32_from_ifconf(struct ifconf *,
					  struct netbsd32_ifconf *, u_long);
static inline void netbsd32_from_ifmediareq(struct ifmediareq *,
					      struct netbsd32_ifmediareq *,
					      u_long);
static inline void netbsd32_from_ifdrv(struct ifdrv *,
					 struct netbsd32_ifdrv *, u_long);
static inline void netbsd32_from_sioc_vif_req(struct sioc_vif_req *,
						struct netbsd32_sioc_vif_req *,
						u_long);
static inline void netbsd32_from_sioc_sg_req(struct sioc_sg_req *,
					       struct netbsd32_sioc_sg_req *,
					       u_long);

/* convert to/from different structures */

static inline void
netbsd32_to_partinfo(struct netbsd32_partinfo *s32p, struct partinfo *p, u_long cmd)
{

	p->disklab = (struct disklabel *)NETBSD32PTR64(s32p->disklab);
	p->part = (struct partition *)NETBSD32PTR64(s32p->part);
}

#if 0
static inline void
netbsd32_to_format_op(struct netbsd32_format_op *s32p, struct format_op *p, u_long cmd)
{

	p->df_buf = (char *)NETBSD32PTR64(s32p->df_buf);
	p->df_count = s32p->df_count;
	p->df_startblk = s32p->df_startblk;
	memcpy(p->df_reg, s32p->df_reg, sizeof(s32p->df_reg));
}
#endif

static inline void
netbsd32_to_ifreq(struct netbsd32_ifreq *s32p, struct ifreq *p, u_long cmd)
{

	memcpy(p, s32p, sizeof *s32p);
	/*
	 * XXX
	 * struct ifreq says the same, but sometimes the ifr_data
	 * union member needs to be converted to 64 bits... this
	 * is very driver specific and so we ignore it for now..
	 */
	if (cmd == SIOCGIFDATA || cmd == SIOCZIFDATA)
		p->ifr_data = (void *)NETBSD32PTR64(s32p->ifr_data);
}

static inline void
netbsd32_to_oifreq(struct netbsd32_oifreq *s32p, struct oifreq *p, u_long cmd)
{

	memcpy(p, s32p, sizeof *s32p);
	/*
	 * XXX
	 * struct ifreq says the same, but sometimes the ifr_data
	 * union member needs to be converted to 64 bits... this
	 * is very driver specific and so we ignore it for now..
	 */
	if (cmd == SIOCGIFDATA || cmd == SIOCZIFDATA)
		p->ifr_data = (void *)NETBSD32PTR64(s32p->ifr_data);
}

static inline void
netbsd32_to_ifconf(struct netbsd32_ifconf *s32p, struct ifconf *p, u_long cmd)
{

	p->ifc_len = s32p->ifc_len;
	/* ifc_buf & ifc_req are the same size so this works */
	p->ifc_buf = (void *)NETBSD32PTR64(s32p->ifc_buf);
}

static inline void
netbsd32_to_ifmediareq(struct netbsd32_ifmediareq *s32p, struct ifmediareq *p, u_long cmd)
{

	memcpy(p, s32p, sizeof *s32p);
	p->ifm_ulist = (int *)NETBSD32PTR64(s32p->ifm_ulist);
}

static inline void
netbsd32_to_ifdrv(struct netbsd32_ifdrv *s32p, struct ifdrv *p, u_long cmd)
{

	memcpy(p, s32p, sizeof *s32p);
	p->ifd_data = (void *)NETBSD32PTR64(s32p->ifd_data);
}

static inline void
netbsd32_to_sioc_vif_req(struct netbsd32_sioc_vif_req *s32p, struct sioc_vif_req *p, u_long cmd)
{

	p->vifi = s32p->vifi;
	p->icount = (u_long)s32p->icount;
	p->ocount = (u_long)s32p->ocount;
	p->ibytes = (u_long)s32p->ibytes;
	p->obytes = (u_long)s32p->obytes;
}

static inline void
netbsd32_to_sioc_sg_req(struct netbsd32_sioc_sg_req *s32p, struct sioc_sg_req *p, u_long cmd)
{

	p->src = s32p->src;
	p->grp = s32p->grp;
	p->pktcnt = (u_long)s32p->pktcnt;
	p->bytecnt = (u_long)s32p->bytecnt;
	p->wrong_if = (u_long)s32p->wrong_if;
}

static inline void
netbsd32_to_vnd_ioctl(struct netbsd32_vnd_ioctl *s32p, struct vnd_ioctl *p, u_long cmd)
{

	p->vnd_file = (char *)NETBSD32PTR64(s32p->vnd_file);
	p->vnd_flags = s32p->vnd_flags;
	p->vnd_geom = s32p->vnd_geom;
	p->vnd_osize = s32p->vnd_osize;
	p->vnd_size = s32p->vnd_size;
}

static inline void
netbsd32_to_vnd_user(struct netbsd32_vnd_user *s32p, struct vnd_user *p, u_long cmd)
{

	p->vnu_unit = s32p->vnu_unit;
	p->vnu_dev = s32p->vnu_dev;
	p->vnu_ino = s32p->vnu_ino;
}

static inline void
netbsd32_to_vnd_ioctl50(struct netbsd32_vnd_ioctl50 *s32p, struct vnd_ioctl50 *p, u_long cmd)
{

	p->vnd_file = (char *)NETBSD32PTR64(s32p->vnd_file);
	p->vnd_flags = s32p->vnd_flags;
	p->vnd_geom = s32p->vnd_geom;
	p->vnd_size = s32p->vnd_size;
}

static inline void
netbsd32_to_u_long(netbsd32_u_long *s32p, u_long *p, u_long cmd)
{

	*p = (u_long)*s32p;
}

/*
 * handle ioctl conversions from 64-bit kernel -> netbsd32
 */

static inline void
netbsd32_from_partinfo(struct partinfo *p, struct netbsd32_partinfo *s32p, u_long cmd)
{

	NETBSD32PTR32(s32p->disklab, p->disklab);
	NETBSD32PTR32(s32p->part, p->part);
}

#if 0
static inline void
netbsd32_from_format_op(struct format_op *p, struct netbsd32_format_op *s32p, u_long cmd)
{

/* filled in */
#if 0
	s32p->df_buf = (netbsd32_charp)p->df_buf;
#endif
	s32p->df_count = p->df_count;
	s32p->df_startblk = p->df_startblk;
	memcpy(s32p->df_reg, p->df_reg, sizeof(p->df_reg));
}
#endif

static inline void
netbsd32_from_ifreq(struct ifreq *p, struct netbsd32_ifreq *s32p, u_long cmd)
{

	/*
	 * XXX
	 * struct ifreq says the same, but sometimes the ifr_data
	 * union member needs to be converted to 64 bits... this
	 * is very driver specific and so we ignore it for now..
	 */
	memcpy(s32p, p, sizeof *s32p);
	if (cmd == SIOCGIFDATA || cmd == SIOCZIFDATA)
		NETBSD32PTR32(s32p->ifr_data, p->ifr_data);
}

static inline void
netbsd32_from_oifreq(struct oifreq *p, struct netbsd32_oifreq *s32p, u_long cmd)
{

	/*
	 * XXX
	 * struct ifreq says the same, but sometimes the ifr_data
	 * union member needs to be converted to 64 bits... this
	 * is very driver specific and so we ignore it for now..
	 */
	memcpy(s32p, p, sizeof *s32p);
	if (cmd == SIOCGIFDATA || cmd == SIOCZIFDATA)
		NETBSD32PTR32(s32p->ifr_data, p->ifr_data);
}

static inline void
netbsd32_from_ifconf(struct ifconf *p, struct netbsd32_ifconf *s32p, u_long cmd)
{

	s32p->ifc_len = p->ifc_len;
	/* ifc_buf & ifc_req are the same size so this works */
	NETBSD32PTR32(s32p->ifc_buf, p->ifc_buf);
}

static inline void
netbsd32_from_ifmediareq(struct ifmediareq *p, struct netbsd32_ifmediareq *s32p, u_long cmd)
{

	memcpy(s32p, p, sizeof *p);
/* filled in? */
#if 0
	s32p->ifm_ulist = (netbsd32_intp_t)p->ifm_ulist;
#endif
}

static inline void
netbsd32_from_ifdrv(struct ifdrv *p, struct netbsd32_ifdrv *s32p, u_long cmd)
{

	memcpy(s32p, p, sizeof *p);
/* filled in? */
#if 0
	s32p->ifm_data = (netbsd32_u_longp_t)p->ifm_data;
#endif
}

static inline void
netbsd32_from_sioc_vif_req(struct sioc_vif_req *p, struct netbsd32_sioc_vif_req *s32p, u_long cmd)
{

	s32p->vifi = p->vifi;
	s32p->icount = (netbsd32_u_long)p->icount;
	s32p->ocount = (netbsd32_u_long)p->ocount;
	s32p->ibytes = (netbsd32_u_long)p->ibytes;
	s32p->obytes = (netbsd32_u_long)p->obytes;
}

static inline void
netbsd32_from_sioc_sg_req(struct sioc_sg_req *p, struct netbsd32_sioc_sg_req *s32p, u_long cmd)
{

	s32p->src = p->src;
	s32p->grp = p->grp;
	s32p->pktcnt = (netbsd32_u_long)p->pktcnt;
	s32p->bytecnt = (netbsd32_u_long)p->bytecnt;
	s32p->wrong_if = (netbsd32_u_long)p->wrong_if;
}

static inline void
netbsd32_from_vnd_ioctl(struct vnd_ioctl *p, struct netbsd32_vnd_ioctl *s32p, u_long cmd)
{

	s32p->vnd_flags = p->vnd_flags;
	s32p->vnd_geom = p->vnd_geom;
	s32p->vnd_osize = p->vnd_osize;
	s32p->vnd_size = p->vnd_size;
}

static inline void
netbsd32_from_vnd_user(struct vnd_user *p, struct netbsd32_vnd_user *s32p, u_long cmd)
{

	s32p->vnu_unit = p->vnu_unit;
	s32p->vnu_dev = p->vnu_dev;
	s32p->vnu_ino = p->vnu_ino;
}

static inline void
netbsd32_from_vnd_ioctl50(struct vnd_ioctl50 *p, struct netbsd32_vnd_ioctl50 *s32p, u_long cmd)
{

	s32p->vnd_flags = p->vnd_flags;
	s32p->vnd_geom = p->vnd_geom;
	s32p->vnd_size = p->vnd_size;
}

static inline void
netbsd32_from_u_long(u_long *p, netbsd32_u_long *s32p, u_long cmd)
{

	*s32p = (netbsd32_u_long)*p;
}


/*
 * main ioctl syscall.
 *
 * ok, here we are in the biggy.  we have to do fix ups depending
 * on the ioctl command before and afterwards.
 */
int
netbsd32_ioctl(struct lwp *l, const struct netbsd32_ioctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_u_long) com;
		syscallarg(netbsd32_voidp) data;
	} */
	struct proc *p = l->l_proc;
	struct file *fp;
	struct filedesc *fdp;
	u_long com;
	int error = 0;
	u_int size, size32;
	void *data, *memp = NULL;
	void *data32, *memp32 = NULL;
	unsigned fd;
	fdfile_t *ff;
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
	fd = SCARG(uap, fd);
	if ((fp = fd_getfile(fd)) == NULL)
		return (EBADF);
	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		error = EBADF;
		goto out;
	}

	ff = fdp->fd_dt->dt_ff[SCARG(uap, fd)];
	switch (com = SCARG(uap, com)) {
	case FIOCLEX:
		ff->ff_exclose = true;
		fdp->fd_exclose = true;
		goto out;

	case FIONCLEX:
		ff->ff_exclose = false;
		goto out;
	}

	/*
	 * Interpret high order word to find amount of data to be
	 * copied to/from the user's address space.
	 */
	size = 0;
	size32 = IOCPARM_LEN(com);
	if (size32 > IOCPARM_MAX) {
		error = ENOTTY;
		goto out;
	}
	if (size32 > sizeof(stkbuf)) {
		memp32 = kmem_alloc((size_t)size32, KM_SLEEP);
		data32 = memp32;
	} else
		data32 = (void *)stkbuf32;
	if (com&IOC_IN) {
		if (size32) {
			error = copyin(SCARG_P32(uap, data), data32, size32);
			if (error) {
				if (memp32)
					kmem_free(memp32, (size_t)size32);
				goto out;
			}
			ktrgenio(fd, UIO_WRITE, SCARG_P32(uap, data),
			    size32, 0);
		} else
			*(void **)data32 = SCARG_P32(uap, data);
	} else if ((com&IOC_OUT) && size32)
		/*
		 * Zero the buffer so the user always
		 * gets back something deterministic.
		 */
		memset(data32, 0, size32);
	else if (com&IOC_VOID)
		*(void **)data32 = SCARG_P32(uap, data);

	/*
	 * convert various structures, pointers, and other objects that
	 * change size from 32 bit -> 64 bit, for all ioctl commands.
	 */
	switch (SCARG(uap, com)) {
	case FIONBIO:
		mutex_enter(&fp->f_lock);
		if ((tmp = *(int *)data32) != 0)
			fp->f_flag |= FNONBLOCK;
		else
			fp->f_flag &= ~FNONBLOCK;
		mutex_exit(&fp->f_lock);
		error = (*fp->f_ops->fo_ioctl)(fp, FIONBIO, (void *)&tmp);
		break;

	case FIOASYNC:
		mutex_enter(&fp->f_lock);
		if ((tmp = *(int *)data32) != 0)
			fp->f_flag |= FASYNC;
		else
			fp->f_flag &= ~FASYNC;
		mutex_exit(&fp->f_lock);
		error = (*fp->f_ops->fo_ioctl)(fp, FIOASYNC, (void *)&tmp);
		break;

	case AUDIO_WSEEK32:
		IOCTL_CONV_TO(AUDIO_WSEEK, u_long);

	case DIOCGPART32:
		IOCTL_STRUCT_CONV_TO(DIOCGPART, partinfo);
#if 0	/* not implemented by anything */
	case DIOCRFORMAT32:
		IOCTL_STRUCT_CONV_TO(DIOCRFORMAT, format_op);
	case DIOCWFORMAT32:
		IOCTL_STRUCT_CONV_TO(DIOCWFORMAT, format_op);
#endif

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
	case BIOCGETIF32:
		IOCTL_STRUCT_CONV_TO(BIOCGETIF, ifreq);
	case BIOCSETIF32:
		IOCTL_STRUCT_CONV_TO(BIOCSETIF, ifreq);
	case SIOCPHASE132:
		IOCTL_STRUCT_CONV_TO(SIOCPHASE1, ifreq);
	case SIOCPHASE232:
		IOCTL_STRUCT_CONV_TO(SIOCPHASE2, ifreq);
#endif

	case OOSIOCGIFCONF32:
		IOCTL_STRUCT_CONV_TO(OOSIOCGIFCONF, ifconf);
	case OSIOCGIFCONF32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFCONF, ifconf);
	case SIOCGIFCONF32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFCONF, ifconf);

	case SIOCGIFFLAGS32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFFLAGS, ifreq);
	case SIOCSIFFLAGS32:
		IOCTL_STRUCT_CONV_TO(SIOCSIFFLAGS, ifreq);

	case OSIOCGIFFLAGS32:
		IOCTL_STRUCT_CONV_TO(OSIOCGIFFLAGS, oifreq);
	case OSIOCSIFFLAGS32:
		IOCTL_STRUCT_CONV_TO(OSIOCSIFFLAGS, oifreq);

	case SIOCGIFMEDIA32:
		IOCTL_STRUCT_CONV_TO(SIOCGIFMEDIA, ifmediareq);

	case SIOCSDRVSPEC32:
		IOCTL_STRUCT_CONV_TO(SIOCSDRVSPEC, ifdrv);

	case SIOCGETVIFCNT32:
		IOCTL_STRUCT_CONV_TO(SIOCGETVIFCNT, sioc_vif_req);

	case SIOCGETSGCNT32:
		IOCTL_STRUCT_CONV_TO(SIOCGETSGCNT, sioc_sg_req);

	case VNDIOCSET32:
		IOCTL_STRUCT_CONV_TO(VNDIOCSET, vnd_ioctl);

	case VNDIOCCLR32:
		IOCTL_STRUCT_CONV_TO(VNDIOCCLR, vnd_ioctl);

	case VNDIOCGET32:
		IOCTL_STRUCT_CONV_TO(VNDIOCGET, vnd_user);

	case VNDIOCSET5032:
		IOCTL_STRUCT_CONV_TO(VNDIOCSET50, vnd_ioctl50);

	case VNDIOCCLR5032:
		IOCTL_STRUCT_CONV_TO(VNDIOCCLR50, vnd_ioctl50);

	default:
#ifdef NETBSD32_MD_IOCTL
		error = netbsd32_md_ioctl(fp, com, data32, l);
#else
		error = (*fp->f_ops->fo_ioctl)(fp, com, data32);
#endif
		break;
	}

	if (error == EPASSTHROUGH)
		error = ENOTTY;

	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (error == 0 && (com&IOC_OUT) && size32) {
		error = copyout(data32, SCARG_P32(uap, data), size32);
		ktrgenio(fd, UIO_READ, SCARG_P32(uap, data),
		    size32, error);
	}

	/* If we allocated data, free it here. */
	if (memp32)
		kmem_free(memp32, (size_t)size32);
	if (memp)
		kmem_free(memp, (size_t)size);
 out:
	fd_putfile(fd);
	return (error);
}
