/*	$NetBSD: cs428x.c,v 1.1.2.3 2001/04/21 17:49:11 bouyer Exp $	*/

/*
 * Copyright (c) 2000 Tatoku Ogaito.  All rights reserved.
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
 *      This product includes software developed by Tatoku Ogaito
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/* Common functions for CS4280 and CS4281 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/midi_if.h>
#include <dev/mulaw.h>
#include <dev/auconv.h>

#include <dev/ic/ac97reg.h>
#include <dev/ic/ac97var.h>

#include <machine/bus.h>

#include <dev/pci/cs428xreg.h>
#include <dev/pci/cs428x.h>

#if defined(CS4280_DEBUG) || defined(CS4281_DEBUG)
int cs428x_debug = 0;
#endif

int
cs428x_open(void *addr, int flags)
{
	return 0;
}

void
cs428x_close(void *addr)
{
	struct cs428x_softc *sc;

	sc = addr;

	(*sc->halt_output)(sc);
	(*sc->halt_input)(sc);
	
	sc->sc_pintr = 0;
	sc->sc_rintr = 0;
}

int
cs428x_round_blocksize(void *addr, int blk)
{
	struct cs428x_softc *sc;
	int retval;
	
	DPRINTFN(5,("cs428x_round_blocksize blk=%d -> ", blk));
	
	sc = addr;
	if (blk < sc->hw_blocksize)
		retval = sc->hw_blocksize;
	else
		retval = blk & -(sc->hw_blocksize);

	DPRINTFN(5,("%d\n", retval));

	return retval;
}

int
cs428x_mixer_set_port(void *addr, mixer_ctrl_t *cp)
{
	struct cs428x_softc *sc;
	int val;

	sc = addr;
	val = sc->codec_if->vtbl->mixer_set_port(sc->codec_if, cp);
	DPRINTFN(3,("mixer_set_port: val=%d\n", val));
	return (val);
}

int
cs428x_mixer_get_port(void *addr, mixer_ctrl_t *cp)
{
	struct cs428x_softc *sc;

	sc = addr;
	return (sc->codec_if->vtbl->mixer_get_port(sc->codec_if, cp));
}

int
cs428x_query_devinfo(void *addr, mixer_devinfo_t *dip)
{
	struct cs428x_softc *sc;

	sc = addr;
	return (sc->codec_if->vtbl->query_devinfo(sc->codec_if, dip));
}

void *
cs428x_malloc(void *addr, int direction, size_t size, int pool, int flags)
{
	struct cs428x_softc *sc;
	struct cs428x_dma   *p;
	int error;

	sc = addr;

	p = malloc(sizeof(*p), pool, flags);
	if (p == NULL)
		return 0;

	error = cs428x_allocmem(sc, size, pool, flags, p);

	if (error) {
		free(p, pool);
		return 0;
	}

	p->next = sc->sc_dmas;
	sc->sc_dmas = p;
	return BUFADDR(p);
}

void
cs428x_free(void *addr, void *ptr, int pool)
{
	struct cs428x_softc *sc;
	struct cs428x_dma **pp, *p;

	sc = addr;
	for (pp = &sc->sc_dmas; (p = *pp) != NULL; pp = &p->next) {
		if (BUFADDR(p) == ptr) {
			bus_dmamap_unload(sc->sc_dmatag, p->map);
			bus_dmamap_destroy(sc->sc_dmatag, p->map);
			bus_dmamem_unmap(sc->sc_dmatag, p->addr, p->size);
			bus_dmamem_free(sc->sc_dmatag, p->segs, p->nsegs);
			free(p->dum, pool);
			*pp = p->next;
			free(p, pool);
			return;
		}
	}
}

size_t
cs428x_round_buffersize(void *addr, int direction, size_t size)
{
	/* The real dma buffersize are 4KB for CS4280
	 * and 64kB/MAX_CHANNELS for CS4281.
	 * But they are too small for high quality audio,
	 * let the upper layer(audio) use a larger buffer.
	 * (originally suggested by Lennart Augustsson.)
	 */
	return size;
}

paddr_t
cs428x_mappage(void *addr, void *mem, off_t off, int prot)
{
	struct cs428x_softc *sc;
	struct cs428x_dma *p;

	sc = addr;

	if (off < 0)
		return -1;

	for (p = sc->sc_dmas; p && BUFADDR(p) != mem; p = p->next)
		;

	if (p == NULL) {
		DPRINTF(("cs428x_mappage: bad buffer address\n"));
		return -1;
	}

	return (bus_dmamem_mmap(sc->sc_dmatag, p->segs, p->nsegs,
	    off, prot, BUS_DMA_WAITOK));
}

int
cs428x_get_props(void *addr)
{
	int retval;

	retval = AUDIO_PROP_INDEPENDENT | AUDIO_PROP_FULLDUPLEX;
#ifdef MMAP_READY
	/* How can I mmap ? */
	retval |= AUDIO_PROP_MMAP;
#endif
	return retval;
}

/* AC97 */
int
cs428x_attach_codec(void *addr, struct ac97_codec_if *codec_if)
{
	struct cs428x_softc *sc;

	DPRINTF(("cs428x_attach_codec:\n"));
	sc = addr;
	sc->codec_if = codec_if;
	return 0;
}

int
cs428x_read_codec(void *addr, u_int8_t ac97_addr, u_int16_t *ac97_data)
{
	struct cs428x_softc *sc;
	u_int32_t acctl;
	int n;

	sc = addr;

	DPRINTFN(5,("read_codec: add=0x%02x ", ac97_addr));
	/*
	 * Make sure that there is not data sitting around from a preivous
	 * uncompleted access.
	 */
	BA0READ4(sc, CS428X_ACSDA);

	/* Set up AC97 control registers. */
	BA0WRITE4(sc, CS428X_ACCAD, ac97_addr);
	BA0WRITE4(sc, CS428X_ACCDA, 0);

	acctl = ACCTL_ESYN | ACCTL_VFRM | ACCTL_CRW  | ACCTL_DCV;
	if (sc->type == TYPE_CS4280)
		acctl |= ACCTL_RSTN;
	BA0WRITE4(sc, CS428X_ACCTL, acctl);

	if (cs428x_src_wait(sc) < 0) {
		printf("%s: AC97 read prob. (DCV!=0) for add=0x%0x\n",
		       sc->sc_dev.dv_xname, ac97_addr);
		return 1;
	}

	/* wait for valid status bit is active */
	n = 0;
	while ((BA0READ4(sc, CS428X_ACSTS) & ACSTS_VSTS) == 0) {
		delay(1);
		while (++n > 1000) {
			printf("%s: AC97 read fail (VSTS==0) for add=0x%0x\n",
			       sc->sc_dev.dv_xname, ac97_addr);
			return 1;
		}
	}
	*ac97_data = BA0READ4(sc, CS428X_ACSDA);
	DPRINTFN(5,("data=0x%04x\n", *ac97_data));
	return 0;
}

int
cs428x_write_codec(void *addr, u_int8_t ac97_addr, u_int16_t ac97_data)
{
	struct cs428x_softc *sc;
	u_int32_t acctl;

	sc = addr;

	DPRINTFN(5,("write_codec: add=0x%02x  data=0x%04x\n", ac97_addr, ac97_data));
	BA0WRITE4(sc, CS428X_ACCAD, ac97_addr);
	BA0WRITE4(sc, CS428X_ACCDA, ac97_data);

	acctl = ACCTL_ESYN | ACCTL_VFRM | ACCTL_DCV;
	if (sc->type == TYPE_CS4280)
		acctl |= ACCTL_RSTN;
	BA0WRITE4(sc, CS428X_ACCTL, acctl);

	if (cs428x_src_wait(sc) < 0) {
		printf("%s: AC97 write fail (DCV!=0) for add=0x%02x data="
		       "0x%04x\n", sc->sc_dev.dv_xname, ac97_addr, ac97_data);
		return 1;
	}
	return 0;
}

/* Internal functions */
int
cs428x_allocmem(struct cs428x_softc *sc, 
		size_t size, int pool, int flags,
		struct cs428x_dma *p)
{
	int error;
	size_t align;

	align   = sc->dma_align;
	p->size = sc->dma_size;
	/* allocate memory for upper audio driver */
	p->dum  = malloc(size, pool, flags);
	if (p->dum == NULL)
		return 1;

	error = bus_dmamem_alloc(sc->sc_dmatag, p->size, align, 0,
				 p->segs, sizeof(p->segs)/sizeof(p->segs[0]),
				 &p->nsegs, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: unable to allocate dma. error=%d\n",
		       sc->sc_dev.dv_xname, error);
		goto allfree;
	}

	error = bus_dmamem_map(sc->sc_dmatag, p->segs, p->nsegs, p->size,
			       &p->addr, BUS_DMA_NOWAIT|BUS_DMA_COHERENT);
	if (error) {
		printf("%s: unable to map dma, error=%d\n",
		       sc->sc_dev.dv_xname, error);
		goto free;
	}

	error = bus_dmamap_create(sc->sc_dmatag, p->size, 1, p->size,
				  0, BUS_DMA_NOWAIT, &p->map);
	if (error) {
		printf("%s: unable to create dma map, error=%d\n",
		       sc->sc_dev.dv_xname, error);
		goto unmap;
	}

	error = bus_dmamap_load(sc->sc_dmatag, p->map, p->addr, p->size, NULL,
				BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: unable to load dma map, error=%d\n",
		       sc->sc_dev.dv_xname, error);
		goto destroy;
	}
	return 0;

 destroy:
	bus_dmamap_destroy(sc->sc_dmatag, p->map);
 unmap:
	bus_dmamem_unmap(sc->sc_dmatag, p->addr, p->size);
 free:
	bus_dmamem_free(sc->sc_dmatag, p->segs, p->nsegs);
 allfree:
	free(p->dum, pool);

	return error;
}

int
cs428x_src_wait(sc)
	struct cs428x_softc *sc;
{
	int n;

	n = 0;
	while ((BA0READ4(sc, CS428X_ACCTL) & ACCTL_DCV)) {
		delay(1000);
		while (++n > 1000) {
			printf("cs428x_src_wait: 0x%08x\n",
			    BA0READ4(sc, CS428X_ACCTL));
			return -1;
		}
	}
	return 0;
}
