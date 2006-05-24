/* $NetBSD: sbjcn.c,v 1.10.8.2 2006/05/24 10:56:59 yamt Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* from: $NetBSD: com.c,v 1.172 2000/05/03 19:19:04 thorpej Exp */

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

/*
 * `sbjcn' driver, supports console over SiByte SB-1250 JTAG.
 *
 *  Accesses a section of JTAG memory space to mimic a console,
 *  if there's a matching program outside to communicate with.
 *  If nobody is there, things will be very quiet.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbjcn.c,v 1.10.8.2 2006/05/24 10:56:59 yamt Exp $");

#define	SBJCN_DEBUG

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/kauth.h>

#include <sbmips/dev/sbscd/sbscdvar.h>
#include <sbmips/dev/sbscd/sbjcnvar.h>
#include <dev/cons.h>
#include <machine/locore.h>

void	sbjcn_attach_channel(struct sbjcn_softc *sc, int chan, int intr);
static void sbjcncn_grabdword(struct sbjcn_channel *ch);
static char sbjcncn_nextbyte(struct sbjcn_channel *ch);
static void sbjcn_cngrabdword(void);

#if defined(DDB) || defined(KGDB)
static void sbjcn_enable_debugport(struct sbjcn_channel *ch);
#endif
void	sbjcn_config(struct sbjcn_channel *ch);
void	sbjcn_shutdown(struct sbjcn_channel *ch);
int	sbjcn_speed(long, long *);
static int cflag2modes(tcflag_t, u_char *, u_char *);
int	sbjcn_param(struct tty *, struct termios *);
void	sbjcn_start(struct tty *);
int	sbjcn_hwiflow(struct tty *, int);

void	sbjcn_loadchannelregs(struct sbjcn_channel *);
void	sbjcn_dohwiflow(struct sbjcn_channel *);
void	sbjcn_break(struct sbjcn_channel *, int);
void	sbjcn_modem(struct sbjcn_channel *, int);
void	tiocm_to_sbjcn(struct sbjcn_channel *, int, int);
int	sbjcn_to_tiocm(struct sbjcn_channel *);
void	sbjcn_iflush(struct sbjcn_channel *);

int	sbjcn_init(u_long addr, int chan, int rate, tcflag_t cflag);
int	sbjcn_common_getc(u_long addr, int chan);
void	sbjcn_common_putc(u_long addr, int chan, int c);

int	sbjcn_cngetc(dev_t dev);
void	sbjcn_cnputc(dev_t dev, int c);
void	sbjcn_cnpollc(dev_t dev, int on);

extern struct cfdriver sbjcn_cd;

dev_type_open(sbjcnopen);
dev_type_close(sbjcnclose);
dev_type_read(sbjcnread);
dev_type_write(sbjcnwrite);
dev_type_ioctl(sbjcnioctl);
dev_type_stop(sbjcnstop);
dev_type_tty(sbjcntty);

const struct cdevsw sbjcn_cdevsw = {
	sbjcnopen, sbjcnclose, sbjcnread, sbjcnwrite, sbjcnioctl,
	sbjcnstop, sbjcntty, nopoll, nommap, ttykqfilter, D_TTY
};

#define	integrate	static inline
integrate void sbjcn_rxsoft(struct sbjcn_channel *, struct tty *);
integrate void sbjcn_txsoft(struct sbjcn_channel *, struct tty *);
integrate void sbjcn_stsoft(struct sbjcn_channel *, struct tty *);
integrate void sbjcn_schedrx(struct sbjcn_channel *);
integrate void sbjcn_recv(struct sbjcn_channel *ch);
void	sbjcn_diag(void *);
void	sbjcn_callout(void *);

/*
 * Make this an option variable one can patch.
 * But be warned:  this must be a power of 2!
 */
u_int sbjcn_rbuf_size = SBJCN_RING_SIZE;

/* Stop input when 3/4 of the ring is full; restart when only 1/4 is full. */
u_int sbjcn_rbuf_hiwat = (SBJCN_RING_SIZE * 1) / 4;
u_int sbjcn_rbuf_lowat = (SBJCN_RING_SIZE * 3) / 4;

static int	sbjcn_cons_present;
static int	sbjcn_cons_attached;
static u_long	sbjcn_cons_addr;
static int	sbjcn_cons_chan;
static int	sbjcn_cons_rate;
static tcflag_t	sbjcn_cons_cflag;
static int	sbjcn_cons_waiting_input;
static uint64_t	sbjcn_cons_input_buf;

#ifdef KGDB
#include <sys/kgdb.h>

static int	sbjcn_kgdb_present;
static int	sbjcn_kgdb_attached;
static u_long	sbjcn_kgdb_addr;
static int	sbjcn_kgdb_chan;

int	sbjcn_kgdb_getc(void *);
void	sbjcn_kgdb_putc(void *, int);
#endif /* KGDB */

static int	sbjcn_match(struct device *, struct cfdata *, void *);
static void	sbjcn_attach(struct device *, struct device *, void *);

CFATTACH_DECL(sbjcn, sizeof(struct sbjcn_softc),
    sbjcn_match, sbjcn_attach, NULL, NULL);

#define	READ_REG(rp)		(mips3_ld((uint64_t *)(rp)))
#define	WRITE_REG(rp, val)	(mips3_sd((uint64_t *)(rp), (val)))

#define	JTAG_CONS_CONTROL  0x00
#define	JTAG_CONS_INPUT    0x20
#define	JTAG_CONS_OUTPUT   0x40
#define	JTAG_CONS_MAGICNUM 0x50FABEEF12349873

#define	jtag_input_len(data)    (((data) >> 56) & 0xFF)


static int
sbjcn_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct sbscd_attach_args *sap = aux;

	if (sap->sa_locs.sa_type != SBSCD_DEVTYPE_JTAGCONS)
		return (0);

	return 1;
}

static void
sbjcn_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbjcn_softc *sc = (struct sbjcn_softc *)self;
	struct sbscd_attach_args *sap = aux;

	sc->sc_addr = sap->sa_base + sap->sa_locs.sa_offset;

	printf("\n");
	sbjcn_attach_channel(sc, 0, sap->sa_locs.sa_intr[0]);
}

void
sbjcn_attach_channel(struct sbjcn_softc *sc, int chan, int intr)
{
	struct sbjcn_channel *ch = &sc->sc_channels[chan];
	u_long chan_addr;
	struct tty *tp;

	ch->ch_sc = sc;
	ch->ch_num = chan;

	chan_addr = sc->sc_addr + (0x100 * chan);
	ch->ch_base = (void *)MIPS_PHYS_TO_KSEG1(chan_addr);
	ch->ch_input_reg =
	    (void *)MIPS_PHYS_TO_KSEG1(sc->sc_addr + JTAG_CONS_INPUT);
	ch->ch_output_reg =
	    (void *)MIPS_PHYS_TO_KSEG1(sc->sc_addr + JTAG_CONS_OUTPUT);
	ch->ch_control_reg =
	    (void *)MIPS_PHYS_TO_KSEG1(sc->sc_addr + JTAG_CONS_CONTROL);
	ch->ch_waiting_input = 0;

	ch->ch_i_dcd = ch->ch_i_dcd_pin = 0 /* XXXCGD */;
	ch->ch_i_cts = ch->ch_i_cts_pin = 0 /* XXXCGD */;
	ch->ch_i_dsr = ch->ch_i_dsr_pin = 0 /* XXXCGD */;
	ch->ch_i_ri = ch->ch_i_ri_pin = 0 /* XXXCGD */;
	ch->ch_i_mask =
	    ch->ch_i_dcd | ch->ch_i_cts | ch->ch_i_dsr | ch->ch_i_ri;
	ch->ch_o_dtr = ch->ch_o_dtr_pin = 0 /* XXXCGD */;
	ch->ch_o_rts = ch->ch_o_rts_pin = 0 /* XXXCGD */;
	ch->ch_o_mask = ch->ch_o_dtr | ch->ch_o_rts;

	callout_init(&ch->ch_diag_callout);
	callout_init(&ch->ch_callout);

	/* Disable interrupts before configuring the device. */
	ch->ch_imr = 0;

	if (sbjcn_cons_present &&
	    sbjcn_cons_addr == chan_addr && sbjcn_cons_chan == chan) {
		sbjcn_cons_attached = 1;

		/* Make sure the console is always "hardwired". */
		delay(1000);			/* wait for output to finish */
		SET(ch->ch_hwflags, SBJCN_HW_CONSOLE);
		SET(ch->ch_swflags, TIOCFLAG_SOFTCAR);
	}

	tp = ttymalloc();
	tp->t_oproc = sbjcn_start;
	tp->t_param = sbjcn_param;
	tp->t_hwiflow = sbjcn_hwiflow;

	ch->ch_tty = tp;
	ch->ch_rbuf = malloc(sbjcn_rbuf_size << 1, M_DEVBUF, M_NOWAIT);
	if (ch->ch_rbuf == NULL) {
		printf("%s: channel %d: unable to allocate ring buffer\n",
		    sc->sc_dev.dv_xname, chan);
		return;
	}
	ch->ch_ebuf = ch->ch_rbuf + (sbjcn_rbuf_size << 1);

	tty_attach(tp);

	if (ISSET(ch->ch_hwflags, SBJCN_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&sbjcn_cdevsw);

		cn_tab->cn_dev = makedev(maj,
		    (device_unit(&sc->sc_dev) << 1) + chan);

		printf("%s: channel %d: console\n", sc->sc_dev.dv_xname, chan);
	}

#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * the kgdb device, it has exclusive use.
	 */
	if (sbjcn_kgdb_present &&
	    sbjcn_kgdb_addr == chan_addr && sbjcn_kgdb_chan == chan) {
		sbjcn_kgdb_attached = 1;

		SET(ch->ch_hwflags, SBJCN_HW_KGDB);
		printf("%s: channel %d: kgdb\n", sc->sc_dev.dv_xname, chan);
	}
#endif

	sbjcn_config(ch);

	callout_reset(&ch->ch_callout, hz/10, sbjcn_callout, ch);

	SET(ch->ch_hwflags, SBJCN_HW_DEV_OK);
}

int
sbjcn_speed(long speed, long *brcp)
{
#define	divrnd(n, q)	(((n)*2/(q)+1)/2)	/* divide and round off */

	int x, err;
	int frequency = 100000000;

	*brcp = divrnd(frequency / 20, speed) - 1;

	if (speed <= 0)
		return (-1);
	x = divrnd(frequency / 20, speed);
	if (x <= 0)
		return (-1);
	err = divrnd(((quad_t)frequency) * 1000 / 20, speed * x) - 1000;
	if (err < 0)
		err = -err;
	if (err > SBJCN_TOLERANCE)
		return (-1);
	*brcp = x - 1;
	return (0);

#undef	divrnd
}

#ifdef SBJCN_DEBUG
void	sbjcn_status(struct sbjcn_channel *, char *);

int	sbjcn_debug = 1 /* XXXCGD */;

void
sbjcn_status(struct sbjcn_channel *ch, char *str)
{
	struct sbjcn_softc *sc = ch->ch_sc;
	struct tty *tp = ch->ch_tty;

	printf("%s: chan %d: %s %sclocal  %sdcd %sts_carr_on %sdtr %stx_stopped\n",
	    sc->sc_dev.dv_xname, ch->ch_num, str,
	    ISSET(tp->t_cflag, CLOCAL) ? "+" : "-",
	    ISSET(ch->ch_iports, ch->ch_i_dcd) ? "+" : "-",
	    ISSET(tp->t_state, TS_CARR_ON) ? "+" : "-",
	    ISSET(ch->ch_oports, ch->ch_o_dtr) ? "+" : "-",
	    ch->ch_tx_stopped ? "+" : "-");

	printf("%s: chan %d: %s %scrtscts %scts %sts_ttstop  %srts %xrx_flags\n",
	    sc->sc_dev.dv_xname, ch->ch_num, str,
	    ISSET(tp->t_cflag, CRTSCTS) ? "+" : "-",
	    ISSET(ch->ch_iports, ch->ch_i_cts) ? "+" : "-",
	    ISSET(tp->t_state, TS_TTSTOP) ? "+" : "-",
	    ISSET(ch->ch_oports, ch->ch_o_rts) ? "+" : "-",
	    ch->ch_rx_flags);
}
#endif

#if defined(DDB) || defined(KGDB)
static void
sbjcn_enable_debugport(struct sbjcn_channel *ch)
{
	int s;

	/* Turn on line break interrupt, set carrier. */
	s = splserial();

	ch->ch_imr = 0x04;
	SET(ch->ch_oports, ch->ch_o_dtr | ch->ch_o_rts);

	splx(s);
}
#endif

void
sbjcn_config(struct sbjcn_channel *ch)
{

	/* Disable interrupts before configuring the device. */
	ch->ch_imr = 0x00;

#ifdef DDB
	if (ISSET(ch->ch_hwflags, SBJCN_HW_CONSOLE))
		sbjcn_enable_debugport(ch);
#endif

#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * the kgdb device, it has exclusive use.
	 */
	if (ISSET(ch->ch_hwflags, SBJCN_HW_KGDB))
		sbjcn_enable_debugport(ch);
#endif
}

void
sbjcn_shutdown(struct sbjcn_channel *ch)
{
	struct tty *tp = ch->ch_tty;
	int s;

	s = splserial();

	/* If we were asserting flow control, then deassert it. */
	SET(ch->ch_rx_flags, RX_IBUF_BLOCKED);
	sbjcn_dohwiflow(ch);

	/* Clear any break condition set with TIOCSBRK. */
	sbjcn_break(ch, 0);

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		sbjcn_modem(ch, 0);
		(void) tsleep(ch, TTIPRI, ttclos, hz);
	}

	/* Turn off interrupts. */
#ifdef DDB
	if (ISSET(ch->ch_hwflags, SBJCN_HW_CONSOLE))
		ch->ch_imr = 0x04; /* interrupt on break */
	else
#endif
		ch->ch_imr = 0;

	splx(s);
}

int
sbjcnopen(dev_t dev, int flag, int mode, struct proc *p)
{
	int unit = SBJCN_UNIT(dev);
	int chan = SBJCN_CHAN(dev);
	struct sbjcn_softc *sc;
	struct sbjcn_channel *ch;
	struct tty *tp;
	int s, s2;
	int error;

	if (unit >= sbjcn_cd.cd_ndevs)
		return (ENXIO);
	sc = sbjcn_cd.cd_devs[unit];
	if (sc == 0)
		return (ENXIO);
	ch = &sc->sc_channels[chan];
	if (!ISSET(ch->ch_hwflags, SBJCN_HW_DEV_OK) || ch->ch_rbuf == NULL)
		return (ENXIO);

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(ch->ch_hwflags, SBJCN_HW_KGDB))
		return (EBUSY);
#endif

	tp = ch->ch_tty;

	if (ISSET(tp->t_state, TS_ISOPEN) &&
	    ISSET(tp->t_state, TS_XCLUDE) &&
	    kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag) != 0)
		return (EBUSY);

	s = spltty();

	/*
	 * Do the following iff this is a first open.
	 */
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		struct termios t;

		tp->t_dev = dev;

		s2 = splserial();

		/* Turn on receive, break, and status change interrupts. */
		ch->ch_imr = 0xe;

		/* Fetch the current modem control status, needed later. */
		ch->ch_iports = 0;
		ch->ch_iports_delta = 0;
		splx(s2);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(ch->ch_hwflags, SBJCN_HW_CONSOLE)) {
			t.c_ospeed = sbjcn_cons_rate;
			t.c_cflag = sbjcn_cons_cflag;
		} else {
			t.c_ospeed = TTYDEF_SPEED;
			t.c_cflag = TTYDEF_CFLAG;
		}
		if (ISSET(ch->ch_swflags, TIOCFLAG_CLOCAL))
			SET(t.c_cflag, CLOCAL);
		if (ISSET(ch->ch_swflags, TIOCFLAG_CRTSCTS))
			SET(t.c_cflag, CRTSCTS);
		if (ISSET(ch->ch_swflags, TIOCFLAG_MDMBUF))
			SET(t.c_cflag, MDMBUF);
		/* Make sure sbjcn_param() will do something. */
		tp->t_ospeed = 0;
		(void) sbjcn_param(tp, &t);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		ttychars(tp);
		ttsetwater(tp);

		s2 = splserial();

		/*
		 * Turn on DTR.  We must always do this, even if carrier is not
		 * present, because otherwise we'd have to use TIOCSDTR
		 * immediately after setting CLOCAL, which applications do not
		 * expect.  We always assert DTR while the device is open
		 * unless explicitly requested to deassert it.
		 */
		sbjcn_modem(ch, 1);

		/* Clear the input ring, and unblock. */
		ch->ch_rbput = ch->ch_rbget = ch->ch_rbuf;
		ch->ch_rbavail = sbjcn_rbuf_size;
		sbjcn_iflush(ch);
		CLR(ch->ch_rx_flags, RX_ANY_BLOCK);
		sbjcn_dohwiflow(ch);

#ifdef SBJCN_DEBUG
		if (sbjcn_debug)
			sbjcn_status(ch, "sbjcnopen  ");
#endif

		splx(s2);
	}

	splx(s);

	error = ttyopen(tp, SBJCN_DIALOUT(dev), ISSET(flag, O_NONBLOCK));
	if (error)
		goto bad;

	error = (*tp->t_linesw->l_open)(dev, tp);
	if (error)
		goto bad;

	return (0);

bad:
	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * We failed to open the device, and nobody else had it opened.
		 * Clean up the state as appropriate.
		 */
		sbjcn_shutdown(ch);
	}

	return (error);
}

int
sbjcnclose(dev_t dev, int flag, int mode, struct proc *p)
{
	struct sbjcn_softc *sc = sbjcn_cd.cd_devs[SBJCN_UNIT(dev)];
	struct sbjcn_channel *ch = &sc->sc_channels[SBJCN_CHAN(dev)];
	struct tty *tp = ch->ch_tty;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return (0);

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);

	if (!ISSET(tp->t_state, TS_ISOPEN) && tp->t_wopen == 0) {
		/*
		 * Although we got a last close, the device may still be in
		 * use; e.g. if this was the dialout node, and there are still
		 * processes waiting for carrier on the non-dialout node.
		 */
		sbjcn_shutdown(ch);
	}

	return (0);
}

int
sbjcnread(dev_t dev, struct uio *uio, int flag)
{
	struct sbjcn_softc *sc = sbjcn_cd.cd_devs[SBJCN_UNIT(dev)];
	struct sbjcn_channel *ch = &sc->sc_channels[SBJCN_CHAN(dev)];
	struct tty *tp = ch->ch_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
sbjcnwrite(dev_t dev, struct uio *uio, int flag)
{
	struct sbjcn_softc *sc = sbjcn_cd.cd_devs[SBJCN_UNIT(dev)];
	struct sbjcn_channel *ch = &sc->sc_channels[SBJCN_CHAN(dev)];
	struct tty *tp = ch->ch_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

struct tty *
sbjcntty(dev_t dev)
{
	struct sbjcn_softc *sc = sbjcn_cd.cd_devs[SBJCN_UNIT(dev)];
	struct sbjcn_channel *ch = &sc->sc_channels[SBJCN_CHAN(dev)];
	struct tty *tp = ch->ch_tty;

	return (tp);
}

int
sbjcnioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct sbjcn_softc *sc = sbjcn_cd.cd_devs[SBJCN_UNIT(dev)];
	struct sbjcn_channel *ch = &sc->sc_channels[SBJCN_CHAN(dev)];
	struct tty *tp = ch->ch_tty;
	int error;
	int s;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	error = 0;

	s = splserial();

	switch (cmd) {
	case TIOCSBRK:
		sbjcn_break(ch, 1);
		break;

	case TIOCCBRK:
		sbjcn_break(ch, 0);
		break;

	case TIOCSDTR:
		sbjcn_modem(ch, 1);
		break;

	case TIOCCDTR:
		sbjcn_modem(ch, 0);
		break;

	case TIOCGFLAGS:
		*(int *)data = ch->ch_swflags;
		break;

	case TIOCSFLAGS:
		error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag);
		if (error)
			break;
		ch->ch_swflags = *(int *)data;
		break;

	case TIOCMSET:
	case TIOCMBIS:
	case TIOCMBIC:
		tiocm_to_sbjcn(ch, cmd, *(int *)data);
		break;

	case TIOCMGET:
		*(int *)data = sbjcn_to_tiocm(ch);
		break;

	default:
		error = ENOTTY;
		break;
	}

	splx(s);

#ifdef SBJCN_DEBUG
	if (sbjcn_debug)
		sbjcn_status(ch, "sbjcn_ioctl ");
#endif

	return (error);
}

integrate void
sbjcn_schedrx(struct sbjcn_channel *ch)
{

	ch->ch_rx_ready = 1;

	/* Next callout will detect this flag. */
}

void
sbjcn_break(struct sbjcn_channel *ch, int onoff)
{
	/* XXXKW do something? */
}

void
sbjcn_modem(struct sbjcn_channel *ch, int onoff)
{

	if (ch->ch_o_dtr == 0)
		return;

	if (onoff)
		SET(ch->ch_oports, ch->ch_o_dtr);
	else
		CLR(ch->ch_oports, ch->ch_o_dtr);

	if (!ch->ch_heldchange) {
		if (ch->ch_tx_busy) {
			ch->ch_heldtbc = ch->ch_tbc;
			ch->ch_tbc = 0;
			ch->ch_heldchange = 1;
		} else
			sbjcn_loadchannelregs(ch);
	}
}

void
tiocm_to_sbjcn(struct sbjcn_channel *ch, int how, int ttybits)
{
	u_char bits;

	bits = 0;
	if (ISSET(ttybits, TIOCM_DTR))
		SET(bits, ch->ch_o_dtr);
	if (ISSET(ttybits, TIOCM_RTS))
		SET(bits, ch->ch_o_rts);

	switch (how) {
	case TIOCMBIC:
		CLR(ch->ch_oports, bits);
		break;

	case TIOCMBIS:
		SET(ch->ch_oports, bits);
		break;

	case TIOCMSET:
		ch->ch_oports = bits;
		break;
	}

	if (!ch->ch_heldchange) {
		if (ch->ch_tx_busy) {
			ch->ch_heldtbc = ch->ch_tbc;
			ch->ch_tbc = 0;
			ch->ch_heldchange = 1;
		} else
			sbjcn_loadchannelregs(ch);
	}
}

int
sbjcn_to_tiocm(struct sbjcn_channel *ch)
{
	u_char hwbits;
	int ttybits = 0;

	hwbits = ch->ch_oports;
	if (ISSET(hwbits, ch->ch_o_dtr))
		SET(ttybits, TIOCM_DTR);
	if (ISSET(hwbits, ch->ch_o_rts))
		SET(ttybits, TIOCM_RTS);

	hwbits = ch->ch_iports;
	if (ISSET(hwbits, ch->ch_i_dcd))
		SET(ttybits, TIOCM_CD);
	if (ISSET(hwbits, ch->ch_i_cts))
		SET(ttybits, TIOCM_CTS);
	if (ISSET(hwbits, ch->ch_i_dsr))
		SET(ttybits, TIOCM_DSR);
	if (ISSET(hwbits, ch->ch_i_ri))
		SET(ttybits, TIOCM_RI);

	if (ch->ch_imr != 0)
		SET(ttybits, TIOCM_LE);

	return (ttybits);
}

static int
cflag2modes(cflag, mode1p, mode2p)
	tcflag_t cflag;
	u_char *mode1p;
	u_char *mode2p;
{
	u_char mode1;
	u_char mode2;
	int err = 0;

	mode1 = mode2 = 0;

	switch (ISSET(cflag, CSIZE)) {
	case CS7:
		mode1 |= 2;			/* XXX */
		break;
	default:
		err = -1;
		/* FALLTHRU for sanity */
	case CS8:
		mode1 |= 3;			/* XXX */
		break;
	}
	if (!ISSET(cflag, PARENB))
		mode1 |= 2 << 3;
	else {
		mode1 |= 0 << 3;
		if (ISSET(cflag, PARODD))
			mode1 |= 1 << 2;
	}

	if (ISSET(cflag, CSTOPB))
		mode2 |= 1 << 3;		/* two stop bits XXX not std */

	if (ISSET(cflag, CRTSCTS)) {
		mode1 |= 1 << 7;
		mode2 |= 1 << 4;
	}

	*mode1p = mode1;
	*mode2p = mode2;
	return (err);
}

int
sbjcn_param(struct tty *tp, struct termios *t)
{
	struct sbjcn_softc *sc = sbjcn_cd.cd_devs[SBJCN_UNIT(tp->t_dev)];
	struct sbjcn_channel *ch = &sc->sc_channels[SBJCN_CHAN(tp->t_dev)];
	long brc;
	u_char mode1, mode2;
	int s;

/* XXX reset to console parameters if console? */
#if 0
XXX disable, enable.
#endif

	/* Check requested parameters. */
	if (sbjcn_speed(t->c_ospeed, &brc) < 0)
		return (EINVAL);
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return (EINVAL);

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(ch->ch_swflags, TIOCFLAG_SOFTCAR) ||
	    ISSET(ch->ch_hwflags, SBJCN_HW_CONSOLE)) {
		SET(t->c_cflag, CLOCAL);
		CLR(t->c_cflag, HUPCL);
	}

	/*
	 * If there were no changes, don't do anything.  This avoids dropping
	 * input and improves performance when all we did was frob things like
	 * VMIN and VTIME.
	 */
	if (tp->t_ospeed == t->c_ospeed &&
	    tp->t_cflag == t->c_cflag)
		return (0);

	if (cflag2modes(t->c_cflag, &mode1, &mode2) < 0)
		return (EINVAL);

	s = splserial();

	ch->ch_mode1 = mode1;
	ch->ch_mode2 = mode2;

	/*
	 * If we're not in a mode that assumes a connection is present, then
	 * ignore carrier changes.
	 */
	if (ISSET(t->c_cflag, CLOCAL | MDMBUF))
		ch->ch_i_dcd = 0;
	else
		ch->ch_i_dcd = ch->ch_i_dcd_pin;
	/*
	 * Set the flow control pins depending on the current flow control
	 * mode.
	 */
	if (ISSET(t->c_cflag, CRTSCTS)) {
		ch->ch_o_dtr = ch->ch_o_dtr_pin;
		ch->ch_o_rts = ch->ch_o_rts_pin;
		ch->ch_i_cts = ch->ch_i_cts_pin;
		/* hw controle enable bits in mod regs set by cflag2modes */
	} else if (ISSET(t->c_cflag, MDMBUF)) {
		/*
		 * For DTR/DCD flow control, make sure we don't toggle DTR for
		 * carrier detection.
		 */
		ch->ch_o_dtr = 0;
		ch->ch_o_rts = ch->ch_o_dtr_pin;
		ch->ch_i_cts = ch->ch_i_dcd_pin;
	} else {
		/*
		 * If no flow control, then always set RTS.  This will make
		 * the other side happy if it mistakenly thinks we're doing
		 * RTS/CTS flow control.
		 */
		ch->ch_o_dtr = ch->ch_o_dtr_pin | ch->ch_o_rts_pin;
		ch->ch_o_rts = 0;
		ch->ch_i_cts = 0;
		if (ISSET(ch->ch_oports, ch->ch_o_dtr_pin))
			SET(ch->ch_oports, ch->ch_o_rts_pin);
		else
			CLR(ch->ch_oports, ch->ch_o_rts_pin);
	}
	/* XXX maybe mask the ports which generate intrs? */

	ch->ch_brc = brc;

	/* XXX maybe set fifo-full receive mode if RTSCTS and high speed? */

	/* And copy to tty. */
	tp->t_ispeed = 0;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;

	if (!ch->ch_heldchange) {
		if (ch->ch_tx_busy) {
			ch->ch_heldtbc = ch->ch_tbc;
			ch->ch_tbc = 0;
			ch->ch_heldchange = 1;
		} else
			sbjcn_loadchannelregs(ch);
	}

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		/* Disable the high water mark. */
		ch->ch_r_hiwat = 0;
		ch->ch_r_lowat = 0;
		if (ISSET(ch->ch_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(ch->ch_rx_flags, RX_TTY_OVERFLOWED);
			sbjcn_schedrx(ch);
		}
		if (ISSET(ch->ch_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED)) {
			CLR(ch->ch_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED);
			sbjcn_dohwiflow(ch);
		}
	} else {
		ch->ch_r_hiwat = sbjcn_rbuf_hiwat;
		ch->ch_r_lowat = sbjcn_rbuf_lowat;
	}

	splx(s);

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	(void) (*tp->t_linesw->l_modem)(tp,
	    ISSET(ch->ch_iports, ch->ch_i_dcd));

#ifdef SBJCN_DEBUG
	if (sbjcn_debug)
		sbjcn_status(ch, "sbjcnparam ");
#endif

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (ch->ch_tx_stopped) {
			ch->ch_tx_stopped = 0;
			sbjcn_start(tp);
		}
	}

	return (0);
}

void
sbjcn_iflush(struct sbjcn_channel *ch)
{
	uint64_t reg;
	int timo;

	timo = 50000;
	/* flush any pending I/O */
	while (((reg = READ_REG(ch->ch_input_reg)) != 0) && --timo)
		;

#ifdef DIAGNOSTIC
	if (!timo)
		printf("%s: sbjcn_iflush timeout %02x\n",
		    ch->ch_sc->sc_dev.dv_xname, reg);
#endif
}

void
sbjcn_loadchannelregs(struct sbjcn_channel *ch)
{
}

int
sbjcn_hwiflow(struct tty *tp, int block)
{
	struct sbjcn_softc *sc = sbjcn_cd.cd_devs[SBJCN_UNIT(tp->t_dev)];
	struct sbjcn_channel *ch = &sc->sc_channels[SBJCN_CHAN(tp->t_dev)];
	int s;

	if (ch->ch_o_rts == 0)
		return (0);

	s = splserial();
	if (block) {
		if (!ISSET(ch->ch_rx_flags, RX_TTY_BLOCKED)) {
			SET(ch->ch_rx_flags, RX_TTY_BLOCKED);
			sbjcn_dohwiflow(ch);
		}
	} else {
		if (ISSET(ch->ch_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(ch->ch_rx_flags, RX_TTY_OVERFLOWED);
			sbjcn_schedrx(ch);
		}
		if (ISSET(ch->ch_rx_flags, RX_TTY_BLOCKED)) {
			CLR(ch->ch_rx_flags, RX_TTY_BLOCKED);
			sbjcn_dohwiflow(ch);
		}
	}
	splx(s);
	return (1);
}

/*
 * (un)block input via hw flowcontrol
 */
void
sbjcn_dohwiflow(struct sbjcn_channel *ch)
{

	if (ch->ch_o_rts == 0)
		return;

	if (ISSET(ch->ch_rx_flags, RX_ANY_BLOCK)) {
		CLR(ch->ch_oports, ch->ch_o_rts);
		CLR(ch->ch_oports_active, ch->ch_o_rts);
	} else {
		SET(ch->ch_oports, ch->ch_o_rts);
		SET(ch->ch_oports_active, ch->ch_o_rts);
	}
}

void
sbjcn_start(struct tty *tp)
{
	struct sbjcn_softc *sc = sbjcn_cd.cd_devs[SBJCN_UNIT(tp->t_dev)];
	struct sbjcn_channel *ch = &sc->sc_channels[SBJCN_CHAN(tp->t_dev)];
	int s;

	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		goto out;
	if (ch->ch_tx_stopped)
		goto out;

	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (ISSET(tp->t_state, TS_ASLEEP)) {
			CLR(tp->t_state, TS_ASLEEP);
			wakeup(&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
		if (tp->t_outq.c_cc == 0)
			goto out;
	}

	/* Grab the first contiguous region of buffer space. */
	{
		u_char *tba;
		int tbc;

		tba = tp->t_outq.c_cf;
		tbc = ndqb(&tp->t_outq, 0);

		(void)splserial();

		ch->ch_tba = tba;
		ch->ch_tbc = tbc;
	}

	SET(tp->t_state, TS_BUSY);
	ch->ch_tx_busy = 1;

	/* Output the first chunk of the contiguous buffer. */
	{
		while (ch->ch_tbc) {
			uint64_t data;
			int bytes, i;

			bytes = (ch->ch_tbc > 7) ? 7 : ch->ch_tbc;
			data = bytes;
			for (i=0; i<bytes; i++) {
				data <<= 8;
				data |= *ch->ch_tba++;
			}
			if (bytes < 7)
				data <<= 56-(bytes<<3);
			ch->ch_tbc -= bytes;
			WRITE_REG(ch->ch_output_reg, data);
		}
		ch->ch_tx_busy = 0;
		ch->ch_tx_done = 1;
	}
out:
	splx(s);
	return;
}

/*
 * Stop output on a line.
 */
void
sbjcnstop(struct tty *tp, int flag)
{
	struct sbjcn_softc *sc = sbjcn_cd.cd_devs[SBJCN_UNIT(tp->t_dev)];
	struct sbjcn_channel *ch = &sc->sc_channels[SBJCN_CHAN(tp->t_dev)];
	int s;

	s = splserial();
	if (ISSET(tp->t_state, TS_BUSY)) {
		/* Stop transmitting at the next chunk. */
		ch->ch_tbc = 0;
		ch->ch_heldtbc = 0;
		if (!ISSET(tp->t_state, TS_TTSTOP))
			SET(tp->t_state, TS_FLUSH);
	}
	splx(s);
}

void
sbjcn_diag(arg)
	void *arg;
{
	struct sbjcn_channel *ch = arg;
	struct sbjcn_softc *sc = ch->ch_sc;
	int overflows, floods;
	int s;

	s = splserial();
	overflows = ch->ch_overflows;
	ch->ch_overflows = 0;
	floods = ch->ch_floods;
	ch->ch_floods = 0;
	ch->ch_errors = 0;
	splx(s);

	log(LOG_WARNING, "%s: channel %d: %d fifo overflow%s, %d ibuf flood%s\n",
	    sc->sc_dev.dv_xname, ch->ch_num,
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

integrate void
sbjcn_rxsoft(struct sbjcn_channel *ch, struct tty *tp)
{
	int (*rint)(int c, struct tty *tp) = tp->t_linesw->l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char sr;
	int code;
	int s;

	end = ch->ch_ebuf;
	get = ch->ch_rbget;
	scc = cc = sbjcn_rbuf_size - ch->ch_rbavail;

	if (cc == sbjcn_rbuf_size) {
		ch->ch_floods++;
		if (ch->ch_errors++ == 0)
			callout_reset(&ch->ch_diag_callout, 60 * hz,
			    sbjcn_diag, ch);
	}

	while (cc) {
		code = get[0];
		sr = get[1];
		if (ISSET(sr, 0xf0)) {
			if (ISSET(sr, 0x10)) {
				ch->ch_overflows++;
				if (ch->ch_errors++ == 0)
					callout_reset(&ch->ch_diag_callout,
					    60 * hz, sbjcn_diag, ch);
			}
			if (ISSET(sr, 0xc0))
				SET(code, TTY_FE);
			if (ISSET(sr, 0x20))
				SET(code, TTY_PE);
		}
		if ((*rint)(code, tp) == -1) {
			/*
			 * The line discipline's buffer is out of space.
			 */
			if (!ISSET(ch->ch_rx_flags, RX_TTY_BLOCKED)) {
				/*
				 * We're either not using flow control, or the
				 * line discipline didn't tell us to block for
				 * some reason.  Either way, we have no way to
				 * know when there's more space available, so
				 * just drop the rest of the data.
				 */
				get += cc << 1;
				if (get >= end)
					get -= sbjcn_rbuf_size << 1;
				cc = 0;
			} else {
				/*
				 * Don't schedule any more receive processing
				 * until the line discipline tells us there's
				 * space available (through comhwiflow()).
				 * Leave the rest of the data in the input
				 * buffer.
				 */
				SET(ch->ch_rx_flags, RX_TTY_OVERFLOWED);
			}
			break;
		}
		get += 2;
		if (get >= end)
			get = ch->ch_rbuf;
		cc--;
	}

	if (cc != scc) {
		ch->ch_rbget = get;
		s = splserial();
		cc = ch->ch_rbavail += scc - cc;
		/* Buffers should be ok again, release possible block. */
		if (cc >= ch->ch_r_lowat) {
			if (ISSET(ch->ch_rx_flags, RX_IBUF_OVERFLOWED)) {
				CLR(ch->ch_rx_flags, RX_IBUF_OVERFLOWED);
				SET(ch->ch_imr, 0x02);
			}
			if (ISSET(ch->ch_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(ch->ch_rx_flags, RX_IBUF_BLOCKED);
				sbjcn_dohwiflow(ch);
			}
		}
		splx(s);
	}
}

integrate void
sbjcn_txsoft(struct sbjcn_channel *ch, struct tty *tp)
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(ch->ch_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

integrate void
sbjcn_stsoft(struct sbjcn_channel *ch, struct tty *tp)
{
	u_char iports, delta;
	int s;

	s = splserial();
	iports = ch->ch_iports;
	delta = ch->ch_iports_delta;
	ch->ch_iports_delta = 0;
	splx(s);

	if (ISSET(delta, ch->ch_i_dcd)) {
		/*
		 * Inform the tty layer that carrier detect changed.
		 */
		(void) (*tp->t_linesw->l_modem)(tp,
		    ISSET(iports, ch->ch_i_dcd));
	}

	if (ISSET(delta, ch->ch_i_cts)) {
		/* Block or unblock output according to flow control. */
		if (ISSET(iports, ch->ch_i_cts)) {
			ch->ch_tx_stopped = 0;
			(*tp->t_linesw->l_start)(tp);
		} else {
			ch->ch_tx_stopped = 1;
		}
	}

#ifdef SBJCN_DEBUG
	if (sbjcn_debug)
		sbjcn_status(ch, "sbjcn_stsoft");
#endif
}

integrate void
sbjcn_recv(struct sbjcn_channel *ch)
{
	u_char *put, *end;
	u_int cc;

	end = ch->ch_ebuf;
	put = ch->ch_rbput;
	cc = ch->ch_rbavail;

	/* XXX process break */

	sbjcncn_grabdword(ch);
	if (ch->ch_waiting_input) {
		if (!ISSET(ch->ch_rx_flags, RX_IBUF_OVERFLOWED)) {
			while (cc > 0) {
				put[0] = sbjcncn_nextbyte(ch);
				put[1] = 1; /* XXXKW ? */
				put += 2;
				if (put >= end)
					put = ch->ch_rbuf;
				cc--;

				if (!ch->ch_waiting_input)
					break;
			}

			/*
			 * Current string of incoming characters ended
			 * because no more data was available or we
			 * ran out of space.  Schedule a receive event
			 * if any data was received.  If we're out of
			 * space, turn off receive interrupts.
			 */
			ch->ch_rbput = put;
			ch->ch_rbavail = cc;
			if (!ISSET(ch->ch_rx_flags, RX_TTY_OVERFLOWED))
				ch->ch_rx_ready = 1;

			/*
			 * See if we are in danger of overflowing a
			 * buffer. If so, use hardware flow control
			 * to ease the pressure.
			 */
			if (!ISSET(ch->ch_rx_flags, RX_IBUF_BLOCKED) &&
			    cc < ch->ch_r_hiwat) {
				SET(ch->ch_rx_flags, RX_IBUF_BLOCKED);
				sbjcn_dohwiflow(ch);
			}

			/*
			 * If we're out of space, disable receive
			 * interrupts until the queue has drained
			 * a bit.
			 */
			if (!cc) {
				SET(ch->ch_rx_flags,
				RX_IBUF_OVERFLOWED);
				CLR(ch->ch_imr, 0x02);
			}
		} else {
			/* XXX panic? */
			CLR(ch->ch_imr, 0x02);
			//			continue;
		}
	}

	/*
	 * If we've delayed a parameter change, do it now, and restart
	 * output.
	 */
	if (ch->ch_heldchange) {
		sbjcn_loadchannelregs(ch);
		ch->ch_heldchange = 0;
		ch->ch_tbc = ch->ch_heldtbc;
		ch->ch_heldtbc = 0;
	}
}

void
sbjcn_callout(void *arg)
{
	struct sbjcn_channel *ch = arg;
	struct tty *tp = ch->ch_tty;

	/* XXX get stuff */
	sbjcn_recv(ch);

	/* XXX check receive */
	if (ch->ch_rx_ready) {
		ch->ch_rx_ready = 0;
		sbjcn_rxsoft(ch, tp);
	}

	/* XXX check transmit */
	if (ch->ch_tx_done) {
		ch->ch_tx_done = 0;
		sbjcn_txsoft(ch, tp);
	}

	callout_reset(&ch->ch_callout, hz/10, sbjcn_callout, ch);
}

static char sbjcncn_nextbyte(struct sbjcn_channel *ch)
{
	char c;

	sbjcncn_grabdword(ch);
	c = (ch->ch_input_buf >> 56) & 0xff;
	ch->ch_input_buf <<= 8;
	ch->ch_waiting_input--;
	return c;
}

static void sbjcncn_grabdword(struct sbjcn_channel *ch)
{
	uint64_t inbuf;

	if (ch->ch_waiting_input)
		return;

	inbuf = READ_REG(ch->ch_input_reg);
	ch->ch_waiting_input = jtag_input_len(inbuf);
	ch->ch_input_buf = inbuf << 8;
}

/*
 * Initialize UART for use as console or KGDB line.
 */
int
sbjcn_init(u_long addr, int chan, int rate, tcflag_t cflag)
{
	/* XXXKW Anything to do here? */
	return (0);
}

/*
 * Following are all routines needed for sbjcn to act as console
 */
int
sbjcn_cnattach(u_long addr, int chan, int rate, tcflag_t cflag)
{
	int res;
	uint64_t ctrl_val;
	static struct consdev sbjcn_cons = {
		NULL, NULL, sbjcn_cngetc, sbjcn_cnputc, sbjcn_cnpollc, NULL,
		    NODEV, CN_NORMAL
	};

	res = sbjcn_init(addr, chan, rate, cflag);
	if (res)
		return (res);

	cn_tab = &sbjcn_cons;

	sbjcn_cons_present = 1;
	sbjcn_cons_addr = addr;
	sbjcn_cons_waiting_input = 0;
	sbjcn_cons_chan = chan;
	sbjcn_cons_rate = rate;
	sbjcn_cons_cflag = cflag;

	/* Wait for sign of life from the other end */
	while ((ctrl_val = READ_REG(MIPS_PHYS_TO_KSEG1(sbjcn_cons_addr + JTAG_CONS_CONTROL))) == 0)
		;

	return (ctrl_val != JTAG_CONS_MAGICNUM);
}

int
sbjcn_cngetc(dev_t dev)
{
	char c;

	while (sbjcn_cons_waiting_input == 0)
		sbjcn_cngrabdword();

	c = (sbjcn_cons_input_buf >> 56) & 0xff;
	sbjcn_cons_input_buf <<= 8;
	sbjcn_cons_waiting_input--;

	return c;
}

/*
 * Console kernel output character routine.
 */
void
sbjcn_cnputc(dev_t dev, int c)
{
	uint64_t outbuf;

	outbuf = (1LL << 56) | (((uint64_t)c) << 48);
	WRITE_REG(MIPS_PHYS_TO_KSEG1(sbjcn_cons_addr + JTAG_CONS_OUTPUT), outbuf);
}

void
sbjcn_cnpollc(dev_t dev, int on)
{

}

static void sbjcn_cngrabdword(void)
{
	uint64_t inbuf;

	if (sbjcn_cons_waiting_input)
		return;

	inbuf = READ_REG(MIPS_PHYS_TO_KSEG1(sbjcn_cons_addr + JTAG_CONS_INPUT));
	sbjcn_cons_waiting_input = jtag_input_len(inbuf);
	sbjcn_cons_input_buf = inbuf << 8;
}

#ifdef KGDB
int
sbjcn_kgdb_attach(u_long addr, int chan, int rate, tcflag_t cflag)
{
	int res;

	if (!sbjcn_cons_present &&
	    sbjcn_cons_addr == addr && sbjcn_cons_chan == chan)
		return (EBUSY); /* cannot share with console */

	res = sbjcn_init(addr, chan, rate, cflag);
	if (res)
		return (res);

	kgdb_attach(sbjcn_kgdb_getc, sbjcn_kgdb_putc, NULL);
	kgdb_dev = 123; /* unneeded, only to satisfy some tests */
	kgdb_rate = rate;

	sbjcn_kgdb_present = 1;
	/* XXX sbjcn_init wants addr, but we need the offset addr */
	sbjcn_kgdb_addr = addr + (chan * 0x100);
	sbjcn_kgdb_chan = chan;

	return (0);
}

/* ARGSUSED */
int
sbjcn_kgdb_getc(arg)
	void *arg;
{

	return (sbjcn_common_getc(sbjcn_kgdb_addr, sbjcn_kgdb_chan));
}

/* ARGSUSED */
void
sbjcn_kgdb_putc(arg, c)
	void *arg;
	int c;
{

	sbjcn_common_putc(sbjcn_kgdb_addr, sbjcn_kgdb_chan, c);
}
#endif /* KGDB */

/*
 * helper function to identify the sbjcn channels used by
 * console or KGDB (and not yet autoconf attached)
 */
int
sbjcn_is_console(u_long addr, int chan)
{

	if (sbjcn_cons_present && !sbjcn_cons_attached &&
	    sbjcn_cons_addr == addr && sbjcn_cons_chan == chan)
		return (1);
#ifdef KGDB
	if (sbjcn_kgdb_present && !sbjcn_kgdb_attached &&
	    sbjcn_kgdb_addr == addr && sbjcn_kgdb_chan == chan)
		return (1);
#endif
	return (0);
}
