/*	$NetBSD: nglereg.h,v 1.4 2025/12/29 06:02:51 macallan Exp $	*/

/*
 * Copyright (c) 2025 Michael Lorenz
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * HP Visualize EG and HCRX
 * sensible register names instead of random numbers
 * mostly found by experiment
 */

#ifndef NGLEREG_H
#define NGLEREG_H

#define	NGLE_HCRX_VBUS		0x000420	/* HCRX video bus access */

/*
 * BINC writes work more or less like FX4, except pixel addressing is a linear
 * address in whatever unit specified in NGLE_DBA ( AddrByte or AddrWord )
 */
#define	NGLE_BINC_SRC		0x000480	/* BINC src */
#define	NGLE_BINC_DST		0x0004a0	/* BINC dst */
#define	NGLE_BINC_MASK		0x0005a0	/* BINC pixel mask */
#define	NGLE_BINC_DATA		0x0005c0	/* BINC data, inc X, some sort of blending */
#define	NGLE_BINC_DATA_R	0x000600	/* BINC data, inc X */
#define	NGLE_BINC_DATA_D	0x000620	/* BINC data, inc Y */
#define	NGLE_BINC_DATA_U	0x000640	/* BINC data, dec Y */
#define	NGLE_BINC_DATA_L	0x000660	/* BINC data, dec X */
#define	NGLE_BINC_DATA_DR	0x000680	/* BINC data, inc X, inc Y */
#define	NGLE_BINC_DATA_DL	0x0006a0	/* BINC data, dec X, inc Y */
#define	NGLE_BINC_DATA_UR	0x0006c0	/* BINC data, inc X, dec Y */
#define	NGLE_BINC_DATA_UL	0x0006e0	/* BINC data, dec X, dec Y */

/*
 * coordinate registers
 * these *do* work like FX4 - you poke coordinates into registers and OR an
 * opcode on the last register write's address
 */
#define	NGLE_DST_XY		0x000800	/* destination XY */
#define	NGLE_SIZE		0x000804	/* size WH */
#define	NGLE_SRC_XY		0x000808	/* source XY */
#define	NGLE_TRANSFER_DATA	0x000820	/* 'transfer data' - this is */
						/* a pixel mask on fills */
#define NGLE_RECT		0x000200	/* opcode to start a fill */
#define NGLE_BLIT		0x000300	/* opcode to start a blit */
#define NGLE_HCRX_FASTFILL	0x000140	/* opcode for HCRX fast rect */
#define	NGLE_RECT_SIZE_START	(NGLE_SIZE | NGLE_RECT)
#define	NGLE_BLT_DST_START	(NGLE_DST_XY | NGLE_BLIT)

/*
 * bitmap access
 * works more or less like FX4, with completely different bit assignments
 */
#define	NGLE_BAboth		0x018000	/* read and write mode */
#define	NGLE_DBA		0x018004	/* Dest. Bitmap Access */
#define	NGLE_SBA		0x018008	/* Source Bitmap Access */

#define BA(F,C,S,A,J,B,I)						\
	(((F)<<31)|((C)<<27)|((S)<<24)|((A)<<21)|((J)<<16)|((B)<<12)|(I))
	/* FCCC CSSS AAAJ JJJJ BBBB IIII IIII IIII */

/* F */
#define	    IndexedDcd	0	/* Pixel data is indexed (pseudo) color */
#define	    FractDcd	1	/* Pixel data is Fractional 8-8-8 */
/* C */
#define	    Otc04	2	/* Pixels in each longword transfer (4) */
#define	    Otc32	5	/* Pixels in each longword transfer (32) */
#define	    Otc24	7	/* NGLE uses this for 24bit blits */
				/* Should really be... */
#define	    Otc01	7	/* one pixel per longword */
/* S */
#define	    Ots08	3	/* Each pixel is size (8)d transfer (1) */
#define	    OtsIndirect	6	/* Each bit goes through FG/BG color(8) */
/* A */
#define	    AddrByte	3	/* byte access? Used by NGLE for direct fb */
#define	    AddrLong	5	/* FB address is Long aligned (pixel) */
#define     Addr24	7	/* used for colour map access */
/* B */
#define	    BINapp0I	0x0	/* Application Buffer 0, Indexed */
#define	    BINapp1I	0x1	/* Application Buffer 1, Indexed */
#define	    BINovly	0x2	/* 8 bit overlay */
#define	    BINcursor	0x6	/* cursor bitmap on EG */
#define	    BINcmask	0x7	/* cursor mask on EG */
#define	    BINapp0F8	0xa	/* Application Buffer 0, Fractional 8-8-8 */
/* next one is a guess, my HCRX24 doesn't seem to have it */
#define	    BINapp1F8	0xb	/* Application Buffer 1, Fractional 8-8-8 */
#define	    BINattr	0xd	/* Attribute Bitmap */
#define	    BINcmap	0xf	/* colour map(s) */
/* I assume one of the undefined BIN* accesses the HCRX Z-buffer add-on. No clue
 * about bit depth or if any bits are used for stencil */
 
/* other buffers are unknown */
/* J - 'BA just point' - function unknown */
/* I - 'BA index base' - function unknown */

#define	NGLE_CPR		0x01800c	/* control plane register */
#define	NGLE_FG			0x018010	/* fg colour */
#define	NGLE_BG			0x018014	/* bg colour */
#define	NGLE_PLANEMASK		0x018018	/* image planemask */
#define	NGLE_IBO		0x01801c	/* image binary op */

#define IBOvals(R,M,X,S,D,L,B,F)					\
	(((R)<<8)|((M)<<16)|((X)<<24)|((S)<<29)|((D)<<28)|((L)<<31)|((B)<<1)|(F))
	/* LSSD XXXX MMMM MMMM RRRR RRRR ???? ??BF */

/* R is a standard X11 ROP, no idea if the other bits are used for anything  */
#define	    RopClr 	0x0
#define	    RopSrc 	0x3
#define	    RopInv 	0xc
#define	    RopSet 	0xf
/* M: 'mask addr offset' - function unknown */
/* X */
#define	    BitmapExtent08  3	/* Each write hits ( 8) bits in depth */
#define	    BitmapExtent32  5	/* Each write hits (32) bits in depth */
/* S: 'static reg' flag, this automatically masks off overhanging pixels on
      fill and copy oprtations if the width is not a multiple of Otc* */
/* D */
#define	    DataDynamic	    0	/* Data register reloaded by direct access */
#define	    MaskDynamic	    1	/* Mask register reloaded by direct access */
/* L */
#define	    MaskOtc	    0	/* Mask contains Object Count valid bits */
/* B = 1 -> background transparency for masked fills */
/* F probably the same for foreground */

#define	NGLE_DODGER		0x200000	/* 'busy dodger' idle */
	#define DODGER_IDLE	0x1000	/* or 0x10000, likely tpyo */
#define	NGLE_BUSY		0x200000	/* busy register */
#define	NGLE_CONTROL		0x200004	/* a guess */
/*
 * byte access, need to write 1 here for fb access to work properly
 * might be controlling FIFO pacing ( as in, turn it off for direct FB access )
 * like on FX
 */
#define	NGLE_CONTROL_FB		0x200005
#define	NGLE_FIFO		0x200008	/* # of fifo slots */
#define	NGLE_EG_CURSOR		0x200100	/* cursor coordinates on EG */
	#define EG_ENABLE_CURSOR	0x80000000
//#define	NGLE_REG_18		0x200104	/* cursor enable */
#define	NGLE_EG_LUTBLT		0x200118	/* EG LUT blt ctrl */
	/* EWRRRROO OOOOOOOO TTRRRRLL LLLLLLLL */
	#define LBC_ENABLE	0x80000000
	#define LBC_WAIT_BLANK	0x40000000
	#define LBS_OFFSET_SHIFT	16
	#define LBC_TYPE_MASK		0xc000
	#define LBC_TYPE_CMAP		0
	#define LBC_TYPE_CURSOR		0x8000
	#define LBC_TYPE_OVERLAY	0xc000
	#define LBC_LENGTH_SHIFT	0
//#define	NGLE_REG_19		0x200200	/* artist sprite size */
//#define	NGLE_REG_20		0x200208	/* cursor geometry */
/* the next two control video output on EG - xf86 sets both to enable output,
 * clears both to turn it off. Need to check which does what exactly */
#define	NGLE_EG_MISCVID		0x200218	/* Artist misc video */
	#define MISCVID_VIDEO_ON	0x0a000000
#define	NGLE_EG_MISCCTL		0x200308	/* Artist misc ctrl */
	#define MISCCTL_VIDEO_ON	0x00800000
#define	NGLE_HCRX_CURSOR	0x210000	/* HCRX cursor coord & enable */
	#define HCRX_ENABLE_CURSOR	0x80000000
/* HCRX uses those to access the cursor image instead of BINcursor/BINcmask */
#define	NGLE_HCRX_CURSOR_ADDR	0x210004	/* HCRX cursor address */
#define	NGLE_HCRX_CURSOR_DATA	0x210008	/* HCRX cursor data */

#define	NGLE_HCRX_LUTBLT	0x210020	/* HCRX LUT blt ctrl */
#define	NGLE_HCRX_PLANE_ENABLE	0x21003c	/* HCRX plane enable */ 
#define	NGLE_HCRX_MISCVID	0x210040	/* HCRX misc video */
	#define HCRX_BOOST_ENABLE	0x80000000 /* extra high signal level */
	#define HCRX_VIDEO_ENABLE	0x0A000000
	#define HCRX_OUTPUT_ENABLE	0x01000000
#define	NGLE_HCRX_HB_MODE2	0x210120	/* HCRX 'hyperbowl' mode 2 */
	#define HYPERBOWL_MODE2_8_24					15
/*
 * this seems to be the HCRX's analogue to FX's force attribute register - we
 * can switch between overlay opacity and image plane display mode on the fly
 */
#define	NGLE_HCRX_HB_MODE	0x210130	/* HCRX 'hyperbowl' */
	#define HYPERBOWL_MODE_FOR_8_OVER_88_LUT0_NO_TRANSPARENCIES	4
	#define HYPERBOWL_MODE01_8_24_LUT0_TRANSPARENT_LUT1_OPAQUE	8
	#define HYPERBOWL_MODE01_8_24_LUT0_OPAQUE_LUT1_OPAQUE		10

#endif /* NGLEREG_H */