/*	$NetBSD: nouveau_pci.c,v 1.3.4.2 2015/04/06 15:18:17 skrll Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: nouveau_pci.c,v 1.3.4.2 2015/04/06 15:18:17 skrll Exp $");

#include <sys/types.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/workqueue.h>

#include <drm/drmP.h>

#include <engine/device.h>

#include "nouveau_drm.h"
#include "nouveau_pci.h"

SIMPLEQ_HEAD(nouveau_task_head, nouveau_task);

struct nouveau_softc {
	device_t		sc_dev;
	enum {
		NOUVEAU_TASK_ATTACH,
		NOUVEAU_TASK_WORKQUEUE,
	}			sc_task_state;
	union {
		struct workqueue		*workqueue;
		struct nouveau_task_head	attach;
	}			sc_task_u;
	struct drm_device	*sc_drm_dev;
	struct pci_dev		sc_pci_dev;
	struct nouveau_device	*sc_nv_dev;
};

static int	nouveau_match(device_t, cfdata_t, void *);
static void	nouveau_attach(device_t, device_t, void *);
static int	nouveau_detach(device_t, int);

static bool	nouveau_suspend(device_t, const pmf_qual_t *);
static bool	nouveau_resume(device_t, const pmf_qual_t *);

static void	nouveau_task_work(struct work *, void *);

CFATTACH_DECL_NEW(nouveau, sizeof(struct nouveau_softc),
    nouveau_match, nouveau_attach, nouveau_detach, NULL);

/* Kludge to get this from nouveau_drm.c.  */
extern struct drm_driver *const nouveau_drm_driver;

static int
nouveau_match(device_t parent, cfdata_t match, void *aux)
{
	extern int nouveau_guarantee_initialized(void);
	const struct pci_attach_args *const pa = aux;
	int error;

	error = nouveau_guarantee_initialized();
	if (error) {
		aprint_error("nouveau: failed to initialize: %d\n", error);
		return 0;
	}

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_NVIDIA &&
	    PCI_VENDOR(pa->pa_id) != PCI_VENDOR_NVIDIA_SGS)
		return 0;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY)
		return 0;

	return 6;		/* XXX Beat genfb_pci...  */
}

extern char *nouveau_config;
extern char *nouveau_debug;

static void
nouveau_attach(device_t parent, device_t self, void *aux)
{
	struct nouveau_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = aux;
	uint64_t devname;
	int error;

	pci_aprint_devinfo(pa, NULL);

	sc->sc_dev = self;

	if (!pmf_device_register(self, &nouveau_suspend,
		&nouveau_resume))
		aprint_error_dev(self, "unable to establish power handler\n");

	sc->sc_task_state = NOUVEAU_TASK_ATTACH;
	SIMPLEQ_INIT(&sc->sc_task_u.attach);

	devname = (uint64_t)device_unit(device_parent(self)) << 32;
	devname |= pa->pa_bus << 16;
	/* XXX errno Linux->NetBSD */
	error = -nouveau_device_create(&sc->sc_pci_dev, NOUVEAU_BUS_PCI,
	    devname, device_xname(self), nouveau_config, nouveau_debug,
	    &sc->sc_nv_dev);
	if (error) {
		aprint_error_dev(self, "unable to create nouveau device: %d\n",
		    error);
		return;
	}

	/* XXX errno Linux->NetBSD */
	error = -drm_pci_attach(self, pa, &sc->sc_pci_dev, nouveau_drm_driver,
	    0, &sc->sc_drm_dev);
	if (error) {
		aprint_error_dev(self, "unable to attach drm: %d\n", error);
		return;
	}

	while (!SIMPLEQ_EMPTY(&sc->sc_task_u.attach)) {
		struct nouveau_task *const task =
		    SIMPLEQ_FIRST(&sc->sc_task_u.attach);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_task_u.attach, nt_u.queue);
		(*task->nt_fn)(task);
	}

	sc->sc_task_state = NOUVEAU_TASK_WORKQUEUE;
	error = workqueue_create(&sc->sc_task_u.workqueue, "intelfb",
	    &nouveau_task_work, NULL, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	if (error) {
		aprint_error_dev(self, "unable to create workqueue: %d\n",
		    error);
		sc->sc_task_u.workqueue = NULL;
		return;
	}
}

static int
nouveau_detach(device_t self, int flags)
{
	struct nouveau_softc *const sc = device_private(self);
	int error;

	/* XXX Check for in-use before tearing it all down...  */
	error = config_detach_children(self, flags);
	if (error)
		return error;

	if (sc->sc_task_state == NOUVEAU_TASK_ATTACH)
		goto out0;
	if (sc->sc_task_u.workqueue != NULL) {
		workqueue_destroy(sc->sc_task_u.workqueue);
		sc->sc_task_u.workqueue = NULL;
	}

	if (sc->sc_nv_dev == NULL)
		goto out0;

	if (sc->sc_drm_dev == NULL)
		goto out1;
	/* XXX errno Linux->NetBSD */
	error = -drm_pci_detach(sc->sc_drm_dev, flags);
	if (error)
		/* XXX Kinda too late to fail now...  */
		return error;
	sc->sc_drm_dev = NULL;

out1:	nouveau_object_ref(NULL, (void *)&sc->sc_nv_dev);
out0:	pmf_device_deregister(self);
	return 0;
}

/*
 * XXX Synchronize with nouveau_do_suspend in nouveau_drm.c.
 */
static bool
nouveau_suspend(device_t self, const pmf_qual_t *qual __unused)
{
	struct nouveau_softc *const sc = device_private(self);
	struct device *const dev = &sc->sc_pci_dev.dev; /* XXX KLUDGE */

	return nouveau_pmops_suspend(dev) == 0;
}

static bool
nouveau_resume(device_t self, const pmf_qual_t *qual)
{
	struct nouveau_softc *const sc = device_private(self);
	struct device *const dev = &sc->sc_pci_dev.dev; /* XXX KLUDGE */

	return nouveau_pmops_resume(dev) == 0;
}

static void
nouveau_task_work(struct work *work, void *cookie __unused)
{
	struct nouveau_task *const task = container_of(work,
	    struct nouveau_task, nt_u.work);

	(*task->nt_fn)(task);
}

int
nouveau_task_schedule(device_t self, struct nouveau_task *task)
{
	struct nouveau_softc *const sc = device_private(self);

	switch (sc->sc_task_state) {
	case NOUVEAU_TASK_ATTACH:
		SIMPLEQ_INSERT_TAIL(&sc->sc_task_u.attach, task, nt_u.queue);
		return 0;
	case NOUVEAU_TASK_WORKQUEUE:
		if (sc->sc_task_u.workqueue == NULL) {
			aprint_error_dev(self, "unable to schedule task\n");
			return EIO;
		}
		workqueue_enqueue(sc->sc_task_u.workqueue, &task->nt_u.work,
		    NULL);
		return 0;
	default:
		panic("nouveau in invalid task state: %d\n",
		    (int)sc->sc_task_state);
	}
}
