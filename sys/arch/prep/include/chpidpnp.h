/* $NetBSD: chpidpnp.h,v 1.1.6.2 2006/06/19 03:44:53 chap Exp $ */
/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*
 * Based on:
 * IBM Power Personal Systems Architecture: Residual Data
 * Document Number: PPS-AR-FW0001 Rev 0.5  April 3, 1996
 */

#ifndef _CHPIDPNP_H_
#define _CHPIDPNP_H_

#define ChipID_Packet	0x70	/* tag for ChipIdPack without size */

/*
 * The chipid is the vendor id followed by 4 hex digits, e.g. for IBM
 * platforms: Chip_ID=IBMxxxx.  To avoid confusion with PnP Device IDs, IBM
 * Chip IDs will begin at 0x8000.
 */
typedef enum _Chip_Type {
	Chip_MemCont = 0,
	Chip_ISABridge = 1,
	Chip_PCIBridge = 2,
	Chip_PCMCIABridge = 3,
	Chip_EISABridge = 4,
	Chip_MCABridge = 5,
	Chip_L2Cache = 6,
	Chip_PM = 7,
	Chip_IntrCont = 8,
	Chip_MiscPlanar = 9,
} Chip_Type;

typedef enum _Chip_ID {

/* Memory Controllers		Memory Controller range: IBM80xx */
Dakota = 0x8001,		/* IBM8001: IBM North/South Dakota */
Idaho = 0x8002,			/* IBM8002: IBM Idaho */
Eagle = 0x8003,			/* IBM8003: Motorola Eagle */
Kauai_Lanai = 0x8004,		/* IBM8004: IBM Kauai/Lanai */
Montana_Nevada = 0x8005,	/* IBM8005: IBM Montana/Nevada */
Union = 0x8006,			/* IBM8006: IBM Union */
Cobra_Viper = 0x8007,		/* IBM8007: IBM Cobra/Viper */
Grackle = 0x8008,		/* IBM8008: Motorola Grackle */

/* ISA Bridge chips		Bus Bridge Range: IBM81xx */
SIO_ZB = 0x8100,		/* IBM8100: Intel 82378ZB */
FireCoral = 0x8101,		/* IBM8101: IBM FireCoral */

/* PCI Bridge chips */
Python = 0x8102,		/* IBM8102: IBM Python */
DEC21050 = 0x8102,		/* IBM8103: PCI-PCI (dec 21050) */
IBM2782351 = 0x8106,		/* IBM8106: PCI-PCI */
IBM2782352 = 0x8109,		/* IBM8109: PCI-PCI352 */

/* PCMCIA Bridge Chips */
INTEL_8236SL = 0x8104,		/* IBM8104: Intel 8236SL */
RICOH_RF5C366C = 0x8105,	/* IBM8105: RICOH RF5C366C */

/* EISA Bridge Chips */
INTEL_82374 = 0x8108,		/* IBM8108: Intel 82374/82375 */

/* MCA Bridge Chips */
MCACoral = 0x8107,		/* IBM8107: 6xxxMx - T=1 MCA (servers) */

/* L2 Cache controller		L2 Controller Range: IBM82xx */
Cheyenne = 0x8200,		/* IBM8200: IBM Cheyenne */
IDT = 0x8201,			/* IBM8201: IDT */
Sony1PB = 0x8202,		/* IBM8202: Sony1PB */
Mamba = 0x8203,			/* IBM8203: IBM Mamba */
Alaska = 0x8204,		/* IBM8204: IBM Alaska */
Glance = 0x8205,		/* IBM8205: IBM Glance */
Ocelot = 0x8206,		/* IBM8206: IBM Ocelot */

/* Power management chips	PM Range: IBM83xx */
Carrera = 0x8300,		/* IBM8300: IBM Carrera */
Sig750 = 0x8301,		/* IBM8301: Signetics 87C750 */

/* Interrupt controller chips	PIC Chip range: IBM84xx */
MPIC_2 = 0x8400,		/* IBM8400: IBM MPIC-2 */

/* Miscellaneous Planar chips	MISC Chip Range: IBM8Fxx */
DallasRTC = 0x8F00,		/* IBM8F00: Dallas 1385 compatible */
Dallas1585 = 0x8F01,		/* IBM8F01: Dallas 1585 compatible */
Timer8254 = 0x8F10,		/* IBM8F10: 8254-compatible timer */
HarddiskLt = 0x8FF0,		/* IBM8FF0: Op Panel HD light */
} Chip_ID;

/* small tag = 0x7n with n bytes.  Type == 1 for ChipID 
 * VendorID0:
 *  bit(7) = 0
 *  bits(6:2) 1st character in compressed ASCII
 *  bits(1:0) 2nd character in compressed ASCII bits(4:3)
 * VendorID1:
 *  bits(7:5) 2nd character in compressed ASCII bits(2:0)
 *  bits(4:0) 3rd character in compressed ASCII
 * Name:
 * Example: IBM8001
 *  I,B,M = 01001, 00010, 01101
 *  bytes 0,1 = 00100100,01001101
 *            =   2   4    4   D
 * byte0 = 24  byte1 = 4D byte2 = 01 byte3 = 80
 */
typedef struct _ChipIDPack {
	unsigned char Tag;
	unsigned char Type;
	unsigned char VendorID0;
	unsigned char VendorID1;
	unsigned char Name[2];
} ChipIDPack;

#endif /* _CHPIDPNP_H_ */
