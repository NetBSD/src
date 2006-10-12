/*	$NetBSD: bpf.c,v 1.123 2006/10/12 01:32:27 christos Exp $	*/

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
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bpf.c,v 1.123 2006/10/12 01:32:27 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/vnode.h>
#include <sys/queue.h>

#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/tty.h>
#include <sys/uio.h>

#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/poll.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>

#include <net/if.h>
#include <net/slip.h>

#include <net/bpf.h>
#include <net/bpfdesc.h>

#include <net/if_arc.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/if_inarp.h>

#if defined(_KERNEL_OPT)
#include "opt_bpf.h"
#include "sl.h"
#include "strip.h"
#endif

#ifndef BPF_BUFSIZE
/*
 * 4096 is too small for FDDI frames. 8192 is too small for gigabit Ethernet
 * jumbos (circa 9k), ATM, or Intel gig/10gig ethernet jumbos (16k).
 */
# define BPF_BUFSIZE 32768
#endif

#define PRINET  26			/* interruptible */

/*
 * The default read buffer size, and limit for BIOCSBLEN, is sysctl'able.
 * XXX the default values should be computed dynamically based
 * on available memory size and available mbuf clusters.
 */
int bpf_bufsize = BPF_BUFSIZE;
int bpf_maxbufsize = BPF_DFLTBUFSIZE;	/* XXX set dynamically, see above */


/*
 * Global BPF statistics returned by net.bpf.stats sysctl.
 */
struct bpf_stat	bpf_gstats;

/*
 * Use a mutex to avoid a race condition between gathering the stats/peers
 * and opening/closing the device.
 */
struct simplelock bpf_slock;

/*
 *  bpf_iflist is the list of interfaces; each corresponds to an ifnet
 *  bpf_dtab holds the descriptors, indexed by minor device #
 */
struct bpf_if	*bpf_iflist;
LIST_HEAD(, bpf_d) bpf_list;

static int	bpf_allocbufs(struct bpf_d *);
static void	bpf_deliver(struct bpf_if *,
		            void *(*cpfn)(void *, const void *, size_t),
			    void *, u_int, u_int, struct ifnet *);
static void	bpf_freed(struct bpf_d *);
static void	bpf_ifname(struct ifnet *, struct ifreq *);
static void	*bpf_mcpy(void *, const void *, size_t);
static int	bpf_movein(struct uio *, int, int,
			        struct mbuf **, struct sockaddr *);
static void	bpf_attachd(struct bpf_d *, struct bpf_if *);
static void	bpf_detachd(struct bpf_d *);
static int	bpf_setif(struct bpf_d *, struct ifreq *);
static void	bpf_timed_out(void *);
static inline void
		bpf_wakeup(struct bpf_d *);
static void	catchpacket(struct bpf_d *, u_char *, u_int, u_int,
                            void *(*)(void *, const void *, size_t),
                            struct timeval*);
static void	reset_d(struct bpf_d *);
static int	bpf_getdltlist(struct bpf_d *, struct bpf_dltlist *);
static int	bpf_setdlt(struct bpf_d *, u_int);

static int	bpf_read(struct file *, off_t *, struct uio *, kauth_cred_t,
    int);
static int	bpf_write(struct file *, off_t *, struct uio *, kauth_cred_t,
    int);
static int	bpf_ioctl(struct file *, u_long, void *, struct lwp *);
static int	bpf_poll(struct file *, int, struct lwp *);
static int	bpf_close(struct file *, struct lwp *);
static int	bpf_kqfilter(struct file *, struct knote *);

static const struct fileops bpf_fileops = {
	bpf_read,
	bpf_write,
	bpf_ioctl,
	fnullop_fcntl,
	bpf_poll,
	fbadop_stat,
	bpf_close,
	bpf_kqfilter,
};

dev_type_open(bpfopen);

const struct cdevsw bpf_cdevsw = {
	bpfopen, noclose, noread, nowrite, noioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_OTHER
};

static int
bpf_movein(struct uio *uio, int linktype, int mtu, struct mbuf **mp,
	   struct sockaddr *sockp)
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

	case DLT_ECONET:
		sockp->sa_family = AF_UNSPEC;
		hlen = 6;
		align = 2;
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

	m = m_gethdr(M_WAIT, MT_DATA);
	m->m_pkthdr.rcvif = 0;
	m->m_pkthdr.len = len - hlen;
	if (len > MHLEN - align) {
		m_clget(m, M_WAIT);
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

	error = uiomove(mtod(m, void *), len, uio);
	if (error)
		goto bad;
	if (hlen != 0) {
		memcpy(sockp->sa_data, mtod(m, void *), hlen);
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
 * Must be called at splnet.
 */
static void
bpf_attachd(struct bpf_d *d, struct bpf_if *bp)
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
bpf_detachd(struct bpf_d *d)
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

/*
 * bpfilterattach() is called at boot time.
 */
/* ARGSUSED */
void
bpfilterattach(int n __unused)
{
	simple_lock_init(&bpf_slock);

	simple_lock(&bpf_slock);
	LIST_INIT(&bpf_list);
	simple_unlock(&bpf_slock);

	bpf_gstats.bs_recv = 0;
	bpf_gstats.bs_drop = 0;
	bpf_gstats.bs_capt = 0;
}

/*
 * Open ethernet device. Clones.
 */
/* ARGSUSED */
int
bpfopen(dev_t dev __unused, int flag __unused, int mode __unused, struct lwp *l)
{
	struct bpf_d *d;
	struct file *fp;
	int error, fd;

	/* falloc() will use the descriptor for us. */
	if ((error = falloc(l, &fp, &fd)) != 0)
		return error;

	d = malloc(sizeof(*d), M_DEVBUF, M_WAITOK);
	(void)memset(d, 0, sizeof(*d));
	d->bd_bufsize = bpf_bufsize;
	d->bd_seesent = 1;
	d->bd_pid = l->l_proc->p_pid;
	callout_init(&d->bd_callout);

	simple_lock(&bpf_slock);
	LIST_INSERT_HEAD(&bpf_list, d, bd_list);
	simple_unlock(&bpf_slock);

	return fdclone(l, fp, fd, flag, &bpf_fileops, d);
}

/*
 * Close the descriptor by detaching it from its interface,
 * deallocating its buffers, and marking it free.
 */
/* ARGSUSED */
static int
bpf_close(struct file *fp, struct lwp *l)
{
	struct bpf_d *d = fp->f_data;
	int s;

	/*
	 * Refresh the PID associated with this bpf file.
	 */
	d->bd_pid = l->l_proc->p_pid;

	s = splnet();
	if (d->bd_state == BPF_WAITING)
		callout_stop(&d->bd_callout);
	d->bd_state = BPF_IDLE;
	if (d->bd_bif)
		bpf_detachd(d);
	splx(s);
	bpf_freed(d);
	simple_lock(&bpf_slock);
	LIST_REMOVE(d, bd_list);
	simple_unlock(&bpf_slock);
	free(d, M_DEVBUF);
	fp->f_data = NULL;

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
static int
bpf_read(struct file *fp, off_t *offp __unused, struct uio *uio,
    kauth_cred_t cred __unused, int flags __unused)
{
	struct bpf_d *d = fp->f_data;
	int timed_out;
	int error;
	int s;

	/*
	 * Restrict application to use a buffer the same size as
	 * the kernel buffers.
	 */
	if (uio->uio_resid != d->bd_bufsize)
		return (EINVAL);

	s = splnet();
	if (d->bd_state == BPF_WAITING)
		callout_stop(&d->bd_callout);
	timed_out = (d->bd_state == BPF_TIMED_OUT);
	d->bd_state = BPF_IDLE;
	/*
	 * If the hold buffer is empty, then do a timed sleep, which
	 * ends when the timeout expires or when enough packets
	 * have arrived to fill the store buffer.
	 */
	while (d->bd_hbuf == 0) {
		if (fp->f_flag & FNONBLOCK) {
			if (d->bd_slen == 0) {
				splx(s);
				return (EWOULDBLOCK);
			}
			ROTATE_BUFFERS(d);
			break;
		}

		if ((d->bd_immediate || timed_out) && d->bd_slen != 0) {
			/*
			 * A packet(s) either arrived since the previous
			 * read or arrived while we were asleep.
			 * Rotate the buffers and return what's here.
			 */
			ROTATE_BUFFERS(d);
			break;
		}
		error = tsleep(d, PRINET|PCATCH, "bpf",
				d->bd_rtout);
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

	s = splnet();
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
static inline void
bpf_wakeup(struct bpf_d *d)
{
	wakeup(d);
	if (d->bd_async)
		fownsignal(d->bd_pgid, SIGIO, 0, 0, NULL);

	selnotify(&d->bd_sel, 0);
	/* XXX */
	d->bd_sel.sel_pid = 0;
}


static void
bpf_timed_out(void *arg)
{
	struct bpf_d *d = arg;
	int s;

	s = splnet();
	if (d->bd_state == BPF_WAITING) {
		d->bd_state = BPF_TIMED_OUT;
		if (d->bd_slen != 0)
			bpf_wakeup(d);
	}
	splx(s);
}


static int
bpf_write(struct file *fp, off_t *offp __unused, struct uio *uio,
    kauth_cred_t cred __unused, int flags __unused)
{
	struct bpf_d *d = fp->f_data;
	struct ifnet *ifp;
	struct mbuf *m;
	int error, s;
	static struct sockaddr_storage dst;

	m = NULL;	/* XXX gcc */

	if (d->bd_bif == 0)
		return (ENXIO);

	ifp = d->bd_bif->bif_ifp;

	if (uio->uio_resid == 0)
		return (0);

	error = bpf_movein(uio, (int)d->bd_bif->bif_dlt, ifp->if_mtu, &m,
		(struct sockaddr *) &dst);
	if (error)
		return (error);

	if (m->m_pkthdr.len > ifp->if_mtu) {
		m_freem(m);
		return (EMSGSIZE);
	}

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
 * receive and drop counts.  Should be called at splnet.
 */
static void
reset_d(struct bpf_d *d)
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
	d->bd_ccount = 0;
}

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
 *  BIOCGHDRCMPLT	Get "header already complete" flag.
 *  BIOCSHDRCMPLT	Set "header already complete" flag.
 */
/* ARGSUSED */
static int
bpf_ioctl(struct file *fp, u_long cmd, void *addr, struct lwp *l)
{
	struct bpf_d *d = fp->f_data;
	int s, error = 0;

	/*
	 * Refresh the PID associated with this bpf file.
	 */
	d->bd_pid = l->l_proc->p_pid;

	s = splnet();
	if (d->bd_state == BPF_WAITING)
		callout_stop(&d->bd_callout);
	d->bd_state = BPF_IDLE;
	splx(s);

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

			s = splnet();
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

			if (size > bpf_maxbufsize)
				*(u_int *)addr = size = bpf_maxbufsize;
			else if (size < BPF_MINBUFSIZE)
				*(u_int *)addr = size = BPF_MINBUFSIZE;
			d->bd_bufsize = size;
		}
		break;

	/*
	 * Set link layer read filter.
	 */
	case BIOCSETF:
		error = bpf_setf(d, addr);
		break;

	/*
	 * Flush read packet buffer.
	 */
	case BIOCFLUSH:
		s = splnet();
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
		s = splnet();
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
	 * Get a list of supported device parameters.
	 */
	case BIOCGDLTLIST:
		if (d->bd_bif == 0)
			error = EINVAL;
		else
			error = bpf_getdltlist(d, addr);
		break;

	/*
	 * Set device parameters.
	 */
	case BIOCSDLT:
		if (d->bd_bif == 0)
			error = EINVAL;
		else
			error = bpf_setdlt(d, *(u_int *)addr);
		break;

	/*
	 * Set interface name.
	 */
	case BIOCGETIF:
		if (d->bd_bif == 0)
			error = EINVAL;
		else
			bpf_ifname(d->bd_bif->bif_ifp, addr);
		break;

	/*
	 * Set interface.
	 */
	case BIOCSETIF:
		error = bpf_setif(d, addr);
		break;

	/*
	 * Set read timeout.
	 */
	case BIOCSRTIMEOUT:
		{
			struct timeval *tv = addr;

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
			struct timeval *tv = addr;

			tv->tv_sec = d->bd_rtout / hz;
			tv->tv_usec = (d->bd_rtout % hz) * tick;
			break;
		}

	/*
	 * Get packet stats.
	 */
	case BIOCGSTATS:
		{
			struct bpf_stat *bs = addr;

			bs->bs_recv = d->bd_rcount;
			bs->bs_drop = d->bd_dcount;
			bs->bs_capt = d->bd_ccount;
			break;
		}

	case BIOCGSTATSOLD:
		{
			struct bpf_stat_old *bs = addr;

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
			struct bpf_version *bv = addr;

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

	/*
	 * Get "see sent packets" flag
	 */
	case BIOCGSEESENT:
		*(u_int *)addr = d->bd_seesent;
		break;

	/*
	 * Set "see sent" packets flag
	 */
	case BIOCSSEESENT:
		d->bd_seesent = *(u_int *)addr;
		break;

	case FIONBIO:		/* Non-blocking I/O */
		/*
		 * No need to do anything special as we use IO_NDELAY in
		 * bpfread() as an indication of whether or not to block
		 * the read.
		 */
		break;

	case FIOASYNC:		/* Send signal on receive packets */
		d->bd_async = *(int *)addr;
		break;

	case TIOCSPGRP:		/* Process or group to send signals to */
	case FIOSETOWN:
		error = fsetown(l->l_proc, &d->bd_pgid, cmd, addr);
		break;

	case TIOCGPGRP:
	case FIOGETOWN:
		error = fgetown(l->l_proc, d->bd_pgid, cmd, addr);
		break;
	}
	return (error);
}

/*
 * Set d's packet filter program to fp.  If this file already has a filter,
 * free it and replace it.  Returns EINVAL for bogus requests.
 */
int
bpf_setf(struct bpf_d *d, struct bpf_program *fp)
{
	struct bpf_insn *fcode, *old;
	u_int flen, size;
	int s;

	old = d->bd_filter;
	if (fp->bf_insns == 0) {
		if (fp->bf_len != 0)
			return (EINVAL);
		s = splnet();
		d->bd_filter = 0;
		reset_d(d);
		splx(s);
		if (old != 0)
			free(old, M_DEVBUF);
		return (0);
	}
	flen = fp->bf_len;
	if (flen > BPF_MAXINSNS)
		return (EINVAL);

	size = flen * sizeof(*fp->bf_insns);
	fcode = malloc(size, M_DEVBUF, M_WAITOK);
	if (copyin(fp->bf_insns, fcode, size) == 0 &&
	    bpf_validate(fcode, (int)flen)) {
		s = splnet();
		d->bd_filter = fcode;
		reset_d(d);
		splx(s);
		if (old != 0)
			free(old, M_DEVBUF);

		return (0);
	}
	free(fcode, M_DEVBUF);
	return (EINVAL);
}

/*
 * Detach a file from its current interface (if attached at all) and attach
 * to the interface indicated by the name stored in ifr.
 * Return an errno or 0.
 */
static int
bpf_setif(struct bpf_d *d, struct ifreq *ifr)
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
		/* skip additional entry */
		if ((caddr_t *)bp->bif_driverp != &ifp->if_bpf)
			continue;
		/*
		 * We found the requested interface.
		 * Allocate the packet buffers if we need to.
		 * If we're already attached to requested interface,
		 * just flush the buffer.
		 */
		if (d->bd_sbuf == 0) {
			error = bpf_allocbufs(d);
			if (error != 0)
				return (error);
		}
		s = splnet();
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
bpf_ifname(struct ifnet *ifp, struct ifreq *ifr)
{
	memcpy(ifr->ifr_name, ifp->if_xname, IFNAMSIZ);
}

/*
 * Support for poll() system call
 *
 * Return true iff the specific operation will not block indefinitely - with
 * the assumption that it is safe to positively acknowledge a request for the
 * ability to write to the BPF device.
 * Otherwise, return false but make a note that a selwakeup() must be done.
 */
static int
bpf_poll(struct file *fp, int events, struct lwp *l)
{
	struct bpf_d *d = fp->f_data;
	int s = splnet();
	int revents;

	/*
	 * Refresh the PID associated with this bpf file.
	 */
	d->bd_pid = l->l_proc->p_pid;

	revents = events & (POLLOUT | POLLWRNORM);
	if (events & (POLLIN | POLLRDNORM)) {
		/*
		 * An imitation of the FIONREAD ioctl code.
		 */
		if ((d->bd_hlen != 0) ||
		    (d->bd_immediate && d->bd_slen != 0)) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else if (d->bd_state == BPF_TIMED_OUT) {
			if (d->bd_slen != 0)
				revents |= events & (POLLIN | POLLRDNORM);
			else
				revents |= events & POLLIN;
		} else {
			selrecord(l, &d->bd_sel);
			/* Start the read timeout if necessary */
			if (d->bd_rtout > 0 && d->bd_state == BPF_IDLE) {
				callout_reset(&d->bd_callout, d->bd_rtout,
					      bpf_timed_out, d);
				d->bd_state = BPF_WAITING;
			}
		}
	}

	splx(s);
	return (revents);
}

static void
filt_bpfrdetach(struct knote *kn)
{
	struct bpf_d *d = kn->kn_hook;
	int s;

	s = splnet();
	SLIST_REMOVE(&d->bd_sel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_bpfread(struct knote *kn, long hint __unused)
{
	struct bpf_d *d = kn->kn_hook;

	kn->kn_data = d->bd_hlen;
	if (d->bd_immediate)
		kn->kn_data += d->bd_slen;
	return (kn->kn_data > 0);
}

static const struct filterops bpfread_filtops =
	{ 1, NULL, filt_bpfrdetach, filt_bpfread };

static int
bpf_kqfilter(struct file *fp, struct knote *kn)
{
	struct bpf_d *d = fp->f_data;
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &d->bd_sel.sel_klist;
		kn->kn_fop = &bpfread_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = d;

	s = splnet();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

/*
 * Incoming linkage from device drivers.  Process the packet pkt, of length
 * pktlen, which is stored in a contiguous buffer.  The packet is parsed
 * by each process' filter, and if accepted, stashed into the corresponding
 * buffer.
 */
void
bpf_tap(void *arg, u_char *pkt, u_int pktlen)
{
	struct bpf_if *bp;
	struct bpf_d *d;
	u_int slen;
	struct timeval tv;
	int gottime=0;

	/*
	 * Note that the ipl does not have to be raised at this point.
	 * The only problem that could arise here is that if two different
	 * interfaces shared any data.  This is not the case.
	 */
	bp = arg;
	for (d = bp->bif_dlist; d != 0; d = d->bd_next) {
		++d->bd_rcount;
		++bpf_gstats.bs_recv;
		slen = bpf_filter(d->bd_filter, pkt, pktlen, pktlen);
		if (slen != 0) {
			if (!gottime) {
				microtime(&tv);
				gottime = 1;
			}
		catchpacket(d, pkt, pktlen, slen, memcpy, &tv);
		}
	}
}

/*
 * Copy data from an mbuf chain into a buffer.  This code is derived
 * from m_copydata in sys/uipc_mbuf.c.
 */
static void *
bpf_mcpy(void *dst_arg, const void *src_arg, size_t len)
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
		memcpy(dst, mtod(m, void *), count);
		m = m->m_next;
		dst += count;
		len -= count;
	}
	return (dst_arg);
}

/*
 * Dispatch a packet to all the listeners on interface bp.
 *
 * marg    pointer to the packet, either a data buffer or an mbuf chain
 * buflen  buffer length, if marg is a data buffer
 * cpfn    a function that can copy marg into the listener's buffer
 * pktlen  length of the packet
 * rcvif   either NULL or the interface the packet came in on.
 */
static inline void
bpf_deliver(struct bpf_if *bp, void *(*cpfn)(void *, const void *, size_t),
	    void *marg, u_int pktlen, u_int buflen, struct ifnet *rcvif)
{
	u_int slen;
	struct bpf_d *d;
	struct timeval tv;
	int gottime = 0;

	for (d = bp->bif_dlist; d != 0; d = d->bd_next) {
		if (!d->bd_seesent && (rcvif == NULL))
			continue;
		++d->bd_rcount;
		++bpf_gstats.bs_recv;
		slen = bpf_filter(d->bd_filter, marg, pktlen, buflen);
		if (slen != 0) {
			if(!gottime) {
				microtime(&tv);
				gottime = 1;
			}
			catchpacket(d, marg, pktlen, slen, cpfn, &tv);
		}
	}
}

/*
 * Incoming linkage from device drivers, when the head of the packet is in
 * a buffer, and the tail is in an mbuf chain.
 */
void
bpf_mtap2(void *arg, void *data, u_int dlen, struct mbuf *m)
{
	struct bpf_if *bp = arg;
	u_int pktlen;
	struct mbuf mb;

	pktlen = m_length(m) + dlen;

	/*
	 * Craft on-stack mbuf suitable for passing to bpf_filter.
	 * Note that we cut corners here; we only setup what's
	 * absolutely needed--this mbuf should never go anywhere else.
	 */
	(void)memset(&mb, 0, sizeof(mb));
	mb.m_next = m;
	mb.m_data = data;
	mb.m_len = dlen;

	bpf_deliver(bp, bpf_mcpy, &mb, pktlen, 0, m->m_pkthdr.rcvif);
}

/*
 * Incoming linkage from device drivers, when packet is in an mbuf chain.
 */
void
bpf_mtap(void *arg, struct mbuf *m)
{
	void *(*cpfn)(void *, const void *, size_t);
	struct bpf_if *bp = arg;
	u_int pktlen, buflen;
	void *marg;

	pktlen = m_length(m);

	if (pktlen == m->m_len) {
		cpfn = memcpy;
		marg = mtod(m, void *);
		buflen = pktlen;
	} else {
		cpfn = bpf_mcpy;
		marg = m;
		buflen = 0;
	}

	bpf_deliver(bp, cpfn, marg, pktlen, buflen, m->m_pkthdr.rcvif);
}

/*
 * We need to prepend the address family as
 * a four byte field.  Cons up a dummy header
 * to pacify bpf.  This is safe because bpf
 * will only read from the mbuf (i.e., it won't
 * try to free it or keep a pointer a to it).
 */
void
bpf_mtap_af(void *arg, u_int32_t af, struct mbuf *m)
{
	struct mbuf m0;

	m0.m_flags = 0;
	m0.m_next = m;
	m0.m_len = 4;
	m0.m_data = (char *)&af;

	bpf_mtap(arg, &m0);
}

void
bpf_mtap_et(void *arg, u_int16_t et, struct mbuf *m)
{
	struct mbuf m0;

	m0.m_flags = 0;
	m0.m_next = m;
	m0.m_len = 14;
	m0.m_data = m0.m_dat;

	((u_int32_t *)m0.m_data)[0] = 0;
	((u_int32_t *)m0.m_data)[1] = 0;
	((u_int32_t *)m0.m_data)[2] = 0;
	((u_int16_t *)m0.m_data)[6] = et;

	bpf_mtap(arg, &m0);
}

#if NSL > 0 || NSTRIP > 0
/*
 * Put the SLIP pseudo-"link header" in place.
 * Note this M_PREPEND() should never fail,
 * swince we know we always have enough space
 * in the input buffer.
 */
void
bpf_mtap_sl_in(void *arg, u_char *chdr, struct mbuf **m)
{
	int s;
	u_char *hp;

	M_PREPEND(*m, SLIP_HDRLEN, M_DONTWAIT);
	if (*m == NULL)
		return;

	hp = mtod(*m, u_char *);
	hp[SLX_DIR] = SLIPDIR_IN;
	(void)memcpy(&hp[SLX_CHDR], chdr, CHDR_LEN);

	s = splnet();
	bpf_mtap(arg, *m);
	splx(s);

	m_adj(*m, SLIP_HDRLEN);
}

/*
 * Put the SLIP pseudo-"link header" in
 * place.  The compressed header is now
 * at the beginning of the mbuf.
 */
void
bpf_mtap_sl_out(void *arg, u_char *chdr, struct mbuf *m)
{
	struct mbuf m0;
	u_char *hp;
	int s;

	m0.m_flags = 0;
	m0.m_next = m;
	m0.m_data = m0.m_dat;
	m0.m_len = SLIP_HDRLEN;

	hp = mtod(&m0, u_char *);

	hp[SLX_DIR] = SLIPDIR_OUT;
	(void)memcpy(&hp[SLX_CHDR], chdr, CHDR_LEN);

	s = splnet();
	bpf_mtap(arg, &m0);
	splx(s);
	m_freem(m);
}
#endif

/*
 * Move the packet data from interface memory (pkt) into the
 * store buffer.  Return 1 if it's time to wakeup a listener (buffer full),
 * otherwise 0.  "copy" is the routine called to do the actual data
 * transfer.  memcpy is passed in to copy contiguous chunks, while
 * bpf_mcpy is passed in to copy mbuf chains.  In the latter case,
 * pkt is really an mbuf.
 */
static void
catchpacket(struct bpf_d *d, u_char *pkt, u_int pktlen, u_int snaplen,
	    void *(*cpfn)(void *, const void *, size_t), struct timeval *tv)
{
	struct bpf_hdr *hp;
	int totlen, curlen;
	int hdrlen = d->bd_bif->bif_hdrlen;

	++d->bd_ccount;
	++bpf_gstats.bs_capt;
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
			++bpf_gstats.bs_drop;
			return;
		}
		ROTATE_BUFFERS(d);
		bpf_wakeup(d);
		curlen = 0;
	}

	/*
	 * Append the bpf header.
	 */
	hp = (struct bpf_hdr *)(d->bd_sbuf + curlen);
	hp->bh_tstamp = *tv;
	hp->bh_datalen = pktlen;
	hp->bh_hdrlen = hdrlen;
	/*
	 * Copy the packet data into the store buffer and update its length.
	 */
	(*cpfn)((u_char *)hp + hdrlen, pkt, (hp->bh_caplen = totlen - hdrlen));
	d->bd_slen = curlen + totlen;

	/*
	 * Call bpf_wakeup after bd_slen has been updated so that kevent(2)
	 * will cause filt_bpfread() to be called with it adjusted.
	 */
	if (d->bd_immediate || d->bd_state == BPF_TIMED_OUT)
		/*
		 * Immediate mode is set, or the read timeout has
		 * already expired during a select call.  A packet
		 * arrived, so the reader should be woken up.
		 */
		bpf_wakeup(d);
}

/*
 * Initialize all nonzero fields of a descriptor.
 */
static int
bpf_allocbufs(struct bpf_d *d)
{

	d->bd_fbuf = malloc(d->bd_bufsize, M_DEVBUF, M_NOWAIT);
	if (!d->bd_fbuf)
		return (ENOBUFS);
	d->bd_sbuf = malloc(d->bd_bufsize, M_DEVBUF, M_NOWAIT);
	if (!d->bd_sbuf) {
		free(d->bd_fbuf, M_DEVBUF);
		return (ENOBUFS);
	}
	d->bd_slen = 0;
	d->bd_hlen = 0;
	return (0);
}

/*
 * Free buffers currently in use by a descriptor.
 * Called on close.
 */
static void
bpf_freed(struct bpf_d *d)
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
		free(d->bd_filter, M_DEVBUF);
}

/*
 * Attach an interface to bpf.  dlt is the link layer type; hdrlen is the
 * fixed size of the link header (variable length headers not yet supported).
 */
void
bpfattach(struct ifnet *ifp, u_int dlt, u_int hdrlen)
{

	bpfattach2(ifp, dlt, hdrlen, &ifp->if_bpf);
}

/*
 * Attach additional dlt for a interface to bpf.  dlt is the link layer type;
 * hdrlen is the fixed size of the link header for the specified dlt
 * (variable length headers not yet supported).
 */
void
bpfattach2(struct ifnet *ifp, u_int dlt, u_int hdrlen, void *driverp)
{
	struct bpf_if *bp;
	bp = malloc(sizeof(*bp), M_DEVBUF, M_DONTWAIT);
	if (bp == 0)
		panic("bpfattach");

	bp->bif_dlist = 0;
	bp->bif_driverp = driverp;
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
bpfdetach(struct ifnet *ifp)
{
	struct bpf_if *bp, **pbp;
	struct bpf_d *d;
	int s;

	/* Nuke the vnodes for any open instances */
	for (d = LIST_FIRST(&bpf_list); d != NULL; d = LIST_NEXT(d, bd_list)) {
		if (d->bd_bif != NULL && d->bd_bif->bif_ifp == ifp) {
			/*
			 * Detach the descriptor from an interface now.
			 * It will be free'ed later by close routine.
			 */
			s = splnet();
			d->bd_promisc = 0;	/* we can't touch device. */
			bpf_detachd(d);
			splx(s);
		}
	}

  again:
	for (bp = bpf_iflist, pbp = &bpf_iflist;
	     bp != NULL; pbp = &bp->bif_next, bp = bp->bif_next) {
		if (bp->bif_ifp == ifp) {
			*pbp = bp->bif_next;
			free(bp, M_DEVBUF);
			goto again;
		}
	}
}

/*
 * Change the data link type of a interface.
 */
void
bpf_change_type(struct ifnet *ifp, u_int dlt, u_int hdrlen)
{
	struct bpf_if *bp;

	for (bp = bpf_iflist; bp != NULL; bp = bp->bif_next) {
		if ((caddr_t *)bp->bif_driverp == &ifp->if_bpf)
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

/*
 * Get a list of available data link type of the interface.
 */
static int
bpf_getdltlist(struct bpf_d *d, struct bpf_dltlist *bfl)
{
	int n, error;
	struct ifnet *ifp;
	struct bpf_if *bp;

	ifp = d->bd_bif->bif_ifp;
	n = 0;
	error = 0;
	for (bp = bpf_iflist; bp != NULL; bp = bp->bif_next) {
		if (bp->bif_ifp != ifp)
			continue;
		if (bfl->bfl_list != NULL) {
			if (n >= bfl->bfl_len)
				return ENOMEM;
			error = copyout(&bp->bif_dlt,
			    bfl->bfl_list + n, sizeof(u_int));
		}
		n++;
	}
	bfl->bfl_len = n;
	return error;
}

/*
 * Set the data link type of a BPF instance.
 */
static int
bpf_setdlt(struct bpf_d *d, u_int dlt)
{
	int s, error, opromisc;
	struct ifnet *ifp;
	struct bpf_if *bp;

	if (d->bd_bif->bif_dlt == dlt)
		return 0;
	ifp = d->bd_bif->bif_ifp;
	for (bp = bpf_iflist; bp != NULL; bp = bp->bif_next) {
		if (bp->bif_ifp == ifp && bp->bif_dlt == dlt)
			break;
	}
	if (bp == NULL)
		return EINVAL;
	s = splnet();
	opromisc = d->bd_promisc;
	bpf_detachd(d);
	bpf_attachd(d, bp);
	reset_d(d);
	if (opromisc) {
		error = ifpromisc(bp->bif_ifp, 1);
		if (error)
			printf("%s: bpf_setdlt: ifpromisc failed (%d)\n",
			    bp->bif_ifp->if_xname, error);
		else
			d->bd_promisc = 1;
	}
	splx(s);
	return 0;
}

static int
sysctl_net_bpf_maxbufsize(SYSCTLFN_ARGS)
{
	int newsize, error;
	struct sysctlnode node;

	node = *rnode;
	node.sysctl_data = &newsize;
	newsize = bpf_maxbufsize;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (newsize < BPF_MINBUFSIZE || newsize > BPF_MAXBUFSIZE)
		return (EINVAL);

	bpf_maxbufsize = newsize;

	return (0);
}

static int
sysctl_net_bpf_peers(SYSCTLFN_ARGS)
{
	int    error, elem_count;
	struct bpf_d	 *dp;
	struct bpf_d_ext  dpe;
	size_t len, needed, elem_size, out_size;
	char   *sp;

	if (namelen == 1 && name[0] == CTL_QUERY)
		return (sysctl_query(SYSCTLFN_CALL(rnode)));

	if (namelen != 2)
		return (EINVAL);

	if ((error = kauth_authorize_generic(l->l_cred,
	    KAUTH_GENERIC_ISSUSER, &l->l_acflag)))
		return (error);

	len = (oldp != NULL) ? *oldlenp : 0;
	sp = oldp;
	elem_size = name[0];
	elem_count = name[1];
	out_size = MIN(sizeof(dpe), elem_size);
	needed = 0;

	if (elem_size < 1 || elem_count < 0)
		return (EINVAL);

	simple_lock(&bpf_slock);
	LIST_FOREACH(dp, &bpf_list, bd_list) {
		if (len >= elem_size && elem_count > 0) {
#define BPF_EXT(field)	dpe.bde_ ## field = dp->bd_ ## field
			BPF_EXT(bufsize);
			BPF_EXT(promisc);
			BPF_EXT(promisc);
			BPF_EXT(state);
			BPF_EXT(immediate);
			BPF_EXT(hdrcmplt);
			BPF_EXT(seesent);
			BPF_EXT(pid);
			BPF_EXT(rcount);
			BPF_EXT(dcount);
			BPF_EXT(ccount);
#undef BPF_EXT
			if (dp->bd_bif)
				(void)strlcpy(dpe.bde_ifname,
				    dp->bd_bif->bif_ifp->if_xname,
				    IFNAMSIZ - 1);
			else
				dpe.bde_ifname[0] = '\0';

			error = copyout(&dpe, sp, out_size);
			if (error)
				break;
			sp += elem_size;
			len -= elem_size;
		}
		if (elem_count > 0) {
			needed += elem_size;
			if (elem_count != INT_MAX)
				elem_count--;
		}
	}
	simple_unlock(&bpf_slock);

	*oldlenp = needed;

	return (error);
}

SYSCTL_SETUP(sysctl_net_bpf_setup, "sysctl net.bpf subtree setup")
{
	const struct sysctlnode *node;

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "net", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_EOL);

	node = NULL;
	sysctl_createv(clog, 0, NULL, &node,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "bpf",
		       SYSCTL_DESCR("BPF options"),
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_CREATE, CTL_EOL);
	if (node != NULL) {
		sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "maxbufsize",
			SYSCTL_DESCR("Maximum size for data capture buffer"),
			sysctl_net_bpf_maxbufsize, 0, &bpf_maxbufsize, 0,
			CTL_NET, node->sysctl_num, CTL_CREATE, CTL_EOL);
		sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT,
			CTLTYPE_STRUCT, "stats",
			SYSCTL_DESCR("BPF stats"),
			NULL, 0, &bpf_gstats, sizeof(bpf_gstats),
			CTL_NET, node->sysctl_num, CTL_CREATE, CTL_EOL);
		sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT,
			CTLTYPE_STRUCT, "peers",
			SYSCTL_DESCR("BPF peers"),
			sysctl_net_bpf_peers, 0, NULL, 0,
			CTL_NET, node->sysctl_num, CTL_CREATE, CTL_EOL);
	}

}
