/* $NetBSD: satmgr.c,v 1.25.2.1 2014/08/10 06:54:07 tls Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/callout.h>
#include <sys/sysctl.h>
#include <sys/reboot.h>
#include <sys/intr.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <dev/ic/comreg.h>

#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/bootinfo.h>

#include <sandpoint/sandpoint/eumbvar.h>
#include "locators.h"


struct satops;

struct satmgr_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct selinfo		sc_rsel;
	callout_t		sc_ch_wdog;
	callout_t		sc_ch_pbutton;
	callout_t		sc_ch_sync;
	struct sysmon_pswitch	sc_sm_pbutton;
	int			sc_open;
	void			*sc_si;
	uint32_t		sc_ierror, sc_overr;
	kmutex_t		sc_lock;
	kcondvar_t		sc_rdcv, sc_wrcv;
	char			sc_rd_buf[16];
	char			*sc_rd_lim, *sc_rd_cur, *sc_rd_ptr;
	char			sc_wr_buf[16];
	char			*sc_wr_lim, *sc_wr_cur, *sc_wr_ptr;
	int			sc_rd_cnt, sc_wr_cnt;
	struct satops		*sc_ops;
	char			sc_btn_buf[8];
	int			sc_btn_cnt;
	char			sc_cmd_buf[8];
	kmutex_t		sc_replk;
	kcondvar_t		sc_repcv;
	int			sc_sysctl_wdog;
	int			sc_sysctl_fanlow;
	int			sc_sysctl_fanhigh;
};

static int  satmgr_match(device_t, cfdata_t, void *);
static void satmgr_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(satmgr, sizeof(struct satmgr_softc),
    satmgr_match, satmgr_attach, NULL, NULL);
extern struct cfdriver satmgr_cd;

static int found = 0;
extern void (*md_reboot)(int);

static dev_type_open(satopen);
static dev_type_close(satclose);
static dev_type_read(satread);
static dev_type_write(satwrite);
static dev_type_poll(satpoll);
static dev_type_kqfilter(satkqfilter);

const struct cdevsw satmgr_cdevsw = {
	.d_open = satopen,
	.d_close = satclose,
	.d_read = satread,
	.d_write = satwrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = satpoll,
	.d_mmap = nommap,
	.d_kqfilter = satkqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

static void satmgr_reboot(int);
static int satmgr_sysctl_wdogenable(SYSCTLFN_PROTO);
static int satmgr_sysctl_fanlow(SYSCTLFN_PROTO);
static int satmgr_sysctl_fanhigh(SYSCTLFN_PROTO);
static void wdog_tickle(void *);
static void send_sat(struct satmgr_softc *, const char *);
static void send_sat_len(struct satmgr_softc *, const char *, int);
static int hwintr(void *);
static void rxintr(struct satmgr_softc *);
static void txintr(struct satmgr_softc *);
static void startoutput(struct satmgr_softc *);
static void swintr(void *);
static void minit(device_t);
static void sinit(device_t);
static void qinit(device_t);
static void iinit(device_t);
static void kreboot(struct satmgr_softc *);
static void mreboot(struct satmgr_softc *);
static void sreboot(struct satmgr_softc *);
static void qreboot(struct satmgr_softc *);
static void ireboot(struct satmgr_softc *);
static void kpwroff(struct satmgr_softc *);
static void mpwroff(struct satmgr_softc *);
static void spwroff(struct satmgr_softc *);
static void qpwroff(struct satmgr_softc *);
static void dpwroff(struct satmgr_softc *);
static void ipwroff(struct satmgr_softc *);
static void kbutton(struct satmgr_softc *, int);
static void mbutton(struct satmgr_softc *, int);
static void sbutton(struct satmgr_softc *, int);
static void qbutton(struct satmgr_softc *, int);
static void dbutton(struct satmgr_softc *, int);
static void ibutton(struct satmgr_softc *, int);
static void msattalk(struct satmgr_softc *, const char *);
static void isattalk(struct satmgr_softc *, int, int, int, int, int, int);
static int  mbtnintr(void *);
static void guarded_pbutton(void *);
static void sched_sysmon_pbutton(void *);

struct satops {
	const char *family;
	void (*init)(device_t);
	void (*reboot)(struct satmgr_softc *);
	void (*pwroff)(struct satmgr_softc *);
	void (*dispatch)(struct satmgr_softc *, int);
};

static struct satops satmodel[] = {
    { "dlink",    NULL,  NULL,    dpwroff, dbutton },
    { "iomega",   iinit, ireboot, ipwroff, ibutton },
    { "kurobox",  NULL,  kreboot, kpwroff, kbutton },
    { "kurot4",   minit, mreboot, mpwroff, mbutton },
    { "qnap",     qinit, qreboot, qpwroff, qbutton },
    { "synology", sinit, sreboot, spwroff, sbutton }
};

/* single byte stride register layout */
#define RBR		0
#define THR		0
#define DLB		0
#define IER		1
#define DMB		1
#define IIR		2
#define LCR		3
#define LSR		5
#define CSR_READ(t,r)	bus_space_read_1((t)->sc_iot, (t)->sc_ioh, (r))
#define CSR_WRITE(t,r,v) bus_space_write_1((t)->sc_iot, (t)->sc_ioh, (r), (v))

static int
satmgr_match(device_t parent, cfdata_t match, void *aux)
{
	struct eumb_attach_args *eaa = aux;
	int unit = eaa->eumb_unit;

	if (unit == EUMBCF_UNIT_DEFAULT && found == 0)
		return 1;
	if (unit == 0 || unit == 1)
		return 1;
	return 0;
}

static void
satmgr_attach(device_t parent, device_t self, void *aux)
{
	struct eumb_attach_args *eaa = aux;
	struct satmgr_softc *sc = device_private(self);
	struct btinfo_prodfamily *pfam;
	struct satops *ops;
	int i, sataddr, epicirq;

	found = 1;

	if ((pfam = lookup_bootinfo(BTINFO_PRODFAMILY)) == NULL)
		goto notavail;
	ops = NULL;
	for (i = 0; i < (int)(sizeof(satmodel)/sizeof(satmodel[0])); i++) {
		if (strcmp(pfam->name, satmodel[i].family) == 0) {
			ops = &satmodel[i];
			break;
		}
	}
	if (ops == NULL)
		goto notavail;
	aprint_naive(": button manager\n");
	aprint_normal(": button manager (%s)\n", ops->family);
	sc->sc_ops = ops;

	sc->sc_dev = self;
	sataddr = (eaa->eumb_unit == 0) ? 0x4500 : 0x4600;
	sc->sc_iot = eaa->eumb_bt;
	bus_space_map(eaa->eumb_bt, sataddr, 0x20, 0, &sc->sc_ioh);
	sc->sc_open = 0;
	sc->sc_rd_cnt = 0;
	sc->sc_rd_cur = sc->sc_rd_ptr = &sc->sc_rd_buf[0];
	sc->sc_rd_lim = sc->sc_rd_cur + sizeof(sc->sc_rd_buf);
	sc->sc_wr_cnt = 0;
	sc->sc_wr_cur = sc->sc_wr_ptr = &sc->sc_wr_buf[0];
	sc->sc_wr_lim = sc->sc_wr_cur + sizeof(sc->sc_wr_buf);
	sc->sc_ierror = sc->sc_overr = 0;
	selinit(&sc->sc_rsel);
	callout_init(&sc->sc_ch_wdog, 0);
	callout_init(&sc->sc_ch_pbutton, 0);
	callout_init(&sc->sc_ch_sync, 0);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);
	cv_init(&sc->sc_rdcv, "satrd");
	cv_init(&sc->sc_wrcv, "satwr");
	sc->sc_btn_cnt = 0;
	mutex_init(&sc->sc_replk, MUTEX_DEFAULT, IPL_SERIAL);
	cv_init(&sc->sc_repcv, "stalk");

	epicirq = (eaa->eumb_unit == 0) ? 24 : 25;
	intr_establish(epicirq + I8259_ICU, IST_LEVEL, IPL_SERIAL, hwintr, sc);
	aprint_normal_dev(self, "interrupting at irq %d\n",
	    epicirq + I8259_ICU);
	sc->sc_si = softint_establish(SOFTINT_SERIAL, swintr, sc);

	CSR_WRITE(sc, IER, 0x7f); /* all but MSR */

	if (!pmf_device_register(sc->sc_dev, NULL, NULL))
		aprint_error_dev(sc->sc_dev, "couldn't establish handler\n");

	sysmon_task_queue_init();
	memset(&sc->sc_sm_pbutton, 0, sizeof(struct sysmon_pswitch));
	sc->sc_sm_pbutton.smpsw_name = device_xname(sc->sc_dev);
	sc->sc_sm_pbutton.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&sc->sc_sm_pbutton) != 0)
		aprint_error_dev(sc->sc_dev,
		    "unable to register power button with sysmon\n");

	/* create machdep.satmgr subtree for those models which support it */
	if (strcmp(ops->family, "kurobox") == 0) {
		const struct sysctlnode *rnode;
		struct sysctllog *clog;

		clog = NULL;
		sysctl_createv(&clog, 0, NULL, &rnode,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "machdep", NULL,
			NULL, 0, NULL, 0,
			CTL_MACHDEP, CTL_EOL);
		sysctl_createv(&clog, 0, &rnode, &rnode,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "satmgr", NULL,
			NULL, 0, NULL, 0,
			CTL_CREATE, CTL_EOL);
		sysctl_createv(&clog, 0, &rnode, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "hwwdog_enable",
			SYSCTL_DESCR("watchdog enable"),
			satmgr_sysctl_wdogenable, 0, NULL, 0,
			CTL_CREATE, CTL_EOL);
	}
	else if (strcmp(ops->family, "iomega") == 0) {
		const struct sysctlnode *rnode;
		struct sysctllog *clog;

		clog = NULL;
		sysctl_createv(&clog, 0, NULL, &rnode,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "machdep", NULL,
			NULL, 0, NULL, 0,
			CTL_MACHDEP, CTL_EOL);
		sysctl_createv(&clog, 0, &rnode, &rnode,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "satmgr", NULL,
			NULL, 0, NULL, 0,
			CTL_CREATE, CTL_EOL);
		sysctl_createv(&clog, 0, &rnode, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "fan_low_temp",
			SYSCTL_DESCR("Turn off fan below this temperature"),
			satmgr_sysctl_fanlow, 0, NULL, 0,
			CTL_CREATE, CTL_EOL);
		sysctl_createv(&clog, 0, &rnode, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "fan_high_temp",
			SYSCTL_DESCR("Turn on fan above this temperature"),
			satmgr_sysctl_fanhigh, 0, NULL, 0,
			CTL_CREATE, CTL_EOL);
	}
	else if (strcmp(ops->family, "kurot4") == 0) {
		intr_establish(2 + I8259_ICU,
			IST_LEVEL, IPL_SERIAL, mbtnintr, sc);
	}

	md_reboot = satmgr_reboot;	/* cpu_reboot() hook */
	if (ops->init != NULL)		/* init sat.cpu, LEDs, etc. */
		config_interrupts(self, ops->init);
	return;

  notavail:
	aprint_normal(": button manager (not supported)\n");
}

static void
satmgr_reboot(int howto)
{
	struct satmgr_softc *sc = device_lookup_private(&satmgr_cd, 0);

	if ((howto & RB_POWERDOWN) == RB_AUTOBOOT) {
		if (sc->sc_ops->reboot != NULL)
			(*sc->sc_ops->reboot)(sc);	/* REBOOT */
		else
			return;				/* default reboot */
	} else
		if (sc->sc_ops->pwroff != NULL)
			(*sc->sc_ops->pwroff)(sc);	/* HALT or POWERDOWN */

	tsleep(satmgr_reboot, PWAIT, "reboot", 0);
	/*NOTREACHED*/
}

static int
satmgr_sysctl_wdogenable(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct satmgr_softc *sc;
	int error, t;

	sc = device_lookup_private(&satmgr_cd, 0);
	node = *rnode;
	t = sc->sc_sysctl_wdog;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	if (t < 0 || t > 1)
		return EINVAL;

	sc->sc_sysctl_wdog = t;
	if (t == 1) {
		callout_setfunc(&sc->sc_ch_wdog, wdog_tickle, sc);
		callout_schedule(&sc->sc_ch_wdog, 90 * hz);
		send_sat(sc, "JJ");
	} else {
		callout_stop(&sc->sc_ch_wdog);
		send_sat(sc, "KK");
	}
	return 0;
}

/* disarm watchdog timer periodically */
static void
wdog_tickle(void *arg)
{
	struct satmgr_softc *sc = arg;

	send_sat(sc, "GG");
	callout_schedule(&sc->sc_ch_wdog, 90 * hz);
}

static int
satmgr_sysctl_fanlow(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct satmgr_softc *sc;
	int error, t;

	sc = device_lookup_private(&satmgr_cd, 0);
	node = *rnode;
	t = sc->sc_sysctl_fanlow;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	if (t < 0 || t > 99)
		return EINVAL;
	sc->sc_sysctl_fanlow = t;
	isattalk(sc, 'b', 'b', 10, 'a',
	    sc->sc_sysctl_fanhigh, sc->sc_sysctl_fanlow);
	return 0;
}

static int
satmgr_sysctl_fanhigh(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct satmgr_softc *sc;
	int error, t;

	sc = device_lookup_private(&satmgr_cd, 0);
	node = *rnode;
	t = sc->sc_sysctl_fanhigh;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;
	if (t < 0 || t > 99)
		return EINVAL;
	sc->sc_sysctl_fanhigh = t;
	isattalk(sc, 'b', 'b', 10, 'a',
	    sc->sc_sysctl_fanhigh, sc->sc_sysctl_fanlow);
	return 0;
}

static void
send_sat(struct satmgr_softc *sc, const char *msg)
{

	send_sat_len(sc, msg, strlen(msg));
}


static void
send_sat_len(struct satmgr_softc *sc, const char *msg, int len)
{
	unsigned lsr;
	int n;

 again:
	do {
		lsr = CSR_READ(sc, LSR);
	} while ((lsr & LSR_TXRDY) == 0);
	n = 16; /* FIFO depth */
	while (len-- > 0 && n-- > 0)
		CSR_WRITE(sc, THR, *msg++);
	if (len > 0)
		goto again;
}

static int
satopen(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct satmgr_softc *sc;

	sc = device_lookup_private(&satmgr_cd, 0);
	if (sc == NULL)
		return ENXIO;
	if (sc->sc_open > 0)
		return EBUSY;
	sc->sc_open = 1;
	return 0;
}

static int
satclose(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct satmgr_softc *sc;

	sc = device_lookup_private(&satmgr_cd, 0);
	if (sc == NULL)
		return ENXIO;
	KASSERT(sc->sc_open > 0);
	sc->sc_open = 0;
	return 0;
}

static int
satread(dev_t dev, struct uio *uio, int flags)
{
	struct satmgr_softc *sc;
	size_t n;
	int error;

	sc = device_lookup_private(&satmgr_cd, 0);
	if (sc == NULL)
		return ENXIO;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_rd_cnt == 0 && (flags & IO_NDELAY)) {
		error = EWOULDBLOCK;
		goto out;
	}
	error = 0;
	while (sc->sc_rd_cnt == 0) {
		error = cv_wait_sig(&sc->sc_rdcv, &sc->sc_lock);
		if (error)
			goto out;
	}
	while (uio->uio_resid > 0 && sc->sc_rd_cnt > 0) {
		n = min(sc->sc_rd_cnt, uio->uio_resid);
		n = min(n, sc->sc_rd_lim - sc->sc_rd_ptr);
		error = uiomove(sc->sc_rd_ptr, n, uio);
		if (error)
			goto out;
		sc->sc_rd_cnt -= n;
		sc->sc_rd_ptr += n;
		if (sc->sc_rd_ptr == sc->sc_rd_lim)
			sc->sc_rd_ptr = &sc->sc_rd_buf[0];
	}
 out:
	mutex_exit(&sc->sc_lock);
	return error;
}

static int
satwrite(dev_t dev, struct uio *uio, int flags)
{
	struct satmgr_softc *sc;
	int error;
	size_t n;

	sc = device_lookup_private(&satmgr_cd, 0);
	if (sc == NULL)
		return ENXIO;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_wr_cnt == sizeof(sc->sc_wr_buf) && (flags & IO_NDELAY)) {
		error = EWOULDBLOCK;
		goto out;
	}
	error = 0;
	while (sc->sc_wr_cnt == sizeof(sc->sc_wr_buf)) {
		error = cv_wait_sig(&sc->sc_wrcv, &sc->sc_lock);
		if (error)
			goto out;
	}
	while (uio->uio_resid > 0 && sc->sc_wr_cnt < sizeof(sc->sc_wr_buf)) {
		n = min(uio->uio_resid, sizeof(sc->sc_wr_buf));
		n = min(n, sc->sc_wr_lim - sc->sc_wr_cur);
		error = uiomove(sc->sc_wr_cur, n, uio);
		if (error)
			goto out;
		sc->sc_wr_cnt += n;
		sc->sc_wr_cur += n;
		if (sc->sc_wr_cur == sc->sc_wr_lim)
			sc->sc_wr_cur = &sc->sc_wr_buf[0];
	}
	startoutput(sc); /* start xmit */
 out:
	mutex_exit(&sc->sc_lock);
	return error;
}

static int
satpoll(dev_t dev, int events, struct lwp *l)
{
	struct satmgr_softc *sc;
	int revents = 0;

	sc = device_lookup_private(&satmgr_cd, 0);
	mutex_enter(&sc->sc_lock);
	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->sc_rd_cnt)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(l, &sc->sc_rsel);
	}
	mutex_exit(&sc->sc_lock);

	return revents;
}

static void
filt_rdetach(struct knote *kn)
{
	struct satmgr_softc *sc = kn->kn_hook;

	mutex_enter(&sc->sc_lock);
	SLIST_REMOVE(&sc->sc_rsel.sel_klist, kn, knote, kn_selnext);
	mutex_exit(&sc->sc_lock);
}

static int
filt_read(struct knote *kn, long hint)
{
	struct satmgr_softc *sc = kn->kn_hook;

	kn->kn_data = sc->sc_rd_cnt;
	return (kn->kn_data > 0);
}

static const struct filterops read_filtops = {
	1, NULL, filt_rdetach, filt_read
};

static int
satkqfilter(dev_t dev, struct knote *kn)
{
	struct satmgr_softc *sc = device_lookup_private(&satmgr_cd, 0);
	struct klist *klist;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_rsel.sel_klist;
		kn->kn_fop = &read_filtops;
		break;

	default:
		return EINVAL;
	}

	kn->kn_hook = sc;

	mutex_enter(&sc->sc_lock);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	mutex_exit(&sc->sc_lock);

	return 0;
}

static int
hwintr(void *arg)
{
	struct satmgr_softc *sc = arg;
	int iir;

	mutex_spin_enter(&sc->sc_lock);
	iir = CSR_READ(sc, IIR) & IIR_IMASK;
	if (iir == IIR_NOPEND) {
		mutex_spin_exit(&sc->sc_lock);
		return 0;
	}
	do {
		switch (iir) {
		case IIR_RLS:	/* LSR updated */
		case IIR_RXRDY: /* RxFIFO has been accumulated */
		case IIR_RXTOUT:/* receive timeout occurred */
			rxintr(sc);
			break;
		case IIR_TXRDY: /* TxFIFO is ready to swallow data */
			txintr(sc);
			break;
		case IIR_MLSC:	/* MSR updated */
			break;
		}
		iir = CSR_READ(sc, IIR) & IIR_IMASK;
	} while (iir != IIR_NOPEND);
	mutex_spin_exit(&sc->sc_lock);
	return 1;
}

static void
rxintr(struct satmgr_softc *sc)
{
	int lsr, ch;

	lsr = CSR_READ(sc, LSR);
	if (lsr & LSR_OE)
		sc->sc_overr++;
	ch = -1;
	while (lsr & LSR_RXRDY) {
		if (lsr & (LSR_BI | LSR_FE | LSR_PE)) {
			(void) CSR_READ(sc, RBR);
			sc->sc_ierror++;
			lsr = CSR_READ(sc, LSR);
			continue;
		}
		ch = CSR_READ(sc, RBR);
		if (sc->sc_rd_cnt < sizeof(sc->sc_rd_buf)) {
			*sc->sc_rd_cur = ch;
			if (++sc->sc_rd_cur == sc->sc_rd_lim)
				sc->sc_rd_cur = &sc->sc_rd_buf[0];
			sc->sc_rd_cnt += 1;
		}
		lsr = CSR_READ(sc, LSR);
	}
	if (ch != -1)
		softint_schedule(sc->sc_si);
}

static void
txintr(struct satmgr_softc *sc)
{

	cv_signal(&sc->sc_wrcv);
	startoutput(sc);
}

static void
startoutput(struct satmgr_softc *sc)
{
	int n;

	mutex_enter(&sc->sc_replk);
	n = min(sc->sc_wr_cnt, 16);
	while (n-- > 0) {
		CSR_WRITE(sc, THR, *sc->sc_wr_ptr);
		if (++sc->sc_wr_ptr == sc->sc_wr_lim)
			sc->sc_wr_ptr = &sc->sc_wr_buf[0];
		sc->sc_wr_cnt -= 1;
	}
	mutex_exit(&sc->sc_replk);
}

static void
swintr(void *arg)
{
	struct satmgr_softc *sc = arg;
	char *ptr;
	int n;

	/* we're now in softint(9) context */
	mutex_spin_enter(&sc->sc_lock);
	ptr = sc->sc_rd_ptr;
	for (n = 0; n < sc->sc_rd_cnt; n++) {
		(*sc->sc_ops->dispatch)(sc, *ptr);
		if (++ptr == sc->sc_rd_lim)
			ptr = &sc->sc_rd_buf[0];
	}
	if (sc->sc_open == 0) {
		sc->sc_rd_cnt = 0;
		sc->sc_rd_ptr = ptr;
		mutex_spin_exit(&sc->sc_lock);
		return;		/* drop characters down to the floor */
	}
	cv_signal(&sc->sc_rdcv);
	selnotify(&sc->sc_rsel, 0, 0);
	mutex_spin_exit(&sc->sc_lock);
}

static void
kreboot(struct satmgr_softc *sc)
{

	send_sat(sc, "CCGG"); /* perform reboot */
}

static void
kpwroff(struct satmgr_softc *sc)
{

	send_sat(sc, "EEGG"); /* force power off */
}

static void
kbutton(struct satmgr_softc *sc, int ch)
{

	switch (ch) {
	case '!':
		/* schedule 3 second poweroff guard time */
		if (callout_pending(&sc->sc_ch_pbutton) == true)
			callout_stop(&sc->sc_ch_pbutton);
		callout_reset(&sc->sc_ch_pbutton,
		     3 * hz, guarded_pbutton, sc);
		break;
	case ' ':
		if (callout_expired(&sc->sc_ch_pbutton) == false)
			callout_stop(&sc->sc_ch_pbutton);
		else
			/* should never come here */;
		break;
	case '#':
	case '"':
		break;
	}
}

static void
sinit(device_t self)
{
	struct satmgr_softc *sc = device_private(self);

	send_sat(sc, "8");	/* status LED green */
}

static void
sreboot(struct satmgr_softc *sc)
{

	send_sat(sc, "C");
}

static void
spwroff(struct satmgr_softc *sc)
{

	send_sat(sc, "1");
}

static void
sbutton(struct satmgr_softc *sc, int ch)
{

	switch (ch) {
	case '0':
		/* notified after 5 seconds guard time */
		sysmon_task_queue_sched(0, sched_sysmon_pbutton, sc);
		break;
	case 'a':
	case '`':
		break;
	}
}

static void
qinit(device_t self)
{
	struct satmgr_softc *sc = device_private(self);

	send_sat(sc, "V");	/* status LED green */
}

static void
qreboot(struct satmgr_softc *sc)
{

	send_sat(sc, "Pf");	/* beep and reboot */
}

static void
qpwroff(struct satmgr_softc *sc)
{

	send_sat(sc, "PA");	/* beep and power off */
}

static void
qbutton(struct satmgr_softc *sc, int ch)
{

	switch (ch) {
	case '@':
		/* power button, notified after 2 seconds guard time */
		sysmon_task_queue_sched(0, sched_sysmon_pbutton, sc);
		break;
	case 'j':	/* reset to default button */
	case 'h':	/* USB copy button */
		break;
	}
}

static void
dpwroff(struct satmgr_softc *sc)
{

	send_sat(sc, "ZWC\n");

	/*
	 * When this line is reached, then this board revision doesn't
	 * support hardware-shutdown, so we flash the power LED
	 * to indicate that the device can be switched off.
	 */
	send_sat(sc, "SYN\nSYN\n");

	/* drops into default power-off handling (looping forever) */
}

static void
dbutton(struct satmgr_softc *sc, int ch)
{

	if (sc->sc_btn_cnt < sizeof(sc->sc_btn_buf))
		sc->sc_btn_buf[sc->sc_btn_cnt++] = ch;
	if (ch == '\n' || ch == '\r') {
		if (memcmp(sc->sc_btn_buf, "PKO", 3) == 0) {
			/* notified after 5 seconds guard time */
			sysmon_task_queue_sched(0, sched_sysmon_pbutton, sc);
		}
		sc->sc_btn_cnt = 0;
	}
}

static void
iinit(device_t self)
{
	struct satmgr_softc *sc = device_private(self);

	/* LED blue, auto-fan, turn on at 50C, turn off at 45C */
	sc->sc_sysctl_fanhigh = 50;
	sc->sc_sysctl_fanlow = 45;
	isattalk(sc, 'b', 'b', 10, 'a',
	    sc->sc_sysctl_fanhigh, sc->sc_sysctl_fanlow);
}

static void
ireboot(struct satmgr_softc *sc)
{

	isattalk(sc, 'g', 0, 0, 0, 0, 0);
}

static void
ipwroff(struct satmgr_softc *sc)
{

	isattalk(sc, 'c', 0, 0, 0, 0, 0);
}

static void
ibutton(struct satmgr_softc *sc, int ch)
{

	mutex_enter(&sc->sc_replk);
	if (++sc->sc_btn_cnt >= 8) {
		cv_signal(&sc->sc_repcv);
		sc->sc_btn_cnt = 0;
	}
	mutex_exit(&sc->sc_replk);
}

static void
isattalk(struct satmgr_softc *sc, int pow, int led, int rat, int fan,
    int fhi, int flo)
{
	char *p = sc->sc_cmd_buf;
	int i, cksum;

	/*
	 * Construct the command packet. Values of -1 (0xff) will be
	 * replaced later by the current values from the last status.
	 */
	p[0] = pow;
	p[1] = led;
	p[2] = rat;
	p[3] = fan;
	p[4] = fhi;
	p[5] = flo;
	p[6] = 7; /* host id */
	for (i = 0, cksum = 0; i < 7; i++)
		cksum += p[i];
	p[7] = cksum & 0x7f;
	send_sat_len(sc, p, 8);

	mutex_enter(&sc->sc_replk);
	sc->sc_btn_cnt = 0;
	cv_wait(&sc->sc_repcv, &sc->sc_replk);
	mutex_exit(&sc->sc_replk);
}


static void
minit(device_t self)
{
	struct satmgr_softc *sc = device_private(self);
#if 0
	static char msg[35] = "\x20\x92NetBSD/sandpoint";
	int m, n;

	m = strlen(osrelease);
	n = (16 - m) / 2;
	memset(&msg[18], ' ', 16);
	memcpy(&msg[18 + n], osrelease, m);

	msattalk(sc, "\x00\x03");	/* boot has completed */
	msattalk(sc, msg);		/* NB banner at disp2 */
	msattalk(sc, "\x01\x32\x80");	/* select disp2 */
	msattalk(sc, "\x00\x27");	/* show disp2 */
#else
	msattalk(sc, "\x00\x03");	/* boot has completed */
#endif
}

static void
mreboot(struct satmgr_softc *sc)
{

	msattalk(sc, "\x01\x35\x00");	/* stop watchdog timer */
	msattalk(sc, "\x00\x0c");	/* shutdown in progress */
	msattalk(sc, "\x00\x03");	/* boot has completed */
	msattalk(sc, "\x00\x0e");	/* perform reboot */
}

static void
mpwroff(struct satmgr_softc *sc)
{

	msattalk(sc, "\x01\x35\x00");	/* stop watchdog timer */
	msattalk(sc, "\x00\x0c");	/* shutdown in progress */
	msattalk(sc, "\x00\x03");	/* boot has completed */
	msattalk(sc, "\x00\x06");	/* force power off */
}

static void
msattalk(struct satmgr_softc *sc, const char *cmd)
{
	int len, i;
	uint8_t pa;

	if (cmd[0] != 0x80)
		len = 2 + cmd[0]; /* cmd[0] is data portion length */
	else
		len = 2; /* read report */

	for (i = 0, pa = 0; i < len; i++)
		pa += cmd[i];
	pa = 0 - pa; /* parity formula */

	send_sat_len(sc, cmd, len);
	send_sat_len(sc, &pa, 1);

	mutex_enter(&sc->sc_replk);
	sc->sc_btn_cnt = 0;
	cv_wait(&sc->sc_repcv, &sc->sc_replk);
	mutex_exit(&sc->sc_replk);
}

static void
mbutton(struct satmgr_softc *sc, int ch)
{

	mutex_enter(&sc->sc_replk);
	if (sc->sc_btn_cnt < 4) /* record the first four */
		sc->sc_btn_buf[sc->sc_btn_cnt] = ch;
	sc->sc_btn_cnt++;
	if (sc->sc_btn_cnt == sc->sc_btn_buf[0] + 3) {
		if (sc->sc_btn_buf[1] == 0x36 && (sc->sc_btn_buf[2]&01) == 0) {
			/* power button pressed */
			sysmon_task_queue_sched(0, sched_sysmon_pbutton, sc);
		}
		else {
			/* unblock the talker */
			cv_signal(&sc->sc_repcv);
		}
		sc->sc_btn_cnt = 0;
	}
	mutex_exit(&sc->sc_replk);
}

static int
mbtnintr(void *arg)
{
	/* notified after 3 seconds guard time */
	struct satmgr_softc *sc = arg;

	send_sat(sc, "\x80\x36\x4a"); /* query button state with parity */
	mutex_enter(&sc->sc_replk);
	sc->sc_btn_cnt = 0;
	mutex_exit(&sc->sc_replk);
	return 1;
}

static void
guarded_pbutton(void *arg)
{
	struct satmgr_softc *sc = arg;

	/* we're now in callout(9) context */
	sysmon_task_queue_sched(0, sched_sysmon_pbutton, sc);
	send_sat(sc, "UU"); /* make front panel LED flashing */
}

static void
sched_sysmon_pbutton(void *arg)
{
	struct satmgr_softc *sc = arg;

	/* we're now in kthread(9) context */
	sysmon_pswitch_event(&sc->sc_sm_pbutton, PSWITCH_EVENT_PRESSED);
}
