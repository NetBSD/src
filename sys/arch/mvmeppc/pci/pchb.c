/*	$NetBSD: pchb.c,v 1.1.14.2 2002/06/23 17:38:37 jdolecek Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

#include <dev/ic/mpc105reg.h>

int	pchbmatch __P((struct device *, struct cfdata *, void *));
void	pchbattach __P((struct device *, struct device *, void *));

struct cfattach pchb_ca = {
	sizeof(struct device), pchbmatch, pchbattach
};

int
pchbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	/*
	 * Match all known PCI host chipsets.
	 */
	if (PCI_CLASS(pa->pa_class) == PCI_CLASS_BRIDGE &&
	    PCI_SUBCLASS(pa->pa_class) == PCI_SUBCLASS_BRIDGE_HOST) {
		return (1);
	}

	return (0);
}

void
pchbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	pcireg_t reg1, reg2;
	char devinfo[256];
	const char *s;

	printf("\n");

	/*
	 * All we do is print out a description.  Eventually, we
	 * might want to add code that does something that's
	 * possibly chipset-specific.
	 */

	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf("%s: %s (rev. 0x%02x)\n", self->dv_xname, devinfo,
	    PCI_REVISION(pa->pa_class));

	switch (PCI_VENDOR(pa->pa_id)) {
	case PCI_VENDOR_MOT:
		switch (PCI_PRODUCT(pa->pa_id)) {
		case PCI_PRODUCT_MOT_MPC105:
			reg1 = pci_conf_read(pa->pa_pc, pa->pa_tag,
			    MPC105_PICR1);
			reg2 = pci_conf_read(pa->pa_pc, pa->pa_tag,
			    MPC105_PICR2);
			printf("%s: L2 cache: ", self->dv_xname);
			switch (reg2 & MPC105_PICR2_L2_SIZE) {
			case MPC105_PICR2_L2_SIZE_256K:
				s = "256K";
				break;
			case MPC105_PICR2_L2_SIZE_512K:
				s = "512K";
				break;
			case MPC105_PICR2_L2_SIZE_1M:
				s = "1M";
				break;
			default:
				s = "reserved size";
				break;
			}
			printf("%s, ", s);
			switch (reg1 & MPC105_PICR1_L2_MP) {
			case MPC105_PICR1_L2_MP_NONE:
				s = "uniprocessor/none";
				break;
			case MPC105_PICR1_L2_MP_WT:
				s = "write-through";
				break;
			case MPC105_PICR1_L2_MP_WB:
				s = "write-back";
				break;
			case MPC105_PICR1_L2_MP_MP:
				s = "multiprocessor";
				break;
			}
			printf("%s mode\n", s);

			/*
			 * Invalidate the L2 cache
			 */
			reg2 &= ~(MPC105_PICR2_L2_UPD_EN | MPC105_PICR2_L2_EN);
			reg2 |= MPC105_PICR2_FLUSH_L2;
			pci_conf_write(pa->pa_pc, pa->pa_tag,
			    MPC105_PICR2, reg2);

			/*
			 * Now enable it.
			 */
			reg2 &= ~MPC105_PICR2_FLUSH_L2;
			reg2 |= MPC105_PICR2_L2_UPD_EN | MPC105_PICR2_L2_EN;
			pci_conf_write(pa->pa_pc, pa->pa_tag,
			    MPC105_PICR2, reg2);
			break;
		}
	}
}
