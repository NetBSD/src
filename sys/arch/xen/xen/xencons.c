/*	$NetBSD: xencons.c,v 1.43.2.1 2018/06/25 07:25:48 pgoyette Exp $	*/

/*
 * Copyright (c) 2006 Manuel Bouyer.
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
 */

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
__KERNEL_RCSID(0, "$NetBSD: xencons.c,v 1.43.2.1 2018/06/25 07:25:48 pgoyette Exp $");

#include "opt_xen.h"

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/kernel.h>

#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <uvm/uvm.h>
#include <machine/pmap.h>
#include <xen/xen-public/io/console.h>

#include <dev/cons.h>

#ifdef DDB
#include <ddb/db_output.h>	/* XXX for db_max_line */
#endif

#undef XENDEBUG
 
#ifdef XENDEBUG
#define XENPRINTK(x) printk x
#else 
#define XENPRINTK(x)
#endif

static int xencons_isconsole = 0;
static struct xencons_softc *xencons_console_device = NULL;
static struct intrhand *ih;

#define	XENCONS_UNIT(x)	(minor(x))
#define XENCONS_BURST 128

int xencons_match(device_t, cfdata_t, void *);
void xencons_attach(device_t, device_t, void *);
int xencons_intr(void *);
void xencons_tty_input(struct xencons_softc *, char*, int);


struct xencons_softc {
	device_t sc_dev;
	struct	tty *sc_tty;
	int polling;
};
volatile struct xencons_interface *xencons_interface;

CFATTACH_DECL_NEW(xencons, sizeof(struct xencons_softc),
    xencons_match, xencons_attach, NULL, NULL);

extern struct cfdriver xencons_cd;

dev_type_open(xencons_open);
dev_type_close(xencons_close);
dev_type_read(xencons_read);
dev_type_write(xencons_write);
dev_type_ioctl(xencons_ioctl);
dev_type_stop(xencons_stop);
dev_type_tty(xencons_tty);
dev_type_poll(xencons_poll);

const struct cdevsw xencons_cdevsw = {
	.d_open = xencons_open,
	.d_close = xencons_close,
	.d_read = xencons_read,
	.d_write = xencons_write,
	.d_ioctl = xencons_ioctl, 
	.d_stop = xencons_stop,
	.d_tty = xencons_tty,
	.d_poll = xencons_poll,
	.d_mmap = NULL,	/* XXX: is this safe? - dholland 20140315 */
	.d_kqfilter = ttykqfilter,
	.d_discard = nodiscard,
	.d_flag = D_TTY
};


static int xencons_handler(void *);
int xenconscn_getc(dev_t);
void xenconscn_putc(dev_t, int);
void xenconscn_pollc(dev_t, int);

static bool xencons_suspend(device_t, const pmf_qual_t *);
static bool xencons_resume(device_t, const pmf_qual_t *);

static struct consdev xencons = {
	NULL, NULL, xenconscn_getc, xenconscn_putc, xenconscn_pollc,
	NULL, NULL, NULL, NODEV, CN_NORMAL
};

static struct cnm_state xencons_cnm_state;

void	xencons_start (struct tty *);
int	xencons_param (struct tty *, struct termios *);

int
xencons_match(device_t parent, cfdata_t match, void *aux)
{
	struct xencons_attach_args *xa = (struct xencons_attach_args *)aux;

	if (strcmp(xa->xa_device, "xencons") == 0)
		return 1;
	return 0;
}

void
xencons_attach(device_t parent, device_t self, void *aux)
{
	struct xencons_softc *sc = device_private(self);

	aprint_normal(": Xen Virtual Console Driver\n");

	sc->sc_dev = self;
	sc->sc_tty = tty_alloc();
	tty_attach(sc->sc_tty);
	sc->sc_tty->t_oproc = xencons_start;
	sc->sc_tty->t_param = xencons_param;

	if (xencons_isconsole) {
		int maj;

		/* Locate the major number. */
		maj = cdevsw_lookup_major(&xencons_cdevsw);

		/* There can be only one, but it can have any unit number. */
		cn_tab->cn_dev = makedev(maj, device_unit(self));

		aprint_verbose_dev(self, "console major %d, unit %d\n",
		    maj, device_unit(self));

		sc->sc_tty->t_dev = cn_tab->cn_dev;

#ifdef DDB
		/* Set db_max_line to avoid paging. */
		db_max_line = 0x7fffffff;
#endif

		xencons_console_device = sc;

		xencons_resume(self, PMF_Q_NONE);
	}
	sc->polling = 0;

	if (!pmf_device_register(self, xencons_suspend, xencons_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static bool
xencons_suspend(device_t dev, const pmf_qual_t *qual) {

	int evtch;

	/* dom0 console should not be suspended */
	if (!xendomain_is_dom0()) {
		evtch = xen_start_info.console_evtchn;
		hypervisor_mask_event(evtch);
		intr_disestablish(ih);
		aprint_verbose_dev(dev, "removed event channel %d\n", ih->ih_pin);
	}

	return true;
}

static bool
xencons_resume(device_t dev, const pmf_qual_t *qual) {

	int evtch = -1;

	if (xendomain_is_dom0()) {
		/* dom0 console resume is required only during first start-up */
		if (cold) {
			evtch = bind_virq_to_evtch(VIRQ_CONSOLE);
			ih = intr_establish_xname(0, &xen_pic, evtch,
			    IST_LEVEL, IPL_TTY, xencons_intr,
			    xencons_console_device, false,
			    device_xname(dev));
			KASSERT(ih != NULL);
		}
	} else {
		evtch = xen_start_info.console_evtchn;
		ih = intr_establish_xname(0, &xen_pic, evtch,
		    IST_LEVEL, IPL_TTY, xencons_handler,
		    xencons_console_device, false, device_xname(dev));
		KASSERT(ih != NULL);
	}

	if (evtch != -1) {
		aprint_verbose_dev(dev, "using event channel %d\n", evtch);
		hypervisor_enable_event(evtch);
	}

	return true;
}

int
xencons_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct xencons_softc *sc;
	struct tty *tp;

	sc = device_lookup_private(&xencons_cd, XENCONS_UNIT(dev));
	if (sc == NULL)
		return (ENXIO);

	tp = sc->sc_tty;

	if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
		return (EBUSY);

	if ((tp->t_state & TS_ISOPEN) == 0 && tp->t_wopen == 0) {
		tp->t_dev = dev;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		xencons_param(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	tp->t_state |= TS_CARR_ON;

	return ((*tp->t_linesw->l_open)(dev, tp));
}

int
xencons_close(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct xencons_softc *sc = device_lookup_private(&xencons_cd,
	    XENCONS_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	if (tp == NULL)
		return (0);
	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
#ifdef notyet /* XXX */
	tty_free(tp);
#endif
	return (0);
}

int
xencons_read(dev_t dev, struct uio *uio, int flag)
{
	struct xencons_softc *sc = device_lookup_private(&xencons_cd,
	    XENCONS_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}

int
xencons_write(dev_t dev, struct uio *uio, int flag)
{
	struct xencons_softc *sc = device_lookup_private(&xencons_cd,
	    XENCONS_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
xencons_poll(dev_t dev, int events, struct lwp *l)
{
	struct xencons_softc *sc = device_lookup_private(&xencons_cd,
	    XENCONS_UNIT(dev));
	struct tty *tp = sc->sc_tty;
 
	return ((*tp->t_linesw->l_poll)(tp, events, l));
}

struct tty *
xencons_tty(dev_t dev)
{
	struct xencons_softc *sc = device_lookup_private(&xencons_cd,
	    XENCONS_UNIT(dev));
	struct tty *tp = sc->sc_tty;

	return (tp);
}

int
xencons_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct xencons_softc *sc = device_lookup_private(&xencons_cd,
	    XENCONS_UNIT(dev));
	struct tty *tp = sc->sc_tty;
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	error = ttioctl(tp, cmd, data, flag, l);
	if (error != EPASSTHROUGH)
		return (error);

	switch (cmd) {
	default:
		return (EPASSTHROUGH);
	}

#ifdef DIAGNOSTIC
	panic("xencons_ioctl: impossible");
#endif
}

void
xencons_start(struct tty *tp)
{
	struct clist *cl;
	int s;

	s = spltty();
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto out;
	tp->t_state |= TS_BUSY;
	splx(s);

	/*
	 * We need to do this outside spl since it could be fairly
	 * expensive and we don't want our serial ports to overflow.
	 */
	cl = &tp->t_outq;
	if (xendomain_is_dom0()) {
		int len, r;
		u_char buf[XENCONS_BURST+1];

		len = q_to_b(cl, buf, XENCONS_BURST);
		while (len > 0) {
			r = HYPERVISOR_console_io(CONSOLEIO_write, len, buf);
			if (r <= 0)
				break;
			len -= r;
		}
	} else {
		XENCONS_RING_IDX cons, prod, len;

#define XNC_OUT (xencons_interface->out)
		cons = xencons_interface->out_cons;
		prod = xencons_interface->out_prod;
		xen_rmb();
		while (prod != cons + sizeof(xencons_interface->out)) {
			if (MASK_XENCONS_IDX(prod, XNC_OUT) <
			    MASK_XENCONS_IDX(cons, XNC_OUT)) {
				len = MASK_XENCONS_IDX(cons, XNC_OUT) -
				    MASK_XENCONS_IDX(prod, XNC_OUT);
			} else {
				len = sizeof(XNC_OUT) -
				    MASK_XENCONS_IDX(prod, XNC_OUT);
			}
			len = q_to_b(cl, __UNVOLATILE(
			    &XNC_OUT[MASK_XENCONS_IDX(prod, XNC_OUT)]), len);
			if (len == 0)
				break;
			prod = prod + len;
		}
		xen_wmb();
		xencons_interface->out_prod = prod;
		xen_wmb();
		hypervisor_notify_via_evtchn(xen_start_info.console.domU.evtchn);
#undef XNC_OUT
	}

	s = spltty();
	tp->t_state &= ~TS_BUSY;
	if (ttypull(tp)) {
		tp->t_state |= TS_TIMEOUT;
		callout_schedule(&tp->t_rstrt_ch, 1);
	}
out:
	splx(s);
}

void
xencons_stop(struct tty *tp, int flag)
{

}


/* Non-privileged console interrupt routine */
static int 
xencons_handler(void *arg)
{
	struct xencons_softc *sc = arg;
	XENCONS_RING_IDX cons, prod, len;
	int s = spltty();

	if (sc->polling) {
		splx(s);
		return 1;
	}
		

#define XNC_IN (xencons_interface->in)

	cons = xencons_interface->in_cons;
	prod = xencons_interface->in_prod;
	xen_rmb();
	while (cons != prod) {
		if (MASK_XENCONS_IDX(cons, XNC_IN) <
		    MASK_XENCONS_IDX(prod, XNC_IN))
			len = MASK_XENCONS_IDX(prod, XNC_IN) -
			    MASK_XENCONS_IDX(cons, XNC_IN);
		else
			len = sizeof(XNC_IN) - MASK_XENCONS_IDX(cons, XNC_IN);

		xencons_tty_input(sc, __UNVOLATILE(
		    &XNC_IN[MASK_XENCONS_IDX(cons, XNC_IN)]), len);
		if (__predict_false(xencons_interface->in_cons != cons)) {
			/* catch up with xenconscn_getc() */
			cons = xencons_interface->in_cons;
			prod = xencons_interface->in_prod;
			xen_rmb();
		} else {
			cons += len;
			xen_wmb();
			xencons_interface->in_cons = cons;
			xen_wmb();
		}
	}
	hypervisor_notify_via_evtchn(xen_start_info.console.domU.evtchn);
	splx(s);
	return 1;
#undef XNC_IN
}


void
xencons_tty_input(struct xencons_softc *sc, char* buf, int len)
{
	struct tty *tp;
	int i;

	tp = sc->sc_tty;
	if (tp == NULL)
		return;

	for (i = 0; i < len; i++) {
		cn_check_magic(sc->sc_tty->t_dev, buf[i], xencons_cnm_state);
		(*tp->t_linesw->l_rint)(buf[i], tp);
	}
}

/* privileged receive callback */
int
xencons_intr(void *p)
{
	static char rbuf[16];
	int len;
	struct xencons_softc *sc = p;

	if (sc == NULL)
		/* Interrupt may happen during resume */
		return 1;

	if (sc->polling)
		return 1;

	while ((len =
	    HYPERVISOR_console_io(CONSOLEIO_read, sizeof(rbuf), rbuf)) > 0) {
		xencons_tty_input(sc, rbuf, len);
	}
	return 1;
}

void
xenconscn_attach(void)
{

	cn_tab = &xencons;

	/* console ring mapped in locore.S */

	cn_init_magic(&xencons_cnm_state);
	cn_set_magic("+++++");

	xencons_isconsole = 1;
}

int
xenconscn_getc(dev_t dev)
{
	char c;
	int s = spltty();
	XENCONS_RING_IDX cons, prod;

	if (xencons_console_device && xencons_console_device->polling == 0) {
		printf("xenconscn_getc() but not polling\n");
		splx(s);
		return 0;
	}
	if (xendomain_is_dom0()) {
		while (HYPERVISOR_console_io(CONSOLEIO_read, 1, &c) == 0)
			;
		cn_check_magic(dev, c, xencons_cnm_state);
		splx(s);
		return c;
	}
	if (xencons_console_device == NULL) {
		printf("xenconscn_getc(): not console\n");
		while (1)
			;  /* loop here instead of in ddb */
		splx(s);
		return 0;
	}

	if (xencons_console_device->polling == 0) {
		printf("xenconscn_getc() but not polling\n");
		splx(s);
		return 0;
	}

	cons = xencons_interface->in_cons;
	prod = xencons_interface->in_prod;
	xen_rmb();
	while (cons == prod) {
		HYPERVISOR_yield();
		prod = xencons_interface->in_prod;
	}
	xen_rmb();
	c = xencons_interface->in[MASK_XENCONS_IDX(xencons_interface->in_cons,
	    xencons_interface->in)];
	xen_rmb();
	xencons_interface->in_cons = cons + 1;
	cn_check_magic(dev, c, xencons_cnm_state);
	splx(s);
	return c;
}

void
xenconscn_putc(dev_t dev, int c)
{
	int s = spltty();
	XENCONS_RING_IDX cons, prod;

	if (xendomain_is_dom0()) {
		u_char buf[1];

		buf[0] = c;
		(void)HYPERVISOR_console_io(CONSOLEIO_write, 1, buf);
	} else {
		XENPRINTK(("xenconscn_putc(%c)\n", c));

		cons = xencons_interface->out_cons;
		prod = xencons_interface->out_prod;
		xen_rmb();
		while (prod == cons + sizeof(xencons_interface->out)) {
			cons = xencons_interface->out_cons;
			prod = xencons_interface->out_prod;
			xen_rmb();
		}
		xencons_interface->out[MASK_XENCONS_IDX(xencons_interface->out_prod,
		    xencons_interface->out)] = c;
		xen_rmb();
		xencons_interface->out_prod++;
		xen_rmb();
		hypervisor_notify_via_evtchn(xen_start_info.console.domU.evtchn);
		splx(s);
	}
}

void
xenconscn_pollc(dev_t dev, int on)
{
	if (xencons_console_device)
		xencons_console_device->polling = on;
}

/*
 * Set line parameters.
 */
int
xencons_param(struct tty *tp, struct termios *t)
{

	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	tp->t_cflag = t->c_cflag;
	return (0);
}
