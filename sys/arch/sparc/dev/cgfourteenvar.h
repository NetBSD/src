/*	$NetBSD: cgfourteenvar.h,v 1.13 2010/08/31 21:14:57 macallan Exp $ */

/*
 * Copyright (c) 1996
 *	The President and Fellows of Harvard College. All rights reserved.
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
 *	This product includes software developed by Harvard University and
 *	its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
struct sbus_reg {
	uint32_t	sbr_slot;
	uint32_t	sbr_offset;
	uint32_t	sbr_size;
};
/*
 * Layout of cg14 hardware colormap
 */
union cg14cmap {
	u_char  	cm_map[256][4];	/* 256 R/G/B/A entries (B is high)*/
	uint32_t   	cm_chip[256];	/* the way the chip gets loaded */
};

/*
 * cg14 hardware cursor colormap
 */
union cg14cursor_cmap {		/* colormap, like bt_cmap, but tiny */
	u_char		cm_map[2][4];	/* 2 R/G/B/A entries */
	uint32_t	cm_chip[2];	/* 2 chip equivalents */
};

/*
 * cg14 hardware cursor status
 */
struct cg14_cursor {		/* cg14 hardware cursor status */
	int	cc_enable;		/* cursor is enabled */
	struct	fbcurpos cc_pos;	/* position */
	struct	fbcurpos cc_hot;	/* hot-spot */
	struct	fbcurpos cc_size;	/* size of mask & image fields */
	uint	cc_eplane[32];		/* enable plane */
	uint	cc_cplane[32];		/* color plane */
	union	cg14cursor_cmap cc_color; /* cursor colormap */
};

#define CG14_SET_PIXELMODE	_IOW('M', 3, int)

/*
 * per-cg14 variables/state
 */
struct cgfourteen_softc {
	device_t	sc_dev;		/* base device */
	struct fbdevice	sc_fb;		/* frame buffer device */
#ifdef RASTERCONSOLE
	struct fbdevice	sc_rcfb;	/* sc_fb variant for rcons */
#endif
	bus_space_tag_t	sc_bustag;
	struct sbus_reg	sc_physadr[2];	/* phys addrs of h/w */
	bus_space_handle_t sc_regh;	/* register space */
#define CG14_CTL_IDX	0
#define CG14_PXL_IDX	1

	union	cg14cmap sc_cmap;	/* current colormap */
	struct	cg14_cursor sc_cursor;	/* Hardware cursor state */
	union 	cg14cmap sc_saveclut; 	/* a place to stash PROM state */
	size_t	sc_vramsize;
	int 	sc_depth;	/* current colour depth */
#if NWSDISPLAY > 0
	struct  vcons_data sc_vd;
	struct 	vcons_screen sc_console_screen;
	struct 	wsscreen_descr sc_defaultscreen_descr;
	const struct wsscreen_descr *sc_screens[1];
	struct 	wsscreen_list sc_screenlist;
	int 	sc_mode;	/* wsdisplay mode - EMUL, DUMB etc. */
#endif

	uint8_t	sc_savexlut[256];
	uint8_t	sc_savectl;
	uint8_t	sc_savehwc;

	struct	cg14ctl  *sc_ctl; 	/* various registers */
	struct	cg14curs *sc_hwc;
	struct 	cg14dac	 *sc_dac;
	struct	cg14xlut *sc_xlut;
	struct 	cg14clut *sc_clut1;
	struct	cg14clut *sc_clut2;
	struct	cg14clut *sc_clut3;
	uint	*sc_clutincr;
	int	sc_opens;
	off_t	sc_regaddr, sc_fbaddr;
};

/* Various offsets in virtual (ie. mmap()) spaces Linux and Solaris support. */
#define CG14_REGS_VOFF		0x00000000	/* registers */
#define CG14_XLUT_VOFF		0x00003000	/* X Look Up Table */
#define CG14_CLUT1_VOFF		0x00004000	/* Color Look Up Table */
#define CG14_CLUT2_VOFF		0x00005000	/* Color Look Up Table */
#define CG14_CLUT3_VOFF		0x00006000	/* Color Look Up Table */
#define CG14_DIRECT_VOFF	0x10000000
#define CG14_CTLREG_VOFF	0x20000000
#define CG14_CURSOR_VOFF	0x30000000
#define CG14_SHDW_VRT_VOFF	0x40000000
#define CG14_XBGR_VOFF		0x50000000
#define CG14_BGR_VOFF		0x60000000
#define CG14_X16_VOFF		0x70000000
#define CG14_C16_VOFF		0x80000000
#define CG14_X32_VOFF		0x90000000
#define CG14_B32_VOFF		0xa0000000
#define CG14_G32_VOFF		0xb0000000
#define CG14_R32_VOFF		0xc0000000
