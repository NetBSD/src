/*	$NetBSD: pcmciadevs_data.h,v 1.2 1998/07/19 17:30:02 christos Exp $	*/

/*
 * THIS FILE AUTOMATICALLY GENERATED.  DO NOT EDIT.
 *
 * generated from:
 *	NetBSD: pcmciadevs,v 1.1 1998/07/19 17:28:17 christos Exp 
 */

/*
 * Copyright (c) 1998, Christos Zoulas
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christos Zoulas
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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

struct pcmcia_knowndev pcmcia_knowndevs[] = {
	{
	    PCMCIA_VENDOR_ADAPTEC, PCMCIA_PRODUCT_ADAPTEC_APA1460_1,
	    PCMCIA_CIS_ADAPTEC_APA1460_1,
	    0,
	    "Adaptec Corporation",
	    "IBM Home and Away Modem"	},
	},
	{
	    PCMCIA_VENDOR_ADAPTEC, PCMCIA_PRODUCT_ADAPTEC_APA1460_2,
	    PCMCIA_CIS_ADAPTEC_APA1460_2,
	    0,
	    "Adaptec Corporation",
	    "IBM Home and Away Modem"	},
	},
	{
	    PCMCIA_VENDOR_3COM, PCMCIA_PRODUCT_3COM_3C562,
	    PCMCIA_CIS_3COM_3C562,
	    0,
	    "3Com Corporation",
	    "3Com 3c562 33.6 Modem/10Mbps Ethernet"	},
	},
	{
	    PCMCIA_VENDOR_3COM, PCMCIA_PRODUCT_3COM_3C589,
	    PCMCIA_CIS_3COM_3C589,
	    0,
	    "3Com Corporation",
	    "3Com 3c562 33.6 Modem/10Mbps Ethernet"	},
	},
	{
	    PCMCIA_VENDOR_DAYNA, PCMCIA_PRODUCT_DAYNA_COMMUNICARD_E,
	    PCMCIA_CIS_DAYNA_COMMUNICARD_E,
	    0,
	    "Dayna Corporation",
	    "Linksys EthernetCard"	},
	},
	{
	    PCMCIA_VENDOR_MOTOROLA, PCMCIA_PRODUCT_MOTOROLA_POWER144,
	    PCMCIA_CIS_MOTOROLA_POWER144,
	    0,
	    "Motorola Corporation",
	    "Adaptec APA-1460/B SCSI Host Adapter"	},
	},
	{
	    PCMCIA_VENDOR_MOTOROLA, PCMCIA_PRODUCT_MOTOROLA_PM100C,
	    PCMCIA_CIS_MOTOROLA_PM100C,
	    0,
	    "Motorola Corporation",
	    "Adaptec APA-1460/B SCSI Host Adapter"	},
	},
	{
	    PCMCIA_VENDOR_IBM, PCMCIA_PRODUCT_IBM_INFOMOVER,
	    PCMCIA_CIS_IBM_INFOMOVER,
	    0,
	    "IBM Corporation",
	    "Adaptec APA-1460/A SCSI Host Adapter"	},
	},
	{
	    PCMCIA_VENDOR_IBM, PCMCIA_PRODUCT_IBM_HOME_AND_AWAY,
	    PCMCIA_CIS_IBM_HOME_AND_AWAY,
	    0,
	    "IBM Corporation",
	    "Adaptec APA-1460/A SCSI Host Adapter"	},
	},
	{
	    PCMCIA_VENDOR_IODATA, PCMCIA_PRODUCT_IODATA_PCLAT,
	    PCMCIA_CIS_IODATA_PCLAT,
	    0,
	    "I-O DATA",
	    "Linksys Combo EthernetCard"	},
	},
	{
	    PCMCIA_VENDOR_LINKSYS, PCMCIA_PRODUCT_LINKSYS_ECARD,
	    PCMCIA_CIS_LINKSYS_ECARD,
	    0,
	    "Linksys Corporation",
	    "I-O DATA PCLA/T"	},
	},
	{
	    PCMCIA_VENDOR_LINKSYS, PCMCIA_PRODUCT_LINKSYS_COMBO_ECARD,
	    PCMCIA_CIS_LINKSYS_COMBO_ECARD,
	    0,
	    "Linksys Corporation",
	    "I-O DATA PCLA/T"	},
	},
	{
	    PCMCIA_VENDOR_MEGAHERTZ, PCMCIA_PRODUCT_MEGAHERTZ_XJ4288,
	    PCMCIA_CIS_MEGAHERTZ_XJ4288,
	    0,
	    "Megahertz Corporation",
	    "3Com 3c589 10Mbps Ethernet"	},
	},
	{
	    PCMCIA_VENDOR_MEGAHERTZ2, PCMCIA_PRODUCT_MEGAHERTZ2_XJACK,
	    PCMCIA_CIS_MEGAHERTZ2_XJACK,
	    0,
	    "Megahertz Corporation",
	    "National Semiconductor InfoMover"	},
	},
	{
	    PCMCIA_VENDOR_USROBOTICS, PCMCIA_PRODUCT_USROBOTICS_WORLDPORT144,
	    PCMCIA_CIS_USROBOTICS_WORLDPORT144,
	    0,
	    "US Robotics Corporation",
	    "Motorola Personal Messenger 100C CDPD Modem"	},
	},
	{
	    PCMCIA_VENDOR_SOCKET, PCMCIA_PRODUCT_SOCKET_PAGECARD,
	    PCMCIA_CIS_SOCKET_PAGECARD,
	    0,
	    "Socket Communications",
	    "Dayna CommuniCard E"	},
	},
	{
	    PCMCIA_VENDOR_TDK, PCMCIA_PRODUCT_TDK_LAK_CD021BX,
	    PCMCIA_CIS_TDK_LAK_CD021BX,
	    0,
	    "TDK Corporation",
	    "Motorola Power 14.4 Modem"	},
	},
	{
	    PCMCIA_VENDOR_TDK, PCMCIA_PRODUCT_TDK_DFL9610,
	    PCMCIA_CIS_TDK_DFL9610,
	    0,
	    "TDK Corporation",
	    "Motorola Power 14.4 Modem"	},
	},
	{
	    PCMCIA_VENDOR_UNKNOWN, PCMCIA_PRODUCT_MEGAHERTZ_XJ2288,
	    PCMCIA_CIS_MEGAHERTZ_XJ2288,
	    0,
	    "Megahertz Corporation",
	    "Megahertz XJ2288 Modem"	},
	},
	{
	    PCMCIA_VENDOR_UNKNOWN, PCMCIA_PRODUCT_PREMAX_PE200,
	    PCMCIA_CIS_PREMAX_PE200,
	    0,
	    "",
	    "PreMax PE-200"	},
	},
	{
	    PCMCIA_VENDOR_UNKNOWN, PCMCIA_PRODUCT_PLANET_SMARTCOM2000,
	    PCMCIA_CIS_PLANET_SMARTCOM2000,
	    0,
	    "",
	    "Planet SmartCOM 2000"	},
	},
	{
	    PCMCIA_VENDOR_UNKNOWN, PCMCIA_PRODUCT_DLINK_DE650,
	    PCMCIA_CIS_DLINK_DE650,
	    0,
	    "",
	    "D-Link DE-650"	},
	},
	{
	    PCMCIA_VENDOR_UNKNOWN, PCMCIA_PRODUCT_RPTI_EP401,
	    PCMCIA_CIS_RPTI_EP401,
	    0,
	    "",
	    "RPTI EP401"	},
	},
	{
	    PCMCIA_VENDOR_UNKNOWN, PCMCIA_PRODUCT_ACCTON_EN2212,
	    PCMCIA_CIS_ACCTON_EN2212,
	    0,
	    "",
	    "Accton EN2212"	},
	},
	{
	    PCMCIA_VENDOR_UNKNOWN, PCMCIA_PRODUCT_DIGITAL_DEPCMBA,
	    PCMCIA_CIS_DIGITAL_DEPCMBA,
	    0,
	    "",
	    "Digital DEPCM-BA"	},
	},
	{
	    PCMCIA_VENDOR_UNKNOWN, PCMCIA_PRODUCT_YEDATA_EXTERNAL_FDD,
	    PCMCIA_CIS_YEDATA_EXTERNAL_FDD,
	    0,
	    "",
	    "Y-E DATA External FDD"	},
	},
	{
	    PCMCIA_VENDOR_UNKNOWN, PCMCIA_PRODUCT_DIGITAL_DEPCMXX,
	    PCMCIA_CIS_DIGITAL_DEPCMXX,
	    0,
	    "",
	    "DEC DEPCM-BA"	},
	},
	{
	    PCMCIA_VENDOR_IBM, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "IBM Corporation",
	    NULL,
	},
	{
	    PCMCIA_VENDOR_MOTOROLA, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "Motorola Corporation",
	    NULL,
	},
	{
	    PCMCIA_VENDOR_3COM, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "3Com Corporation",
	    NULL,
	},
	{
	    PCMCIA_VENDOR_MEGAHERTZ, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "Megahertz Corporation",
	    NULL,
	},
	{
	    PCMCIA_VENDOR_SOCKET, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "Socket Communications",
	    NULL,
	},
	{
	    PCMCIA_VENDOR_TDK, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "TDK Corporation",
	    NULL,
	},
	{
	    PCMCIA_VENDOR_USROBOTICS, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "US Robotics Corporation",
	    NULL,
	},
	{
	    PCMCIA_VENDOR_MEGAHERTZ2, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "Megahertz Corporation",
	    NULL,
	},
	{
	    PCMCIA_VENDOR_ADAPTEC, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "Adaptec Corporation",
	    NULL,
	},
	{
	    PCMCIA_VENDOR_LINKSYS, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "Linksys Corporation",
	    NULL,
	},
	{
	    PCMCIA_VENDOR_DAYNA, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "Dayna Corporation",
	    NULL,
	},
	{
	    PCMCIA_VENDOR_IODATA, 0,
	    PCMCIA_KNOWNDEV_NOPROD,
	    PCMCIA_CIS_INVALID,
	    "I-O DATA",
	    NULL,
	},
	{ 0, 0, { NULL, NULL, NULL, NULL }, 0, NULL, NULL, }
};
