/*	$NetBSD: sysfpgareg.h,v 1.1 2002/07/05 13:31:39 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_SYSFPGAREG_H
#define _SH5_SYSFPGAREG_H

/*
 * Offsets of the devices in the System FPGA area (Actually FEMI Area 1)
 */
#define	SYSFPGA_OFFSET_SUPERIO	0x0000
#define	SYSFPGA_OFFSET_LAN	0x1000
#define	SYSFPGA_OFFSET_REGS	0x2000

/*
 * The System FPGA's registers
 */
#define	SYSFPGA_REG_BDMR	0x00	/* Board operating mode register */
#define	SYSFPGA_REG_CPUMR	0x04	/* CPU mode register */
#define	SYSFPGA_REG_LEDCR	0x08	/* Discrete LED control register */
#define	SYSFPGA_REG_INTSR(n)	(0x10+((n)*4))	/* Interrupt source registers */
#define	SYSFPGA_REG_INTMR(n)	(0x20+((n)*4))	/* Interrupt mask registers */
#define	SYSFPGA_REG_NMISR	0x30	/* NMI source register */
#define	SYSFPGA_REG_NMIMR	0x34	/* NMI mask register */
#define	SYSFPGA_REG_LANWAIT	0x40	/* LAN controller wait register */
#define	SYSFPGA_REG_IOWAIT	0x44	/* Super IO wait register */
#define	SYSFPGA_REG_DATE	0x60	/* FPGA data code register */
#define	SYSFPGA_REG_SOFT_RESET	0x80	/* Software reset register */
#define	SYSFPGA_REG_SZ		0x200

/*
 * Bit definitions for the System FPGA's register
 */
#define	SYSFPGA_BDMR_FLBANK		(1<<15)
#define	SYSFPGA_BDMR_RDYCTRL		(1<<12)
#define	SYSFPGA_BDMR_PCICLKSEL		(1<<11)
#define	SYSFPGA_BDMR_BAUDSEL_MASK	(3<<9)
#define  SYSFPGA_BDMR_BAUDSEL_115200	(0<<9)
#define  SYSFPGA_BDMR_BAUDSEL_57600	(1<<9)
#define  SYSFPGA_BDMR_BAUDSEL_38400	(2<<9)
#define	SYSFPGA_BDMR_CPUCLKSEL(r)	(((r)>>7)&3)
#define	SYSFPGA_BDMR_SOFTWP		(1<<6)
#define	SYSFPGA_BDMR_FLWP_BIT		(1<<5)
#define	SYSFPGA_BDMR_MAPSEL		(1<<4)
#define	SYSFPGA_BDMR_SOFTMAP_MASK	(3<<2)
#define	 SYSFPGA_BDMR_SOFTMAP_NORMAL	(0<<2)
#define	 SYSFPGA_BDMR_SOFTMAP_EXTERNAL	(1<<2)
#define	 SYSFPGA_BDMR_SOFTMAP_BOOT1	(2<<2)
#define	 SYSFPGA_BDMR_SOFTMAP_BOOT2	(3<<2)
#define	SYSFPGA_BDMR_CS0MAP_MASK	(3<<0)
#define	 SYSFPGA_BDMR_CS0MAP_NORMAL	(0<<0)
#define	 SYSFPGA_BDMR_CS0MAP_EXTERNAL	(1<<0)
#define	 SYSFPGA_BDMR_CS0MAP_BOOT1	(2<<0)
#define	 SYSFPGA_BDMR_CS0MAP_BOOT2	(3<<0)

#define	SYSFPGA_CPUMR_CS0BUSWIDTH(r)	(((r)>>7)&3)
#define	SYSFPGA_CPUMR_CS0BUSWIDTH_8BIT	0
#define	SYSFPGA_CPUMR_CS0BUSWIDTH_16BIT	1
#define	SYSFPGA_CPUMR_CS0BUSWIDTH_32BIT	2
#define	SYSFPGA_CPUMR_CS0MEMTYPE(r)	(((r)>>6)&1)
#define	SYSFPGA_CPUMR_CS0MEMTYPE_SRAM	0
#define	SYSFPGA_CPUMR_CS0MEMTYPE_MPX	1
#define	SYSFPGA_CPUMR_CLKMODE(r)	((r)&7)

#define	SYSFPGA_LEDCR_SLED_MASK		1
#define	SYSFPGA_LEDCR_SLED_ON		0
#define	SYSFPGA_LEDCR_SLED_OFF		1

#define	SYSFPGA_DATE_REV(r)		((r) & 0xff)
#define	SYSFPGA_DATE_DATE(r)		(((r) >> 8) & 0xff)
#define	SYSFPGA_DATE_MONTH(r)		(((r) >> 16) & 0xff)
#define	SYSFPGA_DATE_YEAR(r)		(((r) >> 24) & 0xff)

#endif /* _SH5_SYSFPGAREG_H */
