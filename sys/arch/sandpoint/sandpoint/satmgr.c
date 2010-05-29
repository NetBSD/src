/* $NetBSD: satmgr.c,v 1.1 2010/05/29 22:47:02 phx Exp $ */

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

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/bootinfo.h>

#include <sandpoint/sandpoint/eumbvar.h>
#include "locators.h"

struct satmgr_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	kmutex_t		sc_lock;
	struct selinfo		sc_rsel;
	callout_t		sc_ch_wdog;
	callout_t		sc_ch_pbutton;
	struct sysmon_pswitch	sc_sm_pbutton;
	int			sc_open;
	void			*sc_si;
	uint32_t		sc_ierror, sc_overr;
	char sc_rd_buf[16], *sc_rd_lim, *sc_rd_cur, *sc_rd_ptr;
	char sc_wr_buf[16], *sc_wr_lim, *sc_wr_cur, *sc_wr_ptr;
	int sc_rd_cnt, sc_wr_cnt;
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
	satopen, satclose, satread, satwrite, noioctl,
	nostop, notty, satpoll, nommap, satkqfilter, D_OTHER
};

static void satmgr_reboot(int);
static int satmgr_sysctl_wdogenable(SYSCTLFN_PROTO);
static void wdog_tickle(void *);
static void send_sat(struct satmgr_softc *, const char *);
static int hwintr(void *);
static void rxintr(struct satmgr_softc *);
static void startoutput(struct satmgr_softc *);
static void swintr(void *);
static void kbutton(struct satmgr_softc *, int);
static void sbutton(struct satmgr_softc *, int);
static void qbutton(struct satmgr_softc *, int);
static void guarded_pbutton(void *);
static void sched_sysmon_pbutton(void *);

struct satmsg {
	const char *family;
	const char *reboot, *poweroff;
	void (*dispatch)(struct satmgr_softc *, int);
};

const struct satmsg satmodel[] = {
    { "kurobox",  "CCGG", "EEGG", kbutton },
    { "synology", "C",    "1",    sbutton },
    { "qnap",     "f",    "A",    qbutton }
} *satmgr_msg;

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

static int satmgr_wdog;

static int
satmgr_match(device_t parent, cfdata_t match, void *aux)
{
	struct eumb_attach_args *eaa = aux;
	int unit = eaa->eumb_unit;

	if (unit == EUMBCF_UNIT_DEFAULT && found == 0)
		return (1);
	if (unit == 0 || unit == 1)
		return (1);
	return (0);
}

static void
satmgr_attach(device_t parent, device_t self, void *aux)
{
	struct eumb_attach_args *eaa = aux;
	struct satmgr_softc *sc = device_private(self);
	struct btinfo_prodfamily *pfam;
	int i, sataddr, epicirq;

	found = 1;
	
	if ((pfam = lookup_bootinfo(BTINFO_PRODFAMILY)) == NULL)
		goto notavail;
	satmgr_msg = NULL;
	for (i = 0; i < (int)(sizeof(satmodel)/sizeof(satmodel[0])); i++) {
		if (strcmp(pfam->name, satmodel[i].family) == 0) {
			satmgr_msg = &satmodel[i];
			break;
		}
	}
	if (satmgr_msg == NULL)
		goto notavail;
	
	aprint_normal(": button manager (%s)\n", satmgr_msg->family);

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
	selinit(&sc->sc_rsel);
	callout_init(&sc->sc_ch_wdog, 0);
	callout_init(&sc->sc_ch_pbutton, 0);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);

	epicirq = (eaa->eumb_unit == 0) ? 24 : 25;
	intr_establish(epicirq + 16, IST_LEVEL, IPL_SERIAL, hwintr, sc);
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

	if (strcmp(satmgr_msg->family, "kurobox") == 0) {
		const struct sysctlnode *rnode;
		struct sysctllog *clog;

		/* create machdep.satmgr.* subtree */
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

	md_reboot = satmgr_reboot; /* cpu_reboot() hook */
	return;

  notavail:
	aprint_normal(": button manager (not supported)\n");
}

static void
satmgr_reboot(int howto)
{
	const char *msg;
	struct satmgr_softc *sc = device_lookup_private(&satmgr_cd, 0);

	if ((howto & RB_POWERDOWN) == RB_AUTOBOOT)
		msg = satmgr_msg->reboot;	/* REBOOT */
	else
		msg = satmgr_msg->poweroff;	/* HALT or POWERDOWN */
	send_sat(sc, msg);
	tsleep(satmgr_reboot, PWAIT, "reboot", 0);
	/*NOTREACHED*/
}

static int
satmgr_sysctl_wdogenable(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;
	struct satmgr_softc *sc;

	node = *rnode;
	t = satmgr_wdog;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0 || t > 1)
		return EINVAL;

	sc = device_lookup_private(&satmgr_cd, 0);
	if (t == 1) {
		callout_setfunc(&sc->sc_ch_wdog, wdog_tickle, sc);
		callout_schedule(&sc->sc_ch_wdog, 90 * hz);
		send_sat(sc, "JJ");
	}
	else {
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

static void
send_sat(struct satmgr_softc *sc, const char *msg)
{
	unsigned lsr, ch, n;

 again:
	do {
		lsr = CSR_READ(sc, LSR);
	} while ((lsr & LSR_TXRDY) == 0);
	n = 16; /* FIFO depth */
	while ((ch = *msg++) != '\0' && n-- > 0) {
		CSR_WRITE(sc, THR, ch);
	}
	if (ch != '\0')
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

	mutex_spin_enter(&sc->sc_lock);
	if (sc->sc_rd_cnt == 0 && (flags & IO_NDELAY))
		return EWOULDBLOCK;
	error = 0;
	while (sc->sc_rd_cnt == 0) {
		error = tsleep(sc->sc_rd_buf, PZERO|PCATCH, "satrd", 0);
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
	mutex_spin_exit(&sc->sc_lock);
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

	mutex_spin_enter(&sc->sc_lock);
	if (sc->sc_wr_cnt == sizeof(sc->sc_wr_buf) && (flags & IO_NDELAY))
		return EWOULDBLOCK;
	error = 0;
	while (sc->sc_wr_cnt == sizeof(sc->sc_wr_buf)) {
		error = tsleep(sc->sc_wr_buf, PZERO|PCATCH, "satwr", 0);
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
	mutex_spin_exit(&sc->sc_lock);
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

static const struct filterops read_filtops =
	{ 1, NULL, filt_rdetach, filt_read };			

int
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
		return (EINVAL);
	}

	kn->kn_hook = sc;

	mutex_enter(&sc->sc_lock);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	mutex_exit(&sc->sc_lock);

	return (0);
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
			startoutput(sc);
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
startoutput(struct satmgr_softc *sc)
{
	int n, ch;

	n = min(sc->sc_wr_cnt, 16);
	while ((ch = *sc->sc_wr_ptr) && n-- > 0) {
		CSR_WRITE(sc, THR, ch);
		if (++sc->sc_wr_ptr == sc->sc_wr_lim)
			sc->sc_wr_ptr = &sc->sc_wr_buf[0];
		sc->sc_wr_cnt -= 1;
	}
}

static void
swintr(void *arg)
{
	struct satmgr_softc *sc = arg;
	char *ptr;
	int n;

	/* we're now in softint(9) context */
	ptr = sc->sc_rd_ptr;
	for (n = 0; n < sc->sc_rd_cnt; n++) {
		(*satmgr_msg->dispatch)(sc, *ptr);
		if (++ptr == sc->sc_rd_lim)
			ptr = &sc->sc_rd_buf[0];
	}
	if (sc->sc_open == 0) {
		sc->sc_rd_cnt = 0;
		sc->sc_rd_ptr = ptr;
		return; /* drop characters down to floor */
	}
	wakeup(sc->sc_rd_buf);
	selnotify(&sc->sc_rsel, 0, 0);
}

void
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

void
sbutton(struct satmgr_softc *sc, int ch)
{

	switch (ch) {
	case '0':
		/* notified after 3 secord guard time */
		sysmon_task_queue_sched(0, sched_sysmon_pbutton, sc);
		break;
	case '?':
	case '`':
		break;
	}
}

void
qbutton(struct satmgr_softc *sc, int ch)
{
	/* research in progress */
}

static void
guarded_pbutton(void *arg)
{
	struct satmgr_softc *sc = arg;

	/* we're now in callout(9) context */
	sysmon_task_queue_sched(0, sched_sysmon_pbutton, sc);
	send_sat(sc, "UU");
}

static void
sched_sysmon_pbutton(void *arg)
{
	struct satmgr_softc *sc = arg;

	/* we're now in kthread(9) context */
	sysmon_pswitch_event(&sc->sc_sm_pbutton, PSWITCH_EVENT_PRESSED);
}
