/*	$NetBSD: igsfbreg.h,v 1.1.2.2 2002/04/17 00:05:38 nathanw Exp $ */

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
#ifndef _DEV_IC_IGSFBREG_H_
#define _DEV_IC_IGSFBREG_H_

/*
 * Magic address decoding for memory space accesses in CyberPro.
 */
#define IGS_MEM_MMIO_SELECT	0x00800000 /* memory mapped i/o */
#define IGS_MEM_BE_SELECT	0x00400000 /* endian */

/*
 * Registers in I/O space (could be memory-mapped i/o).
 */
#define IGS_IO_SIZE		0x400
#define IGS_COP_SIZE		0x400

/*
 * Cursor sprite data: 64x64 pixels, 2bpp = 1Kb.
 */
#define IGS_CURSOR_DATA_SIZE	0x0400


/*
 * Starting up the chip.
 */

/* Video Enable/Setup */
#define IGS_VDO			0x46e8
#define   IGS_VDO_ENABLE		0x08
#define   IGS_VDO_SETUP			0x10

/* Video Enable */
#define IGS_VSE			0x102
#define   IGS_VSE_ENABLE		0x01



/*
 * Palette Read/Write: write palette index to the index port.
 * Read/write R/G/B in three consecutive accesses to data port.
 * After third access to data the index is autoincremented and you can
 * proceed with reading/writing data port for the next entry.
 * 
 * When MRS2 bit in sprite control is set, these registers are used to
 * access sprite (i.e. cursor) 2-color palette.  (NB: apparently, in
 * this mode index autoincrement doesn't work).
 */
#define IGS_DAC_PEL_READ_IDX	0x3c7
#define IGS_DAC_PEL_WRITE_IDX	0x3c8
#define IGS_DAC_PEL_DATA	0x3c9


/*
 * Extended Registers.  Indexed access via IGS_EXT_PORT.
 */
#define IGS_EXT_IDX		0x3ce


/*
 * Sync Control.
 * Two bit combinations for h/v:
 *     00 - normal, 01 - force 0, 1x - force 1
 */
#define   IGS_EXT_SYNC_CTL		0x16
#define     IGS_EXT_SYNC_H0			0x01
#define     IGS_EXT_SYNC_H1			0x02
#define     IGS_EXT_SYNC_V0			0x04
#define     IGS_EXT_SYNC_V1			0x08

/*
 * For PCI just use normal BAR config.
 */
#define   IGS_EXT_BUS_CTL		0x30
#define     IGS_EXT_BUS_CTL_LINSIZE_SHIFT	0
#define     IGS_EXT_BUS_CTL_LINSIZE_MASK	0x03
#define     IGS_EXT_BUS_CTL_LINSIZE(x) \
    (((x) >> IGS_EXT_BUS_CTL_LINSIZE_SHIFT) & IGS_EXT_BUS_CTL_LINSIZE_MASK)

/*
 * COPREN   - enable direct access to coprocessor registers
 * COPASELB - COP address select 0xbfc00..0xbffff
 */
#define   IGS_EXT_BIU_MISC_CTL		0x33
#define     IGS_EXT_BIU_LINEAREN		0x01
#define     IGS_EXT_BIU_LIN2MEM			0x02
#define     IGS_EXT_BIU_COPREN			0x04
#define     IGS_EXT_BIU_COPASELB		0x08
#define     IGS_EXT_BIU_SEGON			0x10
#define     IGS_EXT_BIU_SEG2MEM			0x20

/*
 * Linear Address register
 *   PCI: don't write directly, just use nomral PCI configuration
 *   ISA: only bits [23..20] are programmable, the rest MBZ
 */
#define   IGS_EXT_LINA_LO		0x34	/* [3..0] -> [23..20] */
#define   IGS_EXT_LINA_HI		0x35	/* [7..0] -> [31..24] */

/* Hardware cursor (sprite) */
#define   IGS_EXT_SPRITE_HSTART_LO	0x50
#define   IGS_EXT_SPRITE_HSTART_HI	0x51	/* bits [2..0] */
#define   IGS_EXT_SPRITE_HPRESET	0x52	/* bits [5..0] */

#define   IGS_EXT_SPRITE_VSTART_LO	0x53
#define   IGS_EXT_SPRITE_VSTART_HI	0x54	/* bits [2..0] */
#define   IGS_EXT_SPRITE_VPRESET	0x55	/* bits [5..0] */

#define   IGS_EXT_SPRITE_CTL		0x56
#define     IGS_EXT_SPRITE_VISIBLE		0x01
#define     IGS_EXT_SPRITE_64x64		0x02
#define     IGS_EXT_SPRITE_SELECT		0x04

/* Overscan R/G/B registers */
#define   IGS_EXT_OVERSCAN_RED		0x58
#define   IGS_EXT_OVERSCAN_GREEN	0x59
#define   IGS_EXT_OVERSCAN_BLUE		0x5a

/* Hardware cursor (sprite) data location */
#define   IGS_EXT_SPRITE_DATA_LO	0x7e
#define   IGS_EXT_SPRITE_DATA_HI	0x7f	/* bits [3..0] */


/*********************************************************************
 *		  Access sugar for indexed registers
 */

static __inline__ u_int8_t
igs_idx_read(bus_space_tag_t, bus_space_handle_t, u_int, u_int8_t);

static __inline__ u_int8_t
igs_idx_read(t, h, idxport, idx)
	bus_space_tag_t t;
	bus_space_handle_t h;
	u_int idxport;
	u_int8_t idx;
{
	bus_space_write_1(t, h, idxport, idx);
	return (bus_space_read_1(t, h, idxport + 1));
}

static __inline__ void
igs_idx_write(bus_space_tag_t, bus_space_handle_t, u_int, u_int8_t, u_int8_t);

static __inline__ void
igs_idx_write(t, h, idxport, idx, val)
	bus_space_tag_t t;
	bus_space_handle_t h;
	u_int idxport;
	u_int8_t idx, val;
{
	bus_space_write_1(t, h, idxport, idx);
	bus_space_write_1(t, h, idxport + 1, val);
}

/* more sugar for extended registers */
#define igs_ext_read(t,h,x)	(igs_idx_read((t),(h),IGS_EXT_IDX,(x)))
#define igs_ext_write(t,h,x,v)	(igs_idx_write((t),(h),IGS_EXT_IDX,(x),(v)))

#endif /* _DEV_IC_IGSFBREG_H_ */
