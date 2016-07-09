/*	$NetBSD: gencons.c,v 1.54.4.1 2016/07/09 20:24:58 skrll Exp $	*/

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *
 *	kd.c,v 1.2 1994/05/05 04:46:51 gwr Exp $
 */

 /* All bugs are subject to removal without further notice */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gencons.c,v 1.54.4.1 2016/07/09 20:24:58 skrll Exp $");

#include "opt_ddb.h"
#include "opt_cputype.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/kauth.h>
#include <sys/proc.h>
#include <sys/tty.h>

#include <dev/cons.h>

#include <machine/sid.h>
#include <machine/scb.h>
#include <vax/vax/gencons.h>

static	struct gc_softc {
	short alive;
	short unit;
	struct tty *gencn_tty;
} gc_softc[4];

static	int maxttys = 1;

static	int pr_txcs[4] = {PR_TXCS, PR_TXCS1, PR_TXCS2, PR_TXCS3};
static	int pr_rxcs[4] = {PR_RXCS, PR_RXCS1, PR_RXCS2, PR_RXCS3};
static	int pr_txdb[4] = {PR_TXDB, PR_TXDB1, PR_TXDB2, PR_TXDB3};
static	int pr_rxdb[4] = {PR_RXDB, PR_RXDB1, PR_RXDB2, PR_RXDB3};

cons_decl(gen);

static	int gencnparam(struct tty *, struct termios *);
static	void gencnstart(struct tty *);

dev_type_open(gencnopen);
dev_type_close(gencnclose);
dev_type_read(gencnread);
dev_type_write(gencnwrite);
dev_type_ioctl(gencnioctl);
dev_type_tty(gencntty);
dev_type_poll(gencnpoll);

const struct cdevsw gen_cdevsw = {
	.d_open = gencnopen,
	.d_close = gencnclose,
	.d_read = gencnread,
	.d_write = gencnwrite,
	.d_ioctl = gencnioctl,
	.d_stop = nostop,
	.d_tty = gencntty,
	.d_poll = gencnpoll,
	.d_mmap = nommap,
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};

int
gencnopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit;
	struct tty *tp;

	unit = minor(dev);
	if (unit >= maxttys)
		return ENXIO;

	if (gc_softc[unit].gencn_tty == NULL)
		gc_softc[unit].gencn_tty = tty_alloc();

	gc_softc[unit].alive = 1;
	gc_softc[unit].unit = unit;
	tp = gc_softc[unit].gencn_tty;

	tp->t_oproc = gencnstart;
	tp->t_param = gencnparam;
	tp->t_dev = dev;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		gencnparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	tp->t_state |= TS_CARR_ON;

	return ((*tp->t_linesw->l_open)(dev, tp));
}

int
gencnclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct tty *tp = gc_softc[minor(dev)].gencn_tty;

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	gc_softc[minor(dev)].alive = 0;
	return (0);
}

struct tty *
gencntty(dev_t dev)
{
	return gc_softc[minor(dev)].gencn_tty;
}

int
gencnread(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp = gc_softc[minor(dev)].gencn_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
gencnwrite(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tp = gc_softc[minor(dev)].gencn_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
gencnpoll(dev_t dev, int events, struct lwp *l)
{
	struct tty *tp = gc_softc[minor(dev)].gencn_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

int
gencnioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct tty *tp = gc_softc[minor(dev)].gencn_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return error;
	return ttioctl(tp, cmd, data, flag, l);
}

void
gencnstart(struct tty *tp)
{
	struct clist *cl;
	int s, ch;

#if defined(MULTIPROCESSOR)
	if ((curcpu()->ci_flags & CI_MASTERCPU) == 0)
		return cpu_send_ipi(IPI_DEST_MASTER, IPI_START_CNTX);
#endif

	s = spltty();
	if (tp->t_state & (TS_BUSY|TS_TTSTOP|TS_TIMEOUT))
		goto out;
	cl = &tp->t_outq;

	if (ttypull(tp)) {
		tp->t_state |= TS_BUSY;
		ch = getc(cl);
		mtpr(ch, pr_txdb[minor(tp->t_dev)]);
	}
 out:
	splx(s);
}

static void
gencnrint(void *arg)
{
	struct gc_softc *sc = arg;
	struct tty *tp = sc->gencn_tty;
	int i;

	if (sc->alive == 0)
		return;
	i = mfpr(pr_rxdb[sc->unit]) & 0377; /* Mask status flags etc... */
	KERNEL_LOCK(1, NULL);

#ifdef DDB
	if (tp->t_dev == cn_tab->cn_dev) {
		int j = kdbrint(i);

		if (j == 1) {	/* Escape received, just return */
			KERNEL_UNLOCK_ONE(NULL);
			return;
		}

		if (j == 2)	/* Second char wasn't 'D' */
			(*tp->t_linesw->l_rint)(27, tp);
	}
#endif

	(*tp->t_linesw->l_rint)(i, tp);
	KERNEL_UNLOCK_ONE(NULL);
}

static void
gencntint(void *arg)
{
	struct gc_softc *sc = arg;
	struct tty *tp = sc->gencn_tty;

	if (sc->alive == 0)
		return;
	KERNEL_LOCK(1, NULL);
	tp->t_state &= ~TS_BUSY;

	gencnstart(tp);
	KERNEL_UNLOCK_ONE(NULL);
}

int
gencnparam(struct tty *tp, struct termios *t)
{
	/* XXX - These are ignored... */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return 0;
}

void
gencnprobe(struct consdev *cndev)
{
	if ((vax_cputype < VAX_TYP_UV2) || /* All older has MTPR console */
	    (vax_boardtype == VAX_BTYP_9RR) ||
	    (vax_boardtype == VAX_BTYP_630) ||
	    (vax_boardtype == VAX_BTYP_660) ||
	    (vax_boardtype == VAX_BTYP_670) ||
	    (vax_boardtype == VAX_BTYP_680) ||
	    (vax_boardtype == VAX_BTYP_681) ||
	    (vax_boardtype == VAX_BTYP_650)) {
		cndev->cn_dev = makedev(cdevsw_lookup_major(&gen_cdevsw), 0);
		cndev->cn_pri = CN_NORMAL;
	} else
		cndev->cn_pri = CN_DEAD;
}

void
gencninit(struct consdev *cndev)
{

	/* Allocate interrupt vectors */
	scb_vecalloc(SCB_G0R, gencnrint, &gc_softc[0], SCB_ISTACK, NULL);
	scb_vecalloc(SCB_G0T, gencntint, &gc_softc[0], SCB_ISTACK, NULL);
	mtpr(GC_RIE, pr_rxcs[0]); /* Turn on interrupts */
	mtpr(GC_TIE, pr_txcs[0]);

	if (vax_cputype == VAX_TYP_8SS) {
		maxttys = 4;
		scb_vecalloc(SCB_G1R, gencnrint, &gc_softc[1], SCB_ISTACK, NULL);
		scb_vecalloc(SCB_G1T, gencntint, &gc_softc[1], SCB_ISTACK, NULL);

		scb_vecalloc(SCB_G2R, gencnrint, &gc_softc[2], SCB_ISTACK, NULL);
		scb_vecalloc(SCB_G2T, gencntint, &gc_softc[2], SCB_ISTACK, NULL);

		scb_vecalloc(SCB_G3R, gencnrint, &gc_softc[3], SCB_ISTACK, NULL);
		scb_vecalloc(SCB_G3T, gencntint, &gc_softc[3], SCB_ISTACK, NULL);
	}
#if 0
	mtpr(0, PR_RXCS);
	mtpr(0, PR_TXCS);
	mtpr(0, PR_TBIA); /* ??? */
#endif
}

void
gencnputc(dev_t dev, int ch)
{
#if VAX8800 || VAXANY
	/*
	 * On KA88 we may get C-S/C-Q from the console.
	 * XXX - this will cause a loop at spltty() in kernel and will
	 * interfere with other console communication. Fortunately
	 * kernel printf's are uncommon.
	 */
	if (vax_cputype == VAX_TYP_8NN) {
		int s = spltty();

		while (mfpr(PR_RXCS) & GC_DON) {
			if ((mfpr(PR_RXDB) & 0x7f) == 19) {
				while (1) {
					while ((mfpr(PR_RXCS) & GC_DON) == 0)
						;
					if ((mfpr(PR_RXDB) & 0x7f) == 17)
						break;
				}
			}
		}
		splx(s);
	}
#endif

	while ((mfpr(PR_TXCS) & GC_RDY) == 0) /* Wait until xmit ready */
		;
	mtpr(ch, PR_TXDB);	/* xmit character */
	if(ch == 10)
		gencnputc(dev, 13); /* CR/LF */

}

int
gencngetc(dev_t dev)
{
	int i;

	while ((mfpr(PR_RXCS) & GC_DON) == 0) /* Receive chr */
		;
	i = mfpr(PR_RXDB) & 0x7f;
	if (i == 13)
		i = 10;
	return i;
}

void 
gencnpollc(dev_t dev, int pollflag)
{
	if (pollflag)  {
		mtpr(0, PR_RXCS);
		mtpr(0, PR_TXCS);
	} else {
		mtpr(GC_RIE, PR_RXCS);
		mtpr(GC_TIE, PR_TXCS);
	}
}

#if defined(MULTIPROCESSOR)
void
gencnstarttx(void)
{
	gencnstart(gc_softc[0].gencn_tty);
}
#endif
