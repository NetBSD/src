/*	$NetBSD: vlpci.c,v 1.13 2022/01/21 19:12:28 thorpej Exp $	*/

/*
 * Copyright (c) 2017 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vlpci.c,v 1.13 2022/01/21 19:12:28 thorpej Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/mutex.h>
#include <uvm/uvm.h>
#include <machine/pio.h>
#include <machine/pmap.h>
#include <machine/ofw.h>

#include <dev/isa/isavar.h>

#include <dev/ofw/openfirm.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>
#include <arm/pci_machdep.h>

#include <shark/ofw/vlpci.h>

#define VLPCI_ADDON_DEV_NO	6
#define VLPCI_IRQ		10

#define VLPCI_PCI_MEM_BASE	0x02000000
#define VLPCI_PCI_MEM_SZ	1048576

#define VLPCI_VL_MEM_BASE	0x08000000
#define VLPCI_VL_MEM_SZ		4194304

static int	vlpci_match(device_t, struct cfdata *, void *);
static void	vlpci_attach(device_t, device_t, void *);

static void	vlpci_pc_attach_hook(device_t, device_t,
    struct pcibus_attach_args *);
static int	vlpci_pc_bus_maxdevs(void *, int);
static pcitag_t	vlpci_pc_make_tag(void *, int, int, int);
static void	vlpci_pc_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t	vlpci_pc_conf_read(void *, pcitag_t, int);
static void	vlpci_pc_conf_write(void *, pcitag_t, int, pcireg_t);

static int	vlpci_pc_intr_map(const struct pci_attach_args *,
    pci_intr_handle_t *);
static const char * vlpci_pc_intr_string(void *, pci_intr_handle_t, char *,
    size_t);
static const struct evcnt * vlpci_pc_intr_evcnt(void *, pci_intr_handle_t);
static void *	vlpci_pc_intr_establish(void *, pci_intr_handle_t, int,
    int (*)(void *), void *, const char *);
static void 	vlpci_pc_intr_disestablish(void *, void *);

#ifdef __HAVE_PCI_CONF_HOOK
static int	vlpci_pc_conf_hook(void *, int, int, int, pcireg_t);
#endif
static void	vlpci_pc_conf_interrupt(void *, int, int, int, int, int *);

struct vlpci_softc {
	device_t			sc_dev;
	kmutex_t			sc_lock;
	bus_space_handle_t		sc_conf_ioh;
	bus_space_handle_t		sc_reg_ioh;
	struct arm32_pci_chipset	sc_pc;
};

CFATTACH_DECL_NEW(vlpci, sizeof(struct vlpci_softc),
    vlpci_match, vlpci_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "via,vt82c505" },
	DEVICE_COMPAT_EOL
};

vaddr_t vlpci_mem_vaddr = 0;
paddr_t vlpci_mem_paddr;
struct bus_space vlpci_memt;

static void
regwrite_1(struct vlpci_softc * const sc, uint8_t off, uint8_t val)
{

	mutex_spin_enter(&sc->sc_lock);
	bus_space_write_1(&isa_io_bs_tag, sc->sc_reg_ioh, VLPCI_INTREG_IDX_OFF,
	    off);
	bus_space_write_1(&isa_io_bs_tag, sc->sc_reg_ioh, VLPCI_INTREG_DATA_OFF,
	    val);
	mutex_spin_exit(&sc->sc_lock);
}

static uint8_t
regread_1(struct vlpci_softc * const sc, uint8_t off)
{
	uint8_t reg;

	mutex_spin_enter(&sc->sc_lock);
	bus_space_write_1(&isa_io_bs_tag, sc->sc_reg_ioh, VLPCI_INTREG_IDX_OFF,
	    off);
	reg = bus_space_read_1(&isa_io_bs_tag, sc->sc_reg_ioh,
	    VLPCI_INTREG_DATA_OFF);
	mutex_spin_exit(&sc->sc_lock);
	return reg;
}

static void
vlpci_dump_window(struct vlpci_softc *sc, int num)
{
	int regaddr = VLPCI_PCI_WND_HIADDR_REG(num);
	uint32_t addr, size;
	uint8_t attr;

	addr = regread_1(sc, regaddr) << 24;
	addr |= regread_1(sc, regaddr + 1) << 16;
	attr = regread_1(sc, regaddr + 2);
	size = 0x00010000 << __SHIFTOUT(attr, VLPCI_PCI_WND_ATTR_SZ);
	printf("memory window #%d at %08x size %08x flags %x\n", num, addr,
	    size, attr);
}

static int
vlpci_map(void *t, bus_addr_t bpa, bus_size_t size, int cacheable,
    bus_space_handle_t *bshp)
{

	*bshp = vlpci_mem_vaddr - VLPCI_PCI_MEM_BASE + bpa;
	printf("%s: %08lx -> %08lx\n", __func__, bpa, *bshp);
	return(0);
}

static paddr_t
vlpci_mmap(void *cookie, bus_addr_t addr, off_t off, int prot,
    int flags)
{
	paddr_t ret;

	ret = vlpci_mem_paddr + addr + off;

	if (flags & BUS_SPACE_MAP_PREFETCHABLE)
		return (arm_btop(ret) | ARM32_MMAP_WRITECOMBINE);
	else
		return arm_btop(ret);
}

static void
vlpci_steer_irq(struct vlpci_softc * const sc)
{
	const unsigned int_ctl[] = {
		VLPCI_INT_CTL_INTA, VLPCI_INT_CTL_INTB,
		VLPCI_INT_CTL_INTC, VLPCI_INT_CTL_INTD
	};
	uint8_t val;

	for (size_t i = 0; i < __arraycount(int_ctl); i++) {
		val = regread_1(sc, VLPCI_INT_CTL_REG(int_ctl[i]));
		val &= ~VLPCI_INT_CTL_INT2IRQ(int_ctl[i]);
		val |= VLPCI_INT_CTL_ENA(int_ctl[i]);
		val |= __SHIFTIN(VLPCI_INT_CTL_IRQ(VLPCI_IRQ),
		    VLPCI_INT_CTL_INT2IRQ(int_ctl[i]));
		regwrite_1(sc, VLPCI_INT_CTL_REG(int_ctl[i]), val);
	}
}

static int
vlpci_match(device_t parent, struct cfdata *match, void *aux)
{
	struct ofbus_attach_args * const oba = aux;

						/* beat generic ofbus */
	return of_compatible_match(oba->oba_phandle, compat_data) * 2;
}

static void
vlpci_attach(device_t parent, device_t self, void *aux)
{
	struct ofbus_attach_args * const oba = aux;
	struct vlpci_softc * const sc = device_private(self);
	pci_chipset_tag_t const pc = &sc->sc_pc;
	struct pcibus_attach_args pba;
	pcitag_t tag;
	pcireg_t cmd;

	aprint_normal("\n");

	sc->sc_dev = self;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);
	memset(&pba, 0, sizeof(pba));

	if (bus_space_map(&isa_io_bs_tag, VLPCI_INTREG_BASE, VLPCI_INTREG_SZ,
	    0, &sc->sc_reg_ioh) != 0) {
		aprint_error_dev(self, "failed to map internal reg port\n");
		return;
	}
	if (bus_space_map(&isa_io_bs_tag, VLPCI_CFGREG_BASE, VLPCI_CFGREG_SZ,
	    0, &sc->sc_conf_ioh) != 0) {
		aprint_error_dev(self, "failed to map configuration port\n");
		return;
	}

	/* Enable VLB/PCI bridge */
	regwrite_1(sc, VLPCI_MISC_1_REG, VLPCI_MISC_1_LOCAL_PIN |
	    VLPCI_MISC_1_COMPAT_ISA_BOFF);
	regwrite_1(sc, VLPCI_MISC_CTL_REG, __SHIFTIN(VLPCI_MISC_CTL_HIADDR_DIS,
	    VLPCI_MISC_CTL_HIADDR) | VLPCI_MISC_CTL_IOCHCK_PIN);
	regwrite_1(sc, VLPCI_CFG_MISC_CTL_REG,
	    __SHIFTIN(VLPCI_CFG_MISC_CTL_INT_CTL_CONV,
	    VLPCI_CFG_MISC_CTL_INT_CTL) | VLPCI_CFG_MISC_CTL_LREQI_LGNTO_PIN);
	regwrite_1(sc, VLPCI_IRQ_MODE_REG, 0x00); /* don't do per-INTx conversions */
	vlpci_steer_irq(sc);
	/*
	 * XXX
	 * set memory size to 255MB, so the bridge knows which cycles go to RAM
	 * shark's RAM is in the upper half of the lower 256MB, part of the
	 * lower half is occupied by the graphics chip
	 * ... or that's the theory. OF puts PCI BARS at 0x02000000 which
	 * overlaps with when we do this and pci memory access doesn't work.
	 */
	regwrite_1(sc, VLPCI_OBD_MEM_SZ_REG, 1);

	regwrite_1(sc, VLPCI_BUF_CTL_REG, VLPCI_BUF_CTL_PCI_DYN_ACC_DEC);
	regwrite_1(sc, VLPCI_VL_TIM_REG, VLPCI_VL_TIM_OBD_MEM_1ST_DAT);
	printf("reg 0x83 %02x\n", regread_1(sc, VLPCI_VL_TIM_REG));

#if 1
	/* program window #0 to 0x08000000 */
	regwrite_1(sc, VLPCI_PCI_WND_HIADDR_REG(VLPCI_PCI_WND_NO_1),
	    VLPCI_PCI_WND_HIADDR_MEM(VLPCI_VL_MEM_BASE));
	regwrite_1(sc, VLPCI_PCI_WND_LOADDR_REG(VLPCI_PCI_WND_NO_1),
	    VLPCI_PCI_WND_LOADDR_MEM(VLPCI_VL_MEM_BASE));
	regwrite_1(sc, VLPCI_PCI_WND_ATTR_REG(VLPCI_PCI_WND_NO_1),
	    VLPCI_PCI_WND_ATTR_VL |
	    __SHIFTIN(VLPCI_PCI_WND_ATTR_SZ_MEM(VLPCI_VL_MEM_SZ),
	    VLPCI_PCI_WND_ATTR_SZ));
#else
	regwrite_1(sc, VLPCI_PCI_WND_HIADDR_REG(VLPCI_PCI_WND_NO_1), 0x00);
	regwrite_1(sc, VLPCI_PCI_WND_LOADDR_REG(VLPCI_PCI_WND_NO_1), 0x00);
	regwrite_1(sc, VLPCI_PCI_WND_ATTR_REG(VLPCI_PCI_WND_NO_1), 0x00);
#endif

	vlpci_mem_paddr = VLPCI_PCI_MEM_BASE;	/* get from OF! */

	/*
	 * we map in 1MB at 0x02000000, so program window #1 accordingly
	 */
	regwrite_1(sc, VLPCI_PCI_WND_HIADDR_REG(VLPCI_PCI_WND_NO_2),
	    VLPCI_PCI_WND_HIADDR_MEM(vlpci_mem_paddr));
	regwrite_1(sc, VLPCI_PCI_WND_LOADDR_REG(VLPCI_PCI_WND_NO_2),
	    VLPCI_PCI_WND_LOADDR_MEM(vlpci_mem_paddr));
	regwrite_1(sc, VLPCI_PCI_WND_ATTR_REG(VLPCI_PCI_WND_NO_2),
	    VLPCI_PCI_WND_ATTR_PCI |
	    __SHIFTIN(VLPCI_PCI_WND_ATTR_SZ_MEM(VLPCI_PCI_MEM_SZ),
	    VLPCI_PCI_WND_ATTR_SZ));

	/* now map in some of the memory space */
	printf("vlpci_mem_vaddr %08lx\n", vlpci_mem_vaddr);
	memcpy(&vlpci_memt, &isa_io_bs_tag, sizeof(struct bus_space));
	vlpci_memt.bs_cookie = (void *)vlpci_mem_vaddr;
	vlpci_memt.bs_map = vlpci_map;
	vlpci_memt.bs_mmap = vlpci_mmap;

	pc->pc_conf_v = sc;
	pc->pc_attach_hook = vlpci_pc_attach_hook;
	pc->pc_bus_maxdevs = vlpci_pc_bus_maxdevs;
	pc->pc_make_tag = vlpci_pc_make_tag;
	pc->pc_decompose_tag = vlpci_pc_decompose_tag;
	pc->pc_conf_read = vlpci_pc_conf_read;
	pc->pc_conf_write = vlpci_pc_conf_write;

	pc->pc_intr_v = sc;
	pc->pc_intr_map = vlpci_pc_intr_map;
	pc->pc_intr_string = vlpci_pc_intr_string;
	pc->pc_intr_evcnt = vlpci_pc_intr_evcnt;
	pc->pc_intr_establish = vlpci_pc_intr_establish;
	pc->pc_intr_disestablish = vlpci_pc_intr_disestablish;

#ifdef __HAVE_PCI_CONF_HOOK
	pc->pc_conf_hook = vlpci_pc_conf_hook;
#endif
	pc->pc_conf_interrupt = vlpci_pc_conf_interrupt;

	/* try to assure IO space is enabled on the default device-function */
	tag = vlpci_pc_make_tag(sc, 0, VLPCI_ADDON_DEV_NO, 0);
	cmd = vlpci_pc_conf_read(sc, tag, PCI_COMMAND_STATUS_REG);
	vlpci_pc_conf_write(sc, tag, PCI_COMMAND_STATUS_REG,
	    cmd | PCI_COMMAND_IO_ENABLE);

	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
	pba.pba_iot = &isa_io_bs_tag;
	pba.pba_memt = &vlpci_memt;
	pba.pba_dmat = &isa_bus_dma_tag;
	pba.pba_pc = &sc->sc_pc;
	pba.pba_bus = 0;

	printf("dma %lx %lx, %lx\n", isa_bus_dma_tag._ranges[0].dr_sysbase,
	    isa_bus_dma_tag._ranges[0].dr_busbase,
	    isa_bus_dma_tag._ranges[0].dr_len);

	vlpci_dump_window(sc, VLPCI_PCI_WND_NO_1);
	vlpci_dump_window(sc, VLPCI_PCI_WND_NO_2);
	vlpci_dump_window(sc, VLPCI_PCI_WND_NO_3);

	config_found(self, &pba, pcibusprint,
	    CFARGS(.devhandle = device_handle(self)));
}

static void
vlpci_pc_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

static int
vlpci_pc_bus_maxdevs(void *v, int busno)
{

	return busno == 0 ? 32 : 0;
}

static pcitag_t
vlpci_pc_make_tag(void *v, int b, int d, int f)
{

	return (b << 16) | (d << 11) | (f << 8);
}

static void
vlpci_pc_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp)
		*bp = (tag >> 16) & 0xff;
	if (dp)
		*dp = (tag >> 11) & 0x1f;
	if (fp)
		*fp = (tag >> 8) & 0x7;
}

static pcireg_t
vlpci_pc_conf_read(void *v, pcitag_t tag, int offset)
{
	struct vlpci_softc * const sc = v;
	pcireg_t ret;

	KASSERT((offset & 3) == 0);

	if (offset >= PCI_CONF_SIZE)
		return 0xffffffff;

	mutex_spin_enter(&sc->sc_lock);
	bus_space_write_4(&isa_io_bs_tag, sc->sc_conf_ioh,
	    VLPCI_CFGREG_ADDR_OFF, 0x80000000UL|tag|offset);
	ret = bus_space_read_4(&isa_io_bs_tag, sc->sc_conf_ioh,
	    VLPCI_CFGREG_DATA_OFF);
	mutex_spin_exit(&sc->sc_lock);

#if 0
	device_printf(sc->sc_dev, "%s tag %x offset %x ret %x\n",
	    __func__, (unsigned int)tag, offset, ret);
#endif

	return ret;
}

static void
vlpci_pc_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct vlpci_softc * const sc = v;

	KASSERT((offset & 3) == 0);

	if (offset >= PCI_CONF_SIZE)
		return;

#if 0
	device_printf(sc->sc_dev, "%s tag %x offset %x val %x\n",
	    __func__, (unsigned int)tag, offset, val);
#endif

	mutex_spin_enter(&sc->sc_lock);
	bus_space_write_4(&isa_io_bs_tag, sc->sc_conf_ioh,
	    VLPCI_CFGREG_ADDR_OFF, 0x80000000UL|tag|offset);
	bus_space_write_4(&isa_io_bs_tag, sc->sc_conf_ioh,
	    VLPCI_CFGREG_DATA_OFF, val);
	mutex_spin_exit(&sc->sc_lock);
}

static int
vlpci_pc_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ih)
{

	switch (pa->pa_intrpin) {
	default:
	case 0:
		return EINVAL;
	case 1:
	case 2:
	case 3:
	case 4:
		*ih = VLPCI_IRQ;
		return 0;
	}
}

static const char *
vlpci_pc_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{

	if (ih == PCI_INTERRUPT_PIN_NONE)
		return NULL;
	snprintf(buf, len, "irq %llu", ih);
	return buf;
}

static const struct evcnt *
vlpci_pc_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	return NULL;
}

static void *
vlpci_pc_intr_establish(void *v, pci_intr_handle_t pih, int ipl,
    int (*callback)(void *), void *arg, const char *foo)
{

	if (pih == 0)
		return NULL;

	return isa_intr_establish(NULL, pih, IST_LEVEL, ipl, callback, arg);
}

static void
vlpci_pc_intr_disestablish(void *v, void *w)
{

	return isa_intr_disestablish(NULL, v);
}

#ifdef __HAVE_PCI_CONF_HOOK
static int
vlpci_pc_conf_hook(void *v, int b, int d, int f, pcireg_t id)
{

	return PCI_CONF_DEFAULT /*& ~PCI_CONF_ENABLE_BM*/;
}
#endif

static void
vlpci_pc_conf_interrupt(void *v, int bus, int dev, int ipin, int swiz,
    int *ilinep)
{

	*ilinep = 0xff; /* XXX */
}
