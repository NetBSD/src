/* 	$NetBSD: pxreg.h,v 1.9 2005/12/24 23:24:01 perry Exp $	*/

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

#ifndef _PMAX_DEV_PXREG_H_
#define _PMAX_DEV_PXREG_H_

/*
 * Definitions for the PixelStamp on Digital's 2D and 3D TurboChannel
 * graphics accelerators. Stamp command packets take this general format:
 *
 * command word
 *
 * per-packet context (optional):
 *      line width
 *      xy mask
 *	cliping rectangle min & max
 *	rgb constant
 *	z constant
 *
 * per-primitive context (optional):
 *      xy mask
 *      xy mask address
 *      primitive data (vertices, spans info, video)
 *      line width
 *      halfspace equals conditions
 *      rgb flat, or rgb{1,2,3} smooth
 *      z flat, or z{1,2,3} smooth
 */

/*
 * These definitions are for the stamp command word, the first in
 * each packet. This is a 32-bit word on all architectures.
 */

/* opcode type */
#define STAMP_CMD_POINTS        (0x0000)
#define STAMP_CMD_LINES         (0x0001)
#define STAMP_CMD_TRIANGLES     (0x0002)
#define STAMP_CMD_COPYSPANS     (0x0005)
#define STAMP_CMD_READSPANS     (0x0006)
#define STAMP_CMD_WRITESPANS    (0x0007)
#define STAMP_CMD_VIDEO         (0x0008)

/* RGB format */
#define STAMP_RGB_NONE          (0x0000)
#define STAMP_RGB_CONST         (0x0010)
#define STAMP_RGB_FLAT          (0x0020)
#define STAMP_RGB_SMOOTH        (0x0030)

/* Z format */
#define STAMP_Z_NONE            (0x0000)
#define STAMP_Z_CONST           (0x0040)
#define STAMP_Z_FLAT            (0x0080)
#define STAMP_Z_SMOOTH          (0x00c0)

/* XY mask format */
#define STAMP_XY_NONE           (0x0000)
#define STAMP_XY_PERPACKET      (0x0100)
#define STAMP_XY_PERPRIMATIVE   (0x0200)

/* line width format */
#define STAMP_LW_NONE           (0x0000)
#define STAMP_LW_PERPACKET      (0x0400)
#define STAMP_LW_PERPRIMATIVE   (0x0800)

/* misc. */
#define STAMP_CLIPRECT          (0x00080000)
#define STAMP_MESH              (0x00200000)
#define STAMP_AALINE            (0x00800000)
#define STAMP_HS_EQUALS         (0x80000000)

/*
 * These definitions are for the stamp update word, also part of
 * each packet.
 */

/* plane */
#define STAMP_PLANE_8X3		(0 << 5)
#define STAMP_PLANE_24		(1 << 5)

/* when to write enable the stamp */
#define STAMP_WE_SIGN		(0x04 << 8)
#define STAMP_WE_XYMASK		(0x02 << 8)
#define STAMP_WE_CLIPRECT	(0x01 << 8)
#define STAMP_WE_NONE		(0x00 << 8)

/* update method */
#define STAMP_METHOD_CLEAR	(0x60 << 12)
#define STAMP_METHOD_AND	(0x14 << 12)
#define STAMP_METHOD_ANDREV	(0x15 << 12)
#define STAMP_METHOD_COPY	(0x20 << 12)
#define STAMP_METHOD_ANDINV	(0x16 << 12)
#define STAMP_METHOD_NOOP	(0x40 << 12)
#define STAMP_METHOD_XOR	(0x11 << 12)
#define STAMP_METHOD_OR		(0x0f << 12)
#define STAMP_METHOD_NOR	(0x17 << 12)
#define STAMP_METHOD_EQUIV	(0x10 << 12)
#define STAMP_METHOD_INV	(0x4e << 12)
#define STAMP_METHOD_ORREV	(0x0e << 12)
#define STAMP_METHOD_COPYINV	(0x2d << 12)
#define STAMP_METHOD_ORINV	(0x0d << 12)
#define STAMP_METHOD_NAND	(0x0c << 12)
#define STAMP_METHOD_SET	(0x6c << 12)
#define STAMP_METHOD_SUM	(0x00 << 12)
#define STAMP_METHOD_DIFF	(0x02 << 12)
#define STAMP_METHOD_REVDIFF	(0x01 << 12)

/* double buffering */
#define STAMP_DB_NONE		(0x00 << 28)
#define STAMP_DB_01		(0x01 << 28)
#define STAMP_DB_12		(0x02 << 28)
#define STAMP_DB_02		(0x04 << 28)

/* misc */
#define STAMP_UPDATE_ENABLE	(1)
#define STAMP_SAVE_SIGN		(1<<6)
#define STAMP_SAVE_ALPHA	(1<<7)
#define STAMP_SUPERSAMPLE	(1<<11)
#define STAMP_SPAN		(1<<19)
#define STAMP_COPYSPAN_ALIGNED	(1<<20)
#define STAMP_MINMAX		(1<<21)
#define STAMP_MULT		(1<<22)
#define STAMP_MULTACC		(1<<23)
#define STAMP_HALF_BUFF		(1<27)
#define STAMP_INITIALIZE	(1<<31)

#ifdef _KERNEL
#define STAMP_WIDTH	(pxi->pxi_stampw)
#define STAMP_HEIGHT	(pxi->pxi_stamph)
#endif

#define XMASKADDR(__sx, __a)	(((__a)-((__sx) % STAMP_WIDTH)) & 0xF)
#define YMASKADDR(__sy, __b)	(((__b)-((__sy) % STAMP_HEIGHT)) & 0xF)
#define XYMASKADDR(_x,_y,_a,_b)	(XMASKADDR(_x,_a) << 16 | YMASKADDR(_y,_b))

/*
 * For the poll register. Don't mess with the # of retries or the delay
 * unless you know what you're doing. According to OSF header files,
 * the delay on Alpha is 20us, and the # of retries should be 4000. This is
 * inadequate, particularly on the PXG which seems to run at a higher
 * frequency. The STIC gets wedged while scrolling quite a lot.
 */
#define STAMP_OK		(0)
#define STAMP_BUSY		(1)
#define STAMP_RETRIES		(7000)
#define STAMP_DELAY		(20)

#ifdef alpha
#define __PXS(n)	((n) << 1)
#else
#define __PXS(n)	(n)
#endif

/*
 * Hardware offsets within PX board's TC slot.
 */
#define PX_STIC_POLL_OFFSET	__PXS(0x000000)	/* STIC DMA poll space */
#define PX_STAMP_OFFSET		__PXS(0x0c0000)	/* pixelstamp space on STIC */
#define PX_STIC_OFFSET		__PXS(0x180000)	/* STIC registers */
#define PX_VDAC_OFFSET		__PXS(0x200000)	/* VDAC registers (bt459) */
#define PX_VDAC_RESET_OFFSET	__PXS(0x300000)	/* VDAC reset register */
#define PX_ROM_OFFSET		__PXS(0x300000)	/* ROM code */

/*
 * Hardware offsets within PXG board's TC slot.
 */
#define PXG_STIC_POLL_OFFSET	__PXS(0x000000)	/* STIC DMA poll space */
#define PXG_STAMP_OFFSET	__PXS(0x0c0000)	/* pixelstamp space on STIC */
#define PXG_STIC_OFFSET		__PXS(0x180000)	/* STIC registers */
#define PXG_SRAM_OFFSET		__PXS(0x200000)	/* N10 SRAM */
#define PXG_HOST_INTR_OFFSET	__PXS(0x280000)	/* N10 host interrupt */
#define PXG_COPROC_INTR_OFFSET	__PXS(0x2c0000)	/* N10 coprocessor interrupt */
#define PXG_VDAC_OFFSET		__PXS(0x300000)	/* VDAC registers (bt459) */
#define PXG_VDAC_RESET_OFFSET	__PXS(0x340000)	/* VDAC reset register */
#define PXG_ROM_OFFSET		__PXS(0x380000)	/* ROM code */
#define PXG_N10_START_OFFSET	__PXS(0x380000)	/* N10 start register */
#define PXG_N10_RESET_OFFSET	__PXS(0x3c0000)	/* N10 reset (stop?) register */

/*
 * STIC registers
 */
struct stic_regs {
#ifdef __alpha
	volatile int32_t	__pad0;
	volatile int32_t	__pad1;
	volatile int32_t	__pad2;
	volatile int32_t	__pad3;
	volatile int32_t        hsync;
	volatile int32_t	__pad4;
	volatile int32_t        hsync2;
	volatile int32_t	__pad5;
	volatile int32_t        hblank;
	volatile int32_t	__pad6;
	volatile int32_t        vsync;
	volatile int32_t	__pad7;
	volatile int32_t        vblank;
	volatile int32_t	__pad8;
	volatile int32_t        vtest;
	volatile int32_t	__pad9;
	volatile int32_t        ipdvint;
	volatile int32_t	__pad10;
	volatile int32_t	__pad11;
	volatile int32_t	__pad12;
	volatile int32_t        sticsr;
	volatile int32_t	__pad13;
	volatile int32_t        busdat;
	volatile int32_t	__pad14;
	volatile int32_t        busadr;
	volatile int32_t	__pad15;
	volatile int32_t        __pad16;
	volatile int32_t	__pad17;
	volatile int32_t        buscsr;
	volatile int32_t	__pad18;
	volatile int32_t        modcl;
	volatile int32_t	__pad19;
#else /* __alpha */
	volatile int32_t	__pad0;
	volatile int32_t	__pad1;
	volatile int32_t        hsync;
	volatile int32_t        hsync2;
	volatile int32_t        hblank;
	volatile int32_t        vsync;
	volatile int32_t        vblank;
	volatile int32_t        vtest;
	volatile int32_t        ipdvint;
	volatile int32_t	__pad2;
	volatile int32_t        sticsr;
	volatile int32_t        busdat;
	volatile int32_t        busadr;
	volatile int32_t        __pad3;
	volatile int32_t        buscsr;
	volatile int32_t        modcl;
#endif /* __alpha */
};

/*
 * Bit definitions for px_stic_regs.stic_csr.
 * these appear to exactly what the PROM tests use.
 */
#define STIC_CSR_TSTFNC		0x00000003
# define STIC_CSR_TSTFNC_NORMAL	0
# define STIC_CSR_TSTFNC_PARITY	1
# define STIC_CSR_TSTFNC_CNTPIX	2
# define STIC_CSR_TSTFNC_TSTDAC	3
#define STIC_CSR_CHECKPAR	0x00000004
#define STIC_CSR_STARTVT	0x00000010
#define STIC_CSR_START		0x00000020
#define STIC_CSR_RESET		0x00000040
#define STIC_CSR_STARTST	0x00000080

/*
 * Bit definitions for stic_regs.int.
 * Three four-bit wide fields, for error (E), vertical-blank (V), and
 * packetbuf-done (P) intererupts, respectively.
 * The low-order three bits of each field are enable, requested,
 * and acknowledge bits. The top bit of each field is unused.
 */
#define STIC_INT_E_EN		0x00000001
#define STIC_INT_E		0x00000002
#define STIC_INT_E_WE		0x00000004

#define STIC_INT_V_EN		0x00000100
#define STIC_INT_V		0x00000200
#define STIC_INT_V_WE		0x00000400

#define STIC_INT_P_EN		0x00010000
#define STIC_INT_P		0x00020000
#define STIC_INT_P_WE		0x00040000

#define STIC_INT_E_MASK	(STIC_INT_E_EN | STIC_INT_E | STIC_INT_E_WE)
#define STIC_INT_V_MASK	(STIC_INT_V_EN | STIC_INT_V | STIC_INT_V_WE)
#define STIC_INT_P_MASK	(STIC_INT_P_EN | STIC_INT_P | STIC_INT_P_WE)
#define STIC_INT_MASK	(STIC_INT_E_MASK | STIC_INT_P_MASK | STIC_INT_V_MASK)

#define STIC_INT_WE	(STIC_INT_E_WE | STIC_INT_V_WE | STIC_INT_P_WE)
#define STIC_INT_CLR	(STIC_INT_E_EN | STIC_INT_V_EN | STIC_INT_P_EN)

/*
 * Convert a system physical address to STIC poll offset. Polling the offset
 * returned will initiate DMA at the provided address. For the PX, the STIC
 * only sees 23-bits (8MB) of address space. Also, bits 21-22 in physical
 * address space map to bits 27-28 in the STIC's warped view of the world.
 * This is also true for bits 15-20, which map to bits 18-23. Bits 0 and 1
 * are meaningless, because everything is word aligned.
 *
 * The final shift-right-by-9 is to map the address to poll register offset.
 * These begin at px_softc.poll (which should obviously be added to the
 * return value of this function to get a vaild poll address).
 *
 * This shift right gives us a granularity of 512 bytes when DMAing. The
 * holes in STIC address space mean that DMAs can never cross a 32kB
 * boundary. The maximum size for a DMA AFAIK is about 4kB.
 *
 * For the PXG, the PA is relative to SRAM (i.e. i860) address space, not
 * system address space. The poll address will either return STAMP_OK, or
 * STAMP_BUSY.
 */
static inline u_long px_sys2stic __P((void *));
static inline u_long px_sys2dma __P((void *));
static inline volatile int32_t *px_poll_addr __P((caddr_t, void *));

static inline u_long
px_sys2stic(addr)
	void *addr;
{
	u_long	v;

	v = (u_long)addr;
	v = ((v & ~0x7fff) << 3) | (v & 0x7fff);
	return (v & 0x1ffff800);
}

static inline u_long
px_sys2dma(addr)
	void *addr;
{

	return px_sys2stic(addr) >> 9;
}

/*
 * This is simply a wrapper for the above that returns a proper VA to
 * poll when given a px_softc.
 */
static inline volatile int32_t *
px_poll_addr(slotbase, addr)
	caddr_t slotbase;
	void *addr;
{

	return (volatile int32_t *)(slotbase + px_sys2dma(addr));
}

#endif	/* !_PMAX_DEV_PXREG_H_ */
