/* $NetBSD: a12c_pci.c,v 1.3 2000/06/29 08:58:45 mrg Exp $ */

/* [Notice revision 2.0]
 * Copyright (c) 1997 Avalon Computer Systems, Inc.
 * All rights reserved.
 *
 * Author: Ross Harvey
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright and
 *    author notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Avalon Computer Systems, Inc. nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. This copyright will be assigned to The NetBSD Foundation on
 *    1/1/2000 unless these terms (including possibly the assignment
 *    date) are updated in writing by Avalon prior to the latest specified
 *    assignment date.
 *
 * THIS SOFTWARE IS PROVIDED BY AVALON COMPUTER SYSTEMS, INC. AND CONTRIBUTORS
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

#include "opt_avalon_a12.h"		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: a12c_pci.c,v 1.3 2000/06/29 08:58:45 mrg Exp $");
__KERNEL_COPYRIGHT(0,
    "Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/a12creg.h>
#include <alpha/pci/a12cvar.h>

#include <machine/rpb.h>	/* XXX for eb164 CIA firmware workarounds. */

#define	A12C_PCI()	/* Generate ctags(1) key */

void		a12c_attach_hook __P((struct device *, struct device *,
		    struct pcibus_attach_args *));
int		a12c_bus_maxdevs __P((void *, int));
pcitag_t	a12c_make_tag __P((void *, int, int, int));
void		a12c_decompose_tag __P((void *, pcitag_t, int *, int *,
		    int *));
pcireg_t	a12c_conf_read __P((void *, pcitag_t, int));
void		a12c_conf_write __P((void *, pcitag_t, int, pcireg_t));

void
a12c_pci_init(pc, v)
	pci_chipset_tag_t pc;
	void *v;
{

	pc->pc_conf_v = v;
	pc->pc_attach_hook = a12c_attach_hook;
	pc->pc_bus_maxdevs = a12c_bus_maxdevs;
	pc->pc_make_tag = a12c_make_tag;
	pc->pc_decompose_tag = a12c_decompose_tag;
	pc->pc_conf_read = a12c_conf_read;
	pc->pc_conf_write = a12c_conf_write;
}

void
a12c_attach_hook(parent, self, pba)
	struct device *parent, *self;
	struct pcibus_attach_args *pba;
{
}

int
a12c_bus_maxdevs(cpv, busno)
	void *cpv;
	int busno;
{
	return 1;
}

pcitag_t
a12c_make_tag(cpv, b, d, f)
	void *cpv;
	int b, d, f;
{

	return (b << 16) | (d << 11) | (f << 8);
}

void
a12c_decompose_tag(cpv, tag, bp, dp, fp)
	void *cpv;
	pcitag_t tag;
	int *bp, *dp, *fp;
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

static void
a12_clear_master_abort(void)
{
	alpha_pal_draina();
	alpha_mb();
	REGVAL(A12_GSR) = REGVAL(A12_GSR) | A12_PCIMasterAbort;
	alpha_mb();
}

static int
a12_check_for_master_abort(void)
{
	alpha_pal_draina();
	alpha_mb();
	return (REGVAL(A12_GSR) & A12_PCIMasterAbort)!=0;
}

static int
a12_set_pci_config_cycle(int offset)
{
	alpha_pal_draina();
	alpha_mb();
	REGVAL(A12_OMR) = REGVAL(A12_OMR)
			| A12_OMR_PCIConfigCycle
			| (offset & 4 ? A12_OMR_PCIAddr2 : 0);
	alpha_mb();
	return offset & ~4;
}

static void
a12_reset_pci_config_cycle(void)
{
	alpha_pal_draina();
	alpha_mb();
	REGVAL(A12_OMR) = REGVAL(A12_OMR)
			& ~(A12_OMR_PCIConfigCycle | A12_OMR_PCIAddr2);
	alpha_mb();
}

pcireg_t
a12c_conf_read(cpv, tag, offset)
	void *cpv;
	pcitag_t tag;
	int offset;
{
	pcireg_t *datap, data;
	int s, ba;
	int32_t old_haxr2;					/* XXX */

	s = 0;					/* XXX gcc -Wuninitialized */
	old_haxr2 = 0;				/* XXX gcc -Wuninitialized */

	if(tag)
		return ~0;

	s = splhigh();
	a12_clear_master_abort();
	offset = a12_set_pci_config_cycle(offset);

	datap = (pcireg_t *)(ALPHA_PHYS_TO_K0SEG(A12_PCITarget+offset));
	data  = ~0L;
	if(!(ba = badaddr(datap, sizeof *datap)))
		data = *datap;

	a12_reset_pci_config_cycle();
	if(a12_check_for_master_abort())
		data = ~0L;

	splx(s);
#if 0
	printf("a12c_conf_read: tag 0x%lx, reg 0x%lx -> %x @ %p%s\n", tag, offset,
	    data, datap, ba ? " (badaddr)" : "");
#endif

	return data;
}



void
a12c_conf_write(cpv, tag, offset, data)
	void *cpv;
	pcitag_t tag;
	int offset;
	pcireg_t data;
{
	pcireg_t *datap;
	int s;

	s = 0;					/* XXX gcc -Wuninitialized */

	if(tag) {
		What();
		return;
	}

	s = splhigh();
	offset = a12_set_pci_config_cycle(offset);
	 datap = (pcireg_t *)(ALPHA_PHYS_TO_K0SEG(A12_PCITarget+offset));
	*datap = data;
	a12_reset_pci_config_cycle();
	splx(s);
#if 0
	printf("a12c_conf_write: tag 0x%lx, reg 0x%lx -> 0x%x @ %p\n", tag,
	    offset, data, datap);
#endif
}
