/*	$NetBSD: usb.c,v 1.119 2009/11/12 20:11:35 dyoung Exp $	*/

/*
 * Copyright (c) 1998, 2002, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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
 * USB specifications and other documentation can be found at
 * http://www.usb.org/developers/docs/ and
 * http://www.usb.org/developers/devclass_docs/
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: usb.c,v 1.119 2009/11/12 20:11:35 dyoung Exp $");

#include "opt_compat_netbsd.h"

#include "ohci.h"
#include "uhci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/vnode.h>
#include <sys/signalvar.h>
#include <sys/intr.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>

#define USB_DEV_MINOR 255

#include <sys/bus.h>

#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_quirks.h>

#ifdef USB_DEBUG
#define DPRINTF(x)	if (usbdebug) logprintf x
#define DPRINTFN(n,x)	if (usbdebug>(n)) logprintf x
int	usbdebug = 0;
#if defined(UHCI_DEBUG) && NUHCI > 0
extern int	uhcidebug;
#endif
#if defined(OHCI_DEBUG) && NOHCI > 0
extern int	ohcidebug;
#endif
/*
 * 0  - do usual exploration
 * 1  - do not use timeout exploration
 * >1 - do no exploration
 */
int	usb_noexplore = 0;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

struct usb_softc {
#if 0
	USBBASEDEVICE	sc_dev;		/* base device */
#endif
	usbd_bus_handle sc_bus;		/* USB controller */
	struct usbd_port sc_port;	/* dummy port for root hub */

	struct lwp	*sc_event_thread;

	char		sc_dying;
};

struct usb_taskq {
	TAILQ_HEAD(, usb_task) tasks;
	struct lwp *task_thread_lwp;
	const char *name;
	int taskcreated;	/* task thread exists. */
};

static struct usb_taskq usb_taskq[USB_NUM_TASKQS];

dev_type_open(usbopen);
dev_type_close(usbclose);
dev_type_read(usbread);
dev_type_ioctl(usbioctl);
dev_type_poll(usbpoll);
dev_type_kqfilter(usbkqfilter);

const struct cdevsw usb_cdevsw = {
	usbopen, usbclose, usbread, nowrite, usbioctl,
	nostop, notty, usbpoll, nommap, usbkqfilter, D_OTHER,
};

Static void	usb_discover(struct usb_softc *);
Static void	usb_create_event_thread(device_t);
Static void	usb_event_thread(void *);
Static void	usb_task_thread(void *);

#define USB_MAX_EVENTS 100
struct usb_event_q {
	struct usb_event ue;
	SIMPLEQ_ENTRY(usb_event_q) next;
};
Static SIMPLEQ_HEAD(, usb_event_q) usb_events =
	SIMPLEQ_HEAD_INITIALIZER(usb_events);
Static int usb_nevents = 0;
Static struct selinfo usb_selevent;
Static usb_proc_ptr usb_async_proc;  /* process that wants USB SIGIO */
Static void *usb_async_sih;
Static int usb_dev_open = 0;
Static struct usb_event *usb_alloc_event(void);
Static void usb_free_event(struct usb_event *);
Static void usb_add_event(int, struct usb_event *);
Static int usb_get_next_event(struct usb_event *);
Static void usb_async_intr(void *);

#ifdef COMPAT_30
Static void usb_copy_old_devinfo(struct usb_device_info_old *, const struct usb_device_info *);
#endif

Static const char *usbrev_str[] = USBREV_STR;

static int usb_match(device_t, cfdata_t, void *);
static void usb_attach(device_t, device_t, void *);
static int usb_detach(device_t, int);
static int usb_activate(device_t, enum devact);
static void usb_childdet(device_t, device_t);
static void usb_doattach(device_t);

extern struct cfdriver usb_cd;

CFATTACH_DECL3_NEW(usb, sizeof(struct usb_softc),
    usb_match, usb_attach, usb_detach, usb_activate, NULL, usb_childdet,
    DVF_DETACH_SHUTDOWN);

int
usb_match(device_t parent, cfdata_t match, void *aux)
{
	DPRINTF(("usbd_match\n"));
	return (UMATCH_GENERIC);
}

void
usb_attach(device_t parent, device_t self, void *aux)
{
	struct usb_softc *sc = device_private(self);
	int usbrev;

	sc->sc_bus = aux;
	usbrev = sc->sc_bus->usbrev;

	aprint_naive("\n");
	aprint_normal(": USB revision %s", usbrev_str[usbrev]);
	switch (usbrev) {
	case USBREV_1_0:
	case USBREV_1_1:
	case USBREV_2_0:
		break;
	default:
		aprint_error(", not supported\n");
		sc->sc_dying = 1;
		USB_ATTACH_ERROR_RETURN;
	}
	aprint_normal("\n");

	config_interrupts(self, usb_doattach);
}

static void
usb_doattach(device_t self)
{
	static bool usb_selevent_init;	/* XXX */
	struct usb_softc *sc = device_private(self);
	usbd_device_handle dev;
	usbd_status err;
	int speed;
	struct usb_event *ue;

	if (!usb_selevent_init) {
		selinit(&usb_selevent);
		usb_selevent_init = true;
	}
	DPRINTF(("usbd_doattach\n"));

	sc->sc_bus->usbctl = self;
	sc->sc_port.power = USB_MAX_POWER;

	switch (sc->sc_bus->usbrev) {
	case USBREV_1_0:
	case USBREV_1_1:
		speed = USB_SPEED_FULL;
		break;
	case USBREV_2_0:
		speed = USB_SPEED_HIGH;
		break;
	default:
		panic("usb_doattach");
	}

	ue = usb_alloc_event();
	ue->u.ue_ctrlr.ue_bus = device_unit(self);
	usb_add_event(USB_EVENT_CTRLR_ATTACH, ue);

#ifdef USB_USE_SOFTINTR
	/* XXX we should have our own level */
	sc->sc_bus->soft = softint_establish(SOFTINT_NET,
	    sc->sc_bus->methods->soft_intr, sc->sc_bus);
	if (sc->sc_bus->soft == NULL) {
		aprint_error("%s: can't register softintr\n",
			     device_xname(self));
		sc->sc_dying = 1;
		USB_ATTACH_ERROR_RETURN;
	}
#endif

	err = usbd_new_device(self, sc->sc_bus, 0, speed, 0,
		  &sc->sc_port);
	if (!err) {
		dev = sc->sc_port.device;
		if (dev->hub == NULL) {
			sc->sc_dying = 1;
			aprint_error("%s: root device is not a hub\n",
				     device_xname(self));
			USB_ATTACH_ERROR_RETURN;
		}
		sc->sc_bus->root_hub = dev;
#if 1
		/*
		 * Turning this code off will delay attachment of USB devices
		 * until the USB event thread is running, which means that
		 * the keyboard will not work until after cold boot.
		 */
		if (cold && (device_cfdata(self)->cf_flags & 1))
			dev->hub->explore(sc->sc_bus->root_hub);
#endif
	} else {
		aprint_error("%s: root hub problem, error=%d\n",
			     device_xname(self), err);
		sc->sc_dying = 1;
	}

	config_pending_incr();
	usb_create_event_thread(self);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	usb_async_sih = softint_establish(SOFTINT_CLOCK | SOFTINT_MPSAFE,
	   usb_async_intr, NULL);

	USB_ATTACH_SUCCESS_RETURN;
}

static const char *taskq_names[] = USB_TASKQ_NAMES;

void
usb_create_event_thread(device_t self)
{
	struct usb_softc *sc = device_private(self);
	struct usb_taskq *taskq;
	int i;

	if (usb_kthread_create1(PRI_NONE, 0, NULL, usb_event_thread, sc,
			&sc->sc_event_thread, "%s", device_xname(self))) {
		printf("%s: unable to create event thread for\n",
		       device_xname(self));
		panic("usb_create_event_thread");
	}
	for (i = 0; i < USB_NUM_TASKQS; i++) {
		taskq = &usb_taskq[i];

		if (taskq->taskcreated)
			continue;

		TAILQ_INIT(&taskq->tasks);
		taskq->taskcreated = 1;
		taskq->name = taskq_names[i];
		if (usb_kthread_create1(PRI_NONE, 0, NULL, usb_task_thread,
		    taskq, &taskq->task_thread_lwp, taskq->name)) {
			printf("unable to create task thread: %s\n", taskq->name);
			panic("usb_create_event_thread task");
		}
	}
}

/*
 * Add a task to be performed by the task thread.  This function can be
 * called from any context and the task will be executed in a process
 * context ASAP.
 */
void
usb_add_task(usbd_device_handle dev, struct usb_task *task, int queue)
{
	struct usb_taskq *taskq;
	int s;

	taskq = &usb_taskq[queue];
	s = splusb();
	if (task->queue == -1) {
		DPRINTFN(2,("usb_add_task: task=%p\n", task));
		TAILQ_INSERT_TAIL(&taskq->tasks, task, next);
		task->queue = queue;
	} else {
		DPRINTFN(3,("usb_add_task: task=%p on q\n", task));
	}
	wakeup(&taskq->tasks);
	splx(s);
}

void
usb_rem_task(usbd_device_handle dev, struct usb_task *task)
{
	struct usb_taskq *taskq;
	int s;

	taskq = &usb_taskq[task->queue];
	s = splusb();
	if (task->queue != -1) {
		TAILQ_REMOVE(&taskq->tasks, task, next);
		task->queue = -1;
	}
	splx(s);
}

void
usb_event_thread(void *arg)
{
	struct usb_softc *sc = arg;

	DPRINTF(("usb_event_thread: start\n"));

	/*
	 * In case this controller is a companion controller to an
	 * EHCI controller we need to wait until the EHCI controller
	 * has grabbed the port.
	 * XXX It would be nicer to do this with a tsleep(), but I don't
	 * know how to synchronize the creation of the threads so it
	 * will work.
	 */
	usb_delay_ms(sc->sc_bus, 500);

	/* Make sure first discover does something. */
	sc->sc_bus->needs_explore = 1;
	usb_discover(sc);
	config_pending_decr();

	while (!sc->sc_dying) {
#ifdef USB_DEBUG
		if (usb_noexplore < 2)
#endif
		usb_discover(sc);
#ifdef USB_DEBUG
		(void)tsleep(&sc->sc_bus->needs_explore, PWAIT, "usbevt",
		    usb_noexplore ? 0 : hz * 60);
#else
		(void)tsleep(&sc->sc_bus->needs_explore, PWAIT, "usbevt",
		    hz * 60);
#endif
		DPRINTFN(2,("usb_event_thread: woke up\n"));
	}
	sc->sc_event_thread = NULL;

	/* In case parent is waiting for us to exit. */
	wakeup(sc);

	DPRINTF(("usb_event_thread: exit\n"));
	kthread_exit(0);
}

void
usb_task_thread(void *arg)
{
	struct usb_task *task;
	struct usb_taskq *taskq;
	int s;

	taskq = arg;
	DPRINTF(("usb_task_thread: start taskq %s\n", taskq->name));

	s = splusb();
	for (;;) {
		task = TAILQ_FIRST(&taskq->tasks);
		if (task == NULL) {
			tsleep(&taskq->tasks, PWAIT, "usbtsk", 0);
			task = TAILQ_FIRST(&taskq->tasks);
		}
		DPRINTFN(2,("usb_task_thread: woke up task=%p\n", task));
		if (task != NULL) {
			TAILQ_REMOVE(&taskq->tasks, task, next);
			task->queue = -1;
			splx(s);
			task->fun(task->arg);
			s = splusb();
		}
	}
}

int
usbctlprint(void *aux, const char *pnp)
{
	/* only "usb"es can attach to host controllers */
	if (pnp)
		aprint_normal("usb at %s", pnp);

	return (UNCONF);
}

int
usbopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit = minor(dev);
	struct usb_softc *sc;

	if (unit == USB_DEV_MINOR) {
		if (usb_dev_open)
			return (EBUSY);
		usb_dev_open = 1;
		mutex_enter(proc_lock);
		usb_async_proc = 0;
		mutex_exit(proc_lock);
		return (0);
	}

	sc = device_lookup_private(&usb_cd, unit);
	if (!sc)
		return (ENXIO);

	if (sc->sc_dying)
		return (EIO);

	return (0);
}

int
usbread(dev_t dev, struct uio *uio, int flag)
{
	struct usb_event *ue;
#ifdef COMPAT_30
	struct usb_event_old *ueo = NULL;	/* XXXGCC */
#endif
	int s, error, n, useold;

	if (minor(dev) != USB_DEV_MINOR)
		return (ENXIO);

	useold = 0;
	switch (uio->uio_resid) {
#ifdef COMPAT_30
	case sizeof(struct usb_event_old):
		ueo = malloc(sizeof(struct usb_event_old), M_USBDEV,
			     M_WAITOK|M_ZERO);
		useold = 1;
		/* FALLTHRU */
#endif
	case sizeof(struct usb_event):
		ue = usb_alloc_event();
		break;
	default:
		return (EINVAL);
	}

	error = 0;
	s = splusb();
	for (;;) {
		n = usb_get_next_event(ue);
		if (n != 0)
			break;
		if (flag & IO_NDELAY) {
			error = EWOULDBLOCK;
			break;
		}
		error = tsleep(&usb_events, PZERO | PCATCH, "usbrea", 0);
		if (error)
			break;
	}
	splx(s);
	if (!error) {
#ifdef COMPAT_30
		if (useold) { /* copy fields to old struct */
			ueo->ue_type = ue->ue_type;
			memcpy(&ueo->ue_time, &ue->ue_time,
			      sizeof(struct timespec));
			switch (ue->ue_type) {
				case USB_EVENT_DEVICE_ATTACH:
				case USB_EVENT_DEVICE_DETACH:
					usb_copy_old_devinfo(&ueo->u.ue_device, &ue->u.ue_device);
					break;

				case USB_EVENT_CTRLR_ATTACH:
				case USB_EVENT_CTRLR_DETACH:
					ueo->u.ue_ctrlr.ue_bus=ue->u.ue_ctrlr.ue_bus;
					break;

				case USB_EVENT_DRIVER_ATTACH:
				case USB_EVENT_DRIVER_DETACH:
					ueo->u.ue_driver.ue_cookie=ue->u.ue_driver.ue_cookie;
					memcpy(ueo->u.ue_driver.ue_devname,
					       ue->u.ue_driver.ue_devname,  
					       sizeof(ue->u.ue_driver.ue_devname));
					break;
				default:
					;
			}

			error = uiomove((void *)ueo, sizeof *ueo, uio);
		} else
#endif
			error = uiomove((void *)ue, sizeof *ue, uio);
	}
	usb_free_event(ue);
#ifdef COMPAT_30
	if (useold)
		free(ueo, M_USBDEV);
#endif

	return (error);
}

int
usbclose(dev_t dev, int flag, int mode,
    struct lwp *l)
{
	int unit = minor(dev);

	if (unit == USB_DEV_MINOR) {
		mutex_enter(proc_lock);
		usb_async_proc = 0;
		mutex_exit(proc_lock);
		usb_dev_open = 0;
	}

	return (0);
}

int
usbioctl(dev_t devt, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct usb_softc *sc;
	int unit = minor(devt);

	if (unit == USB_DEV_MINOR) {
		switch (cmd) {
		case FIONBIO:
			/* All handled in the upper FS layer. */
			return (0);

		case FIOASYNC:
			mutex_enter(proc_lock);
			if (*(int *)data)
				usb_async_proc = l->l_proc;
			else
				usb_async_proc = 0;
			mutex_exit(proc_lock);
			return (0);

		default:
			return (EINVAL);
		}
	}

	sc = device_lookup_private(&usb_cd, unit);

	if (sc->sc_dying)
		return (EIO);

	switch (cmd) {
#ifdef USB_DEBUG
	case USB_SETDEBUG:
		if (!(flag & FWRITE))
			return (EBADF);
		usbdebug  = ((*(int *)data) & 0x000000ff);
#if defined(UHCI_DEBUG) && NUHCI > 0
		uhcidebug = ((*(int *)data) & 0x0000ff00) >> 8;
#endif
#if defined(OHCI_DEBUG) && NOHCI > 0
		ohcidebug = ((*(int *)data) & 0x00ff0000) >> 16;
#endif
		break;
#endif /* USB_DEBUG */
	case USB_REQUEST:
	{
		struct usb_ctl_request *ur = (void *)data;
		int len = UGETW(ur->ucr_request.wLength);
		struct iovec iov;
		struct uio uio;
		void *ptr = 0;
		int addr = ur->ucr_addr;
		usbd_status err;
		int error = 0;

		if (!(flag & FWRITE))
			return (EBADF);

		DPRINTF(("usbioctl: USB_REQUEST addr=%d len=%d\n", addr, len));
		if (len < 0 || len > 32768)
			return (EINVAL);
		if (addr < 0 || addr >= USB_MAX_DEVICES ||
		    sc->sc_bus->devices[addr] == 0)
			return (EINVAL);
		if (len != 0) {
			iov.iov_base = (void *)ur->ucr_data;
			iov.iov_len = len;
			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			uio.uio_resid = len;
			uio.uio_offset = 0;
			uio.uio_rw =
				ur->ucr_request.bmRequestType & UT_READ ?
				UIO_READ : UIO_WRITE;
			uio.uio_vmspace = l->l_proc->p_vmspace;
			ptr = malloc(len, M_TEMP, M_WAITOK);
			if (uio.uio_rw == UIO_WRITE) {
				error = uiomove(ptr, len, &uio);
				if (error)
					goto ret;
			}
		}
		err = usbd_do_request_flags(sc->sc_bus->devices[addr],
			  &ur->ucr_request, ptr, ur->ucr_flags, &ur->ucr_actlen,
			  USBD_DEFAULT_TIMEOUT);
		if (err) {
			error = EIO;
			goto ret;
		}
		if (len > ur->ucr_actlen)
			len = ur->ucr_actlen;
		if (len != 0) {
			if (uio.uio_rw == UIO_READ) {
				error = uiomove(ptr, len, &uio);
				if (error)
					goto ret;
			}
		}
	ret:
		if (ptr)
			free(ptr, M_TEMP);
		return (error);
	}

	case USB_DEVICEINFO:
	{
		usbd_device_handle dev;
		struct usb_device_info *di = (void *)data;
		int addr = di->udi_addr;

		if (addr < 1 || addr >= USB_MAX_DEVICES)
			return EINVAL;
		if ((dev = sc->sc_bus->devices[addr]) == NULL)
			return ENXIO;
		usbd_fill_deviceinfo(dev, di, 1);
		break;
	}

#ifdef COMPAT_30
	case USB_DEVICEINFO_OLD:
	{
		usbd_device_handle dev;
		struct usb_device_info_old *di = (void *)data;
		int addr = di->udi_addr;

		if (addr < 1 || addr >= USB_MAX_DEVICES)
			return EINVAL;
		if ((dev = sc->sc_bus->devices[addr]) == NULL)
			return ENXIO;
		usbd_fill_deviceinfo_old(dev, di, 1);
		break;
	}
#endif

	case USB_DEVICESTATS:
		*(struct usb_device_stats *)data = sc->sc_bus->stats;
		break;

	default:
		return (EINVAL);
	}
	return (0);
}

int
usbpoll(dev_t dev, int events, struct lwp *l)
{
	int revents, mask, s;

	if (minor(dev) == USB_DEV_MINOR) {
		revents = 0;
		mask = POLLIN | POLLRDNORM;

		s = splusb();
		if (events & mask && usb_nevents > 0)
			revents |= events & mask;
		if (revents == 0 && events & mask)
			selrecord(l, &usb_selevent);
		splx(s);

		return (revents);
	} else {
		return (0);
	}
}

static void
filt_usbrdetach(struct knote *kn)
{
	int s;

	s = splusb();
	SLIST_REMOVE(&usb_selevent.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_usbread(struct knote *kn, long hint)
{

	if (usb_nevents == 0)
		return (0);

	kn->kn_data = sizeof(struct usb_event);
	return (1);
}

static const struct filterops usbread_filtops =
	{ 1, NULL, filt_usbrdetach, filt_usbread };

int
usbkqfilter(dev_t dev, struct knote *kn)
{
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		if (minor(dev) != USB_DEV_MINOR)
			return (1);
		klist = &usb_selevent.sel_klist;
		kn->kn_fop = &usbread_filtops;
		break;

	default:
		return (EINVAL);
	}

	kn->kn_hook = NULL;

	s = splusb();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}

/* Explore device tree from the root. */
Static void
usb_discover(struct usb_softc *sc)
{

	DPRINTFN(2,("usb_discover\n"));
#ifdef USB_DEBUG
	if (usb_noexplore > 1)
		return;
#endif
	/*
	 * We need mutual exclusion while traversing the device tree,
	 * but this is guaranteed since this function is only called
	 * from the event thread for the controller.
	 */
	while (sc->sc_bus->needs_explore && !sc->sc_dying) {
		sc->sc_bus->needs_explore = 0;
		sc->sc_bus->root_hub->hub->explore(sc->sc_bus->root_hub);
	}
}

void
usb_needs_explore(usbd_device_handle dev)
{
	DPRINTFN(2,("usb_needs_explore\n"));
	dev->bus->needs_explore = 1;
	wakeup(&dev->bus->needs_explore);
}

void
usb_needs_reattach(usbd_device_handle dev)
{
	DPRINTFN(2,("usb_needs_reattach\n"));
	dev->powersrc->reattach = 1;
	dev->bus->needs_explore = 1;
	wakeup(&dev->bus->needs_explore);
}

/* Called at splusb() */
int
usb_get_next_event(struct usb_event *ue)
{
	struct usb_event_q *ueq;

	if (usb_nevents <= 0)
		return (0);
	ueq = SIMPLEQ_FIRST(&usb_events);
#ifdef DIAGNOSTIC
	if (ueq == NULL) {
		printf("usb: usb_nevents got out of sync! %d\n", usb_nevents);
		usb_nevents = 0;
		return (0);
	}
#endif
	if (ue)
		*ue = ueq->ue;
	SIMPLEQ_REMOVE_HEAD(&usb_events, next);
	usb_free_event((struct usb_event *)(void *)ueq);
	usb_nevents--;
	return (1);
}

void
usbd_add_dev_event(int type, usbd_device_handle udev)
{
	struct usb_event *ue = usb_alloc_event();

	usbd_fill_deviceinfo(udev, &ue->u.ue_device, USB_EVENT_IS_ATTACH(type));
	usb_add_event(type, ue);
}

void
usbd_add_drv_event(int type, usbd_device_handle udev, device_t dev)
{
	struct usb_event *ue = usb_alloc_event();

	ue->u.ue_driver.ue_cookie = udev->cookie;
	strncpy(ue->u.ue_driver.ue_devname, USBDEVPTRNAME(dev),
	    sizeof ue->u.ue_driver.ue_devname);
	usb_add_event(type, ue);
}

Static struct usb_event *
usb_alloc_event(void)
{
	/* Yes, this is right; we allocate enough so that we can use it later */
	return malloc(sizeof(struct usb_event_q), M_USBDEV, M_WAITOK|M_ZERO);
}

Static void
usb_free_event(struct usb_event *uep)
{
	free(uep, M_USBDEV);
}

Static void
usb_add_event(int type, struct usb_event *uep)
{
	struct usb_event_q *ueq;
	struct timeval thetime;
	int s;

	microtime(&thetime);
	/* Don't want to wait here inside splusb() */
	ueq = (struct usb_event_q *)(void *)uep;
	ueq->ue = *uep;
	ueq->ue.ue_type = type;
	TIMEVAL_TO_TIMESPEC(&thetime, &ueq->ue.ue_time);

	s = splusb();
	if (++usb_nevents >= USB_MAX_EVENTS) {
		/* Too many queued events, drop an old one. */
		DPRINTFN(-1,("usb: event dropped\n"));
		(void)usb_get_next_event(0);
	}
	SIMPLEQ_INSERT_TAIL(&usb_events, ueq, next);
	wakeup(&usb_events);
	selnotify(&usb_selevent, 0, 0);
	if (usb_async_proc != NULL) {
		softint_schedule(usb_async_sih);
	}
	splx(s);
}

Static void
usb_async_intr(void *cookie)
{
	proc_t *proc;

	mutex_enter(proc_lock);
	if ((proc = usb_async_proc) != NULL)
		psignal(proc, SIGIO);
	mutex_exit(proc_lock);
}

void
usb_schedsoftintr(usbd_bus_handle bus)
{
	DPRINTFN(10,("usb_schedsoftintr: polling=%d\n", bus->use_polling));
#ifdef USB_USE_SOFTINTR
	if (bus->use_polling) {
		bus->methods->soft_intr(bus);
	} else {
		softint_schedule(bus->soft);
	}
#else
	bus->methods->soft_intr(bus);
#endif /* USB_USE_SOFTINTR */
}

int
usb_activate(device_t self, enum devact act)
{
	struct usb_softc *sc = device_private(self);

	switch (act) {
	case DVACT_DEACTIVATE:
		sc->sc_dying = 1;
		return 0;
	default:
		return EOPNOTSUPP;
	}
}

void
usb_childdet(device_t self, device_t child)
{
	int i;
	struct usb_softc *sc = device_private(self);
	struct usbd_device *dev;

	if ((dev = sc->sc_port.device) == NULL || dev->subdevlen == 0)
		return;

	for (i = 0; i < dev->subdevlen; i++)
		if (dev->subdevs[i] == child)
			dev->subdevs[i] = NULL;
}

int
usb_detach(device_t self, int flags)
{
	struct usb_softc *sc = device_private(self);
	struct usb_event *ue;
	int rc;

	DPRINTF(("usb_detach: start\n"));

	/* Make all devices disconnect. */
	if (sc->sc_port.device != NULL &&
	    (rc = usb_disconnect_port(&sc->sc_port, self, flags)) != 0)
		return rc;

	pmf_device_deregister(self);
	/* Kill off event thread. */
	sc->sc_dying = 1;
	while (sc->sc_event_thread != NULL) {
		wakeup(&sc->sc_bus->needs_explore);
		tsleep(sc, PWAIT, "usbdet", hz * 60);
	}
	DPRINTF(("usb_detach: event thread dead\n"));

#ifdef USB_USE_SOFTINTR
	if (sc->sc_bus->soft != NULL) {
		softint_disestablish(sc->sc_bus->soft);
		sc->sc_bus->soft = NULL;
	}
#endif

	ue = usb_alloc_event();
	ue->u.ue_ctrlr.ue_bus = device_unit(self);
	usb_add_event(USB_EVENT_CTRLR_DETACH, ue);

	return (0);
}

#ifdef COMPAT_30
Static void
usb_copy_old_devinfo(struct usb_device_info_old *uo,
		     const struct usb_device_info *ue)
{
	const unsigned char *p;
	unsigned char *q;
	int i, n;

	uo->udi_bus = ue->udi_bus;
	uo->udi_addr = ue->udi_addr;       
	uo->udi_cookie = ue->udi_cookie;
	for (i = 0, p = (const unsigned char *)ue->udi_product,
	     q = (unsigned char *)uo->udi_product;
	     *p && i < USB_MAX_STRING_LEN - 1; p++) {
		if (*p < 0x80)
			q[i++] = *p;
		else {
			q[i++] = '?';
			if ((*p & 0xe0) == 0xe0)
				p++;
			p++;
		}
	}
	q[i] = 0;

	for (i = 0, p = ue->udi_vendor, q = uo->udi_vendor;
	     *p && i < USB_MAX_STRING_LEN - 1; p++) {
		if (* p < 0x80)
			q[i++] = *p;
		else {
			q[i++] = '?';
			p++;
			if ((*p & 0xe0) == 0xe0)
				p++;
		}
	}
	q[i] = 0;

	memcpy(uo->udi_release, ue->udi_release, sizeof(uo->udi_release));

	uo->udi_productNo = ue->udi_productNo;
	uo->udi_vendorNo = ue->udi_vendorNo;
	uo->udi_releaseNo = ue->udi_releaseNo;
	uo->udi_class = ue->udi_class;
	uo->udi_subclass = ue->udi_subclass;
	uo->udi_protocol = ue->udi_protocol;
	uo->udi_config = ue->udi_config;
	uo->udi_speed = ue->udi_speed;
	uo->udi_power = ue->udi_power;    
	uo->udi_nports = ue->udi_nports;

	for (n=0; n<USB_MAX_DEVNAMES; n++)
		memcpy(uo->udi_devnames[n],
		       ue->udi_devnames[n], USB_MAX_DEVNAMELEN);
	memcpy(uo->udi_ports, ue->udi_ports, sizeof(uo->udi_ports));
}
#endif
