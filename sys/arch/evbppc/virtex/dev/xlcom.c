/* 	$NetBSD: xlcom.c,v 1.10.2.1 2014/08/10 06:53:57 tls Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xlcom.c,v 1.10.2.1 2014/08/10 06:53:57 tls Exp $");

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/intr.h>
#include <sys/bus.h>

#if defined(KGDB)
#include <sys/kgdb.h>
#endif /* KGDB */

#include <dev/cons.h>

#include <evbppc/virtex/virtex.h>
#include <evbppc/virtex/dev/xcvbusvar.h>
#include <evbppc/virtex/dev/xlcomreg.h>


#define XLCOM_UNIT_MASK 	0x7f
#define XLCOM_DIALOUT_MASK 	0x80

#define UNIT(dev) 		(minor(dev) & XLCOM_UNIT_MASK)
#define DIALOUT(dev) 		(minor(dev) & XLCOM_DIALOUT_MASK)

#define XLCOM_CHAR_PE 		0x8000 	/* Parity error flag */
#define XLCOM_CHAR_FE 		0x4000 	/* Frame error flag */
#define next(idx) 		(void)((idx) = ((idx) + 1) % XLCOM_RXBUF_SIZE)

#define XLCOM_RXBUF_SIZE 	1024

struct xlcom_softc {
	device_t 		sc_dev;
	struct tty 		*sc_tty;
	void 			*sc_ih;

	bus_space_tag_t 	sc_iot;
	bus_space_handle_t 	sc_ioh;

	/* Deffered execution context. */
	void 			*sc_rx_soft;
	void 			*sc_tx_soft;

	/* Receive buffer */
	u_short 		sc_rbuf[XLCOM_RXBUF_SIZE];
	volatile u_int 		sc_rput;
	volatile u_int 		sc_rget;
	volatile u_int 		sc_ravail;

 	/* Transmit buffer */
	u_char 			*sc_tba;
	u_int 			sc_tbc;
};

static int 	xlcom_intr(void *);
static void 	xlcom_rx_soft(void *);
static void 	xlcom_tx_soft(void *);
static void 	xlcom_reset(bus_space_tag_t, bus_space_handle_t);
static int 	xlcom_busy_getc(bus_space_tag_t, bus_space_handle_t);
static void 	xlcom_busy_putc(bus_space_tag_t, bus_space_handle_t, int);

/* System console interface. */
static int 	xlcom_cngetc(dev_t);
static void 	xlcom_cnputc(dev_t, int);
void 		xlcom_cninit(struct consdev *);

#if defined(KGDB)

void 		xlcom_kgdbinit(void);
static void 	xlcom_kgdb_putc(void *, int);
static int 	xlcom_kgdb_getc(void *);

#endif /* KGDB */

static struct cnm_state 	xlcom_cnm_state;

struct consdev consdev_xlcom = {
	.cn_probe 	= nullcnprobe,
	.cn_init 	= xlcom_cninit,
	.cn_getc 	= xlcom_cngetc,
	.cn_putc 	= xlcom_cnputc,
	.cn_pollc 	= nullcnpollc,
	.cn_pri 	= CN_REMOTE
};

/* Character device. */
static dev_type_open(xlcom_open);
static dev_type_read(xlcom_read);
static dev_type_write(xlcom_write);
static dev_type_ioctl(xlcom_ioctl);
static dev_type_poll(xlcom_poll);
static dev_type_close(xlcom_close);

static dev_type_tty(xlcom_tty);
static dev_type_stop(xlcom_stop);

const struct cdevsw xlcom_cdevsw = {
	.d_open = xlcom_open,
	.d_close = xlcom_close,
	.d_read = xlcom_read,
	.d_write = xlcom_write,
	.d_ioctl = xlcom_ioctl,
	.d_stop = xlcom_stop,
	.d_tty = xlcom_tty,
	.d_poll = xlcom_poll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

extern struct cfdriver xlcom_cd;

/* Terminal line. */
static int 	xlcom_param(struct tty *, struct termios *);
static void 	xlcom_start(struct tty *);

/* Generic device. */
static void 	xlcom_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(xlcom, sizeof(struct xlcom_softc),
    xcvbus_child_match, xlcom_attach, NULL, NULL);


static void
xlcom_attach(device_t parent, device_t self, void *aux)
{
	struct xcvbus_attach_args 	*vaa = aux;
	struct xlcom_softc 		*sc = device_private(self);
	struct tty 			*tp;
	dev_t 				dev;

	aprint_normal(": UartLite serial port\n");

	sc->sc_dev = self;

#if defined(KGDB)
	/* We don't want to share kgdb port with the user. */
	if (sc->sc_iot == kgdb_iot && sc->sc_ioh == kgdb_ioh) {
		aprint_error_dev(self, "already in use by kgdb\n");
		return;
	}
#endif /* KGDB */

	if ((sc->sc_ih = intr_establish(vaa->vaa_intr, IST_LEVEL, IPL_SERIAL,
	    xlcom_intr, sc)) == NULL) {
		aprint_error_dev(self, "could not establish interrupt\n");
		return ;
	}

	dev = makedev(cdevsw_lookup_major(&xlcom_cdevsw), device_unit(self));
	if (cn_tab == &consdev_xlcom) {
		cn_init_magic(&xlcom_cnm_state);
		cn_set_magic("\x23\x2e"); 		/* #. */
		cn_tab->cn_dev = dev;

		sc->sc_iot = consdev_iot;
		sc->sc_ioh = consdev_ioh;

		aprint_normal_dev(self, "console\n");
	} else {
		sc->sc_iot = vaa->vaa_iot;

		if (bus_space_map(vaa->vaa_iot, vaa->vaa_addr, XLCOM_SIZE, 0,
		    &sc->sc_ioh) != 0) {
			aprint_error_dev(self, "could not map registers\n");
			return;
		}

		/* Reset FIFOs. */
		xlcom_reset(sc->sc_iot, sc->sc_ioh);
	}

	sc->sc_tbc = 0;
	sc->sc_tba = NULL;

	sc->sc_rput = sc->sc_rget = 0;
	sc->sc_ravail = XLCOM_RXBUF_SIZE;

	sc->sc_rx_soft = softint_establish(SOFTINT_SERIAL, xlcom_rx_soft, sc);
	sc->sc_tx_soft = softint_establish(SOFTINT_SERIAL, xlcom_tx_soft, sc);

	if (sc->sc_rx_soft == NULL || sc->sc_tx_soft == NULL) {
		aprint_error_dev(self,
		    "could not establish Rx or Tx softintr\n");
		return;
	}

	tp = tty_alloc();
	tp->t_dev = dev;
	tp->t_oproc = xlcom_start;
	tp->t_param = xlcom_param;
	tp->t_hwiflow = NULL; 				/* No HW flow control */
	tty_attach(tp);

	/* XXX anything else to do for console early? */
	if (cn_tab == &consdev_xlcom) {
		/* Before first open, so that we can enter ddb(4). */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, XLCOM_CNTL,
		    CNTL_INTR_EN);
	}

	sc->sc_tty = tp;
}

/*
 * Misc hooks.
 */
static void
xlcom_tx_soft(void *arg)
{
	struct xlcom_softc 	*sc = (struct xlcom_softc *)arg;
	struct tty 		*tp = sc->sc_tty;

	if (tp->t_state & TS_FLUSH)
		tp->t_state &= ~TS_FLUSH;
	else
		ndflush(&tp->t_outq,
		    (int)(sc->sc_tba - tp->t_outq.c_cf));
	(tp->t_linesw->l_start)(tp);
}

static void
xlcom_rx_soft(void *arg)
{
	struct xlcom_softc 	*sc = (struct xlcom_softc *)arg;
	struct tty 		*tp = sc->sc_tty;
	int 			(*rint)(int, struct tty *);
	u_short 		c;
	int 			d;

	/*
	 * XXX: we don't do any synchronization, rput may change below
	 * XXX: our hands -- it doesn't seem to be troublesome as long
	 * XXX: as "sc->sc_rget = sc->sc_rput" is atomic.
	 */
	rint = tp->t_linesw->l_rint;

	/* Run until we catch our tail. */
	while (sc->sc_rput != sc->sc_rget) {
		c = sc->sc_rbuf[sc->sc_rget];

		next(sc->sc_rget);
		sc->sc_ravail++;

		d = (c & 0xff) |
		    ((c & XLCOM_CHAR_PE) != 0 ? TTY_PE : 0) |
		    ((c & XLCOM_CHAR_FE) != 0 ? TTY_FE : 0);

		/*
		 * Drop the rest of data if discipline runs out of buffer
		 * space. We'd use flow control here, if we had any.
		 */
		if ((rint)(d, tp) == -1) {
			sc->sc_rget = sc->sc_rput;
			return ;
		}
	}
}

static void
xlcom_send_chunk(struct xlcom_softc *sc)
{
	uint32_t 		stat;

	/* Chunk flushed, no more data available. */
	if (sc->sc_tbc <= 0) {
		return ;
	}

	/* Run as long as we have space and data. */
	while (sc->sc_tbc > 0) {
		stat = bus_space_read_4(sc->sc_iot, sc->sc_ioh, XLCOM_STAT);
		if (stat & STAT_TX_FULL)
			break;

		bus_space_write_4(sc->sc_iot, sc->sc_ioh, XLCOM_TX_FIFO,
		    *sc->sc_tba);

		sc->sc_tbc--;
		sc->sc_tba++;
	}

	/* Try to grab more data while FIFO drains. */
	if (sc->sc_tbc == 0) {
		sc->sc_tty->t_state &= ~TS_BUSY;
		softint_schedule(sc->sc_tx_soft);
	}
}

static void
xlcom_recv_chunk(struct xlcom_softc *sc)
{
	uint32_t 		stat;
	u_short 		c;
	u_int 			n;

	n = sc->sc_ravail;
	stat = bus_space_read_4(sc->sc_iot, sc->sc_ioh, XLCOM_STAT);

	/* Run as long as we have data and space. */
	while ((stat & STAT_RX_DATA) != 0 && sc->sc_ravail > 0) {
		c = bus_space_read_4(sc->sc_iot, sc->sc_ioh, XLCOM_RX_FIFO);

		cn_check_magic(sc->sc_tty->t_dev, c, xlcom_cnm_state);

		/* XXX: Should we pass rx-overrun upstream too? */
		c |= ((stat & STAT_PARITY_ERR) != 0 ? XLCOM_CHAR_PE : 0) |
		     ((stat & STAT_FRAME_ERR) != 0 ? XLCOM_CHAR_FE : 0);
		sc->sc_rbuf[sc->sc_rput] = c;
		sc->sc_ravail--;

		next(sc->sc_rput);
		stat = bus_space_read_4(sc->sc_iot, sc->sc_ioh, XLCOM_STAT);
	}

	/* Shedule completion hook if we received any. */
	if (n != sc->sc_ravail)
		softint_schedule(sc->sc_rx_soft);
}

static int
xlcom_intr(void *arg)
{
	struct xlcom_softc 	*sc = arg;
	uint32_t 		stat;

	/*
	 * Xilinx DS422, "OPB UART Lite v1.00b"
	 *
	 * If interrupts are enabled, an interrupt is generated when one
	 * of the following conditions is true:
	 */
	stat = bus_space_read_4(sc->sc_iot, sc->sc_ioh, XLCOM_STAT);

	/*
	 * 1. When there exists any valid character in the receive FIFO,
	 *    the interrupt stays active until the receive FIFO is empty.
	 *    This is a level interrupt.
	 */
	if (stat & STAT_RX_DATA)
		xlcom_recv_chunk(sc);

	/*
	 * 2. When the transmit FIFO goes from not empty to empty, such
	 *    as when the last character in the transmit FIFO is transmitted,
	 *    the interrupt is only active one clock cycle. This is an
	 *    edge interrupt.
	 */
	if (stat & STAT_TX_EMPTY)
		xlcom_send_chunk(sc);

	return (0);
}

/*
 * Character device.
 */
static int
xlcom_open(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct xlcom_softc 	*sc;
	struct tty 		*tp;
	int 			error, s;

	sc = device_lookup_private(&xlcom_cd, minor(dev));
	if (sc == NULL)
		return (ENXIO);

	tp = sc->sc_tty;

	s = spltty(); 							/* { */

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN,
	    tp) != 0) {
	    	error = EBUSY;
	    	goto fail;
	}

	/* Is this the first open? */
	if (!(tp->t_state & TS_ISOPEN) && tp->t_wopen == 0) {
		tp->t_dev = dev;

		/* Values hardwired at synthesis time. XXXFreza: xparam.h */
		tp->t_ispeed = tp->t_ospeed = B38400;
		tp->t_cflag = CLOCAL | CREAD | CS8;
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;

		ttychars(tp);
		ttsetwater(tp);

		/* Enable interrupt. */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, XLCOM_CNTL,
		    CNTL_INTR_EN);
	}

	error = ttyopen(tp, DIALOUT(dev), (flags & O_NONBLOCK));
	if (error)
		goto fail;

	error = (tp->t_linesw->l_open)(dev, tp);
	if (error)
		goto fail;

	splx(s); 							/* } */
	return (0);

 fail:
	/* XXXFreza: Shutdown if nobody else has the device open. */
	splx(s);
	return (error);
}

static int
xlcom_read(dev_t dev, struct uio *uio, int flag)
{
	struct xlcom_softc 	*sc;
	struct tty 		*tp;

	sc = device_lookup_private(&xlcom_cd, minor(dev));
	if (sc == NULL)
		return (ENXIO);
	tp = sc->sc_tty;

	return (tp->t_linesw->l_read)(tp, uio, flag);
}

static int
xlcom_write(dev_t dev, struct uio *uio, int flag)
{
	struct xlcom_softc 	*sc;
	struct tty 		*tp;

	sc = device_lookup_private(&xlcom_cd, minor(dev));
	if (sc == NULL)
		return (ENXIO);
	tp = sc->sc_tty;

	return (tp->t_linesw->l_write)(tp, uio, flag);
}

static int
xlcom_poll(dev_t dev, int events, struct lwp *l)
{
	struct xlcom_softc 	*sc;
	struct tty 		*tp;

	sc = device_lookup_private(&xlcom_cd, minor(dev));
	if (sc == NULL)
		return (ENXIO);
	tp = sc->sc_tty;

	return (tp->t_linesw->l_poll)(tp, events, l);
}

static struct tty *
xlcom_tty(dev_t dev)
{
	struct xlcom_softc 	*sc;
	struct tty 		*tp;

	sc = device_lookup_private(&xlcom_cd, minor(dev));
	if (sc == NULL)
		return (NULL);
	tp = sc->sc_tty;

	return (tp);
}

static int
xlcom_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct xlcom_softc 	*sc;
	struct tty 		*tp;
	int 			error;

	sc = device_lookup_private(&xlcom_cd, minor(dev));
	if (sc == NULL)
		return (ENXIO);
	tp = sc->sc_tty;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	/* XXXFreza: error = 0; switch (cmd); return error; */

	return (error);
}

static int
xlcom_close(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct xlcom_softc 	*sc;
	struct tty 		*tp;

	sc = device_lookup_private(&xlcom_cd, minor(dev));
	if (sc == NULL)
		return (ENXIO);
	tp = sc->sc_tty;

	(tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	/* Is this the last close? XXXFreza: hum? */
	if (!(tp->t_state & TS_ISOPEN) && tp->t_wopen == 0) {
	}

	return (0);
}

static void
xlcom_stop(struct tty *tp, int flag)
{
	struct xlcom_softc 	*sc;
	int 			s;

	sc = device_lookup_private(&xlcom_cd, UNIT(tp->t_dev));
	if (sc == NULL)
		return ;

	s = splserial();
	if (tp->t_state & TS_BUSY) {
		/* XXXFreza: make sure we stop xmitting at next chunk */

		if (! (tp->t_state & TS_TTSTOP))
			tp->t_state |= TS_FLUSH;
	}
	splx(s);
}

/*
 * Terminal line.
 */
static int
xlcom_param(struct tty *tp, struct termios *t)
{
	t->c_cflag &= ~HUPCL;

	if (tp->t_ospeed == t->c_ospeed &&
	    tp->t_ispeed == t->c_ispeed &&
	    tp->t_cflag  == t->c_cflag)
	    	return (0);

	return (EINVAL);
}

static void
xlcom_start(struct tty *tp)
{
	struct xlcom_softc 	*sc;
	int 			s1, s2;

	sc = device_lookup_private(&xlcom_cd, UNIT(tp->t_dev));
	if (sc == NULL)
		return ;

	s1 = spltty();

	if (tp->t_state & (TS_BUSY | TS_TIMEOUT | TS_TTSTOP)) {
		splx(s1);
		return ;
	}

	if (!ttypull(tp)) {
		splx(s1);
		return;
	}

	tp->t_state |= TS_BUSY;
	splx(s1);

	s2 = splserial();
	sc->sc_tba = tp->t_outq.c_cf;
	sc->sc_tbc = ndqb(&tp->t_outq, 0);
	xlcom_send_chunk(sc);
	splx(s2);
}

static void
xlcom_reset(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	/* Wait while the fifo drains. */
	while (! (bus_space_read_4(iot, ioh, XLCOM_STAT) & STAT_TX_EMPTY))
		;

	bus_space_write_4(iot, ioh, XLCOM_CNTL, CNTL_RX_CLEAR | CNTL_TX_CLEAR);
}

static int
xlcom_busy_getc(bus_space_tag_t t, bus_space_handle_t h)
{
	while (! (bus_space_read_4(t, h, XLCOM_STAT) & STAT_RX_DATA))
		;

	return (bus_space_read_4(t, h, XLCOM_RX_FIFO));
}

static void
xlcom_busy_putc(bus_space_tag_t t, bus_space_handle_t h, int c)
{
	while (bus_space_read_4(t, h, XLCOM_STAT) & STAT_TX_FULL)
		;

	bus_space_write_4(t, h, XLCOM_TX_FIFO, c);
}

/*
 * Console on UartLite.
 */
void
nullcnprobe(struct consdev *cn)
{
}

void 
xlcom_cninit(struct consdev *cn)
{
	if (bus_space_map(consdev_iot, CONS_ADDR, XLCOM_SIZE, 0, &consdev_ioh))
		panic("xlcom_cninit: could not map consdev_ioh");

	xlcom_reset(consdev_iot, consdev_ioh);
}

static int 
xlcom_cngetc(dev_t dev)
{
	return (xlcom_busy_getc(consdev_iot, consdev_ioh));
}

static void 
xlcom_cnputc(dev_t dev, int c)
{
	xlcom_busy_putc(consdev_iot, consdev_ioh, c);
}

/*
 * Remote GDB (aka "kgdb") interface.
 */
#if defined(KGDB)

static int
xlcom_kgdb_getc(void *arg)
{
	return (xlcom_busy_getc(kgdb_iot, kgdb_ioh));
}

static void
xlcom_kgdb_putc(void *arg, int c)
{
	xlcom_busy_putc(kgdb_iot, kgdb_ioh, c);
}

void
xlcom_kgdbinit(void)
{
	if (bus_space_map(kgdb_iot, KGDB_ADDR, XLCOM_SIZE, 0, &kgdb_ioh))
		panic("xlcom_kgdbinit: could not map kgdb_ioh");

	xlcom_reset(kgdb_iot, kgdb_ioh);

	kgdb_attach(xlcom_kgdb_getc, xlcom_kgdb_putc, NULL);
	kgdb_dev = 123; /* arbitrary strictly positive value */
}

#endif /* KGDB */
