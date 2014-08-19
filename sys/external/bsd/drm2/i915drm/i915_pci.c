/*	$NetBSD: i915_pci.c,v 1.15.4.2 2014/08/20 00:04:20 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: i915_pci.c,v 1.15.4.2 2014/08/20 00:04:20 tls Exp $");

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/workqueue.h>

#include <drm/drmP.h>

#include "i915_drv.h"
#include "i915_pci.h"

SIMPLEQ_HEAD(i915drmkms_task_head, i915drmkms_task);

struct i915drmkms_softc {
	device_t			sc_dev;
	enum {
		I915DRMKMS_TASK_ATTACH,
		I915DRMKMS_TASK_WORKQUEUE,
	}				sc_task_state;
	union {
		struct workqueue		*workqueue;
		struct i915drmkms_task_head	attach;
	}				sc_task_u;
	struct drm_device		*sc_drm_dev;
	struct pci_dev			sc_pci_dev;
};

static const struct intel_device_info *
		i915drmkms_pci_lookup(const struct pci_attach_args *);

static int	i915drmkms_match(device_t, cfdata_t, void *);
static void	i915drmkms_attach(device_t, device_t, void *);
static int	i915drmkms_detach(device_t, int);

static bool	i915drmkms_suspend(device_t, const pmf_qual_t *);
static bool	i915drmkms_resume(device_t, const pmf_qual_t *);

static void	i915drmkms_task_work(struct work *, void *);

CFATTACH_DECL_NEW(i915drmkms, sizeof(struct i915drmkms_softc),
    i915drmkms_match, i915drmkms_attach, i915drmkms_detach, NULL);

/* XXX Kludge to get these from i915_drv.c.  */
extern struct drm_driver *const i915_drm_driver;
extern const struct pci_device_id *const i915_device_ids;
extern const size_t i915_n_device_ids;

static const struct intel_device_info *
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

	const struct intel_device_info *const info =
	    (const void *)(uintptr_t)i915_device_ids[i].driver_data;

	if (IS_PRELIMINARY_HW(info)) {
		printf("i915drmkms: preliminary hardware support disabled\n");
		return NULL;
	}

	return info;
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
	const struct intel_device_info *const info = i915drmkms_pci_lookup(pa);
	const unsigned long cookie =
	    (unsigned long)(uintptr_t)(const void *)info;
	int error;

	KASSERT(info != NULL);

	sc->sc_dev = self;

	pci_aprint_devinfo(pa, NULL);

	if (!pmf_device_register(self, &i915drmkms_suspend,
		&i915drmkms_resume))
		aprint_error_dev(self, "unable to establish power handler\n");

	sc->sc_task_state = I915DRMKMS_TASK_ATTACH;
	SIMPLEQ_INIT(&sc->sc_task_u.attach);

	/* XXX errno Linux->NetBSD */
	error = -drm_pci_attach(self, pa, &sc->sc_pci_dev, i915_drm_driver,
	    cookie, &sc->sc_drm_dev);
	if (error) {
		aprint_error_dev(self, "unable to attach drm: %d\n", error);
		return;
	}

	while (!SIMPLEQ_EMPTY(&sc->sc_task_u.attach)) {
		struct i915drmkms_task *const task =
		    SIMPLEQ_FIRST(&sc->sc_task_u.attach);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_task_u.attach, ift_u.queue);
		(*task->ift_fn)(task);
	}

	sc->sc_task_state = I915DRMKMS_TASK_WORKQUEUE;
	error = workqueue_create(&sc->sc_task_u.workqueue, "intelfb",
	    &i915drmkms_task_work, NULL, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	if (error) {
		aprint_error_dev(self, "unable to create workqueue: %d\n",
		    error);
		sc->sc_task_u.workqueue = NULL;
		return;
	}
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

	if (sc->sc_task_state == I915DRMKMS_TASK_ATTACH)
		goto out;
	if (sc->sc_task_u.workqueue != NULL) {
		workqueue_destroy(sc->sc_task_u.workqueue);
		sc->sc_task_u.workqueue = NULL;
	}

	if (sc->sc_drm_dev == NULL)
		goto out;
	/* XXX errno Linux->NetBSD */
	error = -drm_pci_detach(sc->sc_drm_dev, flags);
	if (error)
		/* XXX Kinda too late to fail now...  */
		return error;
	sc->sc_drm_dev = NULL;

out:	pmf_device_deregister(self);
	return 0;
}

static bool
i915drmkms_suspend(device_t self, const pmf_qual_t *qual)
{
	struct i915drmkms_softc *const sc = device_private(self);
	struct drm_device *const dev = sc->sc_drm_dev;
	int ret;

	if (dev == NULL)
		return true;

	ret = i915_drm_freeze(dev);
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

	if (dev == NULL)
		return true;

	ret = i915_drm_thaw_early(dev);
	if (ret)
		return false;
	ret = i915_drm_thaw(dev);
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

	switch (sc->sc_task_state) {
	case I915DRMKMS_TASK_ATTACH:
		SIMPLEQ_INSERT_TAIL(&sc->sc_task_u.attach, task, ift_u.queue);
		return 0;
	case I915DRMKMS_TASK_WORKQUEUE:
		if (sc->sc_task_u.workqueue == NULL) {
			aprint_error_dev(self, "unable to schedule task\n");
			return EIO;
		}
		workqueue_enqueue(sc->sc_task_u.workqueue, &task->ift_u.work,
		    NULL);
		return 0;
	default:
		panic("i915drmkms in invalid task state: %d\n",
		    (int)sc->sc_task_state);
	}
}
