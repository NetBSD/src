/* $NetBSD: subr_device.c,v 1.1.2.2 2008/01/30 22:08:51 cube Exp $ */

/*
 * Copyright (c) 1996, 2000 Christopher G. Demetriou
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * --(license Id: LICENSE.proto,v 1.1 2000/06/13 21:40:26 cgd Exp )--
 */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Header: subr_autoconf.c,v 1.12 93/02/01 19:31:48 torek Exp  (LBL)
 *
 *	@(#)subr_autoconf.c	8.3 (Berkeley) 5/17/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_device.c,v 1.1.2.2 2008/01/30 22:08:51 cube Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>

/* list of all devices */
struct devicelist alldevs = TAILQ_HEAD_INITIALIZER(alldevs);

/*
 * device_lookup:
 *
 *	Look up a device instance for a given driver.
 */
void *
device_lookup(cfdriver_t cd, int unit)
{

	if (unit < 0 || unit >= cd->cd_ndevs)
		return (NULL);
	
	return (cd->cd_devs[unit]);
}

/*
 * Accessor functions for the device_t type.
 */
devclass_t
device_class(device_t dev)
{

	return (dev->dv_class);
}

cfdata_t
device_cfdata(device_t dev)
{

	return (dev->dv_cfdata);
}

cfdriver_t
device_cfdriver(device_t dev)
{

	return (dev->dv_cfdriver);
}

cfattach_t
device_cfattach(device_t dev)
{

	return (dev->dv_cfattach);
}

int
device_unit(device_t dev)
{

	return (dev->dv_unit);
}

const char *
device_xname(device_t dev)
{

	return (dev->dv_xname);
}

device_t
device_parent(device_t dev)
{

	return (dev->dv_parent);
}

bool
device_is_active(device_t dev)
{
	int active_flags;

	active_flags = DVF_ACTIVE;
	active_flags |= DVF_CLASS_SUSPENDED;
	active_flags |= DVF_DRIVER_SUSPENDED;
	active_flags |= DVF_BUS_SUSPENDED;

	return ((dev->dv_flags & active_flags) == DVF_ACTIVE);
}

bool
device_is_enabled(device_t dev)
{
	return (dev->dv_flags & DVF_ACTIVE) == DVF_ACTIVE;
}

bool
device_has_power(device_t dev)
{
	int active_flags;

	active_flags = DVF_ACTIVE | DVF_BUS_SUSPENDED;

	return ((dev->dv_flags & active_flags) == DVF_ACTIVE);
}

int
device_locator(device_t dev, u_int locnum)
{

	KASSERT(dev->dv_locators != NULL);
	return (dev->dv_locators[locnum]);
}

void *
device_private(device_t dev)
{

	/*
	 * The reason why device_private(NULL) is allowed is to simplify the
	 * work of a lot of userspace request handlers (i.e., c/bdev
	 * handlers) which grab cfdriver_t->cd_units[n].
	 * It avoids having them test for it to be NULL and only then calling
	 * device_private.
	 */
	return dev == NULL ? NULL : dev->dv_private;
}

prop_dictionary_t
device_properties(device_t dev)
{

	return (dev->dv_properties);
}

/*
 * device_is_a:
 *
 *	Returns true if the device is an instance of the specified
 *	driver.
 */
bool
device_is_a(device_t dev, const char *dname)
{

	return (strcmp(dev->dv_cfdriver->cd_name, dname) == 0);
}

/*
 * Power management related functions.
 */

bool
device_pmf_is_registered(device_t dev)
{
	return (dev->dv_flags & DVF_POWER_HANDLERS) != 0;
}

bool
device_pmf_driver_suspend(device_t dev)
{
	if ((dev->dv_flags & DVF_DRIVER_SUSPENDED) != 0)
		return true;
	if ((dev->dv_flags & DVF_CLASS_SUSPENDED) == 0)
		return false;
	if (*dev->dv_driver_suspend != NULL &&
	    !(*dev->dv_driver_suspend)(dev))
		return false;

	dev->dv_flags |= DVF_DRIVER_SUSPENDED;
	return true;
}

bool
device_pmf_driver_resume(device_t dev)
{
	if ((dev->dv_flags & DVF_DRIVER_SUSPENDED) == 0)
		return true;
	if ((dev->dv_flags & DVF_BUS_SUSPENDED) != 0)
		return false;
	if (*dev->dv_driver_resume != NULL &&
	    !(*dev->dv_driver_resume)(dev))
		return false;

	dev->dv_flags &= ~DVF_DRIVER_SUSPENDED;
	return true;
}

void
device_pmf_driver_register(device_t dev,
    bool (*suspend)(device_t), bool (*resume)(device_t))
{
	dev->dv_driver_suspend = suspend;
	dev->dv_driver_resume = resume;
	dev->dv_flags |= DVF_POWER_HANDLERS;
}

void
device_pmf_driver_deregister(device_t dev)
{
	dev->dv_driver_suspend = NULL;
	dev->dv_driver_resume = NULL;
	dev->dv_flags &= ~DVF_POWER_HANDLERS;
}

bool
device_pmf_driver_child_register(device_t dev)
{
	device_t parent = device_parent(dev);

	if (parent == NULL || parent->dv_driver_child_register == NULL)
		return true;
	return (*parent->dv_driver_child_register)(dev);
}

void
device_pmf_driver_set_child_register(device_t dev,
    bool (*child_register)(device_t))
{
	dev->dv_driver_child_register = child_register;
}

void *
device_pmf_bus_private(device_t dev)
{
	return dev->dv_bus_private;
}

bool
device_pmf_bus_suspend(device_t dev)
{
	if ((dev->dv_flags & DVF_BUS_SUSPENDED) != 0)
		return true;
	if ((dev->dv_flags & DVF_CLASS_SUSPENDED) == 0 ||
	    (dev->dv_flags & DVF_DRIVER_SUSPENDED) == 0)
		return false;
	if (*dev->dv_bus_suspend != NULL &&
	    !(*dev->dv_bus_suspend)(dev))
		return false;

	dev->dv_flags |= DVF_BUS_SUSPENDED;
	return true;
}

bool
device_pmf_bus_resume(device_t dev)
{
	if ((dev->dv_flags & DVF_BUS_SUSPENDED) == 0)
		return true;
	if (*dev->dv_bus_resume != NULL &&
	    !(*dev->dv_bus_resume)(dev))
		return false;

	dev->dv_flags &= ~DVF_BUS_SUSPENDED;
	return true;
}

void
device_pmf_bus_register(device_t dev, void *priv,
    bool (*suspend)(device_t), bool (*resume)(device_t),
    void (*deregister)(device_t))
{
	dev->dv_bus_private = priv;
	dev->dv_bus_resume = resume;
	dev->dv_bus_suspend = suspend;
	dev->dv_bus_deregister = deregister;
}

void
device_pmf_bus_deregister(device_t dev)
{
	if (dev->dv_bus_deregister == NULL)
		return;
	(*dev->dv_bus_deregister)(dev);
	dev->dv_bus_private = NULL;
	dev->dv_bus_suspend = NULL;
	dev->dv_bus_resume = NULL;
	dev->dv_bus_deregister = NULL;
}

void *
device_pmf_class_private(device_t dev)
{
	return dev->dv_class_private;
}

bool
device_pmf_class_suspend(device_t dev)
{
	if ((dev->dv_flags & DVF_CLASS_SUSPENDED) != 0)
		return true;
	if (*dev->dv_class_suspend != NULL &&
	    !(*dev->dv_class_suspend)(dev))
		return false;

	dev->dv_flags |= DVF_CLASS_SUSPENDED;
	return true;
}

bool
device_pmf_class_resume(device_t dev)
{
	if ((dev->dv_flags & DVF_CLASS_SUSPENDED) == 0)
		return true;
	if ((dev->dv_flags & DVF_BUS_SUSPENDED) != 0 ||
	    (dev->dv_flags & DVF_DRIVER_SUSPENDED) != 0)
		return false;
	if (*dev->dv_class_resume != NULL &&
	    !(*dev->dv_class_resume)(dev))
		return false;

	dev->dv_flags &= ~DVF_CLASS_SUSPENDED;
	return true;
}

void
device_pmf_class_register(device_t dev, void *priv,
    bool (*suspend)(device_t), bool (*resume)(device_t),
    void (*deregister)(device_t))
{
	dev->dv_class_private = priv;
	dev->dv_class_suspend = suspend;
	dev->dv_class_resume = resume;
	dev->dv_class_deregister = deregister;
}

void
device_pmf_class_deregister(device_t dev)
{
	if (dev->dv_class_deregister == NULL)
		return;
	(*dev->dv_class_deregister)(dev);
	dev->dv_class_private = NULL;
	dev->dv_class_suspend = NULL;
	dev->dv_class_resume = NULL;
	dev->dv_class_deregister = NULL;
}

bool
device_active(device_t dev, devactive_t type)
{
	size_t i;

	if (dev->dv_activity_count == 0)
		return false;

	for (i = 0; i < dev->dv_activity_count; ++i)
		(*dev->dv_activity_handlers[i])(dev, type);

	return true;
}

bool
device_active_register(device_t dev, void (*handler)(device_t, devactive_t))
{
	void (**new_handlers)(device_t, devactive_t);
	void (**old_handlers)(device_t, devactive_t);
	size_t i, new_size;
	int s;

	old_handlers = dev->dv_activity_handlers;

	for (i = 0; i < dev->dv_activity_count; ++i) {
		if (old_handlers[i] == handler)
			panic("Double registering of idle handlers");
	}

	new_size = dev->dv_activity_count + 1;
	new_handlers = malloc(sizeof(void *) * new_size, M_DEVBUF, M_WAITOK);

	memcpy(new_handlers, old_handlers,
	    sizeof(void *) * dev->dv_activity_count);
	new_handlers[new_size - 1] = handler;

	s = splhigh();
	dev->dv_activity_count = new_size;
	dev->dv_activity_handlers = new_handlers;
	splx(s);

	if (old_handlers != NULL)
		free(old_handlers, M_DEVBUF);

	return true;
}

void
device_active_deregister(device_t dev, void (*handler)(device_t, devactive_t))
{
	void (**new_handlers)(device_t, devactive_t);
	void (**old_handlers)(device_t, devactive_t);
	size_t i, new_size;
	int s;

	old_handlers = dev->dv_activity_handlers;

	for (i = 0; i < dev->dv_activity_count; ++i) {
		if (old_handlers[i] == handler)
			break;
	}

	if (i == dev->dv_activity_count)
		return; /* XXX panic? */

	new_size = dev->dv_activity_count - 1;

	if (new_size == 0) {
		new_handlers = NULL;
	} else {
		new_handlers = malloc(sizeof(void *) * new_size, M_DEVBUF,
		    M_WAITOK);
		memcpy(new_handlers, old_handlers, sizeof(void *) * i);
		memcpy(new_handlers + i, old_handlers + i + 1,
		    sizeof(void *) * (new_size - i));
	}

	s = splhigh();
	dev->dv_activity_count = new_size;
	dev->dv_activity_handlers = new_handlers;
	splx(s);

	free(old_handlers, M_DEVBUF);
}
