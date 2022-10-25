/*	$NetBSD: vmwgfx_irq.c,v 1.6 2022/10/25 23:36:21 riastradh Exp $	*/

// SPDX-License-Identifier: GPL-2.0 OR MIT
/**************************************************************************
 *
 * Copyright 2009-2015 VMware, Inc., Palo Alto, CA., USA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vmwgfx_irq.c,v 1.6 2022/10/25 23:36:21 riastradh Exp $");

#include <linux/sched/signal.h>

#include <drm/drm_irq.h>

#include "vmwgfx_drv.h"

#define VMW_FENCE_WRAP (1 << 24)

/**
 * vmw_thread_fn - Deferred (process context) irq handler
 *
 * @irq: irq number
 * @arg: Closure argument. Pointer to a struct drm_device cast to void *
 *
 * This function implements the deferred part of irq processing.
 * The function is guaranteed to run at least once after the
 * vmw_irq_handler has returned with IRQ_WAKE_THREAD.
 *
 */
#ifdef __NetBSD__
static void
vmw_thread_fn(struct work *work, void *arg)
#else
static irqreturn_t vmw_thread_fn(int irq, void *arg)
#endif
{
	struct drm_device *dev = (struct drm_device *)arg;
	struct vmw_private *dev_priv = vmw_priv(dev);
	irqreturn_t ret = IRQ_NONE;

#ifdef __NetBSD__
	atomic_store_relaxed(&dev_priv->irqthread_scheduled, false);
#endif

	if (test_and_clear_bit(VMW_IRQTHREAD_FENCE,
			       dev_priv->irqthread_pending)) {
		spin_lock(&dev_priv->fence_lock);
		vmw_fences_update(dev_priv->fman);
		DRM_SPIN_WAKEUP_ALL(&dev_priv->fence_queue,
		    &dev_priv->fence_lock);
		spin_unlock(&dev_priv->fence_lock);
		ret = IRQ_HANDLED;
	}

	if (test_and_clear_bit(VMW_IRQTHREAD_CMDBUF,
			       dev_priv->irqthread_pending)) {
		vmw_cmdbuf_irqthread(dev_priv->cman);
		ret = IRQ_HANDLED;
	}

#ifndef __NetBSD__
	return ret;
#endif
}

/**
 * vmw_irq_handler irq handler
 *
 * @irq: irq number
 * @arg: Closure argument. Pointer to a struct drm_device cast to void *
 *
 * This function implements the quick part of irq processing.
 * The function performs fast actions like clearing the device interrupt
 * flags and also reasonably quick actions like waking processes waiting for
 * FIFO space. Other IRQ actions are deferred to the IRQ thread.
 */
static irqreturn_t vmw_irq_handler(int irq, void *arg)
{
	struct drm_device *dev = (struct drm_device *)arg;
	struct vmw_private *dev_priv = vmw_priv(dev);
	uint32_t status, masked_status;
	irqreturn_t ret = IRQ_HANDLED;

#ifdef __NetBSD__
	status = bus_space_read_4(dev_priv->iot, dev_priv->ioh,
	    VMWGFX_IRQSTATUS_PORT);
#else
	status = inl(dev_priv->io_start + VMWGFX_IRQSTATUS_PORT);
#endif
	masked_status = status & READ_ONCE(dev_priv->irq_mask);

	if (likely(status))
#ifdef __NetBSD__
		bus_space_write_4(dev_priv->iot, dev_priv->ioh,
		    VMWGFX_IRQSTATUS_PORT, status);
#else
		outl(status, dev_priv->io_start + VMWGFX_IRQSTATUS_PORT);
#endif

	if (!status)
		return IRQ_NONE;

	if (masked_status & SVGA_IRQFLAG_FIFO_PROGRESS) {
		spin_lock(&dev_priv->fifo_lock);
		DRM_SPIN_WAKEUP_ALL(&dev_priv->fifo_queue,
		    &dev_priv->fifo_lock);
		spin_unlock(&dev_priv->fifo_lock);
	}

	if ((masked_status & (SVGA_IRQFLAG_ANY_FENCE |
			      SVGA_IRQFLAG_FENCE_GOAL)) &&
	    !test_and_set_bit(VMW_IRQTHREAD_FENCE, dev_priv->irqthread_pending))
		ret = IRQ_WAKE_THREAD;

	if ((masked_status & (SVGA_IRQFLAG_COMMAND_BUFFER |
			      SVGA_IRQFLAG_ERROR)) &&
	    !test_and_set_bit(VMW_IRQTHREAD_CMDBUF,
			      dev_priv->irqthread_pending))
		ret = IRQ_WAKE_THREAD;

#ifdef __NetBSD__
	if (ret == IRQ_WAKE_THREAD) {
		if (atomic_swap_uint(&dev_priv->irqthread_scheduled, 1) == 0) {
			workqueue_enqueue(dev_priv->irqthread_wq,
			    &dev_priv->irqthread_work, NULL);
		}
		ret = IRQ_HANDLED;
	}
#endif

	return ret;
}

static bool vmw_fifo_idle(struct vmw_private *dev_priv, uint32_t seqno)
{

	return (vmw_read(dev_priv, SVGA_REG_BUSY) == 0);
}

void vmw_update_seqno(struct vmw_private *dev_priv,
			 struct vmw_fifo_state *fifo_state)
{
	u32 *fifo_mem = dev_priv->mmio_virt;
	uint32_t seqno = vmw_mmio_read(fifo_mem + SVGA_FIFO_FENCE);

	assert_spin_locked(&dev_priv->fence_lock);

	if (dev_priv->last_read_seqno != seqno) {
		dev_priv->last_read_seqno = seqno;
		vmw_marker_pull(&fifo_state->marker_queue, seqno);
		vmw_fences_update(dev_priv->fman);
	}
}

bool vmw_seqno_passed(struct vmw_private *dev_priv,
			 uint32_t seqno)
{
	struct vmw_fifo_state *fifo_state;
	bool ret;

	assert_spin_locked(&dev_priv->fence_lock);

	if (likely(dev_priv->last_read_seqno - seqno < VMW_FENCE_WRAP))
		return true;

	fifo_state = &dev_priv->fifo;
	vmw_update_seqno(dev_priv, fifo_state);
	if (likely(dev_priv->last_read_seqno - seqno < VMW_FENCE_WRAP))
		return true;

	if (!(fifo_state->capabilities & SVGA_FIFO_CAP_FENCE) &&
	    vmw_fifo_idle(dev_priv, seqno))
		return true;

	/**
	 * Then check if the seqno is higher than what we've actually
	 * emitted. Then the fence is stale and signaled.
	 */

	ret = ((atomic_read(&dev_priv->marker_seq) - seqno)
	       > VMW_FENCE_WRAP);

	return ret;
}

int vmw_fallback_wait(struct vmw_private *dev_priv,
		      bool lazy,
		      bool fifo_idle,
		      uint32_t seqno,
		      bool interruptible,
		      unsigned long timeout)
{
	struct vmw_fifo_state *fifo_state = &dev_priv->fifo;

	uint32_t count = 0;
	uint32_t signal_seq;
	int ret;
	unsigned long end_jiffies = jiffies + timeout;
	bool (*wait_condition)(struct vmw_private *, uint32_t);
#ifndef __NetBSD__
	DEFINE_WAIT(__wait);
#endif

	wait_condition = (fifo_idle) ? &vmw_fifo_idle :
		&vmw_seqno_passed;

	/**
	 * Block command submission while waiting for idle.
	 */

	if (fifo_idle) {
		down_read(&fifo_state->rwsem);
		if (dev_priv->cman) {
			ret = vmw_cmdbuf_idle(dev_priv->cman, interruptible,
					      10*HZ);
			if (ret)
				goto out_err;
		}
	}

	spin_lock(&dev_priv->fence_lock);

	signal_seq = atomic_read(&dev_priv->marker_seq);
	ret = 0;

	for (;;) {
#ifdef __NetBSD__
		if (!lazy) {
			if (wait_condition(dev_priv, seqno))
				break;
			spin_unlock(&dev_priv->fence_lock);
			if ((++count & 0xf) == 0)
				yield();
			spin_lock(&dev_priv->fence_lock);
		} else if (interruptible) {
			DRM_SPIN_TIMED_WAIT_UNTIL(ret, &dev_priv->fence_queue,
			    &dev_priv->fence_lock, /*timeout*/1,
			    wait_condition(dev_priv, seqno));
		} else {
			DRM_SPIN_TIMED_WAIT_NOINTR_UNTIL(ret,
			    &dev_priv->fence_queue,
			    &dev_priv->fence_lock, /*timeout*/1,
			    wait_condition(dev_priv, seqno));
		}
		if (ret) {	/* success or error but not timeout */
			if (ret > 0) /* success */
				ret = 0;
			break;
		}
		if (time_after_eq(jiffies, end_jiffies)) {
			DRM_ERROR("SVGA device lockup.\n");
			break;
		}
#else
		prepare_to_wait(&dev_priv->fence_queue, &__wait,
				(interruptible) ?
				TASK_INTERRUPTIBLE : TASK_UNINTERRUPTIBLE);
		if (wait_condition(dev_priv, seqno))
			break;
		if (time_after_eq(jiffies, end_jiffies)) {
			DRM_ERROR("SVGA device lockup.\n");
			break;
		}
		if (lazy)
			schedule_timeout(1);
		else if ((++count & 0x0F) == 0) {
			/**
			 * FIXME: Use schedule_hr_timeout here for
			 * newer kernels and lower CPU utilization.
			 */

			__set_current_state(TASK_RUNNING);
			schedule();
			__set_current_state((interruptible) ?
					    TASK_INTERRUPTIBLE :
					    TASK_UNINTERRUPTIBLE);
		}
		if (interruptible && signal_pending(current)) {
			ret = -ERESTARTSYS;
			break;
		}
#endif
	}
#ifndef __NetBSD__
	finish_wait(&dev_priv->fence_queue, &__wait);
#endif
	if (ret == 0 && fifo_idle) {
		u32 *fifo_mem = dev_priv->mmio_virt;

		vmw_mmio_write(signal_seq, fifo_mem + SVGA_FIFO_FENCE);
	}
#ifdef __NetBSD__
	DRM_SPIN_WAKEUP_ALL(&dev_priv->fence_queue, &dev_priv->fence_lock);
	spin_unlock(&dev_priv->fence_lock);
#else
	wake_up_all(&dev_priv->fence_queue);
#endif
out_err:
	if (fifo_idle)
		up_read(&fifo_state->rwsem);

	return ret;
}

void vmw_generic_waiter_add(struct vmw_private *dev_priv,
			    u32 flag, int *waiter_count)
{
	spin_lock_bh(&dev_priv->waiter_lock);
	if ((*waiter_count)++ == 0) {
#ifdef __NetBSD__
		bus_space_write_4(dev_priv->iot, dev_priv->ioh,
		    VMWGFX_IRQSTATUS_PORT, flag);
#else
		outl(flag, dev_priv->io_start + VMWGFX_IRQSTATUS_PORT);
#endif
		dev_priv->irq_mask |= flag;
		vmw_write(dev_priv, SVGA_REG_IRQMASK, dev_priv->irq_mask);
	}
	spin_unlock_bh(&dev_priv->waiter_lock);
}

void vmw_generic_waiter_remove(struct vmw_private *dev_priv,
			       u32 flag, int *waiter_count)
{
	spin_lock_bh(&dev_priv->waiter_lock);
	if (--(*waiter_count) == 0) {
		dev_priv->irq_mask &= ~flag;
		vmw_write(dev_priv, SVGA_REG_IRQMASK, dev_priv->irq_mask);
	}
	spin_unlock_bh(&dev_priv->waiter_lock);
}

void vmw_seqno_waiter_add(struct vmw_private *dev_priv)
{
	vmw_generic_waiter_add(dev_priv, SVGA_IRQFLAG_ANY_FENCE,
			       &dev_priv->fence_queue_waiters);
}

void vmw_seqno_waiter_remove(struct vmw_private *dev_priv)
{
	vmw_generic_waiter_remove(dev_priv, SVGA_IRQFLAG_ANY_FENCE,
				  &dev_priv->fence_queue_waiters);
}

void vmw_goal_waiter_add(struct vmw_private *dev_priv)
{
	vmw_generic_waiter_add(dev_priv, SVGA_IRQFLAG_FENCE_GOAL,
			       &dev_priv->goal_queue_waiters);
}

void vmw_goal_waiter_remove(struct vmw_private *dev_priv)
{
	vmw_generic_waiter_remove(dev_priv, SVGA_IRQFLAG_FENCE_GOAL,
				  &dev_priv->goal_queue_waiters);
}

int vmw_wait_seqno(struct vmw_private *dev_priv,
		      bool lazy, uint32_t seqno,
		      bool interruptible, unsigned long timeout)
{
	long ret;
	struct vmw_fifo_state *fifo = &dev_priv->fifo;

	spin_lock(&dev_priv->fence_lock);
	if (likely(dev_priv->last_read_seqno - seqno < VMW_FENCE_WRAP)) {
		spin_unlock(&dev_priv->fence_lock);
		return 0;
	}

	if (likely(vmw_seqno_passed(dev_priv, seqno))) {
		spin_unlock(&dev_priv->fence_lock);
		return 0;
	}

	vmw_fifo_ping_host(dev_priv, SVGA_SYNC_GENERIC);

	if (!(fifo->capabilities & SVGA_FIFO_CAP_FENCE)) {
		spin_unlock(&dev_priv->fence_lock);
		return vmw_fallback_wait(dev_priv, lazy, true, seqno,
					 interruptible, timeout);
	}

	if (!(dev_priv->capabilities & SVGA_CAP_IRQMASK)) {
		spin_unlock(&dev_priv->fence_lock);
		return vmw_fallback_wait(dev_priv, lazy, false, seqno,
					 interruptible, timeout);
	}

	vmw_seqno_waiter_add(dev_priv);

	if (interruptible)
		DRM_SPIN_TIMED_WAIT_UNTIL(ret, &dev_priv->fence_queue,
		    &dev_priv->fence_lock, timeout,
		    vmw_seqno_passed(dev_priv, seqno));
	else
		DRM_SPIN_TIMED_WAIT_NOINTR_UNTIL(ret, &dev_priv->fence_queue,
		    &dev_priv->fence_lock, timeout,
		    vmw_seqno_passed(dev_priv, seqno));

	vmw_seqno_waiter_remove(dev_priv);

	spin_unlock(&dev_priv->fence_lock);

	if (unlikely(ret == 0))
		ret = -EBUSY;
	else if (likely(ret > 0))
		ret = 0;

	return ret;
}

static void vmw_irq_preinstall(struct drm_device *dev)
{
	struct vmw_private *dev_priv = vmw_priv(dev);
	uint32_t status;

#ifdef __NetBSD__
	status = bus_space_read_4(dev_priv->iot, dev_priv->ioh,
	    VMWGFX_IRQSTATUS_PORT);
	bus_space_write_4(dev_priv->iot, dev_priv->ioh, VMWGFX_IRQSTATUS_PORT,
	    status);
#else
	status = inl(dev_priv->io_start + VMWGFX_IRQSTATUS_PORT);
	outl(status, dev_priv->io_start + VMWGFX_IRQSTATUS_PORT);
#endif
}

void vmw_irq_uninstall(struct drm_device *dev)
{
	struct vmw_private *dev_priv = vmw_priv(dev);
	uint32_t status;

	if (!(dev_priv->capabilities & SVGA_CAP_IRQMASK))
		return;

	if (!dev->irq_enabled)
		return;

	vmw_write(dev_priv, SVGA_REG_IRQMASK, 0);

#ifdef __NetBSD__
	status = bus_space_read_4(dev_priv->iot, dev_priv->ioh,
	    VMWGFX_IRQSTATUS_PORT);
	bus_space_write_4(dev_priv->iot, dev_priv->ioh, VMWGFX_IRQSTATUS_PORT,
	    status);
#else
	status = inl(dev_priv->io_start + VMWGFX_IRQSTATUS_PORT);
	outl(status, dev_priv->io_start + VMWGFX_IRQSTATUS_PORT);
#endif

	dev->irq_enabled = false;
#ifdef __NetBSD__
	int ret = drm_irq_uninstall(dev);
	KASSERT(ret == 0);
	workqueue_destroy(dev_priv->irqthread_wq);
#else
	free_irq(dev->irq, dev);
#endif
}

/**
 * vmw_irq_install - Install the irq handlers
 *
 * @dev:  Pointer to the drm device.
 * @irq:  The irq number.
 * Return:  Zero if successful. Negative number otherwise.
 */
int vmw_irq_install(struct drm_device *dev, int irq)
{
	int ret;

	if (dev->irq_enabled)
		return -EBUSY;

	vmw_irq_preinstall(dev);

#ifdef __NetBSD__
	/* XXX errno NetBSD->Linux */
	ret = -workqueue_create(&vmw_priv(dev)->irqthread_wq, "vmwgfirq",
	    vmw_thread_fn, dev, PRI_NONE, IPL_DRM, WQ_MPSAFE);
	if (ret < 0)
		return ret;
	ret = drm_irq_install(dev);
	if (ret < 0) {
		workqueue_destroy(vmw_priv(dev)->irqthread_wq);
		vmw_priv(dev)->irqthread_wq = NULL;
	}
#else
	ret = request_threaded_irq(irq, vmw_irq_handler, vmw_thread_fn,
				   IRQF_SHARED, VMWGFX_DRIVER_NAME, dev);
#endif
	if (ret < 0)
		return ret;

	dev->irq_enabled = true;
	dev->irq = irq;

	return ret;
}
