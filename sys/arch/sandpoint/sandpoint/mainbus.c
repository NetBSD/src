/*	$NetBSD: mainbus.c,v 1.29.24.1 2015/09/22 12:05:50 skrll Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.29.24.1 2015/09/22 12:05:50 skrll Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/extent.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <machine/autoconf.h>
#include <machine/bootinfo.h>
#include <machine/isa_machdep.h>

#include <powerpc/oea/spr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

int	mainbus_match(device_t, cfdata_t, void *);
void	mainbus_attach(device_t, device_t, void *);
int	mainbus_print(void *, const char *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

struct powerpc_isa_chipset genppc_ict;

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args mba;
	struct pcibus_attach_args pba;
	struct btinfo_prodfamily *pfam;
#if defined(PCI_NETBSD_CONFIGURE)
	struct extent *ioext, *memext;
#endif

	aprint_naive("\n");
	aprint_normal("\n");

	mba.ma_name = "cpu";
	config_found_ia(self, "mainbus", &mba, mainbus_print);

	mba.ma_name = "eumb";
	mba.ma_bst = &sandpoint_eumb_space_tag;
	config_found_ia(self, "mainbus", &mba, mainbus_print);

	pfam = lookup_bootinfo(BTINFO_PRODFAMILY);
	if (pfam != NULL && strcmp(pfam->name, "nhnas") == 0) {
		/* attach nhpow(4) for NH230/231 only */
		mba.ma_name = "nhpow";
		mba.ma_bst = &sandpoint_nhgpio_space_tag;
		config_found_ia(self, "mainbus", &mba, mainbus_print);
	}

	mba.ma_name = "cfi";
	mba.ma_bst = &sandpoint_flash_space_tag;
	config_found_ia(self, "mainbus", &mba, mainbus_print);

	/*
	 * XXX Note also that the presence of a PCI bus should
	 * XXX _always_ be checked, and if present the bus should be
	 * XXX 'found'.  However, because of the structure of the code,
	 * XXX that's not currently possible.
	 */
#if NPCI > 0
#if defined(PCI_NETBSD_CONFIGURE)
	ioext  = extent_create("pciio",  0x00001000, 0x0000ffff,
	    NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem", 0x80000000, 0x8fffffff,
	    NULL, 0, EX_NOWAIT);

	pci_configure_bus(0, ioext, memext, NULL, 0, 32);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif

	pba.pba_iot = &sandpoint_io_space_tag;
	pba.pba_memt = &sandpoint_mem_space_tag;
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = 0;
	pba.pba_pc = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
#endif
}

static int	cpu_match(device_t, cfdata_t, void *);
static void	cpu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, 0, cpu_match, cpu_attach, NULL, NULL);

extern struct cfdriver cpu_cd;

int
cpu_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *mba = aux;

	if (strcmp(mba->ma_name, cpu_cd.cd_name) != 0)
		return 0;

	if (cpu_info[0].ci_dev != NULL)
		return 0;

	return 1;
}

void
cpu_attach(device_t parent, device_t self, void *aux)
{
	static uint8_t mem_to_cpuclk[] = {
		25, 30, 45, 20, 20, 00, 10, 30,
		30, 20, 45, 30, 25, 35, 30, 35,
		20, 25, 20, 30, 35, 40, 40, 20,
		30, 25, 40, 30, 30, 25, 35, 00
	};
	extern u_long ticks_per_sec;
	struct cpu_info *ci;
	u_int hid1, vers;

	ci = cpu_attach_common(self, 0);
	if (ci == NULL)
		return;

	vers = (mfpvr() >> 16) & 0xffff;
	if (ci->ci_khz == 0 && vers == MPC8245) {
		/* calculate speed from bus clock and PLL ratio */
		asm volatile ("mfspr %0,1009" : "=r"(hid1));
		ci->ci_khz = ((uint64_t)ticks_per_sec * 4 *
		    mem_to_cpuclk[hid1 >> 27] + 10) / 10000;
		aprint_normal_dev(self, "%u.%02u MHz\n",
		    ci->ci_khz / 1000, (ci->ci_khz / 10) % 100);
	}
}

int
mainbus_print(void *aux, const char *pnp)
{
	struct mainbus_attach_args *mba = aux;

	if (pnp)
		aprint_normal("%s at %s", mba->ma_name, pnp);
	return UNCONF;
}
