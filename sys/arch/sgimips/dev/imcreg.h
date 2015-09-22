/*	$NetBSD: imcreg.h,v 1.4.142.1 2015/09/22 12:05:51 skrll Exp $	*/

/*
 * Copyright (c) 2001 Rafal K. Boni
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
 *    derived from this software without specific prior written permission.
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

#ifndef _ARCH_SGIMIPS_DEV_IMCREG_H_
#define	_ARCH_SGIMIPS_DEV_IMCREG_H_

#define IMC_CPUCTRL0		0x04	/* CPU control, register 0 */

#define IMC_CPUCTRL0_REFMASK	0x000f	/* # lines to refresh */
#define IMC_CPUCTRL0_RFE	0x0010	/* refresh enable */
#define IMC_CPUCTRL0_GPR	0x0020	/* GIO parity enable */
#define IMC_CPUCTRL0_MPR	0x0040	/* memory parity enable */
#define IMC_CPUCTRL0_CPR	0x0080	/* cpu bus parity enable */
#define IMC_CPUCTRL0_WDOG	0x0100	/* watchdog enable */
#define IMC_CPUCTRL0_SIN	0x0200	/* reset system */
#define IMC_CPUCTRL0_GRR	0x0400	/* graphics reset */
#define IMC_CPUCTRL0_ENLOCK	0x0800	/* enable EISA memory lock */
#define IMC_CPUCTRL0_CMDPAR	0x1000	/* SysCmd parity enable */
#define IMC_CPUCTRL0_INTENA	0x2000	/* enable CPU interrupts */
#define IMC_CPUCTRL0_SNOOPENA	0x4000	/* enable gfx DMA snoop */
#define IMC_CPUCTRL0_PROM_WRENA	0x8000	/* disable buserr on PROM
						 * writes */
#define IMC_CPUCTRL0_WRST	0x00010000	/* warm restart (reset cpu) */
/* Bit 17 reserved		0x00020000	*/
#define IMC_CPUCTRL0_LITTLE	0x00040000	/* MC little-endian toggle */
#define IMC_CPUCTRL0_WRRST	0x00080000	/* cpu warm reset */
#define IMC_CPUCTRL0_MUXHWMSK	0x01f00000	/* MUX fifo high-water mask */
#define IMC_CPUCTRL0_BADPAR	0x02000000	/* generate bad parity on
						 * CPU->memory writes */
#define IMC_CPUCTRL0_NCHKMEMPAR	0x04000000	/* disable CPU parity check
						 * on memory reads. */
#define IMC_CPUCTRL0_BACK2	0x08000000	/* enable back2back GIO wrt */
#define IMC_CPUCTRL0_BUSRTMSK	0xf0000000	/* stall cycle for berr data */

#define IMC_CPUCTRL1		0x0c	/* CPU control, register 1 */
#define IMC_CPUCTRL1_MCHWMSK	0x0000000f	/* MC FIFO high water mask */
#define IMC_CPUCTRL1_ABORTEN	0x00000010	/* Enable GIO bus timeouts */
/* Bits 5 - 11 reserved		0x00000fe0	*/
#define IMC_CPUCTRL1_HPCFX	0x00001000	/* HPC endian fix */
#define IMC_CPUCTRL1_HPCLITTLE	0x00002000	/* HPC DMA is little-endian */
#define IMC_CPUCTRL1_EXP0FX	0x00004000	/* EXP0 endian fix */
#define IMC_CPUCTRL1_EXP0LITTLE	0x00008000	/* EXP0 DMA is little-endian */
#define IMC_CPUCTRL1_EXP1FX	0x00010000	/* EXP1 endian fix */
#define IMC_CPUCTRL1_EXP1LITTLE	0x00020000	/* EXP1 DMA is little-endian */

#define IMC_WDOG		0x14	/* Watchdog counter */
#define IMC_WDOG_MASK		0x001fffff	/* counter mask */

#define IMC_SYSID		0x1c	/* MC revision register */
#define IMC_SYSID_REVMASK	0x0000000f	/* MC revision mask */
#define IMC_SYSID_HAVEISA	0x00000010	/* EISA present */

#define IMC_RPSSDIV		0x2c	/* RPSS divider */
#define IMC_RPSSDIV_DIVMSK	0x000000ff	/* RPC divider mask */
#define IMC_RPSSDIV_INCMSK	0x0000ff00	/* RPC increment mask */

#define IMC_EEPROM		0x34	/* EEPROM serial interface */
/* Bit 1 is reserved		0x00000001	*/
#define IMC_EEPROM_CS		0x00000002	/* EEPROM chip select */
#define IMC_EEPROM_SCK		0x00000004	/* EEPROM serial clock */
#define IMC_EEPROM_SO		0x00000008	/* Serial data to EEPROM */
#define IMC_EEPROM_SI		0x00000010	/* Serial data from EEPROM */

#define IMC_CTRLD		0x44	/* Refresh counter preload */
#define IMC_CTRLD_MSK		0x000000ff	/* Counter preload mask */

#define IMC_REFCTR		0x4c	/* Refresh counter */
#define IMC_REFCTR_MSK		0x000000ff	/* Refresh counter mask */

#define IMC_GIO64ARB		0x84	/* GIO64 arbitration params */
#define IMC_GIO64ARB_HPC64	0x00000001	/* HPC addr size (32/64bit) */
#define IMC_GIO64ARB_GRX64	0x00000002	/* Gfx addr size (32/64bit) */
#define IMC_GIO64ARB_EXP064	0x00000004	/* EXP0 addr size (32/64bit) */
#define IMC_GIO64ARB_EXP164	0x00000008	/* EXP0 addr size (32/64bit) */
#define IMC_GIO64ARB_EISA64	0x00000010	/* EISA addr size (32/64bit) */
#define IMC_GIO64ARB_HPCEXP64	0x00000020	/* HPC2 addr size (32/64bit) */
#define IMC_GIO64ARB_GRXRT	0x00000040	/* Gfx is realtime device */
#define IMC_GIO64ARB_EXP0RT	0x00000080	/* EXP0 is realtime device */
#define IMC_GIO64ARB_EXP1RT	0x00000100	/* EXP1 is realtime device */
#define IMC_GIO64ARB_EISAMST	0x00000200	/* EISA can be busmaster */
#define IMC_GIO64ARB_ONEGIO	0x00000400	/* One one GIO64 bus */
#define IMC_GIO64ARB_GRXMST	0x00000800	/* Gfx can be busmaster */
#define IMC_GIO64ARB_EXP0MST	0x00001000	/* EXP0 can be busmaster */
#define IMC_GIO64ARB_EXP1MST	0x00002000	/* EXP1 can be busmaster */
#define IMC_GIO64ARB_EXP0PIPE	0x00004000	/* EXP0 is pipelined */
#define IMC_GIO64ARB_EXP1PIPE	0x00008000	/* EXP1 is pipelined */

#define IMC_CPUTIME		0x8c	/* Arbiter CPU time period */

#define IMC_LBTIME		0x9c	/* Arbiter long-burst time */

#define IMC_MEMCFG0		0xc4	/* Mem config, register 0 */

#define IMC_MEMCFG1		0xcc	/* Mem config, register 1 */

#define IMC_CPU_MEMACC		0xd4	/* CPU mem access config */

#define IMC_GIO_MEMACC		0xdc	/* GIO mem access config */

#define IMC_CPU_ERRADDR		0xe4	/* CPU error address */

#define IMC_CPU_ERRSTAT		0xec	/* CPU error status */

#define IMC_GIO_ERRADDR		0xf4	/* GIO error address */

#define IMC_GIO_ERRSTAT		0xfc	/* GIO error status */

#endif	/* _ARCH_SGIMIPS_DEV_IMCREG_H_ */

