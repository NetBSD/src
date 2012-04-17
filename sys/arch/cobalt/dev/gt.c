/*	$NetBSD: gt.c,v 1.27.2.1 2012/04/17 00:06:11 yamt Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gt.c,v 1.27.2.1 2012/04/17 00:06:11 yamt Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/file.h>
#include <sys/intr.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/uio.h>

#include <machine/autoconf.h>

#include <mips/cache.h>

#include <dev/pci/pcivar.h>
#ifdef PCI_NETBSD_CONFIGURE
#include <dev/pci/pciconf.h>
#endif

#include <cobalt/dev/gtreg.h>

struct gt_softc {
	device_t	sc_dev;

	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct cobalt_pci_chipset sc_pc;
};

static int	gt_match(device_t, cfdata_t, void *);
static void	gt_attach(device_t, device_t, void *);
static int	gt_print(void *aux, const char *pnp);

static void	gt_timer_init(struct gt_softc *sc);
#if 0 /* unused */
static void	gt_timer0_init(void *);
static long	gt_timer0_read(void *);
#endif

CFATTACH_DECL_NEW(gt, sizeof(struct gt_softc),
    gt_match, gt_attach, NULL, NULL);

static int
gt_match(device_t parent, cfdata_t cf, void *aux)
{

	return 1;
}

#define GT_REG_REGION	0x1000

static void
gt_attach(device_t parent, device_t self, void *aux)
{
	struct gt_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
#if NPCI > 0
	pci_chipset_tag_t pc;
	struct pcibus_attach_args pba;
#endif

	sc->sc_dev = self;
	sc->sc_bst = ma->ma_iot;
	if (bus_space_map(sc->sc_bst, ma->ma_addr, GT_REG_REGION,
	    0, &sc->sc_bsh)) {
		aprint_error(": unable to map GT64111 registers\n");
		return;
	}

	aprint_normal("\n");

	gt_timer_init(sc);

	bus_space_write_4(sc->sc_bst, sc->sc_bsh, GT_PCI_COMMAND,
	    (bus_space_read_4(sc->sc_bst, sc->sc_bsh, GT_PCI_COMMAND) &
	    ~PCI_SYNCMODE) | PCI_PCLK_HIGH);

	(void)bus_space_read_4(sc->sc_bst, sc->sc_bsh, GT_PCI_TIMEOUT_RETRY);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, GT_PCI_TIMEOUT_RETRY,
	    0x00 << PCI_RETRYCTR_SHIFT | 0xff << PCI_TIMEOUT1_SHIFT | 0xff);

#if NPCI > 0
	pc = &sc->sc_pc;
	pc->pc_bst = sc->sc_bst;
	pc->pc_bsh = sc->sc_bsh;

#ifdef PCI_NETBSD_CONFIGURE
	pc->pc_ioext = extent_create("pciio", 0x10001000, 0x11ffffff,
	    NULL, 0, EX_NOWAIT);
	pc->pc_memext = extent_create("pcimem", 0x12000000, 0x13ffffff,
	    NULL, 0, EX_NOWAIT);
	pci_configure_bus(pc, pc->pc_ioext, pc->pc_memext, NULL, 0,
	    mips_cache_info.mci_dcache_align);
#endif
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
		PCI_FLAGS_MRL_OKAY | /*PCI_FLAGS_MRM_OKAY|*/ PCI_FLAGS_MWI_OKAY;
	pba.pba_pc = pc;
	config_found_ia(self, "pcibus", &pba, gt_print);
#endif
}

static int
gt_print(void *aux, const char *pnp)
{

	/* XXX */
	return 0;
}

static void
gt_timer_init(struct gt_softc *sc)
{

	/* stop timer0 */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, GT_TIMER_CTRL,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, GT_TIMER_CTRL) & ~ENTC0);
	/* mask timer0 interrupt */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, GT_MASTER_MASK,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, GT_MASTER_MASK) & ~T0EXP);
}

#if 0	/* unused; now NetBSD/cobalt uses CPU INT5 for hardclock(9) */
#define TIMER0_INIT_VALUE 500000

static void
gt_timer0_init(void *cookie)
{
	struct gt_softc *sc = cookie;

	bus_space_write_4(sc->sc_bst, sc->sc_bsh,
	    GT_TIMER_COUNTER0, TIMER0_INIT_VALUE);
	/* start timer0 */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, GT_TIMER_CTRL,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, GT_TIMER_CTRL) | ENTC0);
	/* unmask timer0 interrupt */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, GT_MASTER_MASK,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, GT_MASTER_MASK) | T0EXP);
}

static long
gt_timer0_read(void *cookie)
{
	struct gt_softc *sc = cookie;
	uint32_t counter0;

	counter0 = bus_space_read_4(sc->sc_bst, sc->sc_bsh, GT_TIMER_COUNTER0);
	counter0 = TIMER0_INIT_VALUE - counter0;
#if 0
	counter /= 50;
#else
	/*
	 * From pmax/pmax/dec_3min.c:
	 * 1/64 + 1/256 + 1/2048 = 41/2048 = 1/49.9512...
	 */
	counter0 = (counter0 >> 6) + (counter0 >> 8) + (counter0 >> 11);
#endif
	return counter0;
}
#endif
