/*	$NetBSD: picreg.h,v 1.4.6.1 2006/04/22 11:37:55 simonb Exp $	*/

/*
 * Copyright (c) 2002 Steve Rumble
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

#ifndef _ARCH_SGIMIPS_DEV_PICREG_H_
#define	_ARCH_SGIMIPS_DEV_PICREG_H_

#define PIC_CPUCTRL		0x00	/* CPU control */

#define PIC_CPUCTRL_REFRESH	0x0001	/* refresh enable */
#define PIC_CPUCTRL_BIGENDIAN	0x0002	/* big endian mode */
#define PIC_CPUCTRL_DBREFILL	0x0004	/* data block refill */
#define PIC_CPUCTRL_IBREFILL	0x0008	/* instruction block refill */
#define PIC_CPUCTRL_GDMAINTR	0x0010	/* gfx intr on completion */
#define PIC_CPUCTRL_GDMASYNC	0x0020	/* gfx dma sync */
#define PIC_CPUCTRL_FREFRESH	0x0040	/* fast refresh on 33 MHz+ gio */
#define PIC_CPUCTRL_NOVMEERR	0x0080	/* disables vme bus errors */
#define PIC_CPUCTRL_FREFRESHB	0x0080	/* fast refresh on revs. a+b */
#define PIC_CPUCTRL_GR2		0x0100	/* gio gr2 mode (?) */
#define PIC_CPUCTRL_SYSRESET	0x0200	/* vme sysreset line */
#define PIC_CPUCTRL_MPR		0x0400	/* memory read parity enable */
#define PIC_CPUCTRL_SLAVE	0x0800	/* slave accesses permitted */
#define PIC_CPUCTRL_VMEARB	0x1000	/* vme arbiter enable */
#define PIC_CPUCTRL_WPR		0x2000	/* write bad parity */
#define PIC_CPUCTRL_WDOG	0x4000	/* watchdog enable */
#define PIC_CPUCTRL_GFXRESET	0x8000	/* reset graphics */

#define PIC_MODE		0x04	/* system mode */

#define PIC_MODE_DBSIZ		0x0003	/* data block size */
#define PIC_MODE_IBSIZ		0x000c	/* instruction block size */
#define PIC_MODE_ISTREAM	0x0010	/* instruction streaming */
#define	PIC_MODE_NOCACHE	0x0020	/* cache disabled */
#define PIC_MODE_STOREPARTIAL	0x0040	/* store partial */
#define	PIC_MODE_BUSDRIVE	0x0080	/* bus drive */

#define PIC_SYSID		0x08	/* system id */

#define PIC_SYSID_FPU		0x0001	/* fpu exists */
#define PIC_SYSID_GDMAERR	0x0004	/* graphics dma error */
#define PIC_SYSID_GDMADONE	0x0008	/* graphics dma complete */
#define PIC_SYSID_VMERMW	0x0010	/* vme read-mod-write */
#define PIC_SYSID_REVSHIFT	0x0006	/* Rev bits shifted */
#define PIC_SYSID_REVMASK	0x0007	/* PIC revision */

#define PIC_MEMCFG0		0x10000	/* memory config register 0 */
#define PIC_MEMCFG1		0x10004	/* memory config register 1 */
#define PIC_MEMCFG0_PHYSADDR	(0x1fa00000 + PIC_MEMCFG0)
#define PIC_MEMCFG1_PHYSADDR	(0x1fa00000 + PIC_MEMCFG1)

#define PIC_MEMCFG_4MB		0x0000	/* 4 megabytes (never occurs) */
#define PIC_MEMCFG_8MB		0x0001	/* 8 megabytes */
#define PIC_MEMCFG_16MB		0x0003	/* 16 megabytes */
#define PIC_MEMCFG_32MB		0x0007	/* 32 megabytes */
#define PIC_MEMCFG_64MB		0x000f	/* 64 megabytes */

#define PIC_MEMCFG_BADSIZ	0x0000	/* bad memory size */
#define PIC_MEMCFG_ADDRMASK	0x003f	/* memory address mask */
#define PIC_MEMCFG_BADADDR	0x003f	/* no memory in bank */
#define	PIC_MEMCFG_SIZMASK	0x0f00	/* bank size mask */

/*
 * The bank memory address is computed the same way mc's is.
 * Size is similar, only having one less bit (max. 64MB per bank).
 */
#define PIC_MEMCFG_ADDR(x)						\
	((x & PIC_MEMCFG_ADDRMASK) << 22)
#define PIC_MEMCFG_SIZ(x)						\
	(((x & PIC_MEMCFG_SIZMASK) + 0x100) << 14)

#define PIC_WRONLY_REFRESH	0x10100	/* write only refresh timer */

#define PIC_PARITY_ERROR	0x10200	/* parity errors */

#define PIC_PARITY_ERROR_GDMA	0x0001	/* graphics dma */
#define PIC_PARITY_ERROR_DMA	0x0002
#define PIC_PARITY_ERROR_CPU	0x0004
#define PIC_PARITY_ERROR_VME	0x0008
#define PIC_PARITY_ERROR_BYTE3	0x0010	/* error in fourth byte */
#define PIC_PARITY_ERROR_BYTE2	0x0020	/* error in third byte */
#define PIC_PARITY_ERROR_BYTE1	0x0040	/* error in second byte */
#define PIC_PARITY_ERROR_BYTE0	0x0080	/* error in first byte */

#define	PIC_PARITY_ADDR_CPU	0x10204	/* cpu error address */
#define PIC_PARITY_ADDR_DMA	0x10208	/* dma error address */
#define PIC_PARITY_ERROR_CLEAR	0x10210	/* clear parity errors */

/*
 * GIO slot configuration registers described by the 'GIO BUS Specification'
 * apparently no IP20 counterpart on mc.
 */
#define PIC_GIO32ARB_SLOT0	0x20000	/* set slot 0 config */
#define	PIC_GIO32ARB_SLOT1	0x20004	/* set slot 1 config */

#define PIC_GIO32ARB_SLOT_SLAVE	0x0001	/* slave only */
#define PIC_GIO32ARB_SLOT_LONG	0x0002	/* long burst */

#define PIC_GIO32ARB_BURST	0x20008	/* set gio burst */

#define PIC_GIO32ARB_DEFBURST	0x0001	/* default burst value */

#define PIC_GIO32ARB_DELAY	0x2000c	/* set gio delay */

#define PIC_GIO32ARB_DEFDELAY	0x00f2	/* default delay value */

#endif	/* _ARCH_SGIMIPS_DEV_PICREG_H_ */
