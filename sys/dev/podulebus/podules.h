/*	$NetBSD: podules.h,v 1.10 2002/05/22 23:46:54 bjh21 Exp $	*/

/*
 * THIS FILE AUTOMATICALLY GENERATED.  DO NOT EDIT.
 *
 * generated from:
 *	NetBSD: podules,v 1.12 2002/05/22 23:46:36 bjh21 Exp 
 */

/*
 * Copyright (c) 1996 Mark Brinicombe
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
 *      This product includes software developed by Mark Brinicombe
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

/*
 * List of known podule manufacturers
 */

#define	MANUFACTURER_ACORN	0x0000		/* Acorn Computers */
#define	MANUFACTURER_ACORNUSA	0x0001		/* Acorn Computers (USA) */
#define	MANUFACTURER_OLIVETTI	0x0002		/* Olivetti */
#define	MANUFACTURER_WATFORD	0x0003		/* Watford Electronics */
#define	MANUFACTURER_CCONCEPTS	0x0004		/* Computer Concepts */
#define	MANUFACTURER_IINTERFACES	0x0005		/* Intelligent Interfaces */
#define	MANUFACTURER_CAMAN	0x0006		/* Caman */
#define	MANUFACTURER_ARMADILLO	0x0007		/* Armadillo Systems */
#define	MANUFACTURER_SOFTOPTION	0x0008		/* Soft Option */
#define	MANUFACTURER_WILDVISION	0x0009		/* Wild Vision */
#define	MANUFACTURER_ANGLOCOMPUTERS	0x000a		/* Anglo Computers */
#define	MANUFACTURER_RESOURCE	0x000b		/* Resource */
/* RISC iX: #define XCB_COMPANY_ALLIEDINTERACTIVE 12 */
#define	MANUFACTURER_HCCS	0x000c		/* HCCS */
#define	MANUFACTURER_MUSBURYCONSULT	0x000d		/* Musbury Consultants */
#define	MANUFACTURER_GNOME	0x000e		/* Gnome */
#define	MANUFACTURER_AANDGELEC	0x000f		/* A and G Electronics */
#define	MANUFACTURER_SPACETECH	0x0010		/* Spacetech */
#define	MANUFACTURER_ATOMWIDE	0x0011		/* Atomwide */
#define	MANUFACTURER_SYNTEC	0x0012		/* Syntec */
#define	MANUFACTURER_EMR	0x0013		/* ElectroMusic Research */
#define	MANUFACTURER_MILLIPEDE	0x0014		/* Millipede */
#define	MANUFACTURER_VIDEOELEC	0x0015		/* Video Electronics */
#define	MANUFACTURER_BRAINSOFT	0x0016		/* Brainsoft */
/* RISC iX: #define XCB_COMPANY_ASP 23 */
#define	MANUFACTURER_ATOMWIDE2	0x0017		/* Atomwide */
#define	MANUFACTURER_LENDAC	0x0018		/* Lendac Data Systems */
#define	MANUFACTURER_CAMMICROSYS	0x0019		/* Cambridge Micro Systems */
/* RISC iX: #define XCB_COMPANY_JOHNBALANCECOMPUTING 26 */
#define	MANUFACTURER_LINGENUITY	0x001a		/* Lingenuity */
#define	MANUFACTURER_SIPLAN	0x001b		/* Siplan Electronics Research */
#define	MANUFACTURER_SCIFRONTIERS	0x001c		/* Science Frontiers */
#define	MANUFACTURER_PINEAPPLE	0x001d		/* Pineapple Software */
#define	MANUFACTURER_TECHNOMATIC	0x001e		/* Technomatic */
#define	MANUFACTURER_IRLAM	0x001f		/* Irlam Instruments */
#define	MANUFACTURER_NEXUS	0x0020		/* Nexus Electronics */
#define	MANUFACTURER_OAK	0x0021		/* Oak Solutions */
#define	MANUFACTURER_HUGHSYMONS	0x0022		/* Hugh Symons */
#define	MANUFACTURER_BEEBUG	0x0023		/* BEEBUG (RISC Developments) */
#define	MANUFACTURER_TEKNOMUSIK	0x0024		/* Teknomusik */
#define	MANUFACTURER_REELTIME	0x0025		/* Reel Time */
#define	MANUFACTURER_PRES	0x0026		/* PRES */
#define	MANUFACTURER_DIGIHURST	0x0027		/* Digihurst */
#define	MANUFACTURER_SGBCOMPSERV	0x0028		/* SGB Computer Services */
#define	MANUFACTURER_SJ	0x0029		/* SJ Research */
#define	MANUFACTURER_PHOBOX	0x002a		/* Phobox Electronics */
#define	MANUFACTURER_MORLEY	0x002b		/* Morley Electronics */
#define	MANUFACTURER_RACINGCAR	0x002c		/* Raching Car Computers */
#define	MANUFACTURER_HCCS2	0x002d		/* HCCS */
#define	MANUFACTURER_LINDIS	0x002e		/* Lindis International */
#define	MANUFACTURER_CCC	0x002f		/* Computer Control Consultants */
#define	MANUFACTURER_UNILAB	0x0030		/* Unilab */
#define	MANUFACTURER_SEFANFROHLING	0x0031		/* Sefan Frohling */
#define	MANUFACTURER_ROMBO	0x0032		/* Rombo Productions */
#define	MANUFACTURER_3SL	0x0033		/* 3SL */
#define	MANUFACTURER_DELTRONICS	0x0034		/* Deltronics */
/* RISC iX: #define XCB_COMPANY_PCARNOLDTECHNICALSERVICES 53 */
#define	MANUFACTURER_VTI	0x0035		/* Vertical Twist */
#define	MANUFACTURER_SIMIS	0x0036		/* Simis */
#define	MANUFACTURER_DTSOFT	0x0037		/* D.T. Software */
#define	MANUFACTURER_ARMINTERFACES	0x0038		/* ARM Interfaces */
#define	MANUFACTURER_BIA	0x0039		/* BIA */
#define	MANUFACTURER_CUMANA	0x003a		/* Cumana */
#define	MANUFACTURER_IOTA	0x003b		/* Iota */
#define	MANUFACTURER_ICS	0x003c		/* Ian Copestake Software */
#define	MANUFACTURER_BAILDON	0x003d		/* Baildon Electronics */
#define	MANUFACTURER_CSD	0x003e		/* CSD */
#define	MANUFACTURER_SERIALPORT	0x003f		/* Serial Port */
#define	MANUFACTURER_CADSOFT	0x0040		/* CADsoft */
#define	MANUFACTURER_ARXE	0x0041		/* ARXE */
#define	MANUFACTURER_ALEPH1	0x0042		/* Aleph 1 */
#define	MANUFACTURER_ICUBED	0x0046		/* I-Cubed */
#define	MANUFACTURER_BRINI	0x0050		/* Brini */
#define	MANUFACTURER_ANT	0x0053		/* ANT */
#define	MANUFACTURER_CASTLE	0x0055		/* Castle Technology */
#define	MANUFACTURER_ALSYSTEMS	0x005b		/* Alsystems */
#define	MANUFACTURER_SIMTEC	0x005f		/* Simtec Electronics */
#define	MANUFACTURER_YES	0x0060		/* Yellowstone Educational Solutions */
#define	MANUFACTURER_MCS	0x0063		/* MCS */
#define	MANUFACTURER_EESOX	0x0064		/* EESOX */

/*
 * List of known podules.
 */

#define	PODULE_ACORN_SCSI	0x0002		/* Acorn SCSI interface */
#define	PODULE_ETHER1	0x0003		/* Ether1 interface */
#define	PODULE_ROMRAM	0x0005		/* ROM/RAM podule */
#define	PODULE_BBCIO	0x0006		/* BBC I/O podule */
#define	PODULE_FAXPACK	0x0007		/* FaxPack modem */
#define	PODULE_ST506	0x000b		/* ST506 HD interface */
#define	PODULE_ACORN_MIDI	0x0013		/* MIDI interface */
#define	PODULE_LASERDIRECT	0x0014		/* LaserDirect (Canon LBP-4) */
#define	PODULE_A448	0x0016		/* A448 sound sampler */
#define	PODULE_HCCS_IDESCSI	0x0022		/* HCCS IDE or SCSI interface */
#define	PODULE_CUMANA_SCSI2	0x003a		/* SCSI II interface */
#define	PODULE_ACORN_USERMIDI	0x003f		/* User Port/MIDI interface */
#define	PODULE_LINGENUITY_SCSI8	0x0040		/* 8 bit SCSI interface */
#define	PODULE_ARXE_SCSI	0x0041		/* 16 bit SCSI interface */
#define	PODULE_COLOURCARD	0x0050		/* ColourCard */
#define	PODULE_HAWKV9	0x0052		/* Hawk v9 mark2 */
#define	PODULE_WILDVISION_SOUNDSAMPLER	0x0054		/* Wild Vision Sound Sampler */
#define	PODULE_DTSOFT_IDE	0x0055		/* IDE interface */
/* XXX ID 0x0058 is used by Oak ClassNet (EtherO) Ethernet cards */
#define	PODULE_OAK_SCSI	0x0058		/* 16 bit SCSI interface */
#define	PODULE_ETHER2	0x0061		/* Ether2 interface */
#define	PODULE_ULTIMATE	0x0063		/* Ultimate micropodule carrier */
#define	PODULE_NEXUS	0x0064		/* Nexus interface (Podule) */
#define	PODULE_MORLEY_SCSI	0x0067		/* SCSI interface */
#define	PODULE_MORLEY_USERANALOGUE	0x006d		/* User and Analogue ports */
#define	PODULE_HCCS_USERANALOGUE	0x006e		/* User and Analogue ports */
#define	PODULE_WILDVISION_CENTRONICS	0x006f		/* Bi-directional Centronics */
#define	PODULE_LINGENUITY_SCSI8SHARE	0x008c		/* 8 bit SCSIShare interface */
#define	PODULE_VTI_SCSI	0x008d		/* SCSI interface */
#define	PODULE_NEXUSNS	0x008f		/* Nexus interface (A3020 netslot) */
#define	PODULE_ATOMWIDE_SERIAL	0x0090		/* multiport serial interface */
#define	PODULE_LINGENUITY_SCSI	0x0095		/* 16 bit SCSI interface */
#define	PODULE_LINGENUITY_SCSISHARE	0x0096		/* 16 bit SCSIShare interface */
#define	PODULE_BEEBUG_IDE8	0x0097		/* 8 bit IDE */
#define	PODULE_CUMANA_SCSI1	0x00a0		/* SCSI I interface */
#define	PODULE_ETHER3	0x00a4		/* Ether3/Ether5 interface */
#define	PODULE_ICS_IDE	0x00ae		/* IDE Interface */
#define	PODULE_SERIALPORT_DUALSERIAL	0x00b9		/* Serial interface */
#define	PODULE_ETHERLAN200	0x00bd		/* EtherLan 200-series */
#define	PODULE_SCANLIGHTV256	0x00cb		/* ScanLight Video 256 */
#define	PODULE_EAGLEM2	0x00cc		/* Eagle M2 */
#define	PODULE_LARKA16	0x00ce		/* Lark A16 */
#define	PODULE_ETHERLAN100	0x00cf		/* EtherLan 100-series */
#define	PODULE_ETHERLAN500	0x00d4		/* EtherLan 500-series */
#define	PODULE_ETHERM	0x00d8		/* EtherM dual interface NIC */
#define	PODULE_CUMANA_SLCD	0x00dd		/* CDFS & SLCD expansion card */
#define	PODULE_BRINILINK	0x00df		/* BriniLink transputer link adapter */
#define	PODULE_ETHERB	0x00e4		/* EtherB network slot interface */
#define	PODULE_24I16	0x00e6		/* 24i16 digitiser */
#define	PODULE_PCCARD	0x00ea		/* PC card */
#define	PODULE_ETHERLAN600	0x00ec		/* EtherLan 600-series */
#define	PODULE_CASTLE_SCSI16SHARE	0x00f3		/* 8 or 16 bit SCSI2Share interface */
#define	PODULE_CASTLE_ETHERSCSISHARE	0x00f4		/* 8 or 16 bit SCSI2Share interface, possibly with Ethernet */
#define	PODULE_CASTLE_ETHERSCSI	0x00f5		/* EtherSCSI */
#define	PODULE_CASTLE_SCSI16	0x00f6		/* 8 or 16 bit SCSI2 interface */
#define	PODULE_ALSYSTEMS_SCSI	0x0107		/* SCSI II host adapter */
#define	PODULE_RAPIDE	0x0114		/* RapIDE32 interface */
#define	PODULE_ETHERLAN100AEH	0x011c		/* AEH77 (EtherLan 102) */
#define	PODULE_ETHERLAN200AEH	0x011d		/* AEH79 (EtherLan 210) */
#define	PODULE_ETHERLAN600AEH	0x011e		/* AEH62/78/99 (EtherLan 602) */
#define	PODULE_ETHERLAN500AEH	0x011f		/* AEH75 (EtherLan 512) */
#define	PODULE_CONNECT32	0x0125		/* Connect32 SCSI II interface */
#define	PODULE_CASTLE_SCSI32	0x012b		/* 32 bit SCSI2 + DMA interface */
#define	PODULE_ETHERLAN700AEH	0x012e		/* AEH98 (EtherLan 700-series) */
#define	PODULE_ETHERLAN700	0x012f		/* EtherLan 700-series */
#define	PODULE_SIMTEC_IDE8	0x0130		/* 8 bit IDE interface */
#define	PODULE_SIMTEC_IDE	0x0131		/* 16 bit IDE interface */
#define	PODULE_MIDICONNECT	0x0133		/* Midi-Connect */
#define	PODULE_ETHERI	0x0139		/* EtherI interface */
#define	PODULE_MIDIMAX	0x0200		/* MIDI max */
#define	PODULE_MMETHERV	0x1234		/* Multi-media/EtherV */
#define	PODULE_ETHERN	0x5678		/* EtherN interface */
