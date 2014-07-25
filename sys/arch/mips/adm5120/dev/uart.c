/* $NetBSD: uart.c,v 1.12 2014/07/25 08:10:34 dholland Exp $ */

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uart.c,v 1.12 2014/07/25 08:10:34 dholland Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/intr.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>

#include <dev/cons.h>

#include <mips/adm5120/include/adm5120var.h>
#include <mips/adm5120/include/adm5120_obiovar.h>
#include <mips/adm5120/dev/uart.h>

#define REG_READ(o)	bus_space_read_4(sc->sc_st, sc->sc_ioh, (o))
#define REG_WRITE(o,v)	bus_space_write_4(sc->sc_st, sc->sc_ioh, (o),(v))

cons_decl(uart_);

dev_type_open(uart_open);
dev_type_open(uart_close);
dev_type_read(uart_read);
dev_type_write(uart_write);
dev_type_ioctl(uart_ioctl);
dev_type_tty(uart_tty);
dev_type_poll(uart_poll);
dev_type_stop(uart_stop);

const struct cdevsw uart_cdevsw = {
        .d_open = uart_open,
	.d_close = uart_close,
	.d_read = uart_read,
	.d_write = uart_write,
	.d_ioctl = uart_ioctl,
        .d_stop = uart_stop,
	.d_tty = uart_tty,
	.d_poll = uart_poll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

struct consdev uartcons = {
        .cn_getc = uart_cngetc,
	.cn_putc = uart_cnputc,
	.cn_pollc = uart_cnpollc,
        .cn_dev = NODEV,
	.cn_pri = CN_NORMAL
};

struct uart_softc {
        device_t		sc_dev;
	struct tty *		sc_tty;

        bus_space_tag_t		sc_st;
        bus_space_handle_t	sc_ioh;
	void *			sc_ih;
};

extern struct cfdriver uart_cd;
static int  uart_consattached;

static int  uart_probe  (device_t, cfdata_t, void *);
static void uart_attach (device_t, device_t, void *);

void	uart_start(struct tty *);
int	uart_param(struct tty *, struct termios *);
int	uart_intr(void *);

CFATTACH_DECL_NEW(uart, sizeof(struct uart_softc),
    uart_probe, uart_attach, NULL, NULL);

static int
uart_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct obio_attach_args * const oba = aux;

        if (strcmp(oba->oba_name, cf->cf_name) == 0)
                return (1);

        return (0);
}

static void
uart_attach(device_t parent, device_t self, void *aux)
{
        struct obio_attach_args * const oba = aux;
        struct uart_softc * const sc = device_private(self);
	struct tty *tp;
	int maj, minor;

	sc->sc_dev = self;
        sc->sc_st = oba->oba_st;
        if (bus_space_map(oba->oba_st, oba->oba_addr, 256, 0, &sc->sc_ioh)) {
                aprint_error("unable to map device\n");
                return;
	}

	/* Establish the interrupt. */
	sc->sc_ih = adm5120_intr_establish(oba->oba_irq, INTR_FIQ, uart_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error("unable to establish interrupt\n");
		return;
	}
	REG_WRITE(UART_CR_REG,UART_CR_PORT_EN|UART_CR_RX_INT_EN|UART_CR_RX_TIMEOUT_INT_EN);

	maj = cdevsw_lookup_major(&uart_cdevsw);
	minor = device_unit(sc->sc_dev);

	tp = tty_alloc();
	tp->t_oproc = uart_start;
	tp->t_param = uart_param;
	sc->sc_tty = tp;
	tp->t_dev = makedev(maj, minor);
	tty_attach(tp);
	if (minor == 0 && uart_consattached) {
		/* attach as console*/
		cn_tab->cn_dev = tp->t_dev;
		aprint_normal(" console");
	}
        aprint_normal("\n");
}

int 
uart_cnattach(void)
{
	cn_tab = &uartcons;
	uart_consattached = 1;
	return (0);
}

void
uart_cnputc(dev_t dev, int c)
{
	while ((*((volatile unsigned long *)0xb2600018)) & 0x20)
		continue;
	(*((volatile unsigned long *)0xb2600000)) = c;
}

int
uart_cngetc(dev_t dev)
{
	while ((*((volatile unsigned long *)0xb2600018)) & 0x10)
		continue;
	return (*((volatile unsigned long *)0xb2600000)) & 0xff;
}

void
uart_cnpollc(dev_t dev, int on)
{

}


/*
 * TTY device
 */

int
uart_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct uart_softc *sc = device_lookup_private(&uart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;
	int s, error = 0;

	s = spltty();

	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_CARR_ON;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG | CLOCAL;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = 115200;
		ttsetwater(tp);
	} else if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN,
	    tp) != 0) {
		splx(s);
		return (EBUSY);
	}

	splx(s);

	error = (*tp->t_linesw->l_open)(dev, tp);

	return (error);
}

int
uart_close(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct uart_softc *sc = device_lookup_private(&uart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	return (0);
}


int
uart_read(dev_t dev, struct uio *uio, int flag)
{
	struct uart_softc *sc = device_lookup_private(&uart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}
 
int
uart_write(dev_t dev, struct uio *uio, int flag)
{
	struct uart_softc *sc = device_lookup_private(&uart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;
 
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
uart_poll(dev_t dev, int events, struct lwp *l)
{
	struct uart_softc *sc = device_lookup_private(&uart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

int
uart_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct uart_softc *sc = device_lookup_private(&uart_cd, minor(dev));
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);
	return (ttioctl(tp, cmd, data, flag, l));
}

int
uart_param(struct tty *tp, struct termios *t)
{

	return (0);
}

struct tty*
uart_tty(dev_t dev)
{
	struct uart_softc *sc = device_lookup_private(&uart_cd, minor(dev));

	return sc->sc_tty;
}


void
uart_start(struct tty *tp)
{
	int s,i,cnt;

	s = spltty();
	if (tp->t_state & (TS_TTSTOP | TS_BUSY))
		goto out;
	ttypull(tp);
	tp->t_state |= TS_BUSY;
	while (tp->t_outq.c_cc != 0) {
		cnt = ndqb(&tp->t_outq, 0);
		for (i=0; i<cnt; i++)
			uart_cnputc(0,tp->t_outq.c_cf[i]);
		ndflush(&tp->t_outq, cnt);
	}
	tp->t_state &= ~TS_BUSY;
 out:
	splx(s);
}

void
uart_stop(struct tty *tp, int flag)
{
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY)
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	splx(s);
}

int
uart_intr(void *v)
{
	struct uart_softc *sc = v;
	struct tty *tp = sc->sc_tty;
	int c;

	if (REG_READ(UART_RSR_REG) & UART_RSR_BE) {
		REG_WRITE(UART_ECR_REG, UART_ECR_RSR);
		console_debugger();
	}

	while ((REG_READ(UART_FR_REG) & UART_FR_RX_FIFO_EMPTY) == 0) {
		c = REG_READ(UART_DR_REG) & 0xff;
		if (tp->t_state & TS_ISOPEN)
			(*tp->t_linesw->l_rint)(c, tp);
	}
	return 0;
}
