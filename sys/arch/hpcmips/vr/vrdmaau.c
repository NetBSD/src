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

#include <machine/bus.h>

#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/dmaaureg.h>

#ifdef VRDMAAU_DEBUG
int vrdmaau_debug = VRDMAAU_DEBUG;
#define DPRINTFN(n,x) if (vrdmaau_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)
#endif

struct vrdmaau_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct vrdmaau_chipset_tag	sc_chipset;
};

int vrdmaau_match(struct device *, struct cfdata *, void *);
void vrdmaau_attach(struct device *, struct device *, void *);

struct cfattach vrdmaau_ca = {
	sizeof(struct vrdmaau_softc), vrdmaau_match, vrdmaau_attach
};

int vrdmaau_set_aiuin(vrdmaau_chipset_tag_t, void *);
int vrdmaau_set_aiuout(vrdmaau_chipset_tag_t, void *);
int vrdmaau_set_fir(vrdmaau_chipset_tag_t, void *);
static int vrdmaau_phy_addr(struct vrdmaau_softc *, void *, u_int32_t *);

int
vrdmaau_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 2; /* 1st attach group of vrip */
}

void
vrdmaau_attach(struct device *parent, struct device *self, void *aux)
{
	struct vrip_attach_args *va = aux;
	struct vrdmaau_softc *sc = (void*)self;

	sc->sc_iot = va->va_iot;
	sc->sc_chipset.ac_sc = sc;
	sc->sc_chipset.ac_set_aiuin = vrdmaau_set_aiuin;
	sc->sc_chipset.ac_set_aiuout = vrdmaau_set_aiuout;
	sc->sc_chipset.ac_set_fir = vrdmaau_set_fir;

	if (bus_space_map(sc->sc_iot, va->va_addr, va->va_size,
			  0 /* no flags */, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}
	printf("\n");
	vrip_register_dmaau(va->va_vc, &sc->sc_chipset);
}

int
vrdmaau_set_aiuin(vrdmaau_chipset_tag_t ac, void *addr)
{
	struct vrdmaau_softc *sc = ac->ac_sc;
	u_int32_t phy;
	int err;

	DPRINTFN(1, ("vrdmaau_set_aiuin: address %p\n", addr));

	if ((err = vrdmaau_phy_addr(sc, addr, &phy)))
		return err;

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, AIUIBAH_REG_W, phy >> 16);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, AIUIBAL_REG_W, phy & 0xffff);
	return 0;
}

int
vrdmaau_set_aiuout(vrdmaau_chipset_tag_t ac, void *addr)
{
	struct vrdmaau_softc *sc = ac->ac_sc;
	u_int32_t phy;
	int err;

	DPRINTFN(1, ("vrdmaau_set_aiuout: address %p\n", addr));

	if ((err = vrdmaau_phy_addr(sc, addr, &phy)))
		return err;

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, AIUOBAH_REG_W, phy >> 16);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, AIUOBAL_REG_W, phy & 0xffff);
	return 0;
}

int
vrdmaau_set_fir(vrdmaau_chipset_tag_t ac, void *addr)
{
	struct vrdmaau_softc *sc = ac->ac_sc;
	u_int32_t phy;
	int err;

	DPRINTFN(1, ("vrdmaau_set_fir: address %p\n", addr));

	if ((err = vrdmaau_phy_addr(sc, addr, &phy)))
		return err;

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, FIRBAH_REG_W, phy >> 16);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, FIRBAL_REG_W, phy & 0xffff);
	return 0;
}

static int
vrdmaau_phy_addr(struct vrdmaau_softc *sc, void *addr, u_int32_t *phy)
{
	DPRINTFN(1, ("vrdmaau_phy_addr\n"));

	if (addr >= (void *)MIPS_KSEG0_START &&
	    addr < (void *)MIPS_KSEG1_START)
		*phy = MIPS_KSEG0_TO_PHYS(addr);
	else if (addr >= (void *)MIPS_KSEG1_START &&
		 addr < (void *)MIPS_KSEG2_START)
		*phy = MIPS_KSEG1_TO_PHYS(addr);
	else {
		DPRINTFN(0, ("vrdmaau_map_addr: invalid address %p\n", addr));
		return EFAULT;
	}

#ifdef DIAGNOSTIC
	if ((*phy & (VRDMAAU_ALIGNMENT - 1)) ||
	    *phy >= VRDMAAU_BOUNCE_THRESHOLD ) {
		printf("%s: vrdmaau_phy_addr: invalid address %p\n",
		       sc->sc_dev.dv_xname, addr);
		return EINVAL;
	}
#endif
	return 0;
}
