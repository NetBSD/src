/*	$NetBSD: bpf.c,v 1.55.2.1 2000/06/22 17:09:40 minoura Exp $	*/

/*
 * Copyright (c) 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.c) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence
 * Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)bpf.c	8.4 (Berkeley) 1/9/95
 * static char rcsid[] =
 * "Header: bpf.c,v 1.67 96/09/26 22:00:52 leres Exp ";
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/conf.h>
#include <sys/vnode.h>

#include <sys/file.h>
#include <sys/tty.h>
#include <sys/uio.h>

#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/poll.h>

#include <net/if.h>

#include <net/bpf.h>
#include <net/bpfdesc.h>

#include <net/if_arc.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/if_inarp.h>

#ifndef BPF_BUFSIZE
# define BPF_BUFSIZE 8192		/* 4096 too small for FDDI frames */
#endif

#define PRINET  26			/* interruptible */

/*
 * The default read buffer size is patchable.
 */
int bpf_bufsize = BPF_BUFSIZE;

/*
 *  bpf_iflist is the list of interfaces; each corresponds to an ifnet
 *  bpf_dtab holds the descriptors, indexed by minor device #
 */
struct bpf_if	*bpf_iflist;
struct bpf_d	bpf_dtab[NBPFILTER];

static int	bpf_allocbufs __P((struct bpf_d *));
static void	bpf_freed __P((struct bpf_d *));
static void	bpf_ifname __P((struct ifnet *, struct ifreq *));
static void	*bpf_mcpy __P((void *, const void *, size_t));
static int	bpf_movein __P((struct uio *, int, int,
			        struct mbuf **, struct sockaddr *));
static void	bpf_attachd __P((struct bpf_d *, struct bpf_if *));
static void	bpf_detachd __P((struct bpf_d *));
static int	bpf_setif __P((struct bpf_d *, struct ifreq *));
int		bpfpoll __P((dev_t, int, struct proc *));
static __inline void
		bpf_wakeup __P((struct bpf_d *));
static void	catchpacket __P((struct bpf_d *, u_char *, u_int, u_int,
				 void *(*)(void *, const void *, size_t)));
static void	reset_d __P((struct bpf_d *));

static int
bpf_movein(uio, linktype, mtu, mp, sockp)
	struct uio *uio;
	int linktype;
	int mtu;
	struct mbuf **mp;
	struct sockaddr *sockp;
{
	struct mbuf *m;
	int error;
	int len;
	int hlen;
	int align;

	/*
	 * Build a sockaddr based on the data link layer type.
	 * We do this at this level because the ethernet header
	 * is copied directly into the data field of the sockaddr.
	 * In the case of SLIP, there is no header and the packet
	 * is forwarded as is.
	 * Also, we are careful to leave room at the front of the mbuf
	 * for the link level header.
	 */
	switch (linktype) {

	case DLT_SLIP:
		sockp->sa_family = AF_INET;
		hlen = 0;
		align = 0;
		break;

	case DLT_PPP:
		sockp->sa_family = AF_UNSPEC;
		hlen = 0;
		align = 0;
		break;

	case DLT_EN10MB:
		sockp->sa_family = AF_UNSPEC;
		/* XXX Would MAXLINKHDR be better? */
 		/* 6(dst)+6(src)+2(type) */
		hlen = sizeof(struct ether_header);
		align = 2;
		break;

	case DLT_ARCNET:
		sockp->sa_family = AF_UNSPEC;
		hlen = ARC_HDRLEN;
		align = 5;
		break;

	case DLT_FDDI:
		sockp->sa_family = AF_LINK;
		/* XXX 4(FORMAC)+6(dst)+6(src) */
		hlen = 16;
		align = 0;
		break;

	case DLT_NULL:
		sockp->sa_family = AF_UNSPEC;
		hlen = 0;
		align = 0;
		break;

	default:
		return (EIO);
	}

	len = uio->uio_resid;
	/*
	 * If there aren't enough bytes for a link level header or the
	 * packet length exceeds the interface mtu, return an error.
	 */
	if (len < hlen || len - hlen > mtu)
		return (EMSGSIZE);

	/*
	 * XXX Avoid complicated buffer chaining ---
	 * bail if it won't fit in a single mbuf.
	 * (Take into account possible alignment bytes)
	 */
	if ((unsigned)len > MCLBYTES - align)
		return (EIO);

	MGETHDR(m, M_WAIT, MT_DATA);
	if (m == 0)
		return (ENOBUFS);
	m->m_pkthdr.rcvif = 0;
	m->m_pkthdr.len = len - hlen;
	if (len > MHLEN - align) {
		MCLGET(m, M_WAIT);
		if ((m->m_flags & M_EXT) == 0) {
			error = ENOBUFS;
			goto bad;
		}
	}

	/* Insure the data is properly aligned */
	if (align > 0) {
		m->m_data += align;
		m->m_len -= align;
	}

	error = uiomove(mtod(m, caddr_t), len, uio);
	if (error)
		goto bad;
	if (hlen != 0) {
		memcpy(sockp->sa_data, mtod(m, caddr_t), hlen);
		m->m_data += hlen; /* XXX */
		len -= hlen;
	}
	m->m_len = len;
	*mp = m;
	return (0);

bad:
	m_freem(m);
	return (error);
}

/*
 * Attach file to the bpf interface, i.e. make d listen on bp.
 * Must be called at splimp.
 */
static void
bpf_attachd(d, bp)
	struct bpf_d *d;
	struct bpf_if *bp;
{
	/*
	 * Point d at bp, and add d to the interface's list of listeners.
	 * Finally, point the driver's bpf cookie at the interface so
	 * it will divert packets to bpf.
	 */
	d->bd_bif = bp;
	d->bd_next = bp->bif_dlist;
	bp->bif_dlist = d;

	*bp->bif_driverp = bp;
}

/*
 * Detach a file from its interface.
 */
static void
bpf_detachd(d)
	struct bpf_d *d;
{
	struct bpf_d **p;
	struct bpf_if *bp;

	bp = d->bd_bif;
	/*
	 * Check if this descriptor had requested promiscuous mode.
	 * If so, turn it off.
	 */
	if (d->bd_promisc) {
		int error;

		d->bd_promisc = 0;
		/*
		 * Take device out of promiscuous mode.  Since we were
		 * able to enter promiscuous mode, we should be able
		 * to turn it off.  But we can get an error if
		 * the interface was configured down, so only panic
		 * if we don't get an unexpected error.
		 */
  		error = ifpromisc(bp->bif_ifp, 0);
		if (error && error != EINVAL)
			panic("bpf: ifpromisc failed");
	}
	/* Remove d from the interface's descriptor list. */
	p = &bp->bif_dlist;
	while (*p != d) {
		p = &(*p)->bd_next;
		if (*p == 0)
			panic("bpf_detachd: descriptor not in list");
	}
	*p = (*p)->bd_next;
	if (bp->bif_dlist == 0)
		/*
		 * Let the driver know that there are no more listeners.
		 */
		*d->bd_bif->bif_driverp = 0;
	d->bd_bif = 0;
}


/*
 * Mark a descriptor free by making it point to itself.
 * This is probably cheaper than marking with a constant since
 * the address should be in a register anyway.
 */
#define D_ISFREE(d) ((d) == (d)->bd_next)
#define D_MARKFREE(d) ((d)->bd_next = (d))
#define D_MARKUSED(d) ((d)->bd_next = 0)

/*
 * bpfilterattach() is called at boot time.
 */
/* ARGSUSED */
void
bpfilterattach(n)
	int n;
{
	int i;
	/*
	 * Mark all the descriptors free.
	 */
	for (i = 0; i < NBPFILTER; ++i)
		D_MARKFREE(&bpf_dtab[i]);

}

/*
 * Open ethernet device.  Returns ENXIO for illegal minor device number,
 * EBUSY if file is open by another process.
 */
/* ARGSUSED */
int
bpfopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct bpf_d *d;

	if (minor(dev) >= NBPFILTER)
		return (ENXIO);
	/*
	 * Each minor can be opened by only one process.  If the requested
	 * minor is in use, return EBUSY.
	 */
	d = &bpf_dtab[minor(dev)];
	if (!D_ISFREE(d))
		return (EBUSY);

	/* Mark "free" and do most initialization. */
	memset((char *)d, 0, sizeof(*d));
	d->bd_bufsize = bpf_bufsize;

	return (0);
}

/*
 * Close the descriptor by detaching it from its interface,
 * deallocating its buffers, and marking it free.
 */
/* ARGSUSED */
int
bpfclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	struct bpf_d *d = &bpf_dtab[minor(dev)];
	int s;

	s = splimp();
	if (d->bd_bif)
		bpf_detachd(d);
	splx(s);
	bpf_freed(d);

	return (0);
}

/*
 * Rotate the packet buffers in descriptor d.  Move the store buffer
 * into the hold slot, and the free buffer into the store slot.
 * Zero the length of the new store buffer.
 */
#define ROTATE_BUFFERS(d) \
	(d)->bd_hbuf = (d)->bd_sbuf; \
	(d)->bd_hlen = (d)->bd_slen; \
	(d)->bd_sbuf = (d)->bd_fbuf; \
	(d)->bd_slen = 0; \
	(d)->bd_fbuf = 0;
/*
 *  bpfread - read next chunk of packets from buffers
 */
int
bpfread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	struct bpf_d *d = &bpf_dtab[minor(dev)];
	int error;
	int s;

	/*
	 * Restrict application to use a buffer the same size as
	 * as kernel buffers.
	 */
	if (uio->uio_resid != d->bd_bufsize)
		return (EINVAL);

	s = splimp();
	/*
	 * If the hold buffer is empty, then do a timed sleep, which
	 * ends when the timeout expires or when enough packets
	 * have arrived to fill the store buffer.
	 */
	while (d->bd_hbuf == 0) {
		if (d->bd_immediate) {
			if (d->bd_slen == 0) {
				splx(s);
				return (EWOULDBLOCK);
			}
			/*
			 * A packet(s) either arrived since the previous
			 * read or arrived while we were asleep.
			 * Rotate the buffers and return what's here.
			 */
			ROTATE_BUFFERS(d);
			break;
		}
		if (d->bd_rtout != -1)
			error = tsleep((caddr_t)d, PRINET|PCATCH, "bpf",
					  d->bd_rtout);
		else
			error = EWOULDBLOCK; /* User requested non-blocking I/O */
		if (error == EINTR || error == ERESTART) {
			splx(s);
			return (error);
		}
		if (error == EWOULDBLOCK) {
			/*
			 * On a timeout, return what's in the buffer,
			 * which may be nothing.  If there is something
			 * in the store buffer, we can rotate the buffers.
			 */
			if (d->bd_hbuf)
				/*
				 * We filled up the buffer in between
				 * getting the timeout and arriving
				 * here, so we don't need to rotate.
				 */
				break;

			if (d->bd_slen == 0) {
				splx(s);
				return (0);
			}
			ROTATE_BUFFERS(d);
			break;
		}
		if (error != 0)
			goto done;
	}
	/*
	 * At this point, we know we have something in the hold slot.
	 */
	splx(s);

	/*
	 * Move data from hold buffer into user space.
	 * We know the entire buffer is transferred since
	 * we checked above that the read buffer is bpf_bufsize bytes.
	 */
	error = uiomove(d->bd_hbuf, d->bd_hlen, uio);

	s = splimp();
	d->bd_fbuf = d->bd_hbuf;
	d->bd_hbuf = 0;
	d->bd_hlen = 0;
done:
	splx(s);
	return (error);
}


/*
 * If there are processes sleeping on this descriptor, wake them up.
 */
static __inline void
bpf_wakeup(d)
	struct bpf_d *d;
{
	struct proc *p;

	wakeup((caddr_t)d);
	if (d->bd_async) {
		if (d->bd_pgid > 0)
			gsignal (d->bd_pgid, SIGIO);
		else if (d->bd_pgid && (p = pfind (-d->bd_pgid)) != NULL)
			psignal (p, SIGIO);
	}

	selwakeup(&d->bd_sel);
	/* XXX */
	d->bd_sel.si_pid = 0;
}

int
bpfwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	struct bpf_d *d = &bpf_dtab[minor(dev)];
	struct ifnet *ifp;
	struct mbuf *m;
	int error, s;
	static struct sockaddr_storage dst;

	if (d->bd_bif == 0)
		return (ENXIO);

	ifp = d->bd_bif->bif_ifp;

	if (uio->uio_resid == 0)
		return (0);

	error = bpf_movein(uio, (int)d->bd_bif->bif_dlt, ifp->if_mtu, &m,
		(struct sockaddr *) &dst);
	if (error)
		return (error);

	if (m->m_pkthdr.len > ifp->if_mtu)
		return (EMSGSIZE);

	if (d->bd_hdrcmplt)
		dst.ss_family = pseudo_AF_HDRCMPLT;

	s = splsoftnet();
	error = (*ifp->if_output)(ifp, m, (struct sockaddr *) &dst, NULL);
	splx(s);
	/*
	 * The driver frees the mbuf.
	 */
	return (error);
}

/*
 * Reset a descriptor by flushing its packet buffer and clearing the
 * receive and drop counts.  Should be called at splimp.
 */
static void
reset_d(d)
	struct bpf_d *d;
{
	if (d->bd_hbuf) {
		/* Free the hold buffer. */
		d->bd_fbuf = d->bd_hbuf;
		d->bd_hbuf = 0;
	}
	d->bd_slen = 0;
	d->bd_hlen = 0;
	d->bd_rcount = 0;
	d->bd_dcount = 0;
}

#ifdef BPF_KERN_FILTER
extern struct bpf_insn *bpf_tcp_filter;
extern struct bpf_insn *bpf_udp_filter;
#endif

/*
 *  FIONREAD		Check for read packet available.
 *  BIOCGBLEN		Get buffer len [for read()].
 *  BIOCSETF		Set ethernet read filter.
 *  BIOCFLUSH		Flush read packet buffer.
 *  BIOCPROMISC		Put interface into promiscuous mode.
 *  BIOCGDLT		Get link layer type.
 *  BIOCGETIF		Get interface name.
 *  BIOCSETIF		Set interface.
 *  BIOCSRTIMEOUT	Set read timeout.
 *  BIOCGRTIMEOUT	Get read timeout.
 *  BIOCGSTATS		Get packet stats.
 *  BIOCIMMEDIATE	Set immediate mode.
 *  BIOCVERSION		Get filter language version.
 *  BIOGHDRCMPLT	Get "header already complete" flag.
 *  BIOSHDRCMPLT	Set "header already complete" flag.
 */
/* ARGSUSED */
int
bpfioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct bpf_d *d = &bpf_dtab[minor(dev)];
	int s, error = 0;
#ifdef BPF_KERN_FILTER
	struct bpf_insn **p;
#endif

	switch (cmd) {

	default:
		error = EINVAL;
		break;

	/*
	 * Check for read packet available.
	 */
	case FIONREAD:
		{
			int n;

			s = splimp();
			n = d->bd_slen;
			if (d->bd_hbuf)
				n += d->bd_hlen;
			splx(s);

			*(int *)addr = n;
			break;
		}

	/*
	 * Get buffer len [for read()].
	 */
	case BIOCGBLEN:
		*(u_int *)addr = d->bd_bufsize;
		break;

	/*
	 * Set buffer length.
	 */
	case BIOCSBLEN:
		if (d->bd_bif != 0)
			error = EINVAL;
		else {
			u_int size = *(u_int *)addr;

			if (size > BPF_MAXBUFSIZE)
				*(u_int *)addr = size = BPF_MAXBUFSIZE;
			else if (size < BPF_MINBUFSIZE)
				*(u_int *)addr = size = BPF_MINBUFSIZE;
			d->bd_bufsize = size;
		}
		break;

	/*
	 * Set link layer read filter.
	 */
	case BIOCSETF:
		error = bpf_setf(d, (struct bpf_program *)addr);
		break;

#ifdef BPF_KERN_FILTER
	/*
	 * Set TCP or UDP reject filter.
	 */
	case BIOCSTCPF:
	case BIOCSUDPF:
		if (!suser()) {
			error = EPERM;
			break;
		}

		/* Validate and store filter */
		error = bpf_setf(d, (struct bpf_program *)addr);

		/* Free possible old filter */
		if (cmd == BIOCSTCPF)
			p = &bpf_tcp_filter;
		else
			p = &bpf_udp_filter;
		if (*p != NULL)
			free((caddr_t)*p, M_DEVBUF);

		/* Steal new filter (noop if error) */
		s = splimp();
		*p = d->bd_filter;
		d->bd_filter = NULL;
		splx(s);
		break;
#endif

	/*
	 * Flush read packet buffer.
	 */
	case BIOCFLUSH:
		s = splimp();
		reset_d(d);
		splx(s);
		break;

	/*
	 * Put interface into promiscuous mode.
	 */
	case BIOCPROMISC:
		if (d->bd_bif == 0) {
			/*
			 * No interface attached yet.
			 */
			error = EINVAL;
			break;
		}
		s = splimp();
		if (d->bd_promisc == 0) {
			error = ifpromisc(d->bd_bif->bif_ifp, 1);
			if (error == 0)
				d->bd_promisc = 1;
		}
		splx(s);
		break;

	/*
	 * Get device parameters.
	 */
	case BIOCGDLT:
		if (d->bd_bif == 0)
			error = EINVAL;
		else
			*(u_int *)addr = d->bd_bif->bif_dlt;
		break;

	/*
	 * Set interface name.
	 */
	case BIOCGETIF:
		if (d->bd_bif == 0)
			error = EINVAL;
		else
			bpf_ifname(d->bd_bif->bif_ifp, (struct ifreq *)addr);
		break;

	/*
	 * Set interface.
	 */
	case BIOCSETIF:
		error = bpf_setif(d, (struct ifreq *)addr);
		break;

	/*
	 * Set read timeout.
	 */
	case BIOCSRTIMEOUT:
		{
			struct timeval *tv = (struct timeval *)addr;

			/* Compute number of ticks. */
			d->bd_rtout = tv->tv_sec * hz + tv->tv_usec / tick;
			if ((d->bd_rtout == 0) && (tv->tv_usec != 0))
				d->bd_rtout = 1;
			break;
		}

	/*
	 * Get read timeout.
	 */
	case BIOCGRTIMEOUT:
		{
			struct timeval *tv = (struct timeval *)addr;

			tv->tv_sec = d->bd_rtout / hz;
			tv->tv_usec = (d->bd_rtout % hz) * tick;
			break;
		}

	/*
	 * Get packet stats.
	 */
	case BIOCGSTATS:
		{
			struct bpf_stat *bs = (struct bpf_stat *)addr;

			bs->bs_recv = d->bd_rcount;
			bs->bs_drop = d->bd_dcount;
			break;
		}

	/*
	 * Set immediate mode.
	 */
	case BIOCIMMEDIATE:
		d->bd_immediate = *(u_int *)addr;
		break;

	case BIOCVERSION:
		{
			struct bpf_version *bv = (struct bpf_version *)addr;

			bv->bv_major = BPF_MAJOR_VERSION;
			bv->bv_minor = BPF_MINOR_VERSION;
			break;
		}

	case BIOCGHDRCMPLT:	/* get "header already complete" flag */
		*(u_int *)addr = d->bd_hdrcmplt;
		break;

	case BIOCSHDRCMPLT:	/* set "header already complete" flag */
		d->bd_hdrcmplt = *(u_int *)addr ? 1 : 0;
		break;

	case FIONBIO:		/* Non-blocking I/O */
		if (*(int *)addr)
			d->bd_rtout = -1;
		else
			d->bd_rtout = 0;
		break;

	case FIOASYNC:		/* Send signal on receive packets */
		d->bd_async = *(int *)addr;
		break;

	/*
	 * N.B.  ioctl (FIOSETOWN) and fcntl (F_SETOWN) both end up doing
	 * the equivalent of a TIOCSPGRP and hence end up here.  *However*
	 * TIOCSPGRP's arg is a process group if it's positive and a process
	 * id if it's negative.  This is exactly the opposite of what the
	 * other two functions want!  Therefore there is code in ioctl and
	 * fcntl to negate the arg before calling here.
	 */
	case TIOCSPGRP:		/* Process or group to send signals to */
		d->bd_pgid = *(int *)addr;
		break;

	case TIOCGPGRP:
		*(int *)addr = d->bd_pgid;
		break;
	}
	return (error);
}

/*
 * Set d's packet filter program to fp.  If this file already has a filter,
 * free it and replace it.  Returns EINVAL for bogus requests.
 */
int
bpf_setf(d, fp)
	struct bpf_d *d;
	struct bpf_program *fp;
{
	struct bpf_insn *fcode, *old;
	u_int flen, size;
	int s;

	old = d->bd_filter;
	if (fp->bf_insns == 0) {
		if (fp->bf_len != 0)
			return (EINVAL);
		s = splimp();
		d->bd_filter = 0;
		reset_d(d);
		splx(s);
		if (old != 0)
			free((caddr_t)old, M_DEVBUF);
		return (0);
	}
	flen = fp->bf_len;
	if (flen > BPF_MAXINSNS)
		return (EINVAL);

	size = flen * sizeof(*fp->bf_insns);
	fcode = (struct bpf_insn *)malloc(size, M_DEVBUF, M_WAITOK);
	if (copyin((caddr_t)fp->bf_insns, (caddr_t)fcode, size) == 0 &&
	    bpf_validate(fcode, (int)flen)) {
		s = splimp();
		d->bd_filter = fcode;
		reset_d(d);
		splx(s);
		if (old != 0)
			free((caddr_t)old, M_DEVBUF);

		return (0);
	}
	free((caddr_t)fcode, M_DEVBUF);
	return (EINVAL);
}

/*
 * Detach a file from its current interface (if attached at all) and attach
 * to the interface indicated by the name stored in ifr.
 * Return an errno or 0.
 */
static int
bpf_setif(d, ifr)
	struct bpf_d *d;
	struct ifreq *ifr;
{
	struct bpf_if *bp;
	char *cp;
	int unit_seen, i, s, error;

	/*
	 * Make sure the provided name has a unit number, and default
	 * it to '0' if not specified.
	 * XXX This is ugly ... do this differently?
	 */
	unit_seen = 0;
	cp = ifr->ifr_name;
	cp[sizeof(ifr->ifr_name) - 1] = '\0';	/* sanity */
	while (*cp++)
		if (*cp >= '0' && *cp <= '9')
			unit_seen = 1;
	if (!unit_seen) {
		/* Make sure to leave room for the '\0'. */
		for (i = 0; i < (IFNAMSIZ - 1); ++i) {
			if ((ifr->ifr_name[i] >= 'a' &&
			     ifr->ifr_name[i] <= 'z') ||
			    (ifr->ifr_name[i] >= 'A' &&
			     ifr->ifr_name[i] <= 'Z'))
				continue;
			ifr->ifr_name[i] = '0';
		}
	}

	/*
	 * Look through attached interfaces for the named one.
	 */
	for (bp = bpf_iflist; bp != 0; bp = bp->bif_next) {
		struct ifnet *ifp = bp->bif_ifp;

		if (ifp == 0 ||
		    strcmp(ifp->if_xname, ifr->ifr_name) != 0)
			continue;
		/*
		 * We found the requested interface.
		 * If it's not up, return an error.
		 * Allocate the packet buffers if we need to.
		 * If we're already attached to requested interface,
		 * just flush the buffer.
		 */
		if ((ifp->if_flags & IFF_UP) == 0)
			return (ENETDOWN);

		if (d->bd_sbuf == 0) {
			error = bpf_allocbufs(d);
			if (error != 0)
				return (error);
		}
		s = splimp();
		if (bp != d->bd_bif) {
			if (d->bd_bif)
				/*
				 * Detach if attached to something else.
				 */
				bpf_detachd(d);

			bpf_attachd(d, bp);
		}
		reset_d(d);
		splx(s);
		return (0);
	}
	/* Not found. */
	return (ENXIO);
}

/*
 * Copy the interface name to the ifreq.
 */
static void
bpf_ifname(ifp, ifr)
	struct ifnet *ifp;
	struct ifreq *ifr;
{

	memcpy(ifr->ifr_name, ifp->if_xname, IFNAMSIZ);
}

/*
 * Support for poll() system call
 *
 * Return true iff the specific operation will not block indefinitely.
 * Otherwise, return false but make a note that a selwakeup() must be done.
 */
int
bpfpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct bpf_d *d = &bpf_dtab[minor(dev)];
	int revents = 0;
	int s = splimp();

	/*
	 * An imitation of the FIONREAD ioctl code.
	 */
	if (events & (POLLIN | POLLRDNORM)) {
		if (d->bd_hlen != 0 || (d->bd_immediate && d->bd_slen != 0))
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &d->bd_sel);
	}

	splx(s);
	return (revents);
}

/*
 * Incoming linkage from device drivers.  Process the packet pkt, of length
 * pktlen, which is stored in a contiguous buffer.  The packet is parsed
 * by each process' filter, and if accepted, stashed into the corresponding
 * buffer.
 */
void
bpf_tap(arg, pkt, pktlen)
	caddr_t arg;
	u_char *pkt;
	u_int pktlen;
{
	struct bpf_if *bp;
	struct bpf_d *d;
	u_int slen;
	/*
	 * Note that the ipl does not have to be raised at this point.
	 * The only problem that could arise here is that if two different
	 * interfaces shared any data.  This is not the case.
	 */
	bp = (struct bpf_if *)arg;
	for (d = bp->bif_dlist; d != 0; d = d->bd_next) {
		++d->bd_rcount;
		slen = bpf_filter(d->bd_filter, pkt, pktlen, pktlen);
		if (slen != 0)
			catchpacket(d, pkt, pktlen, slen, memcpy);
	}
}

/*
 * Copy data from an mbuf chain into a buffer.  This code is derived
 * from m_copydata in sys/uipc_mbuf.c.
 */
static void *
bpf_mcpy(dst_arg, src_arg, len)
	void *dst_arg;
	const void *src_arg;
	size_t len;
{
	const struct mbuf *m;
	u_int count;
	u_char *dst;

	m = src_arg;
	dst = dst_arg;
	while (len > 0) {
		if (m == 0)
			panic("bpf_mcpy");
		count = min(m->m_len, len);
		memcpy((caddr_t)dst, mtod(m, caddr_t), count);
		m = m->m_next;
		dst += count;
		len -= count;
	}
	return(dst_arg);
}

/*
 * Incoming linkage from device drivers, when packet is in an mbuf chain.
 */
void
bpf_mtap(arg, m)
	caddr_t arg;
	struct mbuf *m;
{
	struct bpf_if *bp = (struct bpf_if *)arg;
	struct bpf_d *d;
	u_int pktlen, slen;
	struct mbuf *m0;

	pktlen = 0;
	for (m0 = m; m0 != 0; m0 = m0->m_next)
		pktlen += m0->m_len;

	for (d = bp->bif_dlist; d != 0; d = d->bd_next) {
		++d->bd_rcount;
		slen = bpf_filter(d->bd_filter, (u_char *)m, pktlen, 0);
		if (slen != 0)
			catchpacket(d, (u_char *)m, pktlen, slen, bpf_mcpy);
	}
}

/*
 * Move the packet data from interface memory (pkt) into the
 * store buffer.  Return 1 if it's time to wakeup a listener (buffer full),
 * otherwise 0.  "copy" is the routine called to do the actual data
 * transfer.  memcpy is passed in to copy contiguous chunks, while
 * bpf_mcpy is passed in to copy mbuf chains.  In the latter case,
 * pkt is really an mbuf.
 */
static void
catchpacket(d, pkt, pktlen, snaplen, cpfn)
	struct bpf_d *d;
	u_char *pkt;
	u_int pktlen, snaplen;
	void *(*cpfn) __P((void *, const void *, size_t));
{
	struct bpf_hdr *hp;
	int totlen, curlen;
	int hdrlen = d->bd_bif->bif_hdrlen;
	/*
	 * Figure out how many bytes to move.  If the packet is
	 * greater or equal to the snapshot length, transfer that
	 * much.  Otherwise, transfer the whole packet (unless
	 * we hit the buffer size limit).
	 */
	totlen = hdrlen + min(snaplen, pktlen);
	if (totlen > d->bd_bufsize)
		totlen = d->bd_bufsize;

	/*
	 * Round up the end of the previous packet to the next longword.
	 */
	curlen = BPF_WORDALIGN(d->bd_slen);
	if (curlen + totlen > d->bd_bufsize) {
		/*
		 * This packet will overflow the storage buffer.
		 * Rotate the buffers if we can, then wakeup any
		 * pending reads.
		 */
		if (d->bd_fbuf == 0) {
			/*
			 * We haven't completed the previous read yet,
			 * so drop the packet.
			 */
			++d->bd_dcount;
			return;
		}
		ROTATE_BUFFERS(d);
		bpf_wakeup(d);
		curlen = 0;
	}
	else if (d->bd_immediate)
		/*
		 * Immediate mode is set.  A packet arrived so any
		 * reads should be woken up.
		 */
		bpf_wakeup(d);

	/*
	 * Append the bpf header.
	 */
	hp = (struct bpf_hdr *)(d->bd_sbuf + curlen);
	microtime(&hp->bh_tstamp);
	hp->bh_datalen = pktlen;
	hp->bh_hdrlen = hdrlen;
	/*
	 * Copy the packet data into the store buffer and update its length.
	 */
	(*cpfn)((u_char *)hp + hdrlen, pkt, (hp->bh_caplen = totlen - hdrlen));
	d->bd_slen = curlen + totlen;
}

/*
 * Initialize all nonzero fields of a descriptor.
 */
static int
bpf_allocbufs(d)
	struct bpf_d *d;
{

	d->bd_fbuf = (caddr_t)malloc(d->bd_bufsize, M_DEVBUF, M_WAITOK);
	d->bd_sbuf = (caddr_t)malloc(d->bd_bufsize, M_DEVBUF, M_WAITOK);
	d->bd_slen = 0;
	d->bd_hlen = 0;
	return (0);
}

/*
 * Free buffers currently in use by a descriptor.
 * Called on close.
 */
static void
bpf_freed(d)
	struct bpf_d *d;
{
	/*
	 * We don't need to lock out interrupts since this descriptor has
	 * been detached from its interface and it yet hasn't been marked
	 * free.
	 */
	if (d->bd_sbuf != 0) {
		free(d->bd_sbuf, M_DEVBUF);
		if (d->bd_hbuf != 0)
			free(d->bd_hbuf, M_DEVBUF);
		if (d->bd_fbuf != 0)
			free(d->bd_fbuf, M_DEVBUF);
	}
	if (d->bd_filter)
		free((caddr_t)d->bd_filter, M_DEVBUF);

	D_MARKFREE(d);
}

/*
 * Attach an interface to bpf.  driverp is a pointer to a (struct bpf_if *)
 * in the driver's softc; dlt is the link layer type; hdrlen is the fixed
 * size of the link header (variable length headers not yet supported).
 */
void
bpfattach(driverp, ifp, dlt, hdrlen)
	caddr_t *driverp;
	struct ifnet *ifp;
	u_int dlt, hdrlen;
{
	struct bpf_if *bp;
	bp = (struct bpf_if *)malloc(sizeof(*bp), M_DEVBUF, M_DONTWAIT);
	if (bp == 0)
		panic("bpfattach");

	bp->bif_dlist = 0;
	bp->bif_driverp = (struct bpf_if **)driverp;
	bp->bif_ifp = ifp;
	bp->bif_dlt = dlt;

	bp->bif_next = bpf_iflist;
	bpf_iflist = bp;

	*bp->bif_driverp = 0;

	/*
	 * Compute the length of the bpf header.  This is not necessarily
	 * equal to SIZEOF_BPF_HDR because we want to insert spacing such
	 * that the network layer header begins on a longword boundary (for
	 * performance reasons and to alleviate alignment restrictions).
	 */
	bp->bif_hdrlen = BPF_WORDALIGN(hdrlen + SIZEOF_BPF_HDR) - hdrlen;

#if 0
	printf("bpf: %s attached\n", ifp->if_xname);
#endif
}

/*
 * Remove an interface from bpf.
 */
void
bpfdetach(ifp)
	struct ifnet *ifp;
{
	struct bpf_if *bp, **pbp;
	struct bpf_d *d;
	int i, s, cmaj;

	/* locate the major number */
	for (cmaj = 0; cmaj <= nchrdev; cmaj++)
		if (cdevsw[cmaj].d_open == bpfopen)
			break;

	/* Nuke the vnodes for any open instances */
	for (i = 0; i < NBPFILTER; ++i) {
		d = &bpf_dtab[i];
		if (!D_ISFREE(d) && d->bd_bif != NULL &&
		    d->bd_bif->bif_ifp == ifp) {
			/*
			 * Detach the descriptor from an interface now.
			 * It will be free'ed later by close routine.
			 */
			s = splimp();
			d->bd_promisc = 0;	/* we can't touch device. */
			bpf_detachd(d);
			splx(s);
			vdevgone(cmaj, i, i, VCHR);
		}
	}

	for (bp = bpf_iflist, pbp = &bpf_iflist;
	     bp != NULL; pbp = &bp->bif_next, bp = bp->bif_next) {
		if (bp->bif_ifp == ifp) {
			*pbp = bp->bif_next;
			free(bp, M_DEVBUF);
			break;
		}
	}
}

/*
 * Change the data link type of a BPF instance.
 */
void
bpf_change_type(driverp, dlt, hdrlen)
	caddr_t *driverp;
	u_int dlt, hdrlen;
{
	struct bpf_if *bp;

	for (bp = bpf_iflist; bp != NULL; bp = bp->bif_next) {
		if (bp->bif_driverp == (struct bpf_if **)driverp)
			break;
	}
	if (bp == NULL)
		panic("bpf_change_type");

	bp->bif_dlt = dlt;

	/*
	 * Compute the length of the bpf header.  This is not necessarily
	 * equal to SIZEOF_BPF_HDR because we want to insert spacing such
	 * that the network layer header begins on a longword boundary (for
	 * performance reasons and to alleviate alignment restrictions).
	 */
	bp->bif_hdrlen = BPF_WORDALIGN(hdrlen + SIZEOF_BPF_HDR) - hdrlen;
}

/* XXX This routine belongs in net/if.c. */
/*
 * Set/clear promiscuous mode on interface ifp based on the truth value
 * of pswitch.  The calls are reference counted so that only the first
 * "on" request actually has an effect, as does the final "off" request.
 * Results are undefined if the "off" and "on" requests are not matched.
 */
int
ifpromisc(ifp, pswitch)
	struct ifnet *ifp;
	int pswitch;
{
	int pcount, ret;
	short flags;
	struct ifreq ifr;

	pcount = ifp->if_pcount;
	flags = ifp->if_flags;
	if (pswitch) {
		/*
		 * If the device is not configured up, we cannot put it in
		 * promiscuous mode.
		 */
		if ((ifp->if_flags & IFF_UP) == 0)
			return (ENETDOWN);
		if (ifp->if_pcount++ != 0)
			return (0);
		ifp->if_flags |= IFF_PROMISC;
	} else {
		if (--ifp->if_pcount > 0)
			return (0);
		ifp->if_flags &= ~IFF_PROMISC;
		/*
		 * If the device is not configured up, we should not need to
		 * turn off promiscuous mode (device should have turned it
		 * off when interface went down; and will look at IFF_PROMISC
		 * again next time interface comes up).
		 */
		if ((ifp->if_flags & IFF_UP) == 0)
			return (0);
	}
	memset((caddr_t)&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = ifp->if_flags;
	ret = (*ifp->if_ioctl)(ifp, SIOCSIFFLAGS, (caddr_t)&ifr);
	/* Restore interface state if not successful */
	if (ret != 0) {
		ifp->if_pcount = pcount;
		ifp->if_flags = flags;
	}
	return (ret);
}
