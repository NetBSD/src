/*	$NetBSD: ifpga_pci.c,v 1.19 2015/10/02 05:22:50 msaitoh Exp $	*/

/*
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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
 *
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

#define _ARM32_BUS_DMA_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ifpga_pci.c,v 1.19 2015/10/02 05:22:50 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <evbarm/integrator/int_bus_dma.h>

#include <machine/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <evbarm/ifpga/ifpgareg.h>
#include <evbarm/ifpga/ifpgamem.h>
#include <evbarm/ifpga/ifpga_pcivar.h>
#include <evbarm/dev/v360reg.h>


void		ifpga_pci_attach_hook (device_t, device_t,
		    struct pcibus_attach_args *);
int		ifpga_pci_bus_maxdevs (void *, int);
pcitag_t	ifpga_pci_make_tag (void *, int, int, int);
void		ifpga_pci_decompose_tag (void *, pcitag_t, int *, int *,
		    int *);
pcireg_t	ifpga_pci_conf_read (void *, pcitag_t, int);
void		ifpga_pci_conf_write (void *, pcitag_t, int, pcireg_t);
int		ifpga_pci_intr_map (const struct pci_attach_args *,
		    pci_intr_handle_t *);
const char	*ifpga_pci_intr_string (void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *ifpga_pci_intr_evcnt (void *, pci_intr_handle_t);
void		*ifpga_pci_intr_establish (void *, pci_intr_handle_t, int,
		    int (*)(void *), void *);
void		ifpga_pci_intr_disestablish (void *, void *);

struct arm32_pci_chipset ifpga_pci_chipset = {
	NULL,	/* conf_v */
	ifpga_pci_attach_hook,
	ifpga_pci_bus_maxdevs,
	ifpga_pci_make_tag,
	ifpga_pci_decompose_tag,
	ifpga_pci_conf_read,
	ifpga_pci_conf_write,
	NULL,	/* intr_v */
	ifpga_pci_intr_map,
	ifpga_pci_intr_string,
	ifpga_pci_intr_evcnt,
	ifpga_pci_intr_establish,
	ifpga_pci_intr_disestablish,
#ifdef __HAVE_PCI_CONF_HOOK
	NULL,
#endif
	ifpga_pci_conf_interrupt,
};

/*
 * Use the integrator-specific bus_dma routines.
 */
struct arm32_bus_dma_tag ifpga_pci_bus_dma_tag = {
	0,
	0,
	NULL,
	_bus_dmamap_create, 
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,	/* pre */
	NULL,			/* post */
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
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
	return 0;
}*/


void
ifpga_pci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
#ifdef PCI_DEBUG
	printf("ifpga_pci_attach_hook()\n");
#endif
}

int
ifpga_pci_bus_maxdevs(void *pcv, int busno)
{
#ifdef PCI_DEBUG
	printf("ifpga_pci_bus_maxdevs(pcv=%p, busno=%d)\n", pcv, busno);
#endif
	return MAX_PCI_DEVICES;
}

pcitag_t
ifpga_pci_make_tag(void *pcv, int bus, int device, int function)
{
#ifdef PCI_DEBUG
	printf("ifpga_pci_make_tag(pcv=%p, bus=%d, device=%d, function=%d)\n",
	    pcv, bus, device, function);
#endif
	return (bus << 16) | (device << 11) | (function << 8);
}

void
ifpga_pci_decompose_tag(void *pcv, pcitag_t tag, int *busp, int *devicep,
    int *functionp)
{
#ifdef PCI_DEBUG
	printf("ifpga_pci_decompose_tag(pcv=%p, tag=0x%08lx, bp=%p, dp=%p, "
	    "fp=%p)\n", pcv, tag, busp, devicep, functionp);
#endif

	if (busp != NULL)
		*busp = (tag >> 16) & 0xff;
	if (devicep != NULL)
		*devicep = (tag >> 11) & 0x1f;
	if (functionp != NULL)
		*functionp = (tag >> 8) & 0x7;
}

pcireg_t
ifpga_pci_conf_read(void *pcv, pcitag_t tag, int reg)
{
	pcireg_t data;
	struct ifpga_pci_softc *sc = (struct ifpga_pci_softc *)pcv;
	int bus, device, function;
	u_int address;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	ifpga_pci_decompose_tag(pcv, tag, &bus, &device, &function);

	/* Reset the appertures so that we can talk to the register space.  */
	bus_space_write_4(sc->sc_memt, sc->sc_reg_ioh, V360_LB_BASE0,
	    IFPGA_PCI_APP0_512MB_BASE);
	bus_space_write_4(sc->sc_memt, sc->sc_reg_ioh, V360_LB_BASE1,
	    IFPGA_PCI_APP1_CONF_BASE);

	if (bus == 0) {
		address = (1 << (device + 11)) | reg;
		bus_space_write_2(sc->sc_memt, sc->sc_reg_ioh, V360_LB_MAP1,
		    IFPGA_PCI_APP1_CONF_T0_MAP | ((address >> 16) & 0xff00));

		/* Read the value from the bus...  */
		data = bus_space_read_4(sc->sc_iot, sc->sc_conf_ioh,
		    address & 0x00ffffff);

	} else {
		bus_space_write_2(sc->sc_memt, sc->sc_reg_ioh, V360_LB_MAP1,
		    IFPGA_PCI_APP1_CONF_T1_MAP);

		/* Read the value from the bus... */
		data = bus_space_read_4(sc->sc_iot, sc->sc_conf_ioh,
		    tag | reg);
	}
	/* ... and put the memory spaces back again.  */

	bus_space_write_4(sc->sc_memt, sc->sc_reg_ioh, V360_LB_BASE1,
	    IFPGA_PCI_APP1_256MB_BASE);
	bus_space_write_2(sc->sc_memt, sc->sc_reg_ioh, V360_LB_MAP1,
	    IFPGA_PCI_APP1_256MB_MAP);
	bus_space_write_4(sc->sc_memt, sc->sc_reg_ioh, V360_LB_BASE0,
	    IFPGA_PCI_APP0_256MB_BASE);
#ifdef PCI_DEBUG
	printf("ifpga_pci_conf_read(pcv=%p tag=0x%08lx reg=0x%02x)=0x%08x\n",
	    pcv, tag, reg, data);
#endif
	return data;
}

void
ifpga_pci_conf_write(void *pcv, pcitag_t tag, int reg, pcireg_t data)
{
	struct ifpga_pci_softc *sc = (struct ifpga_pci_softc *)pcv;
	int bus, device, function;
	u_int address;

#ifdef PCI_DEBUG
	printf("ifpga_pci_conf_write(pcv=%p tag=0x%08lx reg=0x%02x, 0x%08x)\n",
	    pcv, tag, reg, data);
#endif

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	ifpga_pci_decompose_tag(pcv, tag, &bus, &device, &function);

	/* Reset the appertures so that we can talk to the register space.  */
	bus_space_write_4(sc->sc_memt, sc->sc_reg_ioh, V360_LB_BASE0,
	    IFPGA_PCI_APP0_512MB_BASE);
	bus_space_write_4(sc->sc_memt, sc->sc_reg_ioh, V360_LB_BASE1,
	    IFPGA_PCI_APP1_CONF_BASE);

	if (bus == 0) {
		address = (1 << (device + 11)) | reg;
		bus_space_write_2(sc->sc_memt, sc->sc_reg_ioh, V360_LB_MAP1,
		    IFPGA_PCI_APP1_CONF_T0_MAP | ((address >> 16) & 0xff00));

		/* Write the value to the bus...  */
		bus_space_write_4(sc->sc_iot, sc->sc_conf_ioh,
		    address & 0x00ffffff, data);

	} else {
		bus_space_write_2(sc->sc_memt, sc->sc_reg_ioh, V360_LB_MAP1,
		    IFPGA_PCI_APP1_CONF_T1_MAP);

		/* Write the value to the bus... */
		bus_space_write_4(sc->sc_iot, sc->sc_conf_ioh, tag | reg,
		    data);
	}
	/* ... and put the memory spaces back again.  */

	bus_space_write_4(sc->sc_memt, sc->sc_reg_ioh, V360_LB_BASE1,
	    IFPGA_PCI_APP1_256MB_BASE);
	bus_space_write_2(sc->sc_memt, sc->sc_reg_ioh, V360_LB_MAP1,
	    IFPGA_PCI_APP1_256MB_MAP);
	bus_space_write_4(sc->sc_memt, sc->sc_reg_ioh, V360_LB_BASE0,
	    IFPGA_PCI_APP0_256MB_BASE);
}

int
ifpga_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int line = pa->pa_intrline;

#ifdef PCI_DEBUG
	int pin = pa->pa_intrpin;
	void *pcv = pa->pa_pc;
	pcitag_t intrtag = pa->pa_intrtag;
	int bus, device, function;

	ifpga_pci_decompose_tag(pcv, intrtag, &bus, &device, &function);
	printf("ifpga_pci_intr_map: pcv=%p, tag=%08lx pin=%d line=%d "
	    "dev=%d\n", pcv, intrtag, pin, line, device);
#endif


#ifdef PCI_DEBUG
	printf("pin %d, line %d mapped to int %d\n", pin, line, line);
#endif

	*ihp = line;
	return 0;
}

const char *
ifpga_pci_intr_string(void *pcv, pci_intr_handle_t ih, char *buf, size_t len)
{
#ifdef PCI_DEBUG
	printf("ifpga_pci_intr_string(pcv=%p, ih=0x%lx)\n", pcv, ih);
#endif
	if (ih == 0)
		panic("ifpga_pci_intr_string: bogus handle 0x%lx", ih);

	snprintf(buf, len, "pciint%ld", ih - IFPGA_INTRNUM_PCIINT0);
	return buf;	
}

const struct evcnt *
ifpga_pci_intr_evcnt(void *pcv, pci_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
ifpga_pci_intr_establish(void *pcv, pci_intr_handle_t ih, int level,
    int (*func) (void *), void *arg)
{
	void *intr;

#ifdef PCI_DEBUG
	printf("ifpga_pci_intr_establish(pcv=%p, ih=0x%lx, level=%d, "
	    "func=%p, arg=%p)\n", pcv, ih, level, func, arg);
#endif

	intr = ifpga_intr_establish(ih, level, func, arg);

	return intr;
}

void
ifpga_pci_intr_disestablish(void *pcv, void *cookie)
{
#ifdef PCI_DEBUG
	printf("ifpga_pci_intr_disestablish(pcv=%p, cookie=%p)\n",
	    pcv, cookie);
#endif
	ifpga_intr_disestablish(cookie);
}
