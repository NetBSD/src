/* $NetBSD: admpci.c,v 1.1.62.2 2010/01/20 09:04:34 matt Exp $ */

/*-
 * Copyright (c) 2007 David Young.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#include "opt_pci.h"
#include "pci.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: admpci.c,v 1.1.62.2 2010/01/20 09:04:34 matt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/pte.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pciconf.h>

#ifdef	PCI_NETBSD_CONFIGURE
#include <mips/cache.h>
#endif

#include <mips/adm5120/include/adm5120_mainbusvar.h>
#include <mips/adm5120/include/adm5120reg.h>
#include <mips/adm5120/include/adm5120var.h>

#ifdef ADMPCI_DEBUG
int admpci_debug = 1;
#define	ADMPCI_DPRINTF(__fmt, ...)		\
do {						\
	if (admpci_debug)			\
		printf((__fmt), __VA_ARGS__);	\
} while (/*CONSTCOND*/0)
#else /* !ADMPCI_DEBUG */
#define	ADMPCI_DPRINTF(__fmt, ...)	do { } while (/*CONSTCOND*/0)
#endif /* ADMPCI_DEBUG */

#define	ADMPCI_TAG_BUS_MASK		__BITS(23, 16)
/* Bit 11 is reserved.  It selects the AHB-PCI bridge.  Let device 0
 * be the bridge.  For all other device numbers, let bit[11] == 0.
 */
#define	ADMPCI_TAG_DEVICE_MASK		__BITS(15, 11)
#define	ADMPCI_TAG_DEVICE_SUBMASK	__BITS(15, 12)
#define	ADMPCI_TAG_DEVICE_BRIDGE	__BIT(11)
#define	ADMPCI_TAG_FUNCTION_MASK	__BITS(10, 8)
#define	ADMPCI_TAG_REGISTER_MASK	__BITS(7, 0)

#define	ADMPCI_MAX_DEVICE

struct admpci_softc {
	struct device			sc_dev;
	struct mips_pci_chipset		sc_pc;

	bus_space_tag_t			sc_memt;
	bus_space_tag_t			sc_iot;

	bus_space_tag_t			sc_conft;
	bus_space_handle_t		sc_addrh;
	bus_space_handle_t		sc_datah;
};

int		admpcimatch(struct device *, struct cfdata *, void *);
void		admpciattach(struct device *, struct device *, void *);

#if NPCI > 0
static void admpci_attach_hook(struct device *, struct device *,
    struct pcibus_attach_args *);
static int admpci_bus_maxdevs(void *, int);
static pcitag_t admpci_make_tag(void *, int, int, int);
static void admpci_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t admpci_conf_read(void *, pcitag_t, int);
static void admpci_conf_write(void *, pcitag_t, int, pcireg_t);
static const char *admpci_intr_string(void *, pci_intr_handle_t);
static void admpci_conf_interrupt(void *, int, int, int, int, int *);
static void *admpci_intr_establish(void *, pci_intr_handle_t, int,
    int (*)(void *), void *);
static void admpci_intr_disestablish(void *, void *);
static int admpci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);

#ifdef	PCI_NETBSD_CONFIGURE
static struct extent	*io_ex = NULL;
static struct extent	*mem_ex = NULL;
#endif	/* PCI_NETBSD_CONFIGURE */

#endif	/* NPCI > 0 */

CFATTACH_DECL(admpci, sizeof(struct admpci_softc),
    admpcimatch, admpciattach, NULL, NULL);

int admpci_found = 0;

/*
 * Physical PCI addresses are 36-bits long, so we need to have
 * adequate storage space for them.
 */
#if NPCI > 0
#if !defined(_MIPS_PADDR_T_64BIT) && !defined(_LP64)
#error	"admpci requires 64 bit paddr_t!"
#endif
#endif

int
admpcimatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct mainbus_attach_args *ma = (struct mainbus_attach_args *)aux;

	return !admpci_found && strcmp(ma->ma_name, "admpci") == 0;
}

void
admpciattach(struct device *parent, struct device *self, void *aux)
{
	struct adm5120_config		*admc = &adm5120_configuration;
	struct admpci_softc		*sc = (struct admpci_softc *)self;
	struct mainbus_attach_args	*ma = (struct mainbus_attach_args *)aux;
#if NPCI > 0
	u_long				result;
	pcitag_t			tag;
	struct pcibus_attach_args	pba;
#endif
	
	admpci_found = 1;

	sc->sc_conft = ma->ma_obiot;
	if (bus_space_map(sc->sc_conft, ADM5120_BASE_PCI_CONFDATA, 4, 0,
		&sc->sc_datah) != 0) {
		printf(
		    "\n%s: unable to map PCI Configuration Data register\n",
		    device_xname(&sc->sc_dev));
		return;
	}
	if (bus_space_map(sc->sc_conft, ADM5120_BASE_PCI_CONFADDR, 4, 0,
		&sc->sc_addrh) != 0) {
		printf(
		    "\n%s: unable to map PCI Configuration Address register\n",
		    device_xname(&sc->sc_dev));
		return;
	}

	printf(": ADM5120 Host-PCI Bridge, data %"PRIxBSH" addr %"PRIxBSH", sc %p\n",
	    sc->sc_datah, sc->sc_addrh, (void *)sc);

#if NPCI > 0
	sc->sc_memt = &admc->pcimem_space;
	sc->sc_iot = &admc->pciio_space;

	sc->sc_pc.pc_conf_v = sc;
	sc->sc_pc.pc_attach_hook = admpci_attach_hook;
	sc->sc_pc.pc_bus_maxdevs = admpci_bus_maxdevs;
	sc->sc_pc.pc_make_tag = admpci_make_tag;
	sc->sc_pc.pc_decompose_tag = admpci_decompose_tag;
	sc->sc_pc.pc_conf_read = admpci_conf_read;
	sc->sc_pc.pc_conf_write = admpci_conf_write;

	sc->sc_pc.pc_intr_v = sc;
	sc->sc_pc.pc_intr_map = admpci_intr_map;
	sc->sc_pc.pc_intr_string = admpci_intr_string;
	sc->sc_pc.pc_intr_establish = admpci_intr_establish;
	sc->sc_pc.pc_intr_disestablish = admpci_intr_disestablish;
	sc->sc_pc.pc_conf_interrupt = admpci_conf_interrupt;

	tag = pci_make_tag(&sc->sc_pc, 0, 0, 0);
	ADMPCI_DPRINTF("%s: BAR 0x10 0x%08x\n", __func__,
	    pci_conf_read(&sc->sc_pc, tag, PCI_MAPREG_START));

#ifdef PCI_NETBSD_CONFIGURE
	mem_ex = extent_create("pcimem",
	    ADM5120_BOTTOM, ADM5120_TOP,
	    M_DEVBUF, NULL, 0, EX_WAITOK);
	(void)extent_alloc_subregion(mem_ex,
	    ADM5120_BASE_SRAM1, ADM5120_BASE_PCI_MEM - 1,
	    ADM5120_BASE_PCI_MEM - ADM5120_BASE_SRAM1,
	    ADM5120_BASE_PCI_MEM - ADM5120_BASE_SRAM1,
	    0, EX_WAITOK, &result);
	(void)extent_alloc_subregion(mem_ex,
	    ADM5120_BASE_PCI_IO, ADM5120_TOP,
	    ADM5120_TOP - ADM5120_BASE_PCI_IO + 1,
	    ADM5120_TOP - ADM5120_BASE_PCI_IO + 1,
	    0, EX_WAITOK, &result);

	io_ex = extent_create("pciio",
	    ADM5120_BASE_PCI_IO, ADM5120_BASE_PCI_CONFADDR - 1,
	    M_DEVBUF, NULL, 0, EX_WAITOK);

	pci_configure_bus(&sc->sc_pc,
	    io_ex, mem_ex, NULL, 0, mips_cache_info.mci_dcache_align);
	extent_destroy(mem_ex);
	extent_destroy(io_ex);
#endif

	pba.pba_iot = sc->sc_iot;
	pba.pba_memt = sc->sc_memt;
	/* XXX: review dma tag logic */
	pba.pba_dmat = ma->ma_dmat;
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &sc->sc_pc;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
#endif	/* NPCI > 0 */
}

#if NPCI > 0

void
admpci_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{
}

/* There are at most four devices on bus 0.  The ADM5120 has
 * request/grant lines for 3 PCI devices: 1, 2, and 3.  The host
 * bridge is device 0.
 */
int
admpci_bus_maxdevs(void *v, int bus)
{
	if (bus == 0)
		return 4;

	return 1 + __SHIFTOUT_MASK(ADMPCI_TAG_DEVICE_MASK);
}

pcitag_t
admpci_make_tag(void *v, int bus, int device, int function)
{
	if (bus > __SHIFTOUT_MASK(ADMPCI_TAG_BUS_MASK) ||
	    device > __SHIFTOUT_MASK(ADMPCI_TAG_DEVICE_MASK) ||
	    function > __SHIFTOUT_MASK(ADMPCI_TAG_FUNCTION_MASK))
		panic("%s: bad request", __func__);

	return __SHIFTIN(bus, ADMPCI_TAG_BUS_MASK) |
	       __SHIFTIN(device, ADMPCI_TAG_DEVICE_MASK) |
	       __SHIFTIN(function, ADMPCI_TAG_FUNCTION_MASK);
}

void
admpci_decompose_tag(void *v, pcitag_t tag, int *b, int *d, int *f)
{
	int bus, device, function;

	bus = __SHIFTOUT(tag, ADMPCI_TAG_BUS_MASK);
	device = __SHIFTOUT(tag, ADMPCI_TAG_DEVICE_MASK);
	function = __SHIFTOUT(tag, ADMPCI_TAG_FUNCTION_MASK);

	if (b != NULL)
		*b = bus;
	if (d != NULL)
		*d = device;
	if (f != NULL)
		*f = function;
}

static int
admpci_tag_to_addr(void *v, pcitag_t tag, int reg, bus_addr_t *addrp)
{
	int bus, device, function;

	KASSERT(addrp != NULL);
	/* panics if tag is not well-formed */
	admpci_decompose_tag(v, tag, &bus, &device, &function);
	if (reg > __SHIFTOUT_MASK(ADMPCI_TAG_REGISTER_MASK))
		panic("%s: bad register", __func__);

	*addrp = 0x80000000 | tag | __SHIFTIN(reg, ADMPCI_TAG_REGISTER_MASK);

	return 0;
}

static pcireg_t
admpci_conf_read(void *v, pcitag_t tag, int reg)
{
	int s;
	struct admpci_softc *sc = (struct admpci_softc *)v;
	uint32_t data;
	bus_addr_t addr;

	ADMPCI_DPRINTF("%s: sc %p tag %lx reg %d\n", __func__, (void *)sc, tag,
	    reg);

	if (admpci_tag_to_addr(v, tag, reg, &addr) == -1)
		return 0xffffffff;

	ADMPCI_DPRINTF("%s: sc_addrh %lx sc_datah %lx addr %lx\n", __func__,
	    sc->sc_addrh, sc->sc_datah, addr);

	s = splhigh();
	bus_space_write_4(sc->sc_conft, sc->sc_addrh, 0, addr);
	data = bus_space_read_4(sc->sc_conft, sc->sc_datah, 0);
	splx(s);

	ADMPCI_DPRINTF("%s: read 0x%" PRIx32 "\n", __func__, data);
	return data;
}

void
admpci_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{
	int s;
	struct admpci_softc	*sc = (struct admpci_softc *)v;
	bus_addr_t addr;

	ADMPCI_DPRINTF("%s: sc %p tag %lx reg %d\n", __func__, (void *)sc, tag,
	    reg);

	if (admpci_tag_to_addr(v, tag, reg, &addr) == -1)
		return;

	ADMPCI_DPRINTF("%s: sc_addrh %lx sc_datah %lx addr %lx\n", __func__,
	    sc->sc_addrh, sc->sc_datah, addr);

	s = splhigh();
	bus_space_write_4(sc->sc_conft, sc->sc_addrh, 0, addr);
	bus_space_write_4(sc->sc_conft, sc->sc_datah, 0, data);
	splx(s);
}

const char *
admpci_intr_string(void *v, pci_intr_handle_t ih)
{
	static char name[16];

	(void)snprintf(name, sizeof(name), "irq %u", (unsigned)ih);
	return name;
}

void *
admpci_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*handler)(void *), void *arg)
{
	return adm5120_intr_establish(ih, ipl, handler, arg);
}

void
admpci_intr_disestablish(void *v, void *cookie)
{
	adm5120_intr_disestablish(cookie);
}

void
admpci_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz, int *iline)
{
	/*
	 * We let the machdep_pci_intr_map take care of IRQ routing.
	 * On some platforms the BIOS may have handled this properly,
	 * on others it might not have.  For now we avoid clobbering
	 * the settings establishsed by the BIOS, so that they will be
	 * there if the platform logic is confident that it can rely
	 * on them.
	 */
}

/*
 * Map the bus 0 device numbers 1, 2, and 3 to IRQ 6, 7, and 8,
 * respectively.
 * 
 * XXX How to handle bridges?
 */
static int
admpci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int bus, device, function;

	admpci_decompose_tag(pa->pa_pc->pc_conf_v, pa->pa_tag,
	    &bus, &device, &function);

	if (bus != 0 || device > 3)
		return -1;

	*ihp = (device - 1) + 6;

	return 0;
}
#endif
