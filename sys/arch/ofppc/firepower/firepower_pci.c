/*	$NetBSD: firepower_pci.c,v 1.3 2003/07/15 02:46:31 lukem Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * PCI Configuration Space support for the Powerhouse PowerPro
 * and PowerTop system controllers, found in Firepower systems.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: firepower_pci.c,v 1.3 2003/07/15 02:46:31 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/lock.h>

#include <uvm/uvm_extern.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <ofppc/firepower/firepowerreg.h>
#include <ofppc/firepower/firepowervar.h>

#include <powerpc/pio.h>

void		firepower_pci_attach_hook(struct device *, struct device *,
		    struct pcibus_attach_args *);
int		firepower_pci_bus_maxdevs(void *, int);
pcitag_t	firepower_pci_make_tag(void *, int, int, int);
void		firepower_pci_decompose_tag(void *, pcitag_t, int *, int *,
		    int *);
pcireg_t	firepower_pci_conf_read(void *, pcitag_t, int);
void		firepower_pci_conf_write(void *, pcitag_t, int, pcireg_t);

/* Firepower systems can be multi-processor. */
struct simplelock firepower_pci_conf_slock = SIMPLELOCK_INITIALIZER;

#define	PCI_CONF_LOCK(s)						\
do {									\
	(s) = splhigh();						\
	simple_lock(&firepower_pci_conf_slock);				\
} while (/*CONSTCOND*/0)

#define	PCI_CONF_UNLOCK(s)						\
do {									\
	simple_unlock(&firepower_pci_conf_slock);			\
	splx((s));							\
} while (/*CONSTCOND*/0)

void
firepower_pci_init(pci_chipset_tag_t pc, void *v)
{

	pc->pc_conf_v = v;
	pc->pc_attach_hook = firepower_pci_attach_hook;
	pc->pc_bus_maxdevs = firepower_pci_bus_maxdevs;
	pc->pc_make_tag = firepower_pci_make_tag;
	pc->pc_decompose_tag = firepower_pci_decompose_tag;
	pc->pc_conf_read = firepower_pci_conf_read;
	pc->pc_conf_write = firepower_pci_conf_write;
}

void
firepower_pci_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{
}

int
firepower_pci_bus_maxdevs(void *v, int busno)
{

	return (32);
}

pcitag_t
firepower_pci_make_tag(void *v, int b, int d, int f)
{

	return (b << 16) | (d << 11) | (f << 8);
}

void
firepower_pci_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL) 
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

static int
firepower_pci_conf_addr(int b, int d, int f, int offset, bus_addr_t *addrp)
{
	bus_addr_t addr;

	if (b == 0) {
		/*
		 * Type 0 cycles.  Note the OpenFirmware on the Firepower
		 * numbers the device at ID SEL 11 "0", so we do to.
		 */
		if (d > 8) {
			/* Can't represent this device. */
			return (1);
		}
		addr = FP_MAP_PCICFG_BASE | (1U << (d + 11)) | (f << 8) |
		    offset;
	} else {
		/*
		 * Type 1 cycles.  Note we only get 7 bits for the bus
		 * number in a Type 1 cycle.
		 */
		if (b > 127) {
			/* Can't represent this device. */
			return (1);
		}
		addr = FP_MAP_PCICFG_BASE | (b << 16) | (d << 11) | (f << 8) |
		    offset;
	}

	*addrp = addr;
	return (0);
}

pcireg_t
firepower_pci_conf_read(void *v, pcitag_t tag, int offset)
{
	bus_addr_t addr;
	pcireg_t val;
	int b, d, f, s;

	firepower_pci_decompose_tag(v, tag, &b, &d, &f);
	if (firepower_pci_conf_addr(b, d, f, offset, &addr))
		return ((pcireg_t) -1);

	PCI_CONF_LOCK(s);
	if (b == 0)
		CSR_WRITE4(FPR_PCI_CONFIG_TYPE, PCI_CONFIG_TYPE_0);
	else
		CSR_WRITE4(FPR_PCI_CONFIG_TYPE, PCI_CONFIG_TYPE_1);
	val = in32rb(addr);
	CSR_WRITE4(FPR_PCI_CONFIG_TYPE, PCI_CONFIG_TYPE_0);
	PCI_CONF_UNLOCK(s);

	return (val);
}

void
firepower_pci_conf_write(void *v, pcitag_t tag, int offset, pcireg_t data)
{
	bus_addr_t addr;
	int b, d, f, s;

	firepower_pci_decompose_tag(v, tag, &b, &d, &f);
	if (firepower_pci_conf_addr(b, d, f, offset, &addr))
		return;

	PCI_CONF_LOCK(s);
	if (b == 0)
		CSR_WRITE4(FPR_PCI_CONFIG_TYPE, PCI_CONFIG_TYPE_0);
	else
		CSR_WRITE4(FPR_PCI_CONFIG_TYPE, PCI_CONFIG_TYPE_1);
	out32rb(addr, data);
	CSR_WRITE4(FPR_PCI_CONFIG_TYPE, PCI_CONFIG_TYPE_0);
	PCI_CONF_UNLOCK(s);
}
