/*	$NetBSD: dlt.h,v 1.12 2011/12/21 19:04:18 christos Exp $	*/

/*
 * Copyright (c) 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.c) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence
 * Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)bpf.h	8.2 (Berkeley) 1/9/95
 * @(#) Header: bpf.h,v 1.36 97/06/12 14:29:53 leres Exp  (LBL)
 */

#ifndef _NET_DLT_H_
#define _NET_DLT_H_

/*
 * Data-link level type codes.
 */
#define DLT_NULL	0	/* no link-layer encapsulation */
#define DLT_EN10MB	1	/* Ethernet (10Mb) */
#define DLT_EN3MB	2	/* Experimental Ethernet (3Mb) */
#define DLT_AX25	3	/* Amateur Radio AX.25 */
#define DLT_PRONET	4	/* Proteon ProNET Token Ring */
#define DLT_CHAOS	5	/* Chaos */
#define DLT_IEEE802	6	/* IEEE 802 Networks */
#define DLT_ARCNET	7	/* ARCNET */
#define DLT_SLIP	8	/* Serial Line IP */
#define DLT_PPP		9	/* Point-to-point Protocol */
#define DLT_FDDI	10	/* FDDI */
#define DLT_ATM_RFC1483	11	/* LLC/SNAP encapsulated atm */
#define DLT_RAW		12	/* raw IP */
#define DLT_SLIP_BSDOS	13	/* BSD/OS Serial Line IP */
#define DLT_PPP_BSDOS	14	/* BSD/OS Point-to-point Protocol */
#define DLT_HIPPI	15	/* HIPPI */
#define DLT_HDLC	16	/* HDLC framing */

#define DLT_PFSYNC	18	/* Packet filter state syncing */
#define DLT_ATM_CLIP	19	/* Linux Classical-IP over ATM */
#define DLT_ENC		109	/* Encapsulated packets for IPsec */
#define DLT_LINUX_SLL	113	/* Linux cooked sockets */
#define DLT_LTALK	114	/* Apple LocalTalk hardware */
#define DLT_PFLOG	117	/* Packet filter logging, by pcap people */
#define DLT_CISCO_IOS	118	/* Registered for Cisco-internal use */

/* NetBSD-specific types */
#define	DLT_PPP_SERIAL	50	/* PPP over serial (async and sync) */
#define	DLT_PPP_ETHER	51	/* XXX - deprecated! PPP over Ethernet; session only, w/o ether header */

/* Axent Raptor / Symantec Enterprise Firewall */
#define DLT_SYMANTEC_FIREWALL	99

#define DLT_C_HDLC		104	/* Cisco HDLC */
#define DLT_IEEE802_11		105	/* IEEE 802.11 wireless */
#define DLT_FRELAY		107	/* Frame Relay */
#define DLT_LOOP		108	/* OpenBSD DLT_LOOP */
#define DLT_ECONET		115	/* Acorn Econet */
#define DLT_PRISM_HEADER	119	/* 802.11 header plus Prism II info. */
#define DLT_AIRONET_HEADER 	120	/* 802.11 header plus Aironet info. */
#define DLT_HHDLC		121	/* Reserved for Siemens HiPath HDLC */
#define DLT_IP_OVER_FC		122	/* RFC 2625 IP-over-Fibre Channel */
#define DLT_SUNATM		123	/* Solaris+SunATM */
#define DLT_RIO                 124     /* RapidIO */
#define DLT_PCI_EXP             125     /* PCI Express */
#define DLT_AURORA              126     /* Xilinx Aurora link layer */
#define DLT_IEEE802_11_RADIO 	127	/* 802.11 header plus radio info. */
#define DLT_TZSP                128     /* Tazmen Sniffer Protocol */
#define DLT_ARCNET_LINUX	129	/* ARCNET */
#define DLT_JUNIPER_MLPPP       130	/* Juniper-private data link types. */
#define DLT_JUNIPER_MLFR        131
#define DLT_JUNIPER_ES          132
#define DLT_JUNIPER_GGSN        133
#define DLT_JUNIPER_MFR         134
#define DLT_JUNIPER_ATM2        135
#define DLT_JUNIPER_SERVICES    136
#define DLT_JUNIPER_ATM1        137
#define DLT_APPLE_IP_OVER_IEEE1394	138	/* Apple IP-over-IEEE 1394 */

/* Various SS7 encapsulations */
#define DLT_MTP2_WITH_PHDR	139 /* pseudo-header with various info,
				       followed by MTP2 */
#define DLT_MTP2		140 /* MTP2, without pseudo-header */
#define DLT_MTP3		141 /* MTP3, without pseudo-header or MTP2 */
#define DLT_SCCP		142 /* SCCP, without pseudo-header or MTP2
				       or MTP3 */

#define DLT_DOCSIS		143	/* Reserved for DOCSIS MAC frames. */
#define DLT_LINUX_IRDA		144	/* Linux-IrDA packets */

/* Reserved for IBM SP switch and IBM Next Federation switch. */
#define DLT_IBM_SP		145
#define DLT_IBM_SN		146

#define DLT_IEEE802_11_RADIO_AVS	163	/* 802.11 plus AVS header */
#define DLT_JUNIPER_MONITOR     164	/* Juniper-private data link type */
#define DLT_BACNET_MS_TP	165
#define DLT_PPP_PPPD		166	/* Another PPP variant (Linux? */

#define DLT_JUNIPER_PPPOE	167
#define DLT_JUNIPER_PPPOE_ATM	168
#define DLT_JUNIPER_PIC_PEER	174
#define DLT_JUNIPER_ETHER	178
#define DLT_JUNIPER_PPP		179
#define DLT_JUNIPER_FRELAY	180
#define DLT_JUNIPER_CHDLC	181

#define DLT_GPRS_LLC		169	/* GPRS LLC */
#define DLT_GPF_T		170	/* GPF-T (ITU-T G.7041/Y.1303) */
#define DLT_GPF_F		171	/* GPF-F (ITU-T G.7041/Y.1303) */

#define DLT_GCOM_T1E1		172
#define DLT_GCOM_SERIAL		173

/* "EndaceRecordFormat" */
#define DLT_ERF_ETH		175	/* Ethernet */
#define DLT_ERF_POS		176	/* Packet-over-SONET */

#define DLT_LINUX_LAPD		177	/* Raw LAPD for vISDN */

/*
 * Juniper-private data link type, as per request from
 * Hannes Gredler <hannes@juniper.net>. 
 * The DLT_ are used for prepending meta-information
 * like interface index, interface name
 * before standard Ethernet, PPP, Frelay & C-HDLC Frames
 */
#define DLT_JUNIPER_ETHER       178
#define DLT_JUNIPER_PPP         179
#define DLT_JUNIPER_FRELAY      180
#define DLT_JUNIPER_CHDLC       181

/*
 * Multi Link Frame Relay (FRF.16)
 */
#define DLT_MFR                 182

/*
 * Juniper-private data link type, as per request from
 * Hannes Gredler <hannes@juniper.net>. 
 * The DLT_ is used for internal communication with a
 * voice Adapter Card (PIC)
 */
#define DLT_JUNIPER_VP          183

/*
 * Arinc 429 frames.
 * DLT_ requested by Gianluca Varenni <gianluca.varenni@cacetech.com>.
 * Every frame contains a 32bit A429 label.
 * More documentation on Arinc 429 can be found at
 * http://www.condoreng.com/support/downloads/tutorials/ARINCTutorial.pdf
 */
#define DLT_A429                184

/*
 * Arinc 653 Interpartition Communication messages.
 * DLT_ requested by Gianluca Varenni <gianluca.varenni@cacetech.com>.
 * Please refer to the A653-1 standard for more information.
 */
#define DLT_A653_ICM            185

/*
 * USB packets, beginning with a USB setup header; requested by
 * Paolo Abeni <paolo.abeni@email.it>.
 */
#define DLT_USB			186

/*
 * Bluetooth HCI UART transport layer (part H:4); requested by
 * Paolo Abeni.
 */
#define DLT_BLUETOOTH_HCI_H4	187

/*
 * IEEE 802.16 MAC Common Part Sublayer; requested by Maria Cruz
 * <cruz_petagay@bah.com>.
 */
#define DLT_IEEE802_16_MAC_CPS	188

/*
 * USB packets, beginning with a Linux USB header; requested by
 * Paolo Abeni <paolo.abeni@email.it>.
 */
#define DLT_USB_LINUX		189

/*
 * Controller Area Network (CAN) v. 2.0B packets.
 * DLT_ requested by Gianluca Varenni <gianluca.varenni@cacetech.com>.
 * Used to dump CAN packets coming from a CAN Vector board.
 * More documentation on the CAN v2.0B frames can be found at
 * http://www.can-cia.org/downloads/?269
 */
#define DLT_CAN20B              190

/*
 * IEEE 802.15.4, with address fields padded, as is done by Linux
 * drivers; requested by Juergen Schimmer.
 */
#define DLT_IEEE802_15_4_LINUX	191

/*
 * Per Packet Information encapsulated packets.
 * DLT_ requested by Gianluca Varenni <gianluca.varenni@cacetech.com>.
 */
#define DLT_PPI			192

/*
 * Header for 802.16 MAC Common Part Sublayer plus a radiotap radio header;
 * requested by Charles Clancy.
 */
#define DLT_IEEE802_16_MAC_CPS_RADIO	193

/*
 * Juniper-private data link type, as per request from
 * Hannes Gredler <hannes@juniper.net>. 
 * The DLT_ is used for internal communication with a
 * integrated service module (ISM).
 */
#define DLT_JUNIPER_ISM         194

/*
 * IEEE 802.15.4, exactly as it appears in the spec (no padding, no
 * nothing); requested by Mikko Saarnivala <mikko.saarnivala@sensinode.com>.
 */
#define DLT_IEEE802_15_4	195

/*
 * Various link-layer types, with a pseudo-header, for SITA
 * (http://www.sita.aero/); requested by Fulko Hew (fulko.hew@gmail.com).
 */
#define DLT_SITA		196

/*
 * Various link-layer types, with a pseudo-header, for Endace DAG cards;
 * encapsulates Endace ERF records.  Requested by Stephen Donnelly
 * <stephen@endace.com>.
 */
#define DLT_ERF			197

/*
 * Special header prepended to Ethernet packets when capturing from a
 * u10 Networks board.  Requested by Phil Mulholland
 * <phil@u10networks.com>.
 */
#define DLT_RAIF1		198

/*
 * IPMB packet for IPMI, beginning with the I2C slave address, followed
 * by the netFn and LUN, etc..  Requested by Chanthy Toeung
 * <chanthy.toeung@ca.kontron.com>.
 */
#define DLT_IPMB		199

/*
 * Juniper-private data link type, as per request from
 * Hannes Gredler <hannes@juniper.net>. 
 * The DLT_ is used for capturing data on a secure tunnel interface.
 */
#define DLT_JUNIPER_ST          200

/*
 * Bluetooth HCI UART transport layer (part H:4), with pseudo-header
 * that includes direction information; requested by Paolo Abeni.
 */
#define DLT_BLUETOOTH_HCI_H4_WITH_PHDR	201

/*
 * AX.25 packet with a 1-byte KISS header; see
 *
 *	http://www.ax25.net/kiss.htm
 *
 * as per Richard Stearn <richard@rns-stearn.demon.co.uk>.
 */
#define DLT_AX25_KISS		202

/*
 * LAPD packets from an ISDN channel, starting with the address field,
 * with no pseudo-header.
 * Requested by Varuna De Silva <varunax@gmail.com>.
 */
#define DLT_LAPD		203

/*
 * Variants of various link-layer headers, with a one-byte direction
 * pseudo-header prepended - zero means "received by this host",
 * non-zero (any non-zero value) means "sent by this host" - as per
 * Will Barker <w.barker@zen.co.uk>.
 */
#define DLT_PPP_WITH_DIR	204	/* PPP - don't confuse with DLT_PPP_WITH_DIRECTION */
#define DLT_C_HDLC_WITH_DIR	205	/* Cisco HDLC */
#define DLT_FRELAY_WITH_DIR	206	/* Frame Relay */
#define DLT_LAPB_WITH_DIR	207	/* LAPB */

/*
 * 208 is reserved for an as-yet-unspecified proprietary link-layer
 * type, as requested by Will Barker.
 */

/*
 * IPMB with a Linux-specific pseudo-header; as requested by Alexey Neyman
 * <avn@pigeonpoint.com>.
 */
#define DLT_IPMB_LINUX		209

/*
 * FlexRay automotive bus - http://www.flexray.com/ - as requested
 * by Hannes Kaelber <hannes.kaelber@x2e.de>.
 */
#define DLT_FLEXRAY		210

/*
 * Media Oriented Systems Transport (MOST) bus for multimedia
 * transport - http://www.mostcooperation.com/ - as requested
 * by Hannes Kaelber <hannes.kaelber@x2e.de>.
 */
#define DLT_MOST		211

/*
 * Local Interconnect Network (LIN) bus for vehicle networks -
 * http://www.lin-subbus.org/ - as requested by Hannes Kaelber
 * <hannes.kaelber@x2e.de>.
 */
#define DLT_LIN			212

/*
 * X2E-private data link type used for serial line capture,
 * as requested by Hannes Kaelber <hannes.kaelber@x2e.de>.
 */
#define DLT_X2E_SERIAL		213

/*
 * X2E-private data link type used for the Xoraya data logger
 * family, as requested by Hannes Kaelber <hannes.kaelber@x2e.de>.
 */
#define DLT_X2E_XORAYA		214

/*
 * IEEE 802.15.4, exactly as it appears in the spec (no padding, no
 * nothing), but with the PHY-level data for non-ASK PHYs (4 octets
 * of 0 as preamble, one octet of SFD, one octet of frame length+
 * reserved bit, and then the MAC-layer data, starting with the
 * frame control field).
 *
 * Requested by Max Filippov <jcmvbkbc@gmail.com>.
 */
#define DLT_IEEE802_15_4_NONASK_PHY	215

/* 
 * David Gibson <david@gibson.dropbear.id.au> requested this for
 * captures from the Linux kernel /dev/input/eventN devices. This
 * is used to communicate keystrokes and mouse movements from the
 * Linux kernel to display systems, such as Xorg. 
 */
#define DLT_LINUX_EVDEV		216

/*
 * GSM Um and Abis interfaces, preceded by a "gsmtap" header.
 *
 * Requested by Harald Welte <laforge@gnumonks.org>.
 */
#define DLT_GSMTAP_UM		217
#define DLT_GSMTAP_ABIS		218

/*
 * MPLS, with an MPLS label as the link-layer header.
 * Requested by Michele Marchetto <michele@openbsd.org> on behalf
 * of OpenBSD.
 */
#define DLT_MPLS		219

/*
 * USB packets, beginning with a Linux USB header, with the USB header
 * padded to 64 bytes; required for memory-mapped access.
 */
#define DLT_USB_LINUX_MMAPPED	220

/*
 * DECT packets, with a pseudo-header; requested by
 * Matthias Wenzel <tcpdump@mazzoo.de>.
 */
#define DLT_DECT		221

/*
 * From: "Lidwa, Eric (GSFC-582.0)[SGT INC]" <eric.lidwa-1@nasa.gov>
 * Date: Mon, 11 May 2009 11:18:30 -0500
 *
 * DLT_AOS. We need it for AOS Space Data Link Protocol.
 *   I have already written dissectors for but need an OK from
 *   legal before I can submit a patch.
 *
 */
#define DLT_AOS                 222

/*
 * Wireless HART (Highway Addressable Remote Transducer)
 * From the HART Communication Foundation
 * IES/PAS 62591
 *
 * Requested by Sam Roberts <vieuxtech@gmail.com>.
 */
#define DLT_WIHART		223

/*
 * Fibre Channel FC-2 frames, beginning with a Frame_Header.
 * Requested by Kahou Lei <kahou82@gmail.com>.
 */
#define DLT_FC_2		224

/*
 * Fibre Channel FC-2 frames, beginning with an encoding of the
 * SOF, and ending with an encoding of the EOF.
 *
 * The encodings represent the frame delimiters as 4-byte sequences
 * representing the corresponding ordered sets, with K28.5
 * represented as 0xBC, and the D symbols as the corresponding
 * byte values; for example, SOFi2, which is K28.5 - D21.5 - D1.2 - D21.2,
 * is represented as 0xBC 0xB5 0x55 0x55.
 *
 * Requested by Kahou Lei <kahou82@gmail.com>.
 */
#define DLT_FC_2_WITH_FRAME_DELIMS	225

/*
 * Solaris ipnet pseudo-header; requested by Darren Reed <Darren.Reed@Sun.COM>.
 *
 * The pseudo-header starts with a one-byte version number; for version 2,
 * the pseudo-header is:
 *
 * struct dl_ipnetinfo {
 *     u_int8_t   dli_version;
 *     u_int8_t   dli_family;
 *     u_int16_t  dli_htype;
 *     u_int32_t  dli_pktlen;
 *     u_int32_t  dli_ifindex;
 *     u_int32_t  dli_grifindex;
 *     u_int32_t  dli_zsrc;
 *     u_int32_t  dli_zdst;
 * };
 *
 * dli_version is 2 for the current version of the pseudo-header.
 *
 * dli_family is a Solaris address family value, so it's 2 for IPv4
 * and 26 for IPv6.
 *
 * dli_htype is a "hook type" - 0 for incoming packets, 1 for outgoing
 * packets, and 2 for packets arriving from another zone on the same
 * machine.
 *
 * dli_pktlen is the length of the packet data following the pseudo-header
 * (so the captured length minus dli_pktlen is the length of the
 * pseudo-header, assuming the entire pseudo-header was captured).
 *
 * dli_ifindex is the interface index of the interface on which the
 * packet arrived.
 *
 * dli_grifindex is the group interface index number (for IPMP interfaces).
 *
 * dli_zsrc is the zone identifier for the source of the packet.
 *
 * dli_zdst is the zone identifier for the destination of the packet.
 *
 * A zone number of 0 is the global zone; a zone number of 0xffffffff
 * means that the packet arrived from another host on the network, not
 * from another zone on the same machine.
 *
 * An IPv4 or IPv6 datagram follows the pseudo-header; dli_family indicates
 * which of those it is.
 */
#define DLT_IPNET			226

/*
 * CAN (Controller Area Network) frames, with a pseudo-header as supplied
 * by Linux SocketCAN.  See Documentation/networking/can.txt in the Linux
 * source.
 *
 * Requested by Felix Obenhuber <felix@obenhuber.de>.
 */
#define DLT_CAN_SOCKETCAN		227

/*
 * Raw IPv4/IPv6; different from DLT_RAW in that the DLT_ value specifies
 * whether it's v4 or v6.  Requested by Darren Reed <Darren.Reed@Sun.COM>.
 */
#define DLT_IPV4			228
#define DLT_IPV6			229

/*
 * NetBSD-specific generic "raw" link type.  The upper 16-bits indicate
 * that this is the generic raw type, and the lower 16-bits are the
 * address family we're dealing with.
 */
#define	DLT_RAWAF_MASK		0x02240000
#define	DLT_RAWAF(af)		(DLT_RAWAF_MASK | (af))
#define	DLT_RAWAF_AF(x)		((x) & 0x0000ffff)
#define	DLT_IS_RAWAF(x)		(((x) & 0xffff0000) == DLT_RAWAF_MASK)

#endif /* !_NET_DLT_H_ */
