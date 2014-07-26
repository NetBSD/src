/*	$NetBSD: radeon_pci.c,v 1.4 2014/07/26 07:36:09 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: radeon_pci.c,v 1.4 2014/07/26 07:36:09 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "vga.h"
#endif

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/workqueue.h>

#include <dev/pci/pciio.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/pci/wsdisplay_pci.h>
#include <dev/wsfb/genfbvar.h>

#if NVGA > 0
/*
 * XXX All we really need is vga_is_console from vgavar.h, but the
 * header files are missing their own dependencies, so we need to
 * explicitly drag in the other crap.
 */
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include <drm/drmP.h>
#include <drm/drm_fb_helper.h>

#include <radeon.h>
#include "radeon_drv.h"
#include "radeon_task.h"

SIMPLEQ_HEAD(radeon_task_head, radeon_task);

struct radeon_softc {
	device_t			sc_dev;
	struct pci_attach_args		sc_pa;
	enum {
		RADEON_TASK_ATTACH,
		RADEON_TASK_WORKQUEUE,
	}				sc_task_state;
	union {
		struct workqueue		*workqueue;
		struct radeon_task_head		attach;
	}				sc_task_u;
	struct drm_device		*sc_drm_dev;
	struct pci_dev			sc_pci_dev;
};

struct radeon_device *
radeon_device_private(device_t self)
{
	struct radeon_softc *const sc = device_private(self);

	return sc->sc_drm_dev->dev_private;
}

static bool	radeon_pci_lookup(const struct pci_attach_args *,
		    unsigned long *);

static int	radeon_match(device_t, cfdata_t, void *);
static void	radeon_attach(device_t, device_t, void *);
static void	radeon_attach_real(device_t);
static int	radeon_detach(device_t, int);

static void	radeon_task_work(struct work *, void *);

CFATTACH_DECL_NEW(radeon, sizeof(struct radeon_softc),
    radeon_match, radeon_attach, radeon_detach, NULL);

/* XXX Kludge to get these from radeon_drv.c.  */
extern struct drm_driver *const radeon_drm_driver;
extern const struct pci_device_id *const radeon_device_ids;
extern const size_t radeon_n_device_ids;

static bool
radeon_pci_lookup(const struct pci_attach_args *pa, unsigned long *flags)
{
	size_t i;

	for (i = 0; i < radeon_n_device_ids; i++) {
		if ((PCI_VENDOR(pa->pa_id) == radeon_device_ids[i].vendor) &&
		    (PCI_PRODUCT(pa->pa_id) == radeon_device_ids[i].device))
			break;
	}

	/* Did we find it?  */
	if (i == radeon_n_device_ids)
		return false;

	if (flags)
		*flags = radeon_device_ids[i].driver_data;
	return true;
}

static int
radeon_match(device_t parent, cfdata_t match, void *aux)
{
	extern int radeon_guarantee_initialized(void);
	const struct pci_attach_args *const pa = aux;
	int error;

	error = radeon_guarantee_initialized();
	if (error) {
		aprint_error("radeon: failed to initialize: %d\n", error);
		return 0;
	}

	if (!radeon_pci_lookup(pa, NULL))
		return 0;

	return 6;		/* XXX Beat genfb_pci...  */
}

static void
radeon_attach(device_t parent, device_t self, void *aux)
{
	struct radeon_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = aux;

	pci_aprint_devinfo(pa, NULL);

	/*
	 * Trivial initialization first; the rest will come after we
	 * have mounted the root file system and can load firmware
	 * images.
	 */
	sc->sc_dev = NULL;
	sc->sc_pa = *pa;

	config_mountroot(self, &radeon_attach_real);
}

static void
radeon_attach_real(device_t self)
{
	struct radeon_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = &sc->sc_pa;
	bool ok __diagused;
	unsigned long flags;
	int error;

	ok = radeon_pci_lookup(pa, &flags);
	KASSERT(ok);

	sc->sc_task_state = RADEON_TASK_ATTACH;
	SIMPLEQ_INIT(&sc->sc_task_u.attach);

	/* XXX errno Linux->NetBSD */
	error = -drm_pci_attach(self, pa, &sc->sc_pci_dev, radeon_drm_driver,
	    flags, &sc->sc_drm_dev);
	if (error) {
		aprint_error_dev(self, "unable to attach drm: %d\n", error);
		goto out;
	}

	while (!SIMPLEQ_EMPTY(&sc->sc_task_u.attach)) {
		struct radeon_task *const task =
		    SIMPLEQ_FIRST(&sc->sc_task_u.attach);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_task_u.attach, rt_u.queue);
		(*task->rt_fn)(task);
	}

	sc->sc_task_state = RADEON_TASK_WORKQUEUE;
	error = workqueue_create(&sc->sc_task_u.workqueue, "radeonfb",
	    &radeon_task_work, NULL, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	if (error) {
		aprint_error_dev(self, "unable to create workqueue: %d\n",
		    error);
		sc->sc_task_u.workqueue = NULL;
		goto out;
	}

out:	sc->sc_dev = self;
}

static int
radeon_detach(device_t self, int flags)
{
	struct radeon_softc *const sc = device_private(self);
	int error;

	if (sc->sc_dev == NULL)
		/* Not done attaching.  */
		return EBUSY;

	/* XXX Check for in-use before tearing it all down...  */
	error = config_detach_children(self, flags);
	if (error)
		return error;

	if (sc->sc_task_state == RADEON_TASK_ATTACH)
		return 0;
	if (sc->sc_task_u.workqueue != NULL) {
		workqueue_destroy(sc->sc_task_u.workqueue);
		sc->sc_task_u.workqueue = NULL;
	}

	if (sc->sc_drm_dev == NULL)
		return 0;
	/* XXX errno Linux->NetBSD */
	error = -drm_pci_detach(sc->sc_drm_dev, flags);
	if (error)
		/* XXX Kinda too late to fail now...  */
		return error;
	sc->sc_drm_dev = NULL;

	return 0;
}

static void
radeon_task_work(struct work *work, void *cookie __unused)
{
	struct radeon_task *const task = container_of(work, struct radeon_task,
	    rt_u.work);

	(*task->rt_fn)(task);
}

int
radeon_task_schedule(device_t self, struct radeon_task *task)
{
	struct radeon_softc *const sc = device_private(self);

	switch (sc->sc_task_state) {
	case RADEON_TASK_ATTACH:
		SIMPLEQ_INSERT_TAIL(&sc->sc_task_u.attach, task, rt_u.queue);
		return 0;
	case RADEON_TASK_WORKQUEUE:
		if (sc->sc_task_u.workqueue == NULL) {
			aprint_error_dev(self, "unable to schedule task\n");
			return EIO;
		}
		workqueue_enqueue(sc->sc_task_u.workqueue, &task->rt_u.work,
		    NULL);
		return 0;
	default:
		panic("radeon in invalid task state: %d\n",
		    (int)sc->sc_task_state);
	}
}
