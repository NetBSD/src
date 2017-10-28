/*	$NetBSD: usb.c,v 1.168 2017/10/28 00:37:12 pgoyette Exp $	*/

/*
 * Copyright (c) 1998, 2002, 2008, 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology and Matthew R. Green (mrg@eterna.com.au).
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
__KERNEL_RCSID(0, "$NetBSD: usb.c,v 1.168 2017/10/28 00:37:12 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_usb.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
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
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/bus.h>
#include <sys/once.h>
#include <sys/atomic.h>
#include <sys/sysctl.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_verbose.h>
#include <dev/usb/usb_quirks.h>
#include <dev/usb/usbhist.h>

#if defined(USB_DEBUG)

#ifndef USBHIST_SIZE
#define USBHIST_SIZE 50000
#endif

static struct kern_history_ent usbhistbuf[USBHIST_SIZE];
USBHIST_DEFINE(usbhist) = KERNHIST_INITIALIZER(usbhist, usbhistbuf);

#endif

#define USB_DEV_MINOR 255

#ifdef USB_DEBUG
/*
 * 0  - do usual exploration
 * 1  - do not use timeout exploration
 * >1 - do no exploration
 */
int	usb_noexplore = 0;

int	usbdebug = 0;
SYSCTL_SETUP(sysctl_hw_usb_setup, "sysctl hw.usb setup")
{
	int err;
	const struct sysctlnode *rnode;
	const struct sysctlnode *cnode;

	err = sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "usb",
	    SYSCTL_DESCR("usb global controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	if (err)
		goto fail;

	/* control debugging printfs */
	err = sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Enable debugging output"),
	    NULL, 0, &usbdebug, sizeof(usbdebug), CTL_CREATE, CTL_EOL);
	if (err)
		goto fail;

	return;
fail:
	aprint_error("%s: sysctl_createv failed (err = %d)\n", __func__, err);
}
#else
#define	usb_noexplore 0
#endif

#define	DPRINTF(FMT,A,B,C,D)	USBHIST_LOG(usbdebug,FMT,A,B,C,D)
#define	DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(usbdebug,N,FMT,A,B,C,D)

struct usb_softc {
#if 0
	device_t	sc_dev;		/* base device */
#endif
	struct usbd_bus *sc_bus;		/* USB controller */
	struct usbd_port sc_port;	/* dummy port for root hub */

	struct lwp	*sc_event_thread;

	char		sc_dying;
};

struct usb_taskq {
	TAILQ_HEAD(, usb_task) tasks;
	kmutex_t lock;
	kcondvar_t cv;
	struct lwp *task_thread_lwp;
	const char *name;
};

static struct usb_taskq usb_taskq[USB_NUM_TASKQS];

dev_type_open(usbopen);
dev_type_close(usbclose);
dev_type_read(usbread);
dev_type_ioctl(usbioctl);
dev_type_poll(usbpoll);
dev_type_kqfilter(usbkqfilter);

const struct cdevsw usb_cdevsw = {
	.d_open = usbopen,
	.d_close = usbclose,
	.d_read = usbread,
	.d_write = nowrite,
	.d_ioctl = usbioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = usbpoll,
	.d_mmap = nommap,
	.d_kqfilter = usbkqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
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
Static kmutex_t usb_event_lock;
Static kcondvar_t usb_event_cv;
Static proc_t *usb_async_proc;  /* process that wants USB SIGIO */
Static void *usb_async_sih;
Static int usb_dev_open = 0;
Static struct usb_event *usb_alloc_event(void);
Static void usb_free_event(struct usb_event *);
Static void usb_add_event(int, struct usb_event *);
Static int usb_get_next_event(struct usb_event *);
Static void usb_async_intr(void *);
Static void usb_soft_intr(void *);

#ifdef COMPAT_30
Static void usb_copy_old_devinfo(struct usb_device_info_old *, const struct usb_device_info *);
#endif

Static const char *usbrev_str[] = USBREV_STR;

static int usb_match(device_t, cfdata_t, void *);
static void usb_attach(device_t, device_t, void *);
static int usb_detach(device_t, int);
static int usb_activate(device_t, enum devact);
static void usb_childdet(device_t, device_t);
static int usb_once_init(void);
static void usb_doattach(device_t);

extern struct cfdriver usb_cd;

CFATTACH_DECL3_NEW(usb, sizeof(struct usb_softc),
    usb_match, usb_attach, usb_detach, usb_activate, NULL, usb_childdet,
    DVF_DETACH_SHUTDOWN);

static const char *taskq_names[] = USB_TASKQ_NAMES;

int
usb_match(device_t parent, cfdata_t match, void *aux)
{
	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	return UMATCH_GENERIC;
}

void
usb_attach(device_t parent, device_t self, void *aux)
{
	static ONCE_DECL(init_control);
	struct usb_softc *sc = device_private(self);
	int usbrev;

	sc->sc_bus = aux;
	usbrev = sc->sc_bus->ub_revision;

	aprint_naive("\n");
	aprint_normal(": USB revision %s", usbrev_str[usbrev]);
	switch (usbrev) {
	case USBREV_1_0:
	case USBREV_1_1:
	case USBREV_2_0:
	case USBREV_3_0:
		break;
	default:
		aprint_error(", not supported\n");
		sc->sc_dying = 1;
		return;
	}
	aprint_normal("\n");

	/* XXX we should have our own level */
	sc->sc_bus->ub_soft = softint_establish(SOFTINT_USB | SOFTINT_MPSAFE,
	    usb_soft_intr, sc->sc_bus);
	if (sc->sc_bus->ub_soft == NULL) {
		aprint_error("%s: can't register softintr\n",
			     device_xname(self));
		sc->sc_dying = 1;
		return;
	}

	sc->sc_bus->ub_methods->ubm_getlock(sc->sc_bus, &sc->sc_bus->ub_lock);
	KASSERT(sc->sc_bus->ub_lock != NULL);

	RUN_ONCE(&init_control, usb_once_init);
	config_interrupts(self, usb_doattach);
}

static int
usb_once_init(void)
{
	struct usb_taskq *taskq;
	int i;

	USBHIST_LINK_STATIC(usbhist);

	selinit(&usb_selevent);
	mutex_init(&usb_event_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&usb_event_cv, "usbrea");

	for (i = 0; i < USB_NUM_TASKQS; i++) {
		taskq = &usb_taskq[i];

		TAILQ_INIT(&taskq->tasks);
		/*
		 * Since USB task methods usb_{add,rem}_task are callable
		 * from any context, we have to make this lock a spinlock.
		 */
		mutex_init(&taskq->lock, MUTEX_DEFAULT, IPL_USB);
		cv_init(&taskq->cv, "usbtsk");
		taskq->name = taskq_names[i];
		if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
		    usb_task_thread, taskq, &taskq->task_thread_lwp,
		    "%s", taskq->name)) {
			printf("unable to create task thread: %s\n", taskq->name);
			panic("usb_create_event_thread task");
		}
		/*
		 * XXX we should make sure these threads are alive before
		 * end up using them in usb_doattach().
		 */
	}
	return 0;
}

static void
usb_doattach(device_t self)
{
	struct usb_softc *sc = device_private(self);
	struct usbd_device *dev;
	usbd_status err;
	int speed;
	struct usb_event *ue;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	sc->sc_bus->ub_usbctl = self;
	sc->sc_port.up_power = USB_MAX_POWER;

	switch (sc->sc_bus->ub_revision) {
	case USBREV_1_0:
	case USBREV_1_1:
		speed = USB_SPEED_FULL;
		break;
	case USBREV_2_0:
		speed = USB_SPEED_HIGH;
		break;
	case USBREV_3_0:
		speed = USB_SPEED_SUPER;
		break;
	default:
		panic("usb_doattach");
	}

	cv_init(&sc->sc_bus->ub_needsexplore_cv, "usbevt");

	ue = usb_alloc_event();
	ue->u.ue_ctrlr.ue_bus = device_unit(self);
	usb_add_event(USB_EVENT_CTRLR_ATTACH, ue);

	err = usbd_new_device(self, sc->sc_bus, 0, speed, 0,
		  &sc->sc_port);
	if (!err) {
		dev = sc->sc_port.up_dev;
		if (dev->ud_hub == NULL) {
			sc->sc_dying = 1;
			aprint_error("%s: root device is not a hub\n",
				     device_xname(self));
			return;
		}
		sc->sc_bus->ub_roothub = dev;
		usb_create_event_thread(self);
#if 1
		/*
		 * Turning this code off will delay attachment of USB devices
		 * until the USB event thread is running, which means that
		 * the keyboard will not work until after cold boot.
		 */
		if (cold && (device_cfdata(self)->cf_flags & 1))
			dev->ud_hub->uh_explore(sc->sc_bus->ub_roothub);
#endif
	} else {
		aprint_error("%s: root hub problem, error=%s\n",
			     device_xname(self), usbd_errstr(err));
		sc->sc_dying = 1;
	}

	config_pending_incr(self);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	usb_async_sih = softint_establish(SOFTINT_CLOCK | SOFTINT_MPSAFE,
	   usb_async_intr, NULL);

	return;
}

void
usb_create_event_thread(device_t self)
{
	struct usb_softc *sc = device_private(self);

	if (kthread_create(PRI_NONE, KTHREAD_MPSAFE, NULL,
	    usb_event_thread, sc, &sc->sc_event_thread,
	    "%s", device_xname(self))) {
		printf("%s: unable to create event thread for\n",
		       device_xname(self));
		panic("usb_create_event_thread");
	}
}

/*
 * Add a task to be performed by the task thread.  This function can be
 * called from any context and the task will be executed in a process
 * context ASAP.
 */
void
usb_add_task(struct usbd_device *dev, struct usb_task *task, int queue)
{
	struct usb_taskq *taskq;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	KASSERT(0 <= queue);
	KASSERT(queue < USB_NUM_TASKQS);
	taskq = &usb_taskq[queue];
	mutex_enter(&taskq->lock);
	if (atomic_cas_uint(&task->queue, USB_NUM_TASKQS, queue) ==
	    USB_NUM_TASKQS) {
		DPRINTFN(2, "task=%#jx", (uintptr_t)task, 0, 0, 0);
		TAILQ_INSERT_TAIL(&taskq->tasks, task, next);
		cv_signal(&taskq->cv);
	} else {
		DPRINTFN(2, "task=%#jx on q", (uintptr_t)task, 0, 0, 0);
	}
	mutex_exit(&taskq->lock);
}

/*
 * XXX This does not wait for completion!  Most uses need such an
 * operation.  Urgh...
 */
void
usb_rem_task(struct usbd_device *dev, struct usb_task *task)
{
	unsigned queue;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	while ((queue = task->queue) != USB_NUM_TASKQS) {
		struct usb_taskq *taskq = &usb_taskq[queue];
		mutex_enter(&taskq->lock);
		if (__predict_true(task->queue == queue)) {
			TAILQ_REMOVE(&taskq->tasks, task, next);
			task->queue = USB_NUM_TASKQS;
			mutex_exit(&taskq->lock);
			break;
		}
		mutex_exit(&taskq->lock);
	}
}

void
usb_event_thread(void *arg)
{
	struct usb_softc *sc = arg;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

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
	mutex_enter(sc->sc_bus->ub_lock);
	sc->sc_bus->ub_needsexplore = 1;
	usb_discover(sc);
	mutex_exit(sc->sc_bus->ub_lock);
	config_pending_decr(sc->sc_bus->ub_usbctl);

	mutex_enter(sc->sc_bus->ub_lock);
	while (!sc->sc_dying) {
		if (usb_noexplore < 2)
			usb_discover(sc);

		cv_timedwait(&sc->sc_bus->ub_needsexplore_cv,
		    sc->sc_bus->ub_lock, usb_noexplore ? 0 : hz * 60);

		DPRINTFN(2, "sc %#jx woke up", (uintptr_t)sc, 0, 0, 0);
	}
	sc->sc_event_thread = NULL;

	/* In case parent is waiting for us to exit. */
	cv_signal(&sc->sc_bus->ub_needsexplore_cv);
	mutex_exit(sc->sc_bus->ub_lock);

	DPRINTF("sc %#jx exit", (uintptr_t)sc, 0, 0, 0);
	kthread_exit(0);
}

void
usb_task_thread(void *arg)
{
	struct usb_task *task;
	struct usb_taskq *taskq;
	bool mpsafe;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	taskq = arg;
	DPRINTF("start taskq %#jx", (uintptr_t)taskq, 0, 0, 0);

	mutex_enter(&taskq->lock);
	for (;;) {
		task = TAILQ_FIRST(&taskq->tasks);
		if (task == NULL) {
			cv_wait(&taskq->cv, &taskq->lock);
			task = TAILQ_FIRST(&taskq->tasks);
		}
		DPRINTFN(2, "woke up task=%#jx", (uintptr_t)task, 0, 0, 0);
		if (task != NULL) {
			mpsafe = ISSET(task->flags, USB_TASKQ_MPSAFE);
			TAILQ_REMOVE(&taskq->tasks, task, next);
			task->queue = USB_NUM_TASKQS;
			mutex_exit(&taskq->lock);

			if (!mpsafe)
				KERNEL_LOCK(1, curlwp);
			task->fun(task->arg);
			/* Can't dereference task after this point.  */
			if (!mpsafe)
				KERNEL_UNLOCK_ONE(curlwp);

			mutex_enter(&taskq->lock);
		}
	}
	mutex_exit(&taskq->lock);
}

int
usbctlprint(void *aux, const char *pnp)
{
	/* only "usb"es can attach to host controllers */
	if (pnp)
		aprint_normal("usb at %s", pnp);

	return UNCONF;
}

int
usbopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	int unit = minor(dev);
	struct usb_softc *sc;

	if (unit == USB_DEV_MINOR) {
		if (usb_dev_open)
			return EBUSY;
		usb_dev_open = 1;
		mutex_enter(proc_lock);
		usb_async_proc = 0;
		mutex_exit(proc_lock);
		return 0;
	}

	sc = device_lookup_private(&usb_cd, unit);
	if (!sc)
		return ENXIO;

	if (sc->sc_dying)
		return EIO;

	return 0;
}

int
usbread(dev_t dev, struct uio *uio, int flag)
{
	struct usb_event *ue;
#ifdef COMPAT_30
	struct usb_event_old *ueo = NULL;	/* XXXGCC */
	int useold = 0;
#endif
	int error, n;

	if (minor(dev) != USB_DEV_MINOR)
		return ENXIO;

	switch (uio->uio_resid) {
#ifdef COMPAT_30
	case sizeof(struct usb_event_old):
		ueo = kmem_zalloc(sizeof(struct usb_event_old), KM_SLEEP);
		useold = 1;
		/* FALLTHRU */
#endif
	case sizeof(struct usb_event):
		ue = usb_alloc_event();
		break;
	default:
		return EINVAL;
	}

	error = 0;
	mutex_enter(&usb_event_lock);
	for (;;) {
		n = usb_get_next_event(ue);
		if (n != 0)
			break;
		if (flag & IO_NDELAY) {
			error = EWOULDBLOCK;
			break;
		}
		error = cv_wait_sig(&usb_event_cv, &usb_event_lock);
		if (error)
			break;
	}
	mutex_exit(&usb_event_lock);
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

			error = uiomove((void *)ueo, sizeof(*ueo), uio);
		} else
#endif
			error = uiomove((void *)ue, sizeof(*ue), uio);
	}
	usb_free_event(ue);
#ifdef COMPAT_30
	if (useold)
		kmem_free(ueo, sizeof(struct usb_event_old));
#endif

	return error;
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

	return 0;
}

int
usbioctl(dev_t devt, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct usb_softc *sc;
	int unit = minor(devt);

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	if (unit == USB_DEV_MINOR) {
		switch (cmd) {
		case FIONBIO:
			/* All handled in the upper FS layer. */
			return 0;

		case FIOASYNC:
			mutex_enter(proc_lock);
			if (*(int *)data)
				usb_async_proc = l->l_proc;
			else
				usb_async_proc = 0;
			mutex_exit(proc_lock);
			return 0;

		default:
			return EINVAL;
		}
	}

	sc = device_lookup_private(&usb_cd, unit);

	if (sc->sc_dying)
		return EIO;

	int error = 0;
	DPRINTF("cmd %#jx", cmd, 0, 0, 0);
	switch (cmd) {
#ifdef USB_DEBUG
	case USB_SETDEBUG:
		if (!(flag & FWRITE))
			return EBADF;
		usbdebug  = ((*(int *)data) & 0x000000ff);
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

		if (!(flag & FWRITE)) {
			error = EBADF;
			goto fail;
		}

		DPRINTF("USB_REQUEST addr=%jd len=%jd", addr, len, 0, 0);
		if (len < 0 || len > 32768) {
			error = EINVAL;
			goto fail;
		}
		if (addr < 0 || addr >= USB_MAX_DEVICES) {
			error = EINVAL;
			goto fail;
		}
		size_t dindex = usb_addr2dindex(addr);
		if (sc->sc_bus->ub_devices[dindex] == NULL) {
			error = EINVAL;
			goto fail;
		}
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
			ptr = kmem_alloc(len, KM_SLEEP);
			if (uio.uio_rw == UIO_WRITE) {
				error = uiomove(ptr, len, &uio);
				if (error)
					goto ret;
			}
		}
		err = usbd_do_request_flags(sc->sc_bus->ub_devices[dindex],
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
		if (ptr) {
			len = UGETW(ur->ucr_request.wLength);
			kmem_free(ptr, len);
		}
		break;
	}

	case USB_DEVICEINFO:
	{
		struct usbd_device *dev;
		struct usb_device_info *di = (void *)data;
		int addr = di->udi_addr;

		if (addr < 0 || addr >= USB_MAX_DEVICES) {
			error = EINVAL;
			goto fail;
		}
		size_t dindex = usb_addr2dindex(addr);
		if ((dev = sc->sc_bus->ub_devices[dindex]) == NULL) {
			error = ENXIO;
			goto fail;
		}
		usbd_fill_deviceinfo(dev, di, 1);
		break;
	}

#ifdef COMPAT_30
	case USB_DEVICEINFO_OLD:
	{
		struct usbd_device *dev;
		struct usb_device_info_old *di = (void *)data;
		int addr = di->udi_addr;

		if (addr < 1 || addr >= USB_MAX_DEVICES) {
			error = EINVAL;
			goto fail;
		}
		size_t dindex = usb_addr2dindex(addr);
		if ((dev = sc->sc_bus->ub_devices[dindex]) == NULL) {
			error = ENXIO;
			goto fail;
		}
		usbd_fill_deviceinfo_old(dev, di, 1);
		break;
	}
#endif

	case USB_DEVICESTATS:
		*(struct usb_device_stats *)data = sc->sc_bus->ub_stats;
		break;

	default:
		error = EINVAL;
	}

fail:

	DPRINTF("... done (error = %jd)", error, 0, 0, 0);

	return error;
}

int
usbpoll(dev_t dev, int events, struct lwp *l)
{
	int revents, mask;

	if (minor(dev) == USB_DEV_MINOR) {
		revents = 0;
		mask = POLLIN | POLLRDNORM;

		mutex_enter(&usb_event_lock);
		if (events & mask && usb_nevents > 0)
			revents |= events & mask;
		if (revents == 0 && events & mask)
			selrecord(l, &usb_selevent);
		mutex_exit(&usb_event_lock);

		return revents;
	} else {
		return 0;
	}
}

static void
filt_usbrdetach(struct knote *kn)
{

	mutex_enter(&usb_event_lock);
	SLIST_REMOVE(&usb_selevent.sel_klist, kn, knote, kn_selnext);
	mutex_exit(&usb_event_lock);
}

static int
filt_usbread(struct knote *kn, long hint)
{

	if (usb_nevents == 0)
		return 0;

	kn->kn_data = sizeof(struct usb_event);
	return 1;
}

static const struct filterops usbread_filtops = {
	.f_isfd = 1,
	.f_attach = NULL,
	.f_detach = filt_usbrdetach,
	.f_event = filt_usbread,
};

int
usbkqfilter(dev_t dev, struct knote *kn)
{
	struct klist *klist;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		if (minor(dev) != USB_DEV_MINOR)
			return 1;
		klist = &usb_selevent.sel_klist;
		kn->kn_fop = &usbread_filtops;
		break;

	default:
		return EINVAL;
	}

	kn->kn_hook = NULL;

	mutex_enter(&usb_event_lock);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	mutex_exit(&usb_event_lock);

	return 0;
}

/* Explore device tree from the root. */
Static void
usb_discover(struct usb_softc *sc)
{

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	KASSERT(mutex_owned(sc->sc_bus->ub_lock));

	if (usb_noexplore > 1)
		return;
	/*
	 * We need mutual exclusion while traversing the device tree,
	 * but this is guaranteed since this function is only called
	 * from the event thread for the controller.
	 *
	 * Also, we now have sc_bus->ub_lock held.
	 */
	while (sc->sc_bus->ub_needsexplore && !sc->sc_dying) {
		sc->sc_bus->ub_needsexplore = 0;
		mutex_exit(sc->sc_bus->ub_lock);
		sc->sc_bus->ub_roothub->ud_hub->uh_explore(sc->sc_bus->ub_roothub);
		mutex_enter(sc->sc_bus->ub_lock);
	}
}

void
usb_needs_explore(struct usbd_device *dev)
{

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	mutex_enter(dev->ud_bus->ub_lock);
	dev->ud_bus->ub_needsexplore = 1;
	cv_signal(&dev->ud_bus->ub_needsexplore_cv);
	mutex_exit(dev->ud_bus->ub_lock);
}

void
usb_needs_reattach(struct usbd_device *dev)
{

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	mutex_enter(dev->ud_bus->ub_lock);
	dev->ud_powersrc->up_reattach = 1;
	dev->ud_bus->ub_needsexplore = 1;
	cv_signal(&dev->ud_bus->ub_needsexplore_cv);
	mutex_exit(dev->ud_bus->ub_lock);
}

/* Called at with usb_event_lock held. */
int
usb_get_next_event(struct usb_event *ue)
{
	struct usb_event_q *ueq;

	KASSERT(mutex_owned(&usb_event_lock));

	if (usb_nevents <= 0)
		return 0;
	ueq = SIMPLEQ_FIRST(&usb_events);
#ifdef DIAGNOSTIC
	if (ueq == NULL) {
		printf("usb: usb_nevents got out of sync! %d\n", usb_nevents);
		usb_nevents = 0;
		return 0;
	}
#endif
	if (ue)
		*ue = ueq->ue;
	SIMPLEQ_REMOVE_HEAD(&usb_events, next);
	usb_free_event((struct usb_event *)(void *)ueq);
	usb_nevents--;
	return 1;
}

void
usbd_add_dev_event(int type, struct usbd_device *udev)
{
	struct usb_event *ue = usb_alloc_event();

	usbd_fill_deviceinfo(udev, &ue->u.ue_device, false);
	usb_add_event(type, ue);
}

void
usbd_add_drv_event(int type, struct usbd_device *udev, device_t dev)
{
	struct usb_event *ue = usb_alloc_event();

	ue->u.ue_driver.ue_cookie = udev->ud_cookie;
	strncpy(ue->u.ue_driver.ue_devname, device_xname(dev),
	    sizeof(ue->u.ue_driver.ue_devname));
	usb_add_event(type, ue);
}

Static struct usb_event *
usb_alloc_event(void)
{
	/* Yes, this is right; we allocate enough so that we can use it later */
	return kmem_zalloc(sizeof(struct usb_event_q), KM_SLEEP);
}

Static void
usb_free_event(struct usb_event *uep)
{
	kmem_free(uep, sizeof(struct usb_event_q));
}

Static void
usb_add_event(int type, struct usb_event *uep)
{
	struct usb_event_q *ueq;
	struct timeval thetime;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	microtime(&thetime);
	/* Don't want to wait here with usb_event_lock held */
	ueq = (struct usb_event_q *)(void *)uep;
	ueq->ue = *uep;
	ueq->ue.ue_type = type;
	TIMEVAL_TO_TIMESPEC(&thetime, &ueq->ue.ue_time);

	mutex_enter(&usb_event_lock);
	if (++usb_nevents >= USB_MAX_EVENTS) {
		/* Too many queued events, drop an old one. */
		DPRINTF("event dropped", 0, 0, 0, 0);
		(void)usb_get_next_event(0);
	}
	SIMPLEQ_INSERT_TAIL(&usb_events, ueq, next);
	cv_signal(&usb_event_cv);
	selnotify(&usb_selevent, 0, 0);
	if (usb_async_proc != NULL) {
		kpreempt_disable();
		softint_schedule(usb_async_sih);
		kpreempt_enable();
	}
	mutex_exit(&usb_event_lock);
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

Static void
usb_soft_intr(void *arg)
{
	struct usbd_bus *bus = arg;

	mutex_enter(bus->ub_lock);
	bus->ub_methods->ubm_softint(bus);
	mutex_exit(bus->ub_lock);
}

void
usb_schedsoftintr(struct usbd_bus *bus)
{

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	DPRINTFN(10, "polling=%jd", bus->ub_usepolling, 0, 0, 0);

	if (bus->ub_usepolling) {
		bus->ub_methods->ubm_softint(bus);
	} else {
		kpreempt_disable();
		softint_schedule(bus->ub_soft);
		kpreempt_enable();
	}
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

	if ((dev = sc->sc_port.up_dev) == NULL || dev->ud_subdevlen == 0)
		return;

	for (i = 0; i < dev->ud_subdevlen; i++)
		if (dev->ud_subdevs[i] == child)
			dev->ud_subdevs[i] = NULL;
}

int
usb_detach(device_t self, int flags)
{
	struct usb_softc *sc = device_private(self);
	struct usb_event *ue;
	int rc;

	USBHIST_FUNC(); USBHIST_CALLED(usbdebug);

	/* Make all devices disconnect. */
	if (sc->sc_port.up_dev != NULL &&
	    (rc = usb_disconnect_port(&sc->sc_port, self, flags)) != 0)
		return rc;

	pmf_device_deregister(self);
	/* Kill off event thread. */
	sc->sc_dying = 1;
	while (sc->sc_event_thread != NULL) {
		mutex_enter(sc->sc_bus->ub_lock);
		cv_signal(&sc->sc_bus->ub_needsexplore_cv);
		cv_timedwait(&sc->sc_bus->ub_needsexplore_cv,
		    sc->sc_bus->ub_lock, hz * 60);
		mutex_exit(sc->sc_bus->ub_lock);
	}
	DPRINTF("event thread dead", 0, 0, 0, 0);

	if (sc->sc_bus->ub_soft != NULL) {
		softint_disestablish(sc->sc_bus->ub_soft);
		sc->sc_bus->ub_soft = NULL;
	}

	ue = usb_alloc_event();
	ue->u.ue_ctrlr.ue_bus = device_unit(self);
	usb_add_event(USB_EVENT_CTRLR_DETACH, ue);

	cv_destroy(&sc->sc_bus->ub_needsexplore_cv);

	return 0;
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
