/*	$NetBSD: if_fwip.c,v 1.8 2007/04/21 15:27:44 kiyohara Exp $	*/
/*-
 * Copyright (c) 2004
 *	Doug Rabson
 * Copyright (c) 2002-2003
 * 	Hidetoshi Shimokawa. All rights reserved.
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
 *
 *	This product includes software developed by Hidetoshi Shimokawa.
 *
 * 4. Neither the name of the author nor the names of its contributors
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
 * $FreeBSD: /repoman/r/ncvs/src/sys/dev/firewire/if_fwip.c,v 1.14 2007/03/16 05:39:33 simokawa Exp $
 */

#ifdef HAVE_KERNEL_OPTION_HEADERS
#include "opt_device_polling.h"
#include "opt_inet.h"
#endif

#if defined(__FreeBSD__)
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/taskqueue.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <machine/bus.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/firewire.h>
#include <net/if_arp.h>
#include <net/if_types.h>
#ifdef __DragonFly__
#include <bus/firewire/fw_port.h>
#include <bus/firewire/firewire.h>
#include <bus/firewire/firewirereg.h>
#include "if_fwipvar.h"
#else
#include <dev/firewire/fw_port.h>
#include <dev/firewire/firewire.h>
#include <dev/firewire/firewirereg.h>
#include <dev/firewire/iec13213.h>
#include <dev/firewire/if_fwipvar.h>
#endif
#elif defined(__NetBSD__)
#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_ieee1394.h>
#include <net/if_types.h>

#include <dev/ieee1394/fw_port.h>
#include <dev/ieee1394/firewire.h>
#include <dev/ieee1394/firewirereg.h>
#include <dev/ieee1394/iec13213.h>
#include <dev/ieee1394/if_fwipvar.h>
#endif

/*
 * We really need a mechanism for allocating regions in the FIFO
 * address space. We pick a address in the OHCI controller's 'middle'
 * address space. This means that the controller will automatically
 * send responses for us, which is fine since we don't have any
 * important information to put in the response anyway.
 */
#define INET_FIFO	0xfffe00000000LL

#if defined(__FreeBSD__)
#define FWIPDEBUG	if (fwipdebug) if_printf
#elif defined(__NetBSD__)
#define FWIPDEBUG(ifp, fmt, ...) \
	if (fwipdebug) {\
		aprint_normal("%s: ", (ifp)->if_xname); \
		aprint_normal((fmt) ,##__VA_ARGS__); \
	}
#endif
#define TX_MAX_QUEUE	(FWMAXQUEUE - 1)

#if defined(__NetBSD__)
int fwipmatch (struct device *, struct cfdata *, void *);
void fwipattach (struct device *, struct device *, void *);
int fwipdetach (struct device *, int);
int fwipactivate (struct device *, enum devact);

#endif  
/* network interface */
static void fwip_start (struct ifnet *);
static int fwip_ioctl (struct ifnet *, u_long, void *);
IF_INIT(fwip);
IF_STOP(fwip);

static void fwip_post_busreset (void *);
static void fwip_output_callback (struct fw_xfer *);
static void fwip_async_output (struct fwip_softc *, struct ifnet *);
#if defined(__FreeBSD__)
static void fwip_start_send (void *, int);
#endif
static void fwip_stream_input (struct fw_xferq *);
static void fwip_unicast_input(struct fw_xfer *);

static int fwipdebug = 0;
static int broadcast_channel = 0xc0 | 0x1f; /*  tag | channel(XXX) */
static int tx_speed = 2;
static int rx_queue_len = FWMAXQUEUE;

#if defined(__FreeBSD__)
MALLOC_DEFINE(M_FWIP, "if_fwip", "IP over FireWire interface");
SYSCTL_INT(_debug, OID_AUTO, if_fwip_debug, CTLFLAG_RW, &fwipdebug, 0, "");
SYSCTL_DECL(_hw_firewire);
SYSCTL_NODE(_hw_firewire, OID_AUTO, fwip, CTLFLAG_RD, 0,
	"Firewire ip subsystem");
SYSCTL_INT(_hw_firewire_fwip, OID_AUTO, rx_queue_len, CTLFLAG_RW, &rx_queue_len,
	0, "Length of the receive queue");

TUNABLE_INT("hw.firewire.fwip.rx_queue_len", &rx_queue_len);
#elif defined(__NetBSD__)
MALLOC_DEFINE(M_FWIP, "if_fwip", "IP over IEEE1394 interface");
/*
 * Setup sysctl(3) MIB, hw.fwip.*
 *
 * TBD condition CTLFLAG_PERMANENT on being an LKM or not
 */
SYSCTL_SETUP(sysctl_fwip, "sysctl fwip(4) subtree setup")
{
	int rc, fwip_node_num;
	const struct sysctlnode *node;

	if ((rc = sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_EOL)) != 0) {
		goto err;
	}

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "fwip",
	    SYSCTL_DESCR("fwip controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}
	fwip_node_num = node->sysctl_num;

	/* fwip RX queue length */
	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT,
	    "rx_queue_len", SYSCTL_DESCR("Length of the receive queue"),
	    NULL, 0, &rx_queue_len,
	    0, CTL_HW, fwip_node_num, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}

	/* fwip RX queue length */
	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT | CTLFLAG_READWRITE, CTLTYPE_INT,
	    "if_fwip_debug", SYSCTL_DESCR("fwip driver debug flag"),
	    NULL, 0, &fwipdebug,
	    0, CTL_HW, fwip_node_num, CTL_CREATE, CTL_EOL)) != 0) {
		goto err;
	}

	return;

err:
	printf("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}
#endif

#ifdef DEVICE_POLLING
static poll_handler_t fwip_poll;

static void
fwip_poll(struct ifnet *ifp, enum poll_cmd cmd, int count)
{
	struct fwip_softc *fwip;
	struct firewire_comm *fc;

	if (!(ifp->if_drv_flags & IFF_DRV_RUNNING))
		return;

	fwip = ((struct fwip_eth_softc *)ifp->if_softc)->fwip;
	fc = fwip->fd.fc;
	fc->poll(fc, (cmd == POLL_AND_CHECK_STATUS)?0:1, count);
}
#endif /* DEVICE_POLLING */
#if defined(__FreeBSD__)
static void
fwip_identify(driver_t *driver, device_t parent)
{
	BUS_ADD_CHILD(parent, 0, "fwip", device_get_unit(parent));
}

static int
fwip_probe(device_t dev)
{
	device_t pa;

	pa = device_get_parent(dev);
	if(device_get_unit(dev) != device_get_unit(pa)){
		return(ENXIO);
	}

	device_set_desc(dev, "IP over FireWire");
	return (0);
}
#elif defined(__NetBSD__)
int
fwipmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct fw_attach_args *fwa = aux;

	if (strcmp(fwa->name, "fwip") == 0)
		return (1);
	return (0);
}
#endif

FW_ATTACH(fwip)
{
	FW_ATTACH_START(fwip, fwip, fwa);
	FWIP_ATTACH_START;
	struct ifnet *ifp;
	int s;

	FWIP_ATTACH_SETUP;

	ifp = fwip->fw_softc.fwip_ifp;
	if (ifp == NULL)
		FW_ATTACH_RETURN(ENOSPC);

	/* XXX */
	fwip->dma_ch = -1;

	fwip->fd.fc = fwa->fc;
	if (tx_speed < 0)
		tx_speed = fwip->fd.fc->speed;

	fwip->fd.post_explore = NULL;
	fwip->fd.post_busreset = fwip_post_busreset;
	fwip->fw_softc.fwip = fwip;
	TASK_INIT(&fwip->start_send, 0, fwip_start_send, fwip);

	/*
	 * Encode our hardware the way that arp likes it.
	 */
	hwaddr->sender_unique_ID_hi = htonl(fwip->fd.fc->eui.hi);
	hwaddr->sender_unique_ID_lo = htonl(fwip->fd.fc->eui.lo);
	hwaddr->sender_max_rec = fwip->fd.fc->maxrec;
	hwaddr->sspd = fwip->fd.fc->speed;
	hwaddr->sender_unicast_FIFO_hi = htons((uint16_t)(INET_FIFO >> 32));
	hwaddr->sender_unicast_FIFO_lo = htonl((uint32_t)INET_FIFO);

	/* fill the rest and attach interface */	
	ifp->if_softc = &fwip->fw_softc;

#if __FreeBSD_version >= 501113 || defined(__DragonFly__) || defined(__NetBSD__)
	IF_INITNAME(ifp, dev, unit);
#else
	ifp->if_unit = unit;
	ifp->if_name = "fwip";
#endif
#if defined(__NetBSD__)
	IFQ_SET_READY(&ifp->if_snd);
#endif
	SET_IFFUNC(ifp, fwip_start, fwip_ioctl, fwip_init, fwip_stop);
	ifp->if_flags = (IFF_BROADCAST|IFF_SIMPLEX|IFF_MULTICAST|
	    IFF_NEEDSGIANT);
	ifp->if_snd.ifq_maxlen = TX_MAX_QUEUE;
#ifdef DEVICE_POLLING
	ifp->if_capabilities |= IFCAP_POLLING;
#endif

	s = splfwnet();
	FIREWIRE_IFATTACH(ifp, hwaddr);
	splx(s);

	FWIPDEBUG(ifp, "interface created\n");
	FW_ATTACH_RETURN(0);
}

IF_STOP(fwip)
{
	IF_STOP_START(fwip, ifp, fwip);
	struct firewire_comm *fc;
	struct fw_xferq *xferq;
	struct fw_xfer *xfer, *next;
	int i;

	fc = fwip->fd.fc;

	if (fwip->dma_ch >= 0) {
		xferq = fc->ir[fwip->dma_ch];

		if (xferq->flag & FWXFERQ_RUNNING)
			fc->irx_disable(fc, fwip->dma_ch);
		xferq->flag &= 
			~(FWXFERQ_MODEMASK | FWXFERQ_OPEN | FWXFERQ_STREAM |
			FWXFERQ_EXTBUF | FWXFERQ_HANDLER | FWXFERQ_CHTAGMASK);
		xferq->hand =  NULL;

		for (i = 0; i < xferq->bnchunk; i ++)
			m_freem(xferq->bulkxfer[i].mbuf);
		free(xferq->bulkxfer, M_FWIP);

		fw_bindremove(fc, &fwip->fwb);
		for (xfer = STAILQ_FIRST(&fwip->fwb.xferlist); xfer != NULL;
					xfer = next) {
			next = STAILQ_NEXT(xfer, link);
			fw_xfer_free(xfer);
		}

		for (xfer = STAILQ_FIRST(&fwip->xferlist); xfer != NULL;
					xfer = next) {
			next = STAILQ_NEXT(xfer, link);
			fw_xfer_free(xfer);
		}
		STAILQ_INIT(&fwip->xferlist);

		xferq->bulkxfer =  NULL;
		fwip->dma_ch = -1;
	}

#if defined(__FreeBSD__)
	ifp->if_drv_flags &= ~(IFF_DRV_RUNNING | IFF_DRV_OACTIVE);
#elif defined(__NetBSD__)
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
#endif
}

FW_DETACH(fwip)
{
	IF_DETACH_START(fwip, fwip);
	struct ifnet *ifp;
	int s;

	ifp = fwip->fw_softc.fwip_ifp;

#ifdef DEVICE_POLLING
	if (ifp->if_capenable & IFCAP_POLLING)
		ether_poll_deregister(ifp);
#endif

	s = splfwnet();

	FWIP_STOP(fwip);
	FIREWIRE_IFDETACH(ifp);

	splx(s);
	return 0;
}

#if defined(__NetBSD__)
int
fwipactivate(struct device *self, enum devact act)
{
	struct fwip_softc *fwip = (struct fwip_softc *)self;
	int s, error = 0;

	s = splfwnet();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		if_deactivate(fwip->fw_softc.fwip_ifp);
			break;
	}
	splx(s);

	return (error);
}

#endif
IF_INIT(fwip)
{
	IF_INIT_START(fwip, fwip, ifp);
	struct firewire_comm *fc;
	struct fw_xferq *xferq;
	struct fw_xfer *xfer;
	struct mbuf *m;
	int i;

	FWIPDEBUG(ifp, "initializing\n");

	fc = fwip->fd.fc;
#define START 0
	if (fwip->dma_ch < 0) {
		for (i = START; i < fc->nisodma; i ++) {
			xferq = fc->ir[i];
			if ((xferq->flag & FWXFERQ_OPEN) == 0)
				goto found;
		}
		printf("no free dma channel\n");
		IF_INIT_RETURN(ENXIO);
found:
		fwip->dma_ch = i;
		/* allocate DMA channel and init packet mode */
		xferq->flag |= FWXFERQ_OPEN | FWXFERQ_EXTBUF |
				FWXFERQ_HANDLER | FWXFERQ_STREAM;
		xferq->flag &= ~0xff;
		xferq->flag |= broadcast_channel & 0xff;
		/* register fwip_input handler */
		xferq->sc = (void *) fwip;
		xferq->hand = fwip_stream_input;
		xferq->bnchunk = rx_queue_len;
		xferq->bnpacket = 1;
		xferq->psize = MCLBYTES;
		xferq->queued = 0;
		xferq->buf = NULL;
		xferq->bulkxfer = (struct fw_bulkxfer *) malloc(
			sizeof(struct fw_bulkxfer) * xferq->bnchunk,
							M_FWIP, M_WAITOK);
		if (xferq->bulkxfer == NULL) {
			printf("if_fwip: malloc failed\n");
			IF_INIT_RETURN(ENOMEM);
		}
		STAILQ_INIT(&xferq->stvalid);
		STAILQ_INIT(&xferq->stfree);
		STAILQ_INIT(&xferq->stdma);
		xferq->stproc = NULL;
		for (i = 0; i < xferq->bnchunk; i ++) {
			m =
#if defined(__DragonFly__) || __FreeBSD_version < 500000
				m_getcl(M_WAIT, MT_DATA, M_PKTHDR);
#else
				m_getcl(M_TRYWAIT, MT_DATA, M_PKTHDR);
#endif
			xferq->bulkxfer[i].mbuf = m;
			if (m != NULL) {
				m->m_len = m->m_pkthdr.len = m->m_ext.ext_size;
				STAILQ_INSERT_TAIL(&xferq->stfree,
						&xferq->bulkxfer[i], link);
			} else
				printf("fwip_as_input: m_getcl failed\n");
		}

		fwip->fwb.start = INET_FIFO;
		fwip->fwb.end = INET_FIFO + 16384; /* S3200 packet size */

		/* pre-allocate xfer */
		STAILQ_INIT(&fwip->fwb.xferlist);
		for (i = 0; i < rx_queue_len; i ++) {
			xfer = fw_xfer_alloc(M_FWIP);
			if (xfer == NULL)
				break;
			m = m_getcl(M_TRYWAIT, MT_DATA, M_PKTHDR);
			xfer->recv.payload = mtod(m, uint32_t *);
			xfer->recv.pay_len = MCLBYTES;
			xfer->hand = fwip_unicast_input;
			xfer->fc = fc;
			xfer->sc = (void *)fwip;
			xfer->mbuf = m;
			STAILQ_INSERT_TAIL(&fwip->fwb.xferlist, xfer, link);
		}
		fw_bindadd(fc, &fwip->fwb);

		STAILQ_INIT(&fwip->xferlist);
		for (i = 0; i < TX_MAX_QUEUE; i++) {
			xfer = fw_xfer_alloc(M_FWIP);
			if (xfer == NULL)
				break;
			xfer->send.spd = tx_speed;
			xfer->fc = fwip->fd.fc;
			xfer->sc = (void *)fwip;
			xfer->hand = fwip_output_callback;
			STAILQ_INSERT_TAIL(&fwip->xferlist, xfer, link);
		}
	} else
		xferq = fc->ir[fwip->dma_ch];

	fwip->last_dest.hi = 0;
	fwip->last_dest.lo = 0;

	/* start dma */
	if ((xferq->flag & FWXFERQ_RUNNING) == 0)
		fc->irx_enable(fc, fwip->dma_ch);

#if defined(__FreeBSD__)
	ifp->if_drv_flags |= IFF_DRV_RUNNING;
	ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;
#elif defined(__NetBSD__)
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
#endif

#if 0
	/* attempt to start output */
	fwip_start(ifp);
#endif
	IF_INIT_RETURN(0);
}

static int
fwip_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	IF_IOCTL_START(fwip, fwip);
	int s, error;

	switch (cmd) {
	case SIOCSIFFLAGS:
		s = splfwnet();
		if (ifp->if_flags & IFF_UP) {
#if defined(__FreeBSD__)
			if (!(ifp->if_drv_flags & IFF_DRV_RUNNING))
#elif defined(__NetBSD__)
			if (!(ifp->if_flags & IFF_RUNNING))
#endif
				FWIP_INIT(fwip);
		} else {
#if defined(__FreeBSD__)
			if (ifp->if_drv_flags & IFF_DRV_RUNNING)
#elif defined(__NetBSD__)
			if (ifp->if_flags & IFF_RUNNING)
#endif
				FWIP_STOP(fwip);
		}
		splx(s);
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		break;
	case SIOCSIFCAP:
#ifdef DEVICE_POLLING
	    {
		struct ifreq *ifr = (struct ifreq *) data;
		struct firewire_comm *fc = fc = fwip->fd.fc;

		if (ifr->ifr_reqcap & IFCAP_POLLING &&
		    !(ifp->if_capenable & IFCAP_POLLING)) {
			error = ether_poll_register(fwip_poll, ifp);
			if (error)
				return(error);
			/* Disable interrupts */
			fc->set_intr(fc, 0);
			ifp->if_capenable |= IFCAP_POLLING;
			return (error);

		}
		if (!(ifr->ifr_reqcap & IFCAP_POLLING) &&
		    ifp->if_capenable & IFCAP_POLLING) {
			error = ether_poll_deregister(ifp);
			/* Enable interrupts. */
			fc->set_intr(fc, 1);
			ifp->if_capenable &= ~IFCAP_POLLING;
			return (error);
		}
	    }
#endif /* DEVICE_POLLING */
		break;

#if (defined(__FreeBSD__) && __FreeBSD_version >= 500000) || defined(__NetBSD__)
	default:
#else
	case SIOCSIFADDR:
	case SIOCGIFADDR:
	case SIOCSIFMTU:
#endif
		s = splfwnet();
		error = FIREWIRE_IOCTL(ifp, cmd, data);
		splx(s);
		return (error);
#if defined(__DragonFly__) || \
    (defined(__FreeBSD__) && __FreeBSD_version < 500000)
	default:
		return (EINVAL);
#endif
	}

	return (0);
}

static void
fwip_post_busreset(void *arg)
{
	struct fwip_softc *fwip = arg;
	struct crom_src *src;
	struct crom_chunk *root;

	src = fwip->fd.fc->crom_src;
	root = fwip->fd.fc->crom_root;

	/* RFC2734 IPv4 over IEEE1394 */
	bzero(&fwip->unit4, sizeof(struct crom_chunk));
	crom_add_chunk(src, root, &fwip->unit4, CROM_UDIR);
	crom_add_entry(&fwip->unit4, CSRKEY_SPEC, CSRVAL_IETF);
	crom_add_simple_text(src, &fwip->unit4, &fwip->spec4, "IANA");
	crom_add_entry(&fwip->unit4, CSRKEY_VER, 1);
	crom_add_simple_text(src, &fwip->unit4, &fwip->ver4, "IPv4");

	/* RFC3146 IPv6 over IEEE1394 */
	bzero(&fwip->unit6, sizeof(struct crom_chunk));
	crom_add_chunk(src, root, &fwip->unit6, CROM_UDIR);
	crom_add_entry(&fwip->unit6, CSRKEY_SPEC, CSRVAL_IETF);
	crom_add_simple_text(src, &fwip->unit6, &fwip->spec6, "IANA");
	crom_add_entry(&fwip->unit6, CSRKEY_VER, 2);
	crom_add_simple_text(src, &fwip->unit6, &fwip->ver6, "IPv6");

	fwip->last_dest.hi = 0;
	fwip->last_dest.lo = 0;
	FIREWIRE_BUSRESET(fwip->fw_softc.fwip_ifp);
}

static void
fwip_output_callback(struct fw_xfer *xfer)
{
	struct fwip_softc *fwip;
	struct ifnet *ifp;
	int s;

	GIANT_REQUIRED;

	fwip = (struct fwip_softc *)xfer->sc;
	ifp = fwip->fw_softc.fwip_ifp;
	/* XXX error check */
	FWIPDEBUG(ifp, "resp = %d\n", xfer->resp);
	if (xfer->resp != 0)
		ifp->if_oerrors ++;
		
	m_freem(xfer->mbuf);
	fw_xfer_unload(xfer);

	s = splfwnet();
	STAILQ_INSERT_TAIL(&fwip->xferlist, xfer, link);
	splx(s);

	/* for queue full */
	if (ifp->if_snd.ifq_head != NULL)
		fwip_start(ifp);
}

static void
fwip_start(struct ifnet *ifp)
{
	struct fwip_softc *fwip =
	    ((struct fwip_eth_softc *)ifp->if_softc)->fwip;
	int s;

	GIANT_REQUIRED;

	FWIPDEBUG(ifp, "starting\n");

	if (fwip->dma_ch < 0) {
		struct mbuf	*m = NULL;

		FWIPDEBUG(ifp, "not ready\n");

		s = splfwnet();
		do {
			IF_DEQUEUE(&ifp->if_snd, m);
			if (m != NULL)
				m_freem(m);
			ifp->if_oerrors ++;
		} while (m != NULL);
		splx(s);

		return;
	}

	s = splfwnet();
#if defined(__FreeBSD__)
	ifp->if_drv_flags |= IFF_DRV_OACTIVE;
#elif defined(__NetBSD__)
	ifp->if_flags |= IFF_OACTIVE;
#endif

	if (ifp->if_snd.ifq_len != 0)
		fwip_async_output(fwip, ifp);

#if defined(__FreeBSD__)
	ifp->if_drv_flags &= ~IFF_DRV_OACTIVE;
#elif defined(__NetBSD__)
	ifp->if_flags &= ~IFF_OACTIVE;
#endif
	splx(s);
}

/* Async. stream output */
static void
fwip_async_output(struct fwip_softc *fwip, struct ifnet *ifp)
{
	struct firewire_comm *fc = fwip->fd.fc;
	struct mbuf *m;
	struct m_tag *mtag;
	struct fw_hwaddr *destfw;
	struct fw_xfer *xfer;
	struct fw_xferq *xferq;
	struct fw_pkt *fp;
	uint16_t nodeid;
	int error;
	int i = 0;

	GIANT_REQUIRED;

	xfer = NULL;
	xferq = fwip->fd.fc->atq;
	while (xferq->queued < xferq->maxq - 1) {
		xfer = STAILQ_FIRST(&fwip->xferlist);
		if (xfer == NULL) {
			printf("if_fwip: lack of xfer\n");
			return;
		}
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		/*
		 * Dig out the link-level address which
		 * firewire_output got via arp or neighbour
		 * discovery. If we don't have a link-level address,
		 * just stick the thing on the broadcast channel.
		 */
		mtag = m_tag_locate(m, MTAG_FIREWIRE, MTAG_FIREWIRE_HWADDR, 0);
		if (mtag == NULL)
			destfw = 0;
		else
			destfw = (struct fw_hwaddr *) (mtag + 1);

		STAILQ_REMOVE_HEAD(&fwip->xferlist, link);

		/*
		 * We don't do any bpf stuff here - the generic code
		 * in firewire_output gives the packet to bpf before
		 * it adds the link-level encapsulation.
		 */

		/*
		 * Put the mbuf in the xfer early in case we hit an
		 * error case below - fwip_output_callback will free
		 * the mbuf.
		 */
		xfer->mbuf = m;

		/*
		 * We use the arp result (if any) to add a suitable firewire
		 * packet header before handing off to the bus.
		 */
		fp = &xfer->send.hdr;
		nodeid = FWLOCALBUS | fc->nodeid;
		if ((m->m_flags & M_BCAST) || !destfw) {
			/*
			 * Broadcast packets are sent as GASP packets with
			 * specifier ID 0x00005e, version 1 on the broadcast
			 * channel. To be conservative, we send at the
			 * slowest possible speed.
			 */
			uint32_t *p;

			M_PREPEND(m, 2*sizeof(uint32_t), M_DONTWAIT);
			p = mtod(m, uint32_t *);
			fp->mode.stream.len = m->m_pkthdr.len;
			fp->mode.stream.chtag = broadcast_channel;
			fp->mode.stream.tcode = FWTCODE_STREAM;
			fp->mode.stream.sy = 0;
			xfer->send.spd = 0;
			p[0] = htonl(nodeid << 16);
			p[1] = htonl((0x5e << 24) | 1);
		} else {
			/*
			 * Unicast packets are sent as block writes to the
			 * target's unicast fifo address. If we can't
			 * find the node address, we just give up. We
			 * could broadcast it but that might overflow
			 * the packet size limitations due to the
			 * extra GASP header. Note: the hardware
			 * address is stored in network byte order to
			 * make life easier for ARP.
			 */
			struct fw_device *fd;
			struct fw_eui64 eui;

			eui.hi = ntohl(destfw->sender_unique_ID_hi);
			eui.lo = ntohl(destfw->sender_unique_ID_lo);
			if (fwip->last_dest.hi != eui.hi ||
			    fwip->last_dest.lo != eui.lo) {
				fd = fw_noderesolve_eui64(fc, &eui);
				if (!fd) {
					/* error */
					ifp->if_oerrors ++;
					/* XXX set error code */
					fwip_output_callback(xfer);
					continue;

				}
				fwip->last_hdr.mode.wreqb.dst = FWLOCALBUS | fd->dst;
				fwip->last_hdr.mode.wreqb.tlrt = 0;
				fwip->last_hdr.mode.wreqb.tcode = FWTCODE_WREQB;
				fwip->last_hdr.mode.wreqb.pri = 0;
				fwip->last_hdr.mode.wreqb.src = nodeid;
				fwip->last_hdr.mode.wreqb.dest_hi =
					ntohs(destfw->sender_unicast_FIFO_hi);
				fwip->last_hdr.mode.wreqb.dest_lo =
					ntohl(destfw->sender_unicast_FIFO_lo);
				fwip->last_hdr.mode.wreqb.extcode = 0;
				fwip->last_dest = eui;
			}

			fp->mode.wreqb = fwip->last_hdr.mode.wreqb;
			fp->mode.wreqb.len = m->m_pkthdr.len;
			xfer->send.spd = min(destfw->sspd, fc->speed);
		}

		xfer->send.pay_len = m->m_pkthdr.len;

		error = fw_asyreq(fc, -1, xfer);
		if (error == EAGAIN) {
			/*
			 * We ran out of tlabels - requeue the packet
			 * for later transmission.
			 */
			xfer->mbuf = 0;
			STAILQ_INSERT_TAIL(&fwip->xferlist, xfer, link);
			IF_PREPEND(&ifp->if_snd, m);
			break;
		}
		if (error) {
			/* error */
			ifp->if_oerrors ++;
			/* XXX set error code */
			fwip_output_callback(xfer);
			continue;
		} else {
			ifp->if_opackets ++;
			i++;
		}
	}
#if 0
	if (i > 1)
		printf("%d queued\n", i);
#endif
	if (i > 0) {
#if 1
		xferq->start(fc);
#else
		taskqueue_enqueue(taskqueue_swi_giant, &fwip->start_send);
#endif
	}
}

#if defined(__FreeBSD__)
static void
fwip_start_send (void *arg, int count)
{
	struct fwip_softc *fwip = arg;

	GIANT_REQUIRED;
	fwip->fd.fc->atq->start(fwip->fd.fc);
}
#endif

/* Async. stream output */
static void
fwip_stream_input(struct fw_xferq *xferq)
{
	struct mbuf *m, *m0;
	struct m_tag *mtag;
	struct ifnet *ifp;
	struct fwip_softc *fwip;
	struct fw_bulkxfer *sxfer;
	struct fw_pkt *fp;
	uint16_t src;
	uint32_t *p;

	GIANT_REQUIRED;

	fwip = (struct fwip_softc *)xferq->sc;
	ifp = fwip->fw_softc.fwip_ifp;
	while ((sxfer = STAILQ_FIRST(&xferq->stvalid)) != NULL) {
		STAILQ_REMOVE_HEAD(&xferq->stvalid, link);
		fp = mtod(sxfer->mbuf, struct fw_pkt *);
		if (fwip->fd.fc->irx_post != NULL)
			fwip->fd.fc->irx_post(fwip->fd.fc, fp->mode.ld);
		m = sxfer->mbuf;

		/* insert new rbuf */
		sxfer->mbuf = m0 = m_getcl(M_DONTWAIT, MT_DATA, M_PKTHDR);
		if (m0 != NULL) {
			m0->m_len = m0->m_pkthdr.len = m0->m_ext.ext_size;
			STAILQ_INSERT_TAIL(&xferq->stfree, sxfer, link);
		} else
			printf("fwip_as_input: m_getcl failed\n");

		/*
		 * We must have a GASP header - leave the
		 * encapsulation sanity checks to the generic
		 * code. Remeber that we also have the firewire async
		 * stream header even though that isn't accounted for
		 * in mode.stream.len.
		 */
		if (sxfer->resp != 0 || fp->mode.stream.len <
		    2*sizeof(uint32_t)) {
			m_freem(m);
			ifp->if_ierrors ++;
			continue;
		}
		m->m_len = m->m_pkthdr.len = fp->mode.stream.len
			+ sizeof(fp->mode.stream);

		/*
		 * If we received the packet on the broadcast channel,
		 * mark it as broadcast, otherwise we assume it must
		 * be multicast.
		 */
		if (fp->mode.stream.chtag == broadcast_channel)
			m->m_flags |= M_BCAST;
		else
			m->m_flags |= M_MCAST;

		/*
		 * Make sure we recognise the GASP specifier and
		 * version.
		 */
		p = mtod(m, uint32_t *);
		if ((((ntohl(p[1]) & 0xffff) << 8) | ntohl(p[2]) >> 24) != 0x00005e
		    || (ntohl(p[2]) & 0xffffff) != 1) {
			FWIPDEBUG(ifp, "Unrecognised GASP header %#08x %#08x\n",
			    ntohl(p[1]), ntohl(p[2]));
			m_freem(m);
			ifp->if_ierrors ++;
			continue;
		}

		/*
		 * Record the sender ID for possible BPF usage.
		 */
		src = ntohl(p[1]) >> 16;
		if (bpf_peers_present(ifp->if_bpf)) {
			mtag = m_tag_alloc(MTAG_FIREWIRE,
			    MTAG_FIREWIRE_SENDER_EUID,
			    2*sizeof(uint32_t), M_NOWAIT);
			if (mtag) {
				/* bpf wants it in network byte order */
				struct fw_device *fd;
				uint32_t *p2 = (uint32_t *) (mtag + 1);
				fd = fw_noderesolve_nodeid(fwip->fd.fc,
				    src & 0x3f);
				if (fd) {
					p2[0] = htonl(fd->eui.hi);
					p2[1] = htonl(fd->eui.lo);
				} else {
					p2[0] = 0;
					p2[1] = 0;
				}
				m_tag_prepend(m, mtag);
			}
		}

		/*
		 * Trim off the GASP header
		 */
		m_adj(m, 3*sizeof(uint32_t));
		m->m_pkthdr.rcvif = ifp;
		FIREWIRE_INPUT(ifp, m, src);
		ifp->if_ipackets ++;
	}
	if (STAILQ_FIRST(&xferq->stfree) != NULL)
		fwip->fd.fc->irx_enable(fwip->fd.fc, fwip->dma_ch);
}

static inline void
fwip_unicast_input_recycle(struct fwip_softc *fwip, struct fw_xfer *xfer)
{
	struct mbuf *m;

	GIANT_REQUIRED;

	/*
	 * We have finished with a unicast xfer. Allocate a new
	 * cluster and stick it on the back of the input queue.
	 */
	m = m_getcl(M_DONTWAIT, MT_DATA, M_PKTHDR);
	if (m == NULL)
		printf("fwip_unicast_input_recycle: m_getcl failed\n");
	xfer->mbuf = m;
	xfer->recv.payload = mtod(m, uint32_t *);
	xfer->recv.pay_len = MCLBYTES;
	xfer->mbuf = m;
	STAILQ_INSERT_TAIL(&fwip->fwb.xferlist, xfer, link);
}

static void
fwip_unicast_input(struct fw_xfer *xfer)
{
	uint64_t address;
	struct mbuf *m;
	struct m_tag *mtag;
	struct ifnet *ifp;
	struct fwip_softc *fwip;
	struct fw_pkt *fp;
	//struct fw_pkt *sfp;
	int rtcode;

	GIANT_REQUIRED;

	fwip = (struct fwip_softc *)xfer->sc;
	ifp = fwip->fw_softc.fwip_ifp;
	m = xfer->mbuf;
	xfer->mbuf = 0;
	fp = &xfer->recv.hdr;

	/*
	 * Check the fifo address - we only accept addresses of
	 * exactly INET_FIFO.
	 */
	address = ((uint64_t)fp->mode.wreqb.dest_hi << 32)
		| fp->mode.wreqb.dest_lo;
	if (fp->mode.wreqb.tcode != FWTCODE_WREQB) {
		rtcode = FWRCODE_ER_TYPE;
	} else if (address != INET_FIFO) {
		rtcode = FWRCODE_ER_ADDR;
	} else {
		rtcode = FWRCODE_COMPLETE;
	}

	/*
	 * Pick up a new mbuf and stick it on the back of the receive
	 * queue.
	 */
	fwip_unicast_input_recycle(fwip, xfer);

	/*
	 * If we've already rejected the packet, give up now.
	 */
	if (rtcode != FWRCODE_COMPLETE) {
		m_freem(m);
		ifp->if_ierrors ++;
		return;
	}

	if (bpf_peers_present(ifp->if_bpf)) {
		/*
		 * Record the sender ID for possible BPF usage.
		 */
		mtag = m_tag_alloc(MTAG_FIREWIRE, MTAG_FIREWIRE_SENDER_EUID,
		    2*sizeof(uint32_t), M_NOWAIT);
		if (mtag) {
			/* bpf wants it in network byte order */
			struct fw_device *fd;
			uint32_t *p = (uint32_t *) (mtag + 1);
			fd = fw_noderesolve_nodeid(fwip->fd.fc,
			    fp->mode.wreqb.src & 0x3f);
			if (fd) {
				p[0] = htonl(fd->eui.hi);
				p[1] = htonl(fd->eui.lo);
			} else {
				p[0] = 0;
				p[1] = 0;
			}
			m_tag_prepend(m, mtag);
		}
	}

	/*
	 * Hand off to the generic encapsulation code. We don't use
	 * ifp->if_input so that we can pass the source nodeid as an
	 * argument to facilitate link-level fragment reassembly.
	 */
	m->m_len = m->m_pkthdr.len = fp->mode.wreqb.len;
	m->m_pkthdr.rcvif = ifp;
	FIREWIRE_INPUT(ifp, m, fp->mode.wreqb.src);
	ifp->if_ipackets ++;
}

#if defined(__FreeBSD__)
static devclass_t fwip_devclass;

static device_method_t fwip_methods[] = {
	/* device interface */
	DEVMETHOD(device_identify,	fwip_identify),
	DEVMETHOD(device_probe,		fwip_probe),
	DEVMETHOD(device_attach,	fwip_attach),
	DEVMETHOD(device_detach,	fwip_detach),
	{ 0, 0 }
};

static driver_t fwip_driver = {
        "fwip",
	fwip_methods,
	sizeof(struct fwip_softc),
};


#ifdef __DragonFly__
DECLARE_DUMMY_MODULE(fwip);
#endif
DRIVER_MODULE(fwip, firewire, fwip_driver, fwip_devclass, 0, 0);
MODULE_VERSION(fwip, 1);
MODULE_DEPEND(fwip, firewire, 1, 1, 1);
#elif defined(__NetBSD__)
CFATTACH_DECL(fwip, sizeof (struct fwip_softc),
    fwipmatch, fwipattach, fwipdetach, NULL);
#endif
