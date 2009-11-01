/* $NetBSD: xenbus_client.c,v 1.9.8.2 2009/11/01 13:58:48 jym Exp $ */
/******************************************************************************
 * Client-facing interface for the Xenbus driver.  In other words, the
 * interface between the Xenbus and the device-specific code, be it the
 * frontend or the backend of that driver.
 *
 * Copyright (C) 2005 XenSource Ltd
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xenbus_client.c,v 1.9.8.2 2009/11/01 13:58:48 jym Exp $");

#if 0
#define DPRINTK(fmt, args...) \
    printk("xenbus_client (%s:%d) " fmt ".\n", __func__, __LINE__, ##args)
#else
#define DPRINTK(fmt, args...) ((void)0)
#endif

#include <sys/types.h>
#include <sys/null.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/stdarg.h>

#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/xenbus.h>
#include <xen/granttables.h>


int
xenbus_watch_path(struct xenbus_device *dev, char *path,
		      struct xenbus_watch *watch, 
		      void (*callback)(struct xenbus_watch *,
				       const char **, unsigned int))
{
	int err;

	watch->node = path;
	watch->xbw_callback = callback;

	err = register_xenbus_watch(watch);

	if (err) {
		watch->node = NULL;
		watch->xbw_callback = NULL;
		xenbus_dev_fatal(dev, err, "adding watch on %s", path);
	}
	err = 0;

	return err;
}

int
xenbus_watch_path2(struct xenbus_device *dev, const char *path,
		       const char *path2, struct xenbus_watch *watch, 
		       void (*callback)(struct xenbus_watch *,
					const char **, unsigned int))
{
	int err;
	char *state;

	DPRINTK("xenbus_watch_path2 path %s path2 %s\n", path, path2);
	state =
		malloc(strlen(path) + 1 + strlen(path2) + 1, M_DEVBUF,
		    M_NOWAIT);
	if (!state) {
		xenbus_dev_fatal(dev, ENOMEM, "allocating path for watch");
		return ENOMEM;
	}
	strcpy(state, path);
	strcat(state, "/");
	strcat(state, path2);

	err = xenbus_watch_path(dev, state, watch, callback);

	if (err) {
		free(state, M_DEVBUF);
	}
	return err;
}


int
xenbus_switch_state(struct xenbus_device *dev,
			struct xenbus_transaction *xbt,
			XenbusState state)
{
	/* We check whether the state is currently set to the given value, and
	   if not, then the state is set.  We don't want to unconditionally
	   write the given state, because we don't want to fire watches
	   unnecessarily.  Furthermore, if the node has gone, we don't write
	   to it, as the device will be tearing down, and we don't want to
	   resurrect that directory.
	 */

	u_long current_state;

	int err = xenbus_read_ul(xbt, dev->xbusd_path, "state",
	    &current_state, 10);
	if (err)
		return 0;

	if ((XenbusState)current_state == state)
		return 0;

	err = xenbus_printf(xbt, dev->xbusd_path, "state", "%d", state);
	if (err) {
		xenbus_dev_fatal(dev, err, "writing new state");
		return err;
	}
	return 0;
}

/**
 * Return the path to the error node for the given device, or NULL on failure.
 * If the value returned is non-NULL, then it is the caller's to kfree.
 */
static char *
error_path(struct xenbus_device *dev)
{
	char *path_buffer = malloc(strlen("error/") + strlen(dev->xbusd_path) +
				    1, M_DEVBUF, M_NOWAIT);
	if (path_buffer == NULL) {
		return NULL;
	}

	strcpy(path_buffer, "error/");
	strcpy(path_buffer + strlen("error/"), dev->xbusd_path);

	return path_buffer;
}


static void
_dev_error(struct xenbus_device *dev, int err, const char *fmt,
		va_list ap)
{
	int ret;
	unsigned int len;
	char *printf_buffer = NULL, *path_buffer = NULL;

#define PRINTF_BUFFER_SIZE 4096
	printf_buffer = malloc(PRINTF_BUFFER_SIZE, M_DEVBUF, M_NOWAIT);
	if (printf_buffer == NULL)
		goto fail;

	len = snprintf(printf_buffer, PRINTF_BUFFER_SIZE, "%i ", -err);
	ret = vsnprintf(printf_buffer+len, PRINTF_BUFFER_SIZE-len, fmt, ap);

	KASSERT(len + ret < PRINTF_BUFFER_SIZE);
	dev->xbusd_has_error = 1;

	path_buffer = error_path(dev);

	if (path_buffer == NULL) {
		printk("xenbus: failed to write error node for %s (%s)\n",
		       dev->xbusd_path, printf_buffer);
		goto fail;
	}

	if (xenbus_write(NULL, path_buffer, "error", printf_buffer) != 0) {
		printk("xenbus: failed to write error node for %s (%s)\n",
		       dev->xbusd_path, printf_buffer);
		goto fail;
	}

fail:
	if (printf_buffer)
		free(printf_buffer, M_DEVBUF);
	if (path_buffer)
		free(path_buffer, M_DEVBUF);
}


void
xenbus_dev_error(struct xenbus_device *dev, int err, const char *fmt,
		      ...)
{
	va_list ap;

	va_start(ap, fmt);
	_dev_error(dev, err, fmt, ap);
	va_end(ap);
}


void
xenbus_dev_fatal(struct xenbus_device *dev, int err, const char *fmt,
		      ...)
{
	va_list ap;

	va_start(ap, fmt);
	_dev_error(dev, err, fmt, ap);
	va_end(ap);
	
	xenbus_switch_state(dev, NULL, XenbusStateClosing);
}


int
xenbus_grant_ring(struct xenbus_device *dev, paddr_t ring_pa,
    grant_ref_t *entryp)
{
	int err = xengnt_grant_access(dev->xbusd_otherend_id, ring_pa,
	    0, entryp);
	if (err != 0)
		xenbus_dev_fatal(dev, err, "granting access to ring page");
	return err;
}


int
xenbus_alloc_evtchn(struct xenbus_device *dev, int *port)
{
	evtchn_op_t op = {
		.cmd = EVTCHNOP_alloc_unbound,
		.u.alloc_unbound = {
			.dom = DOMID_SELF,
			.remote_dom = dev->xbusd_otherend_id,
			.port = 0
		}
	};

	int err = HYPERVISOR_event_channel_op(&op);
	if (err)
		xenbus_dev_fatal(dev, err, "allocating event channel");
	else
		*port = op.u.alloc_unbound.port;
	return err;
}


XenbusState
xenbus_read_driver_state(const char *path)
{
	u_long result;

	int err = xenbus_read_ul(NULL, path, "state", &result, 10);
	if (err)
		result = XenbusStateClosed;

	return result;
}


/*
 * Local variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
