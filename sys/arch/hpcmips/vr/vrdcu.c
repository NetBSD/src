/*
 * Copyright (c) 2001 HAMAJIMA Katsuomi. All rights reserved.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/bus_dma_hpcmips.h>

#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/dcureg.h>

#ifdef VRDCU_DEBUG
int vrdcu_debug = VRDCU_DEBUG;
#define DPRINTFN(n,x) if (vrdcu_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)
#endif

struct vrdcu_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct vrdcu_chipset_tag	sc_chipset;
	int			sc_status;	/* DMA status */
};

int vrdcu_match(struct device *, struct cfdata *, void *);
void vrdcu_attach(struct device *, struct device *, void *);

struct cfattach vrdcu_ca = {
	sizeof(struct vrdcu_softc), vrdcu_match, vrdcu_attach
};

int vrdcu_enable_aiuin(vrdcu_chipset_tag_t);
int vrdcu_enable_aiuout(vrdcu_chipset_tag_t);
int vrdcu_enable_fir(vrdcu_chipset_tag_t);
void vrdcu_disable(vrdcu_chipset_tag_t);
void vrdcu_fir_direction(vrdcu_chipset_tag_t, int);
int _vrdcu_dmamem_alloc(bus_dma_tag_t, bus_size_t, bus_size_t,
			bus_size_t, bus_dma_segment_t *, int, int *, int);

struct bus_dma_tag vrdcu_bus_dma_tag = {
	NULL,
	{
		_hpcmips_bd_map_create,
		_hpcmips_bd_map_destroy,
		_hpcmips_bd_map_load,
		_hpcmips_bd_map_load_mbuf,
		_hpcmips_bd_map_load_uio,
		_hpcmips_bd_map_load_raw,
		_hpcmips_bd_map_unload,
		_hpcmips_bd_map_sync,
		_vrdcu_dmamem_alloc,
		_hpcmips_bd_mem_free,
		_hpcmips_bd_mem_map,
		_hpcmips_bd_mem_unmap,
		_hpcmips_bd_mem_mmap,
	},
};

int
vrdcu_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 2; /* 1st attach group of vrip */
}

void
vrdcu_attach(struct device *parent, struct device *self, void *aux)
{
	struct vrip_attach_args *va = aux;
	struct vrdcu_softc *sc = (void*)self;

	sc->sc_iot = va->va_iot;
	sc->sc_chipset.dc_sc = sc;
	sc->sc_chipset.dc_enable_aiuin = vrdcu_enable_aiuin;
	sc->sc_chipset.dc_enable_aiuout = vrdcu_enable_aiuout;
	sc->sc_chipset.dc_enable_fir = vrdcu_enable_fir;
	sc->sc_chipset.dc_disable = vrdcu_disable;
	sc->sc_chipset.dc_fir_direction = vrdcu_fir_direction;

	if (bus_space_map(sc->sc_iot, va->va_addr, va->va_size,
			  0 /* no flags */, &sc->sc_ioh)) {
		printf("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}
	printf("\n");
	vrip_register_dcu(va->va_vc, &sc->sc_chipset);

	sc->sc_status = DMASDS;
	/* reset DCU */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, DMARST_REG_W, DMARST);
}

int
vrdcu_enable_aiuin(vrdcu_chipset_tag_t dc)
{
	struct vrdcu_softc *sc = dc->dc_sc;
	int mask;

	DPRINTFN(1, ("vrdcu_enable_aiuin\n"));

	if (sc->sc_status){
		mask = bus_space_read_2(sc->sc_iot, sc->sc_ioh, DMAMSK_REG_W);
		if (mask & DMAMSKAIN) {
			DPRINTFN(0, ("vrdcu_enable_aiuin: already enabled\n"));
			return 0;
		} else {
			DPRINTFN(0, ("vrdcu_enable_aiuin: device busy\n"));
			return EBUSY;
		}
	}
	sc->sc_status = DMASEN;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, DMAMSK_REG_W, DMAMSKAIN);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, DMASEN_REG_W, sc->sc_status);
	return 0;
}

int
vrdcu_enable_aiuout(vrdcu_chipset_tag_t dc)
{
	struct vrdcu_softc *sc = dc->dc_sc;
	int mask;

	DPRINTFN(1, ("vrdcu_enable_aiuout\n"));

	if (sc->sc_status){
		mask = bus_space_read_2(sc->sc_iot, sc->sc_ioh, DMAMSK_REG_W);
		if (mask & DMAMSKAOUT) {
			DPRINTFN(0, ("vrdcu_enable_aiuout: already enabled\n"));
			return 0;
		} else {
			DPRINTFN(0, ("vrdcu_enable_aiuout: device busy\n"));
			return EBUSY;
		}
	}
	sc->sc_status = DMASEN;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, DMAMSK_REG_W, DMAMSKAOUT);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, DMASEN_REG_W, sc->sc_status);
	return 0;
}

int
vrdcu_enable_fir(vrdcu_chipset_tag_t dc)
{
	struct vrdcu_softc *sc = dc->dc_sc;
	int mask;

	DPRINTFN(1, ("vrdcu_enable_fir\n"));

	if (sc->sc_status){
		mask = bus_space_read_2(sc->sc_iot, sc->sc_ioh, DMAMSK_REG_W);
		if (mask & DMAMSKFOUT) {
			DPRINTFN(0, ("vrdcu_enable_fir: already enabled\n"));
			return 0;
		} else {
			DPRINTFN(0, ("vrdcu_enable_fir: device busy\n"));
			return EBUSY;
		}
	}
	sc->sc_status = DMASEN;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, DMAMSK_REG_W, DMAMSKFOUT);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, DMASEN_REG_W, sc->sc_status);
	return 0;
}

void
vrdcu_disable(vrdcu_chipset_tag_t dc)
{
	struct vrdcu_softc *sc = dc->dc_sc;

	DPRINTFN(1, ("vrdcu_disable\n"));

	sc->sc_status = DMASDS;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, DMASEN_REG_W, sc->sc_status);
}

void
vrdcu_fir_direction(vrdcu_chipset_tag_t dc, int dir)
{
	struct vrdcu_softc *sc = dc->dc_sc;

	DPRINTFN(1, ("vrdcu_fir_direction: dir %d\n", dir));

	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			  DMATD_REG_W, dir & DMATDMASK);
}

int
_vrdcu_dmamem_alloc(bus_dma_tag_t t, bus_size_t size, bus_size_t alignment,
		    bus_size_t boundary, bus_dma_segment_t *segs,
		    int nsegs, int *rsegs, int flags)
{
	extern paddr_t avail_start, avail_end;		/* XXX */
	paddr_t high;

	DPRINTFN(1, ("_vrdcu_dmamem_alloc\n"));

	high = (avail_end < VRDMAAU_BOUNCE_THRESHOLD ?
		avail_end : VRDMAAU_BOUNCE_THRESHOLD) - PAGE_SIZE;
	alignment = alignment > VRDMAAU_ALIGNMENT ?
		    alignment : VRDMAAU_ALIGNMENT;

	return _hpcmips_bd_mem_alloc_range(t, size, alignment, boundary,
					   segs, nsegs, rsegs, flags,
					   avail_start, high);
}
