/*	$NetBSD: podules.h,v 1.4.8.2 2002/04/01 07:47:01 nathanw Exp $	*/

/*
 * THIS FILE AUTOMATICALLY GENERATED.  DO NOT EDIT.
 *
 * generated from:
 *	NetBSD: podules,v 1.7 2002/03/09 13:44:09 bjh21 Exp 
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
#define	MANUFACTURER_OLIVETTI	0x0002		/* Olivetti */
#define	MANUFACTURER_WATFORD	0x0003		/* Watford Electronics */
#define	MANUFACTURER_CCONCEPTS	0x0004		/* Computer Concepts */
#define	MANUFACTURER_ARMADILLO	0x0007		/* Armadillo Systems */
#define	MANUFACTURER_WILDVISION	0x0009		/* Wild Vision */
#define	MANUFACTURER_HCCS	0x000c		/* HCCS */
#define	MANUFACTURER_ATOMWIDE	0x0011		/* Atomwide */
#define	MANUFACTURER_ATOMWIDE2	0x0017		/* Atomwide */
#define	MANUFACTURER_LINGENUITY	0x001a		/* Lingenuity */
#define	MANUFACTURER_IRLAM	0x001f		/* Irlam Instruments */
#define	MANUFACTURER_OAK	0x0021		/* Oak Solutions */
#define	MANUFACTURER_SJ	0x0029		/* SJ Research */
#define	MANUFACTURER_MORLEY	0x002b		/* Morley */
#define	MANUFACTURER_HCCS2	0x002d		/* HCCS */
#define	MANUFACTURER_VTI	0x0035		/* Vertical Twist */
#define	MANUFACTURER_DTSOFT	0x0037		/* D.T. Software */
#define	MANUFACTURER_CUMANA	0x003a		/* Cumana */
#define	MANUFACTURER_ICS	0x003c		/* ICS */
#define	MANUFACTURER_SERIALPORT	0x003f		/* Serial Port */
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
 * List of known podules.  Grouped by vendor.
 */

#define	PODULE_ACORN_ETHER3XXX	0x0000		/* Ether3 (NOROM) */
#define	PODULE_ACORN_SCSI	0x0002		/* SCSI 1 interface */
#define	PODULE_ACORN_ETHER1	0x0003		/* ether 1 interface */
#define	PODULE_ACORN_RAMROM	0x0005		/* RAM/ROM podule */
#define	PODULE_ACORN_BBCIO	0x0006		/* BBC IO interface */
#define	PODULE_ACORN_ST506	0x000b		/* ST506 HD interface */
#define	PODULE_ACORN_MIDI	0x0013		/* MIDI interface */
#define	PODULE_ACORN_USERMIDI	0x003F		/* User Port/MIDI interface */
#define	PODULE_ACORN_ETHER2	0x0061		/* ether 2 interface */
#define	PODULE_ACORN_ETHERI	0x0139		/* EtherI interface */

#define	PODULE_CCONCEPTS_LASERDIRECT	0x0014		/* laser direct (Canon LBP-4) */

#define	PODULE_ARMADILLO_A448	0x0016		/* A448 sound sampler */

/* From an Issue 2.0 ColourCard (others may differ) */
#define	PODULE_WILDVISION_COLOURCARD	0x0050		/* ColourCard */
#define	PODULE_WILDVISION_HAWKV9	0x0052		/* hawk v9 mark2 */
#define	PODULE_WILDVISION_SOUNDSAMPLER	0x0054		/* Sound Sampler */
#define	PODULE_WILDVISION_CENTRONICS	0x006f		/* Bi-directional Centronics */
#define	PODULE_WILDVISION_SCANLIGHTV256	0x00cb		/* scanlight video 256 */
#define	PODULE_WILDVISION_EAGLEM2	0x00cc		/* eagle M2 */
#define	PODULE_WILDVISION_LARKA16	0x00ce		/* lark A16 */
#define	PODULE_WILDVISION_MIDIMAX	0x0200		/* MIDI max */

#define	PODULE_HCCS_IDESCSI	0x0022		/* IDE or SCSI interface */
#define	PODULE_HCCS_ULTIMATE	0x0063		/* Ultimate micropodule carrier */

#define	PODULE_ATOMWIDE_ETHER3	0x00A4		/* ether 3/5 interface */

#define	PODULE_ATOMWIDE2_SERIAL	0x0090		/* multiport serial interface */

#define	PODULE_LINGENUITY_SCSI	0x0095		/* 16 bit SCSI interface */
#define	PODULE_LINGENUITY_SCSISHARE	0x0096		/* 16 bit SCSIShare interface */
#define	PODULE_LINGENUITY_SCSI8	0x0040		/* 8 bit SCSI interface */
#define	PODULE_LINGENUITY_SCSI8SHARE	0x008c		/* 8 bit SCSIShare interface */

#define	PODULE_IRLAM_24I16	0x00e6		/* 24i16 digitiser */
#define	PODULE_IRLAM_MMETHERV	0x1234		/* Multi-media/EtherV */
#define	PODULE_IRLAM_ETHERN	0x5678		/* EtherN interface */

#define	PODULE_OAK_SCSI	0x0058		/* 16 bit SCSI interface */

#define	PODULE_SJ_NEXUS	0x0064		/* Nexus interface (Podule) */
#define	PODULE_SJ_NEXUSNS	0x008F		/* Nexus interface (A3020 netslot) */

#define	PODULE_MORLEY_SCSI	0x0067		/* SCSI interface */
#define	PODULE_MORLEY_USERANALOGUE	0x006d		/* User and Analogue ports */

#define	PODULE_HCCS2_USERANALOGUE	0x006e		/* User and Analogue ports */

#define	PODULE_VTI_SCSI	0x008d		/* SCSI interface */

#define	PODULE_DTSOFT_IDE	0x0055		/* IDE interface */

#define	PODULE_CUMANA_SCSI2	0x003a		/* SCSI II interface */
#define	PODULE_CUMANA_SCSI1	0x00a0		/* SCSI I interface */
#define	PODULE_CUMANA_SLCD	0x00dd		/* CDFS & SLCD expansion card */

#define	PODULE_ICS_IDE	0x00ae		/* IDE Interface */

#define	PODULE_SERIALPORT_DUALSERIAL	0x00b9		/* Serial interface */

#define	PODULE_ARXE_SCSI	0x0041		/* 16 bit SCSI interface */

#define	PODULE_ALEPH1_PCCARD	0x00ea		/* PC card */

/* i-cubed's own cards */
#define	PODULE_ICUBED_ETHERLAN100	0x00cf		/* EtherLan 100-series */
#define	PODULE_ICUBED_ETHERLAN200	0x00bd		/* EtherLan 200-series */
#define	PODULE_ICUBED_ETHERLAN500	0x00d4		/* EtherLan 500-series */
#define	PODULE_ICUBED_ETHERLAN600	0x00ec		/* EtherLan 600-series */
#define	PODULE_ICUBED_ETHERLAN700	0x012f		/* EtherLan 700-series */
/* cards made by i-cubed for Acorn */
#define	PODULE_ICUBED_ETHERLAN100AEH	0x011c		/* AEH77 (EtherLan 102) */
#define	PODULE_ICUBED_ETHERLAN200AEH	0x011d		/* AEH79 (EtherLan 210) */
#define	PODULE_ICUBED_ETHERLAN600AEH	0x011e		/* AEH62/78/99 (EtherLan 602) */
#define	PODULE_ICUBED_ETHERLAN500AEH	0x011f		/* AEH75 (EtherLan 512) */
#define	PODULE_ICUBED_ETHERLAN700AEH	0x012e		/* AEH98 (EtherLan 700-series) */
/* XXX Not listed by Design IT. */
#define	PODULE_ICUBED_ETHERLAN100XXX	0x00c4		/* EtherLan 100??? */

#define	PODULE_BRINI_PORT	0x0000		/* BriniPort intelligent I/O interface */
#define	PODULE_BRINI_LINK	0x00df		/* BriniLink transputer link adapter */

#define	PODULE_ANT_ETHER3	0x00a4		/* ether 3/5 interface */
#define	PODULE_ANT_ETHERB	0x00e4		/* ether B network slot interface */
#define	PODULE_ANT_ETHERM	0x00d8		/* ether M dual interface NIC */

#define	PODULE_CASTLE_SCSI16	0x00f6		/* 8 or 16 bit SCSI2 interface */
#define	PODULE_CASTLE_SCSI16SHARE	0x00f3		/* 8 or 16 bit SCSI2Share interface */
#define	PODULE_CASTLE_ETHERSCSI	0x00f5		/* EtherSCSI */
#define	PODULE_CASTLE_ETHERSCSISHARE	0x00f4		/* 8 or 16 bit SCSI2Share interface, possibly with Ethernet */
#define	PODULE_CASTLE_SCSI32	0x012b		/* 32 bit SCSI2 + DMA interface */

#define	PODULE_ALSYSTEMS_SCSI	0x0107		/* SCSI II host adapter */

#define	PODULE_SIMTEC_IDE8	0x0130		/* 8 bit IDE interface */
#define	PODULE_SIMTEC_IDE	0x0131		/* 16 bit IDE interface */

#define	PODULE_YES_RAPIDE	0x0114		/* RapIDE32 interface */

/* MCS also call themselves ACE (Acorn Computer Entertainment) */
#define	PODULE_MCS_SCSI	0x0125		/* Connect32 SCSI II interface */
#define	PODULE_MCS_MIDICONNECT	0x0133		/* Midi-Connect */

#define	PODULE_EESOX_SCSI	0x008c		/* EESOX SCSI II interface */
