/* $NetBSD: bba.c,v 1.11 2000/07/18 06:12:33 thorpej Exp $ */

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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

/* maxine/alpha baseboard audio (bba) */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <uvm/uvm_extern.h>	/* for PAGE_SIZE */

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/ic/am7930reg.h>
#include <dev/ic/am7930var.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicreg.h>
#include <dev/tc/ioasicvar.h>

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (am7930debug) printf x
#else
#define DPRINTF(x)
#endif  /* AUDIO_DEBUG */

#define BBA_MAX_DMA_SEGMENTS	16
#define BBA_DMABUF_SIZE		(BBA_MAX_DMA_SEGMENTS*IOASIC_DMA_BLOCKSIZE)
#define BBA_DMABUF_ALIGN	IOASIC_DMA_BLOCKSIZE
#define BBA_DMABUF_BOUNDARY	0

struct bba_mem {
        struct bba_mem *next;
	bus_addr_t addr;
	bus_size_t size;
	caddr_t kva;
};

struct bba_dma_state {
	bus_dmamap_t dmam;		/* dma map */
	int active;
	int curseg;			/* current segment in dma buffer */
	void (*intr)__P((void *));	/* higher-level audio handler */
	void *intr_arg;
};

struct bba_softc {
	struct am7930_softc sc_am7930;		/* glue to MI code */

	bus_space_tag_t sc_bst;			/* IOASIC bus tag/handle */
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	bus_space_handle_t sc_codec_bsh;	/* codec bus space handle */

	struct bba_mem *sc_mem_head;		/* list of buffers */

	struct bba_dma_state sc_tx_dma_state;
	struct bba_dma_state sc_rx_dma_state;
};

int	bba_match __P((struct device *, struct cfdata *, void *));
void	bba_attach __P((struct device *, struct device *, void *));

struct cfattach bba_ca = {
	sizeof(struct bba_softc), bba_match, bba_attach
};

/*
 * Define our interface into the am7930 MI driver.
 */

u_int8_t	bba_codec_iread __P((struct am7930_softc *, int));
u_int16_t	bba_codec_iread16 __P((struct am7930_softc *, int));
void	bba_codec_iwrite __P((struct am7930_softc *, int, u_int8_t));
void	bba_codec_iwrite16 __P((struct am7930_softc *, int, u_int16_t));
void	bba_onopen __P((struct am7930_softc *sc));
void	bba_onclose __P((struct am7930_softc *sc));
void	bba_output_conv __P((void *, u_int8_t *, int));
void	bba_input_conv __P((void *, u_int8_t *, int));

struct am7930_glue bba_glue = {
	bba_codec_iread,
	bba_codec_iwrite,
	bba_codec_iread16,
	bba_codec_iwrite16,
	bba_onopen,
	bba_onclose,
	4,
	bba_input_conv,
	bba_output_conv,
};

/*
 * Define our interface to the higher level audio driver.
 */

int	bba_round_blocksize __P((void *, int));
int	bba_halt_output __P((void *));
int	bba_halt_input __P((void *));
int	bba_getdev __P((void *, struct audio_device *));
void	*bba_allocm __P((void *, int, size_t, int, int));
void	bba_freem __P((void *, void *, int));
size_t	bba_round_buffersize __P((void *, int, size_t));
int	bba_get_props __P((void *));
paddr_t	bba_mappage __P((void *, void *, off_t, int));
int	bba_trigger_output __P((void *, void *, void *, int,
	    void (*)(void *), void *, struct audio_params *));
int	bba_trigger_input __P((void *, void *, void *, int,
	    void (*)(void *), void *, struct audio_params *));

struct audio_hw_if sa_hw_if = {
	am7930_open,
	am7930_close,
	0,
	am7930_query_encoding,
	am7930_set_params,
	bba_round_blocksize,		/* md */
	am7930_commit_settings,
	0,
	0,
	0,
	0,
	bba_halt_output,		/* md */
	bba_halt_input,			/* md */
	0,
	bba_getdev,
	0,
	am7930_set_port,
	am7930_get_port,
	am7930_query_devinfo,
	bba_allocm,			/* md */
	bba_freem,			/* md */
	bba_round_buffersize,		/* md */
	bba_mappage,
	bba_get_props,
	bba_trigger_output,		/* md */
	bba_trigger_input		/* md */
};

struct audio_device bba_device = {
	"am7930",
	"x",
	"bba"
};

int	bba_intr __P((void *));
void	bba_reset __P((struct bba_softc *, int));
void	bba_codec_dwrite __P((struct am7930_softc *, int, u_int8_t));
u_int8_t	bba_codec_dread __P((struct am7930_softc *, int));

int bba_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ioasicdev_attach_args *ia = aux;

	if (strcmp(ia->iada_modname, "isdn") != 0 &&
	    strcmp(ia->iada_modname, "AMD79c30") != 0)
		return 0;

	return 1;
}


void
bba_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ioasicdev_attach_args *ia = aux;
	struct bba_softc *sc = (struct bba_softc *)self;
	struct am7930_softc *asc = &sc->sc_am7930;

	sc->sc_bst = ((struct ioasic_softc *)parent)->sc_bst;
	sc->sc_bsh = ((struct ioasic_softc *)parent)->sc_bsh;
	sc->sc_dmat = ((struct ioasic_softc *)parent)->sc_dmat;

	/* get the bus space handle for codec */
	if (bus_space_subregion(sc->sc_bst, sc->sc_bsh,
	    ia->iada_offset, 0, &sc->sc_codec_bsh)) {
		printf("%s: unable to map device\n", asc->sc_dev.dv_xname);
		return;
	}

	printf("\n");

	bba_reset(sc,1);

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	asc->sc_glue = &bba_glue;

	/*
	 *  MI initialisation.  We will be doing DMA.
	 */
	am7930_init(asc, AUDIOAMD_DMA_MODE);

	ioasic_intr_establish(parent, ia->iada_cookie, TC_IPL_NONE,
	    bba_intr, sc);

	audio_attach_mi(&sa_hw_if, asc, &asc->sc_dev);
}


void
bba_onopen(sc)
	struct am7930_softc *sc;
{
	bba_reset((struct bba_softc *)sc, 0);
}


void
bba_onclose(sc)
	struct am7930_softc *sc;
{
	bba_halt_input((struct bba_softc *)sc);
	bba_halt_output((struct bba_softc *)sc);
}


void
bba_reset(sc, reset)
	struct bba_softc *sc;
	int reset;
{
	u_int32_t ssr;

	/* disable any DMA and reset the codec */
	ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	ssr &= ~(IOASIC_CSR_DMAEN_ISDN_T | IOASIC_CSR_DMAEN_ISDN_R);
	if (reset)
		ssr &= ~IOASIC_CSR_ISDN_ENABLE;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);
	DELAY(10);	/* 400ns required for codec to reset */

	/* initialise DMA pointers */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_DMAPTR, -1);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_NEXTPTR, -1);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_DMAPTR, -1);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_NEXTPTR, -1);

	/* take out of reset state */
	if (reset) {
		ssr |= IOASIC_CSR_ISDN_ENABLE;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);
	}

}


void *
bba_allocm(addr, direction, size, pool, flags)
	void *addr;
	int direction;
	size_t size;
	int pool, flags;
{
	struct am7930_softc *asc = addr;
	struct bba_softc *sc = addr;
	bus_dma_segment_t seg;
	int rseg;
	caddr_t kva;
	struct bba_mem *m;
	int w;
	int state = 0;

	DPRINTF(("bba_allocm: size = %d\n",size));

	w = (flags & M_NOWAIT) ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK;

	if (bus_dmamem_alloc(sc->sc_dmat, size, BBA_DMABUF_ALIGN,
	    BBA_DMABUF_BOUNDARY, &seg, 1, &rseg, w)) {
		printf("%s: can't allocate DMA buffer\n",
		    asc->sc_dev.dv_xname);
		goto bad;
	}
	state |= 1;

	if (bus_dmamem_map(sc->sc_dmat, &seg, rseg, size,
	    &kva, w | BUS_DMA_COHERENT)) {
		printf("%s: can't map DMA buffer\n", asc->sc_dev.dv_xname);
		goto bad;
	}
	state |= 2;

	m = malloc(sizeof(struct bba_mem), pool, flags);
	if (m == NULL)
		goto bad;
	m->addr = seg.ds_addr;
	m->size = seg.ds_len;
        m->kva = kva;
        m->next = sc->sc_mem_head;
        sc->sc_mem_head = m;

        return (void *)kva;

bad:
	if (state & 2)
		bus_dmamem_unmap(sc->sc_dmat, kva, size);
	if (state & 1)
		bus_dmamem_free(sc->sc_dmat, &seg, 1);
	return NULL;
}


void
bba_freem(addr, ptr, pool)
	void *addr;
	void *ptr;
	int pool;
{
	struct bba_softc *sc = addr;
        struct bba_mem **mp, *m;
	bus_dma_segment_t seg;
        caddr_t kva = (caddr_t)addr;

	for (mp = &sc->sc_mem_head; *mp && (*mp)->kva != kva;
	    mp = &(*mp)->next)
		/* nothing */ ;
	m = *mp;
	if (m == NULL) {
		printf("bba_freem: freeing unallocated memory\n");
		return;
	}
	*mp = m->next;
	bus_dmamem_unmap(sc->sc_dmat, kva, m->size);

        seg.ds_addr = m->addr;
        seg.ds_len = m->size;
	bus_dmamem_free(sc->sc_dmat, &seg, 1);
        free(m, pool);
}


size_t
bba_round_buffersize(addr, direction, size)
	void *addr;
	int direction;
	size_t size;
{
	DPRINTF(("bba_round_buffersize: size=%d\n", size));

	return (size > BBA_DMABUF_SIZE ? BBA_DMABUF_SIZE :
	    roundup(size, IOASIC_DMA_BLOCKSIZE));
}


int
bba_halt_output(addr)
	void *addr;
{
	struct bba_softc *sc = addr;
	struct bba_dma_state *d = &sc->sc_tx_dma_state;
	u_int32_t ssr;

	/* disable any DMA */
	ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_ISDN_T;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_DMAPTR, -1);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_NEXTPTR, -1);

	if (d->active) {
		bus_dmamap_unload(sc->sc_dmat, d->dmam);
		bus_dmamap_destroy(sc->sc_dmat, d->dmam);
		d->active = 0;
	}

	return 0;
}


int
bba_halt_input(addr)
	void *addr;
{
	struct bba_softc *sc = addr;
	struct bba_dma_state *d = &sc->sc_rx_dma_state;
	u_int32_t ssr;

	/* disable any DMA */
	ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_ISDN_R;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_DMAPTR, -1);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_NEXTPTR, -1);

	if (d->active) {
		bus_dmamap_unload(sc->sc_dmat, d->dmam);
		bus_dmamap_destroy(sc->sc_dmat, d->dmam);
		d->active = 0;
	}

	return 0;
}


int
bba_getdev(addr, retp)
	void *addr;
	struct audio_device *retp;
{
	*retp = bba_device;
	return 0;
}


int
bba_trigger_output(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	struct bba_softc *sc = addr;
	struct bba_dma_state *d = &sc->sc_tx_dma_state;
	u_int32_t ssr;
        tc_addr_t phys, nphys;
	int state = 0;

	DPRINTF(("bba_trigger_output: sc=%p start=%p end=%p blksize=%d intr=%p(%p)\n",
	    addr, start, end, blksize, intr, arg));

	/* disable any DMA */
	ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_ISDN_T;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);

	if (bus_dmamap_create(sc->sc_dmat, (char *)end - (char *)start,
	    BBA_MAX_DMA_SEGMENTS, IOASIC_DMA_BLOCKSIZE,
	    BBA_DMABUF_BOUNDARY, BUS_DMA_NOWAIT, &d->dmam)) {
		printf("bba_trigger_output: can't create DMA map\n");
		goto bad;
	}
	state |= 1;

	if (bus_dmamap_load(sc->sc_dmat, d->dmam, start,
	    (char *)end - (char *)start, NULL, BUS_DMA_NOWAIT)) {
	    printf("bba_trigger_output: can't load DMA map\n");
		goto bad;
	}
	state |= 2;

	d->intr = intr;
	d->intr_arg = arg;
	d->curseg = 1;

	/* get physical address of buffer start */
	phys = (tc_addr_t)d->dmam->dm_segs[0].ds_addr;
	nphys = (tc_addr_t)d->dmam->dm_segs[1].ds_addr;

	/* setup DMA pointer */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_DMAPTR,
	    IOASIC_DMA_ADDR(phys));
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_X_NEXTPTR,
	    IOASIC_DMA_ADDR(nphys));

	/* kick off DMA */
	ssr |= IOASIC_CSR_DMAEN_ISDN_T;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);

	d->active = 1;

	return 0;

bad:
	if (state & 2)
		bus_dmamap_unload(sc->sc_dmat, d->dmam);
	if (state & 1)
		bus_dmamap_destroy(sc->sc_dmat, d->dmam);
	return 1;
}


int
bba_trigger_input(addr, start, end, blksize, intr, arg, param)
	void *addr;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	struct bba_softc *sc = (struct bba_softc *)addr;
	struct bba_dma_state *d = &sc->sc_rx_dma_state;
        tc_addr_t phys, nphys;
	u_int32_t ssr;
	int state = 0;

	DPRINTF(("bba_trigger_input: sc=%p start=%p end=%p blksize=%d intr=%p(%p)\n",
	    addr, start, end, blksize, intr, arg));

	/* disable any DMA */
	ssr = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR);
	ssr &= ~IOASIC_CSR_DMAEN_ISDN_R;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);

	if (bus_dmamap_create(sc->sc_dmat, (char *)end - (char *)start,
	    BBA_MAX_DMA_SEGMENTS, IOASIC_DMA_BLOCKSIZE,
	    BBA_DMABUF_BOUNDARY, BUS_DMA_NOWAIT, &d->dmam)) {
		printf("bba_trigger_input: can't create DMA map\n");
		goto bad;
	}
	state |= 1;

	if (bus_dmamap_load(sc->sc_dmat, d->dmam, start,
	    (char *)end - (char *)start, NULL, BUS_DMA_NOWAIT)) {
		printf("bba_trigger_input: can't load DMA map\n");
		goto bad;
	}
	state |= 2;

	d->intr = intr;
	d->intr_arg = arg;
	d->curseg = 1;

	/* get physical address of buffer start */
	phys = (tc_addr_t)d->dmam->dm_segs[0].ds_addr;
	nphys = (tc_addr_t)d->dmam->dm_segs[1].ds_addr;

	/* setup DMA pointer */
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_DMAPTR,
	    IOASIC_DMA_ADDR(phys));
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_ISDN_R_NEXTPTR,
	    IOASIC_DMA_ADDR(nphys));

	/* kick off DMA */
	ssr |= IOASIC_CSR_DMAEN_ISDN_R;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_CSR, ssr);

	d->active = 1;

	return 0;

bad:
	if (state & 2)
		bus_dmamap_unload(sc->sc_dmat, d->dmam);
	if (state & 1)
		bus_dmamap_destroy(sc->sc_dmat, d->dmam);
	return 1;
}

int 
bba_intr(addr)
	void *addr;
{
	struct bba_softc *sc = addr;
	struct bba_dma_state *d;
	tc_addr_t nphys;
	int s, mask;

	s = splaudio();

	mask = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_INTR);

	if (mask & IOASIC_INTR_ISDN_TXLOAD) {
		d = &sc->sc_tx_dma_state;
		d->curseg = (d->curseg+1) % d->dmam->dm_nsegs;
		nphys = (tc_addr_t)d->dmam->dm_segs[d->curseg].ds_addr;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    IOASIC_ISDN_X_NEXTPTR, IOASIC_DMA_ADDR(nphys));
		if (d->intr != NULL)
			(*d->intr)(d->intr_arg);
	}
	if (mask & IOASIC_INTR_ISDN_RXLOAD) {
		d = &sc->sc_rx_dma_state;
		d->curseg = (d->curseg+1) % d->dmam->dm_nsegs;
		nphys = (tc_addr_t)d->dmam->dm_segs[d->curseg].ds_addr;
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    IOASIC_ISDN_R_NEXTPTR, IOASIC_DMA_ADDR(nphys));
		if (d->intr != NULL)
			(*d->intr)(d->intr_arg);
	}

	splx(s);

	return 0;
}

int
bba_get_props(addr)
        void *addr;
{
	return (AUDIO_PROP_MMAP | am7930_get_props(addr));
}

paddr_t
bba_mappage(addr, mem, offset, prot)
	void *addr;
	void *mem;
	off_t offset;
	int prot;
{
	struct bba_softc *sc = addr;
        struct bba_mem **mp;
	bus_dma_segment_t seg;
        caddr_t kva = (caddr_t)mem;

	for (mp = &sc->sc_mem_head; *mp && (*mp)->kva != kva;
	    mp = &(*mp)->next)
		/* nothing */ ;
	if (*mp == NULL || offset < 0) {
		return -1;
	}

        seg.ds_addr = (*mp)->addr;
        seg.ds_len = (*mp)->size;

        return bus_dmamem_mmap(sc->sc_dmat, &seg, 1, offset,
	    prot, BUS_DMA_WAITOK);
}


void
bba_input_conv(v, p, cc)
	void *v;
	u_int8_t *p;
	int cc;
{
	u_int8_t *q = p;

	DPRINTF(("bba_input_conv(): v=%p p=%p cc=%d\n", v, p, cc));

	/*
	 * p points start of buffer
	 * cc is the number of bytes in the destination buffer
	 */

	while (--cc >= 0) {
		*p = ((*(u_int32_t *)q)>>16)&0xff;
		q += 4;
		p++;
	}
}


void
bba_output_conv(v, p, cc)
	void *v;
	u_int8_t *p;
	int cc;
{
	u_int8_t *q = p;

	DPRINTF(("bba_output_conv(): v=%p p=%p cc=%d\n", v, p, cc));

	/*
	 * p points start of buffer
	 * cc is the number of bytes in the source buffer
	 */

	p += cc;
	q += cc * 4;
	while (--cc >= 0) {
		q -= 4;
		p -= 1;
		*(u_int32_t *)q = (*p<<16);
	}
}


int
bba_round_blocksize(addr, blk)
	void *addr;
	int blk;
{
	return (PAGE_SIZE);
}


/* indirect write */
void
bba_codec_iwrite(sc, reg, val)
	struct am7930_softc *sc;
	int reg;
	u_int8_t val;
{
	DPRINTF(("bba_codec_iwrite(): sc=%p, reg=%d, val=%d\n",sc,reg,val));

	bba_codec_dwrite(sc, AM7930_DREG_CR, reg);
	bba_codec_dwrite(sc, AM7930_DREG_DR, val);
}


void
bba_codec_iwrite16(sc, reg, val)
	struct am7930_softc *sc;
	int reg;
	u_int16_t val;
{
	DPRINTF(("bba_codec_iwrite16(): sc=%p, reg=%d, val=%d\n",sc,reg,val));

	bba_codec_dwrite(sc, AM7930_DREG_CR, reg);
	bba_codec_dwrite(sc, AM7930_DREG_DR, val);
	bba_codec_dwrite(sc, AM7930_DREG_DR, val>>8);
}


u_int16_t
bba_codec_iread16(sc, reg)
	struct am7930_softc *sc;
	int reg;
{
	u_int16_t val;
	DPRINTF(("bba_codec_iread16(): sc=%p, reg=%d\n",sc,reg));

	bba_codec_dwrite(sc, AM7930_DREG_CR, reg);
	val = bba_codec_dread(sc, AM7930_DREG_DR) << 8;
	val |= bba_codec_dread(sc, AM7930_DREG_DR);

	return val;
}


/* indirect read */
u_int8_t
bba_codec_iread(sc, reg)
	struct am7930_softc *sc;
	int reg;
{
	u_int8_t val;

	DPRINTF(("bba_codec_iread(): sc=%p, reg=%d\n",sc,reg));

	bba_codec_dwrite(sc, AM7930_DREG_CR, reg);
	val = bba_codec_dread(sc, AM7930_DREG_DR);

	DPRINTF(("read 0x%x (%d)\n", val, val));

	return val;
}

/* direct write */
void
bba_codec_dwrite(asc, reg, val)
	struct am7930_softc *asc;
	int reg;
	u_int8_t val;
{
	struct bba_softc *sc = (struct bba_softc *)asc;

	DPRINTF(("bba_codec_dwrite(): sc=%p, reg=%d, val=%d\n",sc,reg,val));

#if defined(__alpha__)
	bus_space_write_4(sc->sc_bst, sc->sc_codec_bsh,
	    reg << 2, val << 8);
#else
	bus_space_write_4(sc->sc_bst, sc->sc_codec_bsh,
	    reg << 6, val);
#endif
}

/* direct read */
u_int8_t
bba_codec_dread(asc, reg)
	struct am7930_softc *asc;
	int reg;
{
	struct bba_softc *sc = (struct bba_softc *)asc;

	DPRINTF(("bba_codec_dread(): sc=%p, reg=%d\n",sc,reg));

#if defined(__alpha__)
	return ((bus_space_read_4(sc->sc_bst, sc->sc_codec_bsh,
		reg << 2) >> 8) & 0xff);
#else
	return (bus_space_read_4(sc->sc_bst, sc->sc_codec_bsh,
		reg << 6) & 0xff);
#endif
}
