/*	$NetBSD: netbsd32_ioctl.c,v 1.3.8.1 1999/12/27 18:34:29 wrstuden Exp $	*/

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

void
netbsd32_to_fbcmap(s32p, p)
	struct netbsd32_fbcmap *s32p;
	struct fbcmap *p;
{

	p->index = s32p->index;
	p->count = s32p->count;
	p->red = (u_char *)(u_long)s32p->red;
	p->green = (u_char *)(u_long)s32p->green;
	p->blue = (u_char *)(u_long)s32p->blue;
}

void
netbsd32_to_fbcursor(s32p, p)
	struct netbsd32_fbcursor *s32p;
	struct fbcursor *p;
{

	p->set = s32p->set;
	p->enable = s32p->enable;
	p->pos = s32p->pos;
	p->hot = s32p->hot;
	netbsd32_to_fbcmap(&s32p->cmap, &p->cmap);
	p->size = s32p->size;
	p->image = (char *)(u_long)s32p->image;
	p->mask = (char *)(u_long)s32p->mask;
}

void
netbsd32_to_opiocdesc(s32p, p)
	struct netbsd32_opiocdesc *s32p;
	struct opiocdesc *p;
{

	p->op_nodeid = s32p->op_nodeid;
	p->op_namelen = s32p->op_namelen;
	p->op_name = (char *)(u_long)s32p->op_name;
	p->op_buflen = s32p->op_buflen;
	p->op_buf = (char *)(u_long)s32p->op_buf;
}

void
netbsd32_to_partinfo(s32p, p)
	struct netbsd32_partinfo *s32p;
	struct partinfo *p;
{

	p->disklab = (struct disklabel *)(u_long)s32p->disklab;
	p->part = (struct partition *)(u_long)s32p->part;
}

void
netbsd32_to_format_op(s32p, p)
	struct netbsd32_format_op *s32p;
	struct format_op *p;
{

	p->df_buf = (char *)(u_long)s32p->df_buf;
	p->df_count = s32p->df_count;
	p->df_startblk = s32p->df_startblk;
	memcpy(p->df_reg, s32p->df_reg, sizeof(s32p->df_reg));
}

#if 0 /* XXX see below */
void
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

void
netbsd32_to_ifconf(s32p, p)
	struct netbsd32_ifconf *s32p;
	struct ifconf *p;
{

	p->ifc_len = s32p->ifc_len;
	/* ifc_buf & ifc_req are the same size so this works */
	p->ifc_buf = (caddr_t)(u_long)s32p->ifc_buf;
}

void
netbsd32_to_ifmediareq(s32p, p)
	struct netbsd32_ifmediareq *s32p;
	struct ifmediareq *p;
{

	memcpy(p, s32p, sizeof *s32p);
	p->ifm_ulist = (int *)(u_long)s32p->ifm_ulist;
}

void
netbsd32_to_ifdrv(s32p, p)
	struct netbsd32_ifdrv *s32p;
	struct ifdrv *p;
{

	memcpy(p, s32p, sizeof *s32p);
	p->ifd_data = (void *)(u_long)s32p->ifd_data;
}

void
netbsd32_to_sioc_vif_req(s32p, p)
	struct netbsd32_sioc_vif_req *s32p;
	struct sioc_vif_req *p;
{

	p->vifi = s32p->vifi;
	p->icount = (u_long)s32p->icount;
	p->ocount = (u_long)s32p->ocount;
	p->ibytes = (u_long)s32p->ibytes;
	p->obytes = (u_long)s32p->obytes;
}

void
netbsd32_to_sioc_sg_req(s32p, p)
	struct netbsd32_sioc_sg_req *s32p;
	struct sioc_sg_req *p;
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

void
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

void
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

void
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

void
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
void
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

void
netbsd32_from_ifconf(p, s32p)
	struct ifconf *p;
	struct netbsd32_ifconf *s32p;
{

	s32p->ifc_len = p->ifc_len;
	/* ifc_buf & ifc_req are the same size so this works */
	s32p->ifc_buf = (netbsd32_caddr_t)(u_long)p->ifc_buf;
}

void
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

void
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

void
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

void
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
	struct sys_ioctl_args ua;
	void *data = NULL;
	int rv;

	/*
	 * we need to translate some commands (_IOW) before calling sys_ioctl,
	 * some after (_IOR), and some both (_IOWR). 
	 */

/* we define some handy macros here... */
#define IOCTL_STRUCT_CONV_TO(type)	\
	data = malloc(sizeof(struct type), M_TEMP, M_WAITOK); \
	__CONCAT(netbsd32_to_, type)((struct __CONCAT(netbsd32_, type) *) \
	    (u_long)SCARG(uap, data), data)

#define IOCTL_STRUCT_CONV_CMD_TO(type, cmd)	\
	data = malloc(sizeof(struct type), M_TEMP, M_WAITOK); \
	__CONCAT(netbsd32_to_, type)((struct __CONCAT(netbsd32_, type) *) \
	    (u_long)SCARG(uap, data), data, cmd)

#define IOCTL_STRUCT_CONV_FROM(type)	\
	__CONCAT(netbsd32_from_, type)(data, \
	    (struct __CONCAT(netbsd32_, type) *) (u_long)SCARG(uap, data))

#define IOCTL_STRUCT_CONV_CMD_FROM(type, cmd)	\
	__CONCAT(netbsd32_from_, type)(data, \
	    (struct __CONCAT(netbsd32_, type) *) (u_long)SCARG(uap, data), cmd)

	/*
	 * convert various structures, pointers, and other objects that
	 * change size from 32 bit -> 64 bit, for all ioctl commands.
	 */
	switch (SCARG(uap, com)) {
	case FBIOPUTCMAP:
	case FBIOGETCMAP:
		IOCTL_STRUCT_CONV_TO(fbcmap);
		break;

	case FBIOSCURSOR:
	case FBIOGCURSOR:
		IOCTL_STRUCT_CONV_TO(fbcursor);
		break;

	case OPIOCGET:
	case OPIOCSET:
	case OPIOCNEXTPROP:
		IOCTL_STRUCT_CONV_TO(opiocdesc);
		break;

	case DIOCGPART:
		IOCTL_STRUCT_CONV_TO(partinfo);
		break;

	case DIOCRFORMAT:
	case DIOCWFORMAT:
		IOCTL_STRUCT_CONV_TO(format_op);
		break;

/*
 * only a few ifreq syscalls need conversion and those are
 * all driver specific... XXX
 */
#if 0
	case SIOCGADDRROM:
	case SIOCGCHIPID:
	case SIOCSIFADDR:
	case OSIOCGIFADDR:
	case SIOCGIFADDR:
	case SIOCSIFDSTADDR:
	case OSIOCGIFDSTADDR:
	case SIOCGIFDSTADDR:
	case SIOCSIFFLAGS:
	case SIOCGIFFLAGS:
	case OSIOCGIFBRDADDR:
	case SIOCGIFBRDADDR:
	case SIOCSIFBRDADDR:
	case OSIOCGIFNETMASK:
	case SIOCGIFNETMASK:
	case SIOCSIFNETMASK:
	case SIOCGIFMETRIC:
	case SIOCSIFMETRIC:
	case SIOCDIFADDR:
	case SIOCADDMULTI:
	case SIOCDELMULTI:
	case SIOCSIFMEDIA:
	case SIOCSIFMTU:
	case SIOCGIFMTU:
	case SIOCSIFASYNCMAP:
	case SIOCGIFASYNCMAP:
/*	case BIOCGETIF: READ ONLY */
	case BIOCSETIF:
	case SIOCPHASE1:
	case SIOCPHASE2:
		IOCTL_STRUCT_CONV_CMD_TO(ifreq, SCARG(uap, cmd));
		break;
#endif

	case OSIOCGIFCONF:
	case SIOCGIFCONF:
		IOCTL_STRUCT_CONV_TO(ifconf);
		break;

	case SIOCGIFMEDIA:
		IOCTL_STRUCT_CONV_TO(ifmediareq);
		break;

	case SIOCSDRVSPEC:
		IOCTL_STRUCT_CONV_TO(ifdrv);
		break;

	case SIOCGETVIFCNT:
		IOCTL_STRUCT_CONV_TO(sioc_vif_req);
		break;

	case SIOCGETSGCNT:
		IOCTL_STRUCT_CONV_TO(sioc_sg_req);
		break;

	}

	/*
	 * if we malloced a new data segment, plug it into the
	 * syscall args, otherwise copy incoming one as a void
	 * pointer.  also copy the rest of the syscall args...
	 */
	if (data)
		SCARG(&ua, data) = data;
	else
		NETBSD32TOP_UAP(data, void);
	NETBSD32TO64_UAP(fd);
	NETBSD32TOX_UAP(com, u_long);

	/* call the real ioctl */
	rv = sys_ioctl(p, &ua, retval);

	/*
	 * convert _back_ to 32 bit the results of the command.
	 */
	switch (SCARG(uap, com)) {
	case FBIOGETCMAP:
		IOCTL_STRUCT_CONV_FROM(fbcmap);
		break;

	case FBIOGCURSOR:
		IOCTL_STRUCT_CONV_FROM(fbcursor);
		break;

	case OPIOCGET:
	case OPIOCNEXTPROP:
		IOCTL_STRUCT_CONV_FROM(opiocdesc);
		break;

	case DIOCRFORMAT:
	case DIOCWFORMAT:
		IOCTL_STRUCT_CONV_FROM(format_op);
		break;

/*
 * only a few ifreq syscalls need conversion and those are
 * all driver specific... XXX
 */
#if 0
	case SIOCGADDRROM:
	case SIOCGCHIPID:
	case SIOCSIFADDR:
	case OSIOCGIFADDR:
	case SIOCGIFADDR:
	case SIOCSIFDSTADDR:
	case OSIOCGIFDSTADDR:
	case SIOCGIFDSTADDR:
	case SIOCSIFFLAGS:
	case SIOCGIFFLAGS:
	case OSIOCGIFBRDADDR:
	case SIOCGIFBRDADDR:
	case SIOCSIFBRDADDR:
	case OSIOCGIFNETMASK:
	case SIOCGIFNETMASK:
	case SIOCSIFNETMASK:
	case SIOCGIFMETRIC:
	case SIOCSIFMETRIC:
	case SIOCDIFADDR:
	case SIOCADDMULTI:
	case SIOCDELMULTI:
	case SIOCSIFMEDIA:
	case SIOCSIFMTU:
	case SIOCGIFMTU:
	case SIOCSIFASYNCMAP:
	case SIOCGIFASYNCMAP:
/*	case BIOCGETIF: READ ONLY */
	case BIOCSETIF:
	case SIOCPHASE1:
	case SIOCPHASE2:
		IOCTL_STRUCT_CONV_CMD_FROM(ifreq, SCARG(uap, cmd));
		break;
#endif

	case OSIOCGIFCONF:
	case SIOCGIFCONF:
		IOCTL_STRUCT_CONV_FROM(ifconf);
		break;

	case SIOCGIFMEDIA:
		IOCTL_STRUCT_CONV_FROM(ifmediareq);
		break;

	case SIOCSDRVSPEC:
		IOCTL_STRUCT_CONV_FROM(ifdrv);
		break;

	case SIOCGETVIFCNT:
		IOCTL_STRUCT_CONV_FROM(sioc_vif_req);
		break;

	case SIOCGETSGCNT:
		IOCTL_STRUCT_CONV_FROM(sioc_sg_req);
		break;
	}

	/* if we malloced data, free it here */
	if (data)
		free(data, M_TEMP);

	/* done! */
	return (rv);
}
