/* $NetBSD: mcpcia.c,v 1.1 1998/04/15 00:50:14 mjacob Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: mcpcia.c,v 1.1 1998/04/15 00:50:14 mjacob Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <machine/pte.h>

#include <alpha/mcbus/mcbusreg.h>
#include <alpha/mcbus/mcbusvar.h>
#include <alpha/pci/mcpciareg.h>
#include <alpha/pci/mcpciavar.h>
#include <alpha/pci/pci_kn300.h>

struct mcpcia_softc *mcpcias = NULL;
static struct mcpcia_softc *mcpcia_lt = NULL;

#define KV(_addr)	((caddr_t)ALPHA_PHYS_TO_K0SEG((_addr)))
#define	MCPCIA_SYSBASE(sc)	\
	((((unsigned long) (sc)->mcpcia_gid) << MCBUS_GID_SHIFT) | \
	 (((unsigned long) (sc)->mcpcia_mid) << MCBUS_MID_SHIFT) | \
	 (MCBUS_IOSPACE))

static int	mcpciamatch __P((struct device *, struct cfdata *, void *));
static void	mcpciaattach __P((struct device *, struct device *, void *));
struct cfattach mcpcia_ca = {
	sizeof(struct mcpcia_softc), mcpciamatch, mcpciaattach
};

static int	mcpciaprint __P((void *, const char *));
void	mcpcia_intr_establish __P((struct confargs *, int (*)(void *), void *));
void	mcpcia_intr_disestablish __P((struct confargs *));
caddr_t	mcpcia_cvtaddr __P((struct confargs *));
int	mcpcia_matchname __P((struct confargs *, char *));

static int
mcpciaprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	register struct pcibus_attach_args *pba = aux;
	/* only PCIs can attach to MCPCIA for now */
	if (pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}

static int
mcpciamatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mcbus_dev_attach_args *ma = aux;
	if (ma->ma_type == MCBUS_TYPE_PCI)
		return (1);
	return (0);
}

static void
mcpciaattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	static int first = 1;
	struct mcbus_dev_attach_args *ma = aux;
	struct mcpcia_softc *mcp = (struct mcpcia_softc *)self;
	struct pcibus_attach_args pba;

	mcp->mcpcia_dev = *self;
	mcp->mcpcia_mid = ma->ma_mid;
	mcp->mcpcia_gid = ma->ma_gid;
		
	printf("\n");

	mcpcia_init(mcp);


	mcp->mcpcia_next = NULL;
	if (mcpcia_lt == NULL) {
		mcpcias = mcp;
	} else {
		mcpcia_lt->mcpcia_next = mcp;
	}
	mcpcia_lt = mcp;

	/*
	 * Set up interrupts
	 */
	pci_kn300_pickintr(&mcp->mcpcia_cc, first);
#ifdef	EVCNT_COUNTERS
	if (first == 1) {
		evcnt_attach(self, "intr", kn300_intr_evcnt);
		first = 0;
	}
#else
	first = 0;
#endif

	/*
	 * Attach PCI bus
	 */
	pba.pba_busname = "pci";
	pba.pba_iot = &mcp->mcpcia_cc.cc_iot;
	pba.pba_memt = &mcp->mcpcia_cc.cc_memt;
	pba.pba_dmat =	/* start with direct, may change... */
	    alphabus_dma_get_tag(&mcp->mcpcia_cc.cc_dmat_direct, ALPHA_BUS_PCI);
	pba.pba_pc = &mcp->mcpcia_cc.cc_pc;
	pba.pba_bus = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	config_found(self, &pba, mcpciaprint);
}

void
mcpcia_init(mcp)
	struct mcpcia_softc *mcp;
{
	u_int32_t ctl;
	struct mcpcia_config *ccp = &mcp->mcpcia_cc;

	if (ccp->cc_initted == 0) {
		mcpcia_bus_io_init(&ccp->cc_iot, ccp);
		mcpcia_bus_mem_init(&ccp->cc_memt, ccp);
	}
	mcpcia_pci_init(&ccp->cc_pc, ccp);
	ccp->cc_sc = mcp;

	/*
	 * Establish a precalculated base for convenience's sake.
	 */
	ccp->cc_sysbase = MCPCIA_SYSBASE(mcp);


	ctl = REGVAL(MCPCIA_PCI_REV(mcp));
	printf("%s: Horse Revision %d, %s Handed Saddle Revision %d,"
	    " CAP Revision %d\n", mcp->mcpcia_dev.dv_xname, HORSE_REV(ctl),
	    (SADDLE_TYPE(ctl) & 1)? "Right": "Left", SADDLE_REV(ctl),
	    CAP_REV(ctl));

	/*
 	 * Disable interrupts and clear errors prior to probing
	 */
	REGVAL(MCPCIA_INT_MASK0(mcp)) = 0;
	REGVAL(MCPCIA_INT_MASK1(mcp)) = 0;
	REGVAL(MCPCIA_CAP_ERR(mcp)) = 0xFFFFFFFF;
	alpha_mb();

	/*
	 * Set up DMA stuff for this MCPCIA.
	 */
	mcpcia_dma_init(ccp);

	/*
	 * Clean up any post probe errors (W1TC).
	 */
	REGVAL(MCPCIA_CAP_ERR(mcp)) = 0xFFFFFFFF;
	alpha_mb();

	/*
	 * Use this opportunity to also find out the MID and CPU
	 * type of the currently running CPU (that's us, billybob....)
	 */
	ctl = REGVAL(MCPCIA_WHOAMI(mcp));
	mcbus_primary.mcbus_cpu_mid = MCBUS_CPU_MID(ctl);
	if ((ctl & CPU_Fill_Err) == 0 && mcbus_primary.mcbus_valid == 0) {
		mcbus_primary.mcbus_bcache =
		    MCBUS_CPU_INFO(ctl) & CPU_BCacheMask;
		mcbus_primary.mcbus_valid = 1;
	}
	alpha_mb();
	ccp->cc_initted = 1;
}

void
mcpcia_config_cleanup()
{
	volatile u_int32_t ctl;
	struct mcpcia_softc *mcp;

	/*
	 * Turn on Hard, Soft error interrupts. Maybe i2c too.
	 */
	for (mcp = mcpcias; mcp; mcp = mcp->mcpcia_next) {
		ctl =  REGVAL(MCPCIA_INT_MASK0(mcp));
		ctl |= MCPCIA_GEN_IENABL;
		REGVAL(MCPCIA_INT_MASK0(mcp)) = ctl;
		alpha_mb();
		/* force stall while write completes */
		ctl =  REGVAL(MCPCIA_INT_MASK0(mcp));
	}
}
