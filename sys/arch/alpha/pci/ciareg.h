/* $NetBSD: ciareg.h,v 1.7.2.4 1997/08/12 05:56:07 cgd Exp $ */

/*
 * Copyright Notice:
 *
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
 *
 * License:
 *
 * This License applies to this software ("Software"), created
 * by Christopher G. Demetriou ("Author").
 *
 * You may use, copy, modify and redistribute this Software without
 * charge, in either source code form, binary form, or both, on the
 * following conditions:
 *
 * 1.  (a) Binary code: (i) a complete copy of the above copyright notice
 * must be included within each copy of the Software in binary code form,
 * and (ii) a complete copy of the above copyright notice and all terms
 * of this License as presented here must be included within each copy of
 * all documentation accompanying or associated with binary code, in any
 * medium, along with a list of the software modules to which the license
 * applies.
 *
 * (b) Source Code: A complete copy of the above copyright notice and all
 * terms of this License as presented here must be included within: (i)
 * each copy of the Software in source code form, and (ii) each copy of
 * all accompanying or associated documentation, in any medium.
 *
 * 2. The following Acknowledgment must be used in communications
 * involving the Software as described below:
 *
 *      This product includes software developed by
 *      Christopher G. Demetriou for the NetBSD Project.
 *
 * The Acknowledgment must be conspicuously and completely displayed
 * whenever the Software, or any software, products or systems containing
 * the Software, are mentioned in advertising, marketing, informational
 * or publicity materials of any kind, whether in print, electronic or
 * other media (except for information provided to support use of
 * products containing the Software by existing users or customers).
 *
 * 3. The name of the Author may not be used to endorse or promote
 * products derived from this Software without specific prior written
 * permission (conditions (1) and (2) above are not considered
 * endorsement or promotion).
 *
 * 4.  This license applies to: (a) all copies of the Software, whether
 * partial or whole, original or modified, and (b) your actions, and the
 * actions of all those who may act on your behalf.  All uses not
 * expressly permitted are reserved to the Author.
 *
 * 5.  Disclaimer.  THIS SOFTWARE IS MADE AVAILABLE BY THE AUTHOR TO THE
 * PUBLIC FOR FREE AND "AS IS.''  ALL USERS OF THIS FREE SOFTWARE ARE
 * SOLELY AND ENTIRELY RESPONSIBLE FOR THEIR OWN CHOICE AND USE OF THIS
 * SOFTWARE FOR THEIR OWN PURPOSES.  BY USING THIS SOFTWARE, EACH USER
 * AGREES THAT THE AUTHOR SHALL NOT BE LIABLE FOR DAMAGES OF ANY KIND IN
 * RELATION TO ITS USE OR PERFORMANCE.
 *
 * 6.  If you have a special need for a change in one or more of these
 * license conditions, please contact the Author via electronic mail to
 *
 *     cgd@NetBSD.ORG
 *
 * or via the contact information on
 *
 *     http://www.NetBSD.ORG/People/Pages/cgd.html
 */

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
 * Taken from EC-QE18B-TE.
 */

#define	REGVAL(r)	(*(volatile int32_t *)ALPHA_PHYS_TO_K0SEG(r))

/*
 * Base addresses
 * XXX CLEAN UP: split into logical sections, then offsets into section.
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

#define	CIA_CSR_CIA_REV	(CIA_CSRS + 0x080)

#define		CIA_REV_REV_MASK	0x000000ff
#define		CIA_REV_REV_SHIFT	0

#define		CIA_REV_ALT_MEM		0x00000100	/* 21172 only */

#define	CIA_CSR_PCI_LAT (CIA_CSRS + 0x0c0)

#define	CIA_CSR_CIA_CTRL (CIA_CSRS + 0x100)

#define	CIA_CSR_CIA_CNFG (CIA_CSRS + 0x140)		/* 21172 only */

#define		CIA_CNFG_IOA_BWEN	0x00000001	/* 21172 only */

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

#define	CIA_CSR_CFG	(CIA_CSRS + 0x480)

#define	CIA_CSR_CACK_EN	(CIA_CSRS + 0x600)

#define	CIA_CSR_CIA_ERR	(CIA_CSRS + 0x8200)
