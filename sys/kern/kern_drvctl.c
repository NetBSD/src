/* $NetBSD: kern_drvctl.c,v 1.4 2005/12/15 22:01:17 cube Exp $ */

/*
 * Copyright (c) 2004
 * 	Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_drvctl.c,v 1.4 2005/12/15 22:01:17 cube Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/event.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/drvctlio.h>

dev_type_ioctl(drvctlioctl);

const struct cdevsw drvctl_cdevsw = {
	nullopen, nullclose, nullread, nullwrite, drvctlioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

void drvctlattach(int);

#define MAXLOCATORS 100

static int
detachdevbyname(const char *devname)
{
	struct device *d;

	TAILQ_FOREACH(d, &alldevs, dv_list) {
		if (!strcmp(devname, d->dv_xname)) {
#ifndef XXXFULLRISK
			/*
			 * If the parent cannot be notified, it might keep
			 * pointers to the detached device.
			 * There might be a private notification mechanism,
			 * but better play save here.
			 */
			if (d->dv_parent &&
			    !d->dv_parent->dv_cfattach->ca_childdetached)
				return (ENOTSUP);
#endif
			return (config_detach(d, 0));
		}
	}

	return (ENXIO);
}

static int
rescanbus(const char *busname, const char *ifattr,
	  int numlocators, const int *locators)
{
	int i;
	struct device *d;
	const struct cfiattrdata * const *ap;

	/* XXX there should be a way to get limits and defaults (per device)
	   from config generated data */
	int locs[MAXLOCATORS];
	for (i = 0; i < MAXLOCATORS; i++)
		locs[i] = -1;

	for (i = 0; i < numlocators;i++)
		locs[i] = locators[i];

	TAILQ_FOREACH(d, &alldevs, dv_list) {
		if (!strcmp(busname, d->dv_xname)) {
			/*
			 * must support rescan, and must have something
			 * to attach to
			 */
			if (!d->dv_cfattach->ca_rescan ||
			    !d->dv_cfdriver->cd_attrs)
				return (ENODEV);

			/* allow to omit attribute if there is exactly one */
			if (!ifattr) {
				if (d->dv_cfdriver->cd_attrs[1])
					return (EINVAL);
				ifattr = d->dv_cfdriver->cd_attrs[0]->ci_name;
			} else {
				/* check for valid attribute passed */
				for (ap = d->dv_cfdriver->cd_attrs; *ap; ap++)
					if (!strcmp((*ap)->ci_name, ifattr))
						break;
				if (!*ap)
					return (EINVAL);
			}

			return (*d->dv_cfattach->ca_rescan)(d, ifattr, locs);
		}
	}

	return (ENXIO);
}

int
drvctlioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct lwp *p)
{
	int res;
	char *ifattr;
	int *locs;

	switch (cmd) {
	case DRVDETACHDEV:
#define d ((struct devdetachargs *)data)
		res = detachdevbyname(d->devname);
#undef d
		break;
	case DRVRESCANBUS:
#define d ((struct devrescanargs *)data)
		d->busname[sizeof(d->busname) - 1] = '\0';

		/* XXX better copyin? */
		if (d->ifattr[0]) {
			d->ifattr[sizeof(d->ifattr) - 1] = '\0';
			ifattr = d->ifattr;
		} else
			ifattr = 0;

		if (d->numlocators) {
			if (d->numlocators > MAXLOCATORS)
				return (EINVAL);
			locs = malloc(d->numlocators * sizeof(int), M_DEVBUF,
				      M_WAITOK);
			res = copyin(d->locators, locs,
				     d->numlocators * sizeof(int));
			if (res)
				return (res);
		} else
			locs = 0;
		res = rescanbus(d->busname, ifattr, d->numlocators, locs);
		if (locs)
			free(locs, M_DEVBUF);
#undef d
			break;
		default:
			return (EPASSTHROUGH);
	}
	return (res);
}

void
drvctlattach(int arg)
{
}
