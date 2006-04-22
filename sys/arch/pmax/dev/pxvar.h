/* 	$NetBSD: pxvar.h,v 1.11.58.1 2006/04/22 11:37:52 simonb Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _PMAX_DEV_PXVAR_H_
#define _PMAX_DEV_PXVAR_H_

struct px_cliplist {
	int	cl_cur;		/* current cliprect */
	int	cl_loaded;	/* number of cliprects loaded */
	int	cl_notloaded;	/* number of cliprects not loaded yet */
	u_int	cl_minval[32];	/* minval for cliprects */
	u_int	cl_maxval[32];	/* minval for cliprects */
	int32_t	*cl_fixup;	/* cliprect fixup pointer for current packet */
};

/* Do not let this structure grow larger than NBPG - 1! */
struct px_info {
	tc_addr_t pxi_slotbase;
	struct	stic_regs *pxi_stic;
	struct	bt459_regs *pxi_vdac;
	struct	wsdisplay_font *pxi_font;
	int	pxi_wsfcookie;

	int32_t	*pxi_poll;	/* STIC DMA poll area */
	caddr_t	pxi_stamp;	/* Undocumented stamp registers */
	caddr_t	pxi_sram;	/* SRAM on the PXG */

	int	pxi_unit;	/* unit # */
	char	pxi_type[12];	/* card type (PMAG-??) */
	int	pxi_planemask;	/* plane mask */
	int	pxi_stamph;	/* stamp height (1 or 2) */
	int	pxi_stampw;	/* stamp width (4 or 5) */
	char	pxi_nplanes;	/* number of planes */
	char	pxi_revision;	/* card revision */
  	char	pxi_option;	/* option type, from STIC (0=2D, 1=3D) */
  	char	pxi_flg;	/* flags (see below) */

  	void	*pxi_rbuf;	/* KVA of ring buffer */
  	u_long	pxi_rbuf_phys;	/* physical address of ring buffer */
  	int	pxi_rbuf_size;	/* ring buffer size in bytes */
	int	pxi_pbuf_select;/* current packet buffer offset */
	int	pxi_fontscale;	/* font scale factor (per character) */

	int	pxi_curx;	/* cursor X position */
	int	pxi_cury;	/* cursor Y position */
	u_char	pxi_cursor[1024];/* cursor bits */
	char	pxi_curcmap[6];	/* cursor colormap */

	char	pxi_dirty;	/* see PX_DIRTY_????? */
	char	pxi_enable;	/* enable video */
	char	pxi_cmap[768];	/* colormap */
	int	pxi_cmap_idx;	/* dirty colormap index */
	int	pxi_cmap_cnt;	/* dirty colormap count */

	/* Disgusting Xserver hack */
	struct	fbuaccess pxi_fbuaccess;
	struct	fbinfo pxi_fbinfo;

	/* Stuff for interrupt driven operation and Xserver */
	volatile int pxi_lpw;		/* last packet written */
	volatile int pxi_lpr;		/* last packet read */
	volatile int32_t *pxi_qpoll[16];/* packet buffer poll addresses */
	struct px_cliplist pxi_cliplist;/* cliplist for Xserver */
};

/* Map returned by ioctl QIOCGINFO for Xserver */
typedef struct px_map {
	struct stic_regs stic;
	u_char		__pad1[NBPG - sizeof(struct stic_regs)];
	int32_t		poll[4096];
	struct px_info	info;
	u_char		__pad0[NBPG - sizeof(struct px_info)];
	u_char		rbuf[81920];
} px_map;

/*
 * For pxi_info.pxi_dirty. These will be dealt with and cleared every
 * vertical retrace interrupt. Cursor related bits are stored in
 * the first nibble.
 */
#define PX_DIRTY_CURSOR_CMAP	(0x01)	/* Cursor color map */
#define PX_DIRTY_CURSOR		(0x02)	/* Cursor */
#define PX_DIRTY_CURSOR_POS	(0x04)	/* Cursor position */
#define PX_DIRTY_CURSOR_ENABLE	(0x08)	/* Cursor enable */
#define PX_DIRTY_CURSOR_MASK	(0x0F)	/* Mask for cursor bits */

#define PX_DIRTY_CMAP		(0x10)	/* Color map */
#define PX_DIRTY_ENABLE		(0x20)	/* Video output is enabled */

/* For pxi_info.pxi_flg */
#define PX_ISR_ENABLE		(0x01)	/* OK to queue packets via ISR */
#define PX_ISR_ACTIVE		(0x02)	/* ISR has packets waiting */
#define PX_ISR_PASS_CLIP	(0x04)	/* ISR passing pkt through cliplist */
#define PX_ISR_LOAD_CLIP	(0x08)	/* Xserver needs to reload cliplist */
#define PX_ISR_MASK		(0x07)

#define PX_OPEN			(0x10)	/* Device is open */
#define PX_ENABLE		(0x20)	/* Video enabled */
#define PX_CURSOR_ENABLE	(0x80)	/* Cursor enabled */


#ifdef _KERNEL

int px_cnattach __P((paddr_t));

#endif /* _KERNEL */

#endif	/* !_PMAX_DEV_PXVAR_H_ */
