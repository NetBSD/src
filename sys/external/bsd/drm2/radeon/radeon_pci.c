/*	$NetBSD: radeon_pci.c,v 1.21.4.3 2024/10/04 11:40:52 martin Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: radeon_pci.c,v 1.21.4.3 2024/10/04 11:40:52 martin Exp $");

#ifdef _KERNEL_OPT
#include "genfb.h"
#include "vga.h"
#if defined(__arm__) || defined(__aarch64__)
#include "opt_fdt.h"
#endif
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

#ifdef FDT
#include <dev/fdt/fdtvar.h>
#endif

#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_pci.h>

#if NGENFB > 0
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wsfb/genfbvar.h>
#endif

#include <radeon.h>
#include "radeon_drv.h"
#include "radeon_task.h"

SIMPLEQ_HEAD(radeon_task_head, radeon_task);

struct radeon_softc {
	device_t			sc_dev;
	struct pci_attach_args		sc_pa;
	struct lwp			*sc_task_thread;
	struct radeon_task_head		sc_tasks;
	struct workqueue		*sc_task_wq;
	struct drm_device		*sc_drm_dev;
	struct pci_dev			sc_pci_dev;
	bool				sc_pci_attached;
	bool				sc_dev_registered;
#if defined(__i386__)
#define RADEON_PCI_UGLY_MAP_HACK
	/* XXX Used to claim the VGA device before attach_real */
	bus_space_handle_t		sc_temp_memh;
	bool				sc_temp_set;
#endif
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
static bool	radeon_do_suspend(device_t, const pmf_qual_t *);
static bool	radeon_do_resume(device_t, const pmf_qual_t *);

static void	radeon_task_work(struct work *, void *);

CFATTACH_DECL_NEW(radeon, sizeof(struct radeon_softc),
    radeon_match, radeon_attach, radeon_detach, NULL);

/* XXX Kludge to get these from radeon_drv.c.  */
extern struct drm_driver *const radeon_drm_driver;
extern const struct pci_device_id *const radeon_device_ids;
extern const size_t radeon_n_device_ids;

/* Set this to false if you want to match R100/R200 */
bool radeon_pci_ignore_r100_r200 = true;

static bool
radeon_pci_lookup(const struct pci_attach_args *pa, unsigned long *flags)
{
	size_t i;
	enum radeon_family fam;

	for (i = 0; i < radeon_n_device_ids; i++) {
		if ((PCI_VENDOR(pa->pa_id) == radeon_device_ids[i].vendor) &&
		    (PCI_PRODUCT(pa->pa_id) == radeon_device_ids[i].device))
			break;
	}

	/* Did we find it?  */
	if (i == radeon_n_device_ids)
		return false;

	/* NetBSD drm2 fails on R100 and many R200 chipsets, disable for now  */
	fam = radeon_device_ids[i].driver_data & RADEON_FAMILY_MASK;
	if (radeon_pci_ignore_r100_r200 && fam < CHIP_RV280)
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
	int error;

	pci_aprint_devinfo(pa, NULL);

	/* Initialize the Linux PCI device descriptor.  */
	linux_pci_dev_init(&sc->sc_pci_dev, self, device_parent(self), pa, 0);

	sc->sc_dev = self;
	sc->sc_pa = *pa;
	sc->sc_task_thread = NULL;
	SIMPLEQ_INIT(&sc->sc_tasks);
	error = workqueue_create(&sc->sc_task_wq, "radeonfb",
	    &radeon_task_work, NULL, PRI_NONE, IPL_NONE, WQ_MPSAFE);
	if (error) {
		aprint_error_dev(self, "unable to create workqueue: %d\n",
		    error);
		sc->sc_task_wq = NULL;
		return;
	}

#ifdef RADEON_PCI_UGLY_MAP_HACK
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
				       "i386 radeondrmkms hack\n");
#endif

#ifdef FDT
	/*
	 * XXX Remove the simple framebuffer, assuming that this device
	 * will take over.
	 */
	const char *fb_compatible[] = { "simple-framebuffer", NULL };
	fdt_remove_bycompat(fb_compatible);
#endif

	/*
	 * Defer the remainder of initialization until we have mounted
	 * the root file system and can load firmware images.
	 */
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

#ifdef RADEON_PCI_UGLY_MAP_HACK
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

	sc->sc_drm_dev = drm_dev_alloc(radeon_drm_driver, self);
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

#if NGENFB > 0
	/*
	 * If MD initialization has selected this as the console device
	 * with a firmware-provided framebuffer address, we may have to
	 * turn it off early, before we are ready to switch the console
	 * over -- something goes wrong if we're still writing to the
	 * firmware-provided framebuffer during initialization.
	 */
    {
	bool is_console;
	if (prop_dictionary_get_bool(device_properties(self), "is_console",
		&is_console) &&
	    is_console &&
	    genfb_is_console())
		wsdisplay_predetach();
    }
#endif

	/* XXX errno Linux->NetBSD */
	error = -drm_dev_register(sc->sc_drm_dev, flags);
	if (error) {
		aprint_error_dev(self, "unable to register drm: %d\n", error);
		goto out;
	}
	sc->sc_dev_registered = true;

	if (!pmf_device_register(self, &radeon_do_suspend, &radeon_do_resume))
		aprint_error_dev(self, "unable to establish power handler\n");

	/*
	 * Process asynchronous tasks queued synchronously during
	 * attach.  This will be for display detection to attach a
	 * framebuffer, so we have the opportunity for a console device
	 * to attach before autoconf has completed, in time for init(8)
	 * to find that console without panicking.
	 */
	while (!SIMPLEQ_EMPTY(&sc->sc_tasks)) {
		struct radeon_task *const task = SIMPLEQ_FIRST(&sc->sc_tasks);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_tasks, rt_u.queue);
		(*task->rt_fn)(task);
	}

out:	/* Cause any subesquent tasks to be processed by the workqueue.  */
	atomic_store_relaxed(&sc->sc_task_thread, NULL);
}

static int
radeon_detach(device_t self, int flags)
{
	struct radeon_softc *const sc = device_private(self);
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
radeon_do_suspend(device_t self, const pmf_qual_t *qual)
{
	struct radeon_softc *const sc = device_private(self);
	struct drm_device *const dev = sc->sc_drm_dev;
	int ret;
	bool is_console = true; /* XXX */

	drm_suspend_ioctl(dev);

	ret = radeon_suspend_kms(dev, true, is_console, false);
	if (ret)
		return false;

	return true;
}

static bool
radeon_do_resume(device_t self, const pmf_qual_t *qual)
{
	struct radeon_softc *const sc = device_private(self);
	struct drm_device *const dev = sc->sc_drm_dev;
	int ret;
	bool is_console = true; /* XXX */

	ret = radeon_resume_kms(dev, true, is_console);
	if (ret)
		goto out;

out:	drm_resume_ioctl(dev);
	return ret == 0;
}

static void
radeon_task_work(struct work *work, void *cookie __unused)
{
	struct radeon_task *const task = container_of(work, struct radeon_task,
	    rt_u.work);

	(*task->rt_fn)(task);
}

void
radeon_task_schedule(device_t self, struct radeon_task *task)
{
	struct radeon_softc *const sc = device_private(self);

	if (atomic_load_relaxed(&sc->sc_task_thread) == curlwp)
		SIMPLEQ_INSERT_TAIL(&sc->sc_tasks, task, rt_u.queue);
	else
		workqueue_enqueue(sc->sc_task_wq, &task->rt_u.work, NULL);
}
