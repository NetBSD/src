/*	$NetBSD: amdsmn.c,v 1.3.2.2 2018/02/05 13:06:55 martin Exp $	*/

/*-
 * Copyright (c) 2017 Conrad Meyer <cem@FreeBSD.org>
 * All rights reserved.
 *
 * NetBSD port by Ian Clark <mrrooster@gmail.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: amdsmn.c,v 1.3.2.2 2018/02/05 13:06:55 martin Exp $ ");

/*
 * Driver for the AMD Family 17h CPU System Management Network.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/module.h>

#include <machine/specialreg.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include "amdsmn.h"
#include "ioconf.h"

#define	SMN_ADDR_REG	0x60
#define	SMN_DATA_REG	0x64
#define	AMD_17H_MANAGEMENT_NETWORK_PCI_ID	0x14501022

struct amdsmn_softc {
	kmutex_t smn_lock;
	struct pci_attach_args pa;
	pci_chipset_tag_t pc;
	pcitag_t pcitag;
};

static int amdsmn_match(device_t, cfdata_t, void *);
static void amdsmn_attach(device_t, device_t, void *);
static int amdsmn_rescan(device_t, const char *, const int *);
static int amdsmn_detach(device_t, int);
static int amdsmn_misc_search(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL3_NEW(amdsmn, sizeof(struct amdsmn_softc), amdsmn_match,
    amdsmn_attach, amdsmn_detach, NULL, amdsmn_rescan, NULL, 0);
    
static int
amdsmn_match(device_t parent, cfdata_t match, void *aux) 
{
	struct pci_attach_args *pa = aux;
	
	return pa->pa_id == AMD_17H_MANAGEMENT_NETWORK_PCI_ID ? 2 : 0;
}

static int 
amdsmn_misc_search(device_t parent, cfdata_t cf, const int *locs, void *aux) 
{
	if (config_match(parent, cf, aux))
		config_attach_loc(parent, cf, locs, aux, NULL);

	return 0;
}

static void 
amdsmn_attach(device_t parent, device_t self, void *aux) 
{
	struct amdsmn_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	int flags = 0;

	mutex_init(&sc->smn_lock, MUTEX_DEFAULT, IPL_NONE);
	sc->pa = *pa;
	sc->pc = pa->pa_pc;
	sc->pcitag = pa->pa_tag;
	aprint_normal(": AMD Family 17h System Management Network\n");
	amdsmn_rescan(self, "amdsmn", &flags);
}

static int
amdsmn_rescan(device_t self, const char *ifattr, const int *flags)
{
	struct amdsmn_softc *sc = device_private(self);

	config_search_loc(amdsmn_misc_search, self, ifattr, NULL, &sc->pa);

	return 0;
}

static int
amdsmn_detach(device_t self, int flags) 
{
	struct amdsmn_softc *sc = device_private(self);

	mutex_destroy(&sc->smn_lock);
	aprint_normal_dev(self,"detach!\n");

	return 0;
}

int
amdsmn_read(device_t dev, uint32_t addr, uint32_t *value)
{
	struct amdsmn_softc *sc = device_private(dev);

	mutex_enter(&sc->smn_lock);
	pci_conf_write(sc->pc, sc->pcitag, SMN_ADDR_REG, addr);
	*value = pci_conf_read(sc->pc, sc->pcitag, SMN_DATA_REG);
	mutex_exit(&sc->smn_lock);

	return 0;
}

int
amdsmn_write(device_t dev, uint32_t addr, uint32_t value)
{
	struct amdsmn_softc *sc = device_private(dev);

	mutex_enter(&sc->smn_lock);
	pci_conf_write(sc->pc, sc->pcitag, SMN_ADDR_REG, addr);
	pci_conf_write(sc->pc, sc->pcitag, SMN_DATA_REG, value);
	mutex_exit(&sc->smn_lock);

	return 0;
}

MODULE(MODULE_CLASS_DRIVER, amdsmn, "pci");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
amdsmn_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = config_init_component(cfdriver_ioconf_amdsmn,
		    cfattach_ioconf_amdsmn, cfdata_ioconf_amdsmn);
		break;
	case MODULE_CMD_FINI:
		error = config_fini_component(cfdriver_ioconf_amdsmn,
		    cfattach_ioconf_amdsmn, cfdata_ioconf_amdsmn);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}

