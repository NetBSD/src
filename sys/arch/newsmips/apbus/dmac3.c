/*	$NetBSD: dmac3.c,v 1.5 2003/04/02 04:17:50 thorpej Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/locore.h>

#include <newsmips/apbus/apbusvar.h>
#include <newsmips/apbus/dmac3reg.h>

#include <mips/cache.h>

#define DMA_BURST
#define DMA_APAD_OFF

#ifdef DMA_APAD_OFF
# define APAD_MODE	0
#else
# define APAD_MODE	DMAC3_CSR_APAD
#endif

#ifdef DMA_BURST
# define BURST_MODE	(DMAC3_CSR_DBURST | DMAC3_CSR_MBURST)
#else
# define BURST_MODE	0
#endif

struct dmac3_softc {
	struct device sc_dev;
	struct dmac3reg *sc_reg;
	vaddr_t sc_dmaaddr;
	int *sc_dmamap;
	int sc_conf;
	int sc_ctlnum;
};

int dmac3_match __P((struct device *, struct cfdata *, void *));
void dmac3_attach __P((struct device *, struct device *, void *));

paddr_t kvtophys __P((vaddr_t));

CFATTACH_DECL(dmac, sizeof(struct dmac3_softc),
    dmac3_match, dmac3_attach, NULL, NULL);

int
dmac3_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct apbus_attach_args *apa = aux;

	if (strcmp(apa->apa_name, "dmac3") == 0)
		return 1;

	return 0;
}

void
dmac3_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct dmac3_softc *sc = (void *)self;
	struct apbus_attach_args *apa = aux;
	struct dmac3reg *reg;

	static paddr_t dmamap = DMAC3_PAGEMAP;
	static vaddr_t dmaaddr = 0;

	reg = (void *)apa->apa_hwbase;
	sc->sc_reg = reg;
	sc->sc_ctlnum = apa->apa_ctlnum;
	sc->sc_dmamap = (int *)dmamap;
	sc->sc_dmaaddr = dmaaddr;
	dmamap += 0x1000;
	dmaaddr += 0x200000;

	sc->sc_conf = DMAC3_CONF_PCEN | DMAC3_CONF_DCEN | DMAC3_CONF_FASTACCESS;

	dmac3_reset(sc);

	printf(" slot%d addr 0x%lx", apa->apa_slotno, apa->apa_hwbase);
	printf(": ctlnum = %d, map = %p, va = %lx",
	       apa->apa_ctlnum, sc->sc_dmamap, sc->sc_dmaaddr);
	printf("\n");
}

void *
dmac3_link(ctlnum)
	int ctlnum;
{
	struct dmac3_softc *sc;
	struct device *dv;

	for (dv = alldevs.tqh_first; dv; dv = dv->dv_list.tqe_next) {
		if (strncmp(dv->dv_xname, "dmac", 4) == 0) {
			sc = (void *)dv;
			if (sc->sc_ctlnum == ctlnum)
				return sc;
		}
	}
	return NULL;
}

void
dmac3_reset(sc)
	struct dmac3_softc *sc;
{
	struct dmac3reg *reg = sc->sc_reg;

	reg->csr = DMAC3_CSR_RESET;
	reg->csr = 0;
	reg->intr = DMAC3_INTR_EOPIE | DMAC3_INTR_INTEN;
	reg->conf = sc->sc_conf;
}

void
dmac3_start(sc, addr, len, direction)
	struct dmac3_softc *sc;
	vaddr_t addr;
	int len, direction;
{
	struct dmac3reg *reg = sc->sc_reg;
	paddr_t pa;
	vaddr_t start, end, v;
	u_int *p;

	if (reg->csr & DMAC3_CSR_ENABLE)
		dmac3_reset(sc);

	start = mips_trunc_page(addr);
	end   = mips_round_page(addr + len);
	p = sc->sc_dmamap;
	for (v = start; v < end; v += PAGE_SIZE) {
		pa = kvtophys(v);
		mips_dcache_wbinv_range(MIPS_PHYS_TO_KSEG0(pa), PAGE_SIZE);
		*p++ = 0;
		*p++ = (pa >> PGSHIFT) | 0xc0000000;
	}
	*p++ = 0;
	*p++ = 0x003fffff;

	addr &= PGOFSET;
	addr += sc->sc_dmaaddr;

	reg->len = len;
	reg->addr = addr;
	reg->intr = DMAC3_INTR_EOPIE | DMAC3_INTR_INTEN;
	reg->csr = DMAC3_CSR_ENABLE | direction | BURST_MODE | APAD_MODE;
}

int
dmac3_intr(v)
	void *v;
{
	struct dmac3_softc *sc = v;
	struct dmac3reg *reg = sc->sc_reg;
	int intr, conf, rv = 1;

	intr = reg->intr;
	if ((intr & DMAC3_INTR_INT) == 0)
		return 0;

	/* clear interrupt */
	conf = reg->conf;
	reg->conf = conf;
	reg->intr = intr;

	if (intr & DMAC3_INTR_PERR) {
		printf("%s: intr = 0x%x\n", sc->sc_dev.dv_xname, intr);
		rv = -1;
	}

	if (conf & (DMAC3_CONF_IPER | DMAC3_CONF_MPER | DMAC3_CONF_DERR)) {
		printf("%s: conf = 0x%x\n", sc->sc_dev.dv_xname, conf);
		if (conf & DMAC3_CONF_DERR) {
			printf("DMA address = 0x%x\n", reg->addr);
			printf("resetting DMA...\n");
			dmac3_reset(sc);
		}
	}

	return rv;
}

void
dmac3_misc(sc, cmd)
	struct dmac3_softc *sc;
	int cmd;
{
	struct dmac3reg *reg = sc->sc_reg;
	int conf;

	conf = DMAC3_CONF_PCEN | DMAC3_CONF_DCEN | cmd;
	sc->sc_conf = conf;
	reg->conf = conf;
}
