/* $NetBSD: sbbrz_pci.c,v 1.1.2.3 2010/01/21 08:31:24 matt Exp $ */

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

/* from: $NetBSD: apecs_pci.c,v 1.18 2000/06/29 08:58:45 mrg Exp */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: sbbrz_pci.c,v 1.1.2.3 2010/01/21 08:31:24 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <machine/locore.h>
#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_scd.h>
#include <mips/sibyte/include/sb1250_int.h>
#include <mips/sibyte/pci/sbbrzvar.h>

void		sbbrz_pci_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
static int	sbbrz_pci_bus_maxdevs(void *, int);
static pcitag_t	sbbrz_pci_make_tag(void *, int, int, int);
static void	sbbrz_pci_decompose_tag(void *, pcitag_t, int *, int *, int *);

static pcireg_t	sbbrz_pci_conf_read(void *, pcitag_t, int);
static void	sbbrz_pci_conf_write(void *, pcitag_t, int, pcireg_t);
#ifdef PCI_NETBSD_CONFIGURE
static void	sbbrz_pci_conf_interrupt(void *, int, int, int, int, int *);
#endif

static int	sbbrz_pci_intr_map(struct pci_attach_args *,
		    pci_intr_handle_t *);
static const char *
		sbbrz_pci_intr_string(void *, pci_intr_handle_t);
static const struct evcnt *
		sbbrz_pci_intr_evcnt(void *, pci_intr_handle_t);
static void *	sbbrz_pci_intr_establish(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);
static void	sbbrz_pci_intr_disestablish(void *, void *);


void
sbbrz_pci_init(pci_chipset_tag_t pc, void *v)
{

	pc->pc_conf_v = v;
	pc->pc_attach_hook = sbbrz_pci_attach_hook;
	pc->pc_bus_maxdevs = sbbrz_pci_bus_maxdevs;
	pc->pc_make_tag = sbbrz_pci_make_tag;
	pc->pc_decompose_tag = sbbrz_pci_decompose_tag;
	pc->pc_conf_read = sbbrz_pci_conf_read;
	pc->pc_conf_write = sbbrz_pci_conf_write;
	pc->pc_intr_map = sbbrz_pci_intr_map;
	pc->pc_intr_string = sbbrz_pci_intr_string;
	pc->pc_intr_evcnt = sbbrz_pci_intr_evcnt;
	pc->pc_intr_establish = sbbrz_pci_intr_establish;
	pc->pc_intr_disestablish = sbbrz_pci_intr_disestablish;
#ifdef PCI_NETBSD_CONFIGURE
	pc->pc_conf_interrupt = sbbrz_pci_conf_interrupt;
#endif
#ifdef __HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH
	pc->pc_pciide_compat_intr_establish = sbbrz_pciide_compat_intr_establish;
#endif
}

void
sbbrz_pci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

int
sbbrz_pci_bus_maxdevs(void *cpv, int busno)
{
	uint64_t regval;
	int host;

	/* If not the PCI bus directly off the 1250, always up to 32 devs.  */
	if (busno != 0)
		return 32;

	/* If the PCI on the 1250, 32 devices if host mode, otherwise only 2. */
	regval = mips3_ld((void *)MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_CFG));
	host = (regval & M_SYS_PCI_HOST) != 0;

	return (host ? 32 : 2);
}

pcitag_t
sbbrz_pci_make_tag(void *cpv, int b, int d, int f)
{

	return (b << 16) | (d << 11) | (f << 8);
}

void
sbbrz_pci_decompose_tag(void *cpv, pcitag_t tag,
	int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

pcireg_t
sbbrz_pci_conf_read(void *cpv, pcitag_t tag, int offset)
{
	uint64_t addr;
	pcitag_t tmptag;

#ifdef DIAGNOSTIC
	if ((offset & 0x3) != 0)
		panic ("pci_conf_read: misaligned");
#endif

	addr = A_PHYS_LDTPCI_CFG_MATCH_BITS + tag + offset;
#if 0
addr = mips3_ld((void *)MIPS_PHYS_TO_KSEG1(addr));
tmptag = mips3_lw_a64(addr);
printf("s_c_r KSEG1: %"PRIx64"\n", (uint64_t)tmptag);
#endif
	addr = MIPS_PHYS_TO_XKPHYS(MIPS3_TLB_ATTR_UNCACHED, addr);

tmptag = mips3_lw_a64(addr);
printf("s_c_r XKSEG (%"PRIx64"): %"PRIx64"\n", addr, (uint64_t)tmptag);


#ifdef _punt_on_64
	if (badaddr64(addr, 4) != 0)
		return 0xffffffff;
#endif

#if not_yet
	return mips3_lw_a64(addr);
#else
	return tmptag;
#endif
}

void
sbbrz_pci_conf_write(void *cpv, pcitag_t tag, int offset, pcireg_t data)
{
	uint64_t addr;

#ifdef DIAGNOSTIC
	if ((offset & 0x3) != 0)
		panic ("pci_conf_write: misaligned");
#endif

	addr = A_PHYS_LDTPCI_CFG_MATCH_BITS + tag + offset;
	addr = MIPS_PHYS_TO_XKPHYS(MIPS3_TLB_ATTR_UNCACHED, addr);
printf("s_c_W XKSEG (%"PRIx64"): %"PRIx64"\n", addr, (uint64_t)data);

	return mips3_sw_a64(addr, data);
}

int
sbbrz_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int bus, device, func;
	sbbrz_pci_decompose_tag(NULL, pa->pa_intrtag, &bus, &device, &func);
	*ihp = 0;
	if (pa->pa_intrpin == PCI_INTERRUPT_PIN_NONE)
		return EINVAL;
	if (bus == 0) {
		*ihp = K_INT_PCI_INTA
		    + (((device-5) + pa->pa_intrswiz + pa->pa_intrpin - PCI_INTERRUPT_PIN_A) % 4);
		return 0;
	}
	return EOPNOTSUPP;
}

const char *
sbbrz_pci_intr_string(void *v, pci_intr_handle_t ih)
{
	switch (ih) {
	default:		return NULL;
	case K_INT_PCI_INTA:	return "pci inta";
	case K_INT_PCI_INTB:	return "pci intb";
	case K_INT_PCI_INTC:	return "pci intc";
	case K_INT_PCI_INTD:	return "pci intd";
	}
}

const struct evcnt *
sbbrz_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	return NULL;
}

void *
sbbrz_pci_intr_establish(void *v, pci_intr_handle_t ih, int level,
	int (*handler)(void *), void *arg)
{
	return cpu_intr_establish(ih, level,
	    (void (*)(void *, uint32_t, vaddr_t))handler, arg);
}

void
sbbrz_pci_intr_disestablish(void *v, void *ih)
{
}
