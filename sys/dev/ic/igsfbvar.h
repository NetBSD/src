/*	$NetBSD: igsfbvar.h,v 1.1 2002/03/30 19:48:55 uwe Exp $ */

/*
 * Copyright (c) 2002 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
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

/*
 * Integraphics Systems IGA 1682 and (untested) CyberPro 2k.
 */
#ifndef _DEV_IC_IGSFBVAR_H_
#define _DEV_IC_IGSFBVAR_H_


#define	IGS_CMAP_SIZE	256	/* 256 R/G/B entries */
struct igs_hwcmap {
	u_int8_t r[IGS_CMAP_SIZE];
	u_int8_t g[IGS_CMAP_SIZE];
	u_int8_t b[IGS_CMAP_SIZE];
};


#define IGS_CURSOR_MAX_SIZE 64	/* 64x64 sprite */
struct igs_hwcursor {
	struct wsdisplay_curpos cc_pos;
	struct wsdisplay_curpos cc_hot;
	struct wsdisplay_curpos cc_size;
	u_int8_t cc_image[512];		/* save copy of image for GCURSOR */
	u_int8_t cc_mask[512];		/* save copy of mask for GCURSOR */
	u_int16_t cc_sprite[512];	/* sprite in device 2bpp format */
	u_int8_t cc_color[6];		/* 2 colors, 3 rgb components */
};


/*
 * Precomputed bit tables to convert 1bpp image/mask to 2bpp hw cursor
 * sprite.  For IGSFB_HW_BSWAP attachments they are pre-bswapped as well.
 */
struct igs_bittab {
	u_int16_t iexpand[256];	/* image: 0 -> 00, 1 -> 01 */
	u_int16_t mexpand[256];	/* mask:  0 -> 00, 1 -> 11 */
};

struct igsfb_softc {
	struct device sc_dev;

	/* io registers */
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	/* linear memory */
	bus_space_tag_t sc_memt;
	bus_addr_t sc_memaddr;		/* memory phys addr */
	bus_size_t sc_memsz;		/* size of linear address space */
	int sc_memflags;

	/* fb part actually mapped for wsdisplay */
	bus_space_handle_t sc_fbh;
	bus_size_t sc_fbsz;

	/* 1k of cursor sprite data */
	bus_space_handle_t sc_crh;

	/* 
	 * graphic coprocessor can be accessed either via i/o space or
	 * via memory-mapped i/o access through memory space.
	 */
	bus_space_tag_t sc_copt;
	bus_space_handle_t sc_coph;

	/* IGA1682 vs CyberPro 2k (use version num. for finer distinction?) */
	int sc_is2k;

	/* flags that control driver operation */
	int sc_hwflags;

	struct rasops_info *sc_ri;

	struct igs_hwcmap sc_cmap;	/* software copy of colormap */
	struct igs_hwcursor sc_cursor;	/* software copy of cursor sprite */

	/* precomputed bit tables for cursor sprite 1bpp -> 2bpp conversion */
	struct igs_bittab *sc_bittab;

	int nscreens;

	int sc_blanked;			/* screen is currently blanked */
	int sc_curenb;			/* cursor sprite enabled */
};

/* sc_hwflags */
#define IGSFB_HW_BSWAP	1	/* endianness mismatch */


/* methods for bus attachment glue */
int	igsfb_io_enable(bus_space_tag_t, bus_addr_t);
void	igsfb_mem_enable(struct igsfb_softc *);
void	igsfb_common_attach(struct igsfb_softc *, int);

#endif /* _DEV_IC_IGSFBVAR_H_ */
