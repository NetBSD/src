/*	$NetBSD: miidevs.h,v 1.34 2002/02/10 17:09:08 wiz Exp $	*/

/*
 * THIS FILE AUTOMATICALLY GENERATED.  DO NOT EDIT.
 *
 * generated from:
 *	NetBSD: miidevs,v 1.34 2002/02/10 17:08:24 wiz Exp 
 */

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * List of known MII OUIs.
 * For a complete list see http://standards.ieee.org/regauth/oui/
 *
 * XXX Vendors do obviously not agree how OUIs (24 bit) are mapped
 * to the 22 bits available in the id registers.
 * IEEE 802.3u-1995, subclause 22.2.4.3.1, figure 22-12, depicts the right
 * mapping; the bit positions are defined in IEEE 802-1990, figure 5.2.
 * (There is a formal 802.3 interpretation, number 1-07/98 of July 09 1998,
 * about this.)
 * The MII_OUI() macro in "mii.h" reflects this.
 * If a vendor uses a different mapping, an "xx" prefixed OUI is defined here
 * which is mangled accordingly to compensate.
 */

#define	MII_OUI_ALTIMA	0x0010a9	/* Altima Communications */
#define	MII_OUI_AMD	0x00001a	/* Advanced Micro Devices */
#define	MII_OUI_BROADCOM	0x001018	/* Broadcom Corporation */
#define	MII_OUI_DAVICOM	0x00606e	/* Davicom Semiconductor */
#define	MII_OUI_ENABLESEMI	0x0010dd	/* Enable Semiconductor */
#define	MII_OUI_ICS	0x00a0be	/* Integrated Circuit Systems */
#define	MII_OUI_INTEL	0x00aa00	/* Intel */
#define	MII_OUI_LEVEL1	0x00207b	/* Level 1 */
#define	MII_OUI_MARVELL	0x005043	/* Marvell Semiconductor */
#define	MII_OUI_MYSON	0x00c0b4	/* Myson Technology */
#define	MII_OUI_NATSEMI	0x080017	/* National Semiconductor */
#define	MII_OUI_PMCSIERRA	0x00e004	/* PMC-Sierra */
#define	MII_OUI_QUALSEMI	0x006051	/* Quality Semiconductor */
#define	MII_OUI_SEEQ	0x00a07d	/* Seeq */
#define	MII_OUI_SIS	0x00e006	/* Silicon Integrated Systems */
#define	MII_OUI_TI	0x080028	/* Texas Instruments */
#define	MII_OUI_TSC	0x00c039	/* TDK Semiconductor */
#define	MII_OUI_XAQTI	0x00e0ae	/* XaQti Corp. */

/* in the 79c873, AMD uses another OUI (which matches reversed Davicom!) */
#define	MII_OUI_xxAMD	0x000676	/* Advanced Micro Devices */

/* Some Intel 82553's use an alternative OUI. */
#define	MII_OUI_xxINTEL	0x001f00	/* Intel */

/* bad bitorder (bits "g" and "h" (= MSBs byte 1) lost) */
#define	MII_OUI_yyAMD	0x000058	/* Advanced Micro Devices */
#define	MII_OUI_xxBROADCOM	0x000818	/* Broadcom Corporation */
#define	MII_OUI_yyINTEL	0x005500	/* Intel */
#define	MII_OUI_xxMYSON	0x00032d	/* Myson Technology */
#define	MII_OUI_xxNATSEMI	0x1000e8	/* National Semiconductor */
#define	MII_OUI_xxQUALSEMI	0x00068a	/* Quality Semiconductor */
#define	MII_OUI_xxTSC	0x00039c	/* TDK Semiconductor */

/* bad byteorder (bits "q" and "r" (= LSBs byte 3) lost) */
#define	MII_OUI_xxLEVEL1	0x782000	/* Level 1 */
#define	MII_OUI_xxXAQTI	0xace000	/* XaQti Corp. */

/* Don't know what's going on here. */
#define	MII_OUI_xxDAVICOM	0x000602	/* Davicom Semiconductor */
#define	MII_OUI_xxPMCSIERRA	0x0009c0	/* PMC-Sierra */

#define	MII_OUI_xxMARVELL	0x000ac2	/* Marvell Semiconductor */

/*
 * List of known models.  Grouped by oui.
 */

/* Altima Communications PHYs */
/* Don't know the model for ACXXX */
#define	MII_MODEL_ALTIMA_ACXXX	0x0001
#define	MII_STR_ALTIMA_ACXXX	"ACXXX 10/100 media interface"
#define	MII_MODEL_ALTIMA_AC101	0x0021
#define	MII_STR_ALTIMA_AC101	"AC101 10/100 media interface"

/* Advanced Micro Devices PHYs */
#define	MII_MODEL_xxAMD_79C873	0x0000
#define	MII_STR_xxAMD_79C873	"Am79C873 10/100 media interface"
#define	MII_MODEL_yyAMD_79C972_10T	0x0001
#define	MII_STR_yyAMD_79C972_10T	"Am79C972 internal 10BASE-T interface"
#define	MII_MODEL_yyAMD_79c973phy	0x0036
#define	MII_STR_yyAMD_79c973phy	"Am79C973 internal 10/100 media interface"
#define	MII_MODEL_yyAMD_79c901	0x0037
#define	MII_STR_yyAMD_79c901	"Am79C901 10BASE-T interface"
#define	MII_MODEL_yyAMD_79c901home	0x0039
#define	MII_STR_yyAMD_79c901home	"Am79C901 HomePNA 1.0 interface"

/* Broadcom Corp. PHYs */
#define	MII_MODEL_xxBROADCOM_3C905B	0x0012
#define	MII_STR_xxBROADCOM_3C905B	"Broadcom 3c905B internal PHY"
#define	MII_MODEL_xxBROADCOM_3C905C	0x0017
#define	MII_STR_xxBROADCOM_3C905C	"Broadcom 3c905C internal PHY"
#define	MII_MODEL_xxBROADCOM_BCM5201	0x0021
#define	MII_STR_xxBROADCOM_BCM5201	"BCM5201 10/100 media interface"
#define	MII_MODEL_xxBROADCOM_BCM5221	0x001e
#define	MII_STR_xxBROADCOM_BCM5221	"BCM5221 10/100 media interface"
#define	MII_MODEL_BROADCOM_BCM5400	0x0004
#define	MII_STR_BROADCOM_BCM5400	"BCM5400 1000BASE-T media interface"
#define	MII_MODEL_BROADCOM_BCM5401	0x0005
#define	MII_STR_BROADCOM_BCM5401	"BCM5401 1000BASE-T media interface"
#define	MII_MODEL_BROADCOM_BCM5411	0x0007
#define	MII_STR_BROADCOM_BCM5411	"BCM5411 1000BASE-T media interface"

/* Davicom Semiconductor PHYs */
#define	MII_MODEL_xxDAVICOM_DM9101	0x0000
#define	MII_STR_xxDAVICOM_DM9101	"DM9101 10/100 media interface"

/* Integrated Circuit Systems PHYs */
#define	MII_MODEL_ICS_1890	0x0002
#define	MII_STR_ICS_1890	"ICS1890 10/100 media interface"
#define	MII_MODEL_ICS_1893	0x0004
#define	MII_STR_ICS_1893	"ICS1893 10/100 media interface"

/* Intel PHYs */
#define	MII_MODEL_xxINTEL_I82553	0x0000
#define	MII_STR_xxINTEL_I82553	"i82553 10/100 media interface"
#define	MII_MODEL_yyINTEL_I82555	0x0015
#define	MII_STR_yyINTEL_I82555	"i82555 10/100 media interface"
#define	MII_MODEL_yyINTEL_I82562EH	0x0017
#define	MII_STR_yyINTEL_I82562EH	"i82562EH HomePNA interface"
#define	MII_MODEL_yyINTEL_I82562EM	0x0032
#define	MII_STR_yyINTEL_I82562EM	"i82562EM 10/100 media interface"
#define	MII_MODEL_yyINTEL_I82562ET	0x0033
#define	MII_STR_yyINTEL_I82562ET	"i82562ET 10/100 media interface"
#define	MII_MODEL_yyINTEL_I82553	0x0035
#define	MII_STR_yyINTEL_I82553	"i82553 10/100 media interface"

/* Level 1 PHYs */
#define	MII_MODEL_xxLEVEL1_LXT970	0x0000
#define	MII_STR_xxLEVEL1_LXT970	"LXT970 10/100 media interface"
#define	MII_MODEL_LEVEL1_LXT1000_OLD	0x0003
#define	MII_STR_LEVEL1_LXT1000_OLD	"LXT1000 1000BASE-T media interface"
#define	MII_MODEL_LEVEL1_LXT1000	0x000c
#define	MII_STR_LEVEL1_LXT1000	"LXT1000 1000BASE-T media interface"

/* Marvell Semiconductor PHYs */
#define	MII_MODEL_xxMARVELL_E1000_3	0x0003
#define	MII_STR_xxMARVELL_E1000_3	"Marvell 88E1000 Gigabit PHY"
#define	MII_MODEL_xxMARVELL_E1000_5	0x0005
#define	MII_STR_xxMARVELL_E1000_5	"Marvell 88E1000 Gigabit PHY"

/* Myson Technology PHYs */
#define	MII_MODEL_xxMYSON_MTD972	0x0000
#define	MII_STR_xxMYSON_MTD972	"MTD972 10/100 media interface"

/* National Semiconductor PHYs */
#define	MII_MODEL_xxNATSEMI_DP83840	0x0000
#define	MII_STR_xxNATSEMI_DP83840	"DP83840 10/100 media interface"
#define	MII_MODEL_xxNATSEMI_DP83843	0x0001
#define	MII_STR_xxNATSEMI_DP83843	"DP83843 10/100 media interface"
#define	MII_MODEL_xxNATSEMI_DP83815	0x0002
#define	MII_STR_xxNATSEMI_DP83815	"DP83815 10/100 media interface"
#define	MII_MODEL_xxNATSEMI_DP83891	0x0005
#define	MII_STR_xxNATSEMI_DP83891	"DP83891 1000BASE-T media interface"
#define	MII_MODEL_xxNATSEMI_DP83861	0x0006
#define	MII_STR_xxNATSEMI_DP83861	"DP83861 1000BASE-T media interface"

/* PMC Sierra PHYs */
#define	MII_MODEL_xxPMCSIERRA_PM8351	0x0000
#define	MII_STR_xxPMCSIERRA_PM8351	"PM8351 OctalPHY Gigabit interface"

/* Quality Semiconductor PHYs */
#define	MII_MODEL_xxQUALSEMI_QS6612	0x0000
#define	MII_STR_xxQUALSEMI_QS6612	"QS6612 10/100 media interface"

/* Seeq PHYs */
#define	MII_MODEL_SEEQ_80220	0x0003
#define	MII_STR_SEEQ_80220	"Seeq 80220 10/100 media interface"
#define	MII_MODEL_SEEQ_84220	0x0004
#define	MII_STR_SEEQ_84220	"Seeq 84220 10/100 media interface"
#define	MII_MODEL_SEEQ_80225	0x0008
#define	MII_STR_SEEQ_80225	"Seeq 80225 10/100 media interface"

/* Silicon Integrated Systems PHYs */
#define	MII_MODEL_SIS_900	0x0000
#define	MII_STR_SIS_900	"SiS 900 10/100 media interface"

/* Texas Instruments PHYs */
#define	MII_MODEL_TI_TLAN10T	0x0001
#define	MII_STR_TI_TLAN10T	"ThunderLAN 10BASE-T media interface"
#define	MII_MODEL_TI_100VGPMI	0x0002
#define	MII_STR_TI_100VGPMI	"ThunderLAN 100VG-AnyLan media interface"
#define	MII_MODEL_TI_TNETE2101	0x0003
#define	MII_STR_TI_TNETE2101	"TNETE2101 media interface"

/* TDK Semiconductor PHYs */
#define	MII_MODEL_xxTSC_78Q2120	0x0014
#define	MII_STR_xxTSC_78Q2120	"78Q2120 10/100 media interface"
#define	MII_MODEL_xxTSC_78Q2121	0x0015
#define	MII_STR_xxTSC_78Q2121	"78Q2121 100BASE-TX media interface"

/* XaQti Corp. PHYs */
#define	MII_MODEL_xxXAQTI_XMACII	0x0000
#define	MII_STR_xxXAQTI_XMACII	"XaQti Corp. XMAC II gigabit interface"
