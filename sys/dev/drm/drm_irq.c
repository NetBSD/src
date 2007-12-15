/* $NetBSD: drm_irq.c,v 1.7 2007/12/15 00:39:26 perry Exp $ */

/* drm_irq.c -- IRQ IOCTL and function support
 * Created: Fri Oct 18 2003 by anholt@FreeBSD.org
 */
/*-
 * Copyright 2003 Eric Anholt
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_irq.c,v 1.7 2007/12/15 00:39:26 perry Exp $");
/*
__FBSDID("$FreeBSD: src/sys/dev/drm/drm_irq.c,v 1.2 2005/11/28 23:13:52 anholt Exp $");
*/

#include "drmP.h"
#include "drm.h"

int drm_irq_by_busid(DRM_IOCTL_ARGS)
{
	DRM_DEVICE;
	drm_irq_busid_t irq;

	DRM_COPY_FROM_USER_IOCTL(irq, (drm_irq_busid_t *)data, sizeof(irq));

	if ((irq.busnum >> 8) != dev->pci_domain ||
	    (irq.busnum & 0xff) != dev->pci_bus ||
	    irq.devnum != dev->pci_slot ||
	    irq.funcnum != dev->pci_func)
		return EINVAL;

	irq.irq = dev->irq;

	DRM_DEBUG("%d:%d:%d => IRQ %d\n",
		  irq.busnum, irq.devnum, irq.funcnum, irq.irq);

	DRM_COPY_TO_USER_IOCTL( (drm_irq_busid_t *)data, irq, sizeof(irq) );

	return 0;
}

static irqreturn_t
drm_irq_handler_wrap(DRM_IRQ_ARGS)
{
	irqreturn_t ret;
	drm_device_t *dev = (drm_device_t *)arg;

	DRM_SPINLOCK(&dev->irq_lock);
	ret = dev->driver.irq_handler(arg);
	DRM_SPINUNLOCK(&dev->irq_lock);
	return ret;
}

int drm_irq_install(drm_device_t *dev)
{
	int retcode;
	pci_intr_handle_t ih;
	const char *istr;

	if (dev->irq == 0 || dev->dev_private == NULL)
		return DRM_ERR(EINVAL);

	DRM_DEBUG( "%s: irq=%d\n", __func__, dev->irq );

	DRM_LOCK();
	if (dev->irq_enabled) {
		DRM_UNLOCK();
		return DRM_ERR(EBUSY);
	}
	dev->irq_enabled = 1;

	dev->context_flag = 0;

	DRM_SPININIT(&dev->irq_lock, "DRM IRQ lock");

				/* Before installing handler */

	dev->driver.irq_preinstall(dev);
	DRM_UNLOCK();

				/* Install handler */
	if (pci_intr_map(&dev->pa, &ih) != 0) {
		retcode = ENOENT;
		goto err;
	}
	istr = pci_intr_string(dev->pa.pa_pc, ih);
	dev->irqh = pci_intr_establish(dev->pa.pa_pc, ih, IPL_TTY,
	    drm_irq_handler_wrap, dev);
	if (!dev->irqh) {
		retcode = ENOENT;
		goto err;
	}
	aprint_normal("%s: interrupting at %s\n", dev->device.dv_xname, istr);

				/* After installing handler */
	DRM_LOCK();
	dev->driver.irq_postinstall(dev);
	DRM_UNLOCK();

	return 0;
err:
	DRM_LOCK();
	dev->irq_enabled = 0;
	DRM_SPINUNINIT(&dev->irq_lock);
	DRM_UNLOCK();
	return retcode;
}

int drm_irq_uninstall(drm_device_t *dev)
{
	if (!dev->irq_enabled)
		return DRM_ERR(EINVAL);

	dev->irq_enabled = 0;

	DRM_DEBUG( "%s: irq=%d\n", __func__, dev->irq );

	dev->driver.irq_uninstall(dev);

	pci_intr_disestablish(dev->pa.pa_pc, dev->irqh);
	DRM_SPINUNINIT(&dev->irq_lock);

	return 0;
}

int drm_control(DRM_IOCTL_ARGS)
{
	DRM_DEVICE;
	drm_control_t ctl;
	int err;

	DRM_COPY_FROM_USER_IOCTL( ctl, (drm_control_t *) data, sizeof(ctl) );

	switch ( ctl.func ) {
	case DRM_INST_HANDLER:
		/* Handle drivers whose DRM used to require IRQ setup but the
		 * no longer does.
		 */
		if (!dev->driver.use_irq)
			return 0;
		if (dev->if_version < DRM_IF_VERSION(1, 2) &&
		    ctl.irq != dev->irq)
			return DRM_ERR(EINVAL);
		return drm_irq_install(dev);
	case DRM_UNINST_HANDLER:
		if (!dev->driver.use_irq)
			return 0;
		DRM_LOCK();
		err = drm_irq_uninstall(dev);
		DRM_UNLOCK();
		return err;
	default:
		return DRM_ERR(EINVAL);
	}
}

int drm_wait_vblank(DRM_IOCTL_ARGS)
{
	DRM_DEVICE;
	drm_wait_vblank_t vblwait;
	struct timeval now;
	int ret;

	if (!dev->irq_enabled)
		return DRM_ERR(EINVAL);

	DRM_COPY_FROM_USER_IOCTL( vblwait, (drm_wait_vblank_t *)data,
				  sizeof(vblwait) );

	if (vblwait.request.type & _DRM_VBLANK_RELATIVE) {
		vblwait.request.sequence += atomic_read(&dev->vbl_received);
		vblwait.request.type &= ~_DRM_VBLANK_RELATIVE;
	}

	flags = vblwait.request.type & _DRM_VBLANK_FLAGS_MASK;
	if (flags & _DRM_VBLANK_SIGNAL) {
#if 0 /* disabled */
		drm_vbl_sig_t *vbl_sig = malloc(sizeof(drm_vbl_sig_t), M_DRM,
		    M_NOWAIT | M_ZERO);
		if (vbl_sig == NULL)
			return ENOMEM;

		vbl_sig->sequence = vblwait.request.sequence;
		vbl_sig->signo = vblwait.request.signal;
		vbl_sig->pid = DRM_CURRENTPID;

		vblwait.reply.sequence = atomic_read(&dev->vbl_received);
		
		DRM_SPINLOCK(&dev->irq_lock);
		TAILQ_INSERT_HEAD(&dev->vbl_sig_list, vbl_sig, link);
		DRM_SPINUNLOCK(&dev->irq_lock);
		ret = 0;
#endif
		ret = EINVAL;
	} else {
		DRM_LOCK();
		ret = dev->driver.vblank_wait(dev, &vblwait.request.sequence);
		DRM_UNLOCK();

		microtime(&now);
		vblwait.reply.tval_sec = now.tv_sec;
		vblwait.reply.tval_usec = now.tv_usec;
	}

	DRM_COPY_TO_USER_IOCTL( (drm_wait_vblank_t *)data, vblwait,
				sizeof(vblwait) );

	return ret;
}

void drm_vbl_send_signals(drm_device_t *dev)
{
}

#if 0 /* disabled */
void drm_vbl_send_signals( drm_device_t *dev )
{
	drm_vbl_sig_t *vbl_sig;
	unsigned int vbl_seq = atomic_read( &dev->vbl_received );
	struct proc *p;

	vbl_sig = TAILQ_FIRST(&dev->vbl_sig_list);
	while (vbl_sig != NULL) {
		drm_vbl_sig_t *next = TAILQ_NEXT(vbl_sig, link);

		if ( ( vbl_seq - vbl_sig->sequence ) <= (1<<23) ) {
			p = pfind(vbl_sig->pid);
			if (p != NULL)
				psignal(p, vbl_sig->signo);

			TAILQ_REMOVE(&dev->vbl_sig_list, vbl_sig, link);
			DRM_FREE(vbl_sig,sizeof(*vbl_sig));
		}
		vbl_sig = next;
	}
}
#endif
