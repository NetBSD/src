/*	$NetBSD: if_dse.c,v 1.3 2022/12/22 23:06:11 nat Exp $ */

/*
 * Driver for DaynaPORT SCSI/Link SCSI-Ethernet
 *
 * Written by Hiroshi Noguchi <ngc@ff.iij4u.or.jp>
 *
 * Modified by Matt Sandstrom <mattias@beauty.se> for NetBSD 1.5.3
 *
 * This driver is written based on "if_se.c".
 */

/*
 * Copyright (c) 1997 Ian W. Dall <ian.dall@dsto.defence.gov.au>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Ian W. Dall.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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



#include "opt_inet.h"
#include "opt_net_mpsafe.h"
#include "opt_atalk.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/proc.h>
#include <sys/conf.h>

#include <sys/workqueue.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <sys/mbuf.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NETATALK
#include <netatalk/at.h>
#endif

#include <net/bpf.h>


/*
 * debug flag
 */
#if 0
#define	DSE_DEBUG
#endif


#define DSE_TIMEOUT	100000
#define	DSE_OUTSTANDING	4
#define	DSE_RETRIES	4
#define DSE_MINSIZE	60

#define	DSE_HEADER_TX	4
#define	DSE_TAIL_TX	4
#define DSE_EXTRAS_TX	(DSE_HEADER_TX + DSE_TAIL_TX)

#define	DSE_HEADER_RX	6
#define	DSE_TAIL_RX	0
#define	DSE_EXTRAS_RX	(DSE_HEADER_RX + DSE_TAIL_RX)

#define	MAX_BYTES_RX	(ETHERMTU + sizeof(struct ether_header) + ETHER_CRC_LEN)

/* 10 full length packets appears to be the max ever returned. 16k is OK */
#define RBUF_LEN	(16 * 1024)

/*
 * Tuning parameters:
 *   We will attempt to adapt to polling fast enough to get RDATA_GOAL packets
 *   per read
 */
#define RDATA_MAX	10	/* maximum of returned packets (guessed) */
#define RDATA_GOAL 	8

/*
 * maximum of available multicast address entries (guessed)
 */
#define	DSE_MCAST_MAX	10


/* dse_poll and dse_poll0 are the normal polling rate and the minimum
 * polling rate respectively. dse_poll0 should be chosen so that at
 * maximum ethernet speed, we will read nearly RDATA_MAX packets. dse_poll
 * should be chosen for reasonable maximum latency.
 * In practice, if we are being saturated with min length packets, we
 * can't poll fast enough. Polling with zero delay actually
 * worsens performance. dse_poll0 is enforced to be always at least 1
 */
#if MAC68K_DEBUG
#define DSE_POLL		50	/* default in milliseconds */
#define DSE_POLL0 		30	/* default in milliseconds */
#else
#define DSE_POLL		80	/* default in milliseconds */
#define DSE_POLL0 		40	/* default in milliseconds */
#endif
int dse_poll = 0;		/* Delay in ticks set at attach time */
int dse_poll0 = 0;
int dse_max_received = 0;	/* Instrumentation */




/*==========================================
  data type defs
==========================================*/
typedef struct scsipi_inquiry_data dayna_ether_inquiry_data;

typedef struct {
	uint8_t	opcode[2];
	uint8_t	byte3;
	uint8_t	length[2];
	uint8_t	byte6;
} scsi_dayna_ether_generic;

#define	DAYNA_CMD_SEND		0x0A		/* same as generic "Write" */
#define	DAYNA_CMD_RECV		0x08		/* same as generic "Read" */

#define	DAYNA_CMD_GET_ADDR	0x09		/* ???: read MAC address ? */
#define	REQ_LEN_GET_ADDR	0x12

#define	DAYNA_CMD_SET_MULTI	0x0D		/* set multicast address */

#define	DAYNA_CMD_VENDOR1	0x0E		/* ???: initialize signal ? */

#define IS_SEND(generic)	((generic)->opcode == DAYNA_CMD_SEND)
#define IS_RECV(generic)	((generic)->opcode == DAYNA_CMD_RECV)

struct dse_softc {
	device_t sc_dev;
	struct	ethercom sc_ethercom;	/* Ethernet common part */
	struct scsipi_periph *sc_periph;/* contains our targ, lun, etc. */

	struct callout sc_recv_ch;
	struct kmutex sc_iflock;
	struct if_percpuq *sc_ipq;
	struct workqueue *sc_recv_wq, *sc_send_wq;
	struct work sc_recv_work, sc_send_work;
	int sc_recv_work_pending, sc_send_work_pending;

	char *sc_tbuf;
	char *sc_rbuf;
	int sc_debug;
	int sc_flags;
	int sc_last_timeout;
	int sc_enabled;
	int sc_attach_state;
};

/* bit defs of "sc_flags" */
#define DSE_NEED_RECV	0x1

static int	dsematch(device_t, cfdata_t, void *);
static void	dseattach(device_t, device_t, void *);
static int	dsedetach(device_t, int);

static void	dse_ifstart(struct ifnet *);
static void	dse_send_worker(struct work *wk, void *cookie);

static void	dsedone(struct scsipi_xfer *, int);
static int	dse_ioctl(struct ifnet *, u_long, void *);
static void	dsewatchdog(struct ifnet *);

static void	dse_recv_callout(void *);
static void	dse_recv_worker(struct work *wk, void *cookie);
static void	dse_recv(struct dse_softc *);
static struct mbuf*	dse_get(struct dse_softc *, uint8_t *, int);
static int	dse_read(struct dse_softc *, uint8_t *, int);

static int	dse_init_adaptor(struct dse_softc *);
static int	dse_get_addr(struct dse_softc *, uint8_t *);
static int	dse_set_multi(struct dse_softc *);

static int	dse_reset(struct dse_softc *);

#if 0	/* 07/16/2000 comment-out */
static int	dse_set_mode(struct dse_softc *, int, int);
#endif
static int	dse_init(struct dse_softc *);
static void	dse_stop(struct dse_softc *);

#if 0
static __inline uint16_t	ether_cmp(void *, void *);
#endif

static inline int dse_scsipi_cmd(struct scsipi_periph *periph,
			struct scsipi_generic *scsipi_cmd,
			int cmdlen, u_char *data_addr, int datalen,
			int retries, int timeout, struct buf *bp,
			int flags);

int	dse_enable(struct dse_softc *);
void	dse_disable(struct dse_softc *);


CFATTACH_DECL_NEW(dse, sizeof(struct dse_softc),
    dsematch, dseattach, dsedetach, NULL);

extern struct cfdriver dse_cd;

dev_type_open(dseopen);
dev_type_close(dseclose);
dev_type_ioctl(dseioctl);

const struct cdevsw dse_cdevsw = {
	.d_open = dseopen,
	.d_close = dseclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = dseioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

const struct scsipi_periphsw dse_switch = {

	NULL,			/* Use default error handler */
	NULL,		/* have no queue */
	NULL,			/* have no async handler */
	dsedone,		/* deal with stats at interrupt time */
};

struct scsipi_inquiry_pattern dse_patterns[] = {
	{	T_PROCESSOR,	T_FIXED,
		"Dayna",		"SCSI/Link",		"" },
};



/*====================================================
  definitions for SCSI commands
====================================================*/

/*
 * command templates
 */
/* unknown commands */
/* Vendor #1 */
static const scsi_dayna_ether_generic	sonic_ether_vendor1 = {
	{ DAYNA_CMD_VENDOR1, 0x00 },
	0x00,
	{ 0x00, 0x00 },
	0x80
};



#if 0
/*
 * Compare two Ether/802 addredses for equality, inlined and
 * unrolled for speed.
 * Note: use this like memcmp()
 */
static __inline uint16_t
ether_cmp(void *one, void *two)
{
	uint16_t*	a;
	uint16_t*	b;
	uint16_t diff;

	a = (uint16_t *) one;
	b = (uint16_t *) two;

	diff = (a[0] - b[0]) | (a[1] - b[1]) | (a[2] - b[2]);

	return (diff);
}

#define ETHER_CMP	ether_cmp
#endif

/*
 * check to match with SCSI inquiry information
 */
static int
dsematch(device_t parent, cfdata_t match, void *aux)
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    dse_patterns, sizeof(dse_patterns) / sizeof(dse_patterns[0]),
	    sizeof(dse_patterns[0]), &priority);
	return priority;
}


/*
 * The routine called by the low level scsi routine when it discovers
 * a device suitable for this driver.
 */
static void
dseattach(device_t parent, device_t self, void *aux)
{
	struct dse_softc *sc = device_private(self);
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint8_t myaddr[ETHER_ADDR_LEN];
	char wqname[MAXCOMLEN];
	int rv;

	sc->sc_dev = self;

	aprint_normal("\n");
	SC_DEBUG(periph, SCSIPI_DB2, ("dseattach: "));

	sc->sc_attach_state = 0;
	callout_init(&sc->sc_recv_ch, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_recv_ch, dse_recv_callout, (void *)sc);
	mutex_init(&sc->sc_iflock, MUTEX_DEFAULT, IPL_SOFTNET);

	/*
	 * Store information needed to contact our base driver
	 */
	sc->sc_periph = periph;
	periph->periph_dev = sc->sc_dev;
	periph->periph_switch = &dse_switch;
#if 0
	sc_periph->sc_link_dbflags = SCSIPI_DB1;
#endif

	dse_poll = mstohz(DSE_POLL);
	dse_poll = dse_poll? dse_poll: 1;
	dse_poll0 = mstohz(DSE_POLL0);
	dse_poll0 = dse_poll0? dse_poll0: 1;

	/*
	 * Initialize and attach send and receive buffers
	 */
	sc->sc_tbuf = malloc(ETHERMTU + sizeof(struct ether_header) +
	    DSE_EXTRAS_TX + 16, M_DEVBUF, M_WAITOK);

	sc->sc_rbuf = malloc(RBUF_LEN + 16, M_DEVBUF, M_WAITOK);

	/* initialize adaptor and obtain MAC address */
	dse_init_adaptor(sc);
	sc->sc_attach_state = 1;

	/* Initialize ifnet structure. */
	strcpy(ifp->if_xname, device_xname(sc->sc_dev));
	ifp->if_softc = sc;
	ifp->if_start = dse_ifstart;
	ifp->if_ioctl = dse_ioctl;
	ifp->if_watchdog = dsewatchdog;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_extflags = IFEF_MPSAFE;

	dse_get_addr(sc, myaddr);

	/* Attach the interface. */
	if_initialize(ifp);

	snprintf(wqname, sizeof(wqname), "%sRx", device_xname(sc->sc_dev));
	rv = workqueue_create(&sc->sc_recv_wq, wqname, dse_recv_worker, sc,
	    PRI_SOFTNET, IPL_NET, WQ_MPSAFE);
	if (rv != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to create recv Rx workqueue\n");
		dsedetach(sc->sc_dev, 0);
		return; /* Error */
	}
	sc->sc_recv_work_pending = false;
	sc->sc_attach_state = 2;

	snprintf(wqname, sizeof(wqname), "%sTx", device_xname(sc->sc_dev));
	rv = workqueue_create(&sc->sc_send_wq, wqname, dse_send_worker, ifp,
	    PRI_SOFTNET, IPL_NET, WQ_MPSAFE);
	if (rv != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to create send Tx workqueue\n");
		dsedetach(sc->sc_dev, 0);
		return; /* Error */
	}
	sc->sc_send_work_pending = false;
	sc->sc_ipq = if_percpuq_create(&sc->sc_ethercom.ec_if);
	ether_ifattach(ifp, myaddr);
	if_register(ifp);
	sc->sc_attach_state = 4;

	bpf_attach(ifp, DLT_EN10MB, sizeof(struct ether_header));
}

static int
dsedetach(device_t self, int flags)
{
	struct dse_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	switch(sc->sc_attach_state) {
	case 4:
		dse_stop(sc);
		mutex_enter(&sc->sc_iflock);
		ifp->if_flags &= ~IFF_RUNNING;
		dse_disable(sc);
		ether_ifdetach(ifp);
		if_detach(ifp);
		mutex_exit(&sc->sc_iflock);
		if_percpuq_destroy(sc->sc_ipq);
		/*FALLTHROUGH*/
	case 3:
		workqueue_destroy(sc->sc_send_wq);
		/*FALLTHROUGH*/
	case 2:
		workqueue_destroy(sc->sc_recv_wq);
		/*FALLTHROUGH*/
	case 1:
		free(sc->sc_rbuf, M_DEVBUF);
		free(sc->sc_tbuf, M_DEVBUF);
		callout_destroy(&sc->sc_recv_ch);
		mutex_destroy(&sc->sc_iflock);
		break;
	default:
		aprint_error_dev(sc->sc_dev, "detach failed (state %d)\n",
		    sc->sc_attach_state);
		return 1;
		break;
	}

	return 0;
}


/*
 * submit SCSI command
 */
static __inline int
dse_scsipi_cmd(struct scsipi_periph *periph, struct scsipi_generic *cmd,
    int cmdlen, u_char *data_addr, int datalen, int retries, int timeout,
    struct buf *bp, int flags)
{
	int error = 0;

	error = scsipi_command(periph, cmd, cmdlen, data_addr,
   		 datalen, retries, timeout, bp, flags);

	return error;
}


/*
 * Start routine for calling from network sub system
 */
static void
dse_ifstart(struct ifnet *ifp)
{
	struct dse_softc *sc = ifp->if_softc;

	mutex_enter(&sc->sc_iflock);
	if (!sc->sc_send_work_pending)  {
		sc->sc_send_work_pending = true;
		workqueue_enqueue(sc->sc_send_wq, &sc->sc_send_work, NULL);
	}
	mutex_exit(&sc->sc_iflock);
	if (sc->sc_flags & DSE_NEED_RECV) {
		sc->sc_flags &= ~DSE_NEED_RECV;
	}
}

/*
 * Invoke the transmit workqueue and transmission on the interface.
 */
static void
dse_send_worker(struct work *wk, void *cookie)
{
	struct ifnet *ifp = cookie;
	struct dse_softc *sc = ifp->if_softc;
	scsi_dayna_ether_generic cmd_send;
	struct mbuf *m, *m0;
	int len, error;
	u_char *cp;

	mutex_enter(&sc->sc_iflock);
	sc->sc_send_work_pending = false;
	mutex_exit(&sc->sc_iflock);

	KASSERT(if_is_mpsafe(ifp));

	/* Don't transmit if interface is busy or not running */
	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	while (1) {
		IFQ_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;
		/* If BPF is listening on this interface, let it see the
		 * packet before we commit it to the wire.
		 */
		bpf_mtap(ifp, m0, BPF_D_OUT);

		/* We need to use m->m_pkthdr.len, so require the header */
		if ((m0->m_flags & M_PKTHDR) == 0)
			panic("ctscstart: no header mbuf");
		len = m0->m_pkthdr.len;

		/* Mark the interface busy. */
		ifp->if_flags |= IFF_OACTIVE;

		/* Chain; copy into linear buffer allocated at attach time. */
		cp = sc->sc_tbuf;
		for (m = m0; m != NULL; ) {
			memcpy(cp, mtod(m, u_char *), m->m_len);
			cp += m->m_len;
			m = m0 = m_free(m);
		}
		if (len < DSE_MINSIZE) {
#ifdef DSE_DEBUG
			if (sc->sc_debug)
				aprint_error_dev(sc->sc_dev,
				    "packet size %d (%zu) < %d\n", len,
				    cp - (u_char *)sc->sc_tbuf, DSE_MINSIZE);
#endif
			memset(cp, 0, DSE_MINSIZE - len);
			len = DSE_MINSIZE;
		}

		/* Fill out SCSI command. */
		memset(&cmd_send, 0, sizeof(cmd_send));
		cmd_send.opcode[0] = DAYNA_CMD_SEND;
		_lto2b(len, &(cmd_send.length[0]));
		cmd_send.byte6 = 0x00;

		/* Send command to device. */
		error = dse_scsipi_cmd(sc->sc_periph,
		    (void *)&cmd_send, sizeof(cmd_send),
		    sc->sc_tbuf, len, DSE_RETRIES,
		    DSE_TIMEOUT, NULL, XS_CTL_NOSLEEP | XS_CTL_POLL |
		    XS_CTL_DATA_OUT);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "not queued, error %d\n", error);
			if_statinc(ifp, if_oerrors);
			ifp->if_flags &= ~IFF_OACTIVE;
		} else
			if_statinc(ifp, if_opackets);
	}
}


/*
 * Called from the scsibus layer via our scsi device switch.
 */
static void
dsedone(struct scsipi_xfer *xs, int error)
{
	struct dse_softc *sc = device_private(xs->xs_periph->periph_dev);
	struct scsipi_generic *cmd = xs->cmd;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	if (IS_SEND(cmd)) {
		ifp->if_flags &= ~IFF_OACTIVE;
	} else if (IS_RECV(cmd)) {
		/* RECV complete */
		/* pass data up. reschedule a recv */
		/* scsipi_free_xs will call start. Harmless. */

		if (error) {
			/* Reschedule after a delay */
			callout_schedule(&sc->sc_recv_ch, dse_poll);
		} else {
			int n, ntimeo;
			n = dse_read(sc, xs->data, xs->datalen - xs->resid);
			if (n > dse_max_received)
				dse_max_received = n;
			if (n == 0)
				ntimeo = dse_poll;
			else if (n >= RDATA_MAX)
				ntimeo = dse_poll0;
			else {
				ntimeo = sc->sc_last_timeout;
				ntimeo = (ntimeo * RDATA_GOAL)/n;
				ntimeo = (ntimeo < dse_poll0?
					  dse_poll0: ntimeo);
				ntimeo = (ntimeo > dse_poll?
					  dse_poll: ntimeo);
			}
			sc->sc_last_timeout = ntimeo;
			callout_schedule(&sc->sc_recv_ch, ntimeo);
		}
	}
}


/*
 * Setup a receive command by queuing the work.
 * Usually called from a callout, but also from se_init().
 */
static void
dse_recv_callout(void *v)
{
	/* do a recv command */
	struct dse_softc *sc = (struct dse_softc *) v;

	if (sc->sc_enabled == 0)
		return;

	mutex_enter(&sc->sc_iflock);
	if (sc->sc_recv_work_pending == true) {
		callout_schedule(&sc->sc_recv_ch, dse_poll);
		mutex_exit(&sc->sc_iflock);
		return;
	}

	sc->sc_recv_work_pending = true;
	workqueue_enqueue(sc->sc_recv_wq, &sc->sc_recv_work, NULL);
	mutex_exit(&sc->sc_iflock);
}

/*
 * Invoke the receive workqueue
 */
static void
dse_recv_worker(struct work *wk, void *cookie)
{
	struct dse_softc *sc = (struct dse_softc *) cookie;

	dse_recv(sc);
	mutex_enter(&sc->sc_iflock);
	sc->sc_recv_work_pending = false;
	mutex_exit(&sc->sc_iflock);

}

/*
 * Do the actual work of receiving data.
 */
static void
dse_recv(struct dse_softc *sc)
{
	scsi_dayna_ether_generic cmd_recv;
	int error, len;

	/* do a recv command */
	/* fill out command buffer */
	memset(&cmd_recv, 0, sizeof(cmd_recv));
	cmd_recv.opcode[0] = DAYNA_CMD_RECV;
	len = MAX_BYTES_RX + DSE_EXTRAS_RX;
	_lto2b(len, &(cmd_recv.length[0]));
	cmd_recv.byte6 = 0xC0;

	error = dse_scsipi_cmd(sc->sc_periph,
	    (void *)&cmd_recv, sizeof(cmd_recv),
	    sc->sc_rbuf, RBUF_LEN, DSE_RETRIES, DSE_TIMEOUT, NULL,
	    XS_CTL_NOSLEEP | XS_CTL_POLL | XS_CTL_DATA_IN);
	if (error)
		callout_schedule(&sc->sc_recv_ch, dse_poll);
}


/*
 * We copy the data into mbufs.  When full cluster sized units are present
 * we copy into clusters.
 */
static struct mbuf *
dse_get(struct dse_softc *sc, uint8_t *data, int totlen)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m, *m0, *newm;
	int len;

	MGETHDR(m0, M_DONTWAIT, MT_DATA);
	if (m0 == NULL)
		return NULL;

	m_set_rcvif(m0, ifp);
	m0->m_pkthdr.len = totlen;
	len	= MHLEN;
	m = m0;

	while (totlen > 0) {
		if (totlen >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if((m->m_flags & M_EXT) == 0)
				goto bad;

			len = MCLBYTES;
		}

		if (m == m0) {
			char *newdata = (char *)
			    ALIGN(m->m_data + sizeof(struct ether_header)) -
			    sizeof(struct ether_header);
			len -= newdata - m->m_data;
			m->m_data = newdata;
		}

		m->m_len = len = uimin(totlen, len);
		memcpy(mtod(m, void *), data, len);
		data += len;

		totlen -= len;
		if (totlen > 0) {
			MGET(newm, M_DONTWAIT, MT_DATA);
			if (newm == NULL)
				goto bad;

			len = MLEN;
			m = m->m_next = newm;
		}
	}

	return m0;

bad:
	m_freem(m0);
	return NULL ;
}


#ifdef MAC68K_DEBUG
static int
peek_packet(uint8_t*  buf)
{
	struct ether_header *eh;
	uint16_t type;
	int len;

	eh = (struct ether_header*)buf;
	type = _2btol((uint8_t*)&(eh->ether_type));

	len = sizeof(struct ether_header);

	if (type <= ETHERMTU) {
		/* for 802.3 */
		len += type;
	} else{
		/* for Ethernet II (DIX) */
		switch (type) {
		  case ETHERTYPE_ARP:
			len += 28;
			break;
		  case ETHERTYPE_IP:
			len += _2btol(buf + sizeof(struct ether_header) + 2);
			break;
		  default:
			len = 0;
			goto l_end;
			break;
		}
	}
	if (len < DSE_MINSIZE) {
		len = DSE_MINSIZE;
	}
	len += ETHER_CRC_LEN;

  l_end:;
	return len;
}
#endif


/*
 * Pass packets to higher levels.
 */
static int
dse_read(struct dse_softc *sc, uint8_t *data, int datalen)
{
	struct mbuf *m;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int	len;
	int	n;
#ifdef MAC68K_DEBUG
	int	peek_flag = 1;
#endif

	mutex_enter(&sc->sc_iflock);
	n = 0;
	while (datalen >= DSE_HEADER_RX) {
		/*
		 * fetch bytes of stream.
		 * here length = (ether frame length) + (FCS's 4 bytes)
		 */
		/* fetch frame length */
		len = _2btol(data);

		/* skip header part */
		data	+= DSE_HEADER_RX;
		datalen -= DSE_HEADER_RX;

#if 0	/* 03/10/2001  only for debug */
		{
			printf("DATALEN %d len %d\n", datalen, len);
			int	j;
			printf("\ndump[%d]: ",n);
			for ( j = 0 ; j < datalen ; j++ ) {
				printf("%02X ",data[j-DSE_HEADER_RX]);
			}
		}
#endif
#ifdef MAC68K_DEBUG
		if (peek_flag) {
			peek_flag = 0;
			len = peek_packet(data);
		}
#endif
		if (len == 0)
			break;

#ifdef DSE_DEBUG
		aprint_error_dev(sc->sc_dev, "dse_read: datalen = %d, packetlen"
		    " = %d, proto = 0x%04x\n", datalen, len,
		    ntohs(((struct ether_header *)data)->ether_type));
#endif
		if ((len < (DSE_MINSIZE + ETHER_CRC_LEN)) ||
		     (MAX_BYTES_RX < len)) {
#ifdef DSE_DEBUG
			aprint_error_dev(sc->sc_dev, "invalid packet size "
			    "%d; dropping\n", len);
#endif
			if_statinc(ifp, if_ierrors);
			break;
		}

		/* Don't need crc. Must keep ether header for BPF */
		m = dse_get(sc, data, len - ETHER_CRC_LEN);
		if (m == NULL) {
#ifdef DSE_DEBUG
			if (sc->sc_debug)
				aprint_error_dev(sc->sc_dev, "dse_read: "
				    "dse_get returned null\n");
#endif
			if_statinc(ifp, if_ierrors);
			goto next_packet;
		}
		if_statinc(ifp, if_ipackets);

		/*
		 * Check if there's a BPF listener on this interface.
		 * If so, hand off the raw packet to BPF.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp, m, BPF_D_OUT);

		/* Pass the packet up. */
		if_percpuq_enqueue(sc->sc_ipq, m);

  next_packet:
		data	+= len;
		datalen	-= len;
		n++;
	}
	mutex_exit(&sc->sc_iflock);

	return n;
}


static void
dsewatchdog(struct ifnet *ifp)
{
	struct dse_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", device_xname(sc->sc_dev));
	if_statinc(ifp, if_oerrors);

	dse_reset(sc);
}


static int
dse_reset(struct dse_softc *sc)
{
	int error;
#if 0
	/* Maybe we don't *really* want to reset the entire bus
	 * because the ctron isn't working. We would like to send a
	 * "BUS DEVICE RESET" message, but don't think the ctron
	 * understands it.
	 */
	error = dse_scsipi_cmd(sc->sc_periph, 0, 0, 0, 0, DSE_RETRIES, 2000,
	    NULL, XS_CTL_RESET);
#endif
	error = dse_init(sc);
	return error;
}


static int
dse_init_adaptor(struct dse_softc *sc)
{
	scsi_dayna_ether_generic cmd_vend1;
	u_char tmpbuf[sizeof(cmd_vend1)];
	int error;

#if 0	/* 07/21/2001 for test */
	/* Maybe we don't *really* want to reset the entire bus
	 * because the ctron isn't working. We would like to send a
	 * "BUS DEVICE RESET" message, but don't think the ctron
	 * understands it.
	 */
	error = dse_scsipi_cmd(sc->sc_periph, 0, 0, 0, 0, DSE_RETRIES,
	    2000, NULL, XS_CTL_RESET);
#endif

	cmd_vend1 = sonic_ether_vendor1;

	error = dse_scsipi_cmd(sc->sc_periph,
	    (struct scsipi_generic *)&cmd_vend1, sizeof(cmd_vend1),
		&(tmpbuf[0]), sizeof(tmpbuf),
	    DSE_RETRIES, DSE_TIMEOUT, NULL, XS_CTL_POLL | XS_CTL_DATA_IN);

	if (error)
		goto l_end;

	/* wait 500 msec */
	kpause("dsesleep", false, hz / 2, NULL);

l_end:
	return error;
}


static int
dse_get_addr(struct dse_softc *sc, uint8_t *myaddr)
{
	scsi_dayna_ether_generic cmd_get_addr;
	u_char tmpbuf[REQ_LEN_GET_ADDR];
	int error;

	memset(&cmd_get_addr, 0, sizeof(cmd_get_addr));
	cmd_get_addr.opcode[0] = DAYNA_CMD_GET_ADDR;
	_lto2b(REQ_LEN_GET_ADDR, cmd_get_addr.length);

	error = dse_scsipi_cmd(sc->sc_periph,
	    (struct scsipi_generic *)&cmd_get_addr, sizeof(cmd_get_addr),
	    tmpbuf, sizeof(tmpbuf),
	    DSE_RETRIES, DSE_TIMEOUT, NULL, XS_CTL_POLL | XS_CTL_DATA_IN);

	if (error == 0) {
		memcpy(myaddr, &(tmpbuf[0]), ETHER_ADDR_LEN);

		aprint_error_dev(sc->sc_dev, "ethernet address %s\n",
			   ether_sprintf(myaddr));
	}

	return error;
}


#if 0	/* 07/16/2000 comment-out */
static int
dse_set_mode(struct dse_softc *sc, int len, int mode)

	return 0;
}
#endif


static int
dse_init(struct dse_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int error = 0;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_UP)) == IFF_UP) {
		ifp->if_flags |= IFF_RUNNING;
		mutex_enter(&sc->sc_iflock);
		if (!sc->sc_recv_work_pending)  {
			sc->sc_recv_work_pending = true;
			workqueue_enqueue(sc->sc_recv_wq, &sc->sc_recv_work,
			    NULL);
		}
		mutex_exit(&sc->sc_iflock);
		ifp->if_flags &= ~IFF_OACTIVE;
		mutex_enter(&sc->sc_iflock);
		if (!sc->sc_send_work_pending)  {
			sc->sc_send_work_pending = true;
			workqueue_enqueue(sc->sc_send_wq, &sc->sc_send_work,
			    NULL);
		}
		mutex_exit(&sc->sc_iflock);
	}
	return error;
}


static uint8_t	BROADCAST_ADDR[ETHER_ADDR_LEN] =
			{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


static int
dse_set_multi(struct dse_softc *sc)
{
	scsi_dayna_ether_generic cmd_set_multi;
	struct ether_multistep step;
	struct ether_multi *enm;
	u_char *cp, *mybuf;
	int error, len;

	error = 0;

#ifdef DSE_DEBUG
	aprint_error_dev(sc->sc_dev, "dse_set_multi\n");
#endif

	mybuf = malloc(ETHER_ADDR_LEN * DSE_MCAST_MAX, M_DEVBUF, M_NOWAIT);
	if (mybuf == NULL) {
		error = EIO;
		goto l_end;
	}

	/*
	 * copy all entries to transfer buffer
	 */
	cp = mybuf;
	len = 0;
	ETHER_FIRST_MULTI(step, &(sc->sc_ethercom), enm);
	while ((len < (DSE_MCAST_MAX - 1)) && (enm != NULL)) {
		/* ### refer low side entry */
		memcpy(cp, enm->enm_addrlo, ETHER_ADDR_LEN);

		cp += ETHER_ADDR_LEN;
		len++;
		ETHER_NEXT_MULTI(step, enm);
	}

	/* add broadcast address as default */
	memcpy(cp, BROADCAST_ADDR, ETHER_ADDR_LEN);
	len++;

	len *= ETHER_ADDR_LEN;

	memset(&cmd_set_multi, 0, sizeof(cmd_set_multi));
	cmd_set_multi.opcode[0] = DAYNA_CMD_SET_MULTI;
	_lto2b(len, cmd_set_multi.length);

	error = dse_scsipi_cmd(sc->sc_periph,
	    (struct scsipi_generic*)&cmd_set_multi, sizeof(cmd_set_multi),
	    mybuf, len, DSE_RETRIES, DSE_TIMEOUT, NULL, XS_CTL_POLL | XS_CTL_DATA_OUT);

	free(mybuf, M_DEVBUF);

l_end:
	return error;
}


static void
dse_stop(struct dse_softc *sc)
{
	/* Don't schedule any reads */
	callout_stop(&sc->sc_recv_ch);

	/* Wait for the workqueues to finish */
	mutex_enter(&sc->sc_iflock);
	workqueue_wait(sc->sc_recv_wq, &sc->sc_recv_work);
	workqueue_wait(sc->sc_send_wq, &sc->sc_send_work);
	mutex_exit(&sc->sc_iflock);

	/* Abort any scsi cmds in progress */
	mutex_enter(chan_mtx(sc->sc_periph->periph_channel));
	scsipi_kill_pending(sc->sc_periph);
	mutex_exit(chan_mtx(sc->sc_periph->periph_channel));
}


/*
 * Process an ioctl request.
 */
static int
dse_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct dse_softc *sc;
	struct ifaddr *ifa;
	struct ifreq *ifr;
	struct sockaddr *sa;
	int error;

	error = 0;
	sc = ifp->if_softc;
	ifa = (struct ifaddr *)data;
	ifr = (struct ifreq *)data;

	switch (cmd) {
	case SIOCINITIFADDR:
		mutex_enter(&sc->sc_iflock);
		if ((error = dse_enable(sc)) != 0)
			break;
		ifp->if_flags |= IFF_UP;
		mutex_exit(&sc->sc_iflock);

#if 0
		if ((error = dse_set_media(sc, CMEDIA_AUTOSENSE)) != 0)
			break;
#endif

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			if ((error = dse_init(sc)) != 0)
				break;
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NETATALK
		case AF_APPLETALK:
			if ((error = dse_init(sc)) != 0)
				break;
			break;
#endif
		default:
			error = dse_init(sc);
			break;
		}
		break;


	case SIOCSIFADDR:
		mutex_enter(&sc->sc_iflock);
		error = dse_enable(sc);
		mutex_exit(&sc->sc_iflock);
		if (error != 0)
			break;
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			if ((error = dse_init(sc)) != 0)
				break;
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NETATALK
		case AF_APPLETALK:
			if ((error = dse_init(sc)) != 0)
				break;
			break;
#endif
		default:
			error = dse_init(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		/* XXX re-use ether_ioctl() */
		switch (ifp->if_flags & (IFF_UP | IFF_RUNNING)) {
		case IFF_RUNNING:
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			dse_stop(sc);
			mutex_enter(&sc->sc_iflock);
			ifp->if_flags &= ~IFF_RUNNING;
			dse_disable(sc);
			mutex_exit(&sc->sc_iflock);
			break;
		case IFF_UP:
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			mutex_enter(&sc->sc_iflock);
			error = dse_enable(sc);
			mutex_exit(&sc->sc_iflock);
			if (error)
				break;
			error = dse_init(sc);
			break;
		default:
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			mutex_enter(&sc->sc_iflock);
			if (sc->sc_enabled)
				error = dse_init(sc);
			mutex_exit(&sc->sc_iflock);
			break;
		}
#ifdef DSE_DEBUG
		if (ifp->if_flags & IFF_DEBUG)
			sc->sc_debug = 1;
		else
			sc->sc_debug = 0;
#endif
		break;

	case SIOCADDMULTI:
		if (sc->sc_enabled == 0) {
			error = EIO;
			break;
		}
		mutex_enter(&sc->sc_iflock);
		sa = sockaddr_dup(ifreq_getaddr(cmd, ifr), M_WAITOK);
		mutex_exit(&sc->sc_iflock);
		if (ether_addmulti(sa, &sc->sc_ethercom) == ENETRESET) {
			error = dse_set_multi(sc);
#ifdef DSE_DEBUG
			aprint_error_dev(sc->sc_dev, "add multi: %s\n",
				   ether_sprintf(ifr->ifr_addr.sa_data));
#endif
		} else
			error = 0;

		mutex_enter(&sc->sc_iflock);
		sockaddr_free(sa);
		mutex_exit(&sc->sc_iflock);

		break;

	case SIOCDELMULTI:
		if (sc->sc_enabled == 0) {
			error = EIO;
			break;
		}
		mutex_enter(&sc->sc_iflock);
		sa = sockaddr_dup(ifreq_getaddr(cmd, ifr), M_WAITOK);
		mutex_exit(&sc->sc_iflock);
		if (ether_delmulti(sa, &sc->sc_ethercom) == ENETRESET) {
			error = dse_set_multi(sc);
#ifdef DSE_DEBUG
			aprint_error_dev(sc->sc_dev, "delete multi: %s\n",
			    ether_sprintf(ifr->ifr_addr.sa_data));
#endif
		} else
			error = 0;

		mutex_enter(&sc->sc_iflock);
		sockaddr_free(sa);
		mutex_exit(&sc->sc_iflock);

		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		break;
	}


	return error;
}


/*
 * Enable the network interface.
 */
int
dse_enable(struct dse_softc *sc)
{
	struct scsipi_periph *periph = sc->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;
	int error = 0;

	if (sc->sc_enabled == 0) {
		if ((error = scsipi_adapter_addref(adapt)) == 0)
			sc->sc_enabled = 1;
		else
			aprint_error_dev(sc->sc_dev, "device enable failed\n");
	}

	return error;
}


/*
 * Disable the network interface.
 */
void
dse_disable(struct dse_softc *sc)
{
	struct scsipi_periph *periph = sc->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;
	if (sc->sc_enabled != 0) {
		scsipi_adapter_delref(adapt);
		sc->sc_enabled = 0;
	}
}


#define	DSEUNIT(z)	(minor(z))

/*
 * open the device.
 */
int
dseopen(dev_t dev, int flag, int fmt, struct lwp *l)
{
	int unit, error;
	struct dse_softc *sc;
	struct scsipi_periph *periph;
	struct scsipi_adapter *adapt;

	unit = DSEUNIT(dev);
	sc = device_lookup_private(&dse_cd, unit);
	if (sc == NULL)
		return ENXIO;

	periph = sc->sc_periph;
	adapt = periph->periph_channel->chan_adapter;

	if ((error = scsipi_adapter_addref(adapt)) != 0)
		return error;

	SC_DEBUG(periph, SCSIPI_DB1,
	    ("scopen: dev=0x%"PRIx64" (unit %d (of %d))\n", dev, unit,
	    dse_cd.cd_ndevs));

	periph->periph_flags |= PERIPH_OPEN;

	SC_DEBUG(periph, SCSIPI_DB3, ("open complete\n"));

	return 0;
}


/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
int
dseclose(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct dse_softc *sc = device_lookup_private(&dse_cd, DSEUNIT(dev));
	struct scsipi_periph *periph = sc->sc_periph;
	struct scsipi_adapter *adapt = periph->periph_channel->chan_adapter;

	SC_DEBUG(sc->sc_periph, SCSIPI_DB1, ("closing\n"));

	scsipi_wait_drain(periph);

	scsipi_adapter_delref(adapt);
	periph->periph_flags &= ~PERIPH_OPEN;

	return 0;
}


/*
 * Perform special action on behalf of the user
 * Only does generic scsi ioctls.
 */
int
dseioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct dse_softc *sc = device_lookup_private(&dse_cd, DSEUNIT(dev));

	return (scsipi_do_ioctl(sc->sc_periph, dev, cmd, addr, flag, l));
}

