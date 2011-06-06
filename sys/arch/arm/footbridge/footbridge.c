/*	$NetBSD: footbridge.c,v 1.23.2.1 2011/06/06 09:05:02 jruoho Exp $	*/

/*
 * Copyright (c) 1997,1998 Mark Brinicombe.
 * Copyright (c) 1997,1998 Causality Limited
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: footbridge.c,v 1.23.2.1 2011/06/06 09:05:02 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <dev/pci/pcivar.h>
#define _ARM32_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/bootconfig.h>

#include <arm/cpuconf.h>
#include <arm/cpufunc.h>

#include <arm/footbridge/footbridgevar.h>
#include <arm/footbridge/dc21285reg.h>
#include <arm/footbridge/dc21285mem.h>
#include <arm/footbridge/footbridge.h>
 
/*
 * DC21285 'Footbridge' device
 *
 * This probes and attaches the footbridge device
 * It then configures any children
 */

/* Declare prototypes */

static int footbridge_match(device_t parent, cfdata_t cf, void *aux);
static void footbridge_attach(device_t parent, device_t self, void *aux);
static int footbridge_print(void *aux, const char *pnp);
static int footbridge_intr(void *arg);

/* Driver and attach structures */
CFATTACH_DECL_NEW(footbridge, sizeof(struct footbridge_softc),
    footbridge_match, footbridge_attach, NULL, NULL);

/* Various bus space tags */
extern struct bus_space footbridge_bs_tag;
extern void footbridge_create_io_bs_tag(bus_space_tag_t t, void *cookie);
extern void footbridge_create_mem_bs_tag(bus_space_tag_t t, void *cookie);
struct bus_space footbridge_csr_tag;
struct bus_space footbridge_pci_io_bs_tag;
struct bus_space footbridge_pci_mem_bs_tag;
extern struct arm32_pci_chipset footbridge_pci_chipset;
extern struct arm32_bus_dma_tag footbridge_pci_bus_dma_tag;
extern struct arm32_dma_range footbridge_dma_ranges[1];

/* Used in footbridge_clock.c */
struct footbridge_softc *clock_sc;

/* Set to non-zero to enable verbose reporting of footbridge system ints */
int footbridge_intr_report = 0;

int footbridge_found;

void
footbridge_pci_bs_tag_init(void)
{
	/* Set up the PCI bus tags */
	footbridge_create_io_bs_tag(&footbridge_pci_io_bs_tag,
	    (void *)DC21285_PCI_IO_VBASE);
	footbridge_create_mem_bs_tag(&footbridge_pci_mem_bs_tag,
	    (void *)DC21285_PCI_MEM_BASE);
}

/*
 * int footbridge_print(void *aux, const char *name)
 *
 * print configuration info for children
 */

static int
footbridge_print(void *aux, const char *pnp)
{
	union footbridge_attach_args *fba = aux;

	if (pnp)
		aprint_normal("%s at %s", fba->fba_name, pnp);
	return(UNCONF);
}

/*
 * int footbridge_match(struct device *parent, struct cfdata *cf, void *aux)
 *
 * Just return ok for this if it is device 0
 */ 
 
static int
footbridge_match(device_t parent, cfdata_t cf, void *aux)
{
	if (footbridge_found)
		return(0);
	return(1);
}


/*
 * void footbridge_attach(device_t parent, device_t dev, void *aux)
 *
 */
  
static void
footbridge_attach(device_t parent, device_t self, void *aux)
{
	struct footbridge_softc *sc = device_private(self);
	union footbridge_attach_args fba;
	int vendor, device, rev;

	/* There can only be 1 footbridge. */
	footbridge_found = 1;

	clock_sc = sc;

	sc->sc_dev = self;
	sc->sc_iot = &footbridge_bs_tag;

	/* Map the Footbridge */
	if (bus_space_map(sc->sc_iot, DC21285_ARMCSR_VBASE,
	     DC21285_ARMCSR_VSIZE, 0, &sc->sc_ioh))
		panic("%s: Cannot map registers", device_xname(self));

	/* Read the ID to make sure it is what we think it is */
	vendor = bus_space_read_2(sc->sc_iot, sc->sc_ioh, VENDOR_ID);
	device = bus_space_read_2(sc->sc_iot, sc->sc_ioh, DEVICE_ID);
	rev = bus_space_read_1(sc->sc_iot, sc->sc_ioh, REVISION);
	if (vendor != DC21285_VENDOR_ID && device != DC21285_DEVICE_ID)
		panic("%s: Unrecognised ID", device_xname(self));

	aprint_normal(": DC21285 rev %d\n", rev);

	/* Disable all interrupts from the footbridge */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, IRQ_ENABLE_CLEAR, 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, FIQ_ENABLE_CLEAR, 0xffffffff);

	/* Install a generic handler to catch a load of system interrupts */
	sc->sc_serr_ih = footbridge_intr_claim(IRQ_SERR, IPL_HIGH,
	    "serr", footbridge_intr, sc);
	sc->sc_sdram_par_ih = footbridge_intr_claim(IRQ_SDRAM_PARITY, IPL_HIGH,
	    "sdram parity", footbridge_intr, sc);
	sc->sc_data_par_ih = footbridge_intr_claim(IRQ_DATA_PARITY, IPL_HIGH,
	    "data parity", footbridge_intr, sc);
	sc->sc_master_abt_ih = footbridge_intr_claim(IRQ_MASTER_ABORT, IPL_HIGH,
	    "mast abt", footbridge_intr, sc);
	sc->sc_target_abt_ih = footbridge_intr_claim(IRQ_TARGET_ABORT, IPL_HIGH,
	    "targ abt", footbridge_intr, sc);
	sc->sc_parity_ih = footbridge_intr_claim(IRQ_PARITY, IPL_HIGH,
	    "parity", footbridge_intr, sc);
	
	/* Set up the PCI bus tags */
	footbridge_create_io_bs_tag(&footbridge_pci_io_bs_tag,
	    (void *)DC21285_PCI_IO_VBASE);
	footbridge_create_mem_bs_tag(&footbridge_pci_mem_bs_tag,
	    (void *)DC21285_PCI_MEM_BASE);

	/* calibrate the delay loop */
	calibrate_delay();

	/*
	 * It seems that the default of the memory being visible from 0 upwards
	 * on the PCI bus causes issues when DMAing from traditional PC VGA
	 * address.  This breaks dumping core on cats, as DMAing pages in the
	 * range 0xb800-0xc000 cause the system to hang.  This suggests that
	 * the VGA BIOS is taking over those addresses.
	 * (note that the range 0xb800-c000 is on an S3 card, others may vary
	 *
	 * To workaround this the SDRAM window on the PCI bus is shifted
	 * to 0x20000000, and the DMA range setup to match.
	 */
	{
		/* first calculate the correct base address mask */
		int memory_size = bootconfig.dram[0].pages * PAGE_SIZE;
		uint32_t mask;

		/* window has to be at least 256KB, and up to 256MB */
		for (mask = 0x00040000; mask < 0x10000000; mask <<= 1)
			if (mask >= memory_size)
				break;
		mask--;
		mask &= SDRAM_MASK_256MB;
		
		/*
		 * configure the mask, the offset into SDRAM and the address
		 * SDRAM is exposed on the PCI bus.
		 */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SDRAM_BA_MASK, mask);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SDRAM_BA_OFFSET, 0);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SDRAM_MEMORY_ADDR, 0x20000000);

		/* configure the dma range for the footbridge to match */
		footbridge_dma_ranges[0].dr_sysbase = bootconfig.dram[0].address;
		footbridge_dma_ranges[0].dr_busbase = 0x20000000;
		footbridge_dma_ranges[0].dr_len = memory_size;
	}

	/* Attach the PCI bus */
	fba.fba_pba.pba_pc = &footbridge_pci_chipset;
	fba.fba_pba.pba_iot = &footbridge_pci_io_bs_tag;
	fba.fba_pba.pba_memt = &footbridge_pci_mem_bs_tag;
	fba.fba_pba.pba_dmat = &footbridge_pci_bus_dma_tag;
	fba.fba_pba.pba_dmat64 = NULL;
	fba.fba_pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
	fba.fba_pba.pba_bus = 0;
	fba.fba_pba.pba_bridgetag = NULL;
	config_found_ia(self, "pcibus", &fba.fba_pba, pcibusprint);

	/* Attach uart device */
	fba.fba_fca.fca_name = "fcom";
	fba.fba_fca.fca_iot = sc->sc_iot;
	fba.fba_fca.fca_ioh = sc->sc_ioh;
	fba.fba_fca.fca_rx_irq = IRQ_SERIAL_RX;
	fba.fba_fca.fca_tx_irq = IRQ_SERIAL_TX;
	config_found_ia(self, "footbridge", &fba.fba_fca, footbridge_print); 
	
	/* Setup fast SA110 cache clean area */
#ifdef CPU_SA110
	if (cputype == CPU_ID_SA110)
		footbridge_sa110_cc_setup();
#endif	/* CPU_SA110 */

}

/* Generic footbridge interrupt handler */

int
footbridge_intr(void *arg)
{
	struct footbridge_softc *sc = arg;
	u_int ctrl, intr;

	/*
	 * Read the footbridge control register and check for
	 * SERR and parity errors
	 */
	ctrl = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SA_CONTROL);
	intr = ctrl & (RECEIVED_SERR | SA_SDRAM_PARITY_ERROR |
	    PCI_SDRAM_PARITY_ERROR | DMA_SDRAM_PARITY_ERROR);
	if (intr) {
		/* Report the interrupt if reporting is enabled */
		if (footbridge_intr_report)
			printf("footbridge_intr: ctrl=%08x\n", intr);
		/* Clear the interrupt state */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, SA_CONTROL,
		    ctrl | intr);
	}
	/*
	 * Read the PCI status register and check for errors
	 */
	ctrl = bus_space_read_4(sc->sc_iot, sc->sc_ioh, PCI_COMMAND_STATUS_REG);
	intr = ctrl & (PCI_STATUS_PARITY_ERROR | PCI_STATUS_MASTER_TARGET_ABORT
	    | PCI_STATUS_MASTER_ABORT | PCI_STATUS_SPECIAL_ERROR
	    | PCI_STATUS_PARITY_DETECT);
	if (intr) {
		/* Report the interrupt if reporting is enabled */
		if (footbridge_intr_report)
			printf("footbridge_intr: pcistat=%08x\n", intr);
		/* Clear the interrupt state */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    PCI_COMMAND_STATUS_REG, ctrl | intr);
	}
	return(0);
}

/* End of footbridge.c */
