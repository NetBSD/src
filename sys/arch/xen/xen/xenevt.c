/*      $NetBSD: xenevt.c,v 1.30.2.3 2010/10/24 22:48:23 jym Exp $      */

/*
 * Copyright (c) 2005 Manuel Bouyer.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xenevt.c,v 1.30.2.3 2010/10/24 22:48:23 jym Exp $");

#include "opt_xen.h"
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/intr.h>
#include <sys/kmem.h>
#include <sys/simplelock.h>

#include <uvm/uvm_extern.h>

#include <xen/hypervisor.h>
#include <xen/xenpmap.h>
#include <xen/xenio.h>
#include <xen/xenio3.h>
#include <xen/xen.h>

/*
 * Interface between the event channel and userland.
 * Each process with a xenevt device instance open can regiter events it
 * wants to receive. It will get pending events by read(), eventually blocking
 * until some event is available. Pending events are ack'd by a bitmask
 * write()en to the device. Some special operations (such as events binding)
 * are done though ioctl().
 * Processes get a device instance by opening a cloning device.
 */

void		xenevtattach(int);
static int	xenevt_fread(struct file *, off_t *, struct uio *,
    kauth_cred_t, int);
static int	xenevt_fwrite(struct file *, off_t *, struct uio *,
    kauth_cred_t, int);
static int	xenevt_fioctl(struct file *, u_long, void *);
static int	xenevt_fpoll(struct file *, int);
static int	xenevt_fclose(struct file *);
/* static int	xenevt_fkqfilter(struct file *, struct knote *); */

static const struct fileops xenevt_fileops = {
	.fo_read = xenevt_fread,
	.fo_write = xenevt_fwrite,
	.fo_ioctl = xenevt_fioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = xenevt_fpoll,
	.fo_stat = fbadop_stat,
	.fo_close = xenevt_fclose,
	.fo_kqfilter = /* xenevt_fkqfilter */ fnullop_kqfilter,
	.fo_restart = fnullop_restart,
};

dev_type_open(xenevtopen);
dev_type_read(xenevtread);
dev_type_mmap(xenevtmmap);
const struct cdevsw xenevt_cdevsw = {
	xenevtopen, nullclose, xenevtread, nowrite, noioctl,
	nostop, notty, nopoll, xenevtmmap, nokqfilter, D_OTHER
};

/* minor numbers */
#define DEV_EVT 0
#define DEV_XSD 1

/* per-instance datas */
#define XENEVT_RING_SIZE 2048
#define XENEVT_RING_MASK 2047

#define BYTES_PER_PORT (sizeof(evtchn_port_t) / sizeof(uint8_t))

struct xenevt_d {
	struct simplelock lock;
	STAILQ_ENTRY(xenevt_d) pendingq;
	bool pending;
	evtchn_port_t ring[2048]; 
	u_int ring_read; /* pointer of the reader */
	u_int ring_write; /* pointer of the writer */
	u_int flags;
#define XENEVT_F_OVERFLOW 0x01 /* ring overflow */
	struct selinfo sel; /* used by poll */
};

/* event -> user device mapping */
static struct xenevt_d *devevent[NR_EVENT_CHANNELS];

/* pending events */
static void *devevent_sih;
struct simplelock devevent_pending_lock = SIMPLELOCK_INITIALIZER;
STAILQ_HEAD(, xenevt_d) devevent_pending =
    STAILQ_HEAD_INITIALIZER(devevent_pending);

static void xenevt_donotify(struct xenevt_d *);
static void xenevt_record(struct xenevt_d *, evtchn_port_t);

/* pending events */
long xenevt_ev1;
long xenevt_ev2[NR_EVENT_CHANNELS];
static int xenevt_processevt(void *);

/* called at boot time */
void
xenevtattach(int n)
{
	struct intrhand *ih;
	int s;
	int level = IPL_HIGH;
#ifdef MULTIPROCESSOR
	bool mpsafe = (level != IPL_VM);
#endif /* MULTIPROCESSOR */


	devevent_sih = softint_establish(SOFTINT_SERIAL,
	    (void (*)(void *))xenevt_notify, NULL);
	memset(devevent, 0, sizeof(devevent));
	xenevt_ev1 = 0;
	memset(xenevt_ev2, 0, sizeof(xenevt_ev2));

	/* register a handler at splhigh, so that spllower() will call us */
	ih = malloc(sizeof (struct intrhand), M_DEVBUF,
	     M_WAITOK|M_ZERO);
	if (ih == NULL)
		panic("can't allocate xenevt interrupt source");
	ih->ih_level = level;
	ih->ih_fun = ih->ih_realfun = xenevt_processevt;
	ih->ih_arg = ih->ih_realarg = NULL;
	ih->ih_ipl_next = NULL;
#ifdef MULTIPROCESSOR
	if (!mpsafe) {
		ih->ih_fun = intr_biglock_wrapper;
		ih->ih_arg = ih;
	}
#endif /* MULTIPROCESSOR */

	s = splhigh();
	event_set_iplhandler(ih, level);
	splx(s);
}

/* register pending event - always called with interrupt disabled */
void
xenevt_setipending(int l1, int l2)
{
	xenevt_ev1 |= 1UL << l1;
	xenevt_ev2[l1] |= 1UL << l2;
	curcpu()->ci_ipending |= 1 << IPL_HIGH;
}

/* process pending events */
static int
xenevt_processevt(void *v)
{
	long l1, l2;
	int l1i, l2i;
	int port;

	l1 = xen_atomic_xchg(&xenevt_ev1, 0);
	while ((l1i = xen_ffs(l1)) != 0) {
		l1i--;
		l1 &= ~(1UL << l1i);
		l2 = xen_atomic_xchg(&xenevt_ev2[l1i], 0);
		while ((l2i = xen_ffs(l2)) != 0) {
			l2i--;
			l2 &= ~(1UL << l2i);
			port = (l1i << LONG_SHIFT) + l2i;
			xenevt_event(port);
		}
	}

	return 0;
}


/* event callback, called at splhigh() */
void
xenevt_event(int port)
{
	struct xenevt_d *d;

	d = devevent[port];
	if (d != NULL) {
		xenevt_record(d, port);

		if (d->pending) {
			return;
		}

		simple_lock(&devevent_pending_lock);
		STAILQ_INSERT_TAIL(&devevent_pending, d, pendingq);
		simple_unlock(&devevent_pending_lock);
		d->pending = true;
		softint_schedule(devevent_sih);
	}
}

void
xenevt_notify(void)
{

	int s = splhigh();
	simple_lock(&devevent_pending_lock);
	while (/* CONSTCOND */ 1) {
		struct xenevt_d *d;

		d = STAILQ_FIRST(&devevent_pending);
		if (d == NULL) {
			break;
		}
		STAILQ_REMOVE_HEAD(&devevent_pending, pendingq);
		simple_unlock(&devevent_pending_lock);
		splx(s);

		d->pending = false;
		xenevt_donotify(d);

		s = splhigh();
		simple_lock(&devevent_pending_lock);
	}
	simple_unlock(&devevent_pending_lock);
	splx(s);
}

static void
xenevt_donotify(struct xenevt_d *d)
{
	int s;

	s = splsoftserial();
	simple_lock(&d->lock);
	 
	selnotify(&d->sel, 0, 1);
	wakeup(&d->ring_read);

	simple_unlock(&d->lock);
	splx(s);
}

static void
xenevt_record(struct xenevt_d *d, evtchn_port_t port)
{

	/*
	 * This algorithm overflows for one less slot than available.
	 * Not really an issue, and the correct algorithm would be more
	 * complex
	 */

	if (d->ring_read ==
	    ((d->ring_write + 1) & XENEVT_RING_MASK)) {
		d->flags |= XENEVT_F_OVERFLOW;
		printf("xenevt_event: ring overflow port %d\n", port);
	} else {
		d->ring[d->ring_write] = port;
		d->ring_write = (d->ring_write + 1) & XENEVT_RING_MASK;
	}
}

/* open the xenevt device; this is where we clone */
int
xenevtopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct xenevt_d *d;
	struct file *fp;
	int fd, error;

	switch(minor(dev)) {
	case DEV_EVT:
		/* falloc() will use the descriptor for us. */
		if ((error = fd_allocfile(&fp, &fd)) != 0)
			return error;

		d = malloc(sizeof(*d), M_DEVBUF, M_WAITOK | M_ZERO);
		simple_lock_init(&d->lock);
		selinit(&d->sel);
		return fd_clone(fp, fd, flags, &xenevt_fileops, d);
	case DEV_XSD:
		/* no clone for /dev/xsd_kva */
		return (0);
	default:
		break;
	}
	return ENODEV;
}

/* read from device: only for /dev/xsd_kva, xenevt is done though fread */
int
xenevtread(dev_t dev, struct uio *uio, int flags)
{
#define LD_STRLEN 21 /* a 64bit integer needs 20 digits in base10 */
	if (minor(dev) == DEV_XSD) {
		char strbuf[LD_STRLEN], *bf;
		int off, error;
		size_t len;

		off = (int)uio->uio_offset;
		if (off < 0)
			return EINVAL;
		len  = snprintf(strbuf, sizeof(strbuf), "%ld\n",
		    xen_start_info.store_mfn);
		if (off >= len) {
			bf = strbuf;
			len = 0;
		} else {
			bf = &strbuf[off];
			len -= off;
		}
		error = uiomove(bf, len, uio);
		return error;
	}
	return ENODEV;
}

/* mmap: only for xsd_kva */
paddr_t
xenevtmmap(dev_t dev, off_t off, int prot)
{
	if (minor(dev) == DEV_XSD) {
		/* only one page, so off is always 0 */
		if (off != 0)
			return -1;
		return x86_btop(
		   xpmap_mtop((paddr_t)xen_start_info.store_mfn << PAGE_SHIFT));
	}
	return -1;
}

static int
xenevt_fclose(struct file *fp)
{
	struct xenevt_d *d = fp->f_data;
	int i;

	for (i = 0; i < NR_EVENT_CHANNELS; i++ ) {
		if (devevent[i] == d) {
			evtchn_op_t op = { .cmd = 0 };
			int error;

			hypervisor_mask_event(i);
			devevent[i] = NULL;

			op.cmd = EVTCHNOP_close;
			op.u.close.port = i;
			if ((error = HYPERVISOR_event_channel_op(&op))) {
				printf("xenevt_fclose: error %d from "
				    "hypervisor\n", -error);
			}
		}
	}
	seldestroy(&d->sel);
	free(d, M_DEVBUF);
	fp->f_data = NULL;

	return (0);
}

static int
xenevt_fread(struct file *fp, off_t *offp, struct uio *uio,
    kauth_cred_t cred, int flags)
{
	struct xenevt_d *d = fp->f_data;
	int error;
	size_t len, uio_len;
	int ring_read;
	int ring_write;
	int s;

	error = 0;
	s = splsoftserial();
	simple_lock(&d->lock);
	while (error == 0) {
		ring_read = d->ring_read;
		ring_write = d->ring_write;
		if (ring_read != ring_write) {
			break;
		}
		if (d->flags & XENEVT_F_OVERFLOW) {
			break;
		}

		/* nothing to read */
		if (fp->f_flag & FNONBLOCK) {
			error = EAGAIN;
		} else {
			error = ltsleep(&d->ring_read, PRIBIO | PCATCH,
			    "xenevt", 0, &d->lock);
		}
	}
	if (error == 0 && (d->flags & XENEVT_F_OVERFLOW)) {
		error = EFBIG;
	}
	simple_unlock(&d->lock);
	splx(s);

	if (error) {
		return error;
	}

	uio_len = uio->uio_resid / BYTES_PER_PORT;
	if (ring_read <= ring_write)
		len = ring_write - ring_read;
	else
		len = XENEVT_RING_SIZE - ring_read;
	if (len > uio_len)
		len = uio_len;
	error = uiomove(&d->ring[ring_read], len * BYTES_PER_PORT, uio);
	if (error)
		return error;
	ring_read = (ring_read + len) & XENEVT_RING_MASK;
	uio_len = uio->uio_resid / BYTES_PER_PORT;
	if (uio_len == 0)
		goto done;
	/* ring wrapped, read the second part */
	len = ring_write - ring_read;
	if (len > uio_len)
		len = uio_len;
	error = uiomove(&d->ring[ring_read], len * BYTES_PER_PORT, uio);
	if (error)
		return error;
	ring_read = (ring_read + len) & XENEVT_RING_MASK;

done:
	s = splsoftserial();
	simple_lock(&d->lock);
	d->ring_read = ring_read;
	simple_unlock(&d->lock);
	splx(s);

	return 0;
}

static int
xenevt_fwrite(struct file *fp, off_t *offp, struct uio *uio,
    kauth_cred_t cred, int flags)
{
	struct xenevt_d *d = fp->f_data;
	uint16_t *chans;
	int i, nentries, error;

	if (uio->uio_resid == 0)
		return (0);
	nentries = uio->uio_resid / sizeof(uint16_t);
	if (nentries > NR_EVENT_CHANNELS)
		return EMSGSIZE;
	chans = kmem_alloc(nentries * sizeof(uint16_t), KM_SLEEP);
	if (chans == NULL)
		return ENOMEM;
	error = uiomove(chans, uio->uio_resid, uio);
	if (error)
		goto out;
	for (i = 0; i < nentries; i++) {
		if (chans[i] < NR_EVENT_CHANNELS &&
		    devevent[chans[i]] == d) {
			hypervisor_unmask_event(chans[i]);
		}
	}
out:
	kmem_free(chans, nentries * sizeof(uint16_t));
	return 0;
}

static int
xenevt_fioctl(struct file *fp, u_long cmd, void *addr)
{
	struct xenevt_d *d = fp->f_data;
	evtchn_op_t op = { .cmd = 0 };
	int error;

	switch(cmd) {
	case EVTCHN_RESET:
	case IOCTL_EVTCHN_RESET:
		d->ring_read = d->ring_write = 0;
		d->flags = 0;
		break;
	case IOCTL_EVTCHN_BIND_VIRQ:
	{
		struct ioctl_evtchn_bind_virq *bind_virq = addr;
		op.cmd = EVTCHNOP_bind_virq;
		op.u.bind_virq.virq = bind_virq->virq;
		op.u.bind_virq.vcpu = 0;
		if ((error = HYPERVISOR_event_channel_op(&op))) {
			printf("IOCTL_EVTCHN_BIND_VIRQ failed: virq %d error %d\n", bind_virq->virq, error);
			return -error;
		}
		bind_virq->port = op.u.bind_virq.port;
		devevent[bind_virq->port] = d;
		hypervisor_unmask_event(bind_virq->port);
		break;
	}
	case IOCTL_EVTCHN_BIND_INTERDOMAIN:
	{
		struct ioctl_evtchn_bind_interdomain *bind_intd = addr;
		op.cmd = EVTCHNOP_bind_interdomain;
		op.u.bind_interdomain.remote_dom = bind_intd->remote_domain;
		op.u.bind_interdomain.remote_port = bind_intd->remote_port;
		if ((error = HYPERVISOR_event_channel_op(&op)))
			return -error;
		bind_intd->port = op.u.bind_interdomain.local_port;
		devevent[bind_intd->port] = d;
		hypervisor_unmask_event(bind_intd->port);
		break;
	}
	case IOCTL_EVTCHN_BIND_UNBOUND_PORT:
	{
		struct ioctl_evtchn_bind_unbound_port *bind_unbound = addr;
		op.cmd = EVTCHNOP_alloc_unbound;
		op.u.alloc_unbound.dom = DOMID_SELF;
		op.u.alloc_unbound.remote_dom = bind_unbound->remote_domain;
		if ((error = HYPERVISOR_event_channel_op(&op)))
			return -error;
		bind_unbound->port = op.u.alloc_unbound.port;
		devevent[bind_unbound->port] = d;
		hypervisor_unmask_event(bind_unbound->port);
		break;
	}
	case IOCTL_EVTCHN_UNBIND:
	{
		struct ioctl_evtchn_unbind *unbind = addr;
		
		if (unbind->port > NR_EVENT_CHANNELS)
			return EINVAL;
		if (devevent[unbind->port] != d)
			return ENOTCONN;
		devevent[unbind->port] = NULL;
		hypervisor_mask_event(unbind->port);
		op.cmd = EVTCHNOP_close;
		op.u.close.port = unbind->port;
		if ((error = HYPERVISOR_event_channel_op(&op)))
			return -error;
		break;
	}
	case IOCTL_EVTCHN_NOTIFY:
	{
		struct ioctl_evtchn_notify *notify = addr;
		
		if (notify->port > NR_EVENT_CHANNELS)
			return EINVAL;
		if (devevent[notify->port] != d)
			return ENOTCONN;
		hypervisor_notify_via_evtchn(notify->port);
		break;
	}
	case FIONBIO:
		break;
	default:
		return EINVAL;
	}
	return 0;
}

/*      
 * Support for poll() system call  
 *
 * Return true if the specific operation will not block indefinitely.
 */      

static int
xenevt_fpoll(struct file *fp, int events)
{
	struct xenevt_d *d = fp->f_data;
	int revents = events & (POLLOUT | POLLWRNORM); /* we can always write */
	int s;

	s = splsoftserial();
	simple_lock(&d->lock);
	if (events & (POLLIN | POLLRDNORM)) {
		if (d->ring_read != d->ring_write) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			/* Record that someone is waiting */
			selrecord(curlwp, &d->sel);
		}
	}
	simple_unlock(&d->lock);
	splx(s);
	return (revents);
}
