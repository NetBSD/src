/*	$NetBSD: pci_mace.c,v 1.17.6.3 2016/10/05 20:55:34 skrll Exp $	*/

/*
 * Copyright (c) 2001,2003 Christopher Sekiya
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_mace.c,v 1.17.6.3 2016/10/05 20:55:34 skrll Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/vmparam.h>
#include <sys/bus.h>
#include <machine/machtype.h>

#include <mips/cache.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <sys/extent.h>
#include <sys/malloc.h>
#include <dev/pci/pciconf.h>

#include <sgimips/mace/macereg.h>
#include <sgimips/mace/macevar.h>

#include <sgimips/mace/pcireg_mace.h>

#ifndef __mips_o32
#define USE_HIGH_PCI
#endif


struct macepci_softc {
	struct sgimips_pci_chipset sc_pc;
};

static int	macepci_match(device_t, cfdata_t, void *);
static void	macepci_attach(device_t, device_t, void *);
static int	macepci_bus_maxdevs(pci_chipset_tag_t, int);
static pcireg_t	macepci_conf_read(pci_chipset_tag_t, pcitag_t, int);
static void	macepci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
static int	macepci_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
static const char *
		macepci_intr_string(pci_chipset_tag_t, pci_intr_handle_t,
		    char *, size_t);
static int	macepci_intr(void *);

CFATTACH_DECL_NEW(macepci, sizeof(struct macepci_softc),
    macepci_match, macepci_attach, NULL, NULL);

static void pcimem_bus_mem_init(bus_space_tag_t, void *);
static void pciio_bus_mem_init(bus_space_tag_t, void *);
static struct mips_bus_space	pcimem_mbst;
static struct mips_bus_space	pciio_mbst;
bus_space_tag_t	mace_pci_memt = NULL;
bus_space_tag_t	mace_pci_iot = NULL;

static int
macepci_match(device_t parent, cfdata_t match, void *aux)
{

	return (1);
}

static void
macepci_attach(device_t parent, device_t self, void *aux)
{
	struct macepci_softc *sc = device_private(self);
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct mace_attach_args *maa = aux;
	struct pcibus_attach_args pba;
	u_int32_t control;
	int rev;

	if (bus_space_subregion(maa->maa_st, maa->maa_sh,
	    maa->maa_offset, 0, &pc->ioh) )
		panic("macepci_attach: couldn't map");

	pc->iot = maa->maa_st;

	rev = bus_space_read_4(pc->iot, pc->ioh, MACEPCI_REVISION);
	printf(": rev %d\n", rev);

	pcimem_bus_mem_init(&pcimem_mbst, NULL);
	mace_pci_memt = &pcimem_mbst;
	pciio_bus_mem_init(&pciio_mbst, NULL);
	mace_pci_iot = &pciio_mbst;

	pc->pc_bus_maxdevs = macepci_bus_maxdevs;
	pc->pc_conf_read = macepci_conf_read;
	pc->pc_conf_write = macepci_conf_write;
	pc->pc_intr_map = macepci_intr_map;
	pc->pc_intr_string = macepci_intr_string;
	pc->intr_establish = mace_intr_establish;
	pc->intr_disestablish = mace_intr_disestablish;

	bus_space_write_4(pc->iot, pc->ioh, MACE_PCI_ERROR_ADDR, 0);
	bus_space_write_4(pc->iot, pc->ioh, MACE_PCI_ERROR_FLAGS, 0);

	/* Turn on PCI error interrupts */
	bus_space_write_4(pc->iot, pc->ioh, MACE_PCI_CONTROL,
	    MACE_PCI_CONTROL_SERR_ENA |
	    MACE_PCI_CONTROL_PARITY_ERR |
	    MACE_PCI_CONTROL_PARK_LIU |
	    MACE_PCI_CONTROL_OVERRUN_INT |
	    MACE_PCI_CONTROL_PARITY_INT |
	    MACE_PCI_CONTROL_SERR_INT |
	    MACE_PCI_CONTROL_IT_INT |
	    MACE_PCI_CONTROL_RE_INT |
	    MACE_PCI_CONTROL_DPED_INT |
	    MACE_PCI_CONTROL_TAR_INT |
	    MACE_PCI_CONTROL_MAR_INT);

	/*
	 * Enable all MACE PCI interrupts. They will be masked by
	 * the CRIME code.
	 */
	control = bus_space_read_4(pc->iot, pc->ioh, MACEPCI_CONTROL);
	control |= CONTROL_INT_MASK;
	bus_space_write_4(pc->iot, pc->ioh, MACEPCI_CONTROL, control);

#if NPCI > 0
#ifdef USE_HIGH_PCI
	pc->pc_ioext = extent_create("macepciio", 0x00001000, 0x01ffffff,
	    NULL, 0, EX_NOWAIT);
	pc->pc_memext = extent_create("macepcimem", 0x80000000, 0xffffffff,
	    NULL, 0, EX_NOWAIT);
#else
	pc->pc_ioext = extent_create("macepciio", 0x00001000, 0x01ffffff,
	    NULL, 0, EX_NOWAIT);
	/* XXX no idea why we limit ourselves to only half of the 32MB window */
	pc->pc_memext = extent_create("macepcimem", 0x80100000, 0x81ffffff,
	    NULL, 0, EX_NOWAIT);
#endif /* USE_HIGH_PCI */
	pci_configure_bus(pc, pc->pc_ioext, pc->pc_memext, NULL, 0,
	    mips_cache_info.mci_dcache_align);
	memset(&pba, 0, sizeof pba);
	pba.pba_iot = mace_pci_iot;
	pba.pba_memt = mace_pci_memt;
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	pba.pba_pc = pc;

#ifdef MACEPCI_IO_WAS_BUGGY
	if (rev == 0)
		pba.pba_flags &= ~PCI_FLAGS_IO_OKAY;		/* Buggy? */
#endif

	cpu_intr_establish(maa->maa_intr, IPL_NONE, macepci_intr, sc);

	config_found_ia(self, "pcibus", &pba, pcibusprint);
#endif
}

int
macepci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	if (busno == 0)
		return 5;	/* 2 on-board SCSI chips, slots 0, 1 and 2 */
	else
		return 0;	/* XXX */
}

pcireg_t
macepci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	pcireg_t data;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	bus_space_write_4(pc->iot, pc->ioh, MACE_PCI_CONFIG_ADDR, (tag | reg));
	data = bus_space_read_4(pc->iot, pc->ioh, MACE_PCI_CONFIG_DATA);
	bus_space_write_4(pc->iot, pc->ioh, MACE_PCI_CONFIG_ADDR, 0);

	return data;
}

void
macepci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	bus_space_write_4(pc->iot, pc->ioh, MACE_PCI_CONFIG_ADDR, (tag | reg));
	bus_space_write_4(pc->iot, pc->ioh, MACE_PCI_CONFIG_DATA, data);
	bus_space_write_4(pc->iot, pc->ioh, MACE_PCI_CONFIG_ADDR, 0);
}

int
macepci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t intrtag = pa->pa_intrtag;
	int pin = pa->pa_intrpin;
	int bus, dev, func, start;

	pci_decompose_tag(pc, intrtag, &bus, &dev, &func);

	if (dev < 3 && pin != PCI_INTERRUPT_PIN_A)
		panic("SCSI0 and SCSI1 must be hardwired!");

	switch (pin) {
	default:
	case PCI_INTERRUPT_PIN_NONE:
		return -1;

	case PCI_INTERRUPT_PIN_A:
		/*
		 * Each of SCSI{0,1}, & slots 0 - 2 has dedicated interrupt
		 * for pin A?
		 */
		*ihp = dev + 7;
		return 0;

	case PCI_INTERRUPT_PIN_B:
		start = 0;
		break;
	case PCI_INTERRUPT_PIN_C:
		start = 1;
		break;
	case PCI_INTERRUPT_PIN_D:
		start = 2;
		break;
	}

	/* Pins B,C,D are mapped to PCI_SHARED0 - PCI_SHARED2 interrupts */
	*ihp = 13 /* PCI_SHARED0 */ + (start + dev - 3) % 3;
	return 0;
}

const char *
macepci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih, char *buf,
    size_t len)
{
	snprintf(buf, len, "crime interrupt %d", ih);
	return buf;
}


/*
 * Handle PCI error interrupts.
 */
int
macepci_intr(void *arg)
{
	struct macepci_softc *sc = (struct macepci_softc *)arg;
	pci_chipset_tag_t pc = &sc->sc_pc;
	uint32_t error, address;

	error = bus_space_read_4(pc->iot, pc->ioh, MACE_PCI_ERROR_FLAGS);
	address = bus_space_read_4(pc->iot, pc->ioh, MACE_PCI_ERROR_ADDR);
	if (error & 0xffc00000) {
		if (error & MACE_PERR_MASTER_ABORT) {
			/*
			 * this seems to be a more-or-less normal error
			 * condition (e.g., "pcictl pci0 list" generates
			 * a _lot_ of these errors, so no message for now
			 * while I figure out if I missed a trick somewhere.
			 */
		}

		if (error & MACE_PERR_TARGET_ABORT) {
			printf("mace: target abort at %x\n", address);
		}

		if (error & MACE_PERR_DATA_PARITY_ERR) {
			printf("mace: parity error at %x\n", address);
		}

		if (error & MACE_PERR_RETRY_ERR) {
			printf("mace: retry error at %x\n", address);
		}

		if (error & MACE_PERR_ILLEGAL_CMD) {
			printf("mace: illegal command at %x\n", address);
		}

		if (error & MACE_PERR_SYSTEM_ERR) {
			printf("mace: system error at %x\n", address);
		}

		if (error & MACE_PERR_INTERRUPT_TEST) {
			printf("mace: interrupt test at %x\n", address);
		}

		if (error & MACE_PERR_PARITY_ERR) {
			printf("mace: parity error at %x\n", address);
		}

		if (error & MACE_PERR_RSVD) {
			printf("mace: reserved condition at %x\n", address);
		}

		if (error & MACE_PERR_OVERRUN) {
			printf("mace: overrun at %x\n", address);
		}

		/* clear all */
		bus_space_write_4(pc->iot, pc->ioh,
			    MACE_PCI_ERROR_FLAGS, error & ~0xffc00000);
	}
	return 0;
}

/*
 * use the 32MB windows to access PCI space when running a 32bit kernel,
 * use full views at >4GB in LP64
 * XXX access to PCI space is endian-twiddled which can't be turned off so we
 * need to instruct bus_space to un-twiddle them for us so 8bit and 16bit
 * accesses look little-endian
 */
#define CHIP	   		pcimem
#define	CHIP_MEM		/* defined */
#define CHIP_WRONG_ENDIAN

/*
 * the lower 2GB of PCI space are two views of system memory, with and without
 * endianness twiddling
 */
#define	CHIP_W1_BUS_START(v)	0x80000000UL
#define CHIP_W1_BUS_END(v)	0xffffffffUL
#ifdef USE_HIGH_PCI
#define	CHIP_W1_SYS_START(v)	MACE_PCI_HI_MEMORY
#define	CHIP_W1_SYS_END(v)	MACE_PCI_HI_MEMORY + 0x7fffffffUL
#else
#define	CHIP_W1_SYS_START(v)	MACE_PCI_LOW_MEMORY
#define	CHIP_W1_SYS_END(v)	MACE_PCI_LOW_MEMORY + 0x01ffffffUL
#endif

#include <mips/mips/bus_space_alignstride_chipdep.c>

#undef CHIP
#undef CHIP_W1_BUS_START
#undef CHIP_W1_BUS_END
#undef CHIP_W1_SYS_START
#undef CHIP_W1_SYS_END

#define CHIP	   		pciio
/*
 * Even though it's PCI IO space, it's memory mapped so there is no reason not
 * to allow linear mappings or mmapings into userland. In fact we may need to
 * do just that in order to use things like PCI graphics cards in X.
 */
#define	CHIP_MEM		/* defined */
#define	CHIP_W1_BUS_START(v)	0x00000000UL
#define CHIP_W1_BUS_END(v)	0xffffffffUL
#ifdef USE_HIGH_PCI
#define	CHIP_W1_SYS_START(v)	MACE_PCI_HI_IO
#define	CHIP_W1_SYS_END(v)	MACE_PCI_HI_IO + 0xffffffffUL
#else
#define	CHIP_W1_SYS_START(v)	MACE_PCI_LOW_IO
#define	CHIP_W1_SYS_END(v)	MACE_PCI_LOW_IO + 0x01ffffffUL
#endif

#include <mips/mips/bus_space_alignstride_chipdep.c>
