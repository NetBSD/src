/* $NetBSD: pmb.h,v 1.4 2020/07/30 21:25:43 uwe Exp $ */
/*
 * Copyright (c) 2020 Valery Ushakov
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH3_PMB_H_
#define	_SH3_PMB_H_
/*
 * ST40 Privileged Mapping Buffer (PMB)
 *
 * Original SuperH can handle only 29-bit external memory space.
 * "The physical address space is permanently mapped onto 29-bit
 * external memory space."  See <sh3/cpu.h>.
 *
 * ST40-200, ST40-300 and ST40-500 have "space enhanced" SE mode where
 * the mapping from the physical address space P1 and P2 segments to
 * the 32-bit external memory space is defined via 16-entry PMB.
 */


/* on ST40-200 and ST40-500 SE bit is in MMUCR */
#define ST40_MMUCR_SE			0x00000010


/* Physical address space control register (ST4-300) */
#define ST40_PASCR			0xff000070
#define   ST40_PASCR_UB_MASK		  0x0000000f
#define   ST40_PASCR_SE			  0x80000000

#define   ST40_PASCR_BITS			\
		"\177\020"			\
		"b\037"  "SE\0"			\
		"f\0\04" "UB\0"


/* Memory-mapped PMB */
#define ST40_PMB_ENTRY			16

#define ST40_PMB_E_MASK			0x00000f00
#define ST40_PMB_E_SHIFT		8


/* PMB Address Array */
#define ST40_PMB_AA			0xf6100000
#define   ST40_PMB_AA_V			  0x00000100
#define   ST40_PMB_AA_VPN_MASK		  0xff000000
#define   ST40_PMB_AA_VPN_SHIFT		  24

#define   ST40_PMB_AA_BITS			\
	  "\177\020"				\
	  "f\030\010" "VPN\0"			\
	  "b\010"     "V\0"


/* PMB Data Array */
#define   ST40_PMB_DA			0xf7100000
#define   ST40_PMB_DA_WT		0x00000001
#define   ST40_PMB_DA_C			0x00000008
#define   ST40_PMB_DA_UB		0x00000200
#define   ST40_PMB_DA_SZ_MASK		0x00000090
#define     ST40_PMB_DA_SZ_16M		0x00000000
#define     ST40_PMB_DA_SZ_64M		0x00000010
#define     ST40_PMB_DA_SZ_128M		0x00000080
#define     ST40_PMB_DA_SZ_512M		0x00000090
#define   ST40_PMB_DA_V			0x00000100
#define   ST40_PMB_DA_PPN_MASK		0xff000000
#define   ST40_PMB_DA_PPN_SHIFT		24

/*
 * size field is not continuous hence the kludgy list with all the
 * possible junk bits in the middle.
 */
#define   ST40_PMB_DA_BITS			\
	  "\177\020"				\
	  "f\030\010" "PPN\0"			\
	  "b\010"     "V\0"			\
	  "F\04\04"   "\0"			\
	    ":\017"   "512M\0"			\
	    ":\016"   "128M\0"			\
	    ":\015"   "512M\0"			\
	    ":\014"   "128M\0"			\
	    ":\013"   "512M\0"			\
	    ":\012"   "128M\0"			\
	    ":\011"   "512M\0"			\
	    ":\010"   "128M\0"			\
	    ":\007"    "64M\0"			\
	    ":\006"    "16M\0"			\
	    ":\005"    "64M\0"			\
	    ":\004"    "16M\0"			\
	    ":\003"    "64M\0"			\
	    ":\002"    "16M\0"			\
	    ":\001"    "64M\0"			\
	    ":\000"    "16M\0"			\
	  "b\011"     "UB\0"			\
	  "b\03"      "C\0"			\
	  "F\0\01"    "\0"			\
	    ":\01"    "WT\0"			\
	    ":\0"     "CB\0"


#ifndef _LOCORE
void st40_pmb_init(int);
#endif	/* !_LOCORE */

#endif	/* !_SH3_PMB_H_ */
