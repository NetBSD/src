/* $NetBSD: sbscn.c,v 1.21.2.1 2007/07/15 13:16:28 ad Exp $ */

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
 * `sbscn' driver, supports SiByte SB-1250 SOC DUART.
 *
 * This DUART is quite similar in programming model to the scn2681/68681
 * DUARTs supported by the NetBSD/amiga `mfc' and NetBSD/pc532 `scn'
 * driver, but substantial rework would have been necessary to make
 * those drivers sane w.r.t. bus_space (which would then have been
 * required on NetBSD/sbmips very early on), and to accommodate the
 * different register mappings.
 *
 * So, another driver.  Eventually there should be One True Driver,
 * but we're not here to save the world.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbscn.c,v 1.21.2.1 2007/07/15 13:16:28 ad Exp $");

#define	SBSCN_DEBUG

#include "opt_ddb.h"

#include "rnd.h"
#if NRND > 0 && defined(RND_SBSCN)
#include <sys/rnd.h>
#endif

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

#include <mips/sibyte/dev/sbobiovar.h>
#if 0
#include <mips/sibyte/dev/sbscnreg.h>
#endif
#include <mips/sibyte/dev/sbscnvar.h>
#include <dev/cons.h>
#include <machine/locore.h>

void	sbscn_attach_channel(struct sbscn_softc *sc, int chan, int intr);
#if defined(DDB) || defined(KGDB)
static void sbscn_enable_debugport(struct sbscn_channel *ch);
#endif
void	sbscn_config(struct sbscn_channel *ch);
void	sbscn_shutdown(struct sbscn_channel *ch);
int	sbscn_speed(long, long *);
static int cflag2modes(tcflag_t, u_char *, u_char *);
int	sbscn_param(struct tty *, struct termios *);
void	sbscn_start(struct tty *);
int	sbscn_hwiflow(struct tty *, int);

void	sbscn_loadchannelregs(struct sbscn_channel *);
void	sbscn_dohwiflow(struct sbscn_channel *);
void	sbscn_break(struct sbscn_channel *, int);
void	sbscn_modem(struct sbscn_channel *, int);
void	tiocm_to_sbscn(struct sbscn_channel *, int, int);
int	sbscn_to_tiocm(struct sbscn_channel *);
void	sbscn_iflush(struct sbscn_channel *);

int	sbscn_init(u_long addr, int chan, int rate, tcflag_t cflag);
int	sbscn_common_getc(u_long addr, int chan);
void	sbscn_common_putc(u_long addr, int chan, int c);
void	sbscn_intr(void *arg, uint32_t status, uint32_t pc);

int	sbscn_cngetc(dev_t dev);
void	sbscn_cnputc(dev_t dev, int c);
void	sbscn_cnpollc(dev_t dev, int on);

extern struct cfdriver sbscn_cd;

dev_type_open(sbscnopen);
dev_type_close(sbscnclose);
dev_type_read(sbscnread);
dev_type_write(sbscnwrite);
dev_type_ioctl(sbscnioctl);
dev_type_stop(sbscnstop);
dev_type_tty(sbscntty);
dev_type_poll(sbscnpoll);

const struct cdevsw sbscn_cdevsw = {
	sbscnopen, sbscnclose, sbscnread, sbscnwrite, sbscnioctl,
	sbscnstop, sbscntty, sbscnpoll, nommap, ttykqfilter, D_TTY
};

#define	integrate	static inline
void 	sbscn_soft(void *);
integrate void sbscn_rxsoft(struct sbscn_channel *, struct tty *);
integrate void sbscn_txsoft(struct sbscn_channel *, struct tty *);
integrate void sbscn_stsoft(struct sbscn_channel *, struct tty *);
integrate void sbscn_schedrx(struct sbscn_channel *);
void	sbscn_diag(void *);

/*
 * Make this an option variable one can patch.
 * But be warned:  this must be a power of 2!
 */
u_int sbscn_rbuf_size = SBSCN_RING_SIZE;

/* Stop input when 3/4 of the ring is full; restart when only 1/4 is full. */
u_int sbscn_rbuf_hiwat = (SBSCN_RING_SIZE * 1) / 4;
u_int sbscn_rbuf_lowat = (SBSCN_RING_SIZE * 3) / 4;

static int	sbscn_cons_present;
static int	sbscn_cons_attached;
static u_long	sbscn_cons_addr;
static int	sbscn_cons_chan;
static int	sbscn_cons_rate;
static tcflag_t	sbscn_cons_cflag;

#ifdef KGDB
#include <sys/kgdb.h>

static int	sbscn_kgdb_present;
static int	sbscn_kgdb_attached;
static u_long	sbscn_kgdb_addr;
static int	sbscn_kgdb_chan;

int	sbscn_kgdb_getc(void *);
void	sbscn_kgdb_putc(void *, int);
#endif /* KGDB */

static int	sbscn_match(struct device *, struct cfdata *, void *);
static void	sbscn_attach(struct device *, struct device *, void *);

CFATTACH_DECL(sbscn, sizeof(struct sbscn_softc),
    sbscn_match, sbscn_attach, NULL, NULL);

#define	READ_REG(rp)		(mips3_ld((int64_t *)__UNVOLATILE(rp)))
#define	WRITE_REG(rp, val)	(mips3_sd((uint64_t *)__UNVOLATILE(rp), (val)))

/*
 * input and output signals are actually the _inverse_ of the bits in the
 * input and output port registers!
 */
#define	GET_INPUT_SIGNALS(ch) \
    ((~READ_REG(MIPS_PHYS_TO_KSEG1((ch)->ch_sc->sc_addr + 0x280))) & (ch)->ch_i_mask)
#define	SET_OUTPUT_SIGNALS(ch, val) 					\
    do {								\
	int sigs_to_set, sigs_to_clr;					\
									\
	sigs_to_set = (ch)->ch_o_mask & val;				\
	sigs_to_clr = (ch)->ch_o_mask & ~val;				\
									\
	/* set signals by clearing op bits, and vice versa */		\
	WRITE_REG(MIPS_PHYS_TO_KSEG1((ch)->ch_sc->sc_addr + 0x2c0),	\
	    sigs_to_set);						\
	WRITE_REG(MIPS_PHYS_TO_KSEG1((ch)->ch_sc->sc_addr + 0x2b0),	\
	    sigs_to_clr);						\
    } while (0)

static int
sbscn_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct sbobio_attach_args *sap = aux;

	if (sap->sa_locs.sa_type != SBOBIO_DEVTYPE_DUART)
		return (0);

	return 1;
}

static void
sbscn_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbscn_softc *sc = (struct sbscn_softc *)self;
	struct sbobio_attach_args *sap = aux;
	int i;

	sc->sc_addr = sap->sa_base + sap->sa_locs.sa_offset;

	printf("\n");
	for (i = 0; i < 2; i++)
		sbscn_attach_channel(sc, i, sap->sa_locs.sa_intr[i]);

	/* init duart_opcr */
	WRITE_REG(MIPS_PHYS_TO_KSEG1(sc->sc_addr + 0x270), 0);
	/* init duart_aux_ctrl */
	WRITE_REG(MIPS_PHYS_TO_KSEG1(sc->sc_addr + 0x210), 0x0f); /* XXX */
}

void
sbscn_attach_channel(struct sbscn_softc *sc, int chan, int intr)
{
	struct sbscn_channel *ch = &sc->sc_channels[chan];
	u_long chan_addr;
	struct tty *tp;

	ch->ch_sc = sc;
	ch->ch_num = chan;

	chan_addr = sc->sc_addr + (0x100 * chan);
	ch->ch_base = (void *)MIPS_PHYS_TO_KSEG1(chan_addr);
	ch->ch_isr_base =
	    (void *)MIPS_PHYS_TO_KSEG1(sc->sc_addr + 0x220 + (0x20 * chan));
	ch->ch_imr_base =
	    (void *)MIPS_PHYS_TO_KSEG1(sc->sc_addr + 0x230 + (0x20 * chan));
#ifdef XXXCGDnotyet
	ch->ch_inchg_base =
	    (void *)MIPS_PHYS_TO_KSEG1(sc->sc_addr + 0x2d0 + (0x10 * chan));
#endif

	ch->ch_i_dcd = ch->ch_i_dcd_pin = 0 /* XXXCGD */;
	ch->ch_i_cts = ch->ch_i_cts_pin = 0 /* XXXCGD */;
	ch->ch_i_dsr = ch->ch_i_dsr_pin = 0 /* XXXCGD */;
	ch->ch_i_ri = ch->ch_i_ri_pin = 0 /* XXXCGD */;
	ch->ch_i_mask =
	    ch->ch_i_dcd | ch->ch_i_cts | ch->ch_i_dsr | ch->ch_i_ri;
	ch->ch_o_dtr = ch->ch_o_dtr_pin = 0 /* XXXCGD */;
	ch->ch_o_rts = ch->ch_o_rts_pin = 0 /* XXXCGD */;
	ch->ch_o_mask = ch->ch_o_dtr | ch->ch_o_rts;

	ch->ch_intrhand = cpu_intr_establish(intr, IPL_SERIAL, sbscn_intr, ch);
	callout_init(&ch->ch_diag_callout, 0);

	/* Disable interrupts before configuring the device. */
	ch->ch_imr = 0;
	WRITE_REG(ch->ch_imr_base, ch->ch_imr);

	if (sbscn_cons_present &&
	    sbscn_cons_addr == chan_addr && sbscn_cons_chan == chan) {
		sbscn_cons_attached = 1;

		/* Make sure the console is always "hardwired". */
		delay(1000);			/* wait for output to finish */
		SET(ch->ch_hwflags, SBSCN_HW_CONSOLE);
		SET(ch->ch_swflags, TIOCFLAG_SOFTCAR);
	}

	tp = ttymalloc();
	tp->t_oproc = sbscn_start;
	tp->t_param = sbscn_param;
	tp->t_hwiflow = sbscn_hwiflow;

	ch->ch_tty = tp;
	ch->ch_rbuf = malloc(sbscn_rbuf_size << 1, M_DEVBUF, M_NOWAIT);
	if (ch->ch_rbuf == NULL) {
		printf("%s: channel %d: unable to allocate ring buffer\n",
		    sc->sc_dev.dv_xname, chan);
		return;
	}
	ch->ch_ebuf = ch->ch_rbuf + (sbscn_rbuf_size << 1);

	tty_attach(tp);

	if (ISSET(ch->ch_hwflags, SBSCN_HW_CONSOLE)) {
		int maj;

		/* locate the major number */
		maj = cdevsw_lookup_major(&sbscn_cdevsw);

		cn_tab->cn_dev = makedev(maj,
		    (device_unit(&sc->sc_dev) << 1) + chan);

		printf("%s: channel %d: console\n", sc->sc_dev.dv_xname, chan);
	}

#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * the kgdb device, it has exclusive use.
	 */
	if (sbscn_kgdb_present &&
	    sbscn_kgdb_addr == chan_addr && sbscn_kgdb_chan == chan) {
		sbscn_kgdb_attached = 1;

		SET(sc->sc_hwflags, SBSCN_HW_KGDB);
		printf("%s: channel %d: kgdb\n", sc->sc_dev.dv_xname, chan);
	}
#endif

	ch->ch_si = softintr_establish(IPL_SOFTSERIAL, sbscn_soft, ch);

#if NRND > 0 && defined(RND_SBSCN)
	rnd_attach_source(&ch->ch_rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_TTY, 0);
#endif

	sbscn_config(ch);

	SET(ch->ch_hwflags, SBSCN_HW_DEV_OK);
}

int
sbscn_speed(long speed, long *brcp)
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
	if (err > SBSCN_TOLERANCE)
		return (-1);
	*brcp = x - 1;
	return (0);

#undef	divrnd
}

#ifdef SBSCN_DEBUG
void	sbscn_status(struct sbscn_channel *, const char *);

int	sbscn_debug = 0 /* XXXCGD */;

void
sbscn_status(struct sbscn_channel *ch, const char *str)
{
	struct sbscn_softc *sc = ch->ch_sc;
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
sbscn_enable_debugport(struct sbscn_channel *ch)
{
	int s;

	/* Turn on line break interrupt, set carrier. */
	s = splserial();

#if 0	/* DO NOT turn on break interrupt at this time. */
	ch->ch_imr = 0x04;
#else
	ch->ch_imr = 0x00;
#endif
	WRITE_REG(ch->ch_imr_base, ch->ch_imr);
	SET(ch->ch_oports, ch->ch_o_dtr | ch->ch_o_rts);
	SET_OUTPUT_SIGNALS(ch, ch->ch_oports);

	splx(s);
}
#endif

void
sbscn_config(struct sbscn_channel *ch)
{

	/* Disable interrupts before configuring the device. */
	ch->ch_imr = 0x00;
	WRITE_REG(ch->ch_imr_base, ch->ch_imr);

#ifdef DDB
	if (ISSET(ch->ch_hwflags, SBSCN_HW_CONSOLE))
		sbscn_enable_debugport(ch);
#endif

#ifdef KGDB
	/*
	 * Allow kgdb to "take over" this port.  If this is
	 * the kgdb device, it has exclusive use.
	 */
	if (ISSET(ch->ch_hwflags, SBSCN_HW_KGDB))
		sbscn_enable_debugport(ch);
#endif
}

void
sbscn_shutdown(struct sbscn_channel *ch)
{
	struct tty *tp = ch->ch_tty;
	int s;

	s = splserial();

	/* If we were asserting flow control, then deassert it. */
	SET(ch->ch_rx_flags, RX_IBUF_BLOCKED);
	sbscn_dohwiflow(ch);

	/* Clear any break condition set with TIOCSBRK. */
	sbscn_break(ch, 0);

	/*
	 * Hang up if necessary.  Wait a bit, so the other side has time to
	 * notice even if we immediately open the port again.
	 * Avoid tsleeping above splhigh().
	 */
	if (ISSET(tp->t_cflag, HUPCL)) {
		sbscn_modem(ch, 0);
		splx(s);
		/* XXX tsleep will only timeout */
		(void) tsleep(ch, TTIPRI, ttclos, hz);
		s = splserial();
	}

	/* Turn off interrupts. */
#ifdef DDB
	if (ISSET(ch->ch_hwflags, SBSCN_HW_CONSOLE))
#if 0	/* DO NOT turn on break interrupt at this time. */
		ch->ch_imr = 0x04; /* interrupt on break */
#else
		ch->ch_imr = 0x00;
#endif
	else
#endif
		ch->ch_imr = 0;
	WRITE_REG(ch->ch_imr_base, ch->ch_imr);

	splx(s);
}

int
sbscnopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit = SBSCN_UNIT(dev);
	int chan = SBSCN_CHAN(dev);
	struct sbscn_softc *sc;
	struct sbscn_channel *ch;
	struct tty *tp;
	int s, s2;
	int error;

	if (unit >= sbscn_cd.cd_ndevs)
		return (ENXIO);
	sc = sbscn_cd.cd_devs[unit];
	if (sc == 0)
		return (ENXIO);
	ch = &sc->sc_channels[chan];
	if (!ISSET(ch->ch_hwflags, SBSCN_HW_DEV_OK) || ch->ch_rbuf == NULL)
		return (ENXIO);

#ifdef KGDB
	/*
	 * If this is the kgdb port, no other use is permitted.
	 */
	if (ISSET(ch->ch_hwflags, SBSCN_HW_KGDB))
		return (EBUSY);
#endif

	tp = ch->ch_tty;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
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
#if 0	/* DO NOT turn on break or status change interrupt at this time. */
		ch->ch_imr = 0xe;
#else
		ch->ch_imr = 0x2;
#endif
		WRITE_REG(ch->ch_imr_base, ch->ch_imr);

		/* Fetch the current modem control status, needed later. */
		ch->ch_iports = GET_INPUT_SIGNALS(ch);
		ch->ch_iports_delta = 0;
		splx(s2);

		/*
		 * Initialize the termios status to the defaults.  Add in the
		 * sticky bits from TIOCSFLAGS.
		 */
		t.c_ispeed = 0;
		if (ISSET(ch->ch_hwflags, SBSCN_HW_CONSOLE)) {
			t.c_ospeed = sbscn_cons_rate;
			t.c_cflag = sbscn_cons_cflag;
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
		/* Make sure sbscn_param() will do something. */
		tp->t_ospeed = 0;
		(void) sbscn_param(tp, &t);
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
		sbscn_modem(ch, 1);

		/* Clear the input ring, and unblock. */
		ch->ch_rbput = ch->ch_rbget = ch->ch_rbuf;
		ch->ch_rbavail = sbscn_rbuf_size;
		sbscn_iflush(ch);
		CLR(ch->ch_rx_flags, RX_ANY_BLOCK);
		sbscn_dohwiflow(ch);

#ifdef SBSCN_DEBUG
		if (sbscn_debug)
			sbscn_status(ch, "sbscnopen  ");
#endif

		splx(s2);
	}

	splx(s);

	error = ttyopen(tp, SBSCN_DIALOUT(dev), ISSET(flag, O_NONBLOCK));
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
		sbscn_shutdown(ch);
	}

	return (error);
}

int
sbscnclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct sbscn_softc *sc = sbscn_cd.cd_devs[SBSCN_UNIT(dev)];
	struct sbscn_channel *ch = &sc->sc_channels[SBSCN_CHAN(dev)];
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
		sbscn_shutdown(ch);
	}

	return (0);
}

int
sbscnread(dev_t dev, struct uio *uio, int flag)
{
	struct sbscn_softc *sc = sbscn_cd.cd_devs[SBSCN_UNIT(dev)];
	struct sbscn_channel *ch = &sc->sc_channels[SBSCN_CHAN(dev)];
	struct tty *tp = ch->ch_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
sbscnwrite(dev_t dev, struct uio *uio, int flag)
{
	struct sbscn_softc *sc = sbscn_cd.cd_devs[SBSCN_UNIT(dev)];
	struct sbscn_channel *ch = &sc->sc_channels[SBSCN_CHAN(dev)];
	struct tty *tp = ch->ch_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
sbscnpoll(dev_t dev, int events, struct lwp *l)
{
	struct sbscn_softc *sc = sbscn_cd.cd_devs[SBSCN_UNIT(dev)];
	struct sbscn_channel *ch = &sc->sc_channels[SBSCN_CHAN(dev)];
	struct tty *tp = ch->ch_tty;

	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
sbscntty(dev_t dev)
{
	struct sbscn_softc *sc = sbscn_cd.cd_devs[SBSCN_UNIT(dev)];
	struct sbscn_channel *ch = &sc->sc_channels[SBSCN_CHAN(dev)];
	struct tty *tp = ch->ch_tty;

	return (tp);
}

int
sbscnioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct sbscn_softc *sc = sbscn_cd.cd_devs[SBSCN_UNIT(dev)];
	struct sbscn_channel *ch = &sc->sc_channels[SBSCN_CHAN(dev)];
	struct tty *tp = ch->ch_tty;
	int error;
	int s;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = 0;

	s = splserial();

	switch (cmd) {
	case TIOCSBRK:
		sbscn_break(ch, 1);
		break;

	case TIOCCBRK:
		sbscn_break(ch, 0);
		break;

	case TIOCSDTR:
		sbscn_modem(ch, 1);
		break;

	case TIOCCDTR:
		sbscn_modem(ch, 0);
		break;

	case TIOCGFLAGS:
		*(int *)data = ch->ch_swflags;
		break;

	case TIOCSFLAGS:
		error = kauth_authorize_device_tty(l->l_cred,
		    KAUTH_DEVICE_TTY_PRIVSET, tp);
		if (error)
			break;
		ch->ch_swflags = *(int *)data;
		break;

	case TIOCMSET:
	case TIOCMBIS:
	case TIOCMBIC:
		tiocm_to_sbscn(ch, cmd, *(int *)data);
		break;

	case TIOCMGET:
		*(int *)data = sbscn_to_tiocm(ch);
		break;

	default:
		error = EPASSTHROUGH;
		break;
	}

	splx(s);

#ifdef SBSCN_DEBUG
	if (sbscn_debug)
		sbscn_status(ch, "sbscn_ioctl ");
#endif

	return (error);
}

integrate void
sbscn_schedrx(struct sbscn_channel *ch)
{

	ch->ch_rx_ready = 1;

	/* Wake up the poller. */
	softintr_schedule(ch->ch_si);
}

void
sbscn_break(struct sbscn_channel *ch, int onoff)
{

/* XXXCGD delay break until not busy */
	if (onoff)
		WRITE_REG(ch->ch_base + 0x50, 0x60);
	else
		WRITE_REG(ch->ch_base + 0x50, 0x70);

#if 0
	if (!ch->ch_heldchange) {
		if (ch->ch_tx_busy) {
			ch->ch_heldtbc = ch->ch_tbc;
			ch->ch_tbc = 0;
			ch->ch_heldchange = 1;
		} else
			sbscn_loadchannelregs(ch);
	}
#endif
}

void
sbscn_modem(struct sbscn_channel *ch, int onoff)
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
			sbscn_loadchannelregs(ch);
	}
}

void
tiocm_to_sbscn(struct sbscn_channel *ch, int how, int ttybits)
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
			sbscn_loadchannelregs(ch);
	}
}

int
sbscn_to_tiocm(struct sbscn_channel *ch)
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
sbscn_param(struct tty *tp, struct termios *t)
{
	struct sbscn_softc *sc = sbscn_cd.cd_devs[SBSCN_UNIT(tp->t_dev)];
	struct sbscn_channel *ch = &sc->sc_channels[SBSCN_CHAN(tp->t_dev)];
	long brc;
	u_char mode1, mode2;
	int s;

/* XXX reset to console parameters if console? */
#if 0
XXX disable, enable.
#endif

	/* Check requested parameters. */
	if (sbscn_speed(t->c_ospeed, &brc) < 0)
		return (EINVAL);
	if (t->c_ispeed && t->c_ispeed != t->c_ospeed)
		return (EINVAL);

	/*
	 * For the console, always force CLOCAL and !HUPCL, so that the port
	 * is always active.
	 */
	if (ISSET(ch->ch_swflags, TIOCFLAG_SOFTCAR) ||
	    ISSET(ch->ch_hwflags, SBSCN_HW_CONSOLE)) {
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
			sbscn_loadchannelregs(ch);
	}

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		/* Disable the high water mark. */
		ch->ch_r_hiwat = 0;
		ch->ch_r_lowat = 0;
		if (ISSET(ch->ch_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(ch->ch_rx_flags, RX_TTY_OVERFLOWED);
			sbscn_schedrx(ch);
		}
		if (ISSET(ch->ch_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED)) {
			CLR(ch->ch_rx_flags, RX_TTY_BLOCKED|RX_IBUF_BLOCKED);
			sbscn_dohwiflow(ch);
		}
	} else {
		ch->ch_r_hiwat = sbscn_rbuf_hiwat;
		ch->ch_r_lowat = sbscn_rbuf_lowat;
	}

	splx(s);

	/*
	 * Update the tty layer's idea of the carrier bit, in case we changed
	 * CLOCAL or MDMBUF.  We don't hang up here; we only do that by
	 * explicit request.
	 */
	(void) (*tp->t_linesw->l_modem)(tp,
	    ISSET(ch->ch_iports, ch->ch_i_dcd));

#ifdef SBSCN_DEBUG
	if (sbscn_debug)
		sbscn_status(ch, "sbscnparam ");
#endif

	if (!ISSET(t->c_cflag, CHWFLOW)) {
		if (ch->ch_tx_stopped) {
			ch->ch_tx_stopped = 0;
			sbscn_start(tp);
		}
	}

	return (0);
}

void
sbscn_iflush(struct sbscn_channel *ch)
{
#ifdef DIAGNOSTIC
	int reg;
#endif
	int timo;

#ifdef DIAGNOSTIC
	reg = 0xffff;
#endif
	timo = 50000;
	/* flush any pending I/O */
	while (ISSET(READ_REG(ch->ch_base + 0x20), 0x01)
	    && --timo)
#ifdef DIAGNOSTIC
		reg =
#else
		    (void)
#endif
		    READ_REG(ch->ch_base + 0x60);
#ifdef DIAGNOSTIC
	if (!timo)
		printf("%s: sbscn_iflush timeout %02x\n",
		    ch->ch_sc->sc_dev.dv_xname, reg & 0xff);
#endif
}

void
sbscn_loadchannelregs(struct sbscn_channel *ch)
{

	/* XXXXX necessary? */
	sbscn_iflush(ch);

	WRITE_REG(ch->ch_imr_base, 0);

// XXX disable?
	WRITE_REG(ch->ch_base + 0x00, ch->ch_mode1);
	WRITE_REG(ch->ch_base + 0x10, ch->ch_mode2);
	WRITE_REG(ch->ch_base + 0x30, ch->ch_brc);

	ch->ch_oports_active = ch->ch_oports;
	SET_OUTPUT_SIGNALS(ch, ch->ch_oports_active);

	WRITE_REG(ch->ch_imr_base, ch->ch_imr);
}

int
sbscn_hwiflow(struct tty *tp, int block)
{
	struct sbscn_softc *sc = sbscn_cd.cd_devs[SBSCN_UNIT(tp->t_dev)];
	struct sbscn_channel *ch = &sc->sc_channels[SBSCN_CHAN(tp->t_dev)];
	int s;

	if (ch->ch_o_rts == 0)
		return (0);

	s = splserial();
	if (block) {
		if (!ISSET(ch->ch_rx_flags, RX_TTY_BLOCKED)) {
			SET(ch->ch_rx_flags, RX_TTY_BLOCKED);
			sbscn_dohwiflow(ch);
		}
	} else {
		if (ISSET(ch->ch_rx_flags, RX_TTY_OVERFLOWED)) {
			CLR(ch->ch_rx_flags, RX_TTY_OVERFLOWED);
			sbscn_schedrx(ch);
		}
		if (ISSET(ch->ch_rx_flags, RX_TTY_BLOCKED)) {
			CLR(ch->ch_rx_flags, RX_TTY_BLOCKED);
			sbscn_dohwiflow(ch);
		}
	}
	splx(s);
	return (1);
}

/*
 * (un)block input via hw flowcontrol
 */
void
sbscn_dohwiflow(struct sbscn_channel *ch)
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
	SET_OUTPUT_SIGNALS(ch, ch->ch_oports_active);
}

void
sbscn_start(struct tty *tp)
{
	struct sbscn_softc *sc = sbscn_cd.cd_devs[SBSCN_UNIT(tp->t_dev)];
	struct sbscn_channel *ch = &sc->sc_channels[SBSCN_CHAN(tp->t_dev)];
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

	/* Enable transmit completion interrupts if necessary. */
	if (!ISSET(ch->ch_imr, 0x1)) {
		SET(ch->ch_imr, 0x1);
		WRITE_REG(ch->ch_imr_base, ch->ch_imr);
	}

	/* Output the first chunk of the contiguous buffer. */
	{
		u_char c;

		while (ch->ch_tbc && READ_REG(ch->ch_base + 0x20) & 0x04) {
			c = *ch->ch_tba++;
			ch->ch_tbc--;
			WRITE_REG(ch->ch_base + 0x70, c);
		}
	}
out:
	splx(s);
	return;
}

/*
 * Stop output on a line.
 */
void
sbscnstop(struct tty *tp, int flag)
{
	struct sbscn_softc *sc = sbscn_cd.cd_devs[SBSCN_UNIT(tp->t_dev)];
	struct sbscn_channel *ch = &sc->sc_channels[SBSCN_CHAN(tp->t_dev)];
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
sbscn_diag(arg)
	void *arg;
{
	struct sbscn_channel *ch = arg;
	struct sbscn_softc *sc = ch->ch_sc;
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
sbscn_rxsoft(struct sbscn_channel *ch, struct tty *tp)
{
	int (*rint)(int, struct tty *) = tp->t_linesw->l_rint;
	u_char *get, *end;
	u_int cc, scc;
	u_char sr;
	int code;
	int s;

	end = ch->ch_ebuf;
	get = ch->ch_rbget;
	scc = cc = sbscn_rbuf_size - ch->ch_rbavail;

	if (cc == sbscn_rbuf_size) {
		ch->ch_floods++;
		if (ch->ch_errors++ == 0)
			callout_reset(&ch->ch_diag_callout, 60 * hz,
			    sbscn_diag, ch);
	}

	while (cc) {
		code = get[0];
		sr = get[1];
		if (ISSET(sr, 0xf0)) {
			if (ISSET(sr, 0x10)) {
				ch->ch_overflows++;
				if (ch->ch_errors++ == 0)
					callout_reset(&ch->ch_diag_callout,
					    60 * hz, sbscn_diag, ch);
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
					get -= sbscn_rbuf_size << 1;
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
				WRITE_REG(ch->ch_imr_base, ch->ch_imr);
			}
			if (ISSET(ch->ch_rx_flags, RX_IBUF_BLOCKED)) {
				CLR(ch->ch_rx_flags, RX_IBUF_BLOCKED);
				sbscn_dohwiflow(ch);
			}
		}
		splx(s);
	}
}

integrate void
sbscn_txsoft(struct sbscn_channel *ch, struct tty *tp)
{

	CLR(tp->t_state, TS_BUSY);
	if (ISSET(tp->t_state, TS_FLUSH))
		CLR(tp->t_state, TS_FLUSH);
	else
		ndflush(&tp->t_outq, (int)(ch->ch_tba - tp->t_outq.c_cf));
	(*tp->t_linesw->l_start)(tp);
}

integrate void
sbscn_stsoft(struct sbscn_channel *ch, struct tty *tp)
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

#ifdef SBSCN_DEBUG
	if (sbscn_debug)
		sbscn_status(ch, "sbscn_stsoft");
#endif
}

void
sbscn_soft(void *arg)
{
	struct sbscn_channel *ch = arg;
	struct tty *tp = ch->ch_tty;

	if (ch->ch_rx_ready) {
		ch->ch_rx_ready = 0;
		sbscn_rxsoft(ch, tp);
	}

	if (ch->ch_st_check) {
		ch->ch_st_check = 0;
		sbscn_stsoft(ch, tp);
	}

	if (ch->ch_tx_done) {
		ch->ch_tx_done = 0;
		sbscn_txsoft(ch, tp);
	}
}

void
sbscn_intr(void *arg, uint32_t status, uint32_t pc)
{
	struct sbscn_channel *ch = arg;
	u_char *put, *end;
	u_int cc;
	u_char isr, sr;

	/* read ISR */
	isr = READ_REG(ch->ch_isr_base) & ch->ch_imr;
	if (isr == 0)
		return;

	end = ch->ch_ebuf;
	put = ch->ch_rbput;
	cc = ch->ch_rbavail;

	do {
		u_char	iports, delta;

		if (isr & 0x02) {

			sr = READ_REG(ch->ch_base + 0x20);
			/* XXX sr 0x01 bit must be set at this point */

#if defined(DDB) || defined(KGDB)
			if ((sr & 0x80) == 0x80) {
#ifdef DDB
				if (ISSET(ch->ch_hwflags, SBSCN_HW_CONSOLE)) {
					(void)READ_REG(ch->ch_base + 0x60);
					console_debugger();
					continue;
				}
#endif
#ifdef KGDB
				if (ISSET(ch->ch_hwflags, SBSCN_HW_KGDB)) {
					(void)READ_REG(ch->ch_base + 0x60);
					kgdb_connect(1);
					continue;
				}
#endif
			}
#endif /* DDB || KGDB */

		    	if (!ISSET(ch->ch_rx_flags, RX_IBUF_OVERFLOWED)) {
				while (cc > 0) {
					put[0] = READ_REG(ch->ch_base + 0x60);
					put[1] = sr;
					put += 2;
					if (put >= end)
						put = ch->ch_rbuf;
					cc--;

					sr = READ_REG(ch->ch_base + 0x20);
					if (!ISSET(sr, 0x02))
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
					sbscn_dohwiflow(ch);
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
					WRITE_REG(ch->ch_imr_base, ch->ch_imr);
				}
			} else {
				/* XXX panic? */
				CLR(ch->ch_imr, 0x02);
				WRITE_REG(ch->ch_imr_base, ch->ch_imr);
				continue;
			}
		}

		if (isr & 0x01) {
			CLR(ch->ch_imr, 0x01);
			WRITE_REG(ch->ch_imr_base, ch->ch_imr);
		}

#if 0
XXX
		if (isr & 0x08) {
			clear input signal change!
		}
#endif
		iports = GET_INPUT_SIGNALS(ch);
		delta = iports ^ ch->ch_iports;
		ch->ch_iports = iports;

		/*
		 * Process normal status changes
		 */
		if (ISSET(delta, ch->ch_i_mask)) {
			SET(ch->ch_iports_delta, delta);

			/*
			 * Stop output immediately if we lose the output
			 * flow control signal or carrier detect.
			 */
			if (ISSET(~iports, ch->ch_i_mask)) {
				ch->ch_tbc = 0;
				ch->ch_heldtbc = 0;
#ifdef SBSCN_DEBUG
				if (sbscn_debug)
					sbscn_status(ch, "sbscn_intr  ");
#endif
			}

			ch->ch_st_check = 1;
		}
	} while ((isr = (READ_REG(ch->ch_isr_base) & ch->ch_imr)) != 0);

	/*
	 * Done handling any receive interrupts and status changes, and
	 * clearing the tx-ready interrupt if it was set.  See if data can
	 * be transmitted as well. Schedule tx done event if no data left
	 * and tty was marked busy.
	 */
	sr = READ_REG(ch->ch_base + 0x20);
	if (ISSET(sr, 0x4)) {

		/*
		 * If we've delayed a parameter change, do it now, and restart
		 * output.
		 */
		if (ch->ch_heldchange) {
			sbscn_loadchannelregs(ch);
			ch->ch_heldchange = 0;
			ch->ch_tbc = ch->ch_heldtbc;
			ch->ch_heldtbc = 0;
		}

		/* Output the next chunk of the contiguous buffer, if any. */
		if (ch->ch_tbc > 0) {
			int wrote1;
			u_char c;

			wrote1 = 0;
			while (ch->ch_tbc &&
			    READ_REG(ch->ch_base + 0x20) & 0x04) {
				wrote1 = 1;
				c = *ch->ch_tba++;
				ch->ch_tbc--;
				WRITE_REG(ch->ch_base + 0x70, c);
			}
			if (wrote1) {
				SET(ch->ch_imr, 0x01);
				WRITE_REG(ch->ch_imr_base, ch->ch_imr);
			}
		} else {
			/*
			 * transmit completion interrupts already disabled,
			 * mark the channel tx state as done.
			 */
			if (ch->ch_tx_busy) {
				ch->ch_tx_busy = 0;
				ch->ch_tx_done = 1;
			}
		}
	}

	/* Wake up the poller. */
	softintr_schedule(ch->ch_si);

#if NRND > 0 && defined(RND_SBSCN)
	rnd_add_uint32(&ch->ch_rnd_source, isr | sr);
#endif
}

/*
 * The following functions are polled getc and putc routines, shared
 * by the console and kgdb glue.
 */

int
sbscn_common_getc(u_long addr, int chan)
{
	int s = splhigh();
	u_long base = MIPS_PHYS_TO_KSEG1(addr + (chan * 0x100));
	int c;

	/* block until a character becomes available */
	while ((READ_REG(base + 0x20) & 0x01) == 0)
		continue;

	c = READ_REG(base + 0x60) & 0xff;
	splx(s);
	return (c);
}

void
sbscn_common_putc(u_long addr, int chan, int c)
{
	int s = splhigh();
	int timo;
	u_long base = MIPS_PHYS_TO_KSEG1(addr + (chan * 0x100));

	/* wait for any pending transmission to finish */
	timo = 1500000;
	while ((READ_REG(base + 0x20) & 0x08) == 0 && --timo)
		continue;

	WRITE_REG(base + 0x70, c);

	/* wait for this transmission to complete */
	timo = 15000000;
	while ((READ_REG(base + 0x20) & 0x08) == 0 && --timo)
		continue;

	splx(s);
}

/*
 * Initialize UART for use as console or KGDB line.
 */
int
sbscn_init(u_long addr, int chan, int rate, tcflag_t cflag)
{
#if 1
	u_long chanregbase = MIPS_PHYS_TO_KSEG1(addr + (chan * 0x100));
	u_long imaskreg = MIPS_PHYS_TO_KSEG1(addr + 0x230 + (chan * 0x20));
	u_char mode1, mode2;
	u_long brc;
	volatile int timo;

	WRITE_REG(imaskreg, 0);			/* disable channel intrs */

	/* XXX should we really do the following?  how about only if enabled? */
	/* wait for any pending transmission to finish */
	timo = 1500000;
	while ((READ_REG(chanregbase + 0x20) & 0x08) == 0 && --timo)
		continue;

	/* XXX: wait a little.  THIS SHOULD NOT BE NECESSARY!!! (?) */
	timo = 1500000;
	while (--timo)
		;

	WRITE_REG(chanregbase + 0x50, 2 << 4);	/* reset receiver */
	WRITE_REG(chanregbase + 0x50, 3 << 4);	/* reset transmitter */

	/* set up the line for use */
	(void)cflag2modes(cflag, &mode1, &mode2);
	(void)sbscn_speed(rate, &brc);
	WRITE_REG(chanregbase + 0x00, mode1);
	WRITE_REG(chanregbase + 0x10, mode2);
	WRITE_REG(chanregbase + 0x30, brc);

	/* enable transmit and receive */
#define	M_DUART_RX_EN			0x01
#define	M_DUART_TX_EN			0x04
	WRITE_REG(chanregbase + 0x50,M_DUART_RX_EN | M_DUART_TX_EN);
#endif

	/* XXX: wait a little.  THIS SHOULD NOT BE NECESSARY!!! (?) */
	timo = 1500000;
	while (--timo)
		;

#if 0 /* XXXCGD */
	bus_space_handle_t ioh;

	if (bus_space_map(iot, iobase, COM_NPORTS, 0, &ioh))
		return (ENOMEM); /* ??? */

	bus_space_write_1(iot, ioh, com_lcr, LCR_EERS);
	bus_space_write_1(iot, ioh, com_efr, 0);
	bus_space_write_1(iot, ioh, com_lcr, LCR_DLAB);
	rate = comspeed(rate, frequency);
	bus_space_write_1(iot, ioh, com_dlbl, rate);
	bus_space_write_1(iot, ioh, com_dlbh, rate >> 8);
	bus_space_write_1(iot, ioh, com_lcr, cflag2lcr(cflag));
	bus_space_write_1(iot, ioh, com_mcr, MCR_DTR | MCR_RTS);
	bus_space_write_1(iot, ioh, com_fifo,
	    FIFO_ENABLE | FIFO_RCV_RST | FIFO_XMT_RST | FIFO_TRIGGER_1);
	bus_space_write_1(iot, ioh, com_ier, 0);

	*iohp = ioh;
	return (0);
#endif /* XXXCGD */
	/* XXXCGD */
	return (0);
}

/*
 * Following are all routines needed for sbscn to act as console
 */
int
sbscn_cnattach(u_long addr, int chan, int rate, tcflag_t cflag)
{
	int res;
	static struct consdev sbscn_cons = {
		NULL, NULL, sbscn_cngetc, sbscn_cnputc, sbscn_cnpollc, NULL,
		    NULL, NULL, NODEV, CN_NORMAL
	};

	res = sbscn_init(addr, chan, rate, cflag);
	if (res)
		return (res);

	cn_tab = &sbscn_cons;

	sbscn_cons_present = 1;
	sbscn_cons_addr = addr;
	sbscn_cons_chan = chan;
	sbscn_cons_rate = rate;
	sbscn_cons_cflag = cflag;

	return (0);
}

int
sbscn_cngetc(dev_t dev)
{

	return (sbscn_common_getc(sbscn_cons_addr, sbscn_cons_chan));
}

/*
 * Console kernel output character routine.
 */
void
sbscn_cnputc(dev_t dev, int c)
{

	sbscn_common_putc(sbscn_cons_addr, sbscn_cons_chan, c);
}

void
sbscn_cnpollc(dev_t dev, int on)
{

}

#ifdef KGDB
int
sbscn_kgdb_attach(u_long addr, int chan, int rate, tcflag_t cflag)
{
	int res;

	if (!sbscn_cons_present &&
	    sbscn_cons_addr == addr && sbscn_cons_chan == chan)
		return (EBUSY); /* cannot share with console */

	res = sbscn_init(addr, chan, rate, cflag);
	if (res)
		return (res);

	kgdb_attach(sbscn_kgdb_getc, sbscn_kgdb_putc, NULL);
	kgdb_dev = 123; /* unneeded, only to satisfy some tests */

	sbscn_kgdb_present = 1;
	sbscn_kgdb_addr = addr;
	sbscn_kgdb_chan = chan;

	return (0);
}

/* ARGSUSED */
int
sbscn_kgdb_getc(arg)
	void *arg;
{

	return (sbscn_common_getc(sbscn_kgdb_addr, sbscn_kgdb_chan));
}

/* ARGSUSED */
void
sbscn_kgdb_putc(arg, c)
	void *arg;
	int c;
{

	sbscn_common_getc(sbscn_kgdb_addr, sbscn_kgdb_chan, c);
}
#endif /* KGDB */

/*
 * helper function to identify the sbscn channels used by
 * console or KGDB (and not yet autoconf attached)
 */
int
sbscn_is_console(u_long addr, int chan)
{

	if (sbscn_cons_present && !sbscn_cons_attached &&
	    sbscn_cons_addr == addr && sbscn_cons_chan == chan)
		return (1);
#ifdef KGDB
	if (sbscn_kgdb_present && !sbscn_kgdb_attached &&
	    sbscn_kgdb_addr == addr && sbscn_kgdb_chan == chan)
		return (1);
#endif
	return (0);
}
