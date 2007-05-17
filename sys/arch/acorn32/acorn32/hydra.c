/*	$NetBSD: hydra.c,v 1.24 2007/05/17 14:51:11 yamt Exp $	*/

/*-
 * Copyright (c) 2002 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
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
 */

#include "opt_multiprocessor.h"

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: hydra.c,v 1.24 2007/05/17 14:51:11 yamt Exp $");

#include <sys/callout.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_pglist.h>

#include <arch/arm/mainbus/mainbus.h>
#include <arch/acorn32/acorn32/hydrareg.h>
#include <arch/acorn32/acorn32/hydravar.h>

#include <machine/bootconfig.h>

#include "locators.h"

struct hydra_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	paddr_t			sc_bootpage_pa;
	vaddr_t			sc_bootpage_va;
	void			*sc_shutdownhook;
	struct callout		sc_prod;
};

struct hydra_attach_args {
	int ha_slave;
};

struct cpu_hydra_softc {
	struct device		sc_dev;
	struct cpu_info		sc_cpuinfo;
};

static int hydra_match(struct device *, struct cfdata *, void *);
static void hydra_attach(struct device *, struct device *, void *);
static int hydra_probe_slave(struct hydra_softc *, int);
static int hydra_print(void *, char const *);
static int hydra_submatch(struct device *, struct cfdata *,
			  const int *, void *);
static void hydra_shutdown(void *);

static void hydra_reset(struct hydra_softc *);
static void hydra_regdump(struct hydra_softc *);

static int cpu_hydra_match(struct device *, struct cfdata *, void *);
static void cpu_hydra_attach(struct device *, struct device *, void *);
static void cpu_hydra_hatch(void);

#if 0
static void hydra_prod(void *);
#endif

CFATTACH_DECL(hydra, sizeof(struct hydra_softc),
    hydra_match, hydra_attach, NULL, NULL);
CFATTACH_DECL(cpu_hydra, sizeof(struct cpu_hydra_softc),
    cpu_hydra_match, cpu_hydra_attach, NULL, NULL);

extern char const hydra_probecode[], hydra_eprobecode[];
extern char const hydra_hatchcode[], hydra_ehatchcode[];
extern struct cpu_info cpu_info_store;

static struct hydra_softc *the_hydra;
struct cpu_info *cpu_info[CPU_MAXNUM] = {&cpu_info_store};

static int
hydra_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *mba = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	/*
	 * Probing for the Hydra is slightly tricky, since if there's
	 * no Hydra, the data we read seem fairly random.  Happily,
	 * nothing else uses its addresses, so we can be as invasive
	 * as we like.
	 */

	iot = mba->mb_iot;
	if (bus_space_map(iot, HYDRA_PHYS_BASE, HYDRA_PHYS_SIZE, 0, &ioh) != 0)
		return 0;

	/* Make sure all slaves are halted. */
	bus_space_write_1(iot, ioh, HYDRA_HALT_SET, 0xf);
	/* Check that we appear to be the master. */
	if (bus_space_read_1(iot, ioh, HYDRA_ID_STATUS) & HYDRA_ID_ISSLAVE)
		goto fail;
	/* Check that the MMU enable bits behave as expected. */
	bus_space_write_1(iot, ioh, HYDRA_MMU_CLR, 0xf);
	if (bus_space_read_1(iot, ioh, HYDRA_MMU_STATUS) != 0x0)
		goto fail;
	bus_space_write_1(iot, ioh, HYDRA_MMU_SET, 0x5);
	if (bus_space_read_1(iot, ioh, HYDRA_MMU_STATUS) != 0x5)
		goto fail;
	bus_space_write_1(iot, ioh, HYDRA_MMU_SET, 0xa);
	if (bus_space_read_1(iot, ioh, HYDRA_MMU_STATUS) != 0xf)
		goto fail;
	bus_space_write_1(iot, ioh, HYDRA_MMU_CLR, 0x5);
	if (bus_space_read_1(iot, ioh, HYDRA_MMU_STATUS) != 0xa)
		goto fail;
	bus_space_write_1(iot, ioh, HYDRA_MMU_CLR, 0xa);
	if (bus_space_read_1(iot, ioh, HYDRA_MMU_STATUS) != 0x0)
		goto fail;
	bus_space_unmap(iot, ioh, HYDRA_PHYS_SIZE);
	return 1;

fail:
	bus_space_unmap(iot, ioh, HYDRA_PHYS_SIZE);
	return 0;
}

static void
hydra_attach(struct device *parent, struct device *self, void *aux)
{
	struct hydra_softc *sc = (void *)self;
	struct mainbus_attach_args *mba = aux;
	int i, vers;
	struct hydra_attach_args ha;
	struct pglist bootpglist;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	sc->sc_iot = mba->mb_iot;
	if (bus_space_map(sc->sc_iot, HYDRA_PHYS_BASE, HYDRA_PHYS_SIZE, 0,
		&sc->sc_ioh) != 0) {
		printf(": cannot map\n");
		return;
	}
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	if (the_hydra == NULL)
		the_hydra = sc;

	/*
	 * The Hydra has special hardware to allow a slave processor
	 * to see something other than ROM at physical address 0 when
	 * it starts.  This something has to have a physical address
	 * on a 2MB boundary.
	 */
	TAILQ_INIT(&bootpglist);
	if (uvm_pglistalloc(PAGE_SIZE, 0x00000000, 0x1fffffff, 0x00200000, 0,
		&bootpglist, 1, 1) != 0) {
		printf(": Can't allocate bootstrap memory.\n");
		return;
	}
	KASSERT(!TAILQ_EMPTY(&bootpglist));
	sc->sc_bootpage_pa = VM_PAGE_TO_PHYS(TAILQ_FIRST(&bootpglist));
	sc->sc_bootpage_va = uvm_km_alloc(kernel_map, PAGE_SIZE, 0,
	    UVM_KMF_VAONLY);
	if (sc->sc_bootpage_va == 0) {
		uvm_pglistfree(&bootpglist);
		printf(": Can't allocate bootstrap memory.\n");
		return;
	}
	pmap_enter(pmap_kernel(), sc->sc_bootpage_va, sc->sc_bootpage_pa,
	    VM_PROT_READ | VM_PROT_WRITE,
	    VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
	pmap_update(pmap_kernel());

	vers = bus_space_read_1(iot, ioh, HYDRA_HARDWAREVER) & 0xf;
	printf(": hardware version %d", vers);

	hydra_reset(sc);

	/* Ensure that the Hydra gets shut down properly. */
	sc->sc_shutdownhook = shutdownhook_establish(hydra_shutdown, sc);

	/* Initialise MMU */
	bus_space_write_1(iot, ioh, HYDRA_MMU_LSN, sc->sc_bootpage_pa >> 21);
	bus_space_write_1(iot, ioh, HYDRA_MMU_MSN, sc->sc_bootpage_pa >> 25);

	printf("\n");

	hydra_regdump(sc);
	hydra_ipi_unicast(4);
	hydra_regdump(sc);
	for (i = 0; i < HYDRA_NSLAVES; i++) {
		if (hydra_probe_slave(sc, i)) {
			ha.ha_slave = i;
			config_found_sm_loc(self, "hydra", NULL, &ha,
			    hydra_print, hydra_submatch);
		}
	}
}

static int
hydra_probe_slave(struct hydra_softc *sc, int slave)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i, ret;

	memcpy((void *)sc->sc_bootpage_va, hydra_probecode,
	    hydra_eprobecode - hydra_probecode);
	bus_space_write_1(iot, ioh, HYDRA_MMU_SET, 1 << slave);
	bus_space_write_1(iot, ioh, HYDRA_HALT_SET, 1 << slave);
	bus_space_write_1(iot, ioh, HYDRA_RESET, 1 << slave);
	bus_space_write_1(iot, ioh, HYDRA_HALT_CLR, 1 << slave);
	bus_space_write_1(iot, ioh, HYDRA_RESET, 0);
	ret = 0;
	for (i = 0; i < 1000; i++) {
		if ((bus_space_read_1(iot, ioh, HYDRA_HALT_STATUS) &
			(1 << slave)) != 0) {
			ret = 1;
			break;
		}
	}
	bus_space_write_1(iot, ioh, HYDRA_HALT_SET, 1 << slave);
	bus_space_write_1(iot, ioh, HYDRA_MMU_CLR, 1 << slave);
	return ret;
}

static int
hydra_print(void *aux, char const *pnp)
{
	struct hydra_attach_args *ha = aux;

	if (pnp)
		aprint_normal("cpu at %s", pnp);
	aprint_normal(" slave %d", ha->ha_slave);
	return UNCONF;
}

static int
hydra_submatch(struct device *parent, struct cfdata *cf,
	       const int *ldesc, void *aux)
{
	struct hydra_attach_args *ha = aux;

	if (cf->cf_loc[HYDRACF_SLAVE] == HYDRACF_SLAVE_DEFAULT ||
	    cf->cf_loc[HYDRACF_SLAVE] == ha->ha_slave)
		return 1;
	return 0;
}

static void
hydra_shutdown(void *arg)
{
	struct hydra_softc *sc = arg;

	hydra_reset(sc);
}

/*
 * hydra_reset: Put the Hydra back into the state it's in after a hard reset.
 * Must be run on the master CPU.
 */
static void
hydra_reset(struct hydra_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	KASSERT((bus_space_read_1(iot, ioh, HYDRA_ID_STATUS) &
	    HYDRA_ID_ISSLAVE) == 0);
	/* Halt all slaves */
	bus_space_write_1(iot, ioh, HYDRA_HALT_SET, 0xf);
	bus_space_write_1(iot, ioh, HYDRA_RESET, 0x0);
	/* Clear IPFIQs to master */
	bus_space_write_1(iot, ioh, HYDRA_FIQ_CLR, 0xf);
	/* ... and to all slaves */
	bus_space_write_1(iot, ioh, HYDRA_FORCEFIQ_CLR, 0xf);
	/* Ditto IPIRQs */
	bus_space_write_1(iot, ioh, HYDRA_IRQ_CLR, 0xf);
	bus_space_write_1(iot, ioh, HYDRA_FORCEIRQ_CLR, 0xf);
	/* Initialise MMU */
	bus_space_write_1(iot, ioh, HYDRA_MMU_LSN, 0);
	bus_space_write_1(iot, ioh, HYDRA_MMU_MSN, 0);
	bus_space_write_1(iot, ioh, HYDRA_MMU_CLR, 0xf);
}

static void
hydra_regdump(struct hydra_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct cpu_info *ci = curcpu();

#define PRINTREG(name, offset)						\
	printf("%s: " name " = %x\n", ci->ci_dev->dv_xname,		\
	    bus_space_read_1(iot, ioh, (offset)) & 0xf)

	PRINTREG("FIQ_status  ", HYDRA_FIQ_STATUS);
	PRINTREG("FIQ_readback", HYDRA_FIQ_READBACK);
	PRINTREG("HardwareVer ", HYDRA_HARDWAREVER);
	PRINTREG("register 3  ", 3);
	PRINTREG("register 4  ", 4);
	PRINTREG("register 5  ", 5);
	PRINTREG("MMU_status  ", HYDRA_MMU_STATUS);
	PRINTREG("ID_status   ", HYDRA_ID_STATUS);
	PRINTREG("IRQ_status  ", HYDRA_IRQ_STATUS);
	PRINTREG("IRQ_readback", HYDRA_IRQ_READBACK);
	PRINTREG("register 10 ", 10);
	PRINTREG("register 11 ", 11);
	PRINTREG("RST_status  ", HYDRA_RST_STATUS);
	PRINTREG("register 13 ", 13);
	PRINTREG("Halt_status ", HYDRA_HALT_STATUS);
	PRINTREG("register 15 ", 15);
}

static int
cpu_hydra_match(struct device *parent, struct cfdata *cf, void *aux)
{

	/* If there's anything there, it's a CPU. */
	return 1;
}

static void
cpu_hydra_attach(struct device *parent, struct device *self, void *aux)
{
	struct hydra_softc *sc = (void *)parent;
	struct cpu_hydra_softc *cpu = (void *)self;
	struct hydra_attach_args *ha = aux;
	int slave = ha->ha_slave;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int error;
	int i;
	struct hydraboot_vars *hb;

	/* Set up a struct cpu_info for this CPU */
	cpu_info[slave | HYDRA_ID_ISSLAVE] = &cpu->sc_cpuinfo;
	cpu->sc_cpuinfo.ci_dev = &cpu->sc_dev;
	cpu->sc_cpuinfo.ci_cpuid = slave | HYDRA_ID_ISSLAVE;

	error = mi_cpu_attach(&cpu->sc_cpuinfo);
	if (error != 0) {
		aprint_normal("\n");
		aprint_error("%s: mi_cpu_attach failed with %d\n",
			    sc->sc_dev.dv_xname, error);
			return;
	}

	/* Copy hatch code to boot page, and set up arguments */
	memcpy((void *)sc->sc_bootpage_va, hydra_hatchcode,
	    hydra_ehatchcode - hydra_hatchcode);
	KASSERT(hydra_ehatchcode - hydra_hatchcode <= HYDRABOOT_VARS);
	hb = (struct hydraboot_vars *)(sc->sc_bootpage_va + HYDRABOOT_VARS);
	hb->hb_ttb = cpu->sc_cpuinfo.ci_idlepcb->pcb_pagedir;
	hb->hb_bootpage_pa = sc->sc_bootpage_pa;
	hb->hb_sp = cpu->sc_cpuinfo.ci_idlepcb->pcb_un.un_32.pcb32_sp;
	hb->hb_entry = &cpu_hydra_hatch;

	cpu_drain_writebuf();

	bus_space_write_1(iot, ioh, HYDRA_MMU_SET, 1 << slave);
	bus_space_write_1(iot, ioh, HYDRA_HALT_SET, 1 << slave);
	bus_space_write_1(iot, ioh, HYDRA_RESET, 1 << slave);
	bus_space_write_1(iot, ioh, HYDRA_HALT_CLR, 1 << slave);
	bus_space_write_1(iot, ioh, HYDRA_RESET, 0);

	/* The slave will halt itself when it's ready. */
	for (i = 0; i < 100000; i++) {
		if ((bus_space_read_1(iot, ioh, HYDRA_HALT_STATUS) &
			(1 << slave)) != 0)
			return;
	}

	printf(": failed to spin up\n");
	bus_space_write_1(iot, ioh, HYDRA_HALT_SET, 1 << slave);
	bus_space_write_1(iot, ioh, HYDRA_MMU_CLR, 1 << slave);
	return;
}

static void
cpu_hydra_hatch(void)
{
	struct hydra_softc *sc = the_hydra;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	cpuid_t cpunum = cpu_number();

	cpu_setup(boot_args);
	cpu_attach(curcpu()->ci_dev);
	bus_space_write_1(iot, ioh,
	    HYDRA_HALT_SET, 1 << (cpunum & 3));
	cpu_tlb_flushID();
	IRQenable
	printf("%s: I am needed?\n", curcpu()->ci_dev->dv_xname);
	for (;;)
		continue;
	SCHED_LOCK(s);
	cpu_switch(NULL, NULL);
}

int
hydra_intr(void)
{
	struct hydra_softc *sc = the_hydra;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int status;

	if (cpu_number() == 0) /* XXX */
	    return 0;
	if (sc == NULL)
		return 0;

	cpu_tlb_flushID();
/*	hydra_regdump(sc); */

	iot = sc->sc_iot;
	ioh = sc->sc_ioh;
	status = bus_space_read_1(iot, ioh, HYDRA_IRQ_STATUS) & 0xf;
	if (status == 0)
		return 0;

	bus_space_write_1(iot, ioh, HYDRA_IRQ_CLR, status);
	printf("%s: Ow! (status = %x)\n", curcpu()->ci_dev->dv_xname, status);
	return 1;
}

void
hydra_ipi_unicast(cpuid_t target)
{
	cpuid_t me = cpu_number();
	struct hydra_softc *sc = the_hydra;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	KASSERT(target != me);
	KASSERT(sc != NULL);
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;
	if (target & HYDRA_ID_ISSLAVE)
		target &= HYDRA_ID_SLAVE_MASK;
	else
		target = me & HYDRA_ID_SLAVE_MASK;
	bus_space_write_1(iot, ioh, HYDRA_IRQ_SET, 1 << target);
}


#ifdef MULTIPROCESSOR
static void
hydra_prod(void *cookie)
{
	struct hydra_softc *sc = cookie;

	hydra_ipi_unicast((random() & HYDRA_ID_SLAVE_MASK) | HYDRA_ID_ISSLAVE);
	callout_reset(&sc->sc_prod, hz * 5, hydra_prod, sc);
}

void
cpu_boot_secondary_processors(void)
{
	struct hydra_softc *sc = the_hydra;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	if (sc == NULL)
		return;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;
	bus_space_write_1(iot, ioh, HYDRA_HALT_CLR, 0xf);
	callout_init(&sc->sc_prod);
	hydra_prod(sc);
}

cpuid_t
cpu_number(void)
{
	struct hydra_softc *sc = the_hydra;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int id;

	if (sc != NULL) {
		iot = sc->sc_iot;
		ioh = sc->sc_ioh;
		id = bus_space_read_1(iot, ioh, HYDRA_ID_STATUS);
		if (id & HYDRA_ID_ISSLAVE)
			return id & (HYDRA_ID_ISSLAVE | HYDRA_ID_SLAVE_MASK);
	}
	return 0;
}

cpuid_t
cpu_next(cpuid_t cpunum)
{

	do
		cpunum++;
	while (cpunum < CPU_MAXNUM && cpu_info[cpunum] == NULL);
	return cpunum;
}
#endif
