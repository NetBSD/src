/*	$NetBSD: vmwgfx_pci.c,v 1.1 2022/02/17 01:21:03 riastradh Exp $	*/

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: vmwgfx_pci.c,v 1.1 2022/02/17 01:21:03 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "vga.h"
#endif

#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/workqueue.h>

#include <dev/pci/pciio.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/pci/wsdisplay_pci.h>
#include <dev/wsfb/genfbvar.h>

#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_pci.h>

#include "vmwgfx_drv.h"
#include "vmwgfx_task.h"

SIMPLEQ_HEAD(vmwgfx_task_head, vmwgfx_task);

struct vmwgfx_softc {
	device_t			sc_dev;
	struct pci_attach_args		sc_pa;
	struct lwp			*sc_task_thread;
	struct vmwgfx_task_head		sc_tasks;
	struct workqueue		*sc_task_wq;
	struct drm_device		*sc_drm_dev;
	struct pci_dev			sc_pci_dev;
	bool				sc_pci_attached;
	bool				sc_dev_registered;
#if defined(__i386__)
#define VMWGFX_PCI_UGLY_MAP_HACK
	/* XXX Used to claim the VGA device before attach_real */
	bus_space_handle_t		sc_temp_memh;
	bool				sc_temp_set;
#endif
};

static bool	vmwgfx_pci_lookup(const struct pci_attach_args *,
		    unsigned long *);

static int	vmwgfx_match(device_t, cfdata_t, void *);
static void	vmwgfx_attach(device_t, device_t, void *);
static void	vmwgfx_attach_real(device_t);
static int	vmwgfx_detach(device_t, int);
static bool	vmwgfx_do_suspend(device_t, const pmf_qual_t *);
static bool	vmwgfx_do_resume(device_t, const pmf_qual_t *);

static void	vmwgfx_task_work(struct work *, void *);

CFATTACH_DECL_NEW(vmwgfx, sizeof(struct vmwgfx_softc),
    vmwgfx_match, vmwgfx_attach, vmwgfx_detach, NULL);

/* XXX Kludge to get these from vmwgfx_drv.c.  */
extern struct drm_driver *const vmwgfx_drm_driver;
extern const struct pci_device_id *const vmwgfx_device_ids;
extern const size_t vmwgfx_n_device_ids;

static bool
vmwgfx_pci_lookup(const struct pci_attach_args *pa, unsigned long *flags)
{
	size_t i;

	for (i = 0; i < vmwgfx_n_device_ids; i++) {
		if ((PCI_VENDOR(pa->pa_id) == vmwgfx_device_ids[i].vendor) &&
		    (PCI_PRODUCT(pa->pa_id) == vmwgfx_device_ids[i].device))
			break;
	}

	/* Did we find it?  */
	if (i == vmwgfx_n_device_ids)
		return false;

	if (flags)
		*flags = vmwgfx_device_ids[i].driver_data;
	return true;
}

static int
vmwgfx_match(device_t parent, cfdata_t match, void *aux)
{
	extern int vmwgfx_guarantee_initialized(void);
	const struct pci_attach_args *const pa = aux;
	int error;

	error = vmwgfx_guarantee_initialized();
	if (error) {
		aprint_error("vmwgfx: failed to initialize: %d\n", error);
		return 0;
	}

	if (!vmwgfx_pci_lookup(pa, NULL))
		return 0;

	return 6;		/* XXX Beat genfb_pci...  */
}

static void
vmwgfx_attach(device_t parent, device_t self, void *aux)
{
	struct vmwgfx_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = aux;
	int error;

	pci_aprint_devinfo(pa, NULL);

	/* Initialize the Linux PCI device descriptor.  */
	linux_pci_dev_init(&sc->sc_pci_dev, self, device_parent(self), pa, 0);

	sc->sc_dev = self;
	sc->sc_pa = *pa;
	sc->sc_task_thread = NULL;
	SIMPLEQ_INIT(&sc->sc_tasks);
	error = workqueue_create(&sc->sc_task_wq, "vmwgfxfb",
	    &vmwgfx_task_work, NULL, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	if (error) {
		aprint_error_dev(self, "unable to create workqueue: %d\n",
		    error);
		sc->sc_task_wq = NULL;
		return;
	}

#ifdef VMWGFX_PCI_UGLY_MAP_HACK
	/*
	 * XXX
	 * We try to map the VGA registers, in case we can prevent vga@isa or
	 * pcdisplay@isa attaching, and stealing wsdisplay0.  This only works
	 * with serial console, as actual VGA console has already mapped them.
	 * The only way to handle that is for vga@isa to not attach.
	 */
	int rv = bus_space_map(pa->pa_memt, 0xb0000, 0x10000, 0,
			       &sc->sc_temp_memh);
	sc->sc_temp_set = rv == 0;
	if (rv != 0)
		aprint_error_dev(self, "unable to reserve VGA registers for "
				       "i386 vmwgfxdrmkms hack\n");
#endif

	/*
	 * Defer the remainder of initialization until we have mounted
	 * the root file system and can load firmware images.
	 */
	config_mountroot(self, &vmwgfx_attach_real);
}

static void
vmwgfx_attach_real(device_t self)
{
	struct vmwgfx_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = &sc->sc_pa;
	bool ok __diagused;
	unsigned long flags = 0 /*XXXGCC*/;
	int error;

	ok = vmwgfx_pci_lookup(pa, &flags);
	KASSERT(ok);

#ifdef VMWGFX_PCI_UGLY_MAP_HACK
	/*
	 * XXX
	 * Unmap the VGA registers.
	 */
	if (sc->sc_temp_set)
		bus_space_unmap(pa->pa_memt, sc->sc_temp_memh, 0x10000);
#endif

	/*
	 * Cause any tasks issued synchronously during attach to be
	 * processed at the end of this function.
	 */
	sc->sc_task_thread = curlwp;

	sc->sc_drm_dev = drm_dev_alloc(vmwgfx_drm_driver, self);
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

	if (!pmf_device_register(self, &vmwgfx_do_suspend, &vmwgfx_do_resume))
		aprint_error_dev(self, "unable to establish power handler\n");

	/*
	 * Process asynchronous tasks queued synchronously during
	 * attach.  This will be for display detection to attach a
	 * framebuffer, so we have the opportunity for a console device
	 * to attach before autoconf has completed, in time for init(8)
	 * to find that console without panicking.
	 */
	while (!SIMPLEQ_EMPTY(&sc->sc_tasks)) {
		struct vmwgfx_task *const task = SIMPLEQ_FIRST(&sc->sc_tasks);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_tasks, vt_u.queue);
		(*task->vt_fn)(task);
	}

out:	/* Cause any subesquent tasks to be processed by the workqueue.  */
	atomic_store_relaxed(&sc->sc_task_thread, NULL);
}

static int
vmwgfx_detach(device_t self, int flags)
{
	struct vmwgfx_softc *const sc = device_private(self);
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
vmwgfx_do_suspend(device_t self, const pmf_qual_t *qual)
{
	struct vmwgfx_softc *const sc = device_private(self);
	struct drm_device *const dev = sc->sc_drm_dev;
	int ret;

	ret = vmw_kms_suspend(dev);
	if (ret)
		return false;

	return true;
}

static bool
vmwgfx_do_resume(device_t self, const pmf_qual_t *qual)
{
	struct vmwgfx_softc *const sc = device_private(self);
	struct drm_device *const dev = sc->sc_drm_dev;
	int ret;

	ret = vmw_kms_resume(dev);
	if (ret)
		return false;

	return true;
}

static void
vmwgfx_task_work(struct work *work, void *cookie __unused)
{
	struct vmwgfx_task *const task = container_of(work, struct vmwgfx_task,
	    vt_u.work);

	(*task->vt_fn)(task);
}

int
vmwgfx_task_schedule(device_t self, struct vmwgfx_task *task)
{
	struct vmwgfx_softc *const sc = device_private(self);

	if (atomic_load_relaxed(&sc->sc_task_thread) == curlwp)
		SIMPLEQ_INSERT_TAIL(&sc->sc_tasks, task, vt_u.queue);
	else
		workqueue_enqueue(sc->sc_task_wq, &task->vt_u.work, NULL);

	return 0;
}
