/*	$NetBSD: gftty.c,v 1.3 2024/01/06 17:52:43 thorpej Exp $	*/

/*-     
 * Copyright (c) 2023, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *              
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */     

/*
 * Support for the Goldfish virtual TTY.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gftty.c,v 1.3 2024/01/06 17:52:43 thorpej Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/tty.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <dev/goldfish/gfttyvar.h>

#include "ioconf.h"

/*
 * Goldfish TTY registers.
 */
#define	GFTTY_PUT_CHAR		0x00	/* 8 bit output value */
#define	GFTTY_BYTES_READY	0x04	/* number of input bytes available */
#define	GFTTY_CMD		0x08	/* command */
#define	GFTTY_DATA_PTR		0x10	/* DMA pointer */
#define	GFTTY_DATA_LEN		0x14	/* DMA length */
#define	GFTTY_DATA_PTR_HIGH	0x18	/* DMA pointer (64-bit) */
#define	GFTTY_VERSION		0x20	/* TTY version */

#define	CMD_INT_DISABLE		0x00
#define	CMD_INT_ENABLE		0x01
#define	CMD_WRITE_BUFFER	0x02
#define	CMD_READ_BUFFER		0x03

#define	REG_READ0(c, r)		\
	bus_space_read_4((c)->c_bst, (c)->c_bsh, (r))
#define	REG_WRITE0(c, r, v)	\
	bus_space_write_4((c)->c_bst, (c)->c_bsh, (r), (v))

#define	REG_READ(sc, r)		REG_READ0((sc)->sc_config, (r))
#define	REG_WRITE(sc, r, v)	REG_WRITE0((sc)->sc_config, (r), (v))

static int	gftty_cngetc(dev_t);
static void	gftty_cnputc(dev_t, int);
static void	gftty_cnpollc(dev_t, int);

static struct gftty_config gftty_cnconfig;
static struct cnm_state gftty_cnmagic_state;
static struct consdev gftty_consdev = {
	.cn_getc  = gftty_cngetc,
	.cn_putc  = gftty_cnputc,
	.cn_pollc = gftty_cnpollc,
	.cn_dev   = NODEV,
	.cn_pri   = CN_NORMAL,
};

static dev_type_open(gftty_open);
static dev_type_close(gftty_close);
static dev_type_read(gftty_read);
static dev_type_write(gftty_write);
static dev_type_ioctl(gftty_ioctl);
static dev_type_stop(gftty_stop);
static dev_type_tty(gftty_tty);
static dev_type_poll(gftty_poll);

const struct cdevsw gftty_cdevsw = {
	.d_open     = gftty_open,
	.d_close    = gftty_close,
	.d_read     = gftty_read,
	.d_write    = gftty_write,
	.d_ioctl    = gftty_ioctl,
	.d_stop     = gftty_stop,
	.d_tty      = gftty_tty,
	.d_poll     = gftty_poll,
	.d_mmap     = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard  = nodiscard,
	.d_flag     = D_TTY,
};

static void	gftty_start(struct tty *);
static int	gftty_param_locked(struct tty *, struct termios *);
static int	gftty_param(struct tty *, struct termios *);

static void	gftty_softrx(void *);

#define	GFTTY_UNIT(x)		minor(x)
#define	GFTTY_DMASIZE		(64 * 1024)	/* XXX TTY_MAXQSIZE */
#define	GFTTY_MAXSEGS		((GFTTY_DMASIZE / PAGE_SIZE) + 1)
#define	GFTTY_RXBUFSIZE		128
#define	GFTTY_RXBUFALLOC	(128 << 1)

static void
gftty_reset_rxptrs(struct gftty_softc *sc)
{
	sc->sc_rxpos = 0;
	sc->sc_rxcur = 0;
	sc->sc_rxbuf = sc->sc_rxbufs[sc->sc_rxcur];
	sc->sc_rxaddr = sc->sc_rxaddrs[sc->sc_rxcur];
}

/*
 * gftty_attach --
 *	Attach a Goldfish virual TTY.
 */
void
gftty_attach(struct gftty_softc *sc)
{
	device_t self = sc->sc_dev;
	int error;
	bool is_console;

	aprint_naive("\n");
	aprint_normal(": Google Goldfish TTY\n");

	/* If we got here without a config, we're the console. */
	if ((is_console = (sc->sc_config == NULL))) {
		KASSERT(gftty_is_console(sc));
		sc->sc_config = &gftty_cnconfig;
		aprint_normal_dev(sc->sc_dev, "console\n");
	}

	if (sc->sc_config->c_version == 0) {
		aprint_normal_dev(self,
		    "WARNING: version 0 device -- uncharted territory!\n");
	}

	/* Register our Rx soft interrupt. */
	sc->sc_rx_si = softint_establish(SOFTINT_SERIAL, gftty_softrx, sc);
	if (sc->sc_rx_si == NULL) {
		aprint_error_dev(self,
		    "Unable to register software interrupt.\n");
		return;
	}

	error = bus_dmamap_create(sc->sc_dmat, GFTTY_DMASIZE,
	    GFTTY_MAXSEGS, GFTTY_DMASIZE, 0, BUS_DMA_WAITOK,
	    &sc->sc_tx_dma);
	if (error != 0) {
		aprint_error_dev(self,
		    "unable to create Tx DMA map, error %d.\n", error);
		return;
	}
	error = bus_dmamap_create(sc->sc_dmat, GFTTY_RXBUFALLOC,
	    1, GFTTY_RXBUFALLOC, 0, BUS_DMA_WAITOK,
	    &sc->sc_rx_dma);
	if (error != 0) {
		aprint_error_dev(self,
		    "unable to create Rx DMA map, error %d.\n", error);
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_tx_dma);
		sc->sc_tx_dma = NULL;
		return;
	}

	sc->sc_rxbuf = kmem_zalloc(GFTTY_RXBUFALLOC, KM_SLEEP);
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_rx_dma,
	    sc->sc_rxbuf, GFTTY_RXBUFALLOC, NULL, BUS_DMA_WAITOK);
	if (error != 0) {
		aprint_error_dev(self,
		    "unable to load Rx DMA map, error %d.\n", error);
		kmem_free(sc->sc_rxbuf, GFTTY_RXBUFALLOC);
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_rx_dma);
		sc->sc_rx_dma = NULL;
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_tx_dma);
		sc->sc_tx_dma = NULL;
		return;
	}
	sc->sc_rxbufs[0] = sc->sc_rxbuf;
	sc->sc_rxbufs[1] = sc->sc_rxbufs[0] + GFTTY_RXBUFSIZE;
	if (sc->sc_config->c_version == 0) {
		sc->sc_rxaddrs[0] = (bus_addr_t)sc->sc_rxbufs[0];
	} else {
		sc->sc_rxaddrs[0] = sc->sc_rx_dma->dm_segs[0].ds_addr;
	}
	sc->sc_rxaddrs[1] = sc->sc_rxaddrs[0] + GFTTY_RXBUFSIZE;
	gftty_reset_rxptrs(sc);

	struct tty *tp = tty_alloc();
	tp->t_oproc = gftty_start;
	tp->t_param = gftty_param;
	tp->t_softc = sc;

	mutex_init(&sc->sc_hwlock, MUTEX_DEFAULT, IPL_TTY);

	if (is_console) {
		/* Locate the major number. */
		int maj = cdevsw_lookup_major(&gftty_cdevsw);
		tp->t_dev = cn_tab->cn_dev = makedev(maj, device_unit(self));
	}

	mutex_spin_enter(&tty_lock);
	sc->sc_tty = tp;
	mutex_spin_exit(&tty_lock);

	tty_attach(tp);
}

/*
 * gftty_is_console --
 *	Returns true if the specified gftty instance is currently
 *	the console.
 */
bool
gftty_is_console(struct gftty_softc *sc)
{
	if (cn_tab == &gftty_consdev) {
		bool val;

		if (prop_dictionary_get_bool(device_properties(sc->sc_dev),
					     "is-console", &val)) {
			return val;
		}
	}
	return false;
}

/*
 * gftty_init_config --
 *	Initialize a config structure.
 */
static void
gftty_init_config(struct gftty_config *c, bus_space_tag_t bst,
    bus_space_handle_t bsh)
{
	c->c_bst = bst;
	c->c_bsh = bsh;
	c->c_version = REG_READ0(c, GFTTY_VERSION);
}

/*
 * gftty_alloc_config --
 *	Allocate a config structure, initialize it, and assign
 *	it to this device.
 */
void
gftty_alloc_config(struct gftty_softc *sc, bus_space_tag_t bst,
    bus_space_handle_t bsh)
{
	struct gftty_config *c = kmem_zalloc(sizeof(*c), KM_SLEEP);

	gftty_init_config(c, bst, bsh);
	sc->sc_config = c;
}

/*
 * gftty_set_buffer --
 *	Set the buffer address / length for an I/O operation.
 */
static void
gftty_set_buffer(struct gftty_config *c, bus_addr_t addr, bus_size_t size)
{
	REG_WRITE0(c, GFTTY_DATA_PTR, BUS_ADDR_LO32(addr));
	if (sizeof(bus_addr_t) == 8) {
		REG_WRITE0(c, GFTTY_DATA_PTR_HIGH, BUS_ADDR_HI32(addr));
	}
	REG_WRITE0(c, GFTTY_DATA_LEN, (uint32_t)size);
}

/*
 * gftty_flush --
 *	Flush input bytes.
 */
static bool
gftty_flush(struct gftty_softc *sc)
{
	uint32_t count;
	bool claimed = false;

	KASSERT(ttylocked(sc->sc_tty));

	mutex_spin_enter(&sc->sc_hwlock);

	while ((count = REG_READ(sc, GFTTY_BYTES_READY)) != 0) {
		claimed = true;
		if (count > GFTTY_RXBUFALLOC) {
			count = GFTTY_RXBUFALLOC;
		}
		gftty_set_buffer(sc->sc_config,
		    sc->sc_rx_dma->dm_segs[0].ds_addr, count);
		REG_WRITE(sc, GFTTY_CMD, CMD_READ_BUFFER);
	}

	mutex_spin_exit(&sc->sc_hwlock);

	gftty_reset_rxptrs(sc);

	return claimed;
}

/*
 * gftty_rx --
 *	Receive from the virtual TTY.
 */
static bool
gftty_rx(struct gftty_softc *sc)
{
	uint32_t count, avail;
	bool claimed = false;

	KASSERT(ttylocked(sc->sc_tty));

	mutex_spin_enter(&sc->sc_hwlock);

	count = REG_READ(sc, GFTTY_BYTES_READY);
	if (count != 0) {
		claimed = true;
		avail = GFTTY_RXBUFSIZE - sc->sc_rxpos;
		if (count > avail) {
			/*
			 * Receive what we can, but disable the interrupt
			 * until the buffer can be drained.
			 */
			REG_WRITE(sc, GFTTY_CMD, CMD_INT_DISABLE);
			count = avail;
		}
		if (count != 0) {
			bus_addr_t syncoff =
			    (sc->sc_rxaddr - sc->sc_rxaddrs[0]) + sc->sc_rxpos;

			bus_dmamap_sync(sc->sc_dmat, sc->sc_rx_dma,
			    syncoff, count, BUS_DMASYNC_PREREAD);
			gftty_set_buffer(sc->sc_config,
			    sc->sc_rxaddr + sc->sc_rxpos, count);
			REG_WRITE(sc, GFTTY_CMD, CMD_READ_BUFFER);
			sc->sc_rxpos += count;
			bus_dmamap_sync(sc->sc_dmat, sc->sc_rx_dma,
			    syncoff, count, BUS_DMASYNC_POSTREAD);
		}
		softint_schedule(sc->sc_rx_si);
	}

	mutex_spin_exit(&sc->sc_hwlock);

	return claimed;
}

/*
 * gftty_softrx --
 *	Software interrupt to comple Rx processing.
 */
static void
gftty_softrx(void *v)
{
	struct gftty_softc *sc = v;
	struct tty *tp = sc->sc_tty;
	int i, len;
	char *cp;

	ttylock(tp);
	cp = sc->sc_rxbuf;
	len = sc->sc_rxpos;
	sc->sc_rxcur ^= 1;
	sc->sc_rxbuf = sc->sc_rxbufs[sc->sc_rxcur];
	sc->sc_rxaddr = sc->sc_rxaddrs[sc->sc_rxcur];
	sc->sc_rxpos = 0;
	if (ISSET(tp->t_state, TS_ISOPEN)) {
		REG_WRITE(sc, GFTTY_CMD, CMD_INT_ENABLE);
	}
	ttyunlock(tp);

	for (i = 0; i < len; i++) {
		(*tp->t_linesw->l_rint)(*cp++, tp);
	}
}

/*
 * gftty_intr --
 *	Interrupt service routine.
 */
int
gftty_intr(void *v)
{
	struct gftty_softc *sc = v;
	struct tty *tp = sc->sc_tty;
	bool claimed;

	ttylock(tp);
	if (ISSET(tp->t_state, TS_ISOPEN)) {
		claimed = gftty_rx(sc);
	} else {
		claimed = gftty_flush(sc);
	}
	ttyunlock(tp);

	return claimed;
}

/*
 * gftty_open --
 *	cdevsw open routine.
 */
static int
gftty_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct gftty_softc *sc =
	    device_lookup_private(&gftty_cd, GFTTY_UNIT(dev));
	struct tty *tp;

	if (sc == NULL) {
		return ENXIO;
	}

	mutex_spin_enter(&tty_lock);
	tp = sc->sc_tty;
	mutex_spin_exit(&tty_lock);
	if (tp == NULL) {
		return ENXIO;
	}

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp)) {
		return EBUSY;
	}

	ttylock(tp);

	if (ISSET(tp->t_state, TS_KERN_ONLY)) {
		ttyunlock(tp);
		return EBUSY;
	}

	tp->t_oproc = gftty_start;
	tp->t_param = gftty_param;
	tp->t_dev = dev;

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		struct termios t;

		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		t.c_cflag = TTYDEF_CFLAG;
		t.c_ispeed = t.c_ospeed = TTYDEF_SPEED;
		(void) gftty_param_locked(tp, &t);
		ttsetwater(tp);

		gftty_flush(sc);
		REG_WRITE(sc, GFTTY_CMD, CMD_INT_ENABLE);
	}
	SET(tp->t_state, TS_CARR_ON);

	ttyunlock(tp);

	int error = ttyopen(tp, 0, ISSET(flag, O_NONBLOCK));
	if (error == 0) {
		error = (*tp->t_linesw->l_open)(dev, tp);
		if (error != 0) {
			ttyclose(tp);
		}
	}

	if (error != 0 &&
	    !ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		REG_WRITE(sc, GFTTY_CMD, CMD_INT_DISABLE);
	}

	return error;
}

/*
 * gftty_close --
 *	cdevsw close routine.
 */
static int
gftty_close(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct gftty_softc *sc =
	    device_lookup_private(&gftty_cd, GFTTY_UNIT(dev));

	KASSERT(sc != NULL);

	struct tty *tp = sc->sc_tty;

	ttylock(tp);

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		ttyunlock(tp);
		return 0;
	}

	if (ISSET(tp->t_state, TS_KERN_ONLY)) {
		ttyunlock(tp);
		return 0;
	}

	ttyunlock(tp);

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	ttylock(tp);
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		REG_WRITE(sc, GFTTY_CMD, CMD_INT_DISABLE);
	}
	ttyunlock(tp);

	return 0;
}

/*
 * gftty_read --
 *	cdevsw read routine.
 */
static int
gftty_read(dev_t dev, struct uio *uio, int flag)
{
	struct gftty_softc *sc =
	    device_lookup_private(&gftty_cd, GFTTY_UNIT(dev));

	KASSERT(sc != NULL);

	struct tty *tp = sc->sc_tty;
	return (*tp->t_linesw->l_read)(tp, uio, flag);
}

/*
 * gftty_write --
 *	cdevsw write routine.
 */
static int
gftty_write(dev_t dev, struct uio *uio, int flag)
{
	struct gftty_softc *sc =
	    device_lookup_private(&gftty_cd, GFTTY_UNIT(dev));

	KASSERT(sc != NULL);

	struct tty *tp = sc->sc_tty;
	return (*tp->t_linesw->l_write)(tp, uio, flag);
}

/*
 * gftty_poll --
 *	cdevsw poll routine.
 */
static int
gftty_poll(dev_t dev, int events, struct lwp *l)
{
	struct gftty_softc *sc =
	    device_lookup_private(&gftty_cd, GFTTY_UNIT(dev));

	KASSERT(sc != NULL);

	struct tty *tp = sc->sc_tty;
	return (*tp->t_linesw->l_poll)(tp, events, l);
}

/*
 * gftty_tty --
 *	cdevsw tty routine.
 */
static struct tty *
gftty_tty(dev_t dev)
{
	struct gftty_softc *sc =
	    device_lookup_private(&gftty_cd, GFTTY_UNIT(dev));
	
	KASSERT(sc != NULL);

	return sc->sc_tty;
}

/*
 * gftty_ioctl --
 *	cdevsw ioctl routine.
 */
static int
gftty_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct gftty_softc *sc =
	    device_lookup_private(&gftty_cd, GFTTY_UNIT(dev));

	KASSERT(sc != NULL);

	struct tty *tp = sc->sc_tty;
	int error;

	/* Do the line discipline ioctls first. */
	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH) {
		return error;
	}

	/* Next, the TTY ioctls. */
	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH) {
		return error;
	}

	/* None at this layer. */
	return EPASSTHROUGH;
}

/*
 * gftty_tx --
 *	Transmit a buffer on the virtual TTY using DMA.
 */
static void
gftty_tx(struct gftty_softc *sc, void *buf, size_t len)
{
	int error, i;

	KASSERT(len <= GFTTY_DMASIZE);

	error = bus_dmamap_load(sc->sc_dmat, sc->sc_tx_dma, buf, len,
	    NULL, BUS_DMA_NOWAIT);
	if (error) {
		/* XXX report error */
		return;
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_tx_dma, 0, len,
	    BUS_DMASYNC_PREWRITE);

	mutex_spin_enter(&sc->sc_hwlock);
	for (i = 0; i < sc->sc_tx_dma->dm_nsegs; i++) {
		gftty_set_buffer(sc->sc_config,
		    sc->sc_tx_dma->dm_segs[i].ds_addr,
		    sc->sc_tx_dma->dm_segs[i].ds_len);
		REG_WRITE(sc, GFTTY_CMD, CMD_WRITE_BUFFER);
	}
	mutex_spin_exit(&sc->sc_hwlock);

	bus_dmamap_sync(sc->sc_dmat, sc->sc_tx_dma, 0, len,
	    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_unload(sc->sc_dmat, sc->sc_tx_dma);
}

/*
 * gftty_start --
 *	TTY oproc routine.
 */
static void
gftty_start(struct tty *tp)
{
	struct gftty_softc *sc = tp->t_softc;
	u_char *tbuf;
	int n;

	KASSERT(ttylocked(tp));

	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP) ||
	    ttypull(tp) == 0) {
		return;
	}
	SET(tp->t_state, TS_BUSY);

	/*
	 * Drain the output from the ring buffer.  This will normally
	 * be one contiguous chunk, but we have to do it in two pieces
	 * when the ring wraps.
	 */

	n = ndqb(&tp->t_outq, 0);
	tbuf = tp->t_outq.c_cf;
	gftty_tx(sc, tbuf, n);
	ndflush(&tp->t_outq, n);

	if ((n = ndqb(&tp->t_outq, 0)) > 0) {
		tbuf = tp->t_outq.c_cf;
		gftty_tx(sc, tbuf, n);
		ndflush(&tp->t_outq, n);
	}

	CLR(tp->t_state, TS_BUSY);
	/* Come back if there's more to do. */
	if (ttypull(tp)) {
		SET(tp->t_state, TS_TIMEOUT);
		callout_schedule(&tp->t_rstrt_ch, (hz > 128) ? (hz / 128) : 1);
	}
}

/*
 * gftty_stop --
 *	cdevsw stop routine.
 */
static void
gftty_stop(struct tty *tp, int flag)
{
	KASSERT(ttylocked(tp));

	if (ISSET(tp->t_state, TS_BUSY)) {
		if (!ISSET(tp->t_state, TS_TTSTOP)) {
			SET(tp->t_state, TS_FLUSH);
		}
	}
}

/*
 * gftty_param_locked --
 *	Set TTY parameters.  TTY must be locked.
 */
static int
gftty_param_locked(struct tty *tp, struct termios *t)
{

	KASSERT(ttylocked(tp));

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	return 0;
}

/*
 * gftty_param --
 *	TTY param routine.
 */
static int
gftty_param(struct tty *tp, struct termios *t)
{
	int rv;

	ttylock(tp);
	rv = gftty_param_locked(tp, t);
	ttyunlock(tp);

	return rv;
}

/*
 * gftty console routines.
 */
static int
gftty_cngetc(dev_t dev)
{
	struct gftty_config * const c = &gftty_cnconfig;

	if (REG_READ0(c, GFTTY_BYTES_READY) == 0) {
		return -1;
	}

	/*
	 * XXX This is all terrible and should burn to the ground.
	 * XXX This device desperately needs to be improved with
	 * XXX a GET_CHAR register.
	 */
	bus_addr_t addr;
	uint8_t buf[1];

	if (c->c_version == 0) {
		addr = (bus_addr_t)buf;
	} else {
		addr = vtophys((vaddr_t)buf);
	}

	gftty_set_buffer(c, addr, sizeof(buf));
	REG_WRITE0(c, GFTTY_CMD, CMD_READ_BUFFER);

	return buf[0];
}

static void
gftty_cnputc(dev_t dev, int ch)
{
	REG_WRITE0(&gftty_cnconfig, GFTTY_PUT_CHAR, (unsigned char)ch);
}

static void
gftty_cnpollc(dev_t dev, int on)
{
	/* XXX */
}

/*
 * gftty_cnattach --
 *	Attach a Goldfish virtual TTY console.
 */
void
gftty_cnattach(bus_space_tag_t bst, bus_space_handle_t bsh)
{
	gftty_init_config(&gftty_cnconfig, bst, bsh);

	cn_tab = &gftty_consdev;
	cn_init_magic(&gftty_cnmagic_state);
	cn_set_magic("+++++");
}
