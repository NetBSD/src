/*	$NetBSD: icp_ioctl.c,v 1.1 2003/05/13 15:42:34 thorpej Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of Wasabi Systems, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 *       Copyright (c) 2000-01 Intel Corporation
 *       All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * icp_ioctl.c: Ioctl interface for the ICP-Vortex management tools.
 *
 * Based on ICP's FreeBSD "iir" driver ioctl interface, written by
 * Achim Leubner <achim.leubner@intel.com>.
 *
 * This is intended to be ABI-compatile with the ioctl interface for
 * other OSs.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: icp_ioctl.c,v 1.1 2003/05/13 15:42:34 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/ioctl.h>

#include <machine/bus.h>

#include <dev/ic/icpreg.h>
#include <dev/ic/icpvar.h>

/* These are simply the same as ICP's "iir" driver for FreeBSD. */
#define	ICP_DRIVER_VERSION		1
#define	ICP_DRIVER_SUBVERSION		3

static dev_type_open(icpopen);
static dev_type_ioctl(icpioctl);

const struct cdevsw icp_cdevsw = {
	icpopen, nullclose, noread, nowrite, icpioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

extern struct cfdriver icp_cd;

static int
icpopen(dev_t dev, int flag, int mode, struct proc *p)
{

	if (device_lookup(&icp_cd, minor(dev)) == NULL)
		return (ENXIO);

	return (0);
}

static int
icpioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	int error = 0;

	switch (cmd) {
	case GDT_IOCTL_GENERAL:
	    {
		struct icp_softc *icp;
		gdt_ucmd_t *ucmd = (void *) data;

		icp = device_lookup(&icp_cd, ucmd->io_node);
		if (icp == NULL)
			return (ENXIO);

		error = icp_ucmd(icp, ucmd);
		break;
	    }

	case GDT_IOCTL_DRVERS:
		*(int *) data =
		    (ICP_DRIVER_VERSION << 8) | ICP_DRIVER_SUBVERSION;
		break;

	case GDT_IOCTL_CTRTYPE:
	    {
		struct icp_softc *icp;
		gdt_ctrt_t *p = (void *) data;

		icp = device_lookup(&icp_cd, p->io_node);
		if (icp == NULL)
			return (ENXIO);

		/* XXX magic numbers */
		p->oem_id = 0x8000;
		p->type = 0xfd;
		p->info = (icp->icp_pci_bus << 8) | (icp->icp_pci_device << 3);
		p->ext_type = 0x6000 | icp->icp_pci_subdevice_id;
		p->device_id = icp->icp_pci_device_id;
		p->sub_device_id = icp->icp_pci_subdevice_id;
		break;
	    }

	case GDT_IOCTL_OSVERS:
	    {
		gdt_osv_t *p = (void *) data;

		p->oscode = 12;

		/*
		 * __NetBSD_Version__ is encoded thusly:
		 *
		 *	MMmmrrpp00
		 *
		 * M = major version
		 * m = minor version
		 * r = release ["",A-Z[A-Z] but numeric]
		 * p = patchlevel
		 *
		 * Since the ABI is not supposed to change between
		 * patchlevels of the same major/minor version, we
		 * will encode major/minor/release into the returned
		 * data.
		 */

		p->version = __NetBSD_Version__ / 100000000;
		p->subversion = (__NetBSD_Version__ / 1000000) % 100;
		p->revision = (__NetBSD_Version__ / 10000) % 100;

		strcpy(p->name, ostype);
		break;
	    }

	case GDT_IOCTL_CTRCNT:
		*(int *) data = icp_count;
		break;

	case GDT_IOCTL_EVENT:
	    {
		struct icp_softc *icp;
		gdt_event_t *p = (void *) data;
		gdt_evt_str *e = &p->dvr;
		int s;

		icp = device_lookup(&icp_cd, minor(dev));

		switch (p->erase) {
		case 0xff:
			switch (p->dvr.event_source) {
			case GDT_ES_TEST:
				e->event_data.size =
				    sizeof(e->event_data.eu.test);
				break;

			case GDT_ES_DRIVER:
				e->event_data.size =
				    sizeof(e->event_data.eu.driver);
				break;

			case GDT_ES_SYNC:
				e->event_data.size =
				    sizeof(e->event_data.eu.sync);
				break;

			default:
				e->event_data.size =
				    sizeof(e->event_data.eu.async);
				break;
			}
			s = splbio();
			icp_store_event(icp, e->event_source, e->event_idx,
			    &e->event_data);
			splx(s);
			break;

		case 0xfe:
			s = splbio();
			icp_clear_events(icp);
			splx(s);
			break;

		case 0:
			p->handle = icp_read_event(icp, p->handle, e);
			break;

		default:
			icp_readapp_event(icp, (u_int8_t) p->erase, e);
			break;
		}
		break;
	    }

	case GDT_IOCTL_STATIST:
		memcpy(&icp_stats, data, sizeof(gdt_statist_t));
		break;

	default:
		return (ENOTTY);
	}

	return (error);
}
