/* $NetBSD: ciareg.h,v 1.8.2.1 1997/05/23 21:33:42 thorpej Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Authors: Chris G. Demetriou, Jason R. Thorpe
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * 21171 Chipset registers and constants.
 *
 * Taken from XXX
 */

#define	REGVAL(r)	(*(int32_t *)ALPHA_PHYS_TO_K0SEG(r))

/*
 * Base addresses
 */
#define	CIA_PCI_SMEM1	0x8000000000UL
#define	CIA_PCI_SMEM2	0x8400000000UL
#define	CIA_PCI_SMEM3	0x8500000000UL
#define	CIA_PCI_SIO1	0x8580000000UL
#define	CIA_PCI_SIO2	0x85c0000000UL
#define	CIA_PCI_DENSE	0x8600000000UL
#define	CIA_PCI_CONF	0x8700000000UL
#define	CIA_PCI_IACK	0x8720000000UL
#define	CIA_CSRS	0x8740000000UL
#define	CIA_PCI_MC_CSRS	0x8750000000UL
#define	CIA_PCI_ATRANS	0x8760000000UL
#define	CIA_PCI_TBIA	0x8760000100UL

#define	CIA_PCI_W0BASE	0x8760000400UL
#define	CIA_PCI_W0MASK	0x8760000440UL
#define	CIA_PCI_T0BASE	0x8760000480UL

#define	CIA_PCI_W1BASE	0x8760000500UL
#define	CIA_PCI_W1MASK	0x8760000540UL
#define	CIA_PCI_T1BASE	0x8760000580UL

#define	CIA_PCI_W2BASE	0x8760000600UL
#define	CIA_PCI_W2MASK	0x8760000640UL
#define	CIA_PCI_T2BASE	0x8760000680UL

#define	CIA_PCI_W3BASE	0x8760000700UL
#define	CIA_PCI_W3MASK	0x8760000740UL
#define	CIA_PCI_T3BASE	0x8760000780UL

/*
 * Values for CIA_PCI_TBIA
 */
#define	CIA_PCI_TBIA_NOOP	0	/* no operation */
#define	CIA_PCI_TBIA_LOCKED	1	/* invalidate and unlock locked tags */
#define	CIA_PCI_TBIA_UNLOCKED	2	/* invalidate unlocked tags */
#define	CIA_PCI_TBIA_ALL	3	/* invalidate and unlock all tags */

/*
 * Values for CIA_PCI_WnBASE
 */
#define	CIA_PCI_WnBASE_W_BASE	0xfff00000
#define	CIA_PCI_WnBASE_DAC_EN	0x00000008	/* W3BASE only */
#define	CIA_PCI_WnBASE_MEMCS_EN	0x00000004	/* W0BASE only */
#define	CIA_PCI_WnBASE_SG_EN	0x00000002
#define	CIA_PCI_WnBASE_W_EN	0x00000001

/*
 * Values for CIA_PCI_WnMASK
 */
#define	CIA_PCI_WnMASK_W_MASK	0xfff00000
#define	CIA_PCI_WnMASK_1M	0x00000000
#define	CIA_PCI_WnMASK_2M	0x00100000
#define	CIA_PCI_WnMASK_4M	0x00300000
#define	CIA_PCI_WnMASK_8M	0x00700000
#define	CIA_PCI_WnMASK_16M	0x00f00000
#define	CIA_PCI_WnMASK_32M	0x01f00000
#define	CIA_PCI_WnMASK_64M	0x03f00000
#define	CIA_PCI_WnMASK_128M	0x07f00000
#define	CIA_PCI_WnMASK_256M	0x0ff00000
#define	CIA_PCI_WnMASK_512M	0x1ff00000
#define	CIA_PCI_WnMASK_1G	0x3ff00000
#define	CIA_PCI_WnMASK_2G	0x7ff00000
#define	CIA_PCI_WnMASK_4G	0xfff00000

/*
 * Values for CIA_PCI_TnBASE
 */
#define	CIA_PCI_TnBASE_MASK	0xfffffff0
#define	CIA_PCI_TnBASE_SHIFT	2

/*
 * General CSRs
 */

#define	CIA_CSR_HAE_MEM	(CIA_CSRS + 0x400)

#define		HAE_MEM_REG1_START(x)	(((u_int32_t)(x) & 0xe0000000UL) << 0)
#define		HAE_MEM_REG1_MASK	0x1fffffffUL
#define		HAE_MEM_REG2_START(x)	(((u_int32_t)(x) & 0x0000f800UL) << 16)
#define		HAE_MEM_REG2_MASK	0x07ffffffUL
#define		HAE_MEM_REG3_START(x)	(((u_int32_t)(x) & 0x000000fcUL) << 24)
#define		HAE_MEM_REG3_MASK	0x03ffffffUL

#define	CIA_CSR_HAE_IO	(CIA_CSRS + 0x440)

#define		HAE_IO_REG1_START(x)	0UL
#define		HAE_IO_REG1_MASK	0x01ffffffUL
#define		HAE_IO_REG2_START(x)	(((u_int32_t)(x) & 0xfe000000UL) << 0)
#define		HAE_IO_REG2_MASK	0x01ffffffUL

#define	CIA_CSR_CIA_ERR	(CIA_CSRS + 0x8200)
