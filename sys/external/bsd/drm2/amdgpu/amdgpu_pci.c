/*	$NetBSD: amdgpu_pci.c,v 1.11 2022/07/18 23:34:02 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amdgpu_pci.c,v 1.11 2022/07/18 23:34:02 riastradh Exp $");

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/workqueue.h>

#include <dev/pci/pcivar.h>

#include <linux/pci.h>

#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_pci.h>

#include <amdgpu.h>
#include "amdgpu_drv.h"
#include "amdgpu_task.h"

struct drm_device;

SIMPLEQ_HEAD(amdgpu_task_head, amdgpu_task);

struct amdgpu_softc {
	device_t			sc_dev;
	struct pci_attach_args		sc_pa;
	struct lwp			*sc_task_thread;
	struct amdgpu_task_head		sc_tasks;
	struct workqueue		*sc_task_wq;
	struct drm_device		*sc_drm_dev;
	struct pci_dev			sc_pci_dev;
	bool				sc_pci_attached;
	bool				sc_dev_registered;
};

static bool	amdgpu_pci_lookup(const struct pci_attach_args *,
		    unsigned long *);

static int	amdgpu_match(device_t, cfdata_t, void *);
static void	amdgpu_attach(device_t, device_t, void *);
static void	amdgpu_attach_real(device_t);
static int	amdgpu_detach(device_t, int);
static bool	amdgpu_do_suspend(device_t, const pmf_qual_t *);
static bool	amdgpu_do_resume(device_t, const pmf_qual_t *);

static void	amdgpu_task_work(struct work *, void *);

CFATTACH_DECL_NEW(amdgpu, sizeof(struct amdgpu_softc),
    amdgpu_match, amdgpu_attach, amdgpu_detach, NULL);

/* XXX Kludge to get these from amdgpu_drv.c.  */
extern struct drm_driver *const amdgpu_drm_driver;
extern const struct pci_device_id *const amdgpu_device_ids;
extern const size_t amdgpu_n_device_ids;

static bool
amdgpu_pci_lookup(const struct pci_attach_args *pa, unsigned long *flags)
{
	size_t i;

	for (i = 0; i < amdgpu_n_device_ids; i++) {
		if ((PCI_VENDOR(pa->pa_id) == amdgpu_device_ids[i].vendor) &&
		    (PCI_PRODUCT(pa->pa_id) == amdgpu_device_ids[i].device))
			break;
	}

	/* Did we find it?  */
	if (i == amdgpu_n_device_ids)
		return false;

	if (flags)
		*flags = amdgpu_device_ids[i].driver_data;
	return true;
}

static int
amdgpu_match(device_t parent, cfdata_t match, void *aux)
{
	extern int amdgpu_guarantee_initialized(void);
	const struct pci_attach_args *const pa = aux;
	int error;

	error = amdgpu_guarantee_initialized();
	if (error) {
		aprint_error("amdgpu: failed to initialize: %d\n", error);
		return 0;
	}

	if (!amdgpu_pci_lookup(pa, NULL))
		return 0;

	return 7;		/* beat genfb_pci and radeon  */
}

static void
amdgpu_attach(device_t parent, device_t self, void *aux)
{
	struct amdgpu_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = aux;
	int error;

	pci_aprint_devinfo(pa, NULL);

	/* Initialize the Linux PCI device descriptor.  */
	linux_pci_dev_init(&sc->sc_pci_dev, self, device_parent(self), pa, 0);

	sc->sc_dev = self;
	sc->sc_pa = *pa;
	sc->sc_task_thread = NULL;
	SIMPLEQ_INIT(&sc->sc_tasks);
	error = workqueue_create(&sc->sc_task_wq, "amdgpufb",
	    &amdgpu_task_work, NULL, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	if (error) {
		aprint_error_dev(self, "unable to create workqueue: %d\n",
		    error);
		sc->sc_task_wq = NULL;
		return;
	}

	/*
	 * Defer the remainder of initialization until we have mounted
	 * the root file system and can load firmware images.
	 */
	config_mountroot(self, &amdgpu_attach_real);
}

static void
amdgpu_attach_real(device_t self)
{
	struct amdgpu_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = &sc->sc_pa;
	bool ok __diagused;
	unsigned long flags = 0; /* XXXGCC */
	int error;

	ok = amdgpu_pci_lookup(pa, &flags);
	KASSERT(ok);

	/*
	 * Cause any tasks issued synchronously during attach to be
	 * processed at the end of this function.
	 */
	sc->sc_task_thread = curlwp;

	sc->sc_drm_dev = drm_dev_alloc(amdgpu_drm_driver, self);
	if (IS_ERR(sc->sc_drm_dev)) {
		aprint_error_dev(self, "unable to create drm device: %ld\n",
		    PTR_ERR(sc->sc_drm_dev));
		sc->sc_drm_dev = NULL;
		goto out;
	}

	/* XXX errno Linux->NetBSD */
	error = -drm_pci_attach(sc->sc_drm_dev, &sc->sc_pci_dev);
	if (error) {
		aprint_error_dev(self, "unable to attach drm: %d\n", error);
		goto out;
	}
	sc->sc_pci_attached = true;

	/* XXX errno Linux->NetBSD */
	error = -drm_dev_register(sc->sc_drm_dev, flags);
	if (error) {
		aprint_error_dev(self, "unable to register drm: %d\n", error);
		goto out;
	}
	sc->sc_dev_registered = true;

	if (!pmf_device_register(self, &amdgpu_do_suspend, &amdgpu_do_resume))
		aprint_error_dev(self, "unable to establish power handler\n");

	/*
	 * Process asynchronous tasks queued synchronously during
	 * attach.  This will be for display detection to attach a
	 * framebuffer, so we have the opportunity for a console device
	 * to attach before autoconf has completed, in time for init(8)
	 * to find that console without panicking.
	 */
	while (!SIMPLEQ_EMPTY(&sc->sc_tasks)) {
		struct amdgpu_task *const task = SIMPLEQ_FIRST(&sc->sc_tasks);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_tasks, rt_u.queue);
		(*task->rt_fn)(task);
	}

out:	/* Cause any subesquent tasks to be processed by the workqueue.  */
	atomic_store_relaxed(&sc->sc_task_thread, NULL);
}

static int
amdgpu_detach(device_t self, int flags)
{
	struct amdgpu_softc *const sc = device_private(self);
	int error;

	/* XXX Check for in-use before tearing it all down...  */
	error = config_detach_children(self, flags);
	if (error)
		return error;

	KASSERT(sc->sc_task_thread == NULL);
	KASSERT(SIMPLEQ_EMPTY(&sc->sc_tasks));

	pmf_device_deregister(self);
	if (sc->sc_dev_registered)
		drm_dev_unregister(sc->sc_drm_dev);
	if (sc->sc_pci_attached)
		drm_pci_detach(sc->sc_drm_dev);
	if (sc->sc_drm_dev) {
		drm_dev_put(sc->sc_drm_dev);
		sc->sc_drm_dev = NULL;
	}
	if (sc->sc_task_wq) {
		workqueue_destroy(sc->sc_task_wq);
		sc->sc_task_wq = NULL;
	}
	linux_pci_dev_destroy(&sc->sc_pci_dev);

	return 0;
}

static bool
amdgpu_do_suspend(device_t self, const pmf_qual_t *qual)
{
	struct amdgpu_softc *const sc = device_private(self);
	struct drm_device *const dev = sc->sc_drm_dev;
	int ret;

	ret = amdgpu_device_suspend(dev, /*fbcon*/true);
	if (ret)
		return false;

	return true;
}

static bool
amdgpu_do_resume(device_t self, const pmf_qual_t *qual)
{
	struct amdgpu_softc *const sc = device_private(self);
	struct drm_device *const dev = sc->sc_drm_dev;
	int ret;

	ret = amdgpu_device_resume(dev, /*fbcon*/true);
	if (ret)
		return false;

	return true;
}

static void
amdgpu_task_work(struct work *work, void *cookie __unused)
{
	struct amdgpu_task *const task = container_of(work, struct amdgpu_task,
	    rt_u.work);

	(*task->rt_fn)(task);
}

void
amdgpu_task_schedule(device_t self, struct amdgpu_task *task)
{
	struct amdgpu_softc *const sc = device_private(self);

	if (atomic_load_relaxed(&sc->sc_task_thread) == curlwp)
		SIMPLEQ_INSERT_TAIL(&sc->sc_tasks, task, rt_u.queue);
	else
		workqueue_enqueue(sc->sc_task_wq, &task->rt_u.work, NULL);
}
