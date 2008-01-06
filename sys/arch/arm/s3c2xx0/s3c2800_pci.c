/*	$NetBSD: s3c2800_pci.c,v 1.13 2008/01/06 01:37:56 matt Exp $	*/

/*
 * Copyright (c) 2002 Fujitsu Component Limited
 * Copyright (c) 2002 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * derived from evbarm/ifpga/ifpga_pci.c
 */

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

/*
 * PCI configuration support for Samsung s3c2800.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: s3c2800_pci.c,v 1.13 2008/01/06 01:37:56 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <arm/s3c2xx0/s3c2800reg.h>
#include <arm/s3c2xx0/s3c2800var.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pciconf.h>

#include "opt_pci.h"
#include "pci.h"

/*
 * pci tag encoding.
 * also useful for configuration type 0 address
 */
#define BUSNO_SHIFT	16
#define BUSNO_MASK	(0xff<<BUSNO_SHIFT)
#define DEVNO_SHIFT	11
#define DEVNO_MASK	(0x1f<<DEVNO_SHIFT)
#define tag_to_devno(tag)	(((tag)&DEVNO_MASK)>>DEVNO_SHIFT)
#define FUNNO_SHIFT	8
#define FUNNO_MASK	(0x07<<FUNNO_SHIFT)

#define BUS0_DEV_MIN	1
#define BUS0_DEV_MAX	21

void	s3c2800_pci_attach_hook(struct device *, struct device *,
			        struct pcibus_attach_args *);
int	s3c2800_pci_bus_maxdevs(void *, int);
pcitag_t s3c2800_pci_make_tag(void *, int, int, int);
void	s3c2800_pci_decompose_tag(void *, pcitag_t, int *, int *, int *);
pcireg_t s3c2800_pci_conf_read(void *, pcitag_t, int);
void	s3c2800_pci_conf_write(void *, pcitag_t, int, pcireg_t);
int	s3c2800_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *s3c2800_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *s3c2800_pci_intr_evcnt(void *, pci_intr_handle_t);
void *s3c2800_pci_intr_establish(void *, pci_intr_handle_t, int,
				  int (*) (void *), void *);
void	s3c2800_pci_intr_disestablish(void *, void *);

#define	PCI_CONF_LOCK(s)	(s) = disable_interrupts(I32_bit)
#define	PCI_CONF_UNLOCK(s)	restore_interrupts((s))

struct sspci_irq_handler {
	int (*func) (void *);
	void *arg;
	int level;
	SLIST_ENTRY(sspci_irq_handler) link;
};

struct sspci_softc {
	struct device sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_reg_ioh;
	bus_space_handle_t sc_conf0_ioh;	/* config type0 space */
	bus_space_handle_t sc_conf1_ioh;	/* config type1 space */

	uint32_t sc_pciinten;	/* copy of PCIINTEN register */

	/* list of interrupt handlers. SLIST is not good for removing
	 * element from it, but intr_disestablish is rarely called */
	SLIST_HEAD(, sspci_irq_handler) sc_irq_handlers;

	void *sc_softinterrupt;
};

static int sspci_match(struct device *, struct cfdata *, void *aux);
static void sspci_attach(struct device *, struct device *, void *);

static int sspci_bs_map(void *, bus_addr_t, bus_size_t, int, 
			     bus_space_handle_t *);
static int sspci_init_controller(struct sspci_softc *);
static int sspci_intr(void *);
static void sspci_softintr(void *);

/* attach structures */
CFATTACH_DECL(sspci, sizeof(struct sspci_softc), sspci_match, sspci_attach,
    NULL, NULL);


struct arm32_pci_chipset sspci_chipset = {
	NULL,		/* conf_v */
	s3c2800_pci_attach_hook,
	s3c2800_pci_bus_maxdevs,
	s3c2800_pci_make_tag,
	s3c2800_pci_decompose_tag,
	s3c2800_pci_conf_read,
	s3c2800_pci_conf_write,
	NULL,		/* intr_v */
	s3c2800_pci_intr_map,
	s3c2800_pci_intr_string,
	s3c2800_pci_intr_evcnt,
	s3c2800_pci_intr_establish,
	s3c2800_pci_intr_disestablish
};


/*
 * bus space tag for PCI IO/Memory access space.
 * filled in by sspci_attach()
 */
struct bus_space sspci_io_tag, sspci_mem_tag;

static int
sspci_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

static void
sspci_attach(struct device *parent, struct device *self, void *aux)
{
	struct sspci_softc *sc = (struct sspci_softc *) self;
	struct s3c2xx0_attach_args *aa = aux;
	bus_space_tag_t iot;
	bus_dma_tag_t pci_dma_tag;
	const char *error_on;	/* for panic message */
#if defined(PCI_NETBSD_CONFIGURE)
	struct extent *ioext, *memext;
	struct pcibus_attach_args pci_pba;
#endif

#define FAIL(which)  do { \
	error_on=(which); goto abort; }while(/*CONSTCOND*/0)

	iot = sc->sc_iot = aa->sa_iot;
	if (bus_space_map(iot, S3C2800_PCICTL_BASE,
		S3C2800_PCICTL_SIZE, 0, &sc->sc_reg_ioh))
		FAIL("control regs");

	if (bus_space_map(iot, S3C2800_PCI_CONF0_BASE,
		S3C2800_PCI_CONF0_SIZE, 0, &sc->sc_conf0_ioh))
		FAIL("config type 0 area");

#if 0
	if (bus_space_map(iot, S3C2800_PCI_CONF1_BASE,
		S3C2800_PCI_CONF1_SIZE, 0, &sc->sc_conf1_ioh))
		FAIL("config type 1 area");
#endif
	printf("\n");

	SLIST_INIT(&sc->sc_irq_handlers);
	if (!s3c2800_intr_establish(S3C2800_INT_PCI, IPL_AUDIO, IST_LEVEL,
		sspci_intr, sc))
		FAIL("intr_establish");

	sc->sc_softinterrupt = softint_establish(SOFTINT_SERIAL,
	    sspci_softintr, sc);
	if (sc->sc_softinterrupt == NULL)
		FAIL("softint_establish");

#if defined(PCI_NETBSD_CONFIGURE)
	if (sspci_init_controller(sc)) {
		printf("%s: failed to initialize controller\n", self->dv_xname);
		return;
	}
#endif

	sc->sc_pciinten =
	    PCIINT_INA | PCIINT_SER | PCIINT_TPE | PCIINT_MPE |
	    PCIINT_MFE | PCIINT_PRA | PCIINT_PRD;

	bus_space_write_4(iot, sc->sc_reg_ioh, PCICTL_PCIINTEN,
	    sc->sc_pciinten);

	{
		pcireg_t id_reg, class_reg;
		char buf[1000];

		id_reg = bus_space_read_4(iot, sc->sc_reg_ioh,
		    PCI_ID_REG);
		class_reg = bus_space_read_4(iot,
		    sc->sc_reg_ioh, PCI_CLASS_REG);

		pci_devinfo(id_reg, class_reg, 1, buf, sizeof(buf));
		printf("%s: %s\n", self->dv_xname, buf);
	}

#if defined(PCI_NETBSD_CONFIGURE)
	ioext = extent_create("pciio", 0x100, S3C2800_PCI_IOSPACE_SIZE - 0x100,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);

	memext = extent_create("pcimem", 0, S3C2800_PCI_MEMSPACE_SIZE,
	    M_DEVBUF, NULL, 0, EX_NOWAIT);

	sspci_chipset.pc_conf_v = (void *) sc;
	sspci_chipset.pc_intr_v = (void *) sc;

	pci_configure_bus(&sspci_chipset, ioext, memext, NULL, 0,
	    arm_dcache_align);

	extent_destroy(memext);
	extent_destroy(ioext);
#endif				/* PCI_NETBSD_CONFIGURE */

	/* initialize bus space tag */
	sspci_io_tag = *iot;
	sspci_io_tag.bs_cookie = (void *) S3C2800_PCI_IOSPACE_BASE;
	sspci_io_tag.bs_map = sspci_bs_map;
	sspci_mem_tag = *iot;
	sspci_mem_tag.bs_cookie = (void *) S3C2800_PCI_MEMSPACE_BASE;
	sspci_mem_tag.bs_map = sspci_bs_map;


	/* Platform provides PCI DMA tag */
	pci_dma_tag = s3c2800_pci_dma_init();

	pci_pba.pba_pc = &sspci_chipset;
	pci_pba.pba_iot = &sspci_io_tag;
	pci_pba.pba_memt = &sspci_mem_tag;
	pci_pba.pba_dmat = pci_dma_tag;
	pci_pba.pba_dmat64 = NULL;
	pci_pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pci_pba.pba_bus = 0;
	pci_pba.pba_bridgetag = NULL;

	config_found_ia(self, "pcibus", &pci_pba, pcibusprint);

	return;

#undef FAIL
abort:
	panic("%s: map failed (%s)",
	    self->dv_xname, error_on);
}


static int
sspci_bs_map(void *t, bus_addr_t bpa, bus_size_t size, int flag,
	     bus_space_handle_t * bshp)
{
	bus_addr_t startpa, endpa;
	vaddr_t va;

#ifdef PCI_DEBUG
	printf("sspci_bs_map: t=%p, addr=%lx, size=%lx, flag=%d\n",
	    t, bpa, size, flag);
#endif

	/* Round the allocation to page boundries */
	startpa = trunc_page(bpa);
	endpa = round_page(bpa + size);

	/* Get some VM.  */
	va = uvm_km_alloc(kernel_map, endpa - startpa, 0,
	    UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0)
		return ENOMEM;

	/* Store the bus space handle */
	*bshp = va + (bpa & PGOFSET);

	/* Now map the pages */
	/* The cookie is the physical base address for PCI I/O or memory area */
	while (startpa < endpa) {
		/* XXX pmap_kenter_pa maps pages cacheable -- not what we
		 * want.  */
		pmap_enter(pmap_kernel(), va, (bus_addr_t) t + startpa,
		    VM_PROT_READ | VM_PROT_WRITE, 0);
		va += PAGE_SIZE;
		startpa += PAGE_SIZE;
	}
	pmap_update(pmap_kernel());

	return 0;
}



void
pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int func,
		   int swiz, int *iline)
{
#ifdef PCI_DEBUG
	printf("pci_conf_interrupt(pc(%lx), bus(%d), dev(%d), func(%d), swiz(%d), *iline(%p)\n", (unsigned long) pc, bus, dev, func, swiz, iline);
#endif
	if (bus == 0) {
		*iline = dev;
	} else {
		panic("pci_conf_interrupt: bus=%d: not yet implemented", bus);
	}
}

void
s3c2800_pci_attach_hook(struct device * parent, struct device * self,
			struct pcibus_attach_args * pba)
{

	/* Nothing to do. */
#ifdef PCI_DEBUG
	printf("s3c2800_pci_attach_hook()\n");
#endif
}

int
s3c2800_pci_bus_maxdevs(void *v, int busno)
{

#ifdef PCI_DEBUG
	printf("s3c2800_pci_bus_maxdevs(v=%p, busno=%d)\n", v, busno);
#endif
	return (32);
}
pcitag_t
s3c2800_pci_make_tag(void *v, int bus, int device, int function)
{

	return ((bus << BUSNO_SHIFT) | (device << DEVNO_SHIFT) |
	    (function << FUNNO_SHIFT));
}

void
s3c2800_pci_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp != NULL)
		*bp = (tag >> BUSNO_SHIFT) & 0xff;
	if (dp != NULL)
		*dp = (tag >> DEVNO_SHIFT) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> FUNNO_SHIFT) & 0x7;
}

static vaddr_t
make_pci_conf_va(struct sspci_softc * sc, pcitag_t tag, int offset)
{
	if ((tag & BUSNO_MASK) == 0) {
		/* configuration type 0 */
		int devno = tag_to_devno(tag);

		if (devno < BUS0_DEV_MIN || BUS0_DEV_MAX < devno)
			return 0;

		return (vaddr_t) bus_space_vaddr(sc->sc_iot, sc->sc_conf0_ioh) +
		    (tag & (DEVNO_MASK | FUNNO_MASK)) + offset;
	} else {
		/* XXX */
		return (vaddr_t) - 1;	/* cause fault */
	}
}


pcireg_t
s3c2800_pci_conf_read(void *v, pcitag_t tag, int offset)
{
	struct sspci_softc *sc = v;
	vaddr_t va = make_pci_conf_va(sc, tag, offset);
	int s;
	pcireg_t rv;

#ifdef PCI_DEBUG
	printf("s3c2800_pci_conf_read: base=%lx tag=%lx offset=%x\n",
	    sc->sc_conf0_ioh, tag, offset);
#endif
	if (va == 0)
		return -1;

	PCI_CONF_LOCK(s);

	if (badaddr_read((void *) va, sizeof(rv), &rv)) {
#if PCI_DEBUG
		printf("conf_read: %lx bad address\n", va);
#endif
		rv = (pcireg_t) - 1;
	}
	PCI_CONF_UNLOCK(s);

	return rv;
}

void
s3c2800_pci_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct sspci_softc *sc = v;
	vaddr_t va = make_pci_conf_va(sc, tag, offset);
	u_int s;

#ifdef PCI_DEBUG
	printf("s3c2800_pci_conf_write: tag=%lx offset=%x -> va=%lx\n", tag, offset, va);
#endif

	PCI_CONF_LOCK(s);

	*(pcireg_t *) va = val;

	PCI_CONF_UNLOCK(s);
}

void *
s3c2800_pci_intr_establish(void *pcv, pci_intr_handle_t ih, int level,
			   int (*func) (void *), void *arg)
{
	struct sspci_softc *sc = pcv;
	struct sspci_irq_handler *handler;
	int s;

#ifdef PCI_DEBUG
	printf("s3c2800_pci_intr_establish(pcv=%p, ih=0x%lx, level=%d, "
	    "func=%p, arg=%p)\n", pcv, ih, level, func, arg);
#endif

	handler = malloc(sizeof *handler, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (handler == NULL)
		panic("sspci_intr_establish: can't malloc handler info");

	handler->func = func;
	handler->arg = arg;
	handler->level = level;

	s = splhigh();
	SLIST_INSERT_HEAD(&sc->sc_irq_handlers, handler, link);
	splx(s);

	return (handler);
}

void
s3c2800_pci_intr_disestablish(void *pcv, void *cookie)
{
	struct sspci_softc *sc = pcv;
	struct sspci_irq_handler *ih = cookie;
	int s;

#ifdef PCI_DEBUG
	printf("s3c2800_pci_intr_disestablish(pcv=%p, cookie=%p)\n",
	    pcv, cookie);
#endif

	s = splhigh();
	SLIST_REMOVE(&sc->sc_irq_handlers, ih, sspci_irq_handler, link);
	splx(s);
}

int
s3c2800_pci_intr_map(struct pci_attach_args * pa, pci_intr_handle_t * ihp)
{
#ifdef PCI_DEBUG
	int pin = pa->pa_intrpin;
	void *pcv = pa->pa_pc;
	pcitag_t intrtag = pa->pa_intrtag;
	int bus, device, function;

	s3c2800_pci_decompose_tag(pcv, intrtag, &bus, &device, &function);
	printf("s3c2800_pci_intr_map: pcv=%p, tag=%08lx pin=%d dev=%d\n",
	    pcv, intrtag, pin, device);
#endif


	/* S3C2800 has only one interrupt line for PCI */
	*ihp = 0;
	return 0;
}

const char *
s3c2800_pci_intr_string(void *pcv, pci_intr_handle_t ih)
{
	/* We have only one interrupt source from PCI */
	return "pciint";
}

const struct evcnt *
s3c2800_pci_intr_evcnt(void *pcv, pci_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}
/*
 * Initialize PCI controller
 */
int
sspci_init_controller(struct sspci_softc * sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_reg_ioh;

	/* disable PCI command */
	bus_space_write_4(iot, ioh, PCI_COMMAND_STATUS_REG,
	    0xffff0000);

	/* latency=0x10, cacheline=8 */
	bus_space_write_4(iot, ioh, PCI_BHLC_REG,
	    PCI_BHLC_CODE(0, 0, 0, 0x10, 8));

	bus_space_write_4(iot, ioh, PCI_INTERRUPT_REG,
	    PCI_INTERRUPT_CODE(0, 0, 0, 0));



#if 1
	bus_space_write_4(iot, ioh, PCI_MAPREG_START,
	    PCI_MAPREG_MEM_TYPE_32BIT | 0x80000000);
	/* Cover all DBANKs with BAR0 */
	bus_space_write_4(iot, ioh, PCICTL_PCIBAM0, 0xf8000000);
	bus_space_write_4(iot, ioh, PCICTL_PCIBATPA0, S3C2800_DBANK0_START);
#else
	bus_space_write_4(iot, ioh, PCI_MAPREG_START,
	    PCI_MAPREG_MEM_TYPE_32BIT | 0xf0000000);
	bus_space_write_4(iot, ioh, PCI_MAPREG_START + 4,
	    PCI_MAPREG_MEM_TYPE_32BIT | 0x80000000);

	bus_space_write_4(iot, ioh, PCICTL_PCIBAM0, 0xffff0000);
	bus_space_write_4(iot, ioh, PCICTL_PCIBATPA0, 0xffff0000);
	bus_space_write_4(iot, ioh, PCICTL_PCIBAM1, 0xf1000000);
	bus_space_write_4(iot, ioh, PCICTL_PCIBATPA1, S3C2800_DBANK0_START);
#endif

	bus_space_write_4(iot, ioh, PCI_COMMAND_STATUS_REG,
	    PCI_STATUS_PARITY_DETECT |
	    PCI_STATUS_SPECIAL_ERROR |
	    PCI_STATUS_MASTER_ABORT |
	    PCI_STATUS_MASTER_TARGET_ABORT |
	    PCI_STATUS_TARGET_TARGET_ABORT |
	    PCI_STATUS_DEVSEL_MEDIUM |
	    PCI_STATUS_PARITY_ERROR |
	    PCI_STATUS_BACKTOBACK_SUPPORT |
	    PCI_STATUS_CAPLIST_SUPPORT |
	    PCI_COMMAND_MASTER_ENABLE |
	    PCI_COMMAND_MEM_ENABLE |
	    PCI_COMMAND_IO_ENABLE);

	bus_space_write_4(iot, ioh, PCICTL_PCICON,
	    PCICON_ARB | PCICON_HST);

	bus_space_write_4(iot, ioh, PCICTL_PCISET, 0);
	/* clear all interrupts */
	bus_space_write_4(iot, ioh, PCICTL_PCIINTST, 0xffffffff);
	bus_space_write_4(iot, ioh, PCICTL_PCIINTEN, 0);

	bus_space_write_4(iot, ioh, PCICTL_PCICON,
	    PCICON_RDY | PCICON_CFD | PCICON_ATS |
	    PCICON_ARB | PCICON_HST);


#ifdef PCI_DEBUG
	{
		pcireg_t reg;
		int i;

		for (i = 0; i <= 0x40; i += sizeof(pcireg_t)) {
			reg = bus_space_read_4(iot, ioh, i);
			printf("%03x: %08x\n", i, reg);
		}
		for (i = 0x100; i <= 0x154; i += sizeof(pcireg_t)) {
			reg = bus_space_read_4(iot, ioh, i);
			printf("%03x: %08x\n", i, reg);
		}
	}
#endif
	return 0;
}


static const char *pci_abnormal_error_name[] = {
	"PCI reset deasserted",
	"PCI reset asserted",
	"PCI master detected fatal error",
	"PCI master detected parity error",
	"PCI target detected parity error",
	"PCI SERR# asserted",
};

static int
sspci_intr(void *arg)
{
	struct sspci_softc *sc = arg;
	int s;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_reg_ioh;
	uint32_t interrupts, errors;

	interrupts = bus_space_read_4(iot, ioh, PCICTL_PCIINTST);

	if (interrupts & PCIINT_INA) {
		s = splhigh();
		softint_schedule(sc->sc_softinterrupt);

		/* mask INTA itnerrupt until softinterrupt is handled */
		sc->sc_pciinten &= ~PCIINT_INA;
		bus_space_write_4(iot, ioh, PCICTL_PCIINTEN,
		    sc->sc_pciinten);

		/* acknowledge INTA interrupt */
		bus_space_write_4(iot, ioh, PCICTL_PCIINTST, PCIINT_INA);

		splx(s);

		interrupts &= ~PCIINT_INA;

	}
	errors = interrupts & (PCIINT_SER | PCIINT_TPE | PCIINT_MPE |
	    PCIINT_MFE | PCIINT_PRA | PCIINT_PRD);
	if (errors) {
		int i;

		for (i = 0; errors; ++i) {
			if ((errors & (1 << i)) == 0)
				continue;

			printf("%s: %s\n", sc->sc_dev.dv_xname,
			    pci_abnormal_error_name[i > 4 ? 5 : i]);

			errors &= ~(1 << i);
		}
		/* acknowledge interrupts */
		bus_space_write_4(iot, ioh, PCICTL_PCIINTST, interrupts);
	}
	return 0;
}

static void
sspci_softintr(void *arg)
{
	struct sspci_softc *sc = arg;
	struct sspci_irq_handler *ih;
	int s;


	SLIST_FOREACH(ih, &(sc->sc_irq_handlers), link) {
		s = _splraise(ih->level);
		ih->func(ih->arg);
		splx(s);
	}

	/* unmask INTA interrupt */
	s = splhigh();
	sc->sc_pciinten |= PCIINT_INA;
	bus_space_write_4(sc->sc_iot, sc->sc_reg_ioh, PCICTL_PCIINTEN,
	    sc->sc_pciinten);
	splx(s);
}
