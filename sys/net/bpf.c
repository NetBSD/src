/*-
 * Copyright (c) 1991-1993 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)bpf.c	7.4 (Berkeley) 6/17/91
 *
 *	from: Header: bpf.c,v 1.3 93/12/11 02:52:18 mccanne Exp
 *	$Id: bpf.c,v 1.11 1994/01/25 06:10:08 deraadt Exp $
 */

#include "bpfilter.h"

#if (NBPFILTER > 0)

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/select.h>

#include <sys/file.h>
#include <sys/tty.h>
#include <sys/uio.h>

#include <sys/protosw.h>
#include <sys/socket.h>
#include <net/if.h>

#include <net/bpf.h>
#include <net/bpfdesc.h>

#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/vnode.h>

#include "sl.h"
#include "ppp.h"

#ifndef BPF_BUFSIZE
#define BPF_BUFSIZE NBPG
#endif

/*
 * The default read buffer size is patchable.
 */
int bpf_bufsize = BPF_BUFSIZE;

/*
 *  bpf_iflist is the list of interfaces; each corresponds to an ifnet
 *  bpf_dtab holds the descriptors, indexed by minor device #
 */
struct bpf_if 	*bpf_iflist;
struct bpf_d	bpf_dtab[NBPFILTER];

static void	bpf_ifname();
static void	catchpacket();
static void	bpf_freed();
static int	bpf_setif();
static int	bpf_allocbufs();

void
bpfilterattach(n)
	int n;
{
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
		d->bd_promisc = 0;
		if (ifpromisc(bp->bif_ifp, 0))
			/*
			 * Something is really wrong if we were able to put
			 * the driver into promiscuous mode, but can't
			 * take it out.
			 */
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
 * Open ethernet device.  Returns ENXIO for illegal minor device number,
 * EBUSY if file is open by another process.
 */
/* ARGSUSED */
int
bpfopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct bpf_d *d;
	
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
	bzero((char *)d, sizeof(*d));
	d->bd_bufsize = bpf_bufsize;

	return (0);
}

/*
 * Close the descriptor by detaching it from its interface,
 * deallocating its buffers, and marking it free.
 */
/* ARGSUSED */
int
bpfclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct bpf_d *d = &bpf_dtab[minor(dev)];
	register int s;

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
	register struct uio *uio;
	int ioflag;
{
	register struct bpf_d *d = &bpf_dtab[minor(dev)];
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
		if (ioflag & IO_NDELAY) {
			if (d->bd_slen == 0) {
				splx(s);
				return (EWOULDBLOCK);
			}
			ROTATE_BUFFERS(d);
			break;
		}
		if (d->bd_immediate && d->bd_slen != 0) {
			/*
			 * A packet(s) either arrived since the previous
			 * read or arrived while we were asleep.
			 * Rotate the buffers and return what's here.
			 */
			ROTATE_BUFFERS(d);
			break;
		}
		error = tsleep((caddr_t)d, PWAIT | PCATCH, "bpf", d->bd_rtout);
		if (error != 0) {
			if (error == EWOULDBLOCK) {
				/*
				 * On a timeout, return what's in the buffer,
				 * which may be nothing.  If there is something
				 * in the store buffer, we can do a rotation.
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
			splx(s);
			return (error);
		}
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
	splx(s);
	
	return (error);
}


/*
 * If there are processes sleeping on this descriptor, wake them up.  
 */
static inline void
bpf_wakeup(d)
	register struct bpf_d *d;
{
	wakeup((caddr_t)d);
#if (BSD > 199103) || defined(__NetBSD__)
	selwakeup(&d->bd_sel);
	/* XXX */
	d->bd_sel.si_pid = 0;
#else
	if (d->bd_selproc) {
		selwakeup(d->bd_selproc, (int)d->bd_selcoll);
		d->bd_selcoll = 0;
		d->bd_selproc = 0;
	}
#endif
}

int
bpfwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct bpf_if *bp = bpf_dtab[minor(dev)].bd_bif;
	register struct ifnet *ifp;
	register struct mbuf *m;
	register u_int len, hlen;
	register int error, s;
	struct sockaddr dst;

	if (bp == 0)
		return (ENXIO);

	/*
	 * Build a sockaddr based on the data link layer type.
	 * The AF_UNSPEC kludge allows us to hand the link level
	 * header to the driver via the sockaddr.  This isn't
	 * very clean.  It would be better if AF_UNSPEC meant that
	 * the driver shouldn't bother with encapsulation (i.e., the
	 * link header is already in the mbuf).  The code here is
	 * structured this way, then things are kludged back before
	 * calling if_output.
	 *
	 * NOTE:  When adding new link layers make sure the driver supports
	 *	  AF_UNSPEC and that the link header can fit in the sa_data
	 *	  field of a sockaddr.
	 */
	switch (bp->bif_dlt) {

#if NSL > 0
	case DLT_SLIP:
		dst.sa_family = AF_INET;
		hlen = 0;
		break;
#endif

#if NPPP > 0
	case DLT_PPP:
		dst.sa_family = AF_UNSPEC;
		hlen = 0;
		break;
#endif

	case DLT_EN10MB:
		dst.sa_family = AF_UNSPEC;
		hlen = 14;
		break;

	case DLT_FDDI:
		dst.sa_family = AF_UNSPEC;
		/* XXX 4(FORMAC)+6(dst)+6(src)+3(LLC)+5(SNAP) */
		hlen = 24;
		break;

	case DLT_NULL:
		dst.sa_family = AF_UNSPEC;
		hlen = 0;
		break;

	default:
		return (EIO);
	}
	ifp = bp->bif_ifp;
	len = uio->uio_resid;
	/*
	 * If we didn't get enough for the link level header, or we
	 * exceed the interface's mtu, return an error.
	 */
	if (len < hlen || len - hlen > ifp->if_mtu)
		return (EMSGSIZE);

	/*
	 * XXX Avoid complicated buffer chaining --- 
	 * bail if it won't fit in a single mbuf.
	 */
	if (len > MCLBYTES)
		return (EMSGSIZE);

	MGETHDR(m, M_WAIT, MT_DATA);
	if (m == 0)
		return (ENOBUFS);
	if (len > MLEN) {
		MCLGET(m, M_WAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return (ENOBUFS);
		}
	}
	/*
	 * Move the whole packet, including the data link header, 
	 * into the mbuf.  Then, copy the link header back out of the
	 * packet into the sockaddr.  Finally, strip the link header
	 * from the front of the mbuf.
	 */
	error = uiomove(mtod(m, caddr_t), len, uio);
	if (error) {
		m_freem(m);
		return (error);
	}
	if (hlen > 0) {
		bcopy(mtod(m, caddr_t), dst.sa_data, hlen);
		m->m_data += hlen;
		len -= hlen;
	}
	m->m_pkthdr.len = m->m_len = len;
	s = splnet();
	error = (*ifp->if_output)(ifp, m, &dst, (struct rtentry *)0);
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

/*
 *  FIONREAD		Check for read packet available.
 *  SIOCGIFADDR		Get interface address - convenient hook to driver.
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
 */
/* ARGSUSED */
int
bpfioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
{
	register struct bpf_d *d = &bpf_dtab[minor(dev)];
	int s, error = 0;

	switch (cmd) {

	default:
		error = EINVAL;
		break;

	case FIONBIO:
	case FIOASYNC:
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

	case SIOCGIFADDR:
		{
			struct ifnet *ifp;

			if (d->bd_bif == 0)
				error = EINVAL;
			else {
				ifp = d->bd_bif->bif_ifp;
				error =  (*ifp->if_ioctl)(ifp, cmd, addr);
			}
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
			register u_int size = *(u_int *)addr;

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
			u_long msec;

			/* Compute number of milliseconds. */
			msec = tv->tv_sec * 1000 + tv->tv_usec / 1000;
			/* Scale milliseconds to ticks.  Assume hard
			   clock has millisecond or greater resolution
			   (i.e. tick >= 1000).  For 10ms hardclock,
			   tick/1000 = 10, so rtout<-msec/10. */
			d->bd_rtout = msec / (tick / 1000);
			break;
		}

	/*
	 * Get read timeout.
	 */
 	case BIOCGRTIMEOUT:
		{
			struct timeval *tv = (struct timeval *)addr;
			u_long msec = d->bd_rtout;

			msec *= tick / 1000;
			tv->tv_sec = msec / 1000;
			tv->tv_usec = msec % 1000;
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
	if (fcode == 0)
		return (ENOMEM);
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
	int unit, s, error;

	/*
	 * Separate string into name part and unit number.  Put a null
	 * byte at the end of the name part, and compute the number. 
	 * If the a unit number is unspecified, the default is 0,
	 * as initialized above.  XXX This should be common code.
	 */
	unit = 0;
	cp = ifr->ifr_name;
	cp[sizeof(ifr->ifr_name) - 1] = '\0';
	while (*cp++) {
		if (*cp >= '0' && *cp <= '9') {
			unit = *cp - '0';
			*cp++ = '\0';
			while (*cp)
				unit = 10 * unit + *cp++ - '0';
			break;
		}
	}
	/*
	 * Look through attached interfaces for the named one.
	 */
	for (bp = bpf_iflist; bp != 0; bp = bp->bif_next) {
		struct ifnet *ifp = bp->bif_ifp;

		if (ifp == 0 || unit != ifp->if_unit 
		    || strcmp(ifp->if_name, ifr->ifr_name) != 0)
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
 * Convert an interface name plus unit number of an ifp to a single
 * name which is returned in the ifr.
 */
static void
bpf_ifname(ifp, ifr)
	struct ifnet *ifp;
	struct ifreq *ifr;
{
	char *s = ifp->if_name;
	char *d = ifr->ifr_name;

	while (*d++ = *s++)
		continue;
	/* XXX Assume that unit number is less than 10. */
	*d++ = ifp->if_unit + '0';
	*d = '\0';
}

/*
 * Support for select() system call
 * Inspired by the code in tty.c for the same purpose.
 *
 * Return true iff the specific operation will not block indefinitely.
 * Otherwise, return false but make a note that a selwakeup() must be done.
 */
int
bpfselect(dev, rw, p)
	register dev_t dev;
	int rw;
	struct proc *p;
{
	register struct bpf_d *d;
	register int s;
	
	if (rw != FREAD)
		return (0);
	/*
	 * An imitation of the FIONREAD ioctl code.
	 */
	d = &bpf_dtab[minor(dev)];
	
	s = splimp();
	if (d->bd_hlen != 0 || (d->bd_immediate && d->bd_slen != 0)) {
		/*
		 * There is data waiting.
		 */
		splx(s);
		return (1);
	}
	/*
	 * No data ready.  If there's already a select() waiting on this
	 * minor device then this is a collision.  This shouldn't happen 
	 * because minors really should not be shared, but if a process
	 * forks while one of these is open, it is possible that both
	 * processes could select on the same descriptor.
	 */
#if defined(__NetBSD__)
	selrecord(p, &d->bd_sel);
#else
	if (d->bd_selproc && d->bd_selproc->p_wchan == (caddr_t)&selwait)
		d->bd_selcoll = 1;
	else
		d->bd_selproc = p;

#endif
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
bpf_tap(arg, pkt, pktlen)
	caddr_t arg;
	register u_char *pkt;
	register u_int pktlen;
{
	struct bpf_if *bp;
	register struct bpf_d *d;
	register u_int slen;

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
			catchpacket(d, pkt, pktlen, slen, bcopy);
	}
}

/*
 * Copy data from an mbuf chain into a buffer.  This code is derived
 * from m_copydata in sys/uipc_mbuf.c.
 */
static void
bpf_mcopy(src, dst, len)
	u_char *src;
	u_char *dst;
	register int len;
{
	register struct mbuf *m = (struct mbuf *)src;
	register unsigned count;

	while (len > 0) {
		if (m == 0)
			panic("bpf_mcopy");
		count = MIN(m->m_len, len);
		bcopy(mtod(m, caddr_t), (caddr_t)dst, count);
		m = m->m_next;
		dst += count;
		len -= count;
	}
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
			catchpacket(d, (u_char *)m, pktlen, slen, bpf_mcopy);
	}
}

/*
 * Move the packet data from interface memory (pkt) into the
 * store buffer.  Return 1 if it's time to wakeup a listener (buffer full),
 * otherwise 0.  "copy" is the routine called to do the actual data
 * transfer.  bcopy is passed in to copy contiguous chunks, while
 * bpf_mcopy is passed in to copy mbuf chains.  In the latter case,
 * pkt is really an mbuf.
 */
static void
catchpacket(d, pkt, pktlen, snaplen, cpfn)
	register struct bpf_d *d;
	register u_char *pkt;
	register u_int pktlen, snaplen;
	register void (*cpfn)();
{
	register struct bpf_hdr *hp;
	register int totlen, curlen;
	register int hdrlen = d->bd_bif->bif_hdrlen;
	/*
	 * Figure out how many bytes to move.  If the packet is
	 * greater or equal to the snapshot length, transfer that
	 * much.  Otherwise, transfer the whole packet (unless
	 * we hit the buffer size limit).
	 */
	totlen = hdrlen + MIN(snaplen, pktlen);
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
	(*cpfn)(pkt, (u_char *)hp + hdrlen, (hp->bh_caplen = totlen - hdrlen));
	d->bd_slen = curlen + totlen;
}

/* 
 * Initialize all nonzero fields of a descriptor.
 */
static int
bpf_allocbufs(d)
	register struct bpf_d *d;
{
	d->bd_fbuf = (caddr_t)malloc(d->bd_bufsize, M_DEVBUF, M_WAITOK);
	if (d->bd_fbuf == 0)
		return (ENOBUFS);

	d->bd_sbuf = (caddr_t)malloc(d->bd_bufsize, M_DEVBUF, M_WAITOK);
	if (d->bd_sbuf == 0) {
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
bpf_freed(d)
	register struct bpf_d *d;
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
	int i;

	bp = (struct bpf_if *)malloc(sizeof(*bp), M_DEVBUF, M_DONTWAIT);
	if (bp == 0) {
		printf("bpf: no buffers in attach");
		return;
	}
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

	/*
	 * Mark all the descriptors free if this hasn't been done.
	 */
	if (!D_ISFREE(&bpf_dtab[0]))
		for (i = 0; i < NBPFILTER; ++i)
			D_MARKFREE(&bpf_dtab[i]);

#if 0
	printf("bpf: %s%d attached\n", ifp->if_name, ifp->if_unit);
#endif
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
	struct ifreq ifr;
	/* 
	 * If the device is not configured up, we cannot put it in
	 * promiscuous mode.
	 */
	if ((ifp->if_flags & IFF_UP) == 0)
		return (ENETDOWN);

	if (pswitch) {
		if (ifp->if_pcount++ != 0)
			return (0);
		ifp->if_flags |= IFF_PROMISC;
	} else {
		if (--ifp->if_pcount > 0)
			return (0);
		ifp->if_flags &= ~IFF_PROMISC;
	}
	ifr.ifr_flags = ifp->if_flags;
	return ((*ifp->if_ioctl)(ifp, SIOCSIFFLAGS, (caddr_t)&ifr));
}

#endif (NBPFILTER > 0)
