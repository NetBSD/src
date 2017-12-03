/* $NetBSD: aupci.c,v 1.13.6.2 2017/12/03 11:36:26 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: aupci.c,v 1.13.6.2 2017/12/03 11:36:26 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <mips/locore.h>
#include <mips/pte.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pciconf.h>

#ifdef	PCI_NETBSD_CONFIGURE
#include <mips/cache.h>
#endif

#include <mips/alchemy/include/au_himem_space.h>
#include <mips/alchemy/include/aubusvar.h>
#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>

#include <mips/alchemy/dev/aupcireg.h>
#include <mips/alchemy/dev/aupcivar.h>

struct aupci_softc {
	device_t			sc_dev;
	struct mips_pci_chipset		sc_pc;
	struct mips_bus_space		sc_mem_space;
	struct mips_bus_space		sc_io_space;
	struct mips_bus_space		sc_cfg_space;

	bus_space_tag_t			sc_memt;
	bus_space_tag_t			sc_iot;
	bus_space_tag_t			sc_cfgt;

	bus_space_tag_t			sc_bust;

	bus_space_handle_t		sc_bush;
	paddr_t				sc_cfgbase;
	paddr_t				sc_membase;
	paddr_t				sc_iobase;

	/* XXX: dma tag */
};

int		aupcimatch(device_t, struct cfdata *, void *);
void		aupciattach(device_t, device_t, void *);

#if NPCI > 0
static void aupci_attach_hook(device_t, device_t, struct pcibus_attach_args *);
static int aupci_bus_maxdevs(void *, int);
static pcitag_t aupci_make_tag(void *, int, int, int);
static void aupci_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t aupci_conf_read(void *, pcitag_t, int);
static void aupci_conf_write(void *, pcitag_t, int, pcireg_t);
static const char *aupci_intr_string(void *, pci_intr_handle_t, char *, size_t);
static void aupci_conf_interrupt(void *, int, int, int, int, int *);
static void *aupci_intr_establish(void *, pci_intr_handle_t, int,
    int (*)(void *), void *);
static void aupci_intr_disestablish(void *, void *);

#ifdef	PCI_NETBSD_CONFIGURE
static struct extent	*io_ex = NULL;
static struct extent	*mem_ex = NULL;
#endif	/* PCI_NETBSD_CONFIGURE */

#define	PCI_CFG_READ	0
#define	PCI_CFG_WRITE	1

#endif	/* NPCI > 0 */

CFATTACH_DECL_NEW(aupci, sizeof(struct aupci_softc),
    aupcimatch, aupciattach, NULL, NULL);

int aupci_found = 0;

/*
 * Physical PCI addresses are 36-bits long, so we need to have
 * adequate storage space for them.
 */
#if NPCI > 0
#if !defined(_MIPS_PADDR_T_64BIT) && !defined(_LP64)
#error	"aupci requires 64 bit paddr_t!"
#endif
#endif

int
aupcimatch(device_t parent, struct cfdata *match, void *aux)
{
	struct aubus_attach_args *aa = (struct aubus_attach_args *)aux;

	if (strcmp(aa->aa_name, "aupci") != 0)
		return 0;

	if (aupci_found)
		return 0;

	return 1;
}

void
aupciattach(device_t parent, device_t self, void *aux)
{
	struct aupci_softc		*sc = device_private(self);
	struct aubus_attach_args	*aa = (struct aubus_attach_args *)aux;
	uint32_t			cfg;
#if NPCI > 0
	uint32_t			mbar, mask;
	bus_addr_t			mstart;
	struct pcibus_attach_args	pba;
#endif
	
	aupci_found = 1;

	sc->sc_dev = self;
	sc->sc_bust = aa->aa_st;
	if (bus_space_map(sc->sc_bust, aa->aa_addrs[0], 512, 0,
		&sc->sc_bush) != 0) {
		aprint_error(": unable to map PCI registers\n");
		return;
	}

#if NPCI > 0
	/*
	 * These physical addresses are locked in on the CPUs we have
	 * seen.  Perhaps these should be passed in via locators, thru
	 * the configuration file.
	 */
	sc->sc_cfgbase = PCI_CONFIG_BASE;
	sc->sc_membase = PCI_MEM_BASE;
	sc->sc_iobase = PCI_IO_BASE;
#endif

	/*
	 * Configure byte swapping, as YAMON doesn't do it.  YAMON does take
	 * care of most of the rest of the details (clocking, etc.), however.
	 */
#if _BYTE_ORDER == _BIG_ENDIAN
	/*
	 * N.B.: This still doesn't do the DMA thing properly.  I have
	 * not yet figured out how to get DMA access to work properly
	 * without having bytes swapped while the processor is in
	 * big-endian mode.  I'm not even sure that the Alchemy part
	 * can do it without swapping the bytes (which would be a
	 * bummer, since then only parts which had hardware detection
	 * and swapping support would work without special hacks in
	 * their drivers.)
	 */
	cfg = AUPCI_CONFIG_CH | AUPCI_CONFIG_R1H |
	    AUPCI_CONFIG_R2H | AUPCI_CONFIG_AEN | 
	    AUPCI_CONFIG_SM | AUPCI_CONFIG_ST | AUPCI_CONFIG_SIC_DATA;
#else
	cfg = AUPCI_CONFIG_CH | AUPCI_CONFIG_R1H |
	    AUPCI_CONFIG_R2H | AUPCI_CONFIG_AEN;
#endif
	bus_space_write_4(sc->sc_bust, sc->sc_bush, AUPCI_CONFIG, cfg);

	cfg = bus_space_read_4(sc->sc_bust, sc->sc_bush, AUPCI_COMMAND_STATUS);

	aprint_normal(": Alchemy Host-PCI Bridge, %sMHz\n",
	    (cfg & PCI_STATUS_66MHZ_SUPPORT) ? "66" : "33");
	aprint_naive("\n");

#if NPCI > 0
	/*
	 * PCI configuration space.  Address in this bus are
	 * orthogonal to other spaces.  We need to make the entire
	 * 32-bit address space available.
	 */
	sc->sc_cfgt = &sc->sc_cfg_space;
	au_himem_space_init(sc->sc_cfgt, "pcicfg", sc->sc_cfgbase,
	    0x00000000, 0xffffffff, AU_HIMEM_SPACE_IO);

	/*
	 * Virtual PCI memory.  Configured so that we don't overlap
	 * with PCI memory space.
	 */
	mask = bus_space_read_4(sc->sc_bust, sc->sc_bush, AUPCI_MWMASK);
	mask >>= AUPCI_MWMASK_SHIFT;
	mask <<= AUPCI_MWMASK_SHIFT;

	mbar = bus_space_read_4(sc->sc_bust, sc->sc_bush, AUPCI_MBAR);
	mstart = (mbar & mask) + (~mask + 1);

	sc->sc_memt = &sc->sc_mem_space;
	au_himem_space_init(sc->sc_memt, "pcimem", sc->sc_membase,
	    mstart, 0xffffffff, AU_HIMEM_SPACE_LITTLE_ENDIAN);

	/*
	 * IO space.  Address in this bus are orthogonal to other spaces.
	 * 16 MB should be plenty.  We don't start from zero to avoid
	 * potential device bugs.
	 */
	sc->sc_iot = &sc->sc_io_space;
	au_himem_space_init(sc->sc_iot, "pciio",
	    sc->sc_iobase, AUPCI_IO_START, AUPCI_IO_END,
	    AU_HIMEM_SPACE_LITTLE_ENDIAN | AU_HIMEM_SPACE_IO);

	sc->sc_pc.pc_conf_v = sc;
	sc->sc_pc.pc_attach_hook = aupci_attach_hook;
	sc->sc_pc.pc_bus_maxdevs = aupci_bus_maxdevs;
	sc->sc_pc.pc_make_tag = aupci_make_tag;
	sc->sc_pc.pc_decompose_tag = aupci_decompose_tag;
	sc->sc_pc.pc_conf_read = aupci_conf_read;
	sc->sc_pc.pc_conf_write = aupci_conf_write;

	sc->sc_pc.pc_intr_v = sc;
	sc->sc_pc.pc_intr_map = aupci_intr_map;
	sc->sc_pc.pc_intr_string = aupci_intr_string;
	sc->sc_pc.pc_intr_establish = aupci_intr_establish;
	sc->sc_pc.pc_intr_disestablish = aupci_intr_disestablish;
	sc->sc_pc.pc_conf_interrupt = aupci_conf_interrupt;

#ifdef PCI_NETBSD_CONFIGURE
	mem_ex = extent_create("pcimem", mstart, 0xffffffff,
	    NULL, 0, EX_WAITOK);

	io_ex = extent_create("pciio", AUPCI_IO_START, AUPCI_IO_END,
	    NULL, 0, EX_WAITOK);

	pci_configure_bus(&sc->sc_pc,
	    io_ex, mem_ex, NULL, 0, mips_cache_info.mci_dcache_align);
	extent_destroy(mem_ex);
	extent_destroy(io_ex);
#endif

	pba.pba_iot = sc->sc_iot;
	pba.pba_memt = sc->sc_memt;
	/* XXX: review dma tag logic */
	pba.pba_dmat = aa->aa_dt;
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &sc->sc_pc;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
#endif	/* NPCI > 0 */
}

#if NPCI > 0

void
aupci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

int
aupci_bus_maxdevs(void *v, int busno)
{

	return 32;
}

pcitag_t
aupci_make_tag(void *v, int bus, int device, int function)
{
	pcitag_t		tag;

	if (bus >= 256 || device >= 32 || function >= 8)
		panic("aupci_make_tag: bad request");

	tag = (bus << 16) | (device << 11) | (function << 8);

	return tag;
}

void
aupci_decompose_tag(void *v, pcitag_t tag, int *b, int *d, int *f)
{

	if (b != NULL)
		*b = (tag >> 16) & 0xff;
	if (d != NULL)
		*d = (tag >> 11) & 0x1f;
	if (f != NULL)
		*f = (tag >> 8) & 0x07;
}

static inline bool
aupci_conf_access(void *v, int dir, pcitag_t tag, int reg, pcireg_t *datap)
{
	struct aupci_softc	*sc = (struct aupci_softc *)v;
	uint32_t		status;
	int			s;
	bus_addr_t		addr;
	int			b, d, f;
	bus_space_handle_t	h;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return false;

	aupci_decompose_tag(v, tag, &b, &d, &f);
	if (b) {
		/* configuration type 1 */
		addr = 0x80000000 | tag;
	} else if (d > 19) {
		/* device num too big for bus 0 */
		return false;
	} else {
		addr = (0x800 << d) | (f << 8);
	}

	/* probing illegal target is OK, return an error indication */
	if (addr == 0)
		return false;

	if (bus_space_map(sc->sc_cfgt, addr, 256, 0, &h) != 0)
		return false;

	s = splhigh();

	if (dir == PCI_CFG_WRITE)
		bus_space_write_4(sc->sc_cfgt, h, reg, *datap);
	else
		*datap = bus_space_read_4(sc->sc_cfgt, h, reg);

	DELAY(2);

	/* check for and clear master abort condition */
	status = bus_space_read_4(sc->sc_bust, sc->sc_bush, AUPCI_CONFIG);
	bus_space_write_4(sc->sc_bust, sc->sc_bush, AUPCI_CONFIG,
	    status & ~(AUPCI_CONFIG_EF));

	splx(s);

	bus_space_unmap(sc->sc_cfgt, h, 256);
	
	/* if we got a PCI master abort, fail it */
	if (status & AUPCI_CONFIG_EF)
		return false;

	return true;
}

pcireg_t
aupci_conf_read(void *v, pcitag_t tag, int reg)
{
	pcireg_t		data;

	if (aupci_conf_access(v, PCI_CFG_READ, tag, reg, &data) == false)
		return 0xffffffff;

	return (data);
}

void
aupci_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{

	aupci_conf_access(v, PCI_CFG_WRITE, tag, reg, &data);
}

const char *
aupci_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
	snprintf(buf, len, "irq %u", (unsigned)ih);
	return buf;
}

void *
aupci_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*handler)(void *), void *arg)
{

	return (au_intr_establish(ih, 0, ipl, IST_LEVEL_LOW, handler, arg));
}

void
aupci_intr_disestablish(void *v, void *cookie)
{

	au_intr_disestablish(cookie);
}

void
aupci_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz, int *iline)
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

#endif
