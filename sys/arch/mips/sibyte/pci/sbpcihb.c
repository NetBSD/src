/* $NetBSD: sbpcihb.c,v 1.1.2.2 2010/01/21 07:46:01 matt Exp $ */

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

CFATTACH_DECL_NEW(sbpcihb, 0,
    sbpcihb_match, sbpcihb_attach, NULL, NULL);

static int
sbpcihb_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;
	int bus, device;

	pci_decompose_tag(pa->pa_pc, pa->pa_tag, &bus, &device, NULL);

	/* Check for a vendor/device match.  */
	if (bus == 0 && device == 0
	    && PCI_VENDOR(pa->pa_id) == PCI_VENDOR_SIBYTE
	    && PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_SIBYTE_BCM1250_PCIHB)
		return 1;

	return 0;
}

static void
sbpcihb_attach(device_t parent, device_t self, void *aux)
{
	uint64_t regval;
	bool host;

	/* Tell the user whether it's host or device mode. */
	regval = mips3_ld((void *)MIPS_PHYS_TO_KSEG0(A_SCD_SYSTEM_CFG));
	host = (regval & M_SYS_PCI_HOST) != 0;

	aprint_normal(": %s mode\n", host ? "host" : "device");
}
