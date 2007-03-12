/*	$NetBSD: plumohci.c,v 1.10.26.1 2007/03/12 05:48:04 rmind Exp $ */

/*-
 * Copyright (c) 2000 UCHIYAMA Yasushi
 * Copyright (c) 1999 MAEKAWA Masahide <bishop@rr.iij4u.or.jp>
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

/*
 * USB Open Host Controller driver.
 *
 * OHCI spec: ftp://ftp.compaq.com/pub/supportinformation/papers/hcir1_0a.exe
 * USB spec: http://www.usb.org/developers/data/usb11.pdf
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: plumohci.c,v 1.10.26.1 2007/03/12 05:48:04 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/queue.h>

/* busdma */
#include <sys/mbuf.h>
#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/bus_dma_hpcmips.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

#include <dev/usb/ohcireg.h>
#include <dev/usb/ohcivar.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/dev/plumvar.h>
#include <hpcmips/dev/plumicuvar.h>
#include <hpcmips/dev/plumpowervar.h>
#include <hpcmips/dev/plumohcireg.h>

int plumohci_match(struct device *, struct cfdata *, void *);
void plumohci_attach(struct device *, struct device *, void *);
int plumohci_intr(void *);

void __plumohci_dmamap_sync(bus_dma_tag_t, bus_dmamap_t,
    bus_addr_t, bus_size_t, int);
int __plumohci_dmamem_alloc(bus_dma_tag_t, bus_size_t, bus_size_t,
    bus_size_t, bus_dma_segment_t *, int, int *, int);
void __plumohci_dmamem_free(bus_dma_tag_t, bus_dma_segment_t *, int);
int __plumohci_dmamem_map(bus_dma_tag_t, bus_dma_segment_t *,
    int, size_t, void **, int);
void __plumohci_dmamem_unmap(bus_dma_tag_t, void *, size_t);

struct bus_dma_tag_hpcmips plumohci_bus_dma_tag = {
	{
		NULL,
		{
			_hpcmips_bd_map_create,
			_hpcmips_bd_map_destroy,
			_hpcmips_bd_map_load,
			_hpcmips_bd_map_load_mbuf,
			_hpcmips_bd_map_load_uio,
			_hpcmips_bd_map_load_raw,
			_hpcmips_bd_map_unload,
			__plumohci_dmamap_sync,
			__plumohci_dmamem_alloc,
			__plumohci_dmamem_free,
			__plumohci_dmamem_map,
			__plumohci_dmamem_unmap,
			_hpcmips_bd_mem_mmap,
		},
	},
	NULL,
};

struct plumohci_shm {
	bus_space_handle_t ps_bsh;
	paddr_t	ps_paddr;
	void *	ps_caddr;
	size_t	ps_size;
	LIST_ENTRY(plumohci_shm) ps_link;
};

struct plumohci_softc {
	struct ohci_softc sc;
	void *sc_ih;
	void *sc_wakeih;		

	LIST_HEAD(, plumohci_shm) sc_shm_head;
};

CFATTACH_DECL(plumohci, sizeof(struct plumohci_softc),
    plumohci_match, plumohci_attach, NULL, NULL);

int
plumohci_match(struct device *parent, struct cfdata *match, void *aux)
{
	/* PLUM2 builtin OHCI module */

	return (1);
}

void
plumohci_attach(struct device *parent, struct device *self, void *aux)
{
	struct plumohci_softc *sc = (struct plumohci_softc *)self;
	struct plum_attach_args *pa = aux;
	usbd_status r;

	sc->sc.iot = pa->pa_iot;
	sc->sc.sc_bus.dmatag = &plumohci_bus_dma_tag.bdt;
	plumohci_bus_dma_tag._dmamap_chipset_v = sc;

	/* Map I/O space */
	if (bus_space_map(sc->sc.iot, PLUM_OHCI_REGBASE, OHCI_PAGE_SIZE, 
	    0, &sc->sc.ioh)) {
		printf(": cannot map mem space\n");
		return;
	}

	/* power up */
	/* 
	 * in the case of PLUM2, UHOSTC uses the VRAM as the shared RAM
	 * so establish power/clock of Video contoroller
	 */
	plum_power_establish(pa->pa_pc, PLUM_PWR_EXTPW1);
	plum_power_establish(pa->pa_pc, PLUM_PWR_USB);

	/* Disable interrupts, so we don't can any spurious ones. */
	bus_space_write_4(sc->sc.iot, sc->sc.ioh, OHCI_INTERRUPT_DISABLE,
	    OHCI_ALL_INTRS);

	/* master enable */
	sc->sc_ih = plum_intr_establish(pa->pa_pc, PLUM_INT_USB, IST_EDGE,
	    IPL_USB, ohci_intr, sc);
#if 0
	/* 
	 *  enable the clock restart request interrupt 
	 *  (for USBSUSPEND state)
	 */
	sc->sc_wakeih = plum_intr_establish(pa->pa_pc, PLUM_INT_USBWAKE, 
	    IST_EDGE, IPL_USB, 
	    plumohci_intr, sc);
#endif
	/*
	 * Shared memory list.
	 */
	LIST_INIT(&sc->sc_shm_head);

	printf("\n");

	r = ohci_init(&sc->sc);

	if (r != USBD_NORMAL_COMPLETION) {
		printf(": init failed, error=%d\n", r);

		plum_intr_disestablish(pa->pa_pc, sc->sc_ih);
		plum_intr_disestablish(pa->pa_pc, sc->sc_wakeih);

		return;
	}

	/* Attach usb device. */
	sc->sc.sc_child = config_found((void *) sc, &sc->sc.sc_bus,
	    usbctlprint);
}

int
plumohci_intr(void *arg)
{
	printf("Plum2 OHCI: wakeup intr\n");
	return 0;
}

/*
 * Plum2 OHCI specific busdma routines.
 *	Plum2 OHCI shared buffer can't allocate on memory 
 *	but V-RAM (busspace).
 */

void
__plumohci_dmamap_sync(bus_dma_tag_t tx, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	struct bus_dma_tag_hpcmips *t = (struct bus_dma_tag_hpcmips *)tx;
	struct plumohci_softc *sc = t->_dmamap_chipset_v;

	/*
	 * Flush the write buffer allocated on the V-RAM.
	 * Accessing any host controller register flushs write buffer
	 */
	(void)bus_space_read_4(sc->sc.iot, sc->sc.ioh, OHCI_REVISION);
}

int
__plumohci_dmamem_alloc(bus_dma_tag_t tx, bus_size_t size,
    bus_size_t alignment, bus_size_t boundary, bus_dma_segment_t *segs,
    int nsegs, int *rsegs, int flags)
{
	struct bus_dma_tag_hpcmips *t = (struct bus_dma_tag_hpcmips *)tx;
	struct plumohci_softc *sc = t->_dmamap_chipset_v;
	struct plumohci_shm *ps;
	bus_space_handle_t bsh;
	paddr_t paddr;
	void *caddr;
	int error;

	size = round_page(size);

	/*
	 * Allocate buffer from V-RAM area.
	 */
	error = bus_space_alloc(sc->sc.iot, PLUM_OHCI_SHMEMBASE,
	    PLUM_OHCI_SHMEMBASE + PLUM_OHCI_SHMEMSIZE - 1,
	    size, OHCI_PAGE_SIZE, OHCI_PAGE_SIZE, 0,
	    (bus_addr_t *)(void *)&caddr, &bsh);
	if (error)
		return (1);

	pmap_extract(pmap_kernel(), (vaddr_t)caddr, &paddr);

	ps = malloc(sizeof(struct plumohci_shm), M_DEVBUF, M_NOWAIT);
	if (ps == 0)
		return (1);

	ps->ps_bsh = bsh;
	ps->ps_size = segs[0].ds_len = size;
	ps->ps_paddr = segs[0].ds_addr = paddr;
	ps->ps_caddr = caddr;

	LIST_INSERT_HEAD(&sc->sc_shm_head, ps, ps_link);

	*rsegs = 1;

	return (0);
}

void
__plumohci_dmamem_free(bus_dma_tag_t tx, bus_dma_segment_t *segs, int nsegs)
{
	struct bus_dma_tag_hpcmips *t = (struct bus_dma_tag_hpcmips *)tx;
	struct plumohci_softc *sc = t->_dmamap_chipset_v;
	struct plumohci_shm *ps;

	for (ps = LIST_FIRST(&sc->sc_shm_head); ps;
	    ps = LIST_NEXT(ps, ps_link)) {

		if (ps->ps_paddr == segs[0].ds_addr) {
			bus_space_free(sc->sc.iot, ps->ps_bsh, ps->ps_size);
			LIST_REMOVE(ps, ps_link);
			free(ps, M_DEVBUF);

			return;
		}
	}

	panic("__plumohci_dmamem_free: can't find corresponding handle.");
	/* NOTREACHED */
}

int
__plumohci_dmamem_map(bus_dma_tag_t tx, bus_dma_segment_t *segs, int nsegs,
    size_t size, void **kvap, int flags)
{
	struct bus_dma_tag_hpcmips *t = (struct bus_dma_tag_hpcmips *)tx;
	struct plumohci_softc *sc = t->_dmamap_chipset_v;
	struct plumohci_shm *ps;

	for (ps = LIST_FIRST(&sc->sc_shm_head); ps;
	    ps = LIST_NEXT(ps, ps_link)) {
		if (ps->ps_paddr == segs[0].ds_addr) {

			*kvap = ps->ps_caddr;

			return (0);
		}
	}

	return (1);
}

void
__plumohci_dmamem_unmap(bus_dma_tag_t t, void *kva, size_t size)
{
	/* nothing to do */
}
