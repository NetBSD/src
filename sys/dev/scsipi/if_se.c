/*	$NetBSD: if_se.c,v 1.4 1997/04/02 02:29:34 mycroft Exp $	*/

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

/*
 * Written by Ian Dall <ian.dall@dsto.defence.gov.au> Feb 3, 1997
 */
/*
 * Driver for Cabletron EA41x scsi ethernet adaptor.
 *
 * This is a weird device! It doesn't conform to the scsi spec in much
 * at all. About the only standard command supported in inquiry. Most
 * commands are 6 bytes long, but the recv data is only 1 byte.  Data
 * must be received by periodically polling the device with the recv
 * command.
 *
 * This driver is also a bit unusual. It must look like a network
 * interface and it must also appear to be a scsi device to the scsi
 * system. Hence there are cases where there are two entry points. eg
 * sestart is to be called from the scsi subsytem and se_ifstart from
 * the network interface subsystem.  In addition, to facilitate scsi
 * commands issued by userland programs, there are open, close and
 * ioctl entry points. This allows a user program to, for example,
 * display the ea41x stats and download new code into the adaptor ---
 * functions which can't be performed through the ifconfig interface.
 * Normal operation does not require any special userland program. 
 */

#include "bpfilter.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
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

#include <scsi/scsi_all.h>
#include <scsi/scsi_ctron_ether.h>
#include <scsi/scsiconf.h>

#include <sys/mbuf.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if defined(CCITT) && defined(LLC)
#include <sys/socketvar.h>
#include <netccitt/x25.h>
#include <netccitt/pk.h>
#include <netccitt/pk_var.h>
#include <netccitt/pk_extern.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#define SETIMEOUT	1000
#define	SEOUTSTANDING	4
#define	SERETRIES	4
#define SE_PREFIX	4
#define ETHER_CRC	4
#define SEMINSIZE	60

/* Make this big enough for an ETHERMTU packet in promiscuous mode. */
#define MAX_SNAP	(ETHERMTU + sizeof(struct ether_header) + \
			 SE_PREFIX + ETHER_CRC)
#define RBUF_LEN     (16 * 1024)

     /* se_poll and se_poll0 are the normal polling rate and the
      * minimum polling rate respectively. If se_poll0 should be
      * chosen so that at maximum ethernet speed, we will read nearly
      * full buffers. se_poll should be chosen for reasonable maximum
      * latency. 
      */
#define SE_POLL 40		/* default in milliseconds */
#define SE_POLL0 20		/* Default in milliseconds */
int se_poll = 0; /* Delay in ticks set at attach time */
int se_poll0 = 0;

#define	PROTOCMD(p, d) \
	((d) = (p))

#define	PROTOCMD_DECL(name, val) \
	static const struct scsi_ctron_ether_generic name = val

#define	PROTOCMD_DECL_SPECIAL(name, val) \
	static const struct __CONCAT(scsi_,name) name = val

/* Command initializers for commands using scsi_ctron_ether_generic */
PROTOCMD_DECL(ctron_ether_send, {CTRON_ETHER_SEND});
PROTOCMD_DECL(ctron_ether_add_proto, {CTRON_ETHER_ADD_PROTO});
PROTOCMD_DECL(ctron_ether_get_addr, {CTRON_ETHER_GET_ADDR});
PROTOCMD_DECL(ctron_ether_set_media, {CTRON_ETHER_SET_MEDIA});
PROTOCMD_DECL(ctron_ether_set_addr, {CTRON_ETHER_SET_ADDR});
PROTOCMD_DECL(ctron_ether_set_multi, {CTRON_ETHER_SET_MULTI});
PROTOCMD_DECL(ctron_ether_remove_multi, {CTRON_ETHER_REMOVE_MULTI});

/* Command initializers for commands using their own structures */
PROTOCMD_DECL_SPECIAL(ctron_ether_recv, {CTRON_ETHER_RECV});
PROTOCMD_DECL_SPECIAL(ctron_ether_set_mode, {CTRON_ETHER_SET_MODE});

struct se_softc {
	struct device sc_dev;
	struct ethercom sc_ethercom;	/* Ethernet common part */
	struct scsi_link *sc_link;	/* contains our targ, lun, etc. */
	char *sc_tbuf;
	char *sc_rbuf;
	int protos;
#define PROTO_IP	0x1
#define PROTO_ARP	0x2
#define PROTO_REVARP	0x4
	int sc_debug;
	int sc_flags;
#define SE_NEED_RECV 0x1
};

cdev_decl(se);

#ifdef __BROKEN_INDIRECT_CONFIG
int	sematch __P((struct device *, void *, void *));
#else
int	sematch __P((struct device *, struct cfdata *, void *));
#endif
void	seattach __P((struct device *, struct device *, void *));

void	se_ifstart __P((struct ifnet *));
void	sestart __P((void *));

static	void sedone __P((struct scsi_xfer *));
int	se_ioctl __P((struct ifnet *, u_long, caddr_t));
void	sewatchdog __P((struct ifnet *));

static void se_recv __P((void *));
static struct mbuf *se_get __P((struct se_softc *, char *, int));
static int se_read __P((struct se_softc *, char *, int));
void sewatchdog __P((struct ifnet *));
static int se_reset __P((struct se_softc *));
static int se_add_proto __P((struct se_softc *, int));
static int se_get_addr __P((struct se_softc *, u_int8_t *));
static int se_set_media __P((struct se_softc *, int));
static int se_init __P((struct se_softc *));
static int se_set_multi __P((struct se_softc *, u_int8_t *));
static int se_remove_multi __P((struct se_softc *, u_int8_t *));
#if 0
static int sc_set_all_multi __P((struct se_softc *, int));
#endif
static void se_stop __P((struct se_softc *));
static __inline__  int se_scsi_cmd __P((struct scsi_link *sc_link,
			    struct scsi_generic *scsi_cmd,
			    int cmdlen, u_char *data_addr, int datalen,
			    int retries, int timeout, struct buf *bp,
			    int flags));
static void se_delayed_ifstart __P((void *));
static int se_set_mode(struct se_softc *, int, int);

struct cfattach se_ca = {
	sizeof(struct se_softc), sematch, seattach
};

struct cfdriver se_cd = {
	NULL, "se", DV_IFNET
};

struct scsi_device se_switch = {
	NULL,			/* Use default error handler */
	sestart,		/* have a queue, served by this */
	NULL,			/* have no async handler */
	sedone,			/* deal with stats at interrupt time */
};

struct scsi_inquiry_pattern se_patterns[] = {
	{T_PROCESSOR, T_FIXED,
	 "CABLETRN",         "EA412/14/19",                 ""},
	{T_PROCESSOR, T_FIXED,
	 "Cabletrn",         "EA412/14/19",                 ""},
};

static __inline__ u_int16_t ether_cmp __P((void *, void *));

/*
 * Compare two Ether/802 addresses for equality, inlined and
 * unrolled for speed.
 * Note: use this like bcmp()
 */
static __inline__ u_int16_t
ether_cmp(one, two)
	void *one, *two;
{
	register u_int16_t *a = (u_int16_t *) one;
	register u_int16_t *b = (u_int16_t *) two;
	register u_int16_t diff;

	diff = (a[0] - b[0]) | (a[1] - b[1]) | (a[2] - b[2]);

	return (diff);
}

#define ETHER_CMP	ether_cmp

int
sematch(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct scsibus_attach_args *sa = aux;
	int priority;

	(void)scsi_inqmatch(sa->sa_inqbuf,
	    (caddr_t)se_patterns, sizeof(se_patterns)/sizeof(se_patterns[0]),
	    sizeof(se_patterns[0]), &priority);
	return (priority);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * a device suitable for this driver.
 */
void
seattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct se_softc *sc = (void *)self;
	struct scsibus_attach_args *sa = aux;
	struct scsi_link *sc_link = sa->sa_sc_link;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int8_t myaddr[ETHER_ADDR_LEN];

	SC_DEBUG(sc_link, SDEV_DB2, ("seattach: "));

	/*
	 * Store information needed to contact our base driver
	 */
	sc->sc_link = sc_link;
	sc_link->device = &se_switch;
	sc_link->device_softc = sc;
	if (sc_link->openings > SEOUTSTANDING)
		sc_link->openings = SEOUTSTANDING;

	se_poll = (SE_POLL * hz) / 1000;
	se_poll = se_poll? se_poll: 1;
	se_poll0 = (SE_POLL0 * hz) / 1000;
	se_poll0 = se_poll0? se_poll0: 1;

	/*
	 * Initialize and attach a buffer
	 */
	sc->sc_tbuf = malloc(ETHERMTU + sizeof(struct ether_header),
			     M_DEVBUF, M_NOWAIT);
	if(sc->sc_tbuf == 0)
		panic("seattach: can't allocate transmit buffer");

	sc->sc_rbuf = malloc(RBUF_LEN, M_DEVBUF, M_NOWAIT);/* A Guess */
	if(sc->sc_rbuf == 0)
		panic("seattach: can't allocate receive buffer");

	se_get_addr(sc, myaddr);

	/* Initialize ifnet structure. */
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = se_ifstart;
	ifp->if_ioctl = se_ioctl;
	ifp->if_watchdog = sewatchdog;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, myaddr);

#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
}


static __inline__ int se_scsi_cmd(sc_link, scsi_cmd, cmdlen, data_addr, datalen,
		       retries, timeout, bp, flags)
	struct scsi_link *sc_link;
	struct scsi_generic *scsi_cmd;
	int cmdlen;
	u_char *data_addr;
	int datalen;
	int retries;
	int timeout;
	struct buf *bp;
	int flags;
{
	int error;
	int s = splbio();
	error = scsi_scsi_cmd(sc_link, scsi_cmd, cmdlen, data_addr, datalen,
		      retries, timeout, bp, flags);
	splx(s);
	return error;
}
/* Start routine for calling from scsi sub system */
void sestart(void *v)
{
	struct se_softc *sc = (struct se_softc *) v;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int s = splnet();
	se_ifstart(ifp);
	(void) splx(s);
}

static void se_delayed_ifstart(void *v)
{
	struct ifnet *ifp = v;
	int s = splnet();
	ifp->if_flags &= ~IFF_OACTIVE;
	se_ifstart(ifp);
	splx(s);
}

/*
 * Start transmission on the interface.
 * Always called at splnet().
 */
void
se_ifstart(ifp)
	struct ifnet *ifp;
{
	struct se_softc *sc = ifp->if_softc;
	struct scsi_ctron_ether_generic send_cmd;
	struct mbuf *m, *m0;
	int len, error;
	u_char *cp;

	/* Don't transmit if interface is busy or not running */
	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	IF_DEQUEUE(&ifp->if_snd, m0);
	if (m0 == 0)
		return;
#if NBPFILTER > 0
	/* If BPF is listening on this interface, let it see the
	 * packet before we commit it to the wire.
	 */
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m0);
#endif

	/* We need to use m->m_pkthdr.len, so require the header */
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("ctscstart: no header mbuf");
	len = m0->m_pkthdr.len;

	/* Mark the interface busy. */
	ifp->if_flags |= IFF_OACTIVE;

	/* Chain; copy into linear buffer we allocated at attach time. */
	cp = sc->sc_tbuf;
	for (m = m0; m != NULL; ) {
		bcopy(mtod(m, u_char *), cp, m->m_len);
		cp += m->m_len;
		MFREE(m, m0);
		m = m0;
	}
	if (len < SEMINSIZE) {
#ifdef SEDEBUG
		if (sc->sc_debug)
			printf("se: packet size %d (%d) < %d\n", len,
			       cp - (u_char *)sc->sc_tbuf, SEMINSIZE);
#endif
		bzero(cp, SEMINSIZE - len);
		len = SEMINSIZE;
	}

	/* Fill out SCSI command. */
	PROTOCMD(ctron_ether_send, send_cmd);
	_lto2b(len, send_cmd.length);

	/* Send command to device. */
	error = se_scsi_cmd(sc->sc_link,
			    (struct scsi_generic *)&send_cmd, sizeof(send_cmd),
			    sc->sc_tbuf, len, SERETRIES,
			    SETIMEOUT, NULL, SCSI_NOSLEEP|SCSI_DATA_OUT);
	if (error) {
		printf("%s: not queued, error %d\n",
		       sc->sc_dev.dv_xname, error);
		ifp->if_oerrors++;
		ifp->if_flags &= ~IFF_OACTIVE;
	} else
		ifp->if_opackets++;
	if (sc->sc_flags & SE_NEED_RECV) {
		sc->sc_flags &= ~SE_NEED_RECV;
		se_recv((void *) sc);
	}
}


/*
 * Called from the scsibus layer via our scsi device switch.
 */
static void
sedone(xs)
	struct scsi_xfer *xs;
{
	int error;
	struct se_softc *sc = xs->sc_link->device_softc;
	struct scsi_generic *cmd = xs->cmd;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int s;

	error = !(xs->error == XS_NOERROR);

	s = splnet();
	if(IS_SEND(cmd)) {
		if (xs->error == XS_BUSY) {
			printf("se: busy, retry txmit\n");
			timeout(se_delayed_ifstart, ifp, hz);
		} else {
			ifp->if_flags &= ~IFF_OACTIVE;
			/* the generic scsi_done will call
			 * sestart (through scsi_free_xs).
			 */
		}
	} else if(IS_RECV(cmd)) {
		/* RECV complete */
		/* pass data up. reschedule a recv */
		/* scsi_free_xs will call start. Harmless. */
		if (error) {
			/* Reschedule after a delay */
			timeout(se_recv, (void *)sc, se_poll);
		} else {
			int n;
			n = se_read(sc, xs->data, xs->datalen - xs->resid);

			if(n > 0) {
#ifdef SEDEBUG
				if (sc->sc_debug)
					printf("sedone: received %d packets \n", n);
#endif
				if(ifp->if_snd.ifq_head)
					/* Output is
					 * pending. Do next
					 * recv after the next
					 * send. */
					sc->sc_flags |= SE_NEED_RECV;
				else {
					timeout(se_recv, (void *)sc, se_poll0);
				}
			} else {
				/* Reschedule after a delay */
				timeout(se_recv, (void *)sc, se_poll);
			}
		}
	}
	splx(s);
}

static void se_recv(v)
     void *v;
{
	/* do a recv command */
	struct se_softc *sc = (struct se_softc *) v;
	struct scsi_ctron_ether_recv recv_cmd;
	int error;

	PROTOCMD(ctron_ether_recv, recv_cmd);

	error = se_scsi_cmd(sc->sc_link,
				  (struct scsi_generic *)&recv_cmd,
				  sizeof(recv_cmd),
				  sc->sc_rbuf, RBUF_LEN, SERETRIES,
				  SETIMEOUT, NULL, SCSI_NOSLEEP|SCSI_DATA_IN);
	if (error)
		timeout(se_recv, (void *)sc, se_poll);
	
}

/*
 * We copy the data into mbufs.  When full cluster sized units are present
 * we copy into clusters.
 */
static struct mbuf *
se_get(sc, data, totlen)
	struct se_softc *sc;
	char *data;
	int totlen;
{
	struct mbuf *m;
	struct mbuf *top, **mp;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int len, pad;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return (0);
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	pad = ALIGN(sizeof(struct ether_header)) - sizeof(struct ether_header);
	m->m_data += pad;
	len = MHLEN - pad;
	top = 0;
	mp = &top;

	while (totlen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return 0;
			}
			len = MLEN;
		}
		if (top && totlen >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if (m->m_flags & M_EXT)
				len = MCLBYTES;
		}
		m->m_len = len = min(totlen, len);
		bcopy(data, mtod(m, caddr_t), len);
		data += len;
		totlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	return (top);
}

/*
 * Pass packets to higher levels.
 */
static int
se_read(sc, data, datalen)
	register struct se_softc *sc;
	char *data;
	int datalen;
{
	struct mbuf *m;
	struct ether_header *eh;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int n;

	n = 0;
	while (datalen >= 2) {
		int len = _2btol(data);
		data += 2;
		datalen -= 2;

		if (len == 0)
			break;
#ifdef SEDEBUG
		if (sc->sc_debug) {
			printf("se_read: datalen = %d, packetlen = %d, proto = 0x%04x\n", datalen, len,
			 ntohs(((struct ether_header *)data)->ether_type));
		}
#endif
		if (len <= sizeof(struct ether_header) ||
		    len > MAX_SNAP) {
#ifdef SEDEBUG
			printf("%s: invalid packet size %d; dropping\n",
			       sc->sc_dev.dv_xname, len);
#endif
			ifp->if_ierrors++;
			goto next_packet;
		}

		/* Don't need crc. Must keep ether header for BPF */
		m = se_get(sc, data, len - ETHER_CRC);
		if (m == 0) {
#ifdef SEDEBUG
			if (sc->sc_debug) {
				printf("se_read: se_get returned null\n");
			}
#endif
			ifp->if_ierrors++;
			goto next_packet;
		}
		if ((ifp->if_flags & IFF_PROMISC) != 0) {
			m_adj(m, SE_PREFIX);	
		}
		ifp->if_ipackets++;

		/* We assume that the header fit entirely in one mbuf. */
		eh = mtod(m, struct ether_header *);

#if NBPFILTER > 0
		/*
		 * Check if there's a BPF listener on this interface.
		 * If so, hand off the raw packet to BPF.
		 */
		if (ifp->if_bpf) {
			bpf_mtap(ifp->if_bpf, m);

			/* Note that the interface cannot be in
			 * promiscuous mode if there are no BPF
			 * listeners.  And if we are in promiscuous
			 * mode, we have to check if this packet is
			 * really ours.
			 */
			if ((ifp->if_flags & IFF_PROMISC) != 0 &&
			    (eh->ether_dhost[0] & 1) == 0 && /* !mcast and !bcast */
			    ETHER_CMP(eh->ether_dhost, LLADDR(ifp->if_sadl))) {
				m_freem(m);
				goto next_packet;
			}
		}
#endif

		/* Pass the packet up, with the ether header sort-of removed. */
		m_adj(m, sizeof(struct ether_header));
		ether_input(ifp, eh, m);

	next_packet:
		data += len;
		datalen -= len;
		n++;
	}
	return n;
}


void
sewatchdog(ifp)
	struct ifnet *ifp;
{
	struct se_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	++ifp->if_oerrors;

	se_reset(sc);
}

static int
se_reset(sc)
	struct se_softc *sc;
{
	int error = 0;
	int s = splnet();
#if 0
	/* Maybe we don't *really* want to reset the entire bus
	 * because the ctron isn't working. We would like to send a
	 * "BUS DEVICE RESET" message, but don't think the ctron
	 * understands it.
	 */
	error = se_scsi_cmd(sc->sc_link, 0, 0, 0, 0, SERETRIES, 2000, NULL,
	    SCSI_RESET);
#endif
	error = se_init(sc);
	splx(s);
	return error;
}

static int se_add_proto(sc, proto)
     struct se_softc *sc;
     int proto;
{
	int error = 0;
	struct scsi_ctron_ether_generic add_proto_cmd;
	u_int8_t data[2];
	_lto2b(proto, data);
#ifdef SEDEBUG
	if (sc->sc_debug)
		printf("se: adding proto 0x%02x%02x\n", data[0], data[1]);
#endif

	PROTOCMD(ctron_ether_add_proto, add_proto_cmd);
	_lto2b(sizeof(data), add_proto_cmd.length);
	error = se_scsi_cmd(sc->sc_link,
			      (struct scsi_generic *) &add_proto_cmd,
			      sizeof(add_proto_cmd),
			      data,
			      sizeof(data),
			      SERETRIES, SETIMEOUT, NULL,
			      SCSI_DATA_OUT);
	return error;
	
}

static int se_get_addr(sc, myaddr)
     struct se_softc *sc;
     u_int8_t *myaddr;
{
	int error = 0;
	struct scsi_ctron_ether_generic get_addr_cmd;

	PROTOCMD(ctron_ether_get_addr, get_addr_cmd);
	_lto2b(ETHER_ADDR_LEN, get_addr_cmd.length);
	error = se_scsi_cmd(sc->sc_link,
			      (struct scsi_generic *) &get_addr_cmd,
			      sizeof(get_addr_cmd),
			      myaddr, ETHER_ADDR_LEN,
			      SERETRIES, SETIMEOUT, NULL,
			      SCSI_DATA_IN);
	printf("%s: ethernet address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(myaddr));
	return error;
}


static int se_set_media(sc, type)
     struct se_softc *sc;
     int type;
{
	int error = 0;
	struct scsi_ctron_ether_generic set_media_cmd;

	PROTOCMD(ctron_ether_set_media, set_media_cmd);
	set_media_cmd.byte3 = type;
	error = se_scsi_cmd(sc->sc_link,
			      (struct scsi_generic *) &set_media_cmd,
			      sizeof(set_media_cmd),
			      0,
			      0,
			      SERETRIES, SETIMEOUT, NULL,
			      0);
	return error;
}

static int se_set_mode(sc, len, mode)
     struct se_softc *sc;
     int len;
     int mode;
{
	int error = 0;
	struct scsi_ctron_ether_set_mode set_mode_cmd;

	PROTOCMD(ctron_ether_set_mode, set_mode_cmd);
	set_mode_cmd.mode = mode;
	_lto2b(len, set_mode_cmd.length);
	error = se_scsi_cmd(sc->sc_link,
			      (struct scsi_generic *) &set_mode_cmd,
			      sizeof(set_mode_cmd),
			      0,
			      0,
			      SERETRIES, SETIMEOUT, NULL,
			      0);
	return error;
}
	


static int se_init(sc)
     struct se_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct scsi_ctron_ether_generic set_addr_cmd;
	int error;

#if NBPFILTER > 0
	if (ifp->if_flags & IFF_PROMISC) {
		error = se_set_mode(sc, MAX_SNAP, 1);
	}
	else
#endif
		error = se_set_mode(sc, ETHERMTU + sizeof(struct ether_header),
				    0);
	if(error != 0)
		return error;
	
	PROTOCMD(ctron_ether_set_addr, set_addr_cmd);
	_lto2b(ETHER_ADDR_LEN, set_addr_cmd.length);
	error = se_scsi_cmd(sc->sc_link,
			      (struct scsi_generic *) &set_addr_cmd,
			      sizeof(set_addr_cmd),
			      LLADDR(ifp->if_sadl), ETHER_ADDR_LEN,
			      SERETRIES, SETIMEOUT, NULL,
			      SCSI_DATA_OUT);
	if(error != 0) {
		return error;
	}

	if ((sc->protos & PROTO_IP) &&
	    (error = se_add_proto(sc, ETHERTYPE_IP)) != 0)
		return error;
	if ((sc->protos & PROTO_ARP) &&
	    (error = se_add_proto(sc, ETHERTYPE_ARP)) != 0)
		return error;
	if ((sc->protos & PROTO_REVARP) &&
	    (error = se_add_proto(sc, ETHERTYPE_REVARP)) != 0)
		return error;

	if((ifp->if_flags & (IFF_RUNNING|IFF_UP)) == IFF_UP) {
		ifp->if_flags |= IFF_RUNNING;
		se_recv(sc);
		ifp->if_flags &= ~IFF_OACTIVE;
		se_ifstart(ifp);
	}
	return error;
}

static int se_set_multi(sc, addr)
     struct se_softc *sc;
     u_int8_t *addr;
{
	struct scsi_ctron_ether_generic set_multi_cmd;
	int error;

	if (sc->sc_debug)
		printf("%s: set_set_multi: %s\n", sc->sc_dev.dv_xname,
		    ether_sprintf(addr));

	PROTOCMD(ctron_ether_set_multi, set_multi_cmd);
	_lto2b(sizeof(addr), set_multi_cmd.length);
	error = se_scsi_cmd(sc->sc_link,
			      (struct scsi_generic *) &set_multi_cmd,
			      sizeof(set_multi_cmd),
			      addr,
			      sizeof(addr),
			      SERETRIES, SETIMEOUT, NULL,
			      SCSI_DATA_OUT);
	return error;
}

static int se_remove_multi(sc, addr)
     struct se_softc *sc;
     u_int8_t *addr;
{
	struct scsi_ctron_ether_generic remove_multi_cmd;
	int error;

	if (sc->sc_debug)
		printf("%s: se_remove_multi: %s\n", sc->sc_dev.dv_xname,
		    ether_sprintf(addr));

	PROTOCMD(ctron_ether_remove_multi, remove_multi_cmd);
	_lto2b(sizeof(addr), remove_multi_cmd.length);
	error = se_scsi_cmd(sc->sc_link,
			      (struct scsi_generic *) &remove_multi_cmd,
			      sizeof(remove_multi_cmd),
			      addr,
			      sizeof(addr),
			      SERETRIES, SETIMEOUT, NULL,
			      SCSI_DATA_OUT);
	return error;
}

#if 0	/* not used  --thorpej */
static int sc_set_all_multi(sc, set)
     struct se_softc *sc;
     int set;
{
	int error = 0;
	u_int8_t *addr;
	struct ethercom *ac = &sc->sc_ethercom;
	struct ether_multi *enm;
	struct ether_multistep step;
	ETHER_FIRST_MULTI(step, ac, enm);
	while (enm != NULL) {
		if (ETHER_CMP(enm->enm_addrlo, enm->enm_addrhi)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			/* We have no way of adding a range to this device.
			 * stepping through all addresses in the range is
			 * typically not possible. The only real alternative
			 * is to go into promicuous mode and filter by hand.
			 */
			return ENODEV;

		}

		addr = enm->enm_addrlo;
		if((error = set? se_set_multi(sc, addr): se_remove_multi(sc, addr)) != 0)
			return error;
		ETHER_NEXT_MULTI(step, enm);
	}
	return error;
}
#endif /* not used */

static void se_stop(sc)
     struct se_softc *sc;
{
	/* Don't schedule any reads */
	untimeout(se_recv, sc);
	
	/* How can we abort any scsi cmds in progress? */
}


/*
 * Process an ioctl request.
 */
int
se_ioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	register struct se_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		if ((error = se_set_media(sc, CMEDIA_AUTOSENSE) != 0))
			return error;
		
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			sc->protos |= (PROTO_IP | PROTO_ARP | PROTO_REVARP);
			if ((error = se_init(sc)) != 0)
				break;
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host =
				    *(union ns_host *)LLADDR(ifp->if_sadl);
			else
				bcopy(ina->x_host.c_host,
				    LLADDR(ifp->if_sadl), ETHER_ADDR_LEN);
			/* Set new address. */

			error = se_init(sc);
			break;
		    }
#endif
		default:
			error = se_init(sc);
			break;
		}
		break;

#if defined(CCITT) && defined(LLC)
	case SIOCSIFCONF_X25:
		ifp->if_flags |= IFF_UP;
		ifa->ifa_rtrequest = cons_rtrequest; /* XXX */
		error = x25_llcglue(PRC_IFUP, ifa->ifa_addr);
		if (error == 0)
			error = se_init(sc);
		break;
#endif /* CCITT && LLC */

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			se_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    	   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			error = se_init(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			error = se_init(sc);
		}
#ifdef SEDEBUG
		if (ifp->if_flags & IFF_DEBUG)
			sc->sc_debug = 1;
		else
			sc->sc_debug = 0;
#endif
		break;

	case SIOCADDMULTI:
		if (ether_addmulti(ifr, &sc->sc_ethercom) == ENETRESET) {
			error = se_set_multi(sc, ifr->ifr_addr.sa_data);
		} else {
			error = 0;
		}
		break;
	case SIOCDELMULTI:
		if (ether_delmulti(ifr, &sc->sc_ethercom) == ENETRESET) {
			error = se_remove_multi(sc, ifr->ifr_addr.sa_data);
		} else {
			error = 0;
		}
		break;

	default:

		error = EINVAL;
		break;
	}

	splx(s);
	return (error);
}

#define	SEUNIT(z)	(minor(z))
/*
 * open the device.
 */
int
seopen(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	int unit;
	struct se_softc *sc;
	struct scsi_link *sc_link;

	unit = SEUNIT(dev);
	if (unit >= se_cd.cd_ndevs)
		return ENXIO;
	sc = se_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;
		
	sc_link = sc->sc_link;

	SC_DEBUG(sc_link, SDEV_DB1,
	    ("scopen: dev=0x%x (unit %d (of %d))\n", dev, unit, se_cd.cd_ndevs));

	sc_link->flags |= SDEV_OPEN;

	SC_DEBUG(sc_link, SDEV_DB3, ("open complete\n"));
	return 0;
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
 */
int
seclose(dev, flag, fmt, p)
	dev_t dev;
	int flag, fmt;
	struct proc *p;
{
	struct se_softc *sc = se_cd.cd_devs[SEUNIT(dev)];

	SC_DEBUG(sc->sc_link, SDEV_DB1, ("closing\n"));
	sc->sc_link->flags &= ~SDEV_OPEN;

	return 0;
}

/*
 * Perform special action on behalf of the user
 * Only does generic scsi ioctls.
 */
int
seioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	register struct se_softc *sc = se_cd.cd_devs[SEUNIT(dev)];

	return scsi_do_ioctl(sc->sc_link, dev, cmd, addr, flag, p);
}
