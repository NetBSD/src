/* $NetBSD: sbpcihb.c,v 1.1.2.1 2010/01/21 04:22:33 matt Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 * 
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 * 
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 * 
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
 * 
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for SB-1250 PCI Host Bridge.
 *
 * Doesn't have to do much at all.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/ppbreg.h>
#include <dev/pci/pcidevs.h>

#include <machine/locore.h>
#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_scd.h>

static int	sbpcihb_match(device_t, cfdata_t, void *);
static void	sbpcihb_attach(device_t, device_t, void *);
static int	sbpcihb_print(void *, const char *);

CFATTACH_DECL_NEW(sbpcihb, 0,
    sbpcihb_match, sbpcihb_attach, NULL, NULL);

bool sbpcihbfound;

static int
sbpcihb_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (sbpcihbfound) {
		return(0);
	}

	/* Check for a vendor/device match.  */
	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SIBYTE &&
	    PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SIBYTE_BCM1250_PCIHB)
		return (1);

	return (0);
}

static void
sbpcihb_attach(device_t parent, device_t self, void *aux)
{
	uint64_t regval;
	int host;

	sbpcihbfound = true;

	/* Tell the user whether it's host or device mode. */
printf("\n\tA_SCD_SYSTEM_CFG: %x\n", A_SCD_SYSTEM_CFG);
	regval = mips3_ld((void *)MIPS_PHYS_TO_KSEG0(A_SCD_SYSTEM_CFG));
	host = (regval & M_SYS_PCI_HOST) != 0;
printf("\tregval: %"PRIx64"\n", regval);
printf("\thost: %x\n", host);

	aprint_normal(": %s mode\n", host ? "host" : "device");

#if 1
        struct pci_attach_args *pa = aux;
        pci_chipset_tag_t pc = pa->pa_pc;
        struct pcibus_attach_args pba;
        pcireg_t busdata;
        char devinfo[256];

        pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo, sizeof(devinfo));
        aprint_normal(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(pa->pa_class));

printf("\tpc: %p\n", pc);
printf("\tpa: %p\n", pa);
printf("\tpa->pa_tag: %"PRIxPTR"\n", (uintptr_t)(pa->pa_tag));
printf("\tPPB_REG_BUSINFO %x\n", PPB_REG_BUSINFO);
        busdata = pci_conf_read(pc, pa->pa_tag, PPB_REG_BUSINFO);
printf("\tbusdata is: %"PRIx32"\n", busdata);

        if (PPB_BUSINFO_PRIMARY(busdata) == 0 &&
            PPB_BUSINFO_SECONDARY(busdata) == 0) {
                aprint_normal_dev(self, "not configured by system firmware\n");
                return;
        }

        /*
         * Attach the PCI bus than hangs off of it.
         *
         * XXX Don't pass-through Memory Read Multiple.  Should we?
         * XXX Consult the spec...
         */
//      pba.pba_busname = "pci";        /* XXX should be pci_ppb attachment */
        pba.pba_iot = pa->pa_iot;
        pba.pba_memt = pa->pa_memt;
        pba.pba_dmat = pa->pa_dmat;                             /* XXXLDT??? */
        pba.pba_pc = pc;                                        /* XXXLDT??? */
        pba.pba_flags = pa->pa_flags & ~PCI_FLAGS_MRM_OKAY;     /* XXXLDT??? */
        pba.pba_bus = PPB_BUSINFO_SECONDARY(busdata);
        pba.pba_intrswiz = pa->pa_intrswiz;                     /* XXXLDT??? */
        pba.pba_intrtag = pa->pa_intrtag;                       /* XXXLDT??? */

        config_found(self, &pba, sbpcihb_print);
#endif
}

int
sbpcihb_print( void *aux, const char *pnp)
{
	struct pcibus_attach_args *pba = aux;

	/* only PCIs can attach to PPBs; easy. */
	if (pnp)
		printf("pci at %s", pnp);
	printf(" bus %d", pba->pba_bus);
	return (UNCONF);
}

