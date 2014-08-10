/*	$NetBSD: autoconf.c,v 1.95.10.1 2014/08/10 06:54:10 tls Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.95.10.1 2014/08/10 06:54:10 tls Exp $");

#include "opt_compat_netbsd.h"
#include "opt_cputype.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/conf.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/sid.h>
#include <machine/nexus.h>
#include <machine/ioa.h>
#include <machine/ka820.h>
#include <machine/ka750.h>
#include <machine/ka650.h>
#include <machine/clock.h>
#include <machine/rpb.h>
#include <machine/mainbus.h>

#include <vax/vax/gencons.h>

#include <dev/bi/bireg.h>

#include "locators.h"
#include "ioconf.h"

void	gencnslask(void);

const struct cpu_dep *dep_call;

#define MAINBUS	0

void
cpu_configure(void)
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
cpu_rootconf(void)
{
	/*
	 * The device we booted from are looked for during autoconfig.
	 * If there has been a match, it's already been done.
	 */

#ifdef DEBUG
	printf("booted from type %d unit %d csr 0x%lx adapter %lx slave %d\n",
	    rpb.devtyp, rpb.unit, rpb.csrphy, rpb.adpphy, rpb.slave);
#endif
	printf("boot device: %s\n",
	    booted_device ? device_xname(booted_device) : "<unknown>");

	rootconf();
}

static int	mainbus_print(void *, const char *);
static int	mainbus_match(device_t, cfdata_t, void *);
static void	mainbus_attach(device_t, device_t, void *);

extern struct vax_bus_space vax_mem_bus_space;
extern struct vax_bus_dma_tag vax_bus_dma_tag;

int
mainbus_print(void *aux, const char *name)
{
	struct mainbus_attach_args * const ma = aux;
	if (name) {
		aprint_naive("%s at %s", ma->ma_type, name);
		aprint_normal("%s at %s", ma->ma_type, name);
        }
	return UNCONF;
}

int
mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1; /* First (and only) mainbus */
}

void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args ma;
	const char * const * devp;

	aprint_naive("\n");
	aprint_normal("\n");

	for (devp = dep_call->cpu_devs; *devp != NULL; devp++) {
		ma.ma_type = *devp;
		ma.ma_iot = &vax_mem_bus_space;
		ma.ma_dmat = &vax_bus_dma_tag;
		config_found(self, &ma, mainbus_print);
	}

	/*
	 * Hopefully there a master bus?
	 * Maybe should have this as master instead of mainbus.
	 */

#if defined(COMPAT_14)
	if (rpb.rpb_base == (void *)-1)
		printf("\nWARNING: you must update your boot blocks.\n\n");
#endif

}

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

static int	cpu_mainbus_match(device_t, cfdata_t, void *);
static void	cpu_mainbus_attach(device_t, device_t, void *);

int
cpu_mainbus_match(device_t self, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	return strcmp(cpu_cd.cd_name, ma->ma_type) == 0;
}

void
cpu_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct cpu_info *ci;

	KASSERT(device_private(self) == NULL);
	ci = curcpu();
	self->dv_private = ci;
	ci->ci_dev = self;
	ci->ci_cpuid = device_unit(self);

	if (dep_call->cpu_attach_cpu != NULL)
		(*dep_call->cpu_attach_cpu)(self);
	else if (ci->ci_cpustr) {
		aprint_naive(": %s\n", ci->ci_cpustr);
		aprint_normal(": %s\n", ci->ci_cpustr);
        } else {
		aprint_naive("\n");
		aprint_normal("\n");
        }
}

CFATTACH_DECL_NEW(cpu_mainbus, 0,
    cpu_mainbus_match, cpu_mainbus_attach, NULL, NULL);

#include "sd.h"
#include "cd.h"
#include "rl.h"
#include "ra.h"
#include "hp.h"
#include "ry.h"

static int ubtest(void *);
static int jmfr(const char *, device_t, int);
static int booted_qe(device_t, void *);
static int booted_qt(device_t, void *);
static int booted_le(device_t, void *);
static int booted_ze(device_t, void *);
static int booted_de(device_t, void *);
static int booted_ni(device_t, void *);
#if NSD > 0 || NCD > 0
static int booted_sd(device_t, void *);
#endif
#if NRL > 0
static int booted_rl(device_t, void *);
#endif
#if NRA > 0 || NRACD > 0
static int booted_ra(device_t, void *);
#endif
#if NHP
static int booted_hp(device_t, void *);
#endif
#if NRD
static int booted_rd(device_t, void *);
#endif

int (* const devreg[])(device_t, void *) = {
	booted_qe,
	booted_qt,
	booted_le,
	booted_ze,
	booted_de,
	booted_ni,
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
#if NRD
	booted_rd,
#endif
	0,
};

#define	ubreg(x) ((x) & 017777)

void
device_register(device_t dev, void *aux)
{
	int (* const * dp)(device_t, void *) = devreg;

	/* If there's a synthetic RPB, we can't trust it */
	if (rpb.rpb_base == (void *)-1)
		return;

	while (*dp) {
		if ((**dp)(dev, aux)) {
			booted_device = dev;
			break;
		}
		dp++;
	}
}

/*
 * Simple checks. Return 1 on fail.
 */
int
jmfr(const char *n, device_t dev, int nr)
{
	if (rpb.devtyp != nr)
		return 1;
	return !device_is_a(dev, n);
}

#include <dev/qbus/ubavar.h>
int
ubtest(void *aux)
{
	paddr_t p;

	p = kvtophys(((struct uba_attach_args *)aux)->ua_ioh);
	if (rpb.csrphy != p)
		return 1;
	return 0;
}

#if 1 /* NNI */
#include <dev/bi/bivar.h>
int
booted_ni(device_t dev, void *aux)
{
	struct bi_attach_args *ba = aux;

	if (jmfr("ni", dev, BDEV_NI) || (kvtophys(ba->ba_ioh) != rpb.csrphy))
		return 0;

	return 1;
}
#endif /* NNI */

#if 1 /* NDE */
int
booted_de(device_t dev, void *aux)
{

	if (jmfr("de", dev, BDEV_DE) || ubtest(aux))
		return 0;

	return 1;
}
#endif /* NDE */

int
booted_le(device_t dev, void *aux)
{
	if (jmfr("le", dev, BDEV_LE))
		return 0;
	return 1;
}

int
booted_ze(device_t dev, void *aux)
{
	if (jmfr("ze", dev, BDEV_ZE))
		return 0;
	return 1;
}

int
booted_qt(device_t dev, void *aux)
{
	if (jmfr("qt", dev, BDEV_QE) || ubtest(aux))
		return 0;

	return 1;
}

#if 1 /* NQE */
int
booted_qe(device_t dev, void *aux)
{
	if (jmfr("qe", dev, BDEV_QE) || ubtest(aux))
		return 0;

	return 1;
}
#endif /* NQE */

#if NSD > 0 || NCD > 0
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipiconf.h>
int
booted_sd(device_t dev, void *aux)
{
	struct scsipibus_attach_args *sa = aux;
	device_t ppdev;

	/* Is this a SCSI device? */
	if (jmfr("sd", dev, BDEV_SD) && jmfr("sd", dev, BDEV_SDN) &&
	    jmfr("cd", dev, BDEV_SD) && jmfr("cd", dev, BDEV_SDN))
		return 0;

	if (sa->sa_periph->periph_channel->chan_bustype->bustype_type !=
	    SCSIPI_BUSTYPE_SCSI)
		return 0; /* ``Cannot happen'' */

	if (sa->sa_periph->periph_target != (rpb.unit/100) ||
	    sa->sa_periph->periph_lun != (rpb.unit % 100))
		return 0; /* Wrong unit */

	ppdev = device_parent(device_parent(dev));

	/* VS3100 NCR 53C80 (si) & VS4000 NCR 53C94 (asc) */
	if ((jmfr("si",  ppdev, BDEV_SD) == 0 ||	/* new name */
	     jmfr("asc", ppdev, BDEV_SD) == 0 ||
	     jmfr("asc", ppdev, BDEV_SDN) == 0) &&
	    (device_cfdata(ppdev)->cf_loc[VSBUSCF_CSR] == rpb.csrphy))
			return 1;

	return 0; /* Where did we come from??? */
}
#endif
#if NRL > 0
#include <dev/qbus/rlvar.h>
int
booted_rl(device_t dev, void *aux)
{
	struct rlc_attach_args *raa = aux;
	static int ub;

	if (jmfr("rlc", dev, BDEV_RL) == 0)
		ub = ubtest(aux);
	if (ub)
		return 0;
	if (jmfr("rl", dev, BDEV_RL))
		return 0;
	if (raa->hwid != rpb.unit)
		return 0; /* Wrong unit number */
	return 1;
}
#endif

#if NRA > 0 || NRACD > 0
#include <dev/mscp/mscp.h>
#include <dev/mscp/mscpreg.h>
#include <dev/mscp/mscpvar.h>
int
booted_ra(device_t dev, void *aux)
{
	struct drive_attach_args *da = aux;
	struct mscp_softc *pdev = device_private(device_parent(dev));
	paddr_t ioaddr;

	if (jmfr("ra", dev, BDEV_UDA) && jmfr("racd", dev, BDEV_UDA))
		return 0;

	if (da->da_mp->mscp_unit != rpb.unit)
		return 0; /* Wrong unit number */

	ioaddr = kvtophys(pdev->mi_iph); /* Get phys addr of CSR */
	if (rpb.devtyp == BDEV_UDA && rpb.csrphy == ioaddr)
		return 1; /* Did match CSR */

	return 0;
}
#endif

#if NHP
#include <vax/mba/mbavar.h>
int
booted_hp(device_t dev, void *aux)
{
	static int mbaaddr;

	/* Save last adapter address */
	if (jmfr("mba", dev, BDEV_HP) == 0) {
		struct sbi_attach_args *sa = aux;

		mbaaddr = kvtophys(sa->sa_ioh);
		return 0;
	}

	if (jmfr("hp", dev, BDEV_HP))
		return 0;

	if (((struct mba_attach_args *)aux)->ma_unit != rpb.unit)
		return 0;

	if (mbaaddr != rpb.adpphy)
		return 0;

	return 1;
}
#endif
#if NRD
int     
booted_rd(device_t dev, void *aux)
{
	int *nr = aux; /* XXX - use the correct attach struct */

	if (jmfr("rd", dev, BDEV_RD))
		return 0;

	if (*nr != rpb.unit)
		return 0;

	return 1;
}
#endif
