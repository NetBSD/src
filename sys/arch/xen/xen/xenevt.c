/*      $NetBSD: xenevt.c,v 1.65 2022/05/27 18:35:38 bouyer Exp $      */

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
__KERNEL_RCSID(0, "$NetBSD: xenevt.c,v 1.65 2022/05/27 18:35:38 bouyer Exp $");

#include "opt_xen.h"
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
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

#include <uvm/uvm_extern.h>

#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/intr.h>
#ifdef XENPV
#include <xen/xenpmap.h>
#endif
#include <xen/xenio.h>
#include <xen/xenio3.h>
#include <xen/xen.h>

#include "ioconf.h"

/*
 * Interface between the event channel and userland.
 * Each process with a xenevt device instance open can register events it
 * wants to receive. It will get pending events by read(), eventually blocking
 * until some event is available. Pending events are ack'd by a bitmask
 * write()en to the device. Some special operations (such as events binding)
 * are done though ioctl().
 * Processes get a device instance by opening a cloning device.
 */

static int	xenevt_fread(struct file *, off_t *, struct uio *,
    kauth_cred_t, int);
static int	xenevt_fwrite(struct file *, off_t *, struct uio *,
    kauth_cred_t, int);
static int	xenevt_fioctl(struct file *, u_long, void *);
static int	xenevt_fpoll(struct file *, int);
static int	xenevt_fclose(struct file *);
/* static int	xenevt_fkqfilter(struct file *, struct knote *); */

static const struct fileops xenevt_fileops = {
	.fo_name = "xenevt",
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
	.d_open = xenevtopen,
	.d_close = nullclose,
	.d_read = xenevtread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = xenevtmmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

/* minor numbers */
#define DEV_EVT 0
#define DEV_XSD 1

/* per-instance datas */
#define XENEVT_RING_SIZE 2048
#define XENEVT_RING_MASK 2047

#define BYTES_PER_PORT (sizeof(evtchn_port_t) / sizeof(uint8_t))

struct xenevt_d {
	kmutex_t lock;
	kcondvar_t cv;
	STAILQ_ENTRY(xenevt_d) pendingq;
	bool pending;
	evtchn_port_t ring[2048];
	u_int ring_read; /* pointer of the reader */
	u_int ring_write; /* pointer of the writer */
	u_int flags;
#define XENEVT_F_OVERFLOW 0x01 /* ring overflow */
#define XENEVT_F_FREE 0x02 /* free entry */
	struct selinfo sel; /* used by poll */
	struct cpu_info *ci; /* preferred CPU for events for this device */
};

static struct intrhand *xenevt_ih;
static evtchn_port_t xenevt_ev;

/* event -> user device mapping */
static struct xenevt_d *devevent[NR_EVENT_CHANNELS];

/* pending events */
static void *devevent_sih;
static kmutex_t devevent_lock;
static STAILQ_HEAD(, xenevt_d) devevent_pending;

static void xenevt_record(struct xenevt_d *, evtchn_port_t);
static void xenevt_free(struct xenevt_d *);

/* pending events */
long xenevt_ev1;
long xenevt_ev2[NR_EVENT_CHANNELS];
static int xenevt_processevt(void *);

static evtchn_port_t xenevt_alloc_event(void)
{
	evtchn_op_t op;
	op.cmd = EVTCHNOP_alloc_unbound;
	op.u.alloc_unbound.dom = DOMID_SELF;
	op.u.alloc_unbound.remote_dom = DOMID_SELF;
	if (HYPERVISOR_event_channel_op(&op) != 0)
		panic("%s: Failed to allocate loopback event\n", __func__);

	return op.u.alloc_unbound.port;
}

/* called at boot time */
void
xenevtattach(int n)
{
	int level = IPL_HIGH;

	if (!xendomain_is_privileged())
		return;
#ifndef XENPV
	if (vm_guest != VM_GUEST_XENPVH)
		return;
#endif

	mutex_init(&devevent_lock, MUTEX_DEFAULT, IPL_HIGH);
	STAILQ_INIT(&devevent_pending);

	devevent_sih = softint_establish(SOFTINT_SERIAL,
	    (void (*)(void *))xenevt_notify, NULL);
	memset(devevent, 0, sizeof(devevent));
	xenevt_ev1 = 0;
	memset(xenevt_ev2, 0, sizeof(xenevt_ev2));

	/*
	 * Allocate a loopback event port.
	 * It won't be used by itself, but will help registering IPL
	 * handlers.
	 */
	xenevt_ev = xenevt_alloc_event();

	/*
	 * The real objective here is to wiggle into the ih callchain for
	 * IPL level on vCPU 0. (events are bound to vCPU 0 by default).
	 */
	xenevt_ih = event_set_handler(xenevt_ev, xenevt_processevt, NULL,
	    level, NULL, "xenevt", true, &cpu_info_primary);

	KASSERT(xenevt_ih != NULL);
}

/* register pending event - always called with interrupt disabled */
void
xenevt_setipending(int l1, int l2)
{
	KASSERT(curcpu() == xenevt_ih->ih_cpu);
	KASSERT(xenevt_ih->ih_cpu->ci_ilevel >= IPL_HIGH);
	atomic_or_ulong(&xenevt_ev1, 1UL << l1);
	atomic_or_ulong(&xenevt_ev2[l1], 1UL << l2);
	atomic_or_32(&xenevt_ih->ih_cpu->ci_ipending, 1 << SIR_XENIPL_HIGH);
	atomic_add_int(&xenevt_ih->ih_pending, 1);
	evtsource[xenevt_ev]->ev_evcnt.ev_count++;
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

	mutex_enter(&devevent_lock);
	d = devevent[port];
	if (d != NULL) {
		xenevt_record(d, port);

		if (d->pending == false) {
			STAILQ_INSERT_TAIL(&devevent_pending, d, pendingq);
			d->pending = true;
			mutex_exit(&devevent_lock);
			softint_schedule(devevent_sih);
			return;
		}
	}
	mutex_exit(&devevent_lock);
}

void
xenevt_notify(void)
{
	struct xenevt_d *d;

	for (;;) {
		mutex_enter(&devevent_lock);
		d = STAILQ_FIRST(&devevent_pending);
		if (d == NULL) {
			mutex_exit(&devevent_lock);
			break;
		}
		STAILQ_REMOVE_HEAD(&devevent_pending, pendingq);
		d->pending = false;
		mutex_enter(&d->lock);
		if (d->flags & XENEVT_F_FREE) {
			xenevt_free(d);
			mutex_exit(&devevent_lock);
		} else {
			mutex_exit(&devevent_lock);
			selnotify(&d->sel, 0, 1);
			cv_broadcast(&d->cv);
			mutex_exit(&d->lock);
		}
	}
}

static void
xenevt_record(struct xenevt_d *d, evtchn_port_t port)
{

	/*
	 * This algorithm overflows for one less slot than available.
	 * Not really an issue, and the correct algorithm would be more
	 * complex
	 */

	mutex_enter(&d->lock);
	if (d->ring_read ==
	    ((d->ring_write + 1) & XENEVT_RING_MASK)) {
		d->flags |= XENEVT_F_OVERFLOW;
		printf("xenevt_event: ring overflow port %d\n", port);
	} else {
		d->ring[d->ring_write] = port;
		d->ring_write = (d->ring_write + 1) & XENEVT_RING_MASK;
	}
	mutex_exit(&d->lock);
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
		/* falloc() will fill in the descriptor for us. */
		if ((error = fd_allocfile(&fp, &fd)) != 0)
			return error;

		d = kmem_zalloc(sizeof(*d), KM_SLEEP);
		d->ci = xenevt_ih->ih_cpu;
		mutex_init(&d->lock, MUTEX_DEFAULT, IPL_HIGH);
		cv_init(&d->cv, "xenevt");
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
#ifdef XENPV
		return x86_btop(
		   xpmap_mtop((paddr_t)xen_start_info.store_mfn << PAGE_SHIFT));
#else
		return x86_btop(
		   (paddr_t)xen_start_info.store_mfn << PAGE_SHIFT);
#endif
	}
	return -1;
}

static void
xenevt_free(struct xenevt_d *d)
{
	int i;
	KASSERT(mutex_owned(&devevent_lock));
	KASSERT(mutex_owned(&d->lock));

	for (i = 0; i < NR_EVENT_CHANNELS; i++ ) {
		if (devevent[i] == d) {
			evtchn_op_t op = { .cmd = 0 };
			int error;

			hypervisor_mask_event(i);
			xen_atomic_clear_bit(&d->ci->ci_evtmask[0], i);
			devevent[i] = NULL;

			op.cmd = EVTCHNOP_close;
			op.u.close.port = i;
			if ((error = HYPERVISOR_event_channel_op(&op))) {
				printf("xenevt_fclose: error %d from "
				    "hypervisor\n", -error);
			}
		}
	}
	mutex_exit(&d->lock);
	seldestroy(&d->sel);
	cv_destroy(&d->cv);
	mutex_destroy(&d->lock);
	kmem_free(d, sizeof(*d));
}

static int
xenevt_fclose(struct file *fp)
{
	struct xenevt_d *d = fp->f_data;

	mutex_enter(&devevent_lock);
	mutex_enter(&d->lock);
	if (d->pending) {
		d->flags |= XENEVT_F_FREE;
		mutex_exit(&d->lock);
	} else {
		xenevt_free(d);
	}

	mutex_exit(&devevent_lock);
	fp->f_data = NULL;
	return (0);
}

static int
xenevt_fread(struct file *fp, off_t *offp, struct uio *uio,
    kauth_cred_t cred, int flags)
{
	struct xenevt_d *d = fp->f_data;
	int error, ring_read, ring_write;
	size_t len, uio_len;

	error = 0;
	mutex_enter(&d->lock);
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
		if ((fp->f_flag & FNONBLOCK) == 0) {
			error = cv_wait_sig(&d->cv, &d->lock);
		} else {
			error = EAGAIN;
		}
	}
	if (error == 0 && (d->flags & XENEVT_F_OVERFLOW)) {
		error = EFBIG;
	}
	mutex_exit(&d->lock);

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
	mutex_enter(&d->lock);
	d->ring_read = ring_read;
	mutex_exit(&d->lock);

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
	if (nentries >= NR_EVENT_CHANNELS)
		return EMSGSIZE;
	chans = kmem_alloc(nentries * sizeof(uint16_t), KM_SLEEP);
	error = uiomove(chans, uio->uio_resid, uio);
	if (error)
		goto out;
	mutex_enter(&devevent_lock);
	for (i = 0; i < nentries; i++) {
		if (chans[i] < NR_EVENT_CHANNELS &&
		    devevent[chans[i]] == d) {
			hypervisor_unmask_event(chans[i]);
		}
	}
	mutex_exit(&devevent_lock);
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
		mutex_enter(&d->lock);
		d->ring_read = d->ring_write = 0;
		d->flags = 0;
		mutex_exit(&d->lock);
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
		mutex_enter(&devevent_lock);
		KASSERT(devevent[bind_virq->port] == NULL);
		devevent[bind_virq->port] = d;
		mutex_exit(&devevent_lock);
		xen_atomic_set_bit(&d->ci->ci_evtmask[0], bind_virq->port);
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
		mutex_enter(&devevent_lock);
		KASSERT(devevent[bind_intd->port] == NULL);
		devevent[bind_intd->port] = d;
		mutex_exit(&devevent_lock);
		xen_atomic_set_bit(&d->ci->ci_evtmask[0], bind_intd->port);
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
		mutex_enter(&devevent_lock);
		KASSERT(devevent[bind_unbound->port] == NULL);
		devevent[bind_unbound->port] = d;
		mutex_exit(&devevent_lock);
		xen_atomic_set_bit(&d->ci->ci_evtmask[0], bind_unbound->port);
		hypervisor_unmask_event(bind_unbound->port);
		break;
	}
	case IOCTL_EVTCHN_UNBIND:
	{
		struct ioctl_evtchn_unbind *unbind = addr;

		if (unbind->port >= NR_EVENT_CHANNELS)
			return EINVAL;
		mutex_enter(&devevent_lock);
		if (devevent[unbind->port] != d) {
			mutex_exit(&devevent_lock);
			return ENOTCONN;
		}
		devevent[unbind->port] = NULL;
		mutex_exit(&devevent_lock);
		hypervisor_mask_event(unbind->port);
		xen_atomic_clear_bit(&d->ci->ci_evtmask[0], unbind->port);
		op.cmd = EVTCHNOP_close;
		op.u.close.port = unbind->port;
		if ((error = HYPERVISOR_event_channel_op(&op)))
			return -error;
		break;
	}
	case IOCTL_EVTCHN_NOTIFY:
	{
		struct ioctl_evtchn_notify *notify = addr;

		if (notify->port >= NR_EVENT_CHANNELS)
			return EINVAL;
		mutex_enter(&devevent_lock);
		if (devevent[notify->port] != d) {
			mutex_exit(&devevent_lock);
			return ENOTCONN;
		}
		hypervisor_notify_via_evtchn(notify->port);
		mutex_exit(&devevent_lock);
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

	mutex_enter(&d->lock);
	if (events & (POLLIN | POLLRDNORM)) {
		if (d->ring_read != d->ring_write) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			/* Record that someone is waiting */
			selrecord(curlwp, &d->sel);
		}
	}
	mutex_exit(&d->lock);
	return (revents);
}
