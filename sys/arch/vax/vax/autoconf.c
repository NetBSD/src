/*	$NetBSD: autoconf.c,v 1.52 2000/05/21 10:03:55 ragge Exp $	*/

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */

#include "opt_compat_netbsd.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/buf.h>
#include <sys/conf.h>

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/sid.h>
#include <machine/param.h>
#include <machine/vmparam.h>
#include <machine/nexus.h>
#include <machine/ioa.h>
#include <machine/ka820.h>
#include <machine/ka750.h>
#include <machine/ka650.h>
#include <machine/clock.h>
#include <machine/rpb.h>
#include <machine/bus.h>

#include <vax/vax/gencons.h>

#include <dev/bi/bireg.h>

#include "locators.h"

void	gencnslask __P((void));

struct cpu_dep *dep_call;
int	mastercpu;	/* chief of the system */
struct device *booted_from;

#define MAINBUS	0

void
cpu_configure()
{

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("mainbus not configured");

	/*
	 * We're ready to start up. Clear CPU cold start flag.
	 * Soft cold-start flag will be cleared in configure().
	 */
	if (dep_call->cpu_clrf) 
		(*dep_call->cpu_clrf)();
}

void
cpu_rootconf()
{
	struct device *booted_device = NULL;
	int booted_partition = 0;

	/*
	 * The device we booted from are looked for during autoconfig.
	 * There can only be one match.
	 */
	if (rpb.rpb_base != (void *)-1)
		booted_device = booted_from;

#ifdef DEBUG
	printf("booted from type %d unit %d csr 0x%lx adapter %lx slave %d\n",
	    rpb.devtyp, rpb.unit, rpb.csrphy, rpb.adpphy, rpb.slave);
#endif
	printf("boot device: %s\n",
	    booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

int	mainbus_print __P((void *, const char *));
int	mainbus_match __P((struct device *, struct cfdata *, void *));
void	mainbus_attach __P((struct device *, struct device *, void *));

int
mainbus_print(aux, hej)
	void *aux;
	const char *hej;
{
	if (hej)
		printf("nothing at %s", hej);
	return (UNCONF);
}

int
mainbus_match(parent, cf, aux)
	struct	device	*parent;
	struct cfdata *cf;
	void	*aux;
{
	return 1; /* First (and only) mainbus */
}

void
mainbus_attach(parent, self, hej)
	struct	device	*parent, *self;
	void	*hej;
{

	printf("\n");

	/*
	 * Hopefully there a master bus?
	 * Maybe should have this as master instead of mainbus.
	 */
	config_found(self, NULL, mainbus_print);

	if (dep_call->cpu_subconf)
		(*dep_call->cpu_subconf)(self);

#if defined(COMPAT_14)
	if (rpb.rpb_base == (void *)-1)
		printf("\nWARNING: you must update your boot blocks.\n\n");
#endif

}

struct	cfattach mainbus_ca = {
	sizeof(struct device), mainbus_match, mainbus_attach
};

#include "sd.h"
#include "cd.h"
#include "rl.h"
#include "ra.h"
#include "hp.h"

int	booted_qe(struct device *, void *);
int	booted_ze(struct device *, void *);
int	booted_sd(struct device *, void *);
int	booted_rl(struct device *, void *);
int	booted_ra(struct device *, void *);
int	booted_hp(struct device *, void *);

int (*devreg[])(struct device *, void *) = {
	booted_qe,
	booted_ze,
#if NSD > 0 || NCD > 0
	booted_sd,
#endif
#if NRL > 0
	booted_rl,
#endif
#if NRA
	booted_ra,
#endif
#if NHP
	booted_hp,
#endif
	0,
};

#define	ubreg(x) ((x) & 017777)

void
device_register(dev, aux)
	struct device *dev;
	void *aux;
{
	int (**dp)(struct device *, void *) = devreg;

	while (*dp) {
		if ((*dp)(dev, aux)) {
			booted_from = dev;
			break;
		}
		dp++;
	}
}

int
booted_ze(dev, aux)
	struct device *dev;
	void *aux;
{
	if ((rpb.devtyp == BDEV_ZE) &&
	    !strcmp("ze", dev->dv_cfdata->cf_driver->cd_name))
		return 1;
	return 0;
}

int
booted_qe(dev, aux)
	struct device *dev;
	void *aux;
{
	if ((rpb.devtyp == BDEV_QE) &&
	    !strcmp("qe", dev->dv_cfdata->cf_driver->cd_name) &&
	    (ubreg(dev->dv_cfdata->cf_loc[0]) == ubreg(rpb.csrphy)))
		return 1;
	return 0;
}

#if NSD > 0 || NCD > 0
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipiconf.h>
int
booted_sd(dev, aux)
	struct device *dev;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	struct device *ppdev;

	/* Did we boot from SCSI? */
	if (rpb.devtyp != BDEV_SD)
		return 0;

	/* Is this a SCSI device? */
	if (strcmp("sd", dev->dv_cfdata->cf_driver->cd_name) &&
	    strcmp("cd", dev->dv_cfdata->cf_driver->cd_name))
		return 0;

	if (sa->sa_sc_link->type != BUS_SCSI)
		return 0; /* ``Cannot happen'' */

	if (sa->sa_sc_link->scsipi_scsi.target != rpb.unit)
		return 0; /* Wrong unit */

	ppdev = dev->dv_parent->dv_parent;

	/* VS3100 NCR 53C80 */
	if ((strcmp(ppdev->dv_cfdata->cf_driver->cd_name, "ncr") == 0) &&
	    (ppdev->dv_cfdata->cf_loc[0] == rpb.csrphy))
			return 1;

	/* VS4000 NCR 53C94 */
	if ((strcmp(ppdev->dv_cfdata->cf_driver->cd_name, "asc") == 0) &&
	    (ppdev->dv_cfdata->cf_loc[0] == rpb.csrphy))
			return 1;

	return 0; /* Where did we come from??? */
}
#endif
#if NRL > 0
int
booted_rl(dev, aux)
	struct device *dev;
	void *aux;
{
	if ((rpb.devtyp == BDEV_RL) &&
	    !strcmp("rl", dev->dv_cfdata->cf_driver->cd_name))
		return 1; /* XXX should check unit number also */
	return 0;
}
#endif

#if NRA
#include <dev/mscp/mscp.h>
#include <dev/mscp/mscpreg.h>
#include <dev/mscp/mscpvar.h>
int
booted_ra(struct device *dev, void *aux)
{
	struct drive_attach_args *da = aux;
	struct mscp_softc *pdev = (void *)dev->dv_parent;
	paddr_t ioaddr;

	if (rpb.devtyp != BDEV_UDA && rpb.devtyp != BDEV_KDB)
		return 0; /* Not boot device type */
	if (strcmp("ra", dev->dv_cfdata->cf_driver->cd_name))
		return 0; /* Not RA disk drive */

	if (da->da_mp->mscp_unit != rpb.unit)
		return 0; /* Wrong unit number */

	ioaddr = kvtophys(pdev->mi_iph); /* Get phys addr of CSR */
	if (rpb.devtyp == BDEV_UDA && rpb.csrphy == ioaddr)
		return 1; /* Did match CSR */

	if (rpb.devtyp == BDEV_KDB && 0) /* XXX - fix this */
		return 0;
	return 0;
}
#endif
#if NHP
#include <vax/mba/mbavar.h>
int
booted_hp(struct device *dev, void *aux)
{
	static int mbaaddr;

	if (rpb.devtyp != BDEV_HP)
		return 0;

	/* Save last adapter address */
	if (strcmp("mba", dev->dv_cfdata->cf_driver->cd_name) == 0) {
		struct sbi_attach_args *sa = aux;

		mbaaddr = kvtophys(sa->nexaddr);
		return 0;
	}

	if (strcmp("hp", dev->dv_cfdata->cf_driver->cd_name))
		return 0;

	if (((struct mba_attach_args *)aux)->unit != rpb.unit)
		return 0;

	if (mbaaddr != rpb.adpphy)
		return 0;

	return 1;
}
#endif
