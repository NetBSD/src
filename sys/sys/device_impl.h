/*	$NetBSD: device_impl.h,v 1.1 2022/03/28 12:38:59 riastradh Exp $	*/

/*
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

#ifndef	_SYS_DEVICE_IMPL_H_
#define	_SYS_DEVICE_IMPL_H_

/*
 * Private autoconf-internal device structures.
 *
 * DO NOT USE outside autoconf internals.
 */

#include <sys/device.h>

struct device {
	devhandle_t	dv_handle;	/* this device's handle;
					   new device_t's get INVALID */
	devclass_t	dv_class;	/* this device's classification */
	TAILQ_ENTRY(device) dv_list;	/* entry on list of all devices */
	cfdata_t	dv_cfdata;	/* config data that found us
					   (NULL if pseudo-device) */
	cfdriver_t	dv_cfdriver;	/* our cfdriver */
	cfattach_t	dv_cfattach;	/* our cfattach */
	int		dv_unit;	/* device unit number */
					/* external name (name + unit) */
	char		dv_xname[DEVICE_XNAME_SIZE];
	device_t	dv_parent;	/* pointer to parent device
					   (NULL if pseudo- or root node) */
	int		dv_depth;	/* number of parents until root */
	int		dv_flags;	/* misc. flags; see below */
	void		*dv_private;	/* this device's private storage */
	int		*dv_locators;	/* our actual locators (optional) */
	prop_dictionary_t dv_properties;/* properties dictionary */
	struct localcount *dv_localcount;/* reference count */

	int		dv_pending;	/* config_pending count */
	TAILQ_ENTRY(device) dv_pending_list;

	struct lwp	*dv_attaching;	/* thread not yet finished in attach */
	struct lwp	*dv_detaching;	/* detach lock (config_misc_lock/cv) */
	bool		dv_detached;	/* config_misc_lock */

	size_t		dv_activity_count;
	void		(**dv_activity_handlers)(device_t, devactive_t);

	bool		(*dv_driver_suspend)(device_t, const pmf_qual_t *);
	bool		(*dv_driver_resume)(device_t, const pmf_qual_t *);
	bool		(*dv_driver_shutdown)(device_t, int);
	bool		(*dv_driver_child_register)(device_t);

	void		*dv_bus_private;
	bool		(*dv_bus_suspend)(device_t, const pmf_qual_t *);
	bool		(*dv_bus_resume)(device_t, const pmf_qual_t *);
	bool		(*dv_bus_shutdown)(device_t, int);
	void		(*dv_bus_deregister)(device_t);

	void		*dv_class_private;
	bool		(*dv_class_suspend)(device_t, const pmf_qual_t *);
	bool		(*dv_class_resume)(device_t, const pmf_qual_t *);
	void		(*dv_class_deregister)(device_t);

	devgen_t		dv_add_gen,
				dv_del_gen;

	struct device_lock	dv_lock;
	const device_suspensor_t
	    *dv_bus_suspensors[DEVICE_SUSPENSORS_MAX],
	    *dv_driver_suspensors[DEVICE_SUSPENSORS_MAX],
	    *dv_class_suspensors[DEVICE_SUSPENSORS_MAX];
	struct device_garbage dv_garbage;
};

/*
 * struct device::dv_flags (must not overlap with device.h struct
 * cfattach::ca_flags for now)
 */
#define	DVF_ACTIVE		0x0001	/* device is activated */
#define	DVF_POWER_HANDLERS	0x0004	/* device has suspend/resume support */
#define	DVF_CLASS_SUSPENDED	0x0008	/* device class suspend was called */
#define	DVF_DRIVER_SUSPENDED	0x0010	/* device driver suspend was called */
#define	DVF_BUS_SUSPENDED	0x0020	/* device bus suspend was called */
#define	DVF_ATTACH_INPROGRESS	0x0040	/* device attach is in progress */

#endif	/* _SYS_DEVICE_IMPL_H_ */
