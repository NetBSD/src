/*	$NetBSD: qvaux.c,v 1.1.2.2 2015/09/22 12:05:53 skrll Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles H. Dickman
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
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 */

/*
 * Copyright (c) 1996  Ken C. Wellsch.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/kauth.h>

#include <sys/bus.h>
#include <dev/qbus/ubavar.h>

#include <vax/uba/qvareg.h>
#include <vax/uba/qvavar.h>
#include <vax/uba/qvkbdvar.h>

#include <dev/cons.h>
#include "qv.h"
#include "qvkbd.h"
#include "qvms.h"
#include "qv_ic.h"

#define QVAUX_DELAY(x) /* nothing */
#define	control		inline

static control uint
qvaux_read1(struct qvaux_softc *sc, u_int off)
{
	u_int rv;

	rv = bus_space_read_1(sc->sc_iot, sc->sc_ioh, off);
	QVAUX_DELAY(1);
	return rv;
}

static control u_int
qvaux_read2(struct qvaux_softc *sc, u_int off)
{
	u_int rv;

	rv = bus_space_read_2(sc->sc_iot, sc->sc_ioh, off);
	QVAUX_DELAY(1);
	return rv;
}

static control void
qvaux_write1(struct qvaux_softc *sc, u_int off, u_int val)
{

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, off, val);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, sc->sc_qr.qr_firstreg,
	    sc->sc_qr.qr_winsize, BUS_SPACE_BARRIER_WRITE |
	    BUS_SPACE_BARRIER_READ);
	QVAUX_DELAY(10);
}

static control void
qvaux_write2(struct qvaux_softc *sc, u_int off, u_int val)
{

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, off, val);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, sc->sc_qr.qr_firstreg,
	    sc->sc_qr.qr_winsize, BUS_SPACE_BARRIER_WRITE |
	    BUS_SPACE_BARRIER_READ);
	QVAUX_DELAY(10);
}

#include "ioconf.h"

/* Flags used to monitor modem bits, make them understood outside driver */

#define DML_DTR		TIOCM_DTR
#define DML_DCD		TIOCM_CD
#define DML_RI		TIOCM_RI
#define DML_BRK		0100000		/* no equivalent, we will mask */

static const struct speedtab qvauxspeedtab[] =
{
  {       0,	0		},
  {      75,	CSR_B75	        },
  {     110,	CSR_B110	},
  {     134,	CSR_B134	},
  {     150,	CSR_B150	},
  {     300,	CSR_B300	},
  {     600,	CSR_B600	},
  {    1200,	CSR_B1200	},
  {    2000,	CSR_B2000	},
  {    2400,	CSR_B2400	},
  {    4800,	CSR_B4800	},
  {    7200,	CSR_B7200	},
  {    9600,	CSR_B9600	},
  {   19200,	CSR_B19200	},
  {      -1,	-1		}
};

int             qvaux_match(device_t, cfdata_t, void *);
static void     qvaux_attach(device_t , device_t , void *);
static void	qvauxstart(struct tty *);
static int	qvauxparam(struct tty *, struct termios *);
static unsigned	qvauxmctl(struct qvaux_softc *, int, int, int);
//static void	qvauxscan(void *);
int             qvauxgetc(struct qvaux_linestate *);
void            qvauxputc(struct qvaux_linestate *, int);

static dev_type_open(qvauxopen);
static dev_type_close(qvauxclose);
static dev_type_read(qvauxread);
static dev_type_write(qvauxwrite);
static dev_type_ioctl(qvauxioctl);
static dev_type_stop(qvauxstop);
static dev_type_tty(qvauxtty);
static dev_type_poll(qvauxpoll);

const struct cdevsw qvaux_cdevsw = {
	qvauxopen, qvauxclose, qvauxread, qvauxwrite, qvauxioctl,
	qvauxstop, qvauxtty, qvauxpoll, nommap, ttykqfilter, nodiscard, D_TTY
};

int	qvaux_timer;	/* true if timer started */
struct callout qvauxscan_ch;
static struct cnm_state qvaux_cnm_state;

CFATTACH_DECL_NEW(qvaux, sizeof(struct qvaux_softc),
    qvaux_match, qvaux_attach, NULL, NULL);

#if NQVKBD > 0 || NQVMS > 0
static int
qvaux_print(void *aux, const char *name)
{
        struct qvauxkm_attach_args *daa = aux;
        if (name == NULL) {
                aprint_normal(" line %d", daa->daa_line);
        }
        
        return QUIET;
}
#endif 

int
qvaux_match(device_t parent, cfdata_t match, void *aux)
{
        /* always match since we are physically part of parent */
        return 1;
}

/*ARGSUSED*/
static void
qvaux_attach(device_t parent, device_t self, void *aux)
{
	struct qvaux_softc *sc = device_private(self);
	struct uba_attach_args *ua = aux;
#if NQVKBD > 0 || NQVMS > 0
        struct qvauxkm_attach_args daa;
#endif

	/* set floating DUART vector and enable interrupts */
        qv_ic_setvec(ua, QVA_QVIC, QV_DUART_VEC, ua->ua_cvec); 
        qv_ic_arm(ua, QVA_QVIC, QV_IC_ENA);
	bus_space_write_2(ua->ua_iot, ua->ua_ioh, QVA_QVCSR,
	    bus_space_read_2(ua->ua_iot, ua->ua_ioh, QVA_QVCSR) | (1 << 6));  

	sc->sc_dev = self;
	sc->sc_iot = ua->ua_iot;
	sc->sc_ioh = ua->ua_ioh;

        /* device register access structure */
        sc->sc_qr.qr_ipcr = DU_IPCR;        
        sc->sc_qr.qr_acr = DU_ACR;        
        sc->sc_qr.qr_isr = DU_ISR;        
        sc->sc_qr. qr_imr = DU_IMR;        
        sc->sc_qr.qr_ctur = DU_CTUR;        
        sc->sc_qr.qr_ctlr = DU_CTLR;        
        sc->sc_qr.qr_ip = DU_IP;        
        sc->sc_qr.qr_opcr = DU_OPCR;        
        sc->sc_qr.qr_cstrt = DU_IMR;        
        sc->sc_qr.qr_opset = DU_OPSET;        
        sc->sc_qr.qr_cstop = DU_CSTOP;        
        sc->sc_qr.qr_opclr = DU_OPCLR;        
        sc->sc_qr.qr_ch_regs[0].qr_mr = CH_MR(0);
        sc->sc_qr.qr_ch_regs[0].qr_sr = CH_SR(0);        
        sc->sc_qr.qr_ch_regs[0].qr_csr = CH_CSR(0);        
        sc->sc_qr.qr_ch_regs[0].qr_cr = CH_CR(0);        
        sc->sc_qr.qr_ch_regs[0].qr_dat = CH_DAT(0);                
        sc->sc_qr.qr_ch_regs[1].qr_mr = CH_MR(1);
        sc->sc_qr.qr_ch_regs[1].qr_sr = CH_SR(1);        
        sc->sc_qr.qr_ch_regs[1].qr_csr = CH_CSR(1);        
        sc->sc_qr.qr_ch_regs[1].qr_cr = CH_CR(1);        
        sc->sc_qr.qr_ch_regs[1].qr_dat = CH_DAT(1);                

	sc->sc_qr.qr_firstreg = QVA_FIRSTREG;
	sc->sc_qr.qr_winsize = QVA_WINSIZE;

        /* register DUART interrupt handler */
	uba_intr_establish(ua->ua_icookie, ua->ua_cvec,
		qvauxint, sc, &sc->sc_tintrcnt);
        qv_ic_enable(ua, QVA_QVIC, QV_DUART_VEC, QV_IC_ENA);

	qvauxattach(sc, ua->ua_evcnt, -1);
	
#if NQVKBD > 0
        /* XXX set line parameters */
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[0].qr_csr, 
	    (CSR_B4800 << 4) | CSR_B4800);
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[0].qr_cr, CR_CMD_MR1 | CR_ENA_RX);
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[0].qr_mr, MR1_CS8 | MR1_PNONE);
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[0].qr_mr, MR2_STOP1);

        daa.daa_line = 0;
        daa.daa_flags = 0;
        config_found(self, &daa, qvaux_print);
#endif
#if NQVMS > 0
        /* XXX set line parameters */
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[1].qr_csr, 
	    (CSR_B4800 << 4) | CSR_B4800);
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[1].qr_cr, CR_CMD_MR1 | CR_ENA_RX);
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[1].qr_mr, MR1_CS8 | MR1_PODD);
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[1].qr_mr, MR2_STOP1);

        daa.daa_line = 1;
        daa.daa_flags = 0;
        config_found(self, &daa, qvaux_print);
#endif
	
}

void
qvauxattach(struct qvaux_softc *sc, struct evcnt *parent_evcnt, int consline)
{
	int n;
        dev_t dev;

	/* Initialize our softc structure. */
	for (n = 0; n < NQVAUXLINE; n++) {
		sc->sc_qvaux[n].qvaux_sc = sc;
		sc->sc_qvaux[n].qvaux_line = n;
		sc->sc_qvaux[n].qvaux_tty = tty_alloc();
		dev = sc->sc_qvaux[n].qvaux_tty->t_dev;
		sc->sc_qvaux[n].qvaux_tty->t_dev = makedev(major(dev),n);
	}

	evcnt_attach_dynamic(&sc->sc_rintrcnt, EVCNT_TYPE_INTR, parent_evcnt,
	    device_xname(sc->sc_dev), "rintr");
	evcnt_attach_dynamic(&sc->sc_tintrcnt, EVCNT_TYPE_INTR, parent_evcnt,
	    device_xname(sc->sc_dev), "tintr");

	/* Console magic keys */
	cn_init_magic(&qvaux_cnm_state);
	cn_set_magic("\047\001"); /* default magic is BREAK */
				  /* VAX will change it in MD code */

	sc->sc_rxint = sc->sc_brk = 0;
	sc->sc_consline = consline;

	sc->sc_imr = INT_RXA | INT_RXB;
	qvaux_write2(sc, sc->sc_qr.qr_imr, sc->sc_imr);
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[0].qr_cr, CR_ENA_TX | CR_ENA_RX);
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[1].qr_cr, CR_ENA_TX | CR_ENA_RX);
	
	DELAY(10000);

	printf("\n");
}

/* DUART Interrupt entry */

void
qvauxint(void *arg)
{
	struct qvaux_softc *sc = arg;
        int isr;
 
        isr = qvaux_read2(sc, sc->sc_qr.qr_isr);
        
        if (isr & (INT_RXA | INT_RXB | INT_BRKA | INT_BRKB))
                qvauxrint(arg);

        isr = qvaux_read2(sc, sc->sc_qr.qr_isr);

        if (isr & (INT_TXA | INT_TXB) & sc->sc_imr)
                qvauxxint(arg);
}

/* Receiver Interrupt */

void
qvauxrint(void *arg)
{
	struct qvaux_softc *sc = arg;
	struct tty *tp;
	int cc, mcc, line;
	unsigned stat[2];
	int overrun = 0;

        //printf(" qvauxrint ");

	sc->sc_rxint++;

        // determine source and loop until all are no longer active
	for (;;) {
	        stat[0] = qvaux_read2(sc, sc->sc_qr.qr_ch_regs[0].qr_sr);
	        stat[1] = qvaux_read2(sc, sc->sc_qr.qr_ch_regs[1].qr_sr);
	        if ((stat[0] & SR_RX_RDY) == 0) {
	                if ((stat[1] & SR_RX_RDY) == 0)
	                        break;
	                else
	                        line = 1;
	        }
	        else
	                line = 0;
		cc = qvaux_read2(sc, sc->sc_qr.qr_ch_regs[line].qr_dat) & 0xFF;
		tp = sc->sc_qvaux[line].qvaux_tty;

		/* Must be caught early */
		if (sc->sc_qvaux[line].qvaux_catch &&
		    (*sc->sc_qvaux[line].qvaux_catch)(sc->sc_qvaux[line]
	  	    .qvaux_private, cc)) {
			continue;
		}

		if (stat[line] & SR_BREAK) // do SR error bits need to be 
					   //   cleared by an error reset?
			mcc = CNC_BREAK;
		else
			mcc = cc;

		cn_check_magic(tp->t_dev, mcc, qvaux_cnm_state);

		if (!(tp->t_state & TS_ISOPEN)) {
			cv_broadcast(&tp->t_rawcv);
			continue;
		}

		if ((stat[line] & SR_OVERRUN) && overrun == 0) {        // ?
			log(LOG_WARNING, "%s: silo overflow, line %d\n",
			    device_xname(sc->sc_dev), line);
			overrun = 1;
		}

		if (stat[line] & SR_FRAME) // ?
			cc |= TTY_FE;
		if (stat[line] & SR_PARITY) // ?
			cc |= TTY_PE;

		(*tp->t_linesw->l_rint)(cc, tp);
	}
}

/* Transmitter Interrupt */

void
qvauxxint(void *arg)
{
	struct qvaux_softc *sc = arg;
	struct tty *tp;
	struct clist *cl;
	int line, ch, stat[2];

	for (;;) {
	        stat[0] = qvaux_read2(sc, sc->sc_qr.qr_ch_regs[0].qr_sr);
	        stat[1] = qvaux_read2(sc, sc->sc_qr.qr_ch_regs[1].qr_sr);
	        if (((stat[0] & SR_TX_RDY) == 0) 
		    || ((sc->sc_imr & INT_TXA) == 0)) {
	                if (((stat[1] & SR_TX_RDY) == 0) 
			    || ((sc->sc_imr & INT_TXB) == 0))
	                        break;
	                else
	                        line = 1;
	        }
	        else
	                line = 0;
		tp = sc->sc_qvaux[line].qvaux_tty;
		cl = &tp->t_outq;
		tp->t_state &= ~TS_BUSY;

		/* Just send out a char if we have one */
		/* As long as we can fill the chip buffer, we just loop here */
		// no fifo, just holding register
		if (cl->c_cc) {
			tp->t_state |= TS_BUSY;
			ch = getc(cl);
			qvaux_write1(sc, sc->sc_qr.qr_ch_regs[line].qr_dat, ch);
			continue;
		}
		/* Nothing to send, clear the tx flags */
		sc->sc_imr &= ~((line) ? (INT_TXB) : (INT_TXA));
		qvaux_write2(sc, sc->sc_qr.qr_imr, sc->sc_imr);

		if (sc->sc_qvaux[line].qvaux_catch)
			continue;

		if (tp->t_state & TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;
		else
			ndflush (&tp->t_outq, cl->c_cc);

		(*tp->t_linesw->l_start)(tp);
	}
}

int
qvauxopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	const int line = QVA_PORT(minor(dev));
	struct qvaux_softc *sc = device_lookup_private(&qvaux_cd, 
   	    QVA_I2C(minor(dev))); // only one controller
	struct tty *tp;
	int error = 0;

	if (sc == NULL || line >= NQVAUXLINE)
		return ENXIO;

	/* if some other device is using the line, it's busy */
	if (sc->sc_qvaux[line].qvaux_catch)
		return EBUSY;

	tp = sc->sc_qvaux[line].qvaux_tty;
	if (tp == NULL)
		return (ENODEV);
		
	tp->t_oproc = qvauxstart;
	tp->t_param = qvauxparam;
	tp->t_dev = dev;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		if (tp->t_ispeed == 0) {
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		}
		(void) qvauxparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}

	/* Use DMBIS and *not* DMSET or else we clobber incoming bits */
	if (qvauxmctl(sc, line, DML_DTR, DMBIS) & DML_DCD)
		tp->t_state |= TS_CARR_ON;
	mutex_spin_enter(&tty_lock);
	while (!(flag & O_NONBLOCK) && !(tp->t_cflag & CLOCAL) &&
	       !(tp->t_state & TS_CARR_ON)) {
		tp->t_wopen++;
		error = ttysleep(tp, &tp->t_rawcv, true, 0);
		tp->t_wopen--;
		if (error)
			break;
	}
	mutex_spin_exit(&tty_lock);
	if (error)
		return (error);
	return ((*tp->t_linesw->l_open)(dev, tp));
}

/*ARGSUSED*/
int
qvauxclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	const int line = QVA_PORT(minor(dev));
	struct qvaux_softc *sc = device_lookup_private(&qvaux_cd, 
	    QVA_I2C(minor(dev))); // only one controller
	struct tty *tp = sc->sc_qvaux[line].qvaux_tty;

	(*tp->t_linesw->l_close)(tp, flag);

	/* Make sure a BREAK state is not left enabled. */
	(void) qvauxmctl(sc, line, DML_BRK, DMBIC);

	/* Do a hangup if so required. */
	if ((tp->t_cflag & HUPCL) || tp->t_wopen || !(tp->t_state & TS_ISOPEN))
		(void) qvauxmctl(sc, line, 0, DMSET);

	return ttyclose(tp);
}

int
qvauxread(dev_t dev, struct uio *uio, int flag)
{
	struct qvaux_softc *sc = device_lookup_private(&qvaux_cd, 
	    QVA_I2C(minor(dev))); // only one controller
	struct tty *tp = sc->sc_qvaux[QVA_PORT(minor(dev))].qvaux_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
qvauxwrite(dev_t dev, struct uio *uio, int flag)
{
	struct qvaux_softc *sc = device_lookup_private(&qvaux_cd, 
	    QVA_I2C(minor(dev))); // only one controller
	struct tty *tp = sc->sc_qvaux[QVA_PORT(minor(dev))].qvaux_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
qvauxpoll(dev_t dev, int events, struct lwp *l)
{
	struct qvaux_softc *sc = device_lookup_private(&qvaux_cd, 
	    QVA_I2C(minor(dev))); // only one controller
	struct tty *tp = sc->sc_qvaux[QVA_PORT(minor(dev))].qvaux_tty;

	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

/*ARGSUSED*/
int
qvauxioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct qvaux_softc *sc = device_lookup_private(&qvaux_cd, 
	    QVA_I2C(minor(dev))); // only one controller
	const int line = QVA_PORT(minor(dev));
	struct tty *tp = sc->sc_qvaux[line].qvaux_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error >= 0)
		return (error);

	error = ttioctl(tp, cmd, data, flag, l);
	if (error >= 0)
		return (error);

	switch (cmd) {
	case TIOCSBRK:
		(void) qvauxmctl(sc, line, DML_BRK, DMBIS);
		break;

	case TIOCCBRK:
		(void) qvauxmctl(sc, line, DML_BRK, DMBIC);
		break;

	case TIOCSDTR:
		(void) qvauxmctl(sc, line, DML_DTR, DMBIS);
		break;

	case TIOCCDTR:
		(void) qvauxmctl(sc, line, DML_DTR, DMBIC);
		break;

	case TIOCMSET:
		(void) qvauxmctl(sc, line, *(int *)data, DMSET);
		break;

	case TIOCMBIS:
		(void) qvauxmctl(sc, line, *(int *)data, DMBIS);
		break;

	case TIOCMBIC:
		(void) qvauxmctl(sc, line, *(int *)data, DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = (qvauxmctl(sc, line, 0, DMGET) & ~DML_BRK);
		break;

	default:
		return (EPASSTHROUGH);
	}
	return (0);
}

struct tty *
qvauxtty(dev_t dev)
{
	struct qvaux_softc *sc = device_lookup_private(&qvaux_cd, 
	    QVA_I2C(minor(dev)));

	return sc->sc_qvaux[QVA_PORT(minor(dev))].qvaux_tty;
}

/*ARGSUSED*/
void
qvauxstop(struct tty *tp, int flag)
{
	if ((tp->t_state & (TS_BUSY | TS_TTSTOP)) == TS_BUSY)
		tp->t_state |= TS_FLUSH;
}

void
qvauxstart(struct tty *tp)
{
	struct qvaux_softc *sc;
	int line;
	int s;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP)) {
		splx(s);
		return;
	}
	if (!ttypull(tp)) {
		splx(s);
		return;
	}

	line = QVA_PORT(minor(tp->t_dev));
	sc = device_lookup_private(&qvaux_cd, QVA_I2C(minor(tp->t_dev)));

	tp->t_state |= TS_BUSY;
	sc->sc_imr |= ((line) ? (INT_TXB) : (INT_TXA));
	qvaux_write2(sc, sc->sc_qr.qr_imr, sc->sc_imr);
	qvauxxint(sc);
	splx(s);
}

static int
qvauxparam(struct tty *tp, struct termios *t)
{
	struct qvaux_softc *sc = device_lookup_private(&qvaux_cd, 
	    QVA_I2C(minor(tp->t_dev)));
	const int line = QVA_PORT(minor(tp->t_dev));
	int cflag = t->c_cflag;
	int ispeed = ttspeedtab(t->c_ispeed, qvauxspeedtab);
	int ospeed = ttspeedtab(t->c_ospeed, qvauxspeedtab);
	unsigned mr1, mr2;
	int s;


	/* check requested parameters */
        if (ospeed < 0 || ispeed < 0)
                return (EINVAL);

        tp->t_ispeed = t->c_ispeed;
        tp->t_ospeed = t->c_ospeed;
        tp->t_cflag = cflag;

	if (ospeed == 0) {
		(void) qvauxmctl(sc, line, 0, DMSET);	/* hang up line */
		return (0);
	}

	s = spltty();

	/* XXX This is wrong.  Flush output or the chip gets very confused. */
	//ttywait(tp);

	//lpr = DZ_LPR_RX_ENABLE | ((ispeed&0xF)<<8) | line;

	qvaux_write2(sc, sc->sc_qr.qr_acr, ACR_BRG);
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[line].qr_csr, 
	    (ispeed << 4) | ospeed);
        
        mr1 = mr2 = 0;
        
	switch (cflag & CSIZE)
	{
	  case CS5:
		mr1 |= MR1_CS5;
		break;
	  case CS6:
		mr1 |= MR1_CS6;
		break;
	  case CS7:
		mr1 |= MR1_CS7;
		break;
	  default:
		mr1 |= MR1_CS8;
		break;
	}
	if (cflag & PARENB) {
	        if (cflag & PARODD)
		        mr1 |= MR1_PODD;
		else
		        mr1 |= MR1_PEVEN;
	}
	else
		mr1 |= MR1_PNONE;
	if (cflag & CSTOPB)
		mr2 |= MR2_STOP2;
	else
	        mr2 |= MR2_STOP1;

	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[line].qr_cr, 
	    CR_CMD_MR1 | CR_ENA_RX); // reset to mr1
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[line].qr_mr, mr1);
	qvaux_write2(sc, sc->sc_qr.qr_ch_regs[line].qr_mr, mr2);
	qvaux_write2(sc, sc->sc_qr.qr_acr, ACR_BRG);
	(void) splx(s);
	DELAY(10000);

	return (0);
}

// QVSS has no modem control signals
static unsigned
qvauxmctl(struct qvaux_softc *sc, int line, int bits, int how)
{
	/* unsigned status; */ 
	unsigned mbits;
	unsigned bit;
	int s;

	s = spltty();
	mbits = 0;
	bit = (1 << line);
#if 0

	/* external signals as seen from the port */
	status = qvaux_read1(sc, sc->sc_dr.dr_dcd) | sc->sc_dsr;
	if (status & bit)
		mbits |= DML_DCD;
	status = qvaux_read1(sc, sc->sc_dr.dr_ring);
	if (status & bit)
		mbits |= DML_RI;

	/* internal signals/state delivered to port */
	status = qvaux_read1(sc, sc->sc_dr.dr_dtr);
	if (status & bit)
		mbits |= DML_DTR;
#endif
	if (sc->sc_brk & bit)
		mbits |= DML_BRK;

	switch (how)
	{
	  case DMSET:
		mbits = bits;
		break;

	  case DMBIS:
		mbits |= bits;
		break;

	  case DMBIC:
		mbits &= ~bits;
		break;

	  case DMGET:
		(void) splx(s);
		return (mbits);
	}
#if 0
	if (mbits & DML_DTR) {
		qvaux_write1(sc, sc->sc_dr.dr_dtr, 
		    qvaux_read1(sc, sc->sc_dr.dr_dtr) | bit);
	} else {
		qvaux_write1(sc, sc->sc_dr.dr_dtr, 
		    qvaux_read1(sc, sc->sc_dr.dr_dtr) & ~bit);
	}
#endif
	if (mbits & DML_BRK) {
		sc->sc_brk |= bit;
		qvaux_write1(sc, sc->sc_qr.qr_ch_regs[line].qr_cr, 
		    CR_CMD_START_BRK);
	} else {
		sc->sc_brk &= ~bit;
		qvaux_write1(sc, sc->sc_qr.qr_ch_regs[line].qr_cr, 
		    CR_CMD_STOP_BRK);
	}

	(void) splx(s);

	return (mbits);
}

/*
 * Called after an ubareset. The QVSS card is reset, but the only thing
 * that must be done is to start the receiver and transmitter again.
 * No DMA setup to care about.
 */
void
qvauxreset(device_t dev)
{
	struct qvaux_softc *sc = device_private(dev);
	struct tty *tp;
	int i;

	for (i = 0; i < NQVAUXLINE; i++) {
		tp = sc->sc_qvaux[i].qvaux_tty;

		if (((tp->t_state & TS_ISOPEN) == 0) || (tp->t_wopen == 0))
			continue;

		qvauxparam(tp, &tp->t_termios);
		qvauxmctl(sc, i, DML_DTR, DMSET);
		tp->t_state &= ~TS_BUSY;
		qvauxstart(tp);	/* Kick off transmitter again */
	}
}

#if NQVKBD > 0 || NQVMS > 0
int
qvauxgetc(struct qvaux_linestate *ls)
{
#if 0
	int line = ls->qvaux_line;
	int s;
	u_short rbuf;

	s = spltty();
	for (;;) {
		for(; (dz->csr & DZ_CSR_RX_DONE) == 0;);
		rbuf = dz->rbuf;
		if (((rbuf >> 8) & 3) == line) {
			splx(s);
			return (rbuf & 0xff);
		}
	}
#endif
        return 0;
}

void
qvauxputc(struct qvaux_linestate *ls, int ch)
{
	//int line;
	int s;

	/* if the qvaux has already been attached, the MI
	   driver will do the transmitting: */
	if (ls && ls->qvaux_sc) {
		s = spltty();
	//	line = ls->qvaux_line;
		putc(ch, &ls->qvaux_tty->t_outq);
		qvauxstart(ls->qvaux_tty);
		splx(s);
		return;
	}

	/* use qvauxcnputc to do the transmitting: */
	//qvauxcnputc(makedev(cdevsw_lookup_major(&qvaux_cdevsw), 0), ch);
}
#endif /* NQVKBD > 0 || NQVMS > 0 */
