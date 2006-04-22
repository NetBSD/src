/* $NetBSD: ppbus_conf.c,v 1.9.6.1 2006/04/22 11:39:25 simonb Exp $ */

/*-
 * Copyright (c) 1997, 1998, 1999 Nicolas Souchu
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
 *
 * FreeBSD: src/sys/dev/ppbus/ppbconf.c,v 1.17.2.1 2000/05/24 00:20:57 n_hibma Exp
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ppbus_conf.c,v 1.9.6.1 2006/04/22 11:39:25 simonb Exp $");

#include "opt_ppbus.h"
#include "opt_ppbus_1284.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <dev/ppbus/ppbus_1284.h>
#include <dev/ppbus/ppbus_base.h>
#include <dev/ppbus/ppbus_conf.h>
#include <dev/ppbus/ppbus_device.h>
#include <dev/ppbus/ppbus_var.h>

/* Probe, attach, and detach functions for ppbus. */
static int ppbus_probe(struct device *, struct cfdata *, void *);
static void ppbus_attach(struct device *, struct device *, void *);
static int ppbus_detach(struct device *, int);

/* Utility function prototypes */
static int ppbus_search_children(struct device *, struct cfdata *,
				 const int *, void *);


CFATTACH_DECL(ppbus, sizeof(struct ppbus_softc), ppbus_probe, ppbus_attach,
	ppbus_detach, NULL);

/* Probe function for ppbus. */
static int
ppbus_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	struct parport_adapter *sc_link = aux;

	/* Check adapter for consistency */
	if (
		/* Required methods for all parports */
		sc_link->parport_io == NULL ||
		sc_link->parport_exec_microseq == NULL ||
		sc_link->parport_setmode == NULL ||
		sc_link->parport_getmode == NULL ||
		sc_link->parport_read == NULL ||
		sc_link->parport_write == NULL ||
		sc_link->parport_read_ivar == NULL ||
		sc_link->parport_write_ivar == NULL ||
		/* Methods which conditional exist based on capabilities */
		((sc_link->capabilities & PPBUS_HAS_EPP) &&
		(sc_link->parport_reset_epp_timeout == NULL)) ||
		((sc_link->capabilities & PPBUS_HAS_ECP) &&
		(sc_link->parport_ecp_sync == NULL)) ||
		((sc_link->capabilities & PPBUS_HAS_DMA) &&
		(sc_link->parport_dma_malloc == NULL ||
		sc_link->parport_dma_free == NULL)) ||
		((sc_link->capabilities & PPBUS_HAS_INTR) &&
		(sc_link->parport_add_handler == NULL ||
		sc_link->parport_remove_handler == NULL))
		) {

#ifdef PPBUS_DEBUG
		printf("%s(%s): parport_adaptor is incomplete. Child device "
			"probe failed.\n", __func__, parent->dv_xname);
#endif
		return 0;
	} else {
		return 1;
	}
}

/* Attach function for ppbus. */
static void
ppbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct ppbus_softc *ppbus = device_private(self);
	struct parport_adapter *sc_link = aux;
	struct ppbus_attach_args args;

	printf("\n");

	/* Initialize config data from adapter (bus + device methods) */
	args.capabilities = ppbus->sc_capabilities = sc_link->capabilities;
	ppbus->ppbus_io = sc_link->parport_io;
	ppbus->ppbus_exec_microseq = sc_link->parport_exec_microseq;
	ppbus->ppbus_reset_epp_timeout = sc_link->
		parport_reset_epp_timeout;
	ppbus->ppbus_setmode = sc_link->parport_setmode;
	ppbus->ppbus_getmode = sc_link->parport_getmode;
	ppbus->ppbus_ecp_sync = sc_link->parport_ecp_sync;
	ppbus->ppbus_read = sc_link->parport_read;
	ppbus->ppbus_write = sc_link->parport_write;
        ppbus->ppbus_read_ivar = sc_link->parport_read_ivar;
	ppbus->ppbus_write_ivar = sc_link->parport_write_ivar;
	ppbus->ppbus_dma_malloc = sc_link->parport_dma_malloc;
	ppbus->ppbus_dma_free = sc_link->parport_dma_free;
	ppbus->ppbus_add_handler = sc_link->parport_add_handler;
	ppbus->ppbus_remove_handler = sc_link->parport_remove_handler;

	/* Initially there is no device owning the bus */
	ppbus->ppbus_owner = NULL;

	/* Initialize locking structures */
	lockinit(&(ppbus->sc_lock), PPBUSPRI | PCATCH, "ppbuslock", 0,
		LK_NOWAIT);

	/* Set up bus mode and ieee state */
	ppbus->sc_mode = ppbus->ppbus_getmode(device_parent(self));
	ppbus->sc_use_ieee = 1;
	ppbus->sc_1284_state = PPBUS_FORWARD_IDLE;
	ppbus->sc_1284_error = PPBUS_NO_ERROR;

	/* Record device's sucessful attachment */
	ppbus->sc_dev_ok = PPBUS_OK;

#ifndef DONTPROBE_1284
	/* detect IEEE1284 compliant devices */
	if (ppbus_scan_bus(self)) {
		printf("%s: No IEEE1284 device found.\n", self->dv_xname);
	} else {
		printf("%s: IEEE1284 device found.\n", self->dv_xname);
		/*
		 * Detect device ID (interrupts must be disabled because we
		 * cannot do a ltsleep() to wait for it - no context)
		 */
		if (args.capabilities & PPBUS_HAS_INTR) {
			int val = 0;
			if(ppbus_write_ivar(self, PPBUS_IVAR_INTR, &val) != 0) {
				printf(" <problem initializing interrupt "
					"usage>");
			}
		}
		ppbus_pnp_detect(self);
	}
#endif /* !DONTPROBE_1284 */

	/* Configure child devices */
	SLIST_INIT(&(ppbus->sc_childlist_head));
	config_search_ia(ppbus_search_children, self, "ppbus", &args);

	return;
}

/* Detach function for ppbus. */
static int
ppbus_detach(struct device *self, int flag)
{
	struct ppbus_softc * ppbus = device_private(self);
	struct ppbus_device_softc * child;

	if (ppbus->sc_dev_ok != PPBUS_OK) {
		if (!(flag & DETACH_QUIET))
			printf("%s: detach called on unattached device.\n",
				ppbus->sc_dev.dv_xname);
		if (!(flag & DETACH_FORCE))
			return 0;
		if (!(flag & DETACH_QUIET))
			printf("%s: continuing detach (DETACH_FORCE).\n",
				ppbus->sc_dev.dv_xname);
	}

	if (lockmgr(&(ppbus->sc_lock), LK_DRAIN, NULL)) {
		if (!(flag & DETACH_QUIET))
			printf("%s: error while waiting for lock activity to "
				"end.\n", ppbus->sc_dev.dv_xname);
		if (!(flag & DETACH_FORCE))
			return 0;
		if (!(flag & DETACH_QUIET))
			printf("%s: continuing detach (DETACH_FORCE).\n",
				ppbus->sc_dev.dv_xname);
	}

	/* Detach children devices */
	while (!SLIST_EMPTY(&(ppbus->sc_childlist_head))) {
		child = SLIST_FIRST(&(ppbus->sc_childlist_head));
		config_deactivate((struct device *)child);
		if (config_detach((struct device *)child, flag)) {
			if(!(flag & DETACH_QUIET))
				printf("%s: error detaching %s.",
					ppbus->sc_dev.dv_xname,
					child->sc_dev.dv_xname);
			if(!(flag & DETACH_FORCE))
				return 0;
			if(!(flag & DETACH_QUIET))
				printf("%s: continuing (DETACH_FORCE).\n",
					ppbus->sc_dev.dv_xname);
		}
		SLIST_REMOVE_HEAD(&(ppbus->sc_childlist_head), entries);
	}

	if (!(flag & DETACH_QUIET))
		printf("%s: detached.\n", ppbus->sc_dev.dv_xname);

	return 1;
}

/* Search for children device and add to list */
static int
ppbus_search_children(struct device *parent, struct cfdata *cf,
		      const int *ldesc, void *aux)
{
	struct ppbus_softc *ppbus = (struct ppbus_softc *)parent;
	struct ppbus_device_softc *child;
	int rval = 0;

	if (config_match(parent, cf, aux) > 0) {
		child = (struct ppbus_device_softc *) config_attach(parent,
			cf, aux, NULL);
		if (child) {
			SLIST_INSERT_HEAD(&(ppbus->sc_childlist_head), child,
				entries);
			rval = 1;
		}
	}

	return rval;
}

