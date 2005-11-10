/*      $NetBSD: xenevt.c,v 1.2.6.3 2005/11/10 14:00:34 skrll Exp $      */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <machine/hypervisor.h>
#include <machine/xenio.h>
#include <machine/xen.h>

extern struct evcnt softxenevt_evtcnt;

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
static int	xenevt_read(struct file *, off_t *, struct uio *,
    struct ucred *, int);
static int	xenevt_write(struct file *, off_t *, struct uio *,
    struct ucred *, int);
static int	xenevt_ioctl(struct file *, u_long, void *, struct proc *);
static int	xenevt_poll(struct file *, int, struct proc *);
static int	xenevt_close(struct file *, struct proc *);
/* static int	xenevt_kqfilter(struct file *, struct knote *); */

static const struct fileops xenevt_fileops = {
	xenevt_read,
	xenevt_write,
	xenevt_ioctl,
	fnullop_fcntl,
	xenevt_poll,
	fbadop_stat,
	xenevt_close,
	/* xenevt_kqfilter */ fnullop_kqfilter
};

dev_type_open(xenevtopen);
const struct cdevsw xenevt_cdevsw = {
	xenevtopen, noclose, noread, nowrite, noioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

/* per-instance datas */
#define XENEVT_RING_SIZE 2048
#define XENEVT_RING_MASK 2047
struct xenevt_d {
	struct simplelock lock;
	STAILQ_ENTRY(xenevt_d) pendingq;
	boolean_t pending;
	u_int16_t ring[2048]; 
	u_int ring_read; /* pointer of the reader */
	u_int ring_write; /* pointer of the writer */
	u_int flags;
#define XENEVT_F_OVERFLOW 0x01 /* ring overflow */
	struct selinfo sel; /* used by poll */
};

/* event -> user device mapping */
static struct xenevt_d *devevent[NR_EVENT_CHANNELS];

/* pending events */
struct simplelock devevent_pending_lock = SIMPLELOCK_INITIALIZER;
STAILQ_HEAD(, xenevt_d) devevent_pending =
    STAILQ_HEAD_INITIALIZER(devevent_pending);

static void xenevt_donotify(struct xenevt_d *);
static void xenevt_record(struct xenevt_d *, int);

/* called at boot time */
void
xenevtattach(int n)
{
	memset(devevent, 0, sizeof(devevent));
}

/* event callback */
void
xenevt_event(int port)
{
	struct xenevt_d *d;
	struct cpu_info *ci;

	d = devevent[port];
	if (d != NULL) {
		xenevt_record(d, port);

		if (d->pending) {
			return;
		}

		ci = curcpu();

		if (ci->ci_ilevel < IPL_SOFTXENEVT) {
			/* fast and common path */
			softxenevt_evtcnt.ev_count++;
			xenevt_donotify(d);
		} else {
			simple_lock(&devevent_pending_lock);
			STAILQ_INSERT_TAIL(&devevent_pending, d, pendingq);
			simple_unlock(&devevent_pending_lock);
			d->pending = TRUE;
			softintr(SIR_XENEVT);
		}
	}
}

void
xenevt_notify()
{

	cli();
	simple_lock(&devevent_pending_lock);
	while (/* CONSTCOND */ 1) {
		struct xenevt_d *d;

		d = STAILQ_FIRST(&devevent_pending);
		if (d == NULL) {
			break;
		}
		STAILQ_REMOVE_HEAD(&devevent_pending, pendingq);
		simple_unlock(&devevent_pending_lock);
		sti();

		d->pending = FALSE;
		xenevt_donotify(d);

		cli();
		simple_lock(&devevent_pending_lock);
	}
	simple_unlock(&devevent_pending_lock);
	sti();
}

static void
xenevt_donotify(struct xenevt_d *d)
{
	int s;

	s = splsoftxenevt();
	simple_lock(&d->lock);
	 
	selnotify(&d->sel, 1);
	wakeup(&d->ring_read);

	simple_unlock(&d->lock);
	splx(s);
}

static void
xenevt_record(struct xenevt_d *d, int port)
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
xenevtopen(dev_t dev, int flags, int mode, struct proc *p)
{
	struct xenevt_d *d;
	struct file *fp;
	int fd, error;

	/* falloc() will use the descriptor for us. */
	if ((error = falloc(p, &fp, &fd)) != 0)
		return error;

	d = malloc(sizeof(*d), M_DEVBUF, M_WAITOK | M_ZERO);
	simple_lock_init(&d->lock);

	return fdclone(p, fp, fd, flags, &xenevt_fileops, d);
}

static int
xenevt_close(struct file *fp, struct proc *p)
{
	struct xenevt_d *d = fp->f_data;
	int i;

	for (i = 0; i < NR_EVENT_CHANNELS; i++ ) {
		if (devevent[i] == d) {
			hypervisor_mask_event(i);
			devevent[i] = NULL;
		}
	}
	free(d, M_DEVBUF);
	fp->f_data = NULL;

	return (0);
}

static int
xenevt_read(struct file *fp, off_t *offp, struct uio *uio,
    struct ucred *cred, int flags)
{
	struct xenevt_d *d = fp->f_data;
	int error;
	size_t len, uio_len;
	int ring_read;
	int ring_write;
	int s;

	error = 0;
	s = splsoftxenevt();
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

	uio_len = uio->uio_resid >> 1; 
	if (ring_read <= ring_write)
		len = ring_write - ring_read;
	else
		len = XENEVT_RING_SIZE - ring_read;
	if (len > uio_len)
		len = uio_len;
	error = uiomove(&d->ring[ring_read], len << 1, uio);
	if (error)
		return error;
	ring_read = (ring_read + len) & XENEVT_RING_MASK;
	uio_len = uio->uio_resid >> 1; 
	if (uio_len == 0)
		goto done;
	/* ring wrapped, read the second part */
	len = ring_write - ring_read;
	if (len > uio_len)
		len = uio_len;
	error = uiomove(&d->ring[ring_read], len << 1, uio);
	if (error)
		return error;
	ring_read = (ring_read + len) & XENEVT_RING_MASK;

done:
	s = splsoftxenevt();
	simple_lock(&d->lock);
	d->ring_read = ring_read;
	simple_unlock(&d->lock);
	splx(s);

	return 0;
}

static int
xenevt_write(struct file *fp, off_t *offp, struct uio *uio,
    struct ucred *cred, int flags)
{
	struct xenevt_d *d = fp->f_data;
	u_int16_t chans[NR_EVENT_CHANNELS];
	int i, nentries, error;

	if (uio->uio_resid == 0)
		return (0);
	nentries = uio->uio_resid / sizeof(u_int16_t);
	if (nentries > NR_EVENT_CHANNELS)
		return EMSGSIZE;
	error = uiomove(chans, uio->uio_resid, uio);
	if (error)
		return error;
	for (i = 0; i < nentries; i++) {
		if (chans[i] < NR_EVENT_CHANNELS &&
		    devevent[chans[i]] == d) {
			hypervisor_unmask_event(chans[i]);
		}
	}
	return 0;
}

static int
xenevt_ioctl(struct file *fp, u_long cmd, void *addr, struct proc *p)
{
	struct xenevt_d *d = fp->f_data;
	u_int *arg = addr;

	switch(cmd) {
	case EVTCHN_RESET:
		d->ring_read = d->ring_write = 0;
		d->flags = 0;
		break;
	case EVTCHN_BIND:
		if (*arg > NR_EVENT_CHANNELS)
			return EINVAL;
		if (devevent[*arg] != NULL)
			return EISCONN;
		devevent[*arg] = d;
		hypervisor_unmask_event(*arg);
		break;
	case EVTCHN_UNBIND:
		if (*arg > NR_EVENT_CHANNELS)
			return EINVAL;
		if (devevent[*arg] != d)
			return ENOTCONN;
		devevent[*arg] = NULL;
		hypervisor_mask_event(*arg);
		break;
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
xenevt_poll(struct file *fp, int events, struct proc *p)
{
	struct xenevt_d *d = fp->f_data;
	int revents = events & (POLLOUT | POLLWRNORM); /* we can always write */

	if (events & (POLLIN | POLLRDNORM)) {
		if (d->ring_read != d->ring_write) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			/* Record that someone is waiting */
			selrecord(p, &d->sel);
		}
	}
	return (revents);
}
