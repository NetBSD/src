/*	$NetBSD: ifpgamem.h,v 1.2 2013/02/19 10:57:10 skrll Exp $ */

/*
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Physical memory map provided by the integrator FPGA
 */

#define	IFPGA_SDRAM_BASE		0x00000000
#define IFPGA_SDRAM_SIZE		0x10000000	/* 256 MB */

#define	IFPGA_SSRAM_BASE		0x00000000	/* Overlaps SDRAM */
#define IFPGA_SSRAM_SIZE		0x00040000	/* 256 KB */

#define IFPGA_IO_BASE			0x10000000
#define IFPGA_IO_SIZE			0x10000000	/* 256 KB */

#define IFPGA_IO_CM_BASE		0x00000000	/* Core module regs */
#define IFPGA_IO_CM_SIZE		0x00000200

#define IFPGA_IO_SC_BASE		0x01000000	/* System Ctrl regs */
#define IFPGA_IO_SC_SIZE		0x00000028

#define IFPGA_IO_TMR_BASE		0x03000000	/* Countr/timr regs */
#define IFPGA_IO_TMR_SIZE		0x00000210

#define IFPGA_IO_IRQ_BASE		0x04000000	/* IRQ controller */
#define IFPGA_IO_IRQ_SIZE		0x00000100

#define IFPGA_TIMER0_BASE		0x00000000
#define IFPGA_TIMER1_BASE		0x00000100
#define IFPGA_TIMER2_BASE		0x00000200

#define IFPGA_TIMER0_IRQ		5
#define IFPGA_TIMER1_IRQ		6
#define IFPGA_TIMER2_IRQ		7

#if defined(INTEGRATOR_CP)
#define IFPGA_TIMER1_FREQ		1000000		/* 1 MHz */
#define IFPGA_TIMER2_FREQ		1000000		/* 1 MHz */
#else
#define IFPGA_TIMER1_FREQ		24000000	/* 24 MHz */
#define IFPGA_TIMER2_FREQ		24000000	/* 24 MHz */
#endif

#define IFPGA_EBI_ROM_BASE		0x20000000
#define IFPGA_EBI_ROM_SIZE		0x04000000	/* 64MB */

#define IFPGA_EBI_FLASH_BASE		0x24000000
#define IFPGA_EBI_FLASH_SIZE		0x04000000	/* 64MB */

#define IFPGA_EBI_SSRAM_BASE		0x28000000
#define IFPGA_EBI_SSRAM_SIZE		0x04000000	/* 64MB */

#define IFPGA_PCI_BASE			0x40000000	/* Base of entire PCI
							   subsystem.  */

#define IFPGA_PCI_APP0_BASE		0x40000000
#define IFPGA_PCI_APP0_SIZE		0x10000000	/* 256MB */

#define IFPGA_PCI_APP1_BASE		0x50000000
#define IFPGA_PCI_APP1_SIZE		0x10000000	/* 256MB */

#define IFPGA_PCI_IO_BASE		0x60000000	/* Absolute */
#define IFPGA_PCI_IO_VBASE		0xfe000000
#define IFPGA_PCI_IO_VSIZE		0x01000000	/* 16MB */

#define IFPGA_PCI_CONF_BASE		0x61000000	/* Absolute */
#define IFPGA_PCI_CONF_VBASE		0xff000000
#define IFPGA_PCI_CONF_VSIZE		0x01000000      /* 16MB */

#define IFPGA_V360_REG_BASE		0x62000000
#define IFPGA_V360_REG_SIZE		0x00010000	/* 64K */

/* Core module alias memory.  */
#define IFPGA_CM_ALIAS_BASE		0x80000000

/* Logic module memory.  */
#define IFPGA_LM_BASE			0xc0000000

