/* $NetBSD: esavar.h,v 1.2.2.3 2002/02/11 20:09:57 jdolecek Exp $ */

/*
 * Copyright (c) 2001, 2002 Jared D. McNeill <jmcneill@invisible.yi.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * ESS Allegro-1 / Maestro3 Audio Driver
 * 
 * Based on the FreeBSD maestro3 driver
 *
 */

#define KERNADDR(p)	((void *)((p)->addr))
#define	DMAADDR(p)	((p)->map->dm_segs[0].ds_addr)

#define ESA_MINRATE	8000
#define ESA_MAXRATE	48000

struct esa_dma {
	bus_dmamap_t		map;
	caddr_t			addr;
	bus_dma_segment_t	segs[1];
	int			nsegs;
	size_t			size;
	struct esa_dma		*next;
};

struct esa_channel {
	int			active;
	int			data_offset;
	size_t			bufsize;
	int			blksize;
	int			pos;
	void			*buf;
	u_int32_t		start;
	u_int32_t		count;
	struct esa_dma		*dma;
	
	void			(*intr)(void *);
	void			*arg;
};

struct esa_softc
{
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_iob;
	bus_size_t		sc_ios;

	pcitag_t		sc_tag;
	pci_chipset_tag_t	sc_pct;
	bus_dma_tag_t		sc_dmat;
	pcireg_t		sc_pcireg;

	void			*sc_ih;

	struct ac97_codec_if	*codec_if;
	struct ac97_host_if	host_if;
	enum ac97_host_flags	codec_flags;

	struct device		*sc_audiodev;

	struct esa_channel	play;
	struct esa_channel	rec;
	struct esa_dma		*sc_dmas;

	int			type;		/* Allegro-1 or Maestro 3? */
	int			delay1, delay2;

	void			*powerhook;
	u_int16_t		*savemem;
};
