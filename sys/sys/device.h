/* $NetBSD: device.h,v 1.99.4.1 2007/12/16 18:54:07 cube Exp $ */

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
 *	@(#)device.h	8.2 (Berkeley) 2/17/94
 */

#ifndef _SYS_DEVICE_H_
#define	_SYS_DEVICE_H_

#include <sys/autoconf.h>
#include <sys/queue.h>

/* XXX namespace */
#include <sys/pmf.h>

#include <prop/proplib.h>

struct device {
	devclass_t	dv_class;	/* this device's classification */
	TAILQ_ENTRY(device) dv_list;	/* entry on list of all devices */
	cfdata_t	dv_cfdata;	/* config data that found us
					   (NULL if pseudo-device) */
	cfdriver_t	dv_cfdriver;	/* our cfdriver */
	cfattach_t	dv_cfattach;	/* our cfattach */
	int		dv_unit;	/* device unit number */
	char		dv_xname[16];	/* external name (name + unit) */
	device_t	dv_parent;	/* pointer to parent device
					   (NULL if pseudo- or root node) */
	int		dv_depth;	/* number of parents until root */
	int		dv_flags;	/* misc. flags; see below */
	void		*dv_private;	/* this device's private storage */
	int		*dv_locators;	/* our actual locators (optional) */
	prop_dictionary_t dv_properties;/* properties dictionary */

	size_t		dv_activity_count;
	void		(**dv_activity_handlers)(device_t, devactive_t);

	bool		(*dv_driver_suspend)(device_t);
	bool		(*dv_driver_resume)(device_t);
	bool		(*dv_driver_child_register)(device_t);

	void		*dv_bus_private;
	bool		(*dv_bus_suspend)(device_t);
	bool		(*dv_bus_resume)(device_t);
	void		(*dv_bus_deregister)(device_t);

	void		*dv_class_private;
	bool		(*dv_class_suspend)(device_t);
	bool		(*dv_class_resume)(device_t);
	void		(*dv_class_deregister)(device_t);
};

/* dv_flags */
#define	DVF_ACTIVE		0x0001	/* device is activated */
#define	DVF_PRIV_ALLOC		0x0002	/* device private storage != device */
#define	DVF_POWER_HANDLERS	0x0004	/* device has suspend/resume support */
#define	DVF_CLASS_SUSPENDED	0x0008	/* device class suspend was called */
#define	DVF_DRIVER_SUSPENDED	0x0010	/* device driver suspend was called */
#define	DVF_BUS_SUSPENDED	0x0020	/* device bus suspend was called */

TAILQ_HEAD(devicelist, device);

#ifdef _KERNEL

extern struct devicelist alldevs;	/* list of all devices */
extern device_t booted_device;		/* the device we booted from */
extern device_t booted_wedge;		/* the wedge on that device */
extern int booted_partition;		/* or the partition on that device */

void		*device_lookup(cfdriver_t, int);

devclass_t	device_class(device_t);
cfdata_t	device_cfdata(device_t);
cfdriver_t	device_cfdriver(device_t);
cfattach_t	device_cfattach(device_t);
int		device_unit(device_t);
const char	*device_xname(device_t);
device_t	device_parent(device_t);
bool		device_is_active(device_t);
bool		device_is_enabled(device_t);
bool		device_has_power(device_t);
int		device_locator(device_t, u_int);
void		*device_private(device_t);
prop_dictionary_t device_properties(device_t);

bool		device_active(device_t, devactive_t);
bool		device_active_register(device_t,
				       void (*)(device_t, devactive_t));
void		device_active_deregister(device_t,
				         void (*)(device_t, devactive_t));

bool		device_is_a(device_t, const char *);

bool		device_pmf_is_registered(device_t);

bool		device_pmf_driver_suspend(device_t);
bool		device_pmf_driver_resume(device_t);

void		device_pmf_driver_register(device_t,
		    bool (*)(device_t), bool (*)(device_t));
void		device_pmf_driver_deregister(device_t);

bool		device_pmf_driver_child_register(device_t);
void		device_pmf_driver_set_child_register(device_t,
		    bool (*)(device_t));

void		*device_pmf_bus_private(device_t);
bool		device_pmf_bus_suspend(device_t);
bool		device_pmf_bus_resume(device_t);

void		device_pmf_bus_register(device_t, void *,
		    bool (*)(device_t), bool (*)(device_t),
		    void (*)(device_t));
void		device_pmf_bus_deregister(device_t);

void		*device_pmf_class_private(device_t);
bool		device_pmf_class_suspend(device_t);
bool		device_pmf_class_resume(device_t);

void		device_pmf_class_register(device_t, void *,
		    bool (*)(device_t), bool (*)(device_t),
		    void (*)(device_t));
void		device_pmf_class_deregister(device_t);

#endif /* _KERNEL */

#endif /* !_SYS_DEVICE_H_ */
