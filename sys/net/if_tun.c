/*
 * Copyright (c) 1988, Julian Onions.
 *
 * This source may be freely distributed, however I would be interested
 * in any changes that are made.
 *
 *  if_tun.c - tunnel interface module & driver
 *
 * $Id: if_tun.c,v 1.6 1993/11/14 20:07:20 deraadt Exp $
 */

#include "tun.h"
#if NTUN > 0

/*
 * Tunnel driver.
 *
 * This driver takes packets off the IP i/f and hands them up to a
 * user process to have it's wicked way with. This driver has it's
 * roots in a similar driver written by Phil Cockcroft (formerly) at
 * UCL. This driver is based much more on read/write/select mode of
 * operation though.
 * 
 * Julian Onions <jpo@cs.nott.ac.uk>
 * Nottingham University 1987.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>

#include <net/if.h>
#include <net/if_tun.h>
#include <net/netisr.h>
#include <net/route.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#define TUNDEBUG	if (tundebug) printf
int	tundebug = 0;

struct tun_softc tunctl[NTUN];
extern int ifqmaxlen;

int	tunoutput __P((dev_t, int, int, struct proc *));
int	tunclose __P((dev_t, int));
int	tunoutput __P((struct ifnet *, struct mbuf *, struct sockaddr *));
int	tunread __P((dev_t, struct uio *));
int	tunwrite __P((dev_t, struct uio *));
int	tuncioctl __P((dev_t, int, caddr_t, data);
int	tunioctl __P((struct ifnet *, int, caddr_t, int));
int	tunselect __P((dev_t, int);

static int tunattach __P((int));
static int tuninit __P((int));

/*
 * tunnel open - must be superuser & the device must be
 * configured in
 */
int
tunopen(dev, flag, mode, p)
	dev_t	dev;
	int	flag, mode;
	struct proc *p;
{
	struct ifnet	*ifp;
	struct tunctl	*tp;
	register int	unit, error;

	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);

	if ((unit = minor(dev)) >= NTUN)
		return (ENXIO);
	tp = &tunctl[unit];
	if (tp->tun_flags & TUN_OPEN)
		return ENXIO;
	if ((tp->tun_flags & TUN_INITED) == 0) {
		tp->tun_flags = TUN_INITED;
		tunattach(unit);
	}
	ifp = &tp->tun_if;
	tp->tun_flags |= TUN_OPEN;
	TUNDEBUG("%s%d: open\n", ifp->if_name, ifp->if_unit);
	return (0);
}

/*
 * tunclose - close the device - mark i/f down & delete
 * routing info
 */
int
tunclose(dev, flag)
	dev_t	dev;
	int	flag;
{
	struct tunctl	*tp = &tunctl[unit];
	struct ifnet	*ifp = &tp->tun_if;
	struct mbuf	*m;
	register int	unit = minor(dev), s;

	tp->tun_flags &= TUN_INITED;

	/*
	 * junk all pending output
	 */
	do {
		s = splimp();
		IF_DEQUEUE(&ifp->if_snd, m);
		splx(s);
		if (m)
			m_freem(m);
	} while (m);

	if (ifp->if_flags & IFF_UP) {
		s = splimp();
		if_down(ifp);
		if (ifp->if_flags & IFF_RUNNING)
			rtinit(ifp->if_addrlist, (int)SIOCDELRT, RTF_HOST);
		splx(s);
	}
	tp->tun_pgrp = 0;
	if (tp->tun_rsel)
		selwakeup(&tp->tun_rsel);
		
	tp->tun_rsel = tp->tun_wsel = (struct proc *)0;

	TUNDEBUG ("%s%d: closed\n", ifp->if_name, ifp->if_unit);
	return (0);
}

/*
 * attach an interface N.B. argument is not same as other drivers
 */
static int
tunattach(unit)
	int	unit;
{
	struct ifnet	*ifp = &tunctl[unit].tun_if;
	struct sockaddr_in *sin;

	ifp->if_unit = unit;
	ifp->if_name = "tun";
	ifp->if_mtu = TUNMTU;
	ifp->if_ioctl = tunioctl;
	ifp->if_output = tunoutput;
	ifp->if_flags = IFF_POINTOPOINT;
	ifp->if_snd.ifq_maxlen = ifqmaxlen;
	ifp->if_collisions = 0;
	ifp->if_ierrors = 0;
	ifp->if_oerrors = 0;
	ifp->if_ipackets = 0;
	ifp->if_opackets = 0;
	if_attach(ifp);
	TUNDEBUG("%s%d: tunattach\n", ifp->if_name, ifp->if_unit);
	return 0;
}

static int
tuninit(unit)
	int	unit;
{
	struct tunctl	*tp = &tunctl[unit];
	struct ifnet	*ifp = &tp->tun_if;

	ifp->if_flags |= IFF_UP | IFF_RUNNING;
	tp->tun_flags |= TUN_IASET;
	TUNDEBUG("%s%d: tuninit\n", ifp->if_name, ifp->if_unit);
	return 0;
}

/*
 * Process an ioctl request.
 */
int
tunioctl(ifp, cmd, data, flag)
	struct ifnet *ifp;
	int	cmd;
	caddr_t	data;
	int	flag;
{
	int		error = 0, s;
	struct tunctl	*tp = &tunctl[ifp->if_unit];

	s = splimp();
	switch(cmd) {
	case SIOCSIFADDR:
		tuninit(ifp->if_unit);
		break;
	case SIOCSIFDSTADDR:
		tp->tun_flags |= TUN_DSTADDR;
		TUNDEBUG("%s%d: destination address set\n", ifp->if_name,
		    ifp->if_unit);
		break;
	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}

/*
 * tunoutput - queue packets from higher level ready to put out.
 */
int
tunoutput(ifp, m0, dst)
	struct ifnet   *ifp;
	struct mbuf    *m0;
	struct sockaddr *dst;
{
	struct tunctl	*tp = &tunctl[ifp->if_unit];
	struct proc	*p;
	int		s;

	TUNDEBUG ("%s%d: tunoutput\n", ifp->if_name, ifp->if_unit);

	switch(dst->sa_family) {
#ifdef INET
	case AF_INET:
		s = splimp();
		if (IF_QFULL(&ifp->if_snd)) {
			IF_DROP(&ifp->if_snd);
			m_freem(m0);
			splx(s);
			ifp->if_collisions++;
			return (ENOBUFS);
		}
		IF_ENQUEUE(&ifp->if_snd, m0);
		splx(s);
		ifp->if_opackets++;
		break;
#endif
	default:
		m_freem(m0);
		return EAFNOSUPPORT;
	}

	if (tp->tun_flags & TUN_RWAIT) {
		tp->tun_flags &= ~TUN_RWAIT;
		wakeup((caddr_t)tp);
	}
	if (tp->tun_flags & TUN_ASYNC && tp->tun_pgrp) {
		if (tp->tun_pgrp > 0)
			gsignal(tp->tun_pgrp, SIGIO);
		else if (p = pfind(-tp->tun_pgrp))
			psignal(p, SIGIO);
	}
	selwakeup(&tp->tun_rsel);
	return 0;
}

/*
 * the cdevsw interface is now pretty minimal.
 */
int
tuncioctl(dev, cmd, data, flag)
	dev_t		dev;
	int		cmd;
	caddr_t		data;
	int		flag;
{
	int		unit = minor(dev), s;
	struct tunctl	*tp = &tunctl[unit];

	switch (cmd) {
	case TUNSDEBUG:
		tundebug = *(int *)data;
		break;
	case TUNGDEBUG:
		*(int *)data = tundebug;
		break;
	case FIONBIO:
		if (*(int *)data)
			tp->tun_flags |= TUN_NBIO;
		else
			tp->tun_flags &= ~TUN_NBIO;
		break;
	case FIOASYNC:
		if (*(int *)data)
			tp->tun_flags |= TUN_ASYNC;
		else
			tp->tun_flags &= ~TUN_ASYNC;
		break;
	case FIONREAD:
		s = splimp();
		if (tp->tun_if.if_snd.ifq_head)
			*(int *)data = tp->tun_if.if_snd.ifq_head->m_len;
		else	
			*(int *)data = 0;
		splx(s);
		break;
	case TIOCSPGRP:
		tp->tun_pgrp = *(int *)data;
		break;
	case TIOCGPGRP:
		*(int *)data = tp->tun_pgrp;
		break;
	default:
		return (ENOTTY);
	}
	return (0);
}

/*
 * The cdevsw read interface - reads a packet at a time, or at
 * least as much of a packet as can be read.
 */
int
tunread(dev, uio)
	dev_t		dev;
	struct uio	*uio;
{
	struct ifnet	*ifp = &tp->tun_if;
	struct mbuf	*m, *m0;
	struct tunctl	*tp = &tunctl[unit];
	int		unit = minor(dev);
	int		error=0, len, s;

	TUNDEBUG ("%s%d: read\n", ifp->if_name, ifp->if_unit);
	tp->tun_flags &= ~TUN_RWAIT;

	s = splimp();
	do {
		IF_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == 0) {
			if (tp->tun_flags & TUN_NBIO) {
				splx(s);
				return EWOULDBLOCK;
			}
			tp->tun_flags |= TUN_RWAIT;
			tsleep((caddr_t)tp, PZERO + 1, "tunread", 0);
		}
	} while (m0 == 0);
	splx(s);

	while (m0 && uio->uio_resid > 0 && error == 0) {
		len = MIN(uio->uio_resid, m0->m_len);
		if (len == 0)
			break;
		error = uiomove(mtod(m0, caddr_t), len,
		    UIO_READ, uio);
		MFREE(m0, m);
		m0 = m;
	}

	if (m0) {
		TUNDEBUG("Dropping mbuf\n");
		m_freem(m0);
	}
	return error;
}

/*
 * the cdevsw write interface - an atomic write is a packet - or else!
 */
int
tunwrite(dev, uio)
	int		dev;
	struct uio	*uio;
{
	int		unit = minor (dev);
	struct ifnet	*ifp = &(tunctl[unit].tun_if);
	struct mbuf	*top, **mp, *m;
	int		error=0, s;

	TUNDEBUG("%s%d: tunwrite\n", ifp->if_name, ifp->if_unit);

	if (uio->uio_resid < 0 || uio->uio_resid > TUNMTU) {
		TUNDEBUG("%s%d: len=%d!\n", ifp->if_name, ifp->if_unit,
		    uio->uio_resid);
		return EIO;
	}
	top = 0;
	mp = &top;
	while (error == 0 && uio->uio_resid > 0) {
		MGET(m, M_DONTWAIT, MT_DATA);
		if (m == 0) {
			error = ENOBUFS;
			break;
		}
		m->m_len = MIN(MLEN, uio->uio_resid);
		error = uiomove(mtod(m, caddr_t), m->m_len, UIO_WRITE, uio);
		*mp = m;
		mp = &m->m_next;
	}
	if (error) {
		if (top)
			m_freem(top);
		return error;
	}

	/*
	 * Place interface pointer before the data
	 * for the receiving protocol.
	 */
	if (top->m_off <= MMAXOFF &&
	    top->m_off >= MMINOFF + sizeof(struct ifnet *)) {
		top->m_off -= sizeof(struct ifnet *);
		top->m_len += sizeof(struct ifnet *);
	} else {
		MGET(m, M_DONTWAIT, MT_HEADER);
		if (m == (struct mbuf *)0)
			return (ENOBUFS);
		m->m_len = sizeof(struct ifnet *);
		m->m_next = top;
		top = m;
	}
	*(mtod(top, struct ifnet **)) = ifp;

	s = splimp();
	if (IF_QFULL (&ipintrq)) {
		IF_DROP(&ipintrq);
		splx(s);
		ifp->if_collisions++;
		m_freem(top);
		return ENOBUFS;
	}
	IF_ENQUEUE(&ipintrq, top);
	splx(s);
	ifp->if_ipackets++;
	schednetisr(NETISR_IP);
	return error;
}

/*
 * tunselect - the select interface, this is only useful on reads
 * really. The write detect always returns true, write never blocks
 * anyway, it either accepts the packet or drops it.
 */
int
tunselect(dev, rw)
	dev_t		dev;
	int		rw;
{
	int		unit = minor(dev), s;
	struct tunctl	*tp = &tunctl[unit];
	struct ifnet	*ifp = &tp->tun_if;

	s = splimp();
	TUNDEBUG("%s%d: tunselect\n", ifp->if_name, ifp->if_unit);

	switch (rw) {
	case FREAD:
		if (ifp->if_snd.ifq_len > 0) {
			splx(s);
			TUNDEBUG("%s%d: tunselect q=%d\n", ifp->if_name,
			    ifp->if_unit, ifp->if_snd.ifq_len);
			return 1;
		}
		selrecord(p, &tp->tun_rsel);
		break;
	case FWRITE:
		splx(s);
		return 1;
	}
	splx(s);
	TUNDEBUG("%s%d: tunselect waiting\n", ifp->if_name, ifp->if_unit);
	return 0;
}
#endif  NTUN
