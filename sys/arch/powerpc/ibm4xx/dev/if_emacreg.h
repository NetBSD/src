/*	$NetBSD: if_emacreg.h,v 1.1 2001/06/24 02:13:37 simonb Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * MAL buffer descriptor control/status bit definitions, in the
 * md_stat_ctrl field of the MAL descriptor <machine/mal.h>.
 */

/* EMAC transmit control definitions */
#define	EMAC_TXC_GFCS		0x0200	/* Generate FCS */
#define	EMAC_TXC_GPAD		0x0100	/* Generate padding */
#define	EMAC_TXC_ISA		0x0080	/* Insert Source Address */
#define	EMAC_TXC_RSA		0x0040	/* Replace Source Address */
#define	EMAC_TXC_IVT		0x0020	/* Insert VLAN Tag */
#define	EMAC_TXC_RVT		0x0010	/* Replace VLAN Tag */

/* EMAC transmit status definitions */
#define EMAC_TXS_BFCS		0x0200	/* Bad FCS */
#define EMAC_TXS_BPP		0x0100	/* Bad previous packet */
#define EMAC_TXS_LCS		0x0080	/* Loss of carrier sense */
#define EMAC_TXS_ED		0x0040	/* Excessive deferral */
#define EMAC_TXS_EC		0x0020	/* Excessive collisions */
#define EMAC_TXS_LC		0x0010	/* Late collision */
#define EMAC_TXS_MC		0x0008	/* Multiple collision */
#define EMAC_TXS_SC		0x0004	/* Single collision */
#define EMAC_TXS_UR		0x0002	/* Underrun */
#define EMAC_TXS_SQE		0x0001	/* Signal Quality Error */

/* EMAC receive status definitions */
#define EMAC_RXS_OE		0x0200	/* Overrun error */
#define EMAC_RXS_PP		0x0100	/* Pause packet received */
#define EMAC_RXS_BP		0x0080	/* Bad packet */
#define EMAC_RXS_RP		0x0040	/* Runt packet */
#define EMAC_RXS_SE		0x0020	/* Short event */
#define EMAC_RXS_AE		0x0010	/* Alignment error */
#define EMAC_RXS_BFCS		0x0008	/* Bad FCS */
#define EMAC_RXS_PTL		0x0004	/* Packet too long */
#define EMAC_RXS_ORE		0x0002	/* Out of range error */
#define EMAC_RXS_IRE		0x0001	/* In range error */
