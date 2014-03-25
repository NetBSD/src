/*	$NetBSD: kern_ndis.c,v 1.26 2014/03/25 16:23:58 christos Exp $	*/

/*-
 * Copyright (c) 2003
 *	Bill Paul <wpaul@windriver.com>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/sys/compat/ndis/kern_ndis.c,v 1.60.2.5 2005/04/01 17:14:20 wpaul Exp $");
#endif
#ifdef __NetBSD__
__KERNEL_RCSID(0, "$NetBSD: kern_ndis.c,v 1.26 2014/03/25 16:23:58 christos Exp $");
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/callout.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/conf.h>

#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/mbuf.h>
#include <sys/kthread.h>
#include <sys/bus.h>
#ifdef __FreeBSD__
#include <machine/resource.h>
#include <sys/rman.h>
#endif

#ifdef __NetBSD__
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#endif

#include <net/if.h>
#include <net/if_arp.h>
#ifdef __FreeBSD__
#include <net/ethernet.h>
#else
#include <net/if_ether.h>
#endif
#include <net/if_dl.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_ioctl.h>

#include <compat/ndis/pe_var.h>
#include <compat/ndis/resource_var.h>
#include <compat/ndis/ntoskrnl_var.h>
#include <compat/ndis/ndis_var.h>
#include <compat/ndis/hal_var.h>
#include <compat/ndis/cfg_var.h>
#include <compat/ndis/usbd_var.h>
#include <dev/if_ndis/if_ndisvar.h>

MODULE(MODULE_CLASS_EXEC, ndis, NULL);

#define NDIS_DUMMY_PATH "\\\\some\\bogus\\path"

__stdcall static void ndis_status_func(ndis_handle, ndis_status,
	void *, uint32_t);
__stdcall static void ndis_statusdone_func(ndis_handle);
__stdcall static void ndis_setdone_func(ndis_handle, ndis_status);
__stdcall static void ndis_getdone_func(ndis_handle, ndis_status);
__stdcall static void ndis_resetdone_func(ndis_handle, ndis_status, uint8_t);
__stdcall static void ndis_sendrsrcavail_func(ndis_handle);
__stdcall static void ndis_intrhand(kdpc *, device_object *,
	irp *, struct ndis_softc *);

static image_patch_table kernndis_functbl[] = {
	IMPORT_FUNC(ndis_status_func),
	IMPORT_FUNC(ndis_statusdone_func),
	IMPORT_FUNC(ndis_setdone_func),
	IMPORT_FUNC(ndis_getdone_func),
	IMPORT_FUNC(ndis_resetdone_func),
	IMPORT_FUNC(ndis_sendrsrcavail_func),
	IMPORT_FUNC(ndis_intrhand),

	{ NULL, NULL, NULL }
};

struct nd_head ndis_devhead;

struct ndis_req {
	void			(*nr_func)(void *);
	void			*nr_arg;
	int			nr_exit;
	STAILQ_ENTRY(ndis_req)	link;
	/* just for debugging */
	int 			area;
};

struct ndisproc {
	struct ndisqhead	*np_q;
	struct proc		*np_p;
	int			np_state;
	uint8_t			np_stack[PAGE_SIZE*NDIS_KSTACK_PAGES];
};

static void ndis_return(void *);
static int ndis_create_kthreads(void);
static void ndis_destroy_kthreads(void);
static void ndis_stop_thread(int);
static int ndis_enlarge_thrqueue(int);
static int ndis_shrink_thrqueue(int);
//#ifdef NDIS_LKM
static void ndis_runq(void *);
//#endif

#ifdef __FreeBSD__
static struct mtx ndis_thr_mtx;
#else /* __NetBSD__ */
static kmutex_t ndis_thr_mtx;
#endif

static struct mtx ndis_req_mtx;
static STAILQ_HEAD(ndisqhead, ndis_req) ndis_ttodo;
static struct ndisqhead ndis_itodo;
static struct ndisqhead ndis_free;
static int ndis_jobs = 32;

static struct ndisproc ndis_tproc;
static struct ndisproc ndis_iproc;

/*
 * This allows us to export our symbols to other modules.
 * Note that we call ourselves 'ndisapi' to avoid a namespace
 * collision with if_ndis.ko, which internally calls itself
 * 'ndis.'
 */

#ifdef __FreeBSD__
static int
ndis_modevent(module_t mod, int cmd, void *arg)
{
	int			error = 0;
	image_patch_table	*patch;

	switch (cmd) {
	case MOD_LOAD:
		/* Initialize subsystems */
		windrv_libinit();
		hal_libinit();
		ndis_libinit();
		ntoskrnl_libinit();
#ifdef usbimplemented
		usbd_libinit();
#endif

		patch = kernndis_functbl;
		while (patch->ipt_func != NULL) {
			windrv_wrap((funcptr)patch->ipt_func,
			    (funcptr *)&patch->ipt_wrap);
			patch++;
		}

		ndis_create_kthreads();

		TAILQ_INIT(&ndis_devhead);

		break;
	case MOD_SHUTDOWN:
		/* stop kthreads */
		ndis_destroy_kthreads();
		if (TAILQ_FIRST(&ndis_devhead) == NULL) {
			/* Shut down subsystems */
			hal_libfini();
			ndis_libfini();
			ntoskrnl_libfini();
#ifdef usbimplemented
			usbd_libfini();
#endif
			windrv_libfini();

			patch = kernndis_functbl;
			while (patch->ipt_func != NULL) {
				windrv_unwrap(patch->ipt_wrap);
				patch++;
			}
		}
		break;
	case MOD_UNLOAD:
		/* stop kthreads */
		ndis_destroy_kthreads();

		/* Shut down subsystems */
		hal_libfini();
		ndis_libfini();
		ntoskrnl_libfini();
		usbd_libfini();
		windrv_libfini();

		patch = kernndis_functbl;
		while (patch->ipt_func != NULL) {
			windrv_unwrap(patch->ipt_wrap);
			patch++;
		}

		break;
	default:
		error = EINVAL;
		break;
	}

	return(error);
}
DEV_MODULE(ndisapi, ndis_modevent, NULL);
MODULE_VERSION(ndisapi, 1);
#endif

static int
ndis_modcmd(modcmd_t cmd, void *arg)
{
	int			error = 0;
	image_patch_table	*patch;

	switch (cmd) {
	case MODULE_CMD_INIT:
		/* Initialize subsystems */
		windrv_libinit();
		hal_libinit();
		ndis_libinit();
		ntoskrnl_libinit();
#ifdef usbimplemented
		usbd_libinit();
#endif

		patch = kernndis_functbl;
		while (patch->ipt_func != NULL) {
			windrv_wrap((funcptr)patch->ipt_func,
			    (funcptr *)&patch->ipt_wrap);
			patch++;
		}

		TAILQ_INIT(&ndis_devhead);

		ndis_create_kthreads();
		break;

	case MODULE_CMD_FINI:
		/* stop kthreads */
		ndis_destroy_kthreads();

		/* Shut down subsystems */
		hal_libfini();
		ndis_libfini();
		ntoskrnl_libfini();
#ifdef usbimplemented		
		usbd_libfini();
#endif		
		windrv_libfini();

		patch = kernndis_functbl;
		while (patch->ipt_func != NULL) {
			windrv_unwrap(patch->ipt_wrap);
			patch++;
		}

		break;

	default:
		error = ENOTTY;
		break;
	}

	return(error);
}

/*
 * We create two kthreads for the NDIS subsystem. One of them is a task
 * queue for performing various odd jobs. The other is an swi thread
 * reserved exclusively for running interrupt handlers. The reason we
 * have our own task queue is that there are some cases where we may
 * need to sleep for a significant amount of time, and if we were to
 * use one of the taskqueue threads, we might delay the processing
 * of other pending tasks which might need to run right away. We have
 * a separate swi thread because we don't want our interrupt handling
 * to be delayed either.
 *
 * By default there are 32 jobs available to start, and another 8
 * are added to the free list each time a new device is created.
 */
 
/* Just for testing this can be removed later */
struct ndis_req *_ndis_taskqueue_req;
struct ndis_req *_ndis_swi_req;
int calling_in_swi = FALSE;
int calling_in_tq  = FALSE;
int num_swi		 = 0;
int num_tq		 = 0;

static void
ndis_runq(void *arg)
{
	struct ndis_req		*r = NULL, *die = NULL;
	struct ndisproc		*p;

	p = arg;

	for (;;) {
		mtx_lock_spin(&ndis_thr_mtx);
		ndis_thsuspend(p->np_p, &ndis_thr_mtx, 0);

		/* Look for any jobs on the work queue. */
		p->np_state = NDIS_PSTATE_RUNNING;
		while (!STAILQ_EMPTY(p->np_q)) {
			r = STAILQ_FIRST(p->np_q);
			STAILQ_REMOVE_HEAD(p->np_q, link);

			/* for debugging */
			
			if(p == &ndis_tproc) {
				num_tq++;
				_ndis_taskqueue_req = r;
				r->area = 1;
			} else if(p == &ndis_iproc) {
				num_swi++;
				_ndis_swi_req = r;
				r->area = 2;
			}
			mtx_unlock_spin(&ndis_thr_mtx);

			/* Just for debugging */

			if(p == &ndis_tproc) {
				calling_in_tq = TRUE;
			} else if(p == &ndis_iproc) {
				calling_in_swi	   = TRUE;
			}
			
			/* Do the work. */
			if (r->nr_func != NULL)
				(*r->nr_func)(r->nr_arg);
			
			/* Just for debugging */
			if(p == &ndis_tproc) {
				calling_in_tq = FALSE;
			} else if(p == &ndis_iproc) {
				calling_in_swi	   = FALSE;
			}

			mtx_lock_spin(&ndis_thr_mtx);

			/* Zeroing out the ndis_req is just for debugging */
			//memset(r, 0, sizeof(struct ndis_req));
			STAILQ_INSERT_HEAD(&ndis_free, r, link);
			
			/* Check for a shutdown request */
			if (r->nr_exit == TRUE)
				die = r;
		}
		p->np_state = NDIS_PSTATE_SLEEPING;

		mtx_unlock_spin(&ndis_thr_mtx);

		/* Bail if we were told to shut down. */

		if (die != NULL)
			break;
	}

	wakeup(die);

	kthread_exit(0);
	/* NOTREACHED */
}

/*static*/ int
ndis_create_kthreads(void)
{
	struct ndis_req		*r;
	int			i, error = 0;

#ifdef __FreeBSD__
	mtx_init(&ndis_thr_mtx, "NDIS thread lock", NULL, MTX_SPIN);
#else /* __NetBSD__ */
	mutex_init(&ndis_thr_mtx, MUTEX_DEFAULT, IPL_NET);
#endif
	mtx_init(&ndis_req_mtx, "NDIS request lock", MTX_NDIS_LOCK, MTX_DEF);

	STAILQ_INIT(&ndis_ttodo);
	STAILQ_INIT(&ndis_itodo);
	STAILQ_INIT(&ndis_free);

	for (i = 0; i < ndis_jobs; i++) {
		r = malloc(sizeof(struct ndis_req), M_DEVBUF, M_WAITOK|M_ZERO);	
		if (r == NULL) {
			error = ENOMEM;
			break;
		}
		STAILQ_INSERT_HEAD(&ndis_free, r, link);
	}

	if (error == 0) {
		ndis_tproc.np_q = &ndis_ttodo;
		ndis_tproc.np_state = NDIS_PSTATE_SLEEPING;
#ifdef __FreeBSD__
		error = kthread_create(ndis_runq, &ndis_tproc,
		    &ndis_tproc.np_p, RFHIGHPID,
		    NDIS_KSTACK_PAGES, "ndis taskqueue");
#else /* __NetBSD__ */
		error = ndis_kthread_create(ndis_runq, &ndis_tproc,
		    &ndis_tproc.np_p, ndis_tproc.np_stack, PAGE_SIZE*NDIS_KSTACK_PAGES, "ndis taskqueue");
#endif
	}

	if (error == 0) {
		ndis_iproc.np_q = &ndis_itodo;
		ndis_iproc.np_state = NDIS_PSTATE_SLEEPING;
#ifdef __FreeBSD__
		error = kthread_create(ndis_runq, &ndis_iproc,
		    &ndis_iproc.np_p, RFHIGHPID,
		    NDIS_KSTACK_PAGES, "ndis swi");
#else
		error = ndis_kthread_create(ndis_runq, &ndis_iproc,
		    &ndis_iproc.np_p, ndis_iproc.np_stack, PAGE_SIZE*NDIS_KSTACK_PAGES, "ndis swi");
#endif
	}

	if (error) {
		while ((r = STAILQ_FIRST(&ndis_free)) != NULL) {
			STAILQ_REMOVE_HEAD(&ndis_free, link);
			free(r, M_DEVBUF);
		}
		return(error);
	}

	return(0);
}

static void
ndis_destroy_kthreads(void)
{
	struct ndis_req		*r;

	/* Stop the threads. */

	ndis_stop_thread(NDIS_TASKQUEUE);
	ndis_stop_thread(NDIS_SWI);

	/* Destroy request structures. */

	while ((r = STAILQ_FIRST(&ndis_free)) != NULL) {
		STAILQ_REMOVE_HEAD(&ndis_free, link);
		free(r, M_DEVBUF);
	}

	mtx_destroy(&ndis_req_mtx);
	mtx_destroy(&ndis_thr_mtx);

	return;
}

static void
ndis_stop_thread(int t)
{
	struct ndis_req		*r;
	struct ndisqhead	*q;
	struct proc		*p;

	if (t == NDIS_TASKQUEUE) {
		q = &ndis_ttodo;
		p = ndis_tproc.np_p;
	} else {
		q = &ndis_itodo;
		p = ndis_iproc.np_p;
	}

	/* Create and post a special 'exit' job. */

	mtx_lock_spin(&ndis_thr_mtx);

	r = STAILQ_FIRST(&ndis_free);
	STAILQ_REMOVE_HEAD(&ndis_free, link);
	r->nr_func = NULL;
	r->nr_arg = NULL;
	r->nr_exit = TRUE;
	r->area	   = 3;
	STAILQ_INSERT_TAIL(q, r, link);
	mtx_unlock_spin(&ndis_thr_mtx);

	ndis_thresume(p);

	/* Wait for thread exit */
	tsleep(r, PZERO | PCATCH, "ndisthexit", hz * 60);

	/* Now empty the job list. */
	mtx_lock_spin(&ndis_thr_mtx);
	while ((r = STAILQ_FIRST(q)) != NULL) {
		STAILQ_REMOVE_HEAD(q, link);
		STAILQ_INSERT_HEAD(&ndis_free, r, link);
	}
	mtx_unlock_spin(&ndis_thr_mtx);
}

static int
ndis_enlarge_thrqueue(int cnt)
{
	struct ndis_req		*r;
	int			i;

	for (i = 0; i < cnt; i++) {
		r = malloc(sizeof(struct ndis_req), M_DEVBUF, M_WAITOK);
		if (r == NULL)
			return(ENOMEM);

		mtx_lock_spin(&ndis_thr_mtx);
		STAILQ_INSERT_HEAD(&ndis_free, r, link);
		ndis_jobs++;
		mtx_unlock_spin(&ndis_thr_mtx);
	}

	return(0);
}

static int
ndis_shrink_thrqueue(int cnt)
{
	struct ndis_req		*r;
	int			i;

	for (i = 0; i < cnt; i++) {
		mtx_lock_spin(&ndis_thr_mtx);
		r = STAILQ_FIRST(&ndis_free);
		if (r == NULL) {
			mtx_unlock_spin(&ndis_thr_mtx);
			return(ENOMEM);
		}
		STAILQ_REMOVE_HEAD(&ndis_free, link);
		ndis_jobs--;
		mtx_unlock_spin(&ndis_thr_mtx);
		free(r, M_DEVBUF);
	}

	return(0);
}

int
ndis_unsched(void (*func)(void *), void *arg, int t)
{
	struct ndis_req		*r;
	struct ndisqhead	*q;

	if (t == NDIS_TASKQUEUE) {
		q = &ndis_ttodo;
	} else {
		q = &ndis_itodo;
	}

	mtx_lock_spin(&ndis_thr_mtx);
	STAILQ_FOREACH(r, q, link) {
		if (r->nr_func == func && r->nr_arg == arg) {
			r->area = 4;
			STAILQ_REMOVE(q, r, ndis_req, link);
			STAILQ_INSERT_HEAD(&ndis_free, r, link);
			mtx_unlock_spin(&ndis_thr_mtx);
			return(0);
		}
	}
	mtx_unlock_spin(&ndis_thr_mtx);

	return(ENOENT);
}

/* just for testing */
struct ndis_req *ls_tq_req = NULL;
struct ndis_req *ls_swi_req = NULL;

int
ndis_sched(void (*func)(void *), void *arg, int t)
{
	struct ndis_req		*r;
	struct ndisqhead	*q;
	struct proc		*p;
	int			s;
#ifdef __NetBSD__
	/* just for debugging */
	struct ndis_req		**ls;
	//struct lwp		*l = curlwp;
#endif

	if (t == NDIS_TASKQUEUE) {
		ls = &ls_tq_req;
		q = &ndis_ttodo;
		p = ndis_tproc.np_p;
	} else {
		ls = &ls_swi_req;
		q = &ndis_itodo;
		p = ndis_iproc.np_p;
	}

	mtx_lock_spin(&ndis_thr_mtx);

	/*
	 * Check to see if an instance of this job is already
	 * pending. If so, don't bother queuing it again.
	 */
	STAILQ_FOREACH(r, q, link) {
		if (r->nr_func == func && r->nr_arg == arg) {
#ifdef __NetBSD__
			if (t == NDIS_TASKQUEUE)
				s = ndis_tproc.np_state;
			else
				s = ndis_iproc.np_state;
#endif
			mtx_unlock_spin(&ndis_thr_mtx);
#ifdef __NetBSD__
			/* The swi thread seemed to be going to sleep, and not waking up
			 * again, so I thought I'd try this out...
			 */
			if (s == NDIS_PSTATE_SLEEPING)
				ndis_thresume(p);
#endif
			return(0);
		}
	}
	r = STAILQ_FIRST(&ndis_free);
	if (r == NULL) {
		mtx_unlock_spin(&ndis_thr_mtx);
		return(EAGAIN);
	}
	STAILQ_REMOVE_HEAD(&ndis_free, link);
#ifdef __NetBSD__
	//memset(r, 0, sizeof(struct ndis_req));
#endif
	*ls = r;
	r->nr_func = func;
	r->nr_arg = arg;
	r->nr_exit = FALSE;
	r->area	   = 5;
	STAILQ_INSERT_TAIL(q, r, link);
	if (t == NDIS_TASKQUEUE) {
		s = ndis_tproc.np_state;
	} else {
		s = ndis_iproc.np_state;
	}
	mtx_unlock_spin(&ndis_thr_mtx);

	/*
	 * Post the job, but only if the thread is actually blocked
	 * on its own suspend call. If a driver queues up a job with
	 * NdisScheduleWorkItem() which happens to do a KeWaitForObject(),
	 * it may suspend there, and in that case we don't want to wake
	 * it up until KeWaitForObject() gets woken up on its own.
	 */
	if (s == NDIS_PSTATE_SLEEPING) {
		ndis_thresume(p);
	}

	return(0);
}

/* Try out writing my own version of ndis_sched() for NetBSD in which I just
 * call the function instead of scheduling it.  I know this isn't
 * what's supposed to be done, but I've been having a lot of problems
 * with the SWI and taskqueue threads, and just thought I'd give this
 * a try.
 */
 
 /* I don't think this will work, because it means that DPC's will be
  * called from the bottom half of the kernel, so they won't be able
  * to sleep using KeWaitForSingleObject.
  */
 /*
 int
ndis_sched(void (*func)(void *), void *arg, int t)
{
	if(func != NULL) {
		(*func)(arg);
	}
	
	return 0;
}
*/

int
ndis_thsuspend(proc_t *p, kmutex_t *m, int timo)
{

	return mtsleep(&p->p_sigpend.sp_set, PZERO,  "ndissp", timo, m);
}

void
ndis_thresume(struct proc *p)
{

	wakeup(&p->p_sigpend.sp_set);
}

__stdcall static void
ndis_sendrsrcavail_func(ndis_handle adapter)
{
	return;
}

__stdcall static void
ndis_status_func(ndis_handle adapter, ndis_status status, void *sbuf,
    uint32_t slen)
{
	ndis_miniport_block	*block;
	struct ndis_softc	*sc;
	struct ifnet		*ifp;

	block = adapter;
#ifdef __FreeBSD__	
	sc = device_get_softc(block->nmb_physdeviceobj->do_devext);
#else /* __NetBSD__ */
	sc = (struct ndis_softc *)block->nmb_physdeviceobj->pdo_sc;
#endif	
	
#ifdef __FreeBSD__
	ifp = &sc->arpcom.ac_if;
#else
	ifp = &sc->arpcom.ec_if;
#endif
	if (ifp->if_flags & IFF_DEBUG)
		printf("%s: status: %x\n", 
		       device_xname(sc->ndis_dev), status);
	return;
}

__stdcall static void
ndis_statusdone_func(ndis_handle adapter)
{
	ndis_miniport_block	*block;
	struct ndis_softc	*sc;
	struct ifnet		*ifp;

	block = adapter;
#ifdef __FreeBSD__	
	sc = device_get_softc(block->nmb_physdeviceobj->do_devext);
#else /* __NetBSD__ */
	sc = (struct ndis_softc *)block->nmb_physdeviceobj->pdo_sc;
#endif	
	
#ifdef __FreeBSD__
	ifp = &sc->arpcom.ac_if;
#else
	ifp = &sc->arpcom.ec_if;
#endif
	if (ifp->if_flags & IFF_DEBUG)
		printf("%s: status complete\n",
		       device_xname(sc->ndis_dev));
	return;
}

__stdcall static void
ndis_setdone_func(ndis_handle adapter, ndis_status status)
{
	ndis_miniport_block	*block;
	block = adapter;

	block->nmb_setstat = status;
	wakeup(&block->nmb_setstat);
	return;
}

__stdcall static void
ndis_getdone_func(ndis_handle adapter, ndis_status status)
{
	ndis_miniport_block	*block;
	block = adapter;

	block->nmb_getstat = status;
	wakeup(&block->nmb_getstat);
	return;
}

__stdcall static void
ndis_resetdone_func(ndis_handle adapter, ndis_status status,
    uint8_t addressingreset)
{
	ndis_miniport_block	*block;
	struct ndis_softc	*sc;
	struct ifnet		*ifp;

	block = adapter;
#ifdef __FreeBSD__	
	sc = device_get_softc(block->nmb_physdeviceobj->do_devext);
#else /* __NetBSD__ */
	sc = (struct ndis_softc *)block->nmb_physdeviceobj->pdo_sc;
#endif	
	
#ifdef __FreeBSD__
	ifp = &sc->arpcom.ac_if;
#else
	ifp = &sc->arpcom.ec_if;
#endif

	if (ifp->if_flags & IFF_DEBUG)
		printf("%s: reset done...\n",
		       device_xname(sc->ndis_dev));
	wakeup(sc);
	return;
}

#ifdef __FreeBSD__
/* FreeBSD version of ndis_create_sysctls() */
int
ndis_create_sysctls(void *arg)
{
	struct ndis_softc	*sc;
	ndis_cfg		*vals;
	char			buf[256];
	struct sysctl_oid	*oidp;
	struct sysctl_ctx_entry	*e;

	if (arg == NULL)
		return(EINVAL);

	sc = arg;
	vals = sc->ndis_regvals;

	TAILQ_INIT(&sc->ndis_cfglist_head);

#if __FreeBSD_version < 502113
	/* Create the sysctl tree. */

	sc->ndis_tree = SYSCTL_ADD_NODE(&sc->ndis_ctx,
	    SYSCTL_STATIC_CHILDREN(_hw), OID_AUTO,
	    device_get_nameunit(sc->ndis_dev), CTLFLAG_RD, 0,
	    device_get_desc(sc->ndis_dev));

#endif
	/* Add the driver-specific registry keys. */

	vals = sc->ndis_regvals;
	while(1) {
		if (vals->nc_cfgkey == NULL)
			break;
		if (vals->nc_idx != sc->ndis_devidx) {
			vals++;
			continue;
		}

		/* See if we already have a sysctl with this name */

		oidp = NULL;
#if __FreeBSD_version < 502113
		TAILQ_FOREACH(e, &sc->ndis_ctx, link) {
#else
		TAILQ_FOREACH(e, device_get_sysctl_ctx(sc->ndis_dev), link) {
#endif
                	oidp = e->entry;
			if (ndis_strcasecmp(oidp->oid_name,
			    vals->nc_cfgkey) == 0)
				break;
			oidp = NULL;
		}

		if (oidp != NULL) {
			vals++;
			continue;
		}

#if __FreeBSD_version < 502113
		SYSCTL_ADD_STRING(&sc->ndis_ctx,
		    SYSCTL_CHILDREN(sc->ndis_tree),
#else
		SYSCTL_ADD_STRING(device_get_sysctl_ctx(sc->ndis_dev),
		    SYSCTL_CHILDREN(device_get_sysctl_tree(sc->ndis_dev)),
#endif
		    OID_AUTO, vals->nc_cfgkey,
		    CTLFLAG_RW, vals->nc_val,
		    sizeof(vals->nc_val),
		    vals->nc_cfgdesc);
		vals++;
	}

	/* Now add a couple of builtin keys. */

	/*
	 * Environment can be either Windows (0) or WindowsNT (1).
	 * We qualify as the latter.
	 */
	ndis_add_sysctl(sc, "Environment",
	    "Windows environment", "1", CTLFLAG_RD);

	/* NDIS version should be 5.1. */
	ndis_add_sysctl(sc, "NdisVersion",
	    "NDIS API Version", "0x00050001", CTLFLAG_RD);

	/* Bus type (PCI, PCMCIA, etc...) */
	snprintf(buf, sizeof(buf), "%d", (int)sc->ndis_iftype);
	ndis_add_sysctl(sc, "BusType", "Bus Type", buf, CTLFLAG_RD);

	if (sc->ndis_res_io != NULL) {
		snprintf(buf, sizeof(buf), "0x%lx",
		    rman_get_start(sc->ndis_res_io));
		ndis_add_sysctl(sc, "IOBaseAddress",
		    "Base I/O Address", buf, CTLFLAG_RD);
	}

	if (sc->ndis_irq != NULL) {
		snprintf(buf, sizeof(buf), "%lu", rman_get_start(sc->ndis_irq));
		ndis_add_sysctl(sc, "InterruptNumber",
		    "Interrupt Number", buf, CTLFLAG_RD);
	}

	return(0);
}
#endif /* __FreeBSD__ */

#ifdef __NetBSD__
/* NetBSD version of ndis_create_sysctls() */
int
ndis_create_sysctls(void *arg)
{
	struct ndis_softc	*sc;
	ndis_cfg		*vals;
	const struct sysctlnode *ndis_node;
	char buf[256];
	
	printf("in ndis_create_sysctls()\n");

	if (arg == NULL)
		return(EINVAL);

	sc = arg;
	vals = sc->ndis_regvals;

	TAILQ_INIT(&sc->ndis_cfglist_head);

	/* Create the sysctl tree. */
	sysctl_createv(&sc->sysctllog, 0, NULL, &ndis_node, CTLFLAG_READWRITE, CTLTYPE_NODE,
					device_xname(sc->ndis_dev), NULL, NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	/* Store the number of the ndis mib */	
	sc->ndis_sysctl_mib = ndis_node->sysctl_num;
	
	/* Add the driver-specific registry keys. */
	vals = sc->ndis_regvals;
	while(1) {
		if (vals->nc_cfgkey == NULL)
			break;
		if (vals->nc_idx != sc->ndis_devidx) {
			vals++;
			continue;
		}

		/* See if we already have a sysctl with this name */
/* TODO: Is something like this necessary in NetBSD?  I'm guessing this
   TODO: is just checking if any of the information in the .inf file was
   TODO: already determined by FreeBSD's autoconfiguration which seems to
   TODO: add dev.XXX sysctl's beginning with %.  (NetBSD dosen't seem to do this).
*/		   

/* TODO: use CTLFLAG_OWNDATA or not? */
		   /*		   
		sysctl_createv(&sc->sysctllog, 0, NULL, NULL, 
						CTLFLAG_READWRITE|CTLFLAG_OWNDESC|CTLFLAG_OWNDATA, CTLTYPE_STRING, 
						vals->nc_cfgkey, vals->nc_cfgdesc, NULL, 0, vals->nc_val, strlen(vals->nc_val),
					    ndis_node->sysctl_num, CTL_CREATE, CTL_EOL);
		   */
		   ndis_add_sysctl(sc, vals->nc_cfgkey,
						   vals->nc_cfgdesc, vals->nc_val, CTLFLAG_READWRITE);		   
			
   		vals++;
	} /* end while */	

		/* Now add a couple of builtin keys. */

	/*
	 * Environment can be either Windows (0) or WindowsNT (1).
	 * We qualify as the latter.
	 */	
#ifdef __NetBSD__
#define CTLFLAG_RD CTLFLAG_READONLY
/* TODO: do we need something like rman_get_start? */
#define rman_get_start(x) x
#endif						
		ndis_add_sysctl(sc, "Environment",
						"Windows environment", "1", CTLFLAG_RD);
						
		/* NDIS version should be 5.1. */
		ndis_add_sysctl(sc, "NdisVersion",
						/*"NDIS API Version"*/ "Version", "0x00050001", CTLFLAG_RD);
		
		/* Bus type (PCI, PCMCIA, etc...) */
		snprintf(buf, sizeof(buf), "%d", (int)sc->ndis_iftype);
		ndis_add_sysctl(sc, "BusType", "Bus Type", buf, CTLFLAG_RD);

		if (sc->ndis_res_io != NULL) {
			snprintf(buf, sizeof(buf), "0x%lx",
			    (long unsigned int)rman_get_start(sc->ndis_res_io));
			ndis_add_sysctl(sc, "IOBaseAddress",
							/*"Base I/O Address"*/ "Base I/O", buf, CTLFLAG_RD);
		}

		if (sc->ndis_irq != NULL) {
			snprintf(buf, sizeof(buf), "%lu", (long unsigned int)rman_get_start(sc->ndis_irq));
			ndis_add_sysctl(sc, "InterruptNumber",
							"Interrupt Number", buf, CTLFLAG_RD);
		}

		return(0);	
}
#endif /* __NetBSD__ */

char *ndis_strdup(const char *src);

char *ndis_strdup(const char *src)
{
	char *ret;
	
	ret = malloc(strlen(src), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (ret == NULL) {
		printf("ndis_strdup failed\n");
		return(NULL);
	}
	strcpy(ret, src);
	
	return ret;
}

int
ndis_add_sysctl(void *arg, const char *key, const char *desc, const char *val, int flag)
{
	struct ndis_softc	*sc;
	struct ndis_cfglist	*cfg;
	char			descstr[256];
#ifdef __NetBSD__
	char newkey[MAX_SYSCTL_LEN+1];
#endif	

	sc = arg;

	cfg = malloc(sizeof(struct ndis_cfglist), M_DEVBUF, M_NOWAIT|M_ZERO);

	if (cfg == NULL)
		return(ENOMEM);

	/* I added this because NetBSD sysctl node names can't begin with
	 * a digit.
	 */
#ifdef __NetBSD__	
	if(strlen(key) + strlen("ndis_") > MAX_SYSCTL_LEN) {
		panic("sysctl name too long: %s\n", key);
	}
	strcpy(newkey, "ndis_");
	strcpy(newkey + strlen("ndis_"), key);	
	key = newkey;
#endif	
	
	cfg->ndis_cfg.nc_cfgkey = ndis_strdup(key);
	
	if (desc == NULL) {
		snprintf(descstr, sizeof(descstr), "%s (dynamic)", key);
		cfg->ndis_cfg.nc_cfgdesc = ndis_strdup(descstr);
	} else
		cfg->ndis_cfg.nc_cfgdesc = ndis_strdup(desc);
	strcpy(cfg->ndis_cfg.nc_val, val);

	TAILQ_INSERT_TAIL(&sc->ndis_cfglist_head, cfg, link);

#ifdef __FreeBSD__
#if __FreeBSD_version < 502113
	SYSCTL_ADD_STRING(&sc->ndis_ctx, SYSCTL_CHILDREN(sc->ndis_tree),
#else
	SYSCTL_ADD_STRING(device_get_sysctl_ctx(sc->ndis_dev),
	    SYSCTL_CHILDREN(device_get_sysctl_tree(sc->ndis_dev)),
#endif
	    OID_AUTO, cfg->ndis_cfg.nc_cfgkey, flag,
	    cfg->ndis_cfg.nc_val, sizeof(cfg->ndis_cfg.nc_val),
	    cfg->ndis_cfg.nc_cfgdesc);
#else /* __NetBSD__ */
/* TODO: use CTLFLAG_OWNDATA or not? */
	sysctl_createv(&sc->sysctllog, 0, NULL, NULL, flag/*|CTLFLAG_OWNDESC|CTLFLAG_OWNDATA*/, CTLTYPE_STRING, 
					cfg->ndis_cfg.nc_cfgkey, cfg->ndis_cfg.nc_cfgdesc, NULL, 0, cfg->ndis_cfg.nc_val,
					strlen(cfg->ndis_cfg.nc_val), sc->ndis_sysctl_mib, CTL_CREATE, CTL_EOL);	
#endif	
	return(0);
}

int
ndis_flush_sysctls(void *arg)
{
	struct ndis_softc	*sc;
	struct ndis_cfglist	*cfg;

	sc = arg;

	while (!TAILQ_EMPTY(&sc->ndis_cfglist_head)) {
		cfg = TAILQ_FIRST(&sc->ndis_cfglist_head);
		TAILQ_REMOVE(&sc->ndis_cfglist_head, cfg, link);
#ifdef __FreeBSD__		
		free(cfg->ndis_cfg.nc_cfgkey, M_DEVBUF);
		free(cfg->ndis_cfg.nc_cfgdesc, M_DEVBUF);
#endif		
		free(cfg, M_DEVBUF);
	}

	return(0);
}

static void
ndis_return(void *arg)
{
	struct ndis_softc	*sc;
	__stdcall ndis_return_handler	returnfunc;
	ndis_handle		adapter;
	ndis_packet		*p;
	uint8_t			irql = 0;	/* XXX: gcc */

	p = arg;
	sc = p->np_softc;
	adapter = sc->ndis_block->nmb_miniportadapterctx;

	if (adapter == NULL)
		return;

	returnfunc = sc->ndis_chars->nmc_return_packet_func;

	KeAcquireSpinLock(&sc->ndis_block->nmb_lock, &irql);
	MSCALL2(returnfunc, adapter, p);
	KeReleaseSpinLock(&sc->ndis_block->nmb_lock, irql);

	return;
}

void
#ifdef __FreeBSD__
ndis_return_packet(buf, arg)
	void			*buf;	/* not used */
	void			*arg;
#else
ndis_return_packet(struct mbuf *m, void *buf,
    size_t size, void *arg)
#endif

{
	ndis_packet		*p;

	if (arg == NULL)
		return;

	p = arg;

	/* Decrement refcount. */
	p->np_refcnt--;

	/* Release packet when refcount hits zero, otherwise return. */
	if (p->np_refcnt)
		return;

	ndis_sched(ndis_return, p, NDIS_TASKQUEUE);

	return;
}

void
ndis_free_bufs(ndis_buffer *b0)
{
	ndis_buffer		*next;

	if (b0 == NULL)
		return;

	while(b0 != NULL) {
		next = b0->mdl_next;
		IoFreeMdl(b0);
		b0 = next;
	}

	return;
}
int in_reset = 0;
void
ndis_free_packet(ndis_packet *p)
{
	if (p == NULL)
		return;

	ndis_free_bufs(p->np_private.npp_head);
	NdisFreePacket(p);
	return;
}

#ifdef __FreeBSD__
int
ndis_convert_res(void *arg)
{
	struct ndis_softc	*sc;
	ndis_resource_list	*rl = NULL;
	cm_partial_resource_desc	*prd = NULL;
	ndis_miniport_block	*block;
	device_t		dev;
	struct resource_list	*brl;
	struct resource_list_entry	*brle;
#if __FreeBSD_version < 600022
	struct resource_list	brl_rev;
	struct resource_list_entry	*n;
#endif
	int 			error = 0;

	sc = arg;
	block = sc->ndis_block;
	dev = sc->ndis_dev;

#if __FreeBSD_version < 600022
	SLIST_INIT(&brl_rev);
#endif
	rl = malloc(sizeof(ndis_resource_list) +
	    (sizeof(cm_partial_resource_desc) * (sc->ndis_rescnt - 1)),
	    M_DEVBUF, M_NOWAIT|M_ZERO);

	if (rl == NULL)
		return(ENOMEM);

	rl->cprl_version = 5;
	rl->cprl_version = 1;
	rl->cprl_count = sc->ndis_rescnt;
	prd = rl->cprl_partial_descs;

	brl = BUS_GET_RESOURCE_LIST(dev, dev);

	if (brl != NULL) {

#if __FreeBSD_version < 600022
		/*
		 * We have a small problem. Some PCI devices have
		 * multiple I/O ranges. Windows orders them starting
		 * from lowest numbered BAR to highest. We discover
		 * them in that order too, but insert them into a singly
		 * linked list head first, which means when time comes
		 * to traverse the list, we enumerate them in reverse
		 * order. This screws up some drivers which expect the
		 * BARs to be in ascending order so that they can choose
		 * the "first" one as their register space. Unfortunately,
		 * in order to fix this, we have to create our own
		 * temporary list with the entries in reverse order.
		 */
		SLIST_FOREACH(brle, brl, link) {
			n = malloc(sizeof(struct resource_list_entry),
			    M_TEMP, M_NOWAIT);
			if (n == NULL) {
				error = ENOMEM;
				goto bad;
			}
			memcpy( (char *)n, (char *)brle,
			    sizeof(struct resource_list_entry));
			SLIST_INSERT_HEAD(&brl_rev, n, link);
		}

		SLIST_FOREACH(brle, &brl_rev, link) {
#else
		STAILQ_FOREACH(brle, brl, link) {
#endif
			switch (brle->type) {
			case SYS_RES_IOPORT:
				prd->cprd_type = CmResourceTypePort;
				prd->cprd_flags = CM_RESOURCE_PORT_IO;
				prd->cprd_sharedisp =
				    CmResourceShareDeviceExclusive;
				prd->u.cprd_port.cprd_start.np_quad =
				    brle->start;
				prd->u.cprd_port.cprd_len = brle->count;
				break;
			case SYS_RES_MEMORY:
				prd->cprd_type = CmResourceTypeMemory;
				prd->cprd_flags =
				    CM_RESOURCE_MEMORY_READ_WRITE;
				prd->cprd_sharedisp =
				    CmResourceShareDeviceExclusive;
				prd->u.cprd_port.cprd_start.np_quad =
				    brle->start;
				prd->u.cprd_port.cprd_len = brle->count;
				break;
			case SYS_RES_IRQ:
				prd->cprd_type = CmResourceTypeInterrupt;
				prd->cprd_flags = 0;
				prd->cprd_sharedisp =
				    CmResourceShareDeviceExclusive;
				prd->u.cprd_intr.cprd_level = brle->start;
				prd->u.cprd_intr.cprd_vector = brle->start;
				prd->u.cprd_intr.cprd_affinity = 0;
				break;
			default:
				break;
			}
			prd++;
		}
	}

	block->nmb_rlist = rl;

#if __FreeBSD_version < 600022
bad:

	while (!SLIST_EMPTY(&brl_rev)) {
		n = SLIST_FIRST(&brl_rev);
		SLIST_REMOVE_HEAD(&brl_rev, link);
		free (n, M_TEMP);
	}
#endif

	return(error);
}
#endif /* __FreeBSD__ */
/*
 * Map an NDIS packet to an mbuf list. When an NDIS driver receives a
 * packet, it will hand it to us in the form of an ndis_packet,
 * which we need to convert to an mbuf that is then handed off
 * to the stack. Note: we configure the mbuf list so that it uses
 * the memory regions specified by the ndis_buffer structures in
 * the ndis_packet as external storage. In most cases, this will
 * point to a memory region allocated by the driver (either by
 * ndis_malloc_withtag() or ndis_alloc_sharedmem()). We expect
 * the driver to handle free()ing this region for is, so we set up
 * a dummy no-op free handler for it.
 */ 

int
ndis_ptom(struct mbuf **m0, ndis_packet *p)
{
	struct mbuf		*m, *prev = NULL;
	ndis_buffer		*buf;
	ndis_packet_private	*priv;
	uint32_t		totlen = 0;

	if (p == NULL || m0 == NULL)
		return(EINVAL);

	priv = &p->np_private;
	buf = priv->npp_head;
	p->np_refcnt = 0;

	for (buf = priv->npp_head; buf != NULL; buf = buf->mdl_next) {
		if (buf == priv->npp_head)
			MGETHDR(m, M_DONTWAIT, MT_HEADER);
		else
			MGET(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			m_freem(*m0);
			*m0 = NULL;
			return(ENOBUFS);
		}
		m->m_len = MmGetMdlByteCount(buf);
		m->m_data = MmGetMdlVirtualAddress(buf);
#ifdef __FreeBSD__
		MEXTADD(m, m->m_data, m->m_len, ndis_return_packet,
			p, 0, EXT_NDIS);
#else
		MEXTADD(m, m->m_data, m->m_len, M_DEVBUF,
			ndis_return_packet, p);
#endif
		p->np_refcnt++;
		totlen += m->m_len;
		if (m->m_flags & MT_HEADER)
			*m0 = m;
		else
			prev->m_next = m;
		prev = m;
	}

	(*m0)->m_pkthdr.len = totlen;

	return(0);
}

/*
 * Create an NDIS packet from an mbuf chain.
 * This is used mainly when transmitting packets, where we need
 * to turn an mbuf off an interface's send queue and transform it
 * into an NDIS packet which will be fed into the NDIS driver's
 * send routine.
 *
 * NDIS packets consist of two parts: an ndis_packet structure,
 * which is vaguely analagous to the pkthdr portion of an mbuf,
 * and one or more ndis_buffer structures, which define the
 * actual memory segments in which the packet data resides.
 * We need to allocate one ndis_buffer for each mbuf in a chain,
 * plus one ndis_packet as the header.
 */

int
ndis_mtop(struct mbuf *m0, ndis_packet **p)
{
	struct mbuf		*m;
	ndis_buffer		*buf = NULL, *prev = NULL;
	ndis_packet_private	*priv;

	if (p == NULL || *p == NULL || m0 == NULL)
		return(EINVAL);

	priv = &(*p)->np_private;
	priv->npp_totlen = m0->m_pkthdr.len;

	for (m = m0; m != NULL; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		buf = IoAllocateMdl(m->m_data, m->m_len, FALSE, FALSE, NULL);
		if (buf == NULL) {
			ndis_free_packet(*p);
			*p = NULL;
			return(ENOMEM);
		}

		if (priv->npp_head == NULL)
			priv->npp_head = buf;
		else
			prev->mdl_next = buf;
		prev = buf;
	}

	priv->npp_tail = buf;

	return(0);
}

int
ndis_get_supported_oids(void *arg, ndis_oid **oids, int *oidcnt)
{
	int			len, rval;
	ndis_oid		*o;

	if (arg == NULL || oids == NULL || oidcnt == NULL)
		return(EINVAL);
	len = 0;
	ndis_get_info(arg, OID_GEN_SUPPORTED_LIST, NULL, &len);

	o = malloc(len, M_DEVBUF, M_NOWAIT);
	if (o == NULL)
		return(ENOMEM);

	rval = ndis_get_info(arg, OID_GEN_SUPPORTED_LIST, o, &len);

	if (rval) {
		free(o, M_DEVBUF);
		return(rval);
	}

	*oids = o;
	*oidcnt = len / 4;

	return(0);
}

int
ndis_set_info(void *arg, ndis_oid oid, void *buf, int *buflen)
{
	struct ndis_softc	*sc;
	ndis_status		rval;
	ndis_handle		adapter;
	__stdcall ndis_setinfo_handler	setfunc;
	uint32_t		byteswritten = 0, bytesneeded = 0;
	uint8_t			irql = 0;	/* XXX: gcc */

	/*
	 * According to the NDIS spec, MiniportQueryInformation()
	 * and MiniportSetInformation() requests are handled serially:
	 * once one request has been issued, we must wait for it to
 	 * finish before allowing another request to proceed.
	 */

	sc = arg;

	KeAcquireSpinLock(&sc->ndis_block->nmb_lock, &irql);

	if (sc->ndis_block->nmb_pendingreq != NULL)
		panic("ndis_set_info() called while other request pending");
	else
		sc->ndis_block->nmb_pendingreq = (ndis_request *)sc;

	/* I added this lock because it was present in the FreeBSD-current sources */
	NDIS_LOCK(sc);
	
	setfunc = sc->ndis_chars->nmc_setinfo_func;
	adapter = sc->ndis_block->nmb_miniportadapterctx;

	if (adapter == NULL || setfunc == NULL) {
		sc->ndis_block->nmb_pendingreq = NULL;
		KeReleaseSpinLock(&sc->ndis_block->nmb_lock, irql);
		NDIS_UNLOCK(sc);
		return(ENXIO);
	}
	
	NDIS_UNLOCK(sc);
	
	rval = MSCALL6(setfunc, adapter, oid, buf, *buflen,
	    &byteswritten, &bytesneeded);

	sc->ndis_block->nmb_pendingreq = NULL;

	KeReleaseSpinLock(&sc->ndis_block->nmb_lock, irql);

	if (rval == NDIS_STATUS_PENDING) {
		mtx_lock(&ndis_req_mtx);
		mtsleep(&sc->ndis_block->nmb_setstat, PZERO | PNORELOCK, 
		    "ndisset", 5 * hz, &ndis_req_mtx);
		rval = sc->ndis_block->nmb_setstat;
	}

	if (byteswritten)
		*buflen = byteswritten;
	if (bytesneeded)
		*buflen = bytesneeded;

	if (rval == NDIS_STATUS_INVALID_LENGTH)
		return(ENOSPC);

	if (rval == NDIS_STATUS_INVALID_OID)
		return(EINVAL);

	if (rval == NDIS_STATUS_NOT_SUPPORTED ||
	    rval == NDIS_STATUS_NOT_ACCEPTED)
		return(ENOTSUP);

	if (rval != NDIS_STATUS_SUCCESS)
		return(ENODEV);

	return(0);
}

typedef void (*ndis_senddone_func)(ndis_handle, ndis_packet *, ndis_status);

int
ndis_send_packets(void *arg, ndis_packet **packets, int cnt)
{
	struct ndis_softc	*sc;
	ndis_handle		adapter;
	__stdcall ndis_sendmulti_handler	sendfunc;
	__stdcall ndis_senddone_func		senddonefunc;
	int			i;
	ndis_packet		*p;
	uint8_t			irql = 0;	/* XXX: gcc */

	sc = arg;
	adapter = sc->ndis_block->nmb_miniportadapterctx;
	if (adapter == NULL)
		return(ENXIO);
	sendfunc = sc->ndis_chars->nmc_sendmulti_func;
	senddonefunc = sc->ndis_block->nmb_senddone_func;

	if (NDIS_SERIALIZED(sc->ndis_block))
		KeAcquireSpinLock(&sc->ndis_block->nmb_lock, &irql);

	MSCALL3(sendfunc, adapter, packets, cnt);

	for (i = 0; i < cnt; i++) {
		p = packets[i];
		/*
		 * Either the driver already handed the packet to
		 * ndis_txeof() due to a failure, or it wants to keep
		 * it and release it asynchronously later. Skip to the
		 * next one.
		 */
		if (p == NULL || p->np_oob.npo_status == NDIS_STATUS_PENDING)
			continue;
		MSCALL3(senddonefunc, sc->ndis_block, p, p->np_oob.npo_status);
	}

	if (NDIS_SERIALIZED(sc->ndis_block))
		KeReleaseSpinLock(&sc->ndis_block->nmb_lock, irql);

	return(0);
}

int
ndis_send_packet(void *arg, ndis_packet *packet)
{
	struct ndis_softc	*sc;
	ndis_handle		adapter;
	ndis_status		status;
	__stdcall ndis_sendsingle_handler	sendfunc;
	__stdcall ndis_senddone_func		senddonefunc;
	uint8_t			irql = 0;	/* XXX: gcc */

	sc = arg;
	adapter = sc->ndis_block->nmb_miniportadapterctx;
	if (adapter == NULL)
		return(ENXIO);
	sendfunc = sc->ndis_chars->nmc_sendsingle_func;
	senddonefunc = sc->ndis_block->nmb_senddone_func;

	if (NDIS_SERIALIZED(sc->ndis_block))
		KeAcquireSpinLock(&sc->ndis_block->nmb_lock, &irql);
	status = MSCALL3(sendfunc, adapter, packet,
	    packet->np_private.npp_flags);

	if (status == NDIS_STATUS_PENDING) {
		if (NDIS_SERIALIZED(sc->ndis_block))
			KeReleaseSpinLock(&sc->ndis_block->nmb_lock, irql);
		return(0);
	}

	MSCALL3(senddonefunc, sc->ndis_block, packet, status);

	if (NDIS_SERIALIZED(sc->ndis_block))
		KeReleaseSpinLock(&sc->ndis_block->nmb_lock, irql);

	return(0);
}

int
ndis_init_dma(void *arg)
{
	struct ndis_softc	*sc;
	int			i, error = 0;

	sc = arg;

	sc->ndis_tmaps = malloc(sizeof(bus_dmamap_t) * sc->ndis_maxpkts,
	    M_DEVBUF, M_NOWAIT|M_ZERO);

	if (sc->ndis_tmaps == NULL)
		return(ENOMEM);

	for (i = 0; i < sc->ndis_maxpkts; i++) {
#ifdef __FreeBSD__
		error = bus_dmamap_create(sc->ndis_ttag, 0,
		    &sc->ndis_tmaps[i]);
#else
		/*
		bus_dmamap_create(sc->ndis_mtag, sizeof(bus_dmamap_t), 
				  1, sizeof(bus_dmamap_t), BUS_DMA_NOWAIT, 
				  0, &sc->ndis_mmaps[i]);
		*/	
		bus_dmamap_create(sc->ndis_ttag, NDIS_MAXSEG * MCLBYTES, 
				  NDIS_MAXSEG, MCLBYTES, 0, 
				  BUS_DMA_NOWAIT, &sc->ndis_tmaps[i]);
#endif
		if (error) {
			free(sc->ndis_tmaps, M_DEVBUF);
			return(ENODEV);
		}
	}

	return(0);
}

int
ndis_destroy_dma(void *arg)
{
	struct ndis_softc	*sc;
	struct mbuf		*m;
	ndis_packet		*p = NULL;
	int			i;

	sc = arg;

	for (i = 0; i < sc->ndis_maxpkts; i++) {
		if (sc->ndis_txarray[i] != NULL) {
			p = sc->ndis_txarray[i];
			m = (struct mbuf *)p->np_rsvd[1];
			if (m != NULL)
				m_freem(m);
			ndis_free_packet(sc->ndis_txarray[i]);
		}
		bus_dmamap_destroy(sc->ndis_ttag, sc->ndis_tmaps[i]);
	}

	free(sc->ndis_tmaps, M_DEVBUF);

#ifdef __FreeBSD__
	bus_dma_tag_destroy(sc->ndis_ttag);
#endif

	return(0);
}

int
ndis_reset_nic(void *arg)
{
	struct ndis_softc	*sc;
	ndis_handle		adapter;
	__stdcall ndis_reset_handler	resetfunc;
	uint8_t			addressing_reset;
	int			rval;
	uint8_t			irql = 0;	/* XXX: gcc */

	sc = arg;

	adapter = sc->ndis_block->nmb_miniportadapterctx;
	resetfunc = sc->ndis_chars->nmc_reset_func;

	if (adapter == NULL || resetfunc == NULL)
		return(EIO);

	if (NDIS_SERIALIZED(sc->ndis_block))
		KeAcquireSpinLock(&sc->ndis_block->nmb_lock, &irql);

	rval = MSCALL2(resetfunc, &addressing_reset, adapter);

	if (NDIS_SERIALIZED(sc->ndis_block))
		KeReleaseSpinLock(&sc->ndis_block->nmb_lock, irql);

	if (rval == NDIS_STATUS_PENDING) {
		mtsleep(sc, PZERO | PNORELOCK, "ndisrst", 0, &ndis_req_mtx);
	}

	return(0);
}

int
ndis_halt_nic(void *arg)
{
	struct ndis_softc	*sc;
	ndis_handle		adapter;
	__stdcall ndis_halt_handler	haltfunc;

	sc = arg;

	NDIS_LOCK(sc);
	
	adapter = sc->ndis_block->nmb_miniportadapterctx;
	if (adapter == NULL) {
		NDIS_UNLOCK(sc);	
		return(EIO);
	}

	/*
	 * The adapter context is only valid after the init
	 * handler has been called, and is invalid once the
	 * halt handler has been called.
	 */

	haltfunc = sc->ndis_chars->nmc_halt_func;
		
	NDIS_UNLOCK(sc);

	MSCALL1(haltfunc, adapter);

	NDIS_LOCK(sc);
			
	sc->ndis_block->nmb_miniportadapterctx = NULL;

	NDIS_UNLOCK(sc);	

	return(0);
}

int
ndis_shutdown_nic(void *arg)
{
	struct ndis_softc	*sc;
	ndis_handle		adapter;
	__stdcall ndis_shutdown_handler	shutdownfunc;

	sc = arg;
	
	NDIS_LOCK(sc);
	
	adapter = sc->ndis_block->nmb_miniportadapterctx;
	shutdownfunc = sc->ndis_chars->nmc_shutdown_handler;
	
	NDIS_UNLOCK(sc);

	if (adapter == NULL || shutdownfunc == NULL)
		return(EIO);

	if (sc->ndis_chars->nmc_rsvd0 == NULL)
		MSCALL1(shutdownfunc, adapter);
	else
		MSCALL1(shutdownfunc, sc->ndis_chars->nmc_rsvd0);

	ndis_shrink_thrqueue(8);
	TAILQ_REMOVE(&ndis_devhead, sc->ndis_block, link);

	return(0);
}

int
ndis_init_nic(void *arg)
{
	struct ndis_softc	*sc;
	ndis_miniport_block	*block;
        __stdcall ndis_init_handler	initfunc;
	ndis_status		status, openstatus = 0;
	ndis_medium		mediumarray[NdisMediumMax];
	uint32_t		chosenmedium, i;

	if (arg == NULL)
		return(EINVAL);

	sc = arg;

	NDIS_LOCK(sc);
	
	block = sc->ndis_block;
	initfunc = sc->ndis_chars->nmc_init_func;

	NDIS_UNLOCK(sc);	
	
	printf("sc->ndis_chars->nmc_version_major = %d\n\
			sc->ndis_chars->nmc_version_minor = %d\n", 
			sc->ndis_chars->nmc_version_major,
			sc->ndis_chars->nmc_version_minor);

	for (i = 0; i < NdisMediumMax; i++)
		mediumarray[i] = i;

        status = MSCALL6(initfunc, &openstatus, &chosenmedium,
            mediumarray, NdisMediumMax, block, block);
		
	printf("status = %x", status);		

	/*
	 * If the init fails, blow away the other exported routines
	 * we obtained from the driver so we can't call them later.
	 * If the init failed, none of these will work.
	 */
	if (status != NDIS_STATUS_SUCCESS) {
		NDIS_LOCK(sc);
					
		sc->ndis_block->nmb_miniportadapterctx = NULL;
		
		NDIS_UNLOCK(sc);			
		return(ENXIO);
	}

	return(0);
}

void
ndis_enable_intr(void *arg)
{
	struct ndis_softc	*sc;
	ndis_handle		adapter;
	__stdcall ndis_enable_interrupts_handler	intrenbfunc;

	sc = arg;
	adapter = sc->ndis_block->nmb_miniportadapterctx;
	intrenbfunc = sc->ndis_chars->nmc_enable_interrupts_func;
	if (adapter == NULL || intrenbfunc == NULL)
		return;
	MSCALL1(intrenbfunc, adapter);

	return;
}

void
ndis_disable_intr(void *arg)
{
	struct ndis_softc	*sc;
	ndis_handle		adapter;
	__stdcall ndis_disable_interrupts_handler	intrdisfunc;

	sc = arg;
	adapter = sc->ndis_block->nmb_miniportadapterctx;
	intrdisfunc = sc->ndis_chars->nmc_disable_interrupts_func;
	if (adapter == NULL || intrdisfunc == NULL)
	    return;

	MSCALL1(intrdisfunc, adapter);

	return;
}

int
ndis_isr(void *arg, int *ourintr, int *callhandler)
{
	struct ndis_softc	*sc;
	ndis_handle		adapter;
	__stdcall ndis_isr_handler	isrfunc;
	uint8_t			accepted, queue;

	if (arg == NULL || ourintr == NULL || callhandler == NULL)
		return(EINVAL);

	sc = arg;
	adapter = sc->ndis_block->nmb_miniportadapterctx;
	isrfunc = sc->ndis_chars->nmc_isr_func;

	if (adapter == NULL || isrfunc == NULL)
		return(ENXIO);

	MSCALL3(isrfunc, &accepted, &queue, adapter);

	*ourintr = accepted;
	*callhandler = queue;

	return(0);
}

__stdcall static void
ndis_intrhand(kdpc *dpc, device_object *dobj,
    irp *ip, struct ndis_softc *sc)
{
	ndis_handle		adapter;
	__stdcall ndis_interrupt_handler	intrfunc;
	uint8_t			irql = 0;	/* XXX: gcc */

	adapter = sc->ndis_block->nmb_miniportadapterctx;
	intrfunc = sc->ndis_chars->nmc_interrupt_func;

	if (adapter == NULL || intrfunc == NULL)
		return;

	if (NDIS_SERIALIZED(sc->ndis_block))
		KeAcquireSpinLock(&sc->ndis_block->nmb_lock, &irql);

	MSCALL1(intrfunc, adapter);

	/* If there's a MiniportEnableInterrupt() routine, call it. */

	ndis_enable_intr(sc);

	if (NDIS_SERIALIZED(sc->ndis_block))
		KeReleaseSpinLock(&sc->ndis_block->nmb_lock, irql);

	return;
}

int
ndis_get_info(void *arg, ndis_oid oid, void *buf, int *buflen)
{
	struct ndis_softc	*sc;
	ndis_status		rval;
	ndis_handle		adapter;
	__stdcall ndis_queryinfo_handler	queryfunc;
	uint32_t		byteswritten = 0, bytesneeded = 0;
#ifdef __FreeBSD__	
	int			error;
#endif
	uint8_t			irql = 0;	/* XXX: gcc */
	
	//printf("in ndis_get_info\n");
	
	sc = arg;
	KeAcquireSpinLock(&sc->ndis_block->nmb_lock, &irql);

	if (sc->ndis_block->nmb_pendingreq != NULL)
		panic("ndis_get_info() called while other request pending");
	else
		sc->ndis_block->nmb_pendingreq = (ndis_request *)sc;

	queryfunc = sc->ndis_chars->nmc_queryinfo_func;
	adapter = sc->ndis_block->nmb_miniportadapterctx;

	if (adapter == NULL || queryfunc == NULL) {
		sc->ndis_block->nmb_pendingreq = NULL;
		KeReleaseSpinLock(&sc->ndis_block->nmb_lock, irql);
		return(ENXIO);
	}

	rval = MSCALL6(queryfunc, adapter, oid, buf, *buflen,
	    &byteswritten, &bytesneeded);

	sc->ndis_block->nmb_pendingreq = NULL;

	KeReleaseSpinLock(&sc->ndis_block->nmb_lock, irql);

	/* Wait for requests that block. */

	if (rval == NDIS_STATUS_PENDING) {
		mtx_lock(&ndis_req_mtx);
		mtsleep(&sc->ndis_block->nmb_getstat, PZERO | PNORELOCK,
		    "ndisget", 5 * hz, &ndis_req_mtx);
		rval = sc->ndis_block->nmb_getstat;
	}

	if (byteswritten)
		*buflen = byteswritten;
	if (bytesneeded)
		*buflen = bytesneeded;

	if (rval == NDIS_STATUS_INVALID_LENGTH ||
	    rval == NDIS_STATUS_BUFFER_TOO_SHORT)
		return(ENOSPC);

	if (rval == NDIS_STATUS_INVALID_OID)
		return(EINVAL);

	if (rval == NDIS_STATUS_NOT_SUPPORTED ||
	    rval == NDIS_STATUS_NOT_ACCEPTED)
		return(ENOTSUP);

	if (rval != NDIS_STATUS_SUCCESS)
		return(ENODEV);

	return(0);
}

__stdcall uint32_t
NdisAddDevice(driver_object *drv, device_object *pdo)
{
	device_object		*fdo;
	ndis_miniport_block	*block;
	struct ndis_softc	*sc;
	uint32_t		status;

	status = IoCreateDevice(drv, sizeof(ndis_miniport_block), NULL,
	    FILE_DEVICE_UNKNOWN, 0, FALSE, &fdo);

	if (status != STATUS_SUCCESS)
		return(status);

	block = fdo->do_devext;
	block->nmb_deviceobj = fdo;
	block->nmb_physdeviceobj = pdo;
	block->nmb_nextdeviceobj = IoAttachDeviceToDeviceStack(fdo, pdo);
	KeInitializeSpinLock(&block->nmb_lock);
	
#ifdef __NetBSD__
	/* NetBSD has a pointer to the callout object */
	block->nmb_wkupdpctimer.nt_ktimer.k_handle = 
		malloc(sizeof(struct callout), M_DEVBUF, M_NOWAIT|M_ZERO);
#endif	

	/*
	 * Stash pointers to the miniport block and miniport
	 * characteristics info in the if_ndis softc so the
	 * UNIX wrapper driver can get to them later.
     */
#ifdef __FreeBSD__		   	
	sc = device_get_softc(pdo->do_devext);
#else /* __NetBSD__ */
	sc = pdo->pdo_sc;
	fdo->fdo_sc = sc;
#endif   
	sc->ndis_block = block;
	sc->ndis_chars = IoGetDriverObjectExtension(drv, (void *)1);

	IoInitializeDpcRequest(fdo, kernndis_functbl[6].ipt_wrap);

	/* Finish up BSD-specific setup. */

	block->nmb_signature = (void *)0xcafebabe;
	block->nmb_status_func = kernndis_functbl[0].ipt_wrap;
	block->nmb_statusdone_func = kernndis_functbl[1].ipt_wrap;
	block->nmb_setdone_func = kernndis_functbl[2].ipt_wrap;
	block->nmb_querydone_func = kernndis_functbl[3].ipt_wrap;
	block->nmb_resetdone_func = kernndis_functbl[4].ipt_wrap;
	block->nmb_sendrsrc_func = kernndis_functbl[5].ipt_wrap;
	block->nmb_pendingreq = NULL;

	ndis_enlarge_thrqueue(8);

	TAILQ_INSERT_TAIL(&ndis_devhead, block, link);

	return (STATUS_SUCCESS);
}

int
ndis_unload_driver(void *arg)
{
	struct ndis_softc	*sc;
	device_object		*fdo;

	sc = arg;

	if (sc->ndis_block->nmb_rlist != NULL)
		free(sc->ndis_block->nmb_rlist, M_DEVBUF);

	ndis_flush_sysctls(sc);

	ndis_shrink_thrqueue(8);
	TAILQ_REMOVE(&ndis_devhead, sc->ndis_block, link);

	fdo = sc->ndis_block->nmb_deviceobj;
	IoDetachDevice(sc->ndis_block->nmb_nextdeviceobj);
	IoDeleteDevice(fdo);

	return(0);
}
