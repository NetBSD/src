/*	$NetBSD: viocon.c,v 1.5.4.3 2024/10/02 18:20:48 martin Exp $	*/
/*	$OpenBSD: viocon.c,v 1.8 2021/11/05 11:38:29 mpi Exp $	*/

/*
 * Copyright (c) 2013-2015 Stefan Fritsch <sf@sfritsch.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: viocon.c,v 1.5.4.3 2024/10/02 18:20:48 martin Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <dev/pci/virtioreg.h>
#include <dev/pci/virtiovar.h>

#include "ioconf.h"

/* OpenBSD compat shims */
#define	ttymalloc(speed)	tty_alloc()
#define	splassert(ipl)		__nothing
#define	virtio_notify(vsc, vq)	virtio_enqueue_commit(vsc, vq, -1, true)
#define	ttwakeupwr(tp)		__nothing

/* features */
#define	VIRTIO_CONSOLE_F_SIZE		(1ULL<<0)
#define	VIRTIO_CONSOLE_F_MULTIPORT	(1ULL<<1)
#define	VIRTIO_CONSOLE_F_EMERG_WRITE 	(1ULL<<2)

/* config space */
#define VIRTIO_CONSOLE_COLS		0	/* 16 bits */
#define VIRTIO_CONSOLE_ROWS		2	/* 16 bits */
#define VIRTIO_CONSOLE_MAX_NR_PORTS	4	/* 32 bits */
#define VIRTIO_CONSOLE_EMERG_WR		8	/* 32 bits */

#define VIOCON_DEBUG	0

#if VIOCON_DEBUG
#define DPRINTF(x...) printf(x)
#else
#define DPRINTF(x...)
#endif

#define	VIRTIO_CONSOLE_FLAG_BITS					      \
	VIRTIO_COMMON_FLAG_BITS						      \
	"b\x00" "SIZE\0"						      \
	"b\x01" "MULTIPORT\0"						      \
	"b\x02" "EMERG_WRITE\0"

struct virtio_console_control {
	uint32_t id;	/* Port number */

#define	VIRTIO_CONSOLE_DEVICE_READY	0
#define	VIRTIO_CONSOLE_PORT_ADD		1
#define	VIRTIO_CONSOLE_PORT_REMOVE	2
#define	VIRTIO_CONSOLE_PORT_READY	3
#define	VIRTIO_CONSOLE_CONSOLE_PORT	4
#define	VIRTIO_CONSOLE_RESIZE		5
#define	VIRTIO_CONSOLE_PORT_OPEN	6
#define	VIRTIO_CONSOLE_PORT_NAME	7
	uint16_t event;

	uint16_t value;
};

struct virtio_console_control_resize {
	/* yes, the order is different than in config space */
	uint16_t rows;
	uint16_t cols;
};

#define	BUFSIZE		128

#define	VIOCONDEV(u,p)	makedev(cdevsw_lookup_major(&viocon_cdevsw),	      \
			    ((u) << 4) | (p))
#define VIOCONUNIT(x)	(minor(x) >> 4)
#define VIOCONPORT(x)	(minor(x) & 0x0f)

struct viocon_port {
	struct viocon_softc	*vp_sc;
	struct virtqueue	*vp_rx;
	struct virtqueue	*vp_tx;
	void			*vp_si;
	struct tty		*vp_tty;
	const char 		*vp_name;
	bus_dma_segment_t	 vp_dmaseg;
	bus_dmamap_t		 vp_dmamap;
#ifdef NOTYET
	unsigned int		 vp_host_open:1;	/* XXX needs F_MULTIPORT */
	unsigned int		 vp_guest_open:1;	/* XXX needs F_MULTIPORT */
	unsigned int		 vp_is_console:1;	/* XXX needs F_MULTIPORT */
#endif
	unsigned int		 vp_iflow:1;		/* rx flow control */
	uint16_t		 vp_rows;
	uint16_t		 vp_cols;
	u_char			*vp_rx_buf;
	u_char			*vp_tx_buf;
};

struct viocon_softc {
	struct device		*sc_dev;
	struct virtio_softc	*sc_virtio;
	struct virtqueue	*sc_vqs;
#define VIOCON_PORT_RX	0
#define VIOCON_PORT_TX	1
#define VIOCON_PORT_NQS	2

	struct virtqueue        *sc_c_vq_rx;
	struct virtqueue        *sc_c_vq_tx;

	unsigned int		 sc_max_ports;
	struct viocon_port	**sc_ports;
};

int	viocon_match(struct device *, struct cfdata *, void *);
void	viocon_attach(struct device *, struct device *, void *);
int	viocon_tx_intr(struct virtqueue *);
int	viocon_tx_drain(struct viocon_port *, struct virtqueue *vq);
int	viocon_rx_intr(struct virtqueue *);
void	viocon_rx_soft(void *);
void	viocon_rx_fill(struct viocon_port *);
int	viocon_port_create(struct viocon_softc *, int);
void	vioconstart(struct tty *);
int	vioconhwiflow(struct tty *, int);
int	vioconparam(struct tty *, struct termios *);
int	vioconopen(dev_t, int, int, struct lwp *);
int	vioconclose(dev_t, int, int, struct lwp *);
int	vioconread(dev_t, struct uio *, int);
int	vioconwrite(dev_t, struct uio *, int);
void	vioconstop(struct tty *, int);
int	vioconioctl(dev_t, u_long, void *, int, struct lwp *);
struct tty	*viocontty(dev_t dev);

CFATTACH_DECL_NEW(viocon, sizeof(struct viocon_softc),
    viocon_match, viocon_attach, /*detach*/NULL, /*activate*/NULL);

const struct cdevsw viocon_cdevsw = {
	.d_open = vioconopen,
	.d_close = vioconclose,
	.d_read = vioconread,
	.d_write = vioconwrite,
	.d_ioctl = vioconioctl,
	.d_stop = vioconstop,
	.d_tty = viocontty,
	.d_poll = nopoll,	/* XXX */
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY,
};

static inline struct viocon_softc *
dev2sc(dev_t dev)
{
	return device_lookup_private(&viocon_cd, VIOCONUNIT(dev));
}

static inline struct viocon_port *
dev2port(dev_t dev)
{
	return dev2sc(dev)->sc_ports[VIOCONPORT(dev)];
}

int viocon_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct virtio_attach_args *va = aux;
	if (va->sc_childdevid == VIRTIO_DEVICE_ID_CONSOLE)
		return 1;
	return 0;
}

void
viocon_attach(struct device *parent, struct device *self, void *aux)
{
	struct viocon_softc *sc = device_private(self);
	struct virtio_softc *vsc = device_private(parent);
	int maxports = 1;
	size_t nvqs;

	sc->sc_dev = self;
	if (virtio_child(vsc) != NULL) {
		aprint_error(": parent %s already has a child\n",
		    device_xname(parent));
		return;
	}
	sc->sc_virtio = vsc;
	sc->sc_max_ports = maxports;
	nvqs = VIOCON_PORT_NQS * maxports;

	sc->sc_vqs = kmem_zalloc(nvqs * sizeof(sc->sc_vqs[0]),
	    KM_SLEEP);
	sc->sc_ports = kmem_zalloc(maxports * sizeof(sc->sc_ports[0]),
	    KM_SLEEP);

	virtio_child_attach_start(vsc, self, IPL_TTY,
	    /*req_features*/VIRTIO_CONSOLE_F_SIZE, VIRTIO_CONSOLE_FLAG_BITS);

	DPRINTF("%s: softc: %p\n", __func__, sc);
	if (viocon_port_create(sc, 0) != 0) {
		printf("\n%s: viocon_port_create failed\n", __func__);
		goto err;
	}

	if (virtio_child_attach_finish(vsc, sc->sc_vqs, nvqs,
	    /*config_change*/NULL, /*req_flags*/0) != 0)
		goto err;

	viocon_rx_fill(sc->sc_ports[0]);

	return;
err:
	kmem_free(sc->sc_vqs, nvqs * sizeof(sc->sc_vqs[0]));
	kmem_free(sc->sc_ports, maxports * sizeof(sc->sc_ports[0]));
	virtio_child_attach_failed(vsc);
}

int
viocon_port_create(struct viocon_softc *sc, int portidx)
{
	struct virtio_softc *vsc = sc->sc_virtio;
	int rxidx, txidx, allocsize, nsegs;
	char name[6];
	struct viocon_port *vp;
	void *kva;
	struct tty *tp;

	vp = kmem_zalloc(sizeof(*vp), KM_SLEEP);
	if (vp == NULL)
		return ENOMEM;
	sc->sc_ports[portidx] = vp;
	vp->vp_sc = sc;
	DPRINTF("%s: vp: %p\n", __func__, vp);

	rxidx = (portidx * VIOCON_PORT_NQS) + VIOCON_PORT_RX;
	txidx = (portidx * VIOCON_PORT_NQS) + VIOCON_PORT_TX;

	snprintf(name, sizeof(name), "p%drx", portidx);
	virtio_init_vq_vqdone(vsc, &sc->sc_vqs[rxidx], rxidx,
	    viocon_rx_intr);
	if (virtio_alloc_vq(vsc, &sc->sc_vqs[rxidx], BUFSIZE, 1,
	    name) != 0) {
		printf("\nCan't alloc %s virtqueue\n", name);
		goto err;
	}
	vp->vp_rx = &sc->sc_vqs[rxidx];
	vp->vp_si = softint_establish(SOFTINT_SERIAL, viocon_rx_soft, vp);
	DPRINTF("%s: rx: %p\n", __func__, vp->vp_rx);

	snprintf(name, sizeof(name), "p%dtx", portidx);
	virtio_init_vq_vqdone(vsc, &sc->sc_vqs[txidx], txidx,
	    viocon_tx_intr);
	if (virtio_alloc_vq(vsc, &sc->sc_vqs[txidx], BUFSIZE, 1,
	    name) != 0) {
		printf("\nCan't alloc %s virtqueue\n", name);
		goto err;
	}
	vp->vp_tx = &sc->sc_vqs[txidx];
	DPRINTF("%s: tx: %p\n", __func__, vp->vp_tx);

	allocsize = (vp->vp_rx->vq_num + vp->vp_tx->vq_num) * BUFSIZE;

	if (bus_dmamap_create(virtio_dmat(vsc), allocsize, 1, allocsize, 0,
	    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &vp->vp_dmamap) != 0)
		goto err;
	if (bus_dmamem_alloc(virtio_dmat(vsc), allocsize, 8, 0, &vp->vp_dmaseg,
	    1, &nsegs, BUS_DMA_NOWAIT) != 0)
		goto err;
	if (bus_dmamem_map(virtio_dmat(vsc), &vp->vp_dmaseg, nsegs,
	    allocsize, &kva, BUS_DMA_NOWAIT) != 0)
		goto err;
	memset(kva, 0, allocsize);
	if (bus_dmamap_load(virtio_dmat(vsc), vp->vp_dmamap, kva,
	    allocsize, NULL, BUS_DMA_NOWAIT) != 0)
		goto err;
	vp->vp_rx_buf = (unsigned char *)kva;
	/*
	 * XXX use only a small circular tx buffer instead of many BUFSIZE buffers?
	 */
	vp->vp_tx_buf = vp->vp_rx_buf + vp->vp_rx->vq_num * BUFSIZE;

	if (virtio_features(vsc) & VIRTIO_CONSOLE_F_SIZE) {
		vp->vp_cols = virtio_read_device_config_2(vsc,
		    VIRTIO_CONSOLE_COLS);
		vp->vp_rows = virtio_read_device_config_2(vsc,
		    VIRTIO_CONSOLE_ROWS);
	}

	tp = ttymalloc(1000000);
	tp->t_oproc = vioconstart;
	tp->t_param = vioconparam;
	tp->t_hwiflow = vioconhwiflow;
	tp->t_dev = VIOCONDEV(device_unit(sc->sc_dev), portidx);
	vp->vp_tty = tp;
	DPRINTF("%s: tty: %p\n", __func__, tp);

	virtio_start_vq_intr(vsc, vp->vp_rx);
	virtio_start_vq_intr(vsc, vp->vp_tx);

	return 0;
err:
	panic("%s failed", __func__);
	return -1;
}

int
viocon_tx_drain(struct viocon_port *vp, struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	int ndone = 0, len, slot;

	splassert(IPL_TTY);
	while (virtio_dequeue(vsc, vq, &slot, &len) == 0) {
		bus_dmamap_sync(virtio_dmat(vsc), vp->vp_dmamap,
		    vp->vp_tx_buf - vp->vp_rx_buf + slot * BUFSIZE, BUFSIZE,
		    BUS_DMASYNC_POSTWRITE);
		virtio_dequeue_commit(vsc, vq, slot);
		ndone++;
	}
	return ndone;
}

int
viocon_tx_intr(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct viocon_softc *sc = device_private(virtio_child(vsc));
	int ndone = 0;
	int portidx = (vq->vq_index - 1) / 2;
	struct viocon_port *vp = sc->sc_ports[portidx];
	struct tty *tp = vp->vp_tty;

	splassert(IPL_TTY);
	ndone = viocon_tx_drain(vp, vq);
	if (ndone && ISSET(tp->t_state, TS_BUSY)) {
		CLR(tp->t_state, TS_BUSY);
		(*tp->t_linesw->l_start)(tp);
	}

	return 1;
}

void
viocon_rx_fill(struct viocon_port *vp)
{
	struct virtqueue *vq = vp->vp_rx;
	struct virtio_softc *vsc = vp->vp_sc->sc_virtio;
	int r, slot, ndone = 0;

	while ((r = virtio_enqueue_prep(vsc, vq, &slot)) == 0) {
		if (virtio_enqueue_reserve(vsc, vq, slot, 1) != 0)
			break;
		bus_dmamap_sync(virtio_dmat(vsc), vp->vp_dmamap, slot * BUFSIZE,
		    BUFSIZE, BUS_DMASYNC_PREREAD);
		virtio_enqueue_p(vsc, vq, slot, vp->vp_dmamap, slot * BUFSIZE,
		    BUFSIZE, 0);
		virtio_enqueue_commit(vsc, vq, slot, 0);
		ndone++;
	}
	KASSERTMSG(r == 0 || r == EAGAIN, "r=%d", r);
	if (ndone > 0)
		virtio_notify(vsc, vq);
}

int
viocon_rx_intr(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct viocon_softc *sc = device_private(virtio_child(vsc));
	int portidx = (vq->vq_index - 1) / 2;
	struct viocon_port *vp = sc->sc_ports[portidx];

	softint_schedule(vp->vp_si);
	return 1;
}

void
viocon_rx_soft(void *arg)
{
	struct viocon_port *vp = arg;
	struct virtqueue *vq = vp->vp_rx;
	struct virtio_softc *vsc = vq->vq_owner;
	struct tty *tp = vp->vp_tty;
	int slot, len, i;
	u_char *p;

	while (!vp->vp_iflow && virtio_dequeue(vsc, vq, &slot, &len) == 0) {
		bus_dmamap_sync(virtio_dmat(vsc), vp->vp_dmamap,
		    slot * BUFSIZE, BUFSIZE, BUS_DMASYNC_POSTREAD);
		p = vp->vp_rx_buf + slot * BUFSIZE;
		for (i = 0; i < len; i++)
			(*tp->t_linesw->l_rint)(*p++, tp);
		virtio_dequeue_commit(vsc, vq, slot);
	}

	viocon_rx_fill(vp);

	return;
}

void
vioconstart(struct tty *tp)
{
	struct viocon_softc *sc = dev2sc(tp->t_dev);
	struct virtio_softc *vsc;
	struct viocon_port *vp = dev2port(tp->t_dev);
	struct virtqueue *vq;
	u_char *buf;
	int s, cnt, slot, ret, ndone;

	vsc = sc->sc_virtio;
	vq = vp->vp_tx;

	s = spltty();

	ndone = viocon_tx_drain(vp, vq);
	if (ISSET(tp->t_state, TS_BUSY)) {
		if (ndone > 0)
			CLR(tp->t_state, TS_BUSY);
		else
			goto out;
	}
	if (ISSET(tp->t_state, TS_TIMEOUT | TS_TTSTOP))
		goto out;

	if (tp->t_outq.c_cc == 0)
		goto out;
	ndone = 0;

	while (tp->t_outq.c_cc > 0) {
		ret = virtio_enqueue_prep(vsc, vq, &slot);
		if (ret == EAGAIN) {
			SET(tp->t_state, TS_BUSY);
			break;
		}
		KASSERTMSG(ret == 0, "ret=%d", ret);
		ret = virtio_enqueue_reserve(vsc, vq, slot, 1);
		KASSERTMSG(ret == 0, "ret=%d", ret);
		buf = vp->vp_tx_buf + slot * BUFSIZE;
		cnt = q_to_b(&tp->t_outq, buf, BUFSIZE);
		bus_dmamap_sync(virtio_dmat(vsc), vp->vp_dmamap,
		    vp->vp_tx_buf - vp->vp_rx_buf + slot * BUFSIZE, cnt,
		    BUS_DMASYNC_PREWRITE);
		virtio_enqueue_p(vsc, vq, slot, vp->vp_dmamap,
		    vp->vp_tx_buf - vp->vp_rx_buf + slot * BUFSIZE, cnt, 1);
		virtio_enqueue_commit(vsc, vq, slot, 0);
		ndone++;
	}
	if (ndone > 0)
		virtio_notify(vsc, vq);
	ttwakeupwr(tp);
out:
	splx(s);
}

int
vioconhwiflow(struct tty *tp, int stop)
{
	struct viocon_port *vp = dev2port(tp->t_dev);
	int s;

	s = spltty();
	vp->vp_iflow = stop;
	if (stop) {
		virtio_stop_vq_intr(vp->vp_sc->sc_virtio, vp->vp_rx);
	} else {
		virtio_start_vq_intr(vp->vp_sc->sc_virtio, vp->vp_rx);
		softint_schedule(vp->vp_si);
	}
	splx(s);
	return 1;
}

int
vioconparam(struct tty *tp, struct termios *t)
{
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	vioconstart(tp);
	return 0;
}

int
vioconopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit = VIOCONUNIT(dev);
	int port = VIOCONPORT(dev);
	struct viocon_softc *sc;
	struct viocon_port *vp;
	struct tty *tp;
	int s, error;

	sc = device_lookup_private(&viocon_cd, unit);
	if (sc == NULL)
		return (ENXIO);
	if (!device_is_active(sc->sc_dev))
		return (ENXIO);

	s = spltty();
	if (port >= sc->sc_max_ports) {
		splx(s);
		return (ENXIO);
	}
	vp = sc->sc_ports[port];
	tp = vp->vp_tty;
#ifdef NOTYET
	vp->vp_guest_open = 1;
#endif
	splx(s);

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	s = spltty();
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		ttychars(tp);
		tp->t_ispeed = 1000000;
		tp->t_ospeed = 1000000;
		tp->t_cflag = TTYDEF_CFLAG|CLOCAL|CRTSCTS;
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		if (vp->vp_cols != 0) {
			tp->t_winsize.ws_col = vp->vp_cols;
			tp->t_winsize.ws_row = vp->vp_rows;
		}

		vioconparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	splx(s);

	error = (*tp->t_linesw->l_open)(dev, tp);
	return error;
}

int
vioconclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct viocon_port *vp = dev2port(dev);
	struct tty *tp = vp->vp_tty;
	int s;

	if (!ISSET(tp->t_state, TS_ISOPEN))
		return 0;

	(*tp->t_linesw->l_close)(tp, flag);
	s = spltty();
#ifdef NOTYET
	vp->vp_guest_open = 0;
#endif
	CLR(tp->t_state, TS_BUSY | TS_FLUSH);
	ttyclose(tp);
	splx(s);

	return 0;
}

int
vioconread(dev_t dev, struct uio *uio, int flag)
{
	struct viocon_port *vp = dev2port(dev);
	struct tty *tp = vp->vp_tty;

	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

int
vioconwrite(dev_t dev, struct uio *uio, int flag)
{
	struct viocon_port *vp = dev2port(dev);
	struct tty *tp = vp->vp_tty;

	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

struct tty *
viocontty(dev_t dev)
{
	struct viocon_port *vp = dev2port(dev);

	return vp->vp_tty;
}

void
vioconstop(struct tty *tp, int flag)
{
	int s;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY))
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	splx(s);
}

int
vioconioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct viocon_port *vp = dev2port(dev);
	struct tty *tp;
	int error1, error2;

	tp = vp->vp_tty;

	error1 = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error1 >= 0)
		return error1;
	error2 = ttioctl(tp, cmd, data, flag, l);
	if (error2 >= 0)
		return error2;
	return ENOTTY;
}
