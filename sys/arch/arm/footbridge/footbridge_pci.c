/*	$NetBSD: footbridge_pci.c,v 1.22.6.3 2017/12/03 11:35:52 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: footbridge_pci.c,v 1.22.6.3 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>

#define _ARM32_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <arm/footbridge/dc21285reg.h>
#include <arm/footbridge/dc21285mem.h>

#include "isa.h"
#if NISA > 0
#include <dev/isa/isavar.h>
#endif

void		footbridge_pci_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
int		footbridge_pci_bus_maxdevs(void *, int);
pcitag_t	footbridge_pci_make_tag(void *, int, int, int);
void		footbridge_pci_decompose_tag(void *, pcitag_t, int *,
		    int *, int *);
pcireg_t	footbridge_pci_conf_read(void *, pcitag_t, int);
void		footbridge_pci_conf_write(void *, pcitag_t, int,
		    pcireg_t);
int		footbridge_pci_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
const char	*footbridge_pci_intr_string(void *, pci_intr_handle_t,
		    char *, size_t);
void		*footbridge_pci_intr_establish(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);
void		footbridge_pci_intr_disestablish(void *, void *);
const struct evcnt *footbridge_pci_intr_evcnt(void *, pci_intr_handle_t);

struct arm32_pci_chipset footbridge_pci_chipset = {
#ifdef netwinder
	.pc_attach_hook = netwinder_pci_attach_hook,
#else
	.pc_attach_hook = footbridge_pci_attach_hook,
#endif
	.pc_bus_maxdevs = footbridge_pci_bus_maxdevs,
	.pc_make_tag = footbridge_pci_make_tag,
	.pc_decompose_tag = footbridge_pci_decompose_tag,
	.pc_conf_read = footbridge_pci_conf_read,
	.pc_conf_write = footbridge_pci_conf_write,
	.pc_intr_map = footbridge_pci_intr_map,
	.pc_intr_string = footbridge_pci_intr_string,
	.pc_intr_evcnt = footbridge_pci_intr_evcnt,
	.pc_intr_establish = footbridge_pci_intr_establish,
	.pc_intr_disestablish = footbridge_pci_intr_disestablish
};

struct arm32_dma_range footbridge_dma_ranges[1];

/*
 * PCI doesn't have any special needs; just use the generic versions
 * of these functions.
 */
struct arm32_bus_dma_tag footbridge_pci_bus_dma_tag = {
	._ranges = footbridge_dma_ranges,
	._nranges = 1,
	_BUS_DMAMAP_FUNCS,
	_BUS_DMAMEM_FUNCS,
	_BUS_DMATAG_FUNCS,
};

/*
 * Currently we only support 12 devices as we select directly in the
 * type 0 config cycle
 * (See conf_{read,write} for more detail
 */
#define MAX_PCI_DEVICES	21

/*static int
pci_intr(void *arg)
{
	printf("pci int %x\n", (int)arg);
	return(0);
}*/


void
footbridge_pci_attach_hook(device_t parent, device_t self, struct pcibus_attach_args *pba)
{
#ifdef PCI_DEBUG
	printf("footbridge_pci_attach_hook()\n");
#endif

/*	intr_claim(18, IPL_NONE, "pci int 0", pci_intr, (void *)0x10000);
	intr_claim(8, IPL_NONE, "pci int 1", pci_intr, (void *)0x10001);
	intr_claim(9, IPL_NONE, "pci int 2", pci_intr, (void *)0x10002);
	intr_claim(11, IPL_NONE, "pci int 3", pci_intr, (void *)0x10003);*/
}

int
footbridge_pci_bus_maxdevs(void *pcv, int busno)
{
#ifdef PCI_DEBUG
	printf("footbridge_pci_bus_maxdevs(pcv=%p, busno=%d)\n", pcv, busno);
#endif
	return(MAX_PCI_DEVICES);
}

pcitag_t
footbridge_pci_make_tag(void *pcv, int bus, int device, int function)
{
#ifdef PCI_DEBUG
	printf("footbridge_pci_make_tag(pcv=%p, bus=%d, device=%d, function=%d)\n",
	    pcv, bus, device, function);
#endif
	return ((bus << 16) | (device << 11) | (function << 8));
}

void
footbridge_pci_decompose_tag(void *pcv, pcitag_t tag, int *busp, int *devicep, int *functionp)
{
#ifdef PCI_DEBUG
	printf("footbridge_pci_decompose_tag(pcv=%p, tag=0x%08x, bp=%p, dp=%p, fp=%p)\n",
	    pcv, (uint32_t)tag, busp, devicep, functionp);
#endif

	if (busp != NULL)
		*busp = (tag >> 16) & 0xff;
	if (devicep != NULL)
		*devicep = (tag >> 11) & 0x1f;
	if (functionp != NULL)
		*functionp = (tag >> 8) & 0x7;
}

pcireg_t
footbridge_pci_conf_read(void *pcv, pcitag_t tag, int reg)
{
	int bus, device, function;
	u_int address;
	pcireg_t data;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return ((pcireg_t) -1);

	footbridge_pci_decompose_tag(pcv, tag, &bus, &device, &function);
	if (bus == 0)
		/* Limited to 12 devices or we exceed type 0 config space */
		address = DC21285_PCI_TYPE_0_CONFIG_VBASE | (3 << 22) | (device << 11);
	else
		address = DC21285_PCI_TYPE_1_CONFIG_VBASE | (device << 11) |
		    (bus << 16);

	address |= (function << 8) | reg;

	data = *((unsigned int *)address);
#ifdef PCI_DEBUG
	printf("footbridge_pci_conf_read(pcv=%p tag=0x%08x reg=0x%02x)=0x%08x\n",
	    pcv, (uint32_t)tag, reg, data);
#endif
	return(data);
}

void
footbridge_pci_conf_write(void *pcv, pcitag_t tag, int reg, pcireg_t data)
{
	int bus, device, function;
	u_int address;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	footbridge_pci_decompose_tag(pcv, tag, &bus, &device, &function);
	if (bus == 0)
		address = DC21285_PCI_TYPE_0_CONFIG_VBASE | (3 << 22) | (device << 11);
	else
		address = DC21285_PCI_TYPE_1_CONFIG_VBASE | (device << 11) |
		    (bus << 16);

	address |= (function << 8) | reg;

#ifdef PCI_DEBUG
	printf("footbridge_pci_conf_write(pcv=%p tag=0x%08x reg=0x%02x, 0x%08x)\n",
	    pcv, (uint32_t)tag, reg, data);
#endif

	*((unsigned int *)address) = data;
}

int
footbridge_pci_intr_map(const struct pci_attach_args *pa,
    pci_intr_handle_t *ihp)
{
	int pin = pa->pa_intrpin, line = pa->pa_intrline;
	int intr = -1;

#ifdef PCI_DEBUG
	void *pcv = pa->pa_pc;
	pcitag_t intrtag = pa->pa_intrtag;
	int bus, device, function;

	footbridge_pci_decompose_tag(pcv, intrtag, &bus, &device, &function);
	printf("footbridge_pci_intr_map: pcv=%p, tag=%08x pin=%d line=%d dev=%d\n",
	    pcv, (uint32_t)intrtag, pin, line, device);
#endif

	/*
	 * Only the line is used to map the interrupt.
	 * The firmware is expected to setup up the interrupt
	 * line as seen from the CPU
	 * This means the firmware deals with the interrupt rotation
	 * between slots etc.
	 *
	 * Perhaps the firmware should also to the final mapping
	 * to a 21285 interrupt bit so the code below would be
	 * completely MI.
	 */

	switch (line) {
	case PCI_INTERRUPT_PIN_NONE:
	case 0xff:
		/* No IRQ */
		printf("pci_intr_map: no mapping for pin %c\n", '@' + pin);
		*ihp = -1;
		return(1);
		break;
#ifdef cats
	/* This is machine dependent and needs to be moved */
	case PCI_INTERRUPT_PIN_A:
		intr = IRQ_PCI;
		break;
	case PCI_INTERRUPT_PIN_B:
		intr = IRQ_IN_L0;
		break;
	case PCI_INTERRUPT_PIN_C:
		intr = IRQ_IN_L1;
		break;
	case PCI_INTERRUPT_PIN_D:
		intr = IRQ_IN_L3;
		break;
#endif
	default:
		/*
		 * Experimental firmware feature ...
		 *
		 * If the interrupt line is in the range 0x80 to 0x8F
		 * then the lower 4 bits indicate the ISA interrupt
		 * bit that should be used.
		 * If the interrupt line is in the range 0x40 to 0x5F
		 * then the lower 5 bits indicate the actual DC21285
		 * interrupt bit that should be used.
		 */

		if (line >= 0x40 && line <= 0x5f)
			intr = line & 0x1f;
		else if (line >= 0x80 && line <= 0x8f)
			intr = line;
		else {
	                printf("footbridge_pci_intr_map: out of range interrupt"
			       "pin %d line %d (%#x)\n", pin, line, line);
			*ihp = -1;
			return(1);
		}
		break;
	}

#ifdef PCI_DEBUG
	printf("pin %d, line %d mapped to int %d\n", pin, line, intr);
#endif

	*ihp = intr;
	return(0);
}

const char *
footbridge_pci_intr_string(void *pcv, pci_intr_handle_t ih, char *buf, size_t len)
{
#ifdef PCI_DEBUG
	printf("footbridge_pci_intr_string(pcv=%p, ih=0x%lx)\n", pcv, ih);
#endif
	if (ih == 0)
		panic("footbridge_pci_intr_string: bogus handle 0x%lx", ih);

#if NISA > 0
	if (ih >= 0x80 && ih <= 0x8f) {
		snprintf(buf, len, "isairq %ld", (ih & 0x0f));
		return buf;
	}
#endif
	snprintf(buf, len, "irq %ld", ih);
	return buf;	
}

void *
footbridge_pci_intr_establish(
	void *pcv,
	pci_intr_handle_t ih,
	int level,
	int (*func)(void *),
	void *arg)
{
	void *intr;
	char buf[PCI_INTRSTR_LEN];
	const char *intrstr;

#ifdef PCI_DEBUG
	printf("footbridge_pci_intr_establish(pcv=%p, ih=0x%lx, level=%d, func=%p, arg=%p)\n",
	    pcv, ih, level, func, arg);
#endif

	/* Copy the interrupt string to a private buffer */
	intrstr = footbridge_pci_intr_string(pcv, ih, buf, sizeof(buf));
#if NISA > 0
	/*
	 * XXX the IDE driver will attach the interrupts in compat mode and
	 * thus we need to fail this here.
	 * This assumes that the interrupts are 14 and 15 which they are for
	 * IDE compat mode.
	 * Really the firmware should make this clear in the interrupt reg.
	 */
	if (ih >= 0x80 && ih <= 0x8d) {
		intr = isa_intr_establish(NULL, (ih & 0x0f), IST_EDGE,
		    level, func, arg);
	} else
#endif
	intr = footbridge_intr_claim(ih, level, intrstr, func, arg);

	return(intr);
}

void
footbridge_pci_intr_disestablish(void *pcv, void *cookie)
{
#ifdef PCI_DEBUG
	printf("footbridge_pci_intr_disestablish(pcv=%p, cookie=0x%p)\n",
	    pcv, cookie);
#endif
	/* XXXX Need to free the string */
	footbridge_intr_disestablish(cookie);
}
