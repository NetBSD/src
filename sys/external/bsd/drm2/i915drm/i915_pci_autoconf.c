/*	$NetBSD: i915_pci_autoconf.c,v 1.10 2021/12/19 12:28:12 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: i915_pci_autoconf.c,v 1.10 2021/12/19 12:28:12 riastradh Exp $");

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/workqueue.h>

#include <drm/drm_pci.h>

#include "i915_drv.h"
#include "i915_pci.h"

struct drm_device;

SIMPLEQ_HEAD(i915drmkms_task_head, i915drmkms_task);

struct i915drmkms_softc {
	device_t			sc_dev;
	struct pci_attach_args		sc_pa;
	struct lwp			*sc_task_thread;
	struct i915drmkms_task_head	sc_tasks;
	struct workqueue		*sc_task_wq;
	struct drm_device		*sc_drm_dev;
	struct pci_dev			sc_pci_dev;
};

static const struct pci_device_id *
		i915drmkms_pci_lookup(const struct pci_attach_args *);

static int	i915drmkms_match(device_t, cfdata_t, void *);
static void	i915drmkms_attach(device_t, device_t, void *);
static void	i915drmkms_attach_real(device_t);
static int	i915drmkms_detach(device_t, int);

static bool	i915drmkms_suspend(device_t, const pmf_qual_t *);
static bool	i915drmkms_resume(device_t, const pmf_qual_t *);

static void	i915drmkms_task_work(struct work *, void *);

CFATTACH_DECL_NEW(i915drmkms, sizeof(struct i915drmkms_softc),
    i915drmkms_match, i915drmkms_attach, i915drmkms_detach, NULL);

/* XXX Kludge to get these from i915_pci.c.  */
extern const struct pci_device_id *const i915_device_ids;
extern const size_t i915_n_device_ids;

static const struct pci_device_id *
i915drmkms_pci_lookup(const struct pci_attach_args *pa)
{
	size_t i;

	/* Attach only at function 0 to work around Intel lossage.  */
	if (pa->pa_function != 0)
		return NULL;

	/* We're interested only in Intel products.  */
	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return NULL;

	/* We're interested only in Intel display devices.  */
	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY)
		return NULL;

	for (i = 0; i < i915_n_device_ids; i++)
		if (PCI_PRODUCT(pa->pa_id) == i915_device_ids[i].device)
			break;

	/* Did we find it?  */
	if (i == i915_n_device_ids)
		return NULL;

	const struct pci_device_id *ent = &i915_device_ids[i];
	const struct intel_device_info *const info = (struct intel_device_info *) ent->driver_data;

	if (info->require_force_probe) {
		printf("i915drmkms: preliminary hardware support disabled\n");
		return NULL;
	}

	return ent;
}

static int
i915drmkms_match(device_t parent, cfdata_t match, void *aux)
{
	extern int i915drmkms_guarantee_initialized(void);
	const struct pci_attach_args *const pa = aux;
	int error;

	error = i915drmkms_guarantee_initialized();
	if (error) {
		aprint_error("i915drmkms: failed to initialize: %d\n", error);
		return 0;
	}

	if (i915drmkms_pci_lookup(pa) == NULL)
		return 0;

	return 6;		/* XXX Beat genfb_pci...  */
}

static void
i915drmkms_attach(device_t parent, device_t self, void *aux)
{
	struct i915drmkms_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = aux;
	int error;

	pci_aprint_devinfo(pa, NULL);

	/* Initialize the Linux PCI device descriptor.  */
	linux_pci_dev_init(&sc->sc_pci_dev, self, parent, pa, 0);

	sc->sc_dev = self;
	sc->sc_pa = *pa;
	sc->sc_task_thread = NULL;
	SIMPLEQ_INIT(&sc->sc_tasks);
	error = workqueue_create(&sc->sc_task_wq, "intelfb",
	    &i915drmkms_task_work, NULL, PRI_NONE, IPL_NONE, WQ_MPSAFE);
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
	config_mountroot(self, &i915drmkms_attach_real);
}

static void
i915drmkms_attach_real(device_t self)
{
	struct i915drmkms_softc *const sc = device_private(self);
	struct pci_attach_args *const pa = &sc->sc_pa;
	const struct pci_device_id *ent = i915drmkms_pci_lookup(pa);
	const struct intel_device_info *const info = (struct intel_device_info *) ent->driver_data;
	int error;

	KASSERT(info != NULL);

	/*
	 * Cause any tasks issued synchronously during attach to be
	 * processed at the end of this function.
	 */
	sc->sc_task_thread = curlwp;

	/* Attach the drm driver.  */
	/* XXX errno Linux->NetBSD */
	error = -i915_driver_probe(&sc->sc_pci_dev, ent);
	if (error) {
		aprint_error_dev(self, "unable to register drm: %d\n", error);
		return;
	}
	sc->sc_drm_dev = pci_get_drvdata(&sc->sc_pci_dev);

	/*
	 * Now that the drm driver is attached, we can safely suspend
	 * and resume.
	 */
	if (!pmf_device_register(self, &i915drmkms_suspend,
		&i915drmkms_resume))
		aprint_error_dev(self, "unable to establish power handler\n");

	/*
	 * Process asynchronous tasks queued synchronously during
	 * attach.  This will be for display detection to attach a
	 * framebuffer, so we have the opportunity for a console device
	 * to attach before autoconf has completed, in time for init(8)
	 * to find that console without panicking.
	 */
	while (!SIMPLEQ_EMPTY(&sc->sc_tasks)) {
		struct i915drmkms_task *const task =
		    SIMPLEQ_FIRST(&sc->sc_tasks);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_tasks, ift_u.queue);
		(*task->ift_fn)(task);
	}

	/* Cause any subesquent tasks to be processed by the workqueue.  */
	atomic_store_relaxed(&sc->sc_task_thread, NULL);
}

static int
i915drmkms_detach(device_t self, int flags)
{
	struct i915drmkms_softc *const sc = device_private(self);
	int error;

	/* XXX Check for in-use before tearing it all down...  */
	error = config_detach_children(self, flags);
	if (error)
		return error;

	KASSERT(sc->sc_task_thread == NULL);
	KASSERT(SIMPLEQ_EMPTY(&sc->sc_tasks));

	pmf_device_deregister(self);
	if (sc->sc_drm_dev) {
		i915_driver_remove(sc->sc_drm_dev->dev_private);
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
i915drmkms_suspend(device_t self, const pmf_qual_t *qual)
{
	struct i915drmkms_softc *const sc = device_private(self);
	struct drm_device *const dev = sc->sc_drm_dev;
	int ret;

	ret = i915_drm_suspend(dev);
	if (ret)
		return false;
	ret = i915_drm_suspend_late(dev, false);
	if (ret)
		return false;

	return true;
}

static bool
i915drmkms_resume(device_t self, const pmf_qual_t *qual)
{
	struct i915drmkms_softc *const sc = device_private(self);
	struct drm_device *const dev = sc->sc_drm_dev;
	int ret;

	ret = i915_drm_resume_early(dev);
	if (ret)
		return false;
	ret = i915_drm_resume(dev);
	if (ret)
		return false;

	return true;
}

static void
i915drmkms_task_work(struct work *work, void *cookie __unused)
{
	struct i915drmkms_task *const task = container_of(work,
	    struct i915drmkms_task, ift_u.work);

	(*task->ift_fn)(task);
}

int
i915drmkms_task_schedule(device_t self, struct i915drmkms_task *task)
{
	struct i915drmkms_softc *const sc = device_private(self);

	if (atomic_load_relaxed(&sc->sc_task_thread) == curlwp)
		SIMPLEQ_INSERT_TAIL(&sc->sc_tasks, task, ift_u.queue);
	else
		workqueue_enqueue(sc->sc_task_wq, &task->ift_u.work, NULL);

	return 0;
}
