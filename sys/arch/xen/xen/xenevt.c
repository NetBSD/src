/*      $NetBSD: xenevt.c,v 1.1.2.1 2005/01/31 17:21:16 bouyer Exp $      */

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
#include <sys/proc.h>
#include <sys/conf.h>

#include <machine/hypervisor.h>
#include <machine/xenio.h>
#include <machine/xen.h>

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
	u_int16_t ring[2048]; 
	u_int ring_read; /* pointer of the reader */
	u_int ring_write; /* pointer of the writer */
	u_int flags;
#define XENEVT_F_OVERFLOW 0x01 /* ring overflow */
};

/* event -> user device mapping */
static struct xenevt_d *devevent[NR_EVENT_CHANNELS];


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
	hypervisor_mask_event(port);
	hypervisor_clear_event(port);
	if (devevent[port] != NULL) {
		/*
		 * This algorithm overflows for one less slot than available.
		 * Not really an issue, and the correct algorithm would be more
		 * complex
		 */
		 
		if (devevent[port]->ring_read ==
		    ((devevent[port]->ring_write + 1) & XENEVT_RING_MASK)) {
			devevent[port]->flags |= XENEVT_F_OVERFLOW;
			return;
		}
		devevent[port]->ring[devevent[port]->ring_write] = port;
		devevent[port]->ring_write =
		    (devevent[port]->ring_write + 1) & XENEVT_RING_MASK;
		wakeup(&devevent[port]->ring_read);
	}
}

/* open the xenevt device; this is where we clone */
int
xenevtopen(dev_t dev, int flag, int mode, struct proc *p)
{
	struct xenevt_d *d;
	struct file *fp;
	int fd, error;

	/* falloc() will use the descriptor for us. */
	if ((error = falloc(p, &fp, &fd)) != 0)
		return error;

	d = malloc(sizeof(*d), M_DEVBUF, M_WAITOK | M_ZERO);

	return fdclone(p, fp, fd, &xenevt_fileops, d);
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

	while (d->ring_read == d->ring_write) {
		if (d->flags & XENEVT_F_OVERFLOW)
			return EFBIG;
		/* nothing to read */
		if (fp->f_flag & FNONBLOCK) {
			return (EWOULDBLOCK);
		}
		error = tsleep(&d->ring_read, PRIBIO | PCATCH, "xenevt", 0);
		if (error == EINTR || error == ERESTART) {
			return(error);
		}
		if (d->ring_read != d->ring_write)
			break;
	}
	if (d->flags & XENEVT_F_OVERFLOW)
		return EFBIG;

	uio_len = uio->uio_resid >> 1; 
	if (d->ring_read <= d->ring_write)
		len = d->ring_write - d->ring_read;
	else
		len = XENEVT_RING_SIZE - d->ring_read;
	if (len > uio_len)
		len = uio_len;
	error = uiomove(&d->ring[d->ring_read], len << 1, uio);
	if (error)
		return error;
	d->ring_read = (d->ring_read + len) & XENEVT_RING_MASK;
	uio_len = uio->uio_resid >> 1; 
	if (uio_len == 0)
		return 0; /* we're done */
	/* ring wrapped, read the second part */
	len = d->ring_write - d->ring_read;
	if (len > uio_len)
		len = uio_len;
	error = uiomove(&d->ring[d->ring_read], len << 1, uio);
	if (error)
		return error;
	d->ring_read = (d->ring_read + len) & XENEVT_RING_MASK;
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
		    devevent[chans[i]] == d)
			hypervisor_unmask_event(chans[i]);
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
	default:
		return EINVAL;
	}
	return 0;
}

/*      
 * Support for poll() system call  
 *
 * Return true iff the specific operation will not block indefinitely.
 */      

static int
xenevt_poll(struct file *fp, int events, struct proc *p)
{
	struct xenevt_d *d = fp->f_data;
	int revents = events & (POLLOUT | POLLWRNORM); /* we can always write */

	if (events & (POLLIN | POLLRDNORM)) {
		if (d->ring_read != d->ring_write) {
			revents |= events & (POLLIN | POLLRDNORM);
		}
	}
	return (revents);
}

