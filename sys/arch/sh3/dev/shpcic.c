/*	$NetBSD: shpcic.c,v 1.15.2.1 2012/04/17 00:06:52 yamt Exp $	*/

/*-
 * Copyright (C) 2005 NONAKA Kimihiro <nonaka@netbsd.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: shpcic.c,v 1.15.2.1 2012/04/17 00:06:52 yamt Exp $");

#include "opt_pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <dev/pci/pcidevs.h>

#include <sh3/bscreg.h>
#include <sh3/cache.h>
#include <sh3/exception.h>
#include <sh3/pcicreg.h>

#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/pci_machdep.h>


#if defined(DEBUG) && !defined(SHPCIC_DEBUG)
#define SHPCIC_DEBUG 0
#endif
#if defined(SHPCIC_DEBUG)
int shpcic_debug = SHPCIC_DEBUG + 0;
#define	DPRINTF(arg)	if (shpcic_debug) printf arg
#else
#define	DPRINTF(arg)
#endif

#define	PCI_MODE1_ENABLE	0x80000000UL


static int	shpcic_match(device_t, cfdata_t, void *);
static void	shpcic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(shpcic, 0,
    shpcic_match, shpcic_attach, NULL, NULL);


/* There can be only one. */
static int shpcic_found = 0;

/* PCIC intr priotiry */
static int shpcic_intr_priority[2] = { IPL_BIO, IPL_BIO };


static int
shpcic_match(device_t parent, cfdata_t cf, void *aux)
{
	pcireg_t id;

	if (shpcic_found)
		return (0);

	switch (cpu_product) {
	case CPU_PRODUCT_7751:
	case CPU_PRODUCT_7751R:
		break;

	default:
		return (0);
	}


	id = _reg_read_4(SH4_PCICONF0);

	switch (PCI_VENDOR(id)) {
	case PCI_VENDOR_HITACHI:
		break;

	default:
		return (0);
	}


	switch (PCI_PRODUCT(id)) {
	case PCI_PRODUCT_HITACHI_SH7751: /* FALLTHROUGH */
	case PCI_PRODUCT_HITACHI_SH7751R:
		break;

	default:
		return (0);
	}

	if (_reg_read_2(SH4_BCR2) & BCR2_PORTEN)
		return (0);

	return (1);
}

static void
shpcic_attach(device_t parent, device_t self, void *aux)
{
	struct pcibus_attach_args pba;
#ifdef PCI_NETBSD_CONFIGURE
	struct extent *ioext, *memext;
#endif
	pcireg_t id, class;
	char devinfo[256];

	shpcic_found = 1;

	aprint_naive("\n");

	id = _reg_read_4(SH4_PCICONF0);
	class = _reg_read_4(SH4_PCICONF2);
	pci_devinfo(id, class, 1, devinfo, sizeof(devinfo));
	aprint_normal(": %s\n", devinfo);

	/* allow PCIC request */
	_reg_write_4(SH4_BCR1, _reg_read_4(SH4_BCR1) | BCR1_BREQEN);

	/* Initialize PCIC */
	_reg_write_4(SH4_PCICR, PCICR_BASE | PCICR_RSTCTL);
	delay(10 * 1000);
	_reg_write_4(SH4_PCICR, PCICR_BASE);

	/* Class: Host-Bridge */
	_reg_write_4(SH4_PCICONF2,
	    PCI_CLASS_CODE(PCI_CLASS_BRIDGE, PCI_SUBCLASS_BRIDGE_HOST, 0x00));

#if !defined(DONT_INIT_PCIBSC)
#if defined(PCIBCR_BCR1_VAL)
	_reg_write_4(SH4_PCIBCR1, PCIBCR_BCR1_VAL);
#else
	_reg_write_4(SH4_PCIBCR1, _reg_read_4(SH4_BCR1) | BCR1_MASTER);
#endif
#if defined(PCIBCR_BCR2_VAL)
	_reg_write_4(SH4_PCIBCR2, PCIBCR_BCR2_VAL);
#else
	_reg_write_4(SH4_PCIBCR2, _reg_read_2(SH4_BCR2));
#endif
#if defined(SH4) && defined(SH7751R)
	if (cpu_product == CPU_PRODUCT_7751R) {
#if defined(PCIBCR_BCR3_VAL)
		_reg_write_4(SH4_PCIBCR3, PCIBCR_BCR3_VAL);
#else
		_reg_write_4(SH4_PCIBCR3, _reg_read_2(SH4_BCR3));
#endif
	}
#endif	/* SH4 && SH7751R && PCIBCR_BCR3_VAL */
#if defined(PCIBCR_WCR1_VAL)
	_reg_write_4(SH4_PCIWCR1, PCIBCR_WCR1_VAL);
#else
	_reg_write_4(SH4_PCIWCR1, _reg_read_4(SH4_WCR1));
#endif
#if defined(PCIBCR_WCR2_VAL)
	_reg_write_4(SH4_PCIWCR2, PCIBCR_WCR2_VAL);
#else
	_reg_write_4(SH4_PCIWCR2, _reg_read_4(SH4_WCR2));
#endif
#if defined(PCIBCR_WCR3_VAL)
	_reg_write_4(SH4_PCIWCR3, PCIBCR_WCR3_VAL);
#else
	_reg_write_4(SH4_PCIWCR3, _reg_read_4(SH4_WCR3));
#endif
#if defined(PCIBCR_MCR_VAL)
	_reg_write_4(SH4_PCIMCR, PCIBCR_MCR_VAL);
#else
	_reg_write_4(SH4_PCIMCR, _reg_read_4(SH4_MCR));
#endif
#endif	/* !DONT_INIT_PCIBSC */

	/* set PCI I/O, memory base address */
	_reg_write_4(SH4_PCIIOBR, SH4_PCIC_IO);
	_reg_write_4(SH4_PCIMBR, SH4_PCIC_MEM);

	/* set PCI local address 0 */
	_reg_write_4(SH4_PCILSR0, (64 - 1) << 20);
	_reg_write_4(SH4_PCILAR0, 0xac000000);
	_reg_write_4(SH4_PCICONF5, 0xac000000);

	/* set PCI local address 1 */
	_reg_write_4(SH4_PCILSR1, (64 - 1) << 20);
	_reg_write_4(SH4_PCILAR1, 0xac000000);
	_reg_write_4(SH4_PCICONF6, 0x8c000000);

	/* Enable I/O, memory, bus-master */
	_reg_write_4(SH4_PCICONF1, PCI_COMMAND_IO_ENABLE
	                           | PCI_COMMAND_MEM_ENABLE
	                           | PCI_COMMAND_MASTER_ENABLE
	                           | PCI_COMMAND_STEPPING_ENABLE
				   | PCI_STATUS_DEVSEL_MEDIUM);

	/* Initialize done. */
	_reg_write_4(SH4_PCICR, PCICR_BASE | PCICR_CFINIT);

	/* set PCI controller interrupt priority */
	intpri_intr_priority(SH4_INTEVT_PCIERR, shpcic_intr_priority[0]);
	intpri_intr_priority(SH4_INTEVT_PCISERR, shpcic_intr_priority[1]);

	/* PCI bus */
#ifdef PCI_NETBSD_CONFIGURE
	ioext  = extent_create("pciio",
	    SH4_PCIC_IO, SH4_PCIC_IO + SH4_PCIC_IO_SIZE - 1,
	    NULL, 0, EX_NOWAIT);
	memext = extent_create("pcimem",
	    SH4_PCIC_MEM, SH4_PCIC_MEM + SH4_PCIC_MEM_SIZE - 1,
	    NULL, 0, EX_NOWAIT);

	pci_configure_bus(NULL, ioext, memext, NULL, 0, sh_cache_line_size);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif

	/* PCI bus */
	memset(&pba, 0, sizeof(pba));
	pba.pba_iot = shpcic_get_bus_io_tag();
	pba.pba_memt = shpcic_get_bus_mem_tag();
	pba.pba_dmat = shpcic_get_bus_dma_tag();
	pba.pba_dmat64 = NULL;
	pba.pba_pc = NULL;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
	config_found(self, &pba, NULL);
}

int
shpcic_bus_maxdevs(void *v, int busno)
{

	/*
	 * Bus number is irrelevant.  Configuration Mechanism 1 is in
	 * use, can have devices 0-32 (i.e. the `normal' range).
	 */
	return (32);
}

pcitag_t
shpcic_make_tag(void *v, int bus, int device, int function)
{
	pcitag_t tag;

	if (bus >= 256 || device >= 32 || function >= 8)
		panic("pci_make_tag: bad request");

	tag = PCI_MODE1_ENABLE |
		    (bus << 16) | (device << 11) | (function << 8);

	return (tag);
}

void
shpcic_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

pcireg_t
shpcic_conf_read(void *v, pcitag_t tag, int reg)
{
	pcireg_t data;
	int s;

	s = splhigh();
	_reg_write_4(SH4_PCIPAR, tag | reg);
	data = _reg_read_4(SH4_PCIPDR);
	_reg_write_4(SH4_PCIPAR, 0);
	splx(s);

	return data;
}

void
shpcic_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{
	int s;

	s = splhigh();
	_reg_write_4(SH4_PCIPAR, tag | reg);
	_reg_write_4(SH4_PCIPDR, data);
	_reg_write_4(SH4_PCIPAR, 0);
	splx(s);
}

int
shpcic_set_intr_priority(int intr, int level)
{
	int evtcode;

	if ((intr != 0) && (intr != 1)) {
		return (-1);
	}
	if ((level < IPL_NONE) || (level > IPL_HIGH)) {
		return (-1);
	}

	if (intr == 0) {
		evtcode = SH4_INTEVT_PCIERR;
	} else {
		evtcode = SH4_INTEVT_PCISERR;
	}

	intpri_intr_priority(evtcode, shpcic_intr_priority[intr]);
	shpcic_intr_priority[intr] = level;

	return (0);
}

void *
shpcic_intr_establish(int evtcode, int (*ih_func)(void *), void *ih_arg)
{
	int level;

	switch (evtcode) {
	case SH4_INTEVT_PCISERR:
		level = shpcic_intr_priority[1];
		break;

	case SH4_INTEVT_PCIDMA3:
	case SH4_INTEVT_PCIDMA2:
	case SH4_INTEVT_PCIDMA1:
	case SH4_INTEVT_PCIDMA0:
	case SH4_INTEVT_PCIPWON:
	case SH4_INTEVT_PCIPWDWN:
	case SH4_INTEVT_PCIERR:
		level = shpcic_intr_priority[0];
		break;

	default:
		printf("shpcic_intr_establish: unknown evtcode = 0x%08x\n",
		    evtcode);
		return NULL;
	}

	return intc_intr_establish(evtcode, IST_LEVEL, level, ih_func, ih_arg);
}

void
shpcic_intr_disestablish(void *ih)
{

	intc_intr_disestablish(ih);
}

/*
 * shpcic bus space
 */
int
shpcic_iomem_map(void *v, bus_addr_t bpa, bus_size_t size,
    int flags, bus_space_handle_t *bshp)
{

	*bshp = (bus_space_handle_t)bpa;

	return (0);
}

void
shpcic_iomem_unmap(void *v, bus_space_handle_t bsh, bus_size_t size)
{

	/* Nothing to do */
}

int
shpcic_iomem_subregion(void *v, bus_space_handle_t bsh,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nbshp)
{

	*nbshp = bsh + offset;

	return (0);
}

int
shpcic_iomem_alloc(void *v, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp)
{

	*bshp = *bpap = rstart;

	return (0);
}

void
shpcic_iomem_free(void *v, bus_space_handle_t bsh, bus_size_t size)
{

	/* Nothing to do */
}

paddr_t
shpcic_iomem_mmap(void *v, bus_addr_t addr, off_t off, int prot, int flags)
{

	return (paddr_t)-1;
}

/*
 * shpcic bus space io/mem read/write
 */
/* read */
static inline uint8_t __shpcic_io_read_1(bus_space_handle_t bsh,
    bus_size_t offset);
static inline uint16_t __shpcic_io_read_2(bus_space_handle_t bsh,
    bus_size_t offset);
static inline uint32_t __shpcic_io_read_4(bus_space_handle_t bsh,
    bus_size_t offset);
static inline uint8_t __shpcic_mem_read_1(bus_space_handle_t bsh,
    bus_size_t offset);
static inline uint16_t __shpcic_mem_read_2(bus_space_handle_t bsh,
    bus_size_t offset);
static inline uint32_t __shpcic_mem_read_4(bus_space_handle_t bsh,
    bus_size_t offset);

static inline uint8_t
__shpcic_io_read_1(bus_space_handle_t bsh, bus_size_t offset)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_IO_MASK;

	return *(volatile uint8_t *)(SH4_PCIC_IO + adr);
}

static inline uint16_t
__shpcic_io_read_2(bus_space_handle_t bsh, bus_size_t offset)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_IO_MASK;

	return *(volatile uint16_t *)(SH4_PCIC_IO + adr);
}

static inline uint32_t
__shpcic_io_read_4(bus_space_handle_t bsh, bus_size_t offset)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_IO_MASK;

	return *(volatile uint32_t *)(SH4_PCIC_IO + adr);
}

static inline uint8_t
__shpcic_mem_read_1(bus_space_handle_t bsh, bus_size_t offset)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_MEM_MASK;

	return *(volatile uint8_t *)(SH4_PCIC_MEM + adr);
}

static inline uint16_t
__shpcic_mem_read_2(bus_space_handle_t bsh, bus_size_t offset)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_MEM_MASK;

	return *(volatile uint16_t *)(SH4_PCIC_MEM + adr);
}

static inline uint32_t
__shpcic_mem_read_4(bus_space_handle_t bsh, bus_size_t offset)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_MEM_MASK;

	return *(volatile uint32_t *)(SH4_PCIC_MEM + adr);
}

/*
 * read single
 */
uint8_t
shpcic_io_read_1(void *v, bus_space_handle_t bsh, bus_size_t offset)
{
	uint8_t value;

	value = __shpcic_io_read_1(bsh, offset);

	return value;
}

uint16_t
shpcic_io_read_2(void *v, bus_space_handle_t bsh, bus_size_t offset)
{
	uint16_t value;

	value = __shpcic_io_read_2(bsh, offset);

	return value;
}

uint32_t
shpcic_io_read_4(void *v, bus_space_handle_t bsh, bus_size_t offset)
{
	uint32_t value;

	value = __shpcic_io_read_4(bsh, offset);

	return value;
}

uint8_t
shpcic_mem_read_1(void *v, bus_space_handle_t bsh, bus_size_t offset)
{
	uint8_t value;

	value = __shpcic_mem_read_1(bsh, offset);

	return value;
}

uint16_t
shpcic_mem_read_2(void *v, bus_space_handle_t bsh, bus_size_t offset)
{
	uint16_t value;

	value = __shpcic_mem_read_2(bsh, offset);

	return value;
}

uint32_t
shpcic_mem_read_4(void *v, bus_space_handle_t bsh, bus_size_t offset)
{
	uint32_t value;

	value = __shpcic_mem_read_4(bsh, offset);

	return value;
}

/*
 * read multi
 */
void
shpcic_io_read_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_io_read_1(bsh, offset);
	}
}

void
shpcic_io_read_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_io_read_2(bsh, offset);
	}
}

void
shpcic_io_read_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_io_read_4(bsh, offset);
	}
}

void
shpcic_mem_read_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_mem_read_1(bsh, offset);
	}
}

void
shpcic_mem_read_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_mem_read_2(bsh, offset);
	}
}

void
shpcic_mem_read_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_mem_read_4(bsh, offset);
	}
}

/*
 *
 * read region
 */
void
shpcic_io_read_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_io_read_1(bsh, offset);
		offset += 1;
	}
}

void
shpcic_io_read_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_io_read_2(bsh, offset);
		offset += 2;
	}
}

void
shpcic_io_read_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_io_read_4(bsh, offset);
		offset += 4;
	}
}

void
shpcic_mem_read_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_mem_read_1(bsh, offset);
		offset += 1;
	}
}

void
shpcic_mem_read_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_mem_read_2(bsh, offset);
		offset += 2;
	}
}

void
shpcic_mem_read_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count)
{

	while (count--) {
		*addr++ = __shpcic_mem_read_4(bsh, offset);
		offset += 4;
	}
}

/* write */
static inline void __shpcic_io_write_1(bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value);
static inline void __shpcic_io_write_2(bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value);
static inline void __shpcic_io_write_4(bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value);
static inline void __shpcic_mem_write_1(bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value);
static inline void __shpcic_mem_write_2(bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value);
static inline void __shpcic_mem_write_4(bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value);

static inline void
__shpcic_io_write_1(bus_space_handle_t bsh, bus_size_t offset,
    uint8_t value)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_IO_MASK;

	*(volatile uint8_t *)(SH4_PCIC_IO + adr) = value;
}

static inline void
__shpcic_io_write_2(bus_space_handle_t bsh, bus_size_t offset,
    uint16_t value)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_IO_MASK;

	*(volatile uint16_t *)(SH4_PCIC_IO + adr) = value;
}

static inline void
__shpcic_io_write_4(bus_space_handle_t bsh, bus_size_t offset,
    uint32_t value)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_IO_MASK;

	*(volatile uint32_t *)(SH4_PCIC_IO + adr) = value;
}

static inline void
__shpcic_mem_write_1(bus_space_handle_t bsh, bus_size_t offset,
    uint8_t value)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_MEM_MASK;

	*(volatile uint8_t *)(SH4_PCIC_MEM + adr) = value;
}

static inline void
__shpcic_mem_write_2(bus_space_handle_t bsh, bus_size_t offset,
    uint16_t value)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_MEM_MASK;

	*(volatile uint16_t *)(SH4_PCIC_MEM + adr) = value;
}

static inline void
__shpcic_mem_write_4(bus_space_handle_t bsh, bus_size_t offset,
    uint32_t value)
{
	u_long adr = (u_long)(bsh + offset) & SH4_PCIC_MEM_MASK;

	*(volatile uint32_t *)(SH4_PCIC_MEM + adr) = value;
}

/*
 * write single
 */
void
shpcic_io_write_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value)
{

	__shpcic_io_write_1(bsh, offset, value);
}

void
shpcic_io_write_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value)
{

	__shpcic_io_write_2(bsh, offset, value);
}

void
shpcic_io_write_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value)
{

	__shpcic_io_write_4(bsh, offset, value);
}

void
shpcic_mem_write_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value)
{

	__shpcic_mem_write_1(bsh, offset, value);
}

void
shpcic_mem_write_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value)
{

	__shpcic_mem_write_2(bsh, offset, value);
}

void
shpcic_mem_write_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value)
{

	__shpcic_mem_write_4(bsh, offset, value);
}

/*
 * write multi
 */
void
shpcic_io_write_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_1(bsh, offset, *addr++);
	}
}

void
shpcic_io_write_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_2(bsh, offset, *addr++);
	}
}

void
shpcic_io_write_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_4(bsh, offset, *addr++);
	}
}

void
shpcic_mem_write_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_1(bsh, offset, *addr++);
	}
}

void
shpcic_mem_write_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_2(bsh, offset, *addr++);
	}
}

void
shpcic_mem_write_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_4(bsh, offset, *addr++);
	}
}

/*
 * write region
 */
void
shpcic_io_write_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_1(bsh, offset, *addr++);
		offset += 1;
	}
}

void
shpcic_io_write_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_2(bsh, offset, *addr++);
		offset += 2;
	}
}

void
shpcic_io_write_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_4(bsh, offset, *addr++);
		offset += 4;
	}
}

void
shpcic_mem_write_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_1(bsh, offset, *addr++);
		offset += 1;
	}
}

void
shpcic_mem_write_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_2(bsh, offset, *addr++);
		offset += 2;
	}
}

void
shpcic_mem_write_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_4(bsh, offset, *addr++);
		offset += 4;
	}
}

/*
 * set multi
 */
void
shpcic_io_set_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_1(bsh, offset, value);
	}
}

void
shpcic_io_set_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_2(bsh, offset, value);
	}
}

void
shpcic_io_set_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_4(bsh, offset, value);
	}
}

void
shpcic_mem_set_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_1(bsh, offset, value);
	}
}

void
shpcic_mem_set_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_2(bsh, offset, value);
	}
}

void
shpcic_mem_set_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_4(bsh, offset, value);
	}
}

/*
 * set region
 */
void
shpcic_io_set_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_1(bsh, offset, value);
		offset += 1;
	}
}

void
shpcic_io_set_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_2(bsh, offset, value);
		offset += 2;
	}
}

void
shpcic_io_set_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_io_write_4(bsh, offset, value);
		offset += 4;
	}
}

void
shpcic_mem_set_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_1(bsh, offset, value);
		offset += 1;
	}
}

void
shpcic_mem_set_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_2(bsh, offset, value);
		offset += 2;
	}
}

void
shpcic_mem_set_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value, bus_size_t count)
{

	while (count--) {
		__shpcic_mem_write_4(bsh, offset, value);
		offset += 4;
	}
}

/*
 * copy region
 */
void
shpcic_io_copy_region_1(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2, bus_size_t count)
{
	u_long addr1 = bsh1 + off1;
	u_long addr2 = bsh2 + off2;
	uint8_t value;

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (count--) {
			value = __shpcic_io_read_1(bsh1, off1);
			__shpcic_io_write_1(bsh2, off2, value);
			off1 += 1;
			off2 += 1;
		}
	} else {		/* dest after src: copy backwards */
		off1 += (count - 1) * 1;
		off2 += (count - 1) * 1;
		while (count--) {
			value = __shpcic_io_read_1(bsh1, off1);
			__shpcic_io_write_1(bsh2, off2, value);
			off1 -= 1;
			off2 -= 1;
		}
	}
}

void
shpcic_io_copy_region_2(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2, bus_size_t count)
{
	u_long addr1 = bsh1 + off1;
	u_long addr2 = bsh2 + off2;
	uint16_t value;

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (count--) {
			value = __shpcic_io_read_2(bsh1, off1);
			__shpcic_io_write_2(bsh2, off2, value);
			off1 += 2;
			off2 += 2;
		}
	} else {		/* dest after src: copy backwards */
		off1 += (count - 1) * 2;
		off2 += (count - 1) * 2;
		while (count--) {
			value = __shpcic_io_read_2(bsh1, off1);
			__shpcic_io_write_2(bsh2, off2, value);
			off1 -= 2;
			off2 -= 2;
		}
	}
}

void
shpcic_io_copy_region_4(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2, bus_size_t count)
{
	u_long addr1 = bsh1 + off1;
	u_long addr2 = bsh2 + off2;
	uint32_t value;

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (count--) {
			value = __shpcic_io_read_4(bsh1, off1);
			__shpcic_io_write_4(bsh2, off2, value);
			off1 += 4;
			off2 += 4;
		}
	} else {		/* dest after src: copy backwards */
		off1 += (count - 1) * 4;
		off2 += (count - 1) * 4;
		while (count--) {
			value = __shpcic_io_read_4(bsh1, off1);
			__shpcic_io_write_4(bsh2, off2, value);
			off1 -= 4;
			off2 -= 4;
		}
	}
}

void
shpcic_mem_copy_region_1(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2, bus_size_t count)
{
	u_long addr1 = bsh1 + off1;
	u_long addr2 = bsh2 + off2;
	uint8_t value;

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (count--) {
			value = __shpcic_mem_read_1(bsh1, off1);
			__shpcic_mem_write_1(bsh2, off2, value);
			off1 += 1;
			off2 += 1;
		}
	} else {		/* dest after src: copy backwards */
		off1 += (count - 1) * 1;
		off2 += (count - 1) * 1;
		while (count--) {
			value = __shpcic_mem_read_1(bsh1, off1);
			__shpcic_mem_write_1(bsh2, off2, value);
			off1 -= 1;
			off2 -= 1;
		}
	}
}

void
shpcic_mem_copy_region_2(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2, bus_size_t count)
{
	u_long addr1 = bsh1 + off1;
	u_long addr2 = bsh2 + off2;
	uint16_t value;

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (count--) {
			value = __shpcic_mem_read_2(bsh1, off1);
			__shpcic_mem_write_2(bsh2, off2, value);
			off1 += 2;
			off2 += 2;
		}
	} else {		/* dest after src: copy backwards */
		off1 += (count - 1) * 2;
		off2 += (count - 1) * 2;
		while (count--) {
			value = __shpcic_mem_read_2(bsh1, off1);
			__shpcic_mem_write_2(bsh2, off2, value);
			off1 -= 2;
			off2 -= 2;
		}
	}
}

void
shpcic_mem_copy_region_4(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2, bus_size_t count)
{
	u_long addr1 = bsh1 + off1;
	u_long addr2 = bsh2 + off2;
	uint32_t value;

	if (addr1 >= addr2) {	/* src after dest: copy forward */
		while (count--) {
			value = __shpcic_mem_read_4(bsh1, off1);
			__shpcic_mem_write_4(bsh2, off2, value);
			off1 += 4;
			off2 += 4;
		}
	} else {		/* dest after src: copy backwards */
		off1 += (count - 1) * 4;
		off2 += (count - 1) * 4;
		while (count--) {
			value = __shpcic_mem_read_4(bsh1, off1);
			__shpcic_mem_write_4(bsh2, off2, value);
			off1 -= 4;
			off2 -= 4;
		}
	}
}
