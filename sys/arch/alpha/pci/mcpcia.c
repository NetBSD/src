/* $NetBSD: mcpcia.c,v 1.28.2.1 2012/04/17 00:05:57 yamt Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1998 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
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
 * MCPCIA mcbus to PCI bus adapter
 * found on AlphaServer 4100 systems.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mcpcia.c,v 1.28.2.1 2012/04/17 00:05:57 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <machine/sysarch.h>

#include <alpha/mcbus/mcbusreg.h>
#include <alpha/mcbus/mcbusvar.h>
#include <alpha/pci/mcpciareg.h>
#include <alpha/pci/mcpciavar.h>
#include <alpha/pci/pci_kn300.h>

#define KV(_addr)	((void *)ALPHA_PHYS_TO_K0SEG((_addr)))
#define	MCPCIA_SYSBASE(mc)	\
	((((unsigned long) (mc)->cc_gid) << MCBUS_GID_SHIFT) | \
	 (((unsigned long) (mc)->cc_mid) << MCBUS_MID_SHIFT) | \
	 (MCBUS_IOSPACE))

#define	MCPCIA_PROBE(mid, gid)	\
	badaddr((void *)KV(((((unsigned long) gid) << MCBUS_GID_SHIFT) | \
	 (((unsigned long) mid) << MCBUS_MID_SHIFT) | \
	 (MCBUS_IOSPACE) | MCPCIA_PCI_BRIDGE | _MCPCIA_PCI_REV)), \
	sizeof(uint32_t))

static int	mcpciamatch(device_t, cfdata_t, void *);
static void	mcpciaattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mcpcia, sizeof(struct mcpcia_softc),
    mcpciamatch, mcpciaattach, NULL, NULL);

void	mcpcia_init0(struct mcpcia_config *, int);

/*
 * We have one statically-allocated mcpcia_config structure; this is
 * the one used for the console (which, coincidentally, is the only
 * MCPCIA with an EISA adapter attached to it).
 */
struct mcpcia_config mcpcia_console_configuration;

int	mcpcia_bus_get_window(int, int,
	    struct alpha_bus_space_translation *abst);

static int
mcpciamatch(device_t parent, cfdata_t cf, void *aux)
{
	struct mcbus_dev_attach_args *ma = aux;
	if (ma->ma_type == MCBUS_TYPE_PCI)
		return (1);
	return (0);
}

static void
mcpciaattach(device_t parent, device_t self, void *aux)
{
	static int first = 1;
	struct mcbus_dev_attach_args *ma = aux;
	struct mcpcia_softc *mcp = device_private(self);
	struct mcpcia_config *ccp;
	struct pcibus_attach_args pba;
	uint32_t ctl;

	/*
	 * Make sure this MCPCIA exists...
	 */
	if (MCPCIA_PROBE(ma->ma_mid, ma->ma_gid)) {
		mcp->mcpcia_cc = NULL;
		printf(" (not present)\n");
		return;
	}
	printf("\n");

	/*
	 * Determine if we're the console's MCPCIA.
	 */
	if (ma->ma_mid == mcpcia_console_configuration.cc_mid &&
	    ma->ma_gid == mcpcia_console_configuration.cc_gid)
		ccp = &mcpcia_console_configuration;
	else {
		ccp = malloc(sizeof(struct mcpcia_config), M_DEVBUF, M_WAITOK);
		memset(ccp, 0, sizeof(struct mcpcia_config));

		ccp->cc_mid = ma->ma_mid;
		ccp->cc_gid = ma->ma_gid;
	}

	mcp->mcpcia_dev = self;
	mcp->mcpcia_cc = ccp;
	ccp->cc_sc = mcp;

	/* This initializes cc_sysbase so we can do register access. */
	mcpcia_init0(ccp, 1);

	ctl = REGVAL(MCPCIA_PCI_REV(ccp));
	aprint_normal_dev(self,
	    "Horse Revision %d, %s Handed Saddle Revision %d,"
	    " CAP Revision %d\n", HORSE_REV(ctl),
	    (SADDLE_TYPE(ctl) & 1)? "Right": "Left", SADDLE_REV(ctl),
	    CAP_REV(ctl));

	mcpcia_dma_init(ccp);

	/*
	 * Set up interrupts
	 */
	pci_kn300_pickintr(ccp, first);
	first = 0;

	/*
	 * Attach PCI bus
	 */
	pba.pba_iot = &ccp->cc_iot;
	pba.pba_memt = &ccp->cc_memt;
	pba.pba_dmat =	/* start with direct, may change... */
	    alphabus_dma_get_tag(&ccp->cc_dmat_direct, ALPHA_BUS_PCI);
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &ccp->cc_pc;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	(void) config_found_ia(self, "pcibus", &pba, pcibusprint);

	/*
	 * Clear any errors that may have occurred during the probe
	 * sequence.
	 */
	REGVAL(MCPCIA_CAP_ERR(ccp)) = 0xFFFFFFFF;
	alpha_mb();
}

void
mcpcia_init(void)
{
	struct mcpcia_config *ccp = &mcpcia_console_configuration;
	int i;

	/*
	 * Look for all of the MCPCIAs on the system.  One of them
	 * will have an EISA attached to it.  This MCPCIA is the
	 * only one that can be used for the console.  Once we find
	 * that one, initialize it.
	 */

	for (i = 0; i < MCPCIA_PER_MCBUS; i++) {
		ccp->cc_mid = mcbus_mcpcia_probe_order[i];
		/*
		 * XXX If we ever support more than one MCBUS, we'll
		 * XXX have to probe for them, and map them to unit
		 * XXX numbers.
		 */
		ccp->cc_gid = MCBUS_GID_FROM_INSTANCE(0);
		ccp->cc_sysbase = MCPCIA_SYSBASE(ccp);

		if (badaddr((void *)ALPHA_PHYS_TO_K0SEG(MCPCIA_PCI_REV(ccp)),
		    sizeof(uint32_t)))
			continue;

		if (EISA_PRESENT(REGVAL(MCPCIA_PCI_REV(ccp)))) {
			mcpcia_init0(ccp, 0);

			alpha_bus_window_count[ALPHA_BUS_TYPE_PCI_IO] = 2;
			alpha_bus_window_count[ALPHA_BUS_TYPE_PCI_MEM] = 3;

			alpha_bus_get_window = mcpcia_bus_get_window;
			return;
		}
	}

	panic("mcpcia_init: unable to find EISA bus");
}

void
mcpcia_init0(struct mcpcia_config *ccp, int mallocsafe)
{
	uint32_t ctl;

	if (ccp->cc_initted == 0) {
		/* don't do these twice since they set up extents */
		mcpcia_bus_io_init(&ccp->cc_iot, ccp);
		mcpcia_bus_mem_init(&ccp->cc_memt, ccp);
	}
	ccp->cc_mallocsafe = mallocsafe;

	mcpcia_pci_init(&ccp->cc_pc, ccp);

	/*
	 * Establish a precalculated base for convenience's sake.
	 */
	ccp->cc_sysbase = MCPCIA_SYSBASE(ccp);

	/*
 	 * Disable interrupts and clear errors prior to probing
	 */
	REGVAL(MCPCIA_INT_MASK0(ccp)) = 0;
	REGVAL(MCPCIA_INT_MASK1(ccp)) = 0;
	REGVAL(MCPCIA_CAP_ERR(ccp)) = 0xFFFFFFFF;
	alpha_mb();

	if (ccp == &mcpcia_console_configuration) {
		/*
		 * Use this opportunity to also find out the MID and CPU
		 * type of the currently running CPU (that's us, billybob....)
		 */
		ctl = REGVAL(MCPCIA_WHOAMI(ccp));
		mcbus_primary.mcbus_cpu_mid = MCBUS_CPU_MID(ctl);
		if ((MCBUS_CPU_INFO(ctl) & CPU_Fill_Err) == 0 &&
		    mcbus_primary.mcbus_valid == 0) {
			mcbus_primary.mcbus_bcache =
			    MCBUS_CPU_INFO(ctl) & CPU_BCacheMask;
			mcbus_primary.mcbus_valid = 1;
		}
		alpha_mb();
	}

	ccp->cc_initted = 1;
}

#ifdef TEST_PROBE_DEATH
static void
die_heathen_dog(void *arg)
{
	struct mcpcia_config *ccp = arg;

	/* this causes a fatal machine check (0x670) */
	REGVAL(MCPCIA_CAP_DIAG(ccp)) |= CAP_DIAG_MC_ADRPE;
}
#endif

void
mcpcia_config_cleanup(void)
{
	volatile uint32_t ctl;
	struct mcpcia_softc *mcp;
	struct mcpcia_config *ccp;
	int i;
	extern struct cfdriver mcpcia_cd;

	/*
	 * Turn on Hard, Soft error interrupts. Maybe i2c too.
	 */
	for (i = 0; i < mcpcia_cd.cd_ndevs; i++) {
		if ((mcp = device_lookup_private(&mcpcia_cd, i)) == NULL)
			continue;
		
		ccp = mcp->mcpcia_cc;
		if (ccp == NULL)
			continue;

		ctl = REGVAL(MCPCIA_INT_MASK0(ccp));
		ctl |= MCPCIA_GEN_IENABL;
		REGVAL(MCPCIA_INT_MASK0(ccp)) = ctl;
		alpha_mb();

		/* force stall while write completes */
		ctl = REGVAL(MCPCIA_INT_MASK0(ccp));
	}
#ifdef TEST_PROBE_DEATH
	(void) timeout (die_heathen_dog, &mcpcia_console_configuration,
	    30 * hz);
#endif
}

int
mcpcia_bus_get_window(int type, int window, struct alpha_bus_space_translation *abst)
{
	struct mcpcia_config *ccp = &mcpcia_console_configuration;
	bus_space_tag_t st;

	switch (type) {
	case ALPHA_BUS_TYPE_PCI_IO:
		st = &ccp->cc_iot;
		break;

	case ALPHA_BUS_TYPE_PCI_MEM:
		st = &ccp->cc_memt;
		break;

	default:
		panic("mcpcia_bus_get_window");
	}

	return (alpha_bus_space_get_window(st, window, abst));
}
