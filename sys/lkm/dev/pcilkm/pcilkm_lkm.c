/*	$NetBSD: pcilkm_lkm.c,v 1.2 2005/02/26 22:58:57 perry Exp $	*/

/*
 *  Copyright (c) 2004 The NetBSD Foundation.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to the NetBSD Foundation
 *   by Quentin Garnier.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement:
 *         This product includes software developed by the NetBSD
 *         Foundation, Inc. and its contributors.
 *  4. Neither the name of The NetBSD Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcilkm_lkm.c,v 1.2 2005/02/26 22:58:57 perry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/lkm.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/vmparam.h>
#include <machine/pmap.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <lkm/dev/pcilkm/pcilkm.h>

MOD_MISC("pcilkm");

/* Module functions */
int		pcilkm_lkmentry(struct lkm_table *, int, int);
static int	pcilkm_lkmload(struct lkm_table *, int);
static int	pcilkm_lkmunload(struct lkm_table *, int);
static int	pcilkm_submatch(struct device *, struct cfdata *, void *);
static int	pcilkm_print(void *, const char *);
static int	pcilkm_probe(struct pci_attach_args *);

/* Host functions */
int		pcilkm_hostlkmentry(struct lkm_table *, int, int, struct pcilkm_driver *);
static	int	pcilkm_hostlkmload(struct pcilkm_driver *pld);
static	int	pcilkm_hostlkmunload(struct pcilkm_driver *pld);

/* Non-exported PCI functions */
int		pciprint __P((void *, const char *));
int		pcisubmatch __P((struct device *, struct cfdata *, void *));

static struct pcilkm_driver		*cur_module;
static struct simplelock		curmod_lock;
static int				nclients;

MALLOC_DEFINE(M_PCILKM, "pcilkm", "LKM PCI driver helper");

const struct cfparent			pcilkm_pspec = { "pci", "pci", DVUNIT_ANY };
int					pcilkm_loc[] = { -1, -1 };
const char * const			pcilkmcf_locnames[] = { "dev", "function", NULL };

/* LKM management routines */

int
pcilkm_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{
	DISPATCH(lkmtp, cmd, ver, pcilkm_lkmload, pcilkm_lkmunload, lkm_nofunc);
}

static int
pcilkm_lkmload(struct lkm_table *lkmtp, int cmd)
{
	malloc_type_attach(M_PCILKM);
	simple_lock_init(&curmod_lock);
	nclients = 0;

	return (0);
}

static int
pcilkm_lkmunload(struct lkm_table *lkmtp, int cmd)
{
	if (nclients > 0)
		return (EBUSY);

	malloc_type_detach(M_PCILKM);

	return (0);
}

/* Host LKM management routines */

int
pcilkm_hostlkmentry(struct lkm_table *lkmtp, int cmd, int ver, struct pcilkm_driver *pld)
{
	switch(cmd) {
	int error;
	case LKM_E_LOAD:
		lkmtp->private.lkm_any = (void *)pld->pld_module;
		if ((error = lkmdispatch(lkmtp, cmd)) != 0)
			return error;
		if ((error = pcilkm_hostlkmload(pld)) != 0)
			return error;
		break;
	case LKM_E_UNLOAD:
		if ((error = pcilkm_hostlkmunload(pld)) != 0)
			return error;
		if ((error = lkmdispatch(lkmtp, cmd)) != 0)
			return error;
		break;
	}

	return (0);
}

static int
pcilkm_hostlkmload(struct pcilkm_driver *pld)
{
	int error;

	if ((error = config_cfdriver_attach(pld->pld_cfd)) != 0)
		return error;
	if ((error = config_cfattach_attach(pld->pld_module->lkm_name, pld->pld_cfa)) != 0)
		return error;

	TAILQ_INSERT_TAIL(&allcftables, &pld->pld_cft, ct_list);

	simple_lock(&curmod_lock);
	cur_module = pld;
	(void)pci_find_device(NULL, pcilkm_probe);
	simple_unlock(&curmod_lock);

	if (pld->pld_cfd->cd_ndevs == 0) {
		TAILQ_REMOVE(&allcftables, &pld->pld_cft, ct_list);
		return (ENXIO);
	}

	nclients++;

	return (0);
}

static int
pcilkm_hostlkmunload(struct pcilkm_driver *pld)
{
	int error, i;

	for (i = 0; i<pld->pld_cfd->cd_ndevs; i++)
		if ((error = config_detach(pld->pld_cfd->cd_devs[i], 0)) != 0)
			return error;

	if ((error = config_cfattach_detach(pld->pld_module->lkm_name, pld->pld_cfa)) != 0)
		return error;
	if ((error = config_cfdriver_detach(pld->pld_cfd)) != 0)
		return error;
	TAILQ_REMOVE(&allcftables, &pld->pld_cft, ct_list);

	nclients--;

	return (0);
}

/* PCI bus probe call-back */

static int
pcilkm_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	if (cf != cur_module->pld_cft.ct_cfdata)
		return (0);

	return (pcisubmatch(parent, cf, aux));
}

static int
pcilkm_print(void *aux, const char *pnp)
{
	if (pnp == NULL)
		return pciprint(aux, pnp);

	return (QUIET);
}

static int
pcilkm_probe(struct pci_attach_args *paa)
{
	struct device *parent = device_lookup(&pci_cd, paa->pa_bus);

	if (parent)
		(void)config_found_sm(parent, (void *)paa, pcilkm_print,
		    pcilkm_submatch);

	/*
	 * The way pci_find_device() is designed doesn't
	 * allow us to report a success to the caller,
	 * unfortunately, since returning non-zero makes
	 * the probe stop, although there may be more
	 * cards.
	 */
	return (0);
}
