/*	$NetBSD: podule_data.h,v 1.10 2002/05/22 23:46:53 bjh21 Exp $	*/

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

static struct podule_description known_podules[] = {
	{ PODULE_ACORN_SCSI,	"Acorn SCSI interface" },
	{ PODULE_ETHER1,	"Ether1 interface" },
	{ PODULE_ROMRAM,	"ROM/RAM podule" },
	{ PODULE_BBCIO,	"BBC I/O podule" },
	{ PODULE_FAXPACK,	"FaxPack modem" },
	{ PODULE_ST506,	"ST506 HD interface" },
	{ PODULE_ACORN_MIDI,	"MIDI interface" },
	{ PODULE_LASERDIRECT,	"LaserDirect (Canon LBP-4)" },
	{ PODULE_A448,	"A448 sound sampler" },
	{ PODULE_HCCS_IDESCSI,	"HCCS IDE or SCSI interface" },
	{ PODULE_CUMANA_SCSI2,	"SCSI II interface" },
	{ PODULE_ACORN_USERMIDI,	"User Port/MIDI interface" },
	{ PODULE_LINGENUITY_SCSI8,	"8 bit SCSI interface" },
	{ PODULE_ARXE_SCSI,	"16 bit SCSI interface" },
	{ PODULE_COLOURCARD,	"ColourCard" },
	{ PODULE_HAWKV9,	"Hawk v9 mark2" },
	{ PODULE_WILDVISION_SOUNDSAMPLER,	"Wild Vision Sound Sampler" },
	{ PODULE_DTSOFT_IDE,	"IDE interface" },
	{ PODULE_OAK_SCSI,	"16 bit SCSI interface" },
	{ PODULE_ETHER2,	"Ether2 interface" },
	{ PODULE_ULTIMATE,	"Ultimate micropodule carrier" },
	{ PODULE_NEXUS,	"Nexus interface (Podule)" },
	{ PODULE_MORLEY_SCSI,	"SCSI interface" },
	{ PODULE_MORLEY_USERANALOGUE,	"User and Analogue ports" },
	{ PODULE_HCCS_USERANALOGUE,	"User and Analogue ports" },
	{ PODULE_WILDVISION_CENTRONICS,	"Bi-directional Centronics" },
	{ PODULE_LINGENUITY_SCSI8SHARE,	"8 bit SCSIShare interface" },
	{ PODULE_VTI_SCSI,	"SCSI interface" },
	{ PODULE_NEXUSNS,	"Nexus interface (A3020 netslot)" },
	{ PODULE_ATOMWIDE_SERIAL,	"multiport serial interface" },
	{ PODULE_LINGENUITY_SCSI,	"16 bit SCSI interface" },
	{ PODULE_LINGENUITY_SCSISHARE,	"16 bit SCSIShare interface" },
	{ PODULE_BEEBUG_IDE8,	"8 bit IDE" },
	{ PODULE_CUMANA_SCSI1,	"SCSI I interface" },
	{ PODULE_ETHER3,	"Ether3/Ether5 interface" },
	{ PODULE_ICS_IDE,	"IDE Interface" },
	{ PODULE_SERIALPORT_DUALSERIAL,	"Serial interface" },
	{ PODULE_ETHERLAN200,	"EtherLan 200-series" },
	{ PODULE_SCANLIGHTV256,	"ScanLight Video 256" },
	{ PODULE_EAGLEM2,	"Eagle M2" },
	{ PODULE_LARKA16,	"Lark A16" },
	{ PODULE_ETHERLAN100,	"EtherLan 100-series" },
	{ PODULE_ETHERLAN500,	"EtherLan 500-series" },
	{ PODULE_ETHERM,	"EtherM dual interface NIC" },
	{ PODULE_CUMANA_SLCD,	"CDFS & SLCD expansion card" },
	{ PODULE_BRINILINK,	"BriniLink transputer link adapter" },
	{ PODULE_ETHERB,	"EtherB network slot interface" },
	{ PODULE_24I16,	"24i16 digitiser" },
	{ PODULE_PCCARD,	"PC card" },
	{ PODULE_ETHERLAN600,	"EtherLan 600-series" },
	{ PODULE_CASTLE_SCSI16SHARE,	"8 or 16 bit SCSI2Share interface" },
	{ PODULE_CASTLE_ETHERSCSISHARE,	"8 or 16 bit SCSI2Share interface, possibly with Ethernet" },
	{ PODULE_CASTLE_ETHERSCSI,	"EtherSCSI" },
	{ PODULE_CASTLE_SCSI16,	"8 or 16 bit SCSI2 interface" },
	{ PODULE_ALSYSTEMS_SCSI,	"SCSI II host adapter" },
	{ PODULE_RAPIDE,	"RapIDE32 interface" },
	{ PODULE_ETHERLAN100AEH,	"AEH77 (EtherLan 102)" },
	{ PODULE_ETHERLAN200AEH,	"AEH79 (EtherLan 210)" },
	{ PODULE_ETHERLAN600AEH,	"AEH62/78/99 (EtherLan 602)" },
	{ PODULE_ETHERLAN500AEH,	"AEH75 (EtherLan 512)" },
	{ PODULE_CONNECT32,	"Connect32 SCSI II interface" },
	{ PODULE_CASTLE_SCSI32,	"32 bit SCSI2 + DMA interface" },
	{ PODULE_ETHERLAN700AEH,	"AEH98 (EtherLan 700-series)" },
	{ PODULE_ETHERLAN700,	"EtherLan 700-series" },
	{ PODULE_SIMTEC_IDE8,	"8 bit IDE interface" },
	{ PODULE_SIMTEC_IDE,	"16 bit IDE interface" },
	{ PODULE_MIDICONNECT,	"Midi-Connect" },
	{ PODULE_ETHERI,	"EtherI interface" },
	{ PODULE_MIDIMAX,	"MIDI max" },
	{ PODULE_MMETHERV,	"Multi-media/EtherV" },
	{ PODULE_ETHERN,	"EtherN interface" },
	{ 0x0000, NULL }
};


struct manufacturer_description known_manufacturers[] = {
	{ MANUFACTURER_ACORN, 		"Acorn Computers" },
	{ MANUFACTURER_ACORNUSA, 	"Acorn Computers (USA)" },
	{ MANUFACTURER_OLIVETTI, 	"Olivetti" },
	{ MANUFACTURER_WATFORD, 	"Watford Electronics" },
	{ MANUFACTURER_CCONCEPTS, 	"Computer Concepts" },
	{ MANUFACTURER_IINTERFACES, 	"Intelligent Interfaces" },
	{ MANUFACTURER_CAMAN, 		"Caman" },
	{ MANUFACTURER_ARMADILLO, 	"Armadillo Systems" },
	{ MANUFACTURER_SOFTOPTION, 	"Soft Option" },
	{ MANUFACTURER_WILDVISION, 	"Wild Vision" },
	{ MANUFACTURER_ANGLOCOMPUTERS, 	"Anglo Computers" },
	{ MANUFACTURER_RESOURCE, 	"Resource" },
	{ MANUFACTURER_HCCS, 		"HCCS" },
	{ MANUFACTURER_MUSBURYCONSULT, 	"Musbury Consultants" },
	{ MANUFACTURER_GNOME, 		"Gnome" },
	{ MANUFACTURER_AANDGELEC, 	"A and G Electronics" },
	{ MANUFACTURER_SPACETECH, 	"Spacetech" },
	{ MANUFACTURER_ATOMWIDE, 	"Atomwide" },
	{ MANUFACTURER_SYNTEC, 		"Syntec" },
	{ MANUFACTURER_EMR, 		"ElectroMusic Research" },
	{ MANUFACTURER_MILLIPEDE, 	"Millipede" },
	{ MANUFACTURER_VIDEOELEC, 	"Video Electronics" },
	{ MANUFACTURER_BRAINSOFT, 	"Brainsoft" },
	{ MANUFACTURER_ATOMWIDE2, 	"Atomwide" },
	{ MANUFACTURER_LENDAC, 		"Lendac Data Systems" },
	{ MANUFACTURER_CAMMICROSYS, 	"Cambridge Micro Systems" },
	{ MANUFACTURER_LINGENUITY, 	"Lingenuity" },
	{ MANUFACTURER_SIPLAN, 		"Siplan Electronics Research" },
	{ MANUFACTURER_SCIFRONTIERS, 	"Science Frontiers" },
	{ MANUFACTURER_PINEAPPLE, 	"Pineapple Software" },
	{ MANUFACTURER_TECHNOMATIC, 	"Technomatic" },
	{ MANUFACTURER_IRLAM, 		"Irlam Instruments" },
	{ MANUFACTURER_NEXUS, 		"Nexus Electronics" },
	{ MANUFACTURER_OAK, 		"Oak Solutions" },
	{ MANUFACTURER_HUGHSYMONS, 	"Hugh Symons" },
	{ MANUFACTURER_BEEBUG, 		"BEEBUG (RISC Developments)" },
	{ MANUFACTURER_TEKNOMUSIK, 	"Teknomusik" },
	{ MANUFACTURER_REELTIME, 	"Reel Time" },
	{ MANUFACTURER_PRES, 		"PRES" },
	{ MANUFACTURER_DIGIHURST, 	"Digihurst" },
	{ MANUFACTURER_SGBCOMPSERV, 	"SGB Computer Services" },
	{ MANUFACTURER_SJ, 		"SJ Research" },
	{ MANUFACTURER_PHOBOX, 		"Phobox Electronics" },
	{ MANUFACTURER_MORLEY, 		"Morley Electronics" },
	{ MANUFACTURER_RACINGCAR, 	"Raching Car Computers" },
	{ MANUFACTURER_HCCS2, 		"HCCS" },
	{ MANUFACTURER_LINDIS, 		"Lindis International" },
	{ MANUFACTURER_CCC, 		"Computer Control Consultants" },
	{ MANUFACTURER_UNILAB, 		"Unilab" },
	{ MANUFACTURER_SEFANFROHLING, 	"Sefan Frohling" },
	{ MANUFACTURER_ROMBO, 		"Rombo Productions" },
	{ MANUFACTURER_3SL, 		"3SL" },
	{ MANUFACTURER_DELTRONICS, 	"Deltronics" },
	{ MANUFACTURER_VTI, 		"Vertical Twist" },
	{ MANUFACTURER_SIMIS, 		"Simis" },
	{ MANUFACTURER_DTSOFT, 		"D.T. Software" },
	{ MANUFACTURER_ARMINTERFACES, 	"ARM Interfaces" },
	{ MANUFACTURER_BIA, 		"BIA" },
	{ MANUFACTURER_CUMANA, 		"Cumana" },
	{ MANUFACTURER_IOTA, 		"Iota" },
	{ MANUFACTURER_ICS, 		"Ian Copestake Software" },
	{ MANUFACTURER_BAILDON, 	"Baildon Electronics" },
	{ MANUFACTURER_CSD, 		"CSD" },
	{ MANUFACTURER_SERIALPORT, 	"Serial Port" },
	{ MANUFACTURER_CADSOFT, 	"CADsoft" },
	{ MANUFACTURER_ARXE, 		"ARXE" },
	{ MANUFACTURER_ALEPH1, 		"Aleph 1" },
	{ MANUFACTURER_ICUBED, 		"I-Cubed" },
	{ MANUFACTURER_BRINI, 		"Brini" },
	{ MANUFACTURER_ANT, 		"ANT" },
	{ MANUFACTURER_CASTLE, 		"Castle Technology" },
	{ MANUFACTURER_ALSYSTEMS, 	"Alsystems" },
	{ MANUFACTURER_SIMTEC, 		"Simtec Electronics" },
	{ MANUFACTURER_YES, 		"Yellowstone Educational Solutions" },
	{ MANUFACTURER_MCS, 		"MCS" },
	{ MANUFACTURER_EESOX, 		"EESOX" },
	{ 0, NULL }
};
