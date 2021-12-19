/*	$NetBSD: nouveau_pci.c,v 1.30 2021/12/19 11:05:13 riastradh Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nouveau_pci.c,v 1.30 2021/12/19 11:05:13 riastradh Exp $");

#ifdef _KERNEL_OPT
#if defined(__arm__) || defined(__aarch64__)
#include "opt_fdt.h"
#endif
#endif

#include <sys/types.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/workqueue.h>
#include <sys/module.h>

#ifdef FDT
#include <dev/fdt/fdtvar.h>
#endif

#include <drm/drm_pci.h>

#include <core/device.h>
#include <core/pci.h>

#include "nouveau_drv.h"
#include "nouveau_pci.h"

struct drm_device;

MODULE(MODULE_CLASS_DRIVER, nouveau_pci, "nouveau,drmkms_pci");

SIMPLEQ_HEAD(nouveau_pci_task_head, nouveau_pci_task);

struct nouveau_pci_softc {
	device_t		sc_dev;
	struct pci_attach_args	sc_pa;
	enum {
		NOUVEAU_TASK_ATTACH,
		NOUVEAU_TASK_WORKQUEUE,
	}			sc_task_state;
	union {
		struct workqueue		*workqueue;
		struct nouveau_pci_task_head	attach;
	}			sc_task_u;
	struct drm_device	*sc_drm_dev;
	struct pci_dev		sc_pci_dev;
	struct nvkm_device	*sc_nv_dev;
	bool			sc_pci_attached;
	bool			sc_dev_registered;
};

static int	nouveau_pci_match(device_t, cfdata_t, void *);
static void	nouveau_pci_attach(device_t, device_t, void *);
static void	nouveau_pci_attach_real(device_t);
static int	nouveau_pci_detach(device_t, int);

static bool	nouveau_pci_suspend(device_t, const pmf_qual_t *);
static bool	nouveau_pci_resume(device_t, const pmf_qual_t *);

static void	nouveau_pci_task_work(struct work *, void *);

CFATTACH_DECL_NEW(nouveau_pci, sizeof(struct nouveau_pci_softc),
    nouveau_pci_match, nouveau_pci_attach, nouveau_pci_detach, NULL);

/* Kludge to get this from nouveau_drm.c.  */
extern struct drm_driver *const nouveau_drm_driver_pci;

static int
nouveau_pci_match(device_t parent, cfdata_t match, void *aux)
{
	const struct pci_attach_args *const pa = aux;
	struct pci_dev pdev;
	struct nvkm_device *device;
	int ret;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_NVIDIA &&
	    PCI_VENDOR(pa->pa_id) != PCI_VENDOR_NVIDIA_SGS)
		return 0;

	if (PCI_CLASS(pa->pa_class) != PCI_CLASS_DISPLAY)
		return 0;

	/*
	 * NetBSD drm2 doesn't support Pascal, Volta or Turing based cards:
	 *   0x1580-0x15ff 	GP100
	 *   0x1b00-0x1b7f 	GP102
	 *   0x1b80-0x1bff 	GP104
	 *   0x1c00-0x1c7f 	GP106
	 *   0x1c80-0x1cff 	GP107
	 *   0x1d00-0x1d7f 	GP108
	 *   0x1d80-0x1dff 	GV100
	 *   0x1e00-0x1e7f 	TU102
	 *   0x1e80-0x1eff 	TU104
	 *   0x1f00-0x1f7f 	TU106
	 *   0x1f80-0x1fff 	TU117
	 *   0x2180-0x21ff 	TU116
	 *
	 * reduce this to >= 1580, so that new chipsets not explictly
	 * listed above will be picked up.
	 *
	 * XXX perhaps switch this to explicitly match known list.
	 */
	if (PCI_PRODUCT(pa->pa_id) >= 0x1580)
		return 0;

	linux_pci_dev_init(&pdev, parent /* XXX bogus */, parent, pa, 0);
	ret = nvkm_device_pci_new(&pdev, NULL, "error",
	    /* detect */ true, /* mmio */ false, /* subdev_mask */ 0, &device);
	if (ret == 0)		/* don't want to hang onto it */
		nvkm_device_del(&device);
	linux_pci_dev_destroy(&pdev);
	if (ret)		/* failure */
		return 0;

	return 6;		/* XXX Beat genfb_pci...  */
}

extern char *nouveau_config;
extern char *nouveau_debug;

static void
nouveau_pci_attach(device_t parent, device_t self, void *aux)
{
	struct nouveau_pci_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = aux;

	pci_aprint_devinfo(pa, NULL);

	if (!pmf_device_register(self, &nouveau_pci_suspend, &nouveau_pci_resume))
		aprint_error_dev(self, "unable to establish power handler\n");

	/*
	 * Trivial initialization first; the rest will come after we
	 * have mounted the root file system and can load firmware
	 * images.
	 */
	sc->sc_dev = NULL;
	sc->sc_pa = *pa;

#ifdef FDT
	/*
	 * XXX Remove the simple framebuffer, assuming that this device
	 * will take over.
	 */
	const char *fb_compatible[] = { "simple-framebuffer", NULL };
	fdt_remove_bycompat(fb_compatible);
#endif

	config_mountroot(self, &nouveau_pci_attach_real);
}

static void
nouveau_pci_attach_real(device_t self)
{
	struct nouveau_pci_softc *const sc = device_private(self);
	const struct pci_attach_args *const pa = &sc->sc_pa;
	int error;

	sc->sc_dev = self;
	sc->sc_task_state = NOUVEAU_TASK_ATTACH;
	SIMPLEQ_INIT(&sc->sc_task_u.attach);

	/* Initialize the Linux PCI device descriptor.  */
	linux_pci_dev_init(&sc->sc_pci_dev, self, device_parent(self), pa, 0);

	/* XXX errno Linux->NetBSD */
	error = -nvkm_device_pci_new(&sc->sc_pci_dev,
	    nouveau_config, nouveau_debug,
	    /* detect */ true, /* mmio */ true, /* subdev_mask */ ~0ULL,
	    &sc->sc_nv_dev);
	if (error) {
		aprint_error_dev(self, "unable to create nouveau device: %d\n",
		    error);
		sc->sc_nv_dev = NULL;
		return;
	}

	sc->sc_drm_dev = drm_dev_alloc(nouveau_drm_driver_pci, self);
	if (IS_ERR(sc->sc_drm_dev)) {
		aprint_error_dev(self, "unable to create drm device: %ld\n",
		    PTR_ERR(sc->sc_drm_dev));
		sc->sc_drm_dev = NULL;
		return;
	}

	/* XXX errno Linux->NetBSD */
	error = -drm_pci_attach(sc->sc_drm_dev, pa, &sc->sc_pci_dev);
	if (error) {
		aprint_error_dev(self, "unable to attach drm: %d\n", error);
		return;
	}
	sc->sc_pci_attached = true;

	/* XXX errno Linux->NetBSD */
	error = -drm_dev_register(sc->sc_drm_dev, 0);
	if (error) {
		aprint_error_dev(self, "unable to register drm: %d\n", error);
		return;
	}
	sc->sc_dev_registered = true;

	while (!SIMPLEQ_EMPTY(&sc->sc_task_u.attach)) {
		struct nouveau_pci_task *const task =
		    SIMPLEQ_FIRST(&sc->sc_task_u.attach);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_task_u.attach, nt_u.queue);
		(*task->nt_fn)(task);
	}

	sc->sc_task_state = NOUVEAU_TASK_WORKQUEUE;
	error = workqueue_create(&sc->sc_task_u.workqueue, "nouveau_pci",
	    &nouveau_pci_task_work, NULL, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	if (error) {
		aprint_error_dev(self, "unable to create workqueue: %d\n",
		    error);
		sc->sc_task_u.workqueue = NULL;
		return;
	}
}

static int
nouveau_pci_detach(device_t self, int flags)
{
	struct nouveau_pci_softc *const sc = device_private(self);
	int error;

	if (sc->sc_dev == NULL)
		/* Not done attaching.  */
		return EBUSY;

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
	if (!sc->sc_pci_attached)
		goto out2;
	if (!sc->sc_dev_registered)
		goto out3;

	drm_dev_unregister(sc->sc_drm_dev);
out3:	drm_pci_detach(sc->sc_drm_dev);
out2:	drm_dev_put(sc->sc_drm_dev);
	sc->sc_drm_dev = NULL;
out1:	nvkm_device_del(&sc->sc_nv_dev);
out0:	linux_pci_dev_destroy(&sc->sc_pci_dev);
	pmf_device_deregister(self);
	return 0;
}

/*
 * XXX Synchronize with nouveau_do_suspend in nouveau_drm.c.
 */
static bool
nouveau_pci_suspend(device_t self, const pmf_qual_t *qual __unused)
{
	struct nouveau_pci_softc *const sc = device_private(self);

	return nouveau_pmops_suspend(sc->sc_drm_dev) == 0;
}

static bool
nouveau_pci_resume(device_t self, const pmf_qual_t *qual)
{
	struct nouveau_pci_softc *const sc = device_private(self);

	return nouveau_pmops_resume(sc->sc_drm_dev) == 0;
}

static void
nouveau_pci_task_work(struct work *work, void *cookie __unused)
{
	struct nouveau_pci_task *const task = container_of(work,
	    struct nouveau_pci_task, nt_u.work);

	(*task->nt_fn)(task);
}

int
nouveau_pci_task_schedule(device_t self, struct nouveau_pci_task *task)
{
	struct nouveau_pci_softc *const sc = device_private(self);

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

extern struct drm_driver *const nouveau_drm_driver_stub; /* XXX */
extern struct drm_driver *const nouveau_drm_driver_pci;	 /* XXX */

static int
nouveau_pci_modcmd(modcmd_t cmd, void *arg __unused)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		*nouveau_drm_driver_pci = *nouveau_drm_driver_stub;
		nouveau_drm_driver_pci->set_busid = drm_pci_set_busid;
		nouveau_drm_driver_pci->request_irq = drm_pci_request_irq;
		nouveau_drm_driver_pci->free_irq = drm_pci_free_irq;
#if 0		/* XXX nouveau acpi */
		nouveau_register_dsm_handler();
#endif
		break;
	case MODULE_CMD_FINI:
#if 0		/* XXX nouveau acpi */
		nouveau_unregister_dsm_handler();
#endif
		break;
	default:
		return ENOTTY;
	}

	return 0;
}
