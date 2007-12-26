/* $NetBSD: kern_pmf.c,v 1.9.4.2 2007/12/26 19:17:22 ad Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_pmf.c,v 1.9.4.2 2007/12/26 19:17:22 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/pmf.h>
#include <sys/queue.h>
#include <sys/syscallargs.h> /* for sys_sync */
#include <sys/workqueue.h>
#include <prop/proplib.h>

#ifdef PMF_DEBUG
int pmf_debug_event;
int pmf_debug_idle;
int pmf_debug_transition;

#define	PMF_EVENT_PRINTF(x)		if (pmf_debug_event) printf x
#define	PMF_IDLE_PRINTF(x)		if (pmf_debug_idle) printf x
#define	PMF_TRANSITION_PRINTF(x)	if (pmf_debug_transition) printf x
#define	PMF_TRANSITION_PRINTF2(y,x)	if (pmf_debug_transition>y) printf x
#else
#define	PMF_EVENT_PRINTF(x)		do { } while (0)
#define	PMF_IDLE_PRINTF(x)		do { } while (0)
#define	PMF_TRANSITION_PRINTF(x)	do { } while (0)
#define	PMF_TRANSITION_PRINTF2(y,x)	do { } while (0)
#endif

/* #define PMF_DEBUG */

MALLOC_DEFINE(M_PMF, "pmf", "device pmf messaging memory");

static prop_dictionary_t pmf_platform = NULL;
static struct workqueue *pmf_event_workqueue;

typedef struct pmf_event_handler {
	TAILQ_ENTRY(pmf_event_handler) pmf_link;
	pmf_generic_event_t pmf_event;
	void (*pmf_handler)(device_t);
	device_t pmf_device;
	bool pmf_global;
} pmf_event_handler_t;

static TAILQ_HEAD(, pmf_event_handler) pmf_all_events =
    TAILQ_HEAD_INITIALIZER(pmf_all_events);

typedef struct pmf_event_workitem {
	struct work		pew_work;
	pmf_generic_event_t	pew_event;
	device_t		pew_device;
} pmf_event_workitem_t;

static void
pmf_event_worker(struct work *wk, void *dummy)
{
	pmf_event_workitem_t *pew;
	pmf_event_handler_t *event;

	pew = (void *)wk;
	KASSERT(wk == &pew->pew_work);
	KASSERT(pew != NULL);
	
	TAILQ_FOREACH(event, &pmf_all_events, pmf_link) {
		if (event->pmf_event != pew->pew_event)
			continue;
		if (event->pmf_device == pew->pew_device || event->pmf_global)
			(*event->pmf_handler)(event->pmf_device);
	}

	free(pew, M_TEMP);

	return;
}

static bool
pmf_check_system_drivers(void)
{
	device_t curdev;
	bool unsupported_devs;

	unsupported_devs = false;
	TAILQ_FOREACH(curdev, &alldevs, dv_list) {
		if (device_pmf_is_registered(curdev))
			continue;
		if (!unsupported_devs)
			printf("Devices without power management support:");
		printf(" %s", device_xname(curdev));
		unsupported_devs = true;
	}
	if (unsupported_devs) {
		printf("\n");
		return false;
	}
	return true;
}

bool
pmf_system_bus_resume(void)
{
	int depth, maxdepth;
	bool rv;
	device_t curdev;

	maxdepth = 0;
	TAILQ_FOREACH(curdev, &alldevs, dv_list) {
		if (curdev->dv_depth > maxdepth)
			maxdepth = curdev->dv_depth;
	}
	++maxdepth;

	aprint_debug("Powering devices:");
	/* D0 handlers are run in order */
	depth = 0;
	rv = true;
	for (depth = 0; depth < maxdepth; ++depth) {
		TAILQ_FOREACH(curdev, &alldevs, dv_list) {
			if (!device_pmf_is_registered(curdev))
				continue;
			if (device_is_active(curdev) ||
			    !device_is_enabled(curdev))
				continue;
			if (curdev->dv_depth != depth)
				continue;

			aprint_debug(" %s", device_xname(curdev));

			if (!device_pmf_bus_resume(curdev))
				aprint_debug("(failed)");
		}
	}
	aprint_debug("\n");

	return rv;
}

bool
pmf_system_resume(void)
{
	int depth, maxdepth;
	bool rv;
	device_t curdev, parent;

	if (!pmf_check_system_drivers())
		return false;

	maxdepth = 0;
	TAILQ_FOREACH(curdev, &alldevs, dv_list) {
		if (curdev->dv_depth > maxdepth)
			maxdepth = curdev->dv_depth;
	}
	++maxdepth;

	aprint_debug("Resuming devices:");
	/* D0 handlers are run in order */
	depth = 0;
	rv = true;
	for (depth = 0; depth < maxdepth; ++depth) {
		TAILQ_FOREACH(curdev, &alldevs, dv_list) {
			if (device_is_active(curdev) ||
			    !device_is_enabled(curdev))
				continue;
			if (curdev->dv_depth != depth)
				continue;
			parent = device_parent(curdev);
			if (parent != NULL &&
			    !device_is_active(parent))
				continue;

			aprint_debug(" %s", device_xname(curdev));

			if (!pmf_device_resume(curdev)) {
				rv = false;
				aprint_debug("(failed)");
			}
		}
	}
	aprint_debug(".\n");

	return rv;
}

bool
pmf_system_suspend(void)
{
	int depth, maxdepth;
	device_t curdev;

	if (!pmf_check_system_drivers())
		return false;

	/*
	 * Flush buffers only if the shutdown didn't do so
	 * already and if there was no panic.
	 */
	if (doing_shutdown == 0 && panicstr == NULL) {
		printf("Flushing disk caches: ");
		sys_sync(NULL, NULL, NULL);
		if (buf_syncwait() != 0)
			printf("giving up\n");
		else
			printf("done\n");
	}

	aprint_debug("Suspending devices:");

	maxdepth = 0;
	TAILQ_FOREACH(curdev, &alldevs, dv_list) {
		if (curdev->dv_depth > maxdepth)
			maxdepth = curdev->dv_depth;
	}

	for (depth = maxdepth; depth >= 0; --depth) {
		TAILQ_FOREACH_REVERSE(curdev, &alldevs, devicelist, dv_list) {
			if (curdev->dv_depth != depth)
				continue;
			if (!device_is_active(curdev))
				continue;

			aprint_debug(" %s", device_xname(curdev));

			/* XXX joerg check return value and abort suspend */
			if (!pmf_device_suspend(curdev))
				aprint_debug("(failed)");
		}
	}

	aprint_debug(".\n");

	return true;
}

void
pmf_system_shutdown(void)
{
	int depth, maxdepth;
	device_t curdev;

	if (!pmf_check_system_drivers())
		delay(2000000);

	aprint_debug("Shutting down devices:");

	maxdepth = 0;
	TAILQ_FOREACH(curdev, &alldevs, dv_list) {
		if (curdev->dv_depth > maxdepth)
			maxdepth = curdev->dv_depth;
	}

	for (depth = maxdepth; depth >= 0; --depth) {
		TAILQ_FOREACH_REVERSE(curdev, &alldevs, devicelist, dv_list) {
			if (curdev->dv_depth != depth)
				continue;
			if (!device_is_active(curdev))
				continue;

			aprint_debug(" %s", device_xname(curdev));

			if (!device_pmf_is_registered(curdev))
				continue;
			if (!device_pmf_class_suspend(curdev)) {
				aprint_debug("(failed)");
				continue;
			}
			if (!device_pmf_driver_suspend(curdev)) {
				aprint_debug("(failed)");
				continue;
			}
		}
	}

	aprint_debug(".\n");
}

bool
pmf_set_platform(const char *key, const char *value)
{
	if (pmf_platform == NULL)
		pmf_platform = prop_dictionary_create();
	if (pmf_platform == NULL)
		return false;

	return prop_dictionary_set_cstring(pmf_platform, key, value);
}

const char *
pmf_get_platform(const char *key)
{
	const char *value;

	if (pmf_platform == NULL)
		return NULL;

	if (!prop_dictionary_get_cstring_nocopy(pmf_platform, key, &value))
		return NULL;

	return value;
}

bool
pmf_device_register(device_t dev,
    bool (*suspend)(device_t), bool (*resume)(device_t))
{
	device_pmf_driver_register(dev, suspend, resume);

	if (!device_pmf_driver_child_register(dev)) {
		device_pmf_driver_deregister(dev);
		return false;
	}

	return true;
}

void
pmf_device_deregister(device_t dev)
{
	device_pmf_class_deregister(dev);
	device_pmf_bus_deregister(dev);
	device_pmf_driver_deregister(dev);
}

bool
pmf_device_suspend(device_t dev)
{
	PMF_TRANSITION_PRINTF(("%s: suspend enter\n", device_xname(dev)));
	if (!device_pmf_is_registered(dev))
		return false;
	PMF_TRANSITION_PRINTF2(1, ("%s: class suspend\n", device_xname(dev)));
	if (!device_pmf_class_suspend(dev))
		return false;
	PMF_TRANSITION_PRINTF2(1, ("%s: driver suspend\n", device_xname(dev)));
	if (!device_pmf_driver_suspend(dev))
		return false;
	PMF_TRANSITION_PRINTF2(1, ("%s: bus suspend\n", device_xname(dev)));
	if (!device_pmf_bus_suspend(dev))
		return false;
	PMF_TRANSITION_PRINTF(("%s: suspend exit\n", device_xname(dev)));
	return true;
}

bool
pmf_device_resume(device_t dev)
{
	PMF_TRANSITION_PRINTF(("%s: resume enter\n", device_xname(dev)));
	if (!device_pmf_is_registered(dev))
		return false;
	PMF_TRANSITION_PRINTF2(1, ("%s: bus resume\n", device_xname(dev)));
	if (!device_pmf_bus_resume(dev))
		return false;
	PMF_TRANSITION_PRINTF2(1, ("%s: driver resume\n", device_xname(dev)));
	if (!device_pmf_driver_resume(dev))
		return false;
	PMF_TRANSITION_PRINTF2(1, ("%s: class resume\n", device_xname(dev)));
	if (!device_pmf_class_resume(dev))
		return false;
	PMF_TRANSITION_PRINTF(("%s: resume exit\n", device_xname(dev)));
	return true;
}

bool
pmf_device_recursive_suspend(device_t dv)
{
	device_t curdev;

	if (!device_is_active(dv))
		return true;

	TAILQ_FOREACH(curdev, &alldevs, dv_list) {
		if (device_parent(curdev) != dv)
			continue;
		if (!pmf_device_recursive_suspend(curdev))
			return false;
	}

	return pmf_device_suspend(dv);
}

bool
pmf_device_recursive_resume(device_t dv)
{
	device_t parent;

	if (device_is_active(dv))
		return true;

	parent = device_parent(dv);
	if (parent != NULL) {
		if (!pmf_device_recursive_resume(parent))
			return false;
	}

	return pmf_device_resume(dv);
}

bool
pmf_device_resume_subtree(device_t dv)
{
	device_t curdev;

	if (!pmf_device_recursive_resume(dv))
		return false;

	TAILQ_FOREACH(curdev, &alldevs, dv_list) {
		if (device_parent(curdev) != dv)
			continue;
		if (!pmf_device_resume_subtree(curdev))
			return false;
	}
	return true;
}

#include <net/if.h>

static bool
pmf_class_network_suspend(device_t dev)
{
	struct ifnet *ifp = device_pmf_class_private(dev);
	int s;

	s = splnet();
	(*ifp->if_stop)(ifp, 1);
	splx(s);

	return true;
}

static bool
pmf_class_network_resume(device_t dev)
{
	struct ifnet *ifp = device_pmf_class_private(dev);
	int s;

	s = splnet();
	if (ifp->if_flags & IFF_UP) {
		ifp->if_flags &= ~IFF_RUNNING;
		(*ifp->if_init)(ifp);
		(*ifp->if_start)(ifp);
	}
	splx(s);

	return true;
}

void
pmf_class_network_register(device_t dev, struct ifnet *ifp)
{
	device_pmf_class_register(dev, ifp, pmf_class_network_suspend,
	    pmf_class_network_resume, NULL);
}

bool
pmf_event_inject(device_t dv, pmf_generic_event_t ev)
{
	pmf_event_workitem_t *pew;

	pew = malloc(sizeof(pmf_event_workitem_t), M_TEMP, M_NOWAIT);
	if (pew == NULL) {
		PMF_EVENT_PRINTF(("%s: PMF event %d dropped (no memory)\n",
		    dv ? device_xname(dv) : "<anonymous>", ev));
		return false;
	}

	pew->pew_event = ev;
	pew->pew_device = dv;

	workqueue_enqueue(pmf_event_workqueue, (void *)pew, NULL);
	PMF_EVENT_PRINTF(("%s: PMF event %d injected\n",
	    dv ? device_xname(dv) : "<anonymous>", ev));

	return true;
}

bool
pmf_event_register(device_t dv, pmf_generic_event_t ev,
    void (*handler)(device_t), bool global)
{
	pmf_event_handler_t *event; 
	
	event = malloc(sizeof(*event), M_DEVBUF, M_WAITOK);
	event->pmf_event = ev;
	event->pmf_handler = handler;
	event->pmf_device = dv;
	event->pmf_global = global;
	TAILQ_INSERT_TAIL(&pmf_all_events, event, pmf_link);

	return true;
}

void
pmf_event_deregister(device_t dv, pmf_generic_event_t ev,
    void (*handler)(device_t), bool global)
{
	pmf_event_handler_t *event;

	TAILQ_FOREACH(event, &pmf_all_events, pmf_link) {
		if (event->pmf_event != ev)
			continue;
		if (event->pmf_device != dv)
			continue;
		if (event->pmf_global != global)
			continue;
		if (event->pmf_handler != handler)
			continue;
		TAILQ_REMOVE(&pmf_all_events, event, pmf_link);
		free(event, M_WAITOK);
		return;
	}
}

struct display_class_softc {
	TAILQ_ENTRY(display_class_softc) dc_link;
	device_t dc_dev;
};

static TAILQ_HEAD(, display_class_softc) all_displays;
static callout_t global_idle_counter;
static int idle_timeout = 30;

static void
input_idle(void *dummy)
{
	PMF_IDLE_PRINTF(("Input idle handler called\n"));
	pmf_event_inject(NULL, PMFE_DISPLAY_OFF);
}

static void
input_activity_handler(device_t dv, devactive_t type)
{
	if (!TAILQ_EMPTY(&all_displays))
		callout_schedule(&global_idle_counter, idle_timeout * hz);
}

static void
pmf_class_input_deregister(device_t dv)
{
	device_active_deregister(dv, input_activity_handler);
}

bool
pmf_class_input_register(device_t dv)
{
	if (!device_active_register(dv, input_activity_handler))
		return false;
	
	device_pmf_class_register(dv, NULL, NULL, NULL,
	    pmf_class_input_deregister);

	return true;
}

static void
pmf_class_display_deregister(device_t dv)
{
	struct display_class_softc *sc = device_pmf_class_private(dv);
	int s;

	s = splsoftclock();
	TAILQ_REMOVE(&all_displays, sc, dc_link);
	if (TAILQ_EMPTY(&all_displays))
		callout_stop(&global_idle_counter);
	splx(s);

	free(sc, M_DEVBUF);
}

bool
pmf_class_display_register(device_t dv)
{
	struct display_class_softc *sc;
	int s;

	sc = malloc(sizeof(*sc), M_DEVBUF, M_WAITOK);

	s = splsoftclock();
	if (TAILQ_EMPTY(&all_displays))
		callout_schedule(&global_idle_counter, idle_timeout * hz);

	TAILQ_INSERT_HEAD(&all_displays, sc, dc_link);
	splx(s);

	device_pmf_class_register(dv, sc, NULL, NULL,
	    pmf_class_display_deregister);

	return true;
}

void
pmf_init(void)
{
	int err;

	KASSERT(pmf_event_workqueue == NULL);
	err = workqueue_create(&pmf_event_workqueue, "pmfevent",
	    pmf_event_worker, NULL, PRI_NONE, IPL_VM, 0);
	if (err)
		panic("couldn't create pmfevent workqueue");

	callout_init(&global_idle_counter, 0);
	callout_setfunc(&global_idle_counter, input_idle, NULL);
}
