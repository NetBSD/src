/*	$NetBSD: vme_pcc.c,v 1.7.2.1 2000/06/22 17:01:44 minoura Exp $	*/

/*-
 * Copyright (c) 1996-2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * VME support specific to the Type 1 VMEchip found on the
 * MVME-147.
 *
 * For a manual on the MVME-147, call: 408.991.8634.  (Yes, this
 * is the Sunnyvale sales office.)
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kcore.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

#include <mvme68k/mvme68k/isr.h>

#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>
#include <mvme68k/dev/vme_pccreg.h>
#include <mvme68k/dev/vme_pccvar.h>


int vme_pcc_match __P((struct device *, struct cfdata *, void *));
void vme_pcc_attach __P((struct device *, struct device *, void *));

struct cfattach vmepcc_ca = {
	sizeof(struct vme_pcc_softc), vme_pcc_match, vme_pcc_attach
};

extern struct cfdriver vmepcc_cd;

extern phys_ram_seg_t mem_clusters[];
static int vme_pcc_attached;

#ifdef DIAGNOSTIC
const char *_vme1_mod_string __P((vme_addr_t, vme_size_t,
				  vme_am_t, vme_datasize_t));
#endif

/*
 * Describe the VMEbus ranges available from the MVME147
 */
struct vme_pcc_range {
	vme_am_t pr_am;			/* Address Modifier for this range */
	vme_datasize_t pr_datasize;	/* Usable Data Sizes (D8, D16, * D32) */
	vme_addr_t pr_locstart;		/* Local-bus start address of range */
	vme_addr_t pr_start;		/* VMEbus start address of range */
	vme_addr_t pr_end;		/* VMEbus end address of range */
};

static struct vme_pcc_range vme_pcc_ranges[] = {
	{VME_AM_MBO | VME_AM_A24 | VME_AM_DATA | VME_AM_SUPER,
		VME_D32 | VME_D16 | VME_D8,
		VME1_A24D32_LOC_START,
		VME1_A24D32_START,
		VME1_A24D32_END},

	{VME_AM_MBO | VME_AM_A32 | VME_AM_DATA | VME_AM_SUPER,
		VME_D32 | VME_D16 | VME_D8,
		VME1_A32D32_LOC_START,
		VME1_A32D32_START,
		VME1_A32D32_END},

	{VME_AM_MBO | VME_AM_A24 | VME_AM_DATA | VME_AM_SUPER,
		VME_D16 | VME_D8,
		VME1_A24D16_LOC_START,
		VME1_A24D16_START,
		VME1_A24D16_END},

	{VME_AM_MBO | VME_AM_A32 | VME_AM_DATA | VME_AM_SUPER,
		VME_D16 | VME_D8,
		VME1_A32D16_LOC_START,
		VME1_A32D16_START,
		VME1_A32D16_END},

	{VME_AM_MBO | VME_AM_A16 | VME_AM_DATA | VME_AM_SUPER,
		VME_D16 | VME_D8,
		VME1_A16D16_LOC_START,
		VME1_A16D16_START,
		VME1_A16D16_END}
};
#define VME1_NRANGES	(sizeof(vme_pcc_ranges)/sizeof(struct vme_pcc_range))


/* ARGSUSED */
int
vme_pcc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcc_attach_args *pa;

	pa = aux;

	/* Only one VME chip, please. */
	if (vme_pcc_attached)
		return (0);

	if (strcmp(pa->pa_name, vmepcc_cd.cd_name))
		return (0);

	return (1);
}

void
vme_pcc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct pcc_attach_args *pa;
	struct vme_pcc_softc *sc;
	struct vmebus_attach_args vaa;
	u_int8_t reg;
	int i;

	sc = (struct vme_pcc_softc *) self;
	pa = aux;

	sc->sc_dmat = pa->pa_dmat;
	sc->sc_bust = pa->pa_bust;
	sc->sc_vmet = MVME68K_VME_BUS_SPACE;

	bus_space_map(sc->sc_bust, pa->pa_offset, VME1REG_SIZE, 0,
	    &sc->sc_bush);

	/* Initialize the chip. */
	reg = vme1_reg_read(sc, VME1REG_SCON) & ~VME1_SCON_SYSFAIL;
	vme1_reg_write(sc, VME1REG_SCON, reg);

	printf(": Type 1 VMEchip, scon jumper %s\n",
	    (reg & VME1_SCON_SWITCH) ? "enabled" : "disabled");

	sc->sc_vct.cookie = self;
	sc->sc_vct.vct_probe = _vme_pcc_probe;
	sc->sc_vct.vct_map = _vme_pcc_map;
	sc->sc_vct.vct_unmap = _vme_pcc_unmap;
	sc->sc_vct.vct_int_map = _vme_pcc_intmap;
	sc->sc_vct.vct_int_evcnt = _vme_pcc_intr_evcnt;
	sc->sc_vct.vct_int_establish = _vme_pcc_intr_establish;
	sc->sc_vct.vct_int_disestablish = _vme_pcc_intr_disestablish;
	sc->sc_vct.vct_dmamap_create = _vme_pcc_dmamap_create;
	sc->sc_vct.vct_dmamap_destroy = _vme_pcc_dmamap_destroy;
	sc->sc_vct.vct_dmamem_alloc = _vme_pcc_dmamem_alloc;
	sc->sc_vct.vct_dmamem_free = _vme_pcc_dmamem_free;

	/*
	 * Adjust the start address of the first range in vme_pcc_ranges[]
	 * according to how much onboard memory exists. Disable the first
	 * range if onboard memory >= 16Mb, and adjust the start of the
	 * second range (A32D32).
	 */
	vme_pcc_ranges[0].pr_start = (vme_addr_t) mem_clusters[0].size;
	if (mem_clusters[0].size >= 0x01000000) {
		vme_pcc_ranges[0].pr_am = (vme_am_t) - 1;
		vme_pcc_ranges[1].pr_start +=
		    (vme_addr_t) (mem_clusters[0].size - 0x01000000);
	}

#ifdef DIAGNOSTIC
	for (i = 0; i < VME1_NRANGES; i++) {
		vme_addr_t mask;

		switch (vme_pcc_ranges[i].pr_am & VME_AM_ADRSIZEMASK) {
		case VME_AM_A32:
			mask = 0xffffffffu;
			break;

		case VME_AM_A24:
			mask = 0x00ffffffu;
			break;

		case VME_AM_A16:
			mask = 0x0000ffffu;
			break;

		default:
			printf("%s: Map#%d: disabled\n",
			    sc->sc_dev.dv_xname, i);
			continue;
		}

		printf("%s: Map#%d: 0x%08x -> %s\n", sc->sc_dev.dv_xname, i,
		    vme_pcc_ranges[i].pr_locstart + vme_pcc_ranges[i].pr_start,
		    _vme1_mod_string(vme_pcc_ranges[i].pr_start & mask,
			(vme_pcc_ranges[i].pr_end -
			    vme_pcc_ranges[i].pr_start) + 1,
			vme_pcc_ranges[i].pr_am,
			vme_pcc_ranges[i].pr_datasize));
	}
#endif

	vaa.va_vct = &(sc->sc_vct);
	vaa.va_bdt = sc->sc_dmat;
	vaa.va_slaveconfig = NULL;

	vme_pcc_attached = 1;

	/* Attach the MI VMEbus glue. */
	config_found(self, &vaa, 0);
}

/* ARGSUSED */
int
_vme_pcc_map(vsc, vmeaddr, len, am, datasize, swap, tag, handle, resc)
	void *vsc;
	vme_addr_t vmeaddr;
	vme_size_t len;
	vme_am_t am;
	vme_datasize_t datasize;
	vme_swap_t swap;
	bus_space_tag_t *tag;
	bus_space_handle_t *handle;
	vme_mapresc_t *resc;
{
	struct vme_pcc_softc *sc;
	struct vme_pcc_mapresc_t *pm;
	struct vme_pcc_range *pr;
	vme_addr_t end;
	paddr_t paddr;
	int rv;
	int i;

	sc = vsc;
	end = (vmeaddr + len) - 1;
	paddr = 0;

	for (i = 0, pr = &vme_pcc_ranges[0]; i < VME1_NRANGES; i++, pr++) {
		/* Ignore if range is disabled */
		if (pr->pr_am == (vme_am_t) - 1)
			continue;

		/*
		 * Accept the range if it matches the constraints
		 */
		if (am == pr->pr_am &&
		    datasize <= pr->pr_datasize &&
		    vmeaddr >= pr->pr_start && end <= pr->pr_end) {
			/*
			 * We have a match.
			 */
			paddr = pr->pr_locstart + vmeaddr;
			break;
		}
	}

	if (paddr == 0) {
#ifdef DIAGNOSTIC
		printf("%s: Unable to map %s\n", sc->sc_dev.dv_xname,
		    _vme1_mod_string(vmeaddr, len, am, datasize));
#endif
		return (ENOMEM);
	}
	if ((rv = bus_space_map(sc->sc_vmet, paddr, len, 0, handle)) != 0)
		return (rv);

	if ((pm = malloc(sizeof(*pm), M_DEVBUF, M_NOWAIT)) == NULL) {
		bus_space_unmap(sc->sc_vmet, *handle, len);
		return (ENOMEM);
	}
	*tag = sc->sc_vmet;
	pm->pm_am = am;
	pm->pm_datasize = datasize;
	pm->pm_addr = vmeaddr;
	pm->pm_size = len;
	pm->pm_handle = *handle;
	*resc = (vme_mapresc_t *) pm;

	return (0);
}

void
_vme_pcc_unmap(vsc, resc)
	void *vsc;
	vme_mapresc_t resc;
{
	struct vme_pcc_softc *sc;
	struct vme_pcc_mapresc_t *pm;

	sc = (struct vme_pcc_softc *) vsc;
	pm = (struct vme_pcc_mapresc_t *) resc;

	bus_space_unmap(sc->sc_vmet, pm->pm_handle, pm->pm_size);

	free(pm, M_DEVBUF);
}

int
_vme_pcc_probe(vsc, vmeaddr, len, am, datasize, callback, arg)
	void *vsc;
	vme_addr_t vmeaddr;
	vme_size_t len;
	vme_am_t am;
	vme_datasize_t datasize;
	int (*callback) __P((void *, bus_space_tag_t, bus_space_handle_t));
	void *arg;
{
	bus_space_tag_t tag;
	bus_space_handle_t handle;
	vme_mapresc_t resc;
	int rv;

	rv = _vme_pcc_map(vsc, vmeaddr, len, am, datasize,
	    0, &tag, &handle, &resc);
	if (rv)
		return (rv);

	if (callback)
		rv = (*callback) (arg, tag, handle);
	else {
		/*
		 * FIXME: datasize is fixed by hardware, so using badaddr() in
		 * this way may cause several accesses to each VMEbus address.
		 * Also, using 'handle' in this way is a bit presumptuous...
		 */
		rv = badaddr((caddr_t) handle, (int) len) ? EIO : 0;
	}

	_vme_pcc_unmap(vsc, resc);

	return (rv);
}

/* ARGSUSED */
int
_vme_pcc_intmap(vsc, level, vector, handlep)
	void *vsc;
	int level, vector;
	vme_intr_handle_t *handlep;
{

	if (level < 1 || level > 7 || vector < 0x80 || vector > 0xff)
		return (EINVAL);

	/* This is rather gross */
	*handlep = (void *) (int) ((level << 8) | vector);

	return (0);
}

const struct evcnt *
_vme_pcc_intr_evcnt(vsc, handle)
	void *vsc;
	vme_intr_handle_t handle;
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
_vme_pcc_intr_establish(vsc, handle, prior, func, arg)
	void *vsc;
	vme_intr_handle_t handle;
	int prior;
	int (*func) __P((void *));
	void *arg;
{
	struct vme_pcc_softc *sc;
	int level, vector;

	sc = vsc;
	level = ((int) handle) >> 8;
	vector = ((int) handle) & 0xff;

	isrlink_vectored(func, arg, level, vector);
	sc->sc_irqref[level]++;

	/*
	 * There had better not be another VMEbus master responding
	 * to this interrupt level...
	 */
	vme1_reg_write(sc, VME1REG_IRQEN,
	    vme1_reg_read(sc, VME1REG_IRQEN) | VME1_IRQ_VME(level));

	return ((void *) handle);
}

void
_vme_pcc_intr_disestablish(vsc, handle)
	void *vsc;
	vme_intr_handle_t handle;
{
	struct vme_pcc_softc *sc;
	int level, vector;

	sc = (struct vme_pcc_softc *) vsc;
	level = ((int) handle) >> 8;
	vector = ((int) handle) & 0xff;

	isrunlink_vectored(vector);

	/* Disable VME IRQ if possible. */
	switch (sc->sc_irqref[level]) {
	case 0:
		printf("vme_pcc_intr_disestablish: nothing using IRQ %d\n",
		    level);
		panic("vme_pcc_intr_disestablish");
		/* NOTREACHED */

	case 1:
		vme1_reg_write(sc, VME1REG_IRQEN,
		    vme1_reg_read(sc, VME1REG_IRQEN) & ~VME1_IRQ_VME(level));
		/* FALLTHROUGH */

	default:
		sc->sc_irqref[level]--;
	}
}

/* ARGSUSED */
int
_vme_pcc_dmamap_create(vsc, len, am, datasize, swap, nsegs,
    segsz, bound, flags, mapp)
	void *vsc;
	vme_size_t len;
	vme_am_t am;
	vme_datasize_t datasize;
	vme_swap_t swap;
	int nsegs;
	vme_size_t segsz;
	vme_addr_t bound;
	int flags;
	bus_dmamap_t *mapp;
{

	return (EINVAL);
}

/* ARGSUSED */
void
_vme_pcc_dmamap_destroy(vsc, map)
	void *vsc;
	bus_dmamap_t map;
{
}

/* ARGSUSED */
int
_vme_pcc_dmamem_alloc(vsc, len, am, datasizes, swap,
    segs, nsegs, rsegs, flags)
	void *vsc;
	vme_size_t len;
	vme_am_t am;
	vme_datasize_t datasizes;
	vme_swap_t swap;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{

	return (EINVAL);
}

/* ARGSUSED */
void
_vme_pcc_dmamem_free(vsc, segs, nsegs)
	void *vsc;
	bus_dma_segment_t *segs;
	int nsegs;
{
}

#ifdef DIAGNOSTIC
const char *
_vme1_mod_string(addr, len, am, ds)
	vme_addr_t addr;
	vme_size_t len;
	vme_am_t am;
	vme_datasize_t ds;
{
	static const char *mode[] = {"BLT64)", "DATA)", "PROG)", "BLT32)"};
	static const char *dsiz[] = {"(", "(D8,", "(D16,", "(D16-D8,",
	"(D32,", "(D32,D8,", "(D32-D16,", "(D32-D8,"};
	static char mstring[40];
	char *fmt;

	switch (am & VME_AM_ADRSIZEMASK) {
	case VME_AM_A32:
		fmt = "A32:%08x-%08x ";
		break;

	case VME_AM_A24:
		fmt = "A24:%06x-%06x ";
		break;

	case VME_AM_A16:
		fmt = "A16:%04x-%04x ";
		break;

	case VME_AM_USERDEF:
		fmt = "USR:%08x-%08x ";
		break;
	}

	sprintf(mstring, fmt, addr, addr + len - 1);
	strcat(mstring, dsiz[ds & 0x7]);
	strcat(mstring, ((am & VME_AM_PRIVMASK) == VME_AM_USER) ?
	    "USER," : "SUPER,");
	strcat(mstring, mode[am & VME_AM_MODEMASK]);

	return (mstring);
}
#endif
