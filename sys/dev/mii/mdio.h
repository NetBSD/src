/*	$NetBSD: mdio.h,v 1.34 2026/11/25 14:25:41 msaitoh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Masanobu SAITOH.
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

#ifndef _DEV_MII_MDIO_H_
#define _DEV_MII_MDIO_H_

/*
 * IEEE 802.3 Clause 45 definitions.
 * From:
 *	IEEE 802.3-2022
 *
 * Naming policy for macro names:
 *
 * - When using abbreviated names, basically follow the guidelines in IEEE
 *   802.3 1.5 "Abbreviations".
 * - If a macro name ends with both speed and media type, separate them with
 *   an underscore. For example, use XXX_YYY10G_T instead of XXX_YYY10GT.
 * - Other abbreviations:
 *	ABLTY:	Ability
 *	BASER:	BASE-R
 *	CAP:	Capability
 *	CTRL:	Control
 *	STAT:	Status
 *	MGN:	Margin
 *	MULTGT: MultiGBASE-T
 *	TSYNC:	TimeSync
 */

/*
 * MDIO Manageable Device (MMD) addresses.
 * Table 45-1
 */
#define MDIO_MMD_PMAPMD		1
#define MDIO_MMD_WIS		2
#define MDIO_MMD_PCS		3
#define MDIO_MMD_PHYXS		4
#define MDIO_MMD_DTEXS		5
#define MDIO_MMD_TC		6
#define MDIO_MMD_AN		7
#define MDIO_MMD_SEPPMA1	8
#define MDIO_MMD_SEPPMA2	9
#define MDIO_MMD_SEPPMA3	10
#define MDIO_MMD_SEPPMA4	11
#define MDIO_MMD_OFDM_PMAPMD	12
#define MDIO_MMD_PU		13
#define MDIO_MMD_CL22EXT	29
#define MDIO_MMD_VNDSP1		30
#define MDIO_MMD_VNDSP2		31

/*
 * PMA/PMD registers.
 * Table 45-3
 */
#define MDIO_PMAPMD_CTRL1	0   /* PMA/PMD control 1 */
#define   PMAPMD_CTRL1_RESET	0x8000	/* Reset */
#define   PMAPMD_CTRL1_SPEED0	0x2000	/* Speed selection (LSB) */
#define   PMAPMD_CTRL1_LOWPWR	0x0800	/* Low power */
#define   PMAPMD_CTRL1_SPEED1	0x0040	/* Speed selection (MSB) */
#define   PMAPMD_CTRL1_SPEED2	0x003c	/* Speed selection (over 1G) */
#define   PMAPMD_CTRL1_LOOP_REM	0x0002	/* PMA remote loopback */
#define   PMAPMD_CTRL1_LOOP_LOC	0x0001	/* PMA local loopback */
#define   PMAPMD_CTRL1_SPEED_SEL52 (PMAPMD_CTRL1_SPEED0 | PMAPMD_CTRL1_SPEED1)
#define   PMAPMD_CTRL1_SPEED_MASK  (PMAPMD_CTRL1_SPEED_SEL52 | \
	    PMAPMD_CTRL1_SPEED2)
#define   PMAPMD_CTRL1_SPEED_10	     0
#define   PMAPMD_CTRL1_SPEED_100     PMAPMD_CTRL1_SPEED0
#define   PMAPMD_CTRL1_SPEED_1G	     PMAPMD_CTRL1_SPEED1
#define   PMAPMD_CTRL1_SPEED_10G     PMAPMD_CTRL1_SPEED_SEL52
#define   PMAPMD_CTRL1_SPEED_10PASS (PMAPMD_CTRL1_SPEED_SEL52 | (1 << 2))
#define   PMAPMD_CTRL1_SPEED_40G    (PMAPMD_CTRL1_SPEED_SEL52 | (2 << 2))
#define   PMAPMD_CTRL1_SPEED_100G   (PMAPMD_CTRL1_SPEED_SEL52 | (3 << 2))
#define   PMAPMD_CTRL1_SPEED_25G    (PMAPMD_CTRL1_SPEED_SEL52 | (4 << 2))
#define   PMAPMD_CTRL1_SPEED_2_5G   (PMAPMD_CTRL1_SPEED_SEL52 | (6 << 2))
#define   PMAPMD_CTRL1_SPEED_5G     (PMAPMD_CTRL1_SPEED_SEL52 | (7 << 2))

#define MDIO_PMAPMD_STAT1	1   /* PMA/PMD status 1 */
#define MDIO_PMAPMD_DEVID1	2   /* PMA/PMD device identifier 1 */
#define MDIO_PMAPMD_DEVID2	3   /*				 2 */
#define MDIO_PMAPMD_SPEED	4   /* PMA/PMD speed ability */
#define MDIO_PMAPMD_DEVS1	5   /* PMA/PMD devices in package 1 */
#define MDIO_PMAPMD_DEVS2	6   /*				  2 */

#define MDIO_PMAPMD_CTRL2	7   /* PMA/PMD control 2 */
#define   PMAPMD_CTRL2_PIASE		0x0200
#define   PMAPMD_CTRL2_PEASE		0x0100
#define   PMAPMD_CTRL2_TYPE_MASK	0x007f
#define   PMAPMD_CTRL2_TYPE_50G_BR40_U	0x78
#define   PMAPMD_CTRL2_TYPE_50G_BR20_U	0x77
#define   PMAPMD_CTRL2_TYPE_50G_BR10_U	0x76
#define   PMAPMD_CTRL2_TYPE_50G_BR40_D	0x75
#define   PMAPMD_CTRL2_TYPE_50G_BR20_D	0x74
#define   PMAPMD_CTRL2_TYPE_50G_BR10_D	0x73
#define   PMAPMD_CTRL2_TYPE_25G_BR40_U	0x72
#define   PMAPMD_CTRL2_TYPE_25G_BR20_U	0x71
#define   PMAPMD_CTRL2_TYPE_25G_BR10_U	0x70
#define   PMAPMD_CTRL2_TYPE_25G_BR40_D	0x6f
#define   PMAPMD_CTRL2_TYPE_25G_BR20_D	0x6e
#define   PMAPMD_CTRL2_TYPE_25G_BR10_D	0x6d
#define   PMAPMD_CTRL2_TYPE_10G_BR40_U	0x6c
#define   PMAPMD_CTRL2_TYPE_10G_BR20_U	0x6b
#define   PMAPMD_CTRL2_TYPE_10G_BR10_U	0x6a
#define   PMAPMD_CTRL2_TYPE_10G_BR40_D	0x69
#define   PMAPMD_CTRL2_TYPE_10G_BR20_D	0x68
#define   PMAPMD_CTRL2_TYPE_10G_BR10_D	0x67
#define   PMAPMD_CTRL2_TYPE_400G_ER8	0x63
#define   PMAPMD_CTRL2_TYPE_400G_LR4_6	0x62
#define   PMAPMD_CTRL2_TYPE_400G_FR4	0x61
#define   PMAPMD_CTRL2_TYPE_400G_SR4_2	0x60
#define   PMAPMD_CTRL2_TYPE_400G_SR8	0x5f
#define   PMAPMD_CTRL2_TYPE_400G_LR8	0x5c
#define   PMAPMD_CTRL2_TYPE_400G_FR8	0x5b
#define   PMAPMD_CTRL2_TYPE_400G_DR4	0x5a
#define   PMAPMD_CTRL2_TYPE_400G_SR16	0x59
#define   PMAPMD_CTRL2_TYPE_200G_ER4	0x58
#define   PMAPMD_CTRL2_TYPE_200G_LR4	0x55
#define   PMAPMD_CTRL2_TYPE_200G_FR4	0x54
#define   PMAPMD_CTRL2_TYPE_200G_DR4	0x53
#define   PMAPMD_CTRL2_TYPE_200G_SR4	0x52
#define   PMAPMD_CTRL2_TYPE_200G_CR4	0x51
#define   PMAPMD_CTRL2_TYPE_200G_KR4	0x50
#define   PMAPMD_CTRL2_TYPE_100G_ZR	0x4e
#define   PMAPMD_CTRL2_TYPE_100G_LR1	0x4d
#define   PMAPMD_CTRL2_TYPE_100G_FR1	0x4c
#define   PMAPMD_CTRL2_TYPE_100G_DR	0x4b
#define   PMAPMD_CTRL2_TYPE_100G_SR2	0x4a
#define   PMAPMD_CTRL2_TYPE_100G_CR2	0x49
#define   PMAPMD_CTRL2_TYPE_100G_KR2	0x48
#define   PMAPMD_CTRL2_TYPE_50G_ER	0x45
#define   PMAPMD_CTRL2_TYPE_50G_LR	0x44
#define   PMAPMD_CTRL2_TYPE_50G_FR	0x43
#define   PMAPMD_CTRL2_TYPE_50G_SR	0x42
#define   PMAPMD_CTRL2_TYPE_50G_CR	0x41
#define   PMAPMD_CTRL2_TYPE_50G_KR	0x40
#define   PMAPMD_CTRL2_TYPE_BASE_T1	0x3d	/* See 1.2100.3:0 */
#define   PMAPMD_CTRL2_TYPE_5G_KR	0x3c
#define   PMAPMD_CTRL2_TYPE_2_5G_KX	0x3b
#define   PMAPMD_CTRL2_TYPE_25G_SR	0x3a
#define   PMAPMD_CTRL2_TYPE_25G_KR	0x39	/* 25G-KR or 25G-KR-S */
#define   PMAPMD_CTRL2_TYPE_25G_CR	0x38	/* 25G-CR or 25G-CR-S */
#define   PMAPMD_CTRL2_TYPE_25G_T	0x37
#define   PMAPMD_CTRL2_TYPE_25G_ER	0x36
#define   PMAPMD_CTRL2_TYPE_25G_LR	0x35
#define   PMAPMD_CTRL2_TYPE_BASE_H	0x34	/* See 1.900 */
#define   PMAPMD_CTRL2_TYPE_10GPASS_XR_U 0x33
#define   PMAPMD_CTRL2_TYPE_10GPASS_XR_D 0x32
#define   PMAPMD_CTRL2_TYPE_5G_T	0x31
#define   PMAPMD_CTRL2_TYPE_2_5G_T	0x30
#define   PMAPMD_CTRL2_TYPE_100G_SR4	0x2f
#define   PMAPMD_CTRL2_TYPE_100G_CR4	0x2e
#define   PMAPMD_CTRL2_TYPE_100G_KR4	0x2d
#define   PMAPMD_CTRL2_TYPE_100G_KP4	0x2c
#define   PMAPMD_CTRL2_TYPE_100G_ER4	0x2b
#define   PMAPMD_CTRL2_TYPE_100G_LR4	0x2a
#define   PMAPMD_CTRL2_TYPE_100G_SR10	0x29
#define   PMAPMD_CTRL2_TYPE_100G_CR10	0x28
#define   PMAPMD_CTRL2_TYPE_40G_T	0x26
#define   PMAPMD_CTRL2_TYPE_40G_ER4	0x25
#define   PMAPMD_CTRL2_TYPE_40G_FR	0x24
#define   PMAPMD_CTRL2_TYPE_40G_LR4	0x23
#define   PMAPMD_CTRL2_TYPE_40G_SR4	0x22
#define   PMAPMD_CTRL2_TYPE_40G_CR4	0x21
#define   PMAPMD_CTRL2_TYPE_40G_KR4	0x20
#define   PMAPMD_CTRL2_TYPE_10_1G_PRX_U4 0x1f
#define   PMAPMD_CTRL2_TYPE_10G_PR_U4	0x1e
#define   PMAPMD_CTRL2_TYPE_10_1G_PRX_D4 0x1d
#define   PMAPMD_CTRL2_TYPE_10G_PR_D4	0x1c
#define   PMAPMD_CTRL2_TYPE_10G_PR_U3	0x1a
#define   PMAPMD_CTRL2_TYPE_10G_PR_U1	0x19
#define   PMAPMD_CTRL2_TYPE_10_1G_PRX_U3 0x18
#define   PMAPMD_CTRL2_TYPE_10_1G_PRX_U2 0x17
#define   PMAPMD_CTRL2_TYPE_10_1G_PRX_U1 0x16
#define   PMAPMD_CTRL2_TYPE_10G_PR_D3	0x15
#define   PMAPMD_CTRL2_TYPE_10G_PR_D2	0x14
#define   PMAPMD_CTRL2_TYPE_10G_PR_D1	0x13
#define   PMAPMD_CTRL2_TYPE_10_1G_PRX_D3 0x12
#define   PMAPMD_CTRL2_TYPE_10_1G_PRX_D2 0x11
#define   PMAPMD_CTRL2_TYPE_10_1G_PRX_D1 0x10
#define   PMAPMD_CTRL2_TYPE_10_T	0x0f
#define   PMAPMD_CTRL2_TYPE_100_TX	0x0e
#define   PMAPMD_CTRL2_TYPE_1000_KX	0x0d
#define   PMAPMD_CTRL2_TYPE_1000_T	0x0c
#define   PMAPMD_CTRL2_TYPE_10G_KR	0x0b
#define   PMAPMD_CTRL2_TYPE_10G_KX4	0x0a
#define   PMAPMD_CTRL2_TYPE_10G_T	0x09
#define   PMAPMD_CTRL2_TYPE_10G_LRM	0x08
#define   PMAPMD_CTRL2_TYPE_10G_SR	0x07
#define   PMAPMD_CTRL2_TYPE_10G_LR	0x06
#define   PMAPMD_CTRL2_TYPE_10G_ER	0x05
#define   PMAPMD_CTRL2_TYPE_10G_LX4	0x04
#define   PMAPMD_CTRL2_TYPE_10G_SW	0x03
#define   PMAPMD_CTRL2_TYPE_10G_LW	0x02
#define   PMAPMD_CTRL2_TYPE_10G_EW	0x01
#define   PMAPMD_CTRL2_TYPE_10G_CX4	0x00

#define MDIO_PMAPMD_STAT2	8   /* PMA/PMD status 2 */
#define MDIO_PMAPMD_TXDIS	9   /* PMA/PMD transmit disable */
#define MDIO_PMAPMD_RXSIGDTCT	10  /* PMD receive signal detect */

#define MDIO_PMAPMD_EXTABLTY	11  /* PMA/PMD extended ability */
#define   PMAPMD_EXTABLTY_BASE_H	0x8000 /* BASE-H */
#define   PMAPMD_EXTABLTY_2P5G_5G	0x4000 /* 2.5G and 5G */
#define   PMAPMD_EXTABLTY_2HG_4HG	0x2000 /* 200G and 400G */
#define   PMAPMD_EXTABLTY_25G		0x1000 /* 25G */
#define   PMAPMD_EXTABLTY_BASE_T1	0x0800 /* BASE-T1 */
#define   PMAPMD_EXTABLTY_40G_100G	0x0400 /* 40G and 100G */
#define   PMAPMD_EXTABLTY_P2MP		0x0200 /* P2MP */
#define   PMAPMD_EXTABLTY_10_T		0x0100 /* 10BASE-T */
#define   PMAPMD_EXTABLTY_100_TX	0x0080 /* 100BASE-TX */
#define   PMAPMD_EXTABLTY_1000_KX	0x0040 /* 1000BASE-KX */
#define   PMAPMD_EXTABLTY_1000_T	0x0020 /* 1000BASE-T */
#define   PMAPMD_EXTABLTY_10G_KR	0x0010 /* 10GBASE-KR */
#define   PMAPMD_EXTABLTY_10G_KX4	0x0008 /* 10GBASE-KX4 */
#define   PMAPMD_EXTABLTY_10G_T		0x0004 /* 10GBASE-T */
#define   PMAPMD_EXTABLTY_10G_LRM	0x0002 /* 10GBASE-LRM */
#define   PMAPMD_EXTABLTY_10G_CX4	0x0001 /* 10GBASE-CX4 */

#define MDIO_PMAPMD_P2MPABLTY	12  /* P2MP ability */

#define MDIO_PMAPMD_40G100GEXTABLTY 13 /* 40G/100G extended ability */
#define   PMAPMD_40G100GEXTABLTY_40G_T	0x0040 /* 40GBASE-T */

#define MDIO_PMAPMD_PKGID1	14  /* PMA/PMD package identifier 1 */
#define MDIO_PMAPMD_PKGID2	15  /*				  2 */
#define MDIO_PMAPMD_EEECAP	16  /* PMA/PMD EEE capability */
	/* Values from 17 to 20 are reserved */

#define MDIO_PMAPMD_2P5G5GABLTY	21  /* 2.5G/5G PMA/PMD extended ability */
#define   PMAPMD_2P5G5GABLTY_5G_KR	0x0008
#define   PMAPMD_2P5G5GABLTY_2P5G_KX	0x0004
#define   PMAPMD_2P5G5GABLTY_5G_T	0x0002
#define   PMAPMD_2P5G5GABLTY_2P5G_T	0x0001

	/* Values from 22 to 29 are reserved */
#define MDIO_PMAPMD_10P2B_CTRL	30  /* 10P/2B PMA/PMD control */
#define MDIO_PMAPMD_10P2B_STAT	31  /* 10P/2B	      status */
#define MDIO_PMAPMD_10P2B_LPCTRL 32 /* 10P/2B LP PMA/PMD control */
#define MDIO_PMAPMD_10P2B_LPSTAT 33 /*			 status */
	/* Values from 34 to 35 are reserved */
#define MDIO_PMAPMD_10P2B_LLOSCNT     36 /* 10P/2B link loss counter */
#define MDIO_PMAPMD_10P2B_RXSNRMGN    37 /* 10P/2B RX SNR margin */
#define MDIO_PMAPMD_10P2B_LPRXSNRMGN  38 /* 10P/2B LP RX SNR margin */
#define MDIO_PMAPMD_10P2B_LINEATTN    39 /* 10P/2B line attenuation */
#define MDIO_PMAPMD_10P2B_LPLINEATTN  40 /* 10P/2B LP line attenuation */
#define MDIO_PMAPMD_10P2B_LQTHRES     41 /* 10P/2B line quality thresholds */
#define MDIO_PMAPMD_10P2B_LPLQLTHRES  42 /* 10P/2B LP LQ thresholds */
#define MDIO_PMAPMD_10P_FECCOERRS     43 /* 10P FEC correctable errors count */
#define MDIO_PMAPMD_10P_FECUNCOERRS   44 /* 10P FEC uncorrectable err count */
#define MDIO_PMAPMD_10P_LPFECCOERRS   45 /* 10P LP FEC correctable err count */
#define MDIO_PMAPMD_10P_LPFECUNCOERRS 46 /* 10P LP FEC uncorrectable err cnt */
#define MDIO_PMAPMD_10P_ELECLENGTH    47 /* 10P electrical length */
#define MDIO_PMAPMD_10P_LPELECLENGTH  48 /* 10P LP electrical length */
#define MDIO_PMAPMD_10P_GENCONFIG     49 /* 10P PMA/PMD general config. */
#define MDIO_PMAPMD_10P_PSDCONFIG     50 /* 10P PSD configuration */
#define MDIO_PMAPMD_10P_DSDRCONF1     51 /* 10P downstream data rate conf 1 */
#define MDIO_PMAPMD_10P_DSDRCONF2     52 /*				  2 */
#define MDIO_PMAPMD_10P_DSRSCONF      53 /* 10P downstream ReedSolomon conf */
#define MDIO_PMAPMD_10P_USDR1	      54 /* 10P upstream data rate config 1 */
#define MDIO_PMAPMD_10P_USDR2	      55 /*				  2 */
#define MDIO_PMAPMD_10P_USRSCONF      56 /* 10P upstream ReedSolomon config */
#define MDIO_PMAPMD_10P_TONEGROUP1    57 /* 10P tone group 1 */
#define MDIO_PMAPMD_10P_TONEGROUP2    58 /*		   2 */
#define MDIO_PMAPMD_10P_TONEPARAM1    59 /* 10P tone parameter 1 */
#define MDIO_PMAPMD_10P_TONEPARAM2    60 /*		       2 */
#define MDIO_PMAPMD_10P_TONEPARAM3    61 /*		       3 */
#define MDIO_PMAPMD_10P_TONEPARAM4    62 /*		       4 */
#define MDIO_PMAPMD_10P_TONEPARAM5    63 /*		       5 */
#define MDIO_PMAPMD_10P_TONECTLACTN   64 /* 10P tone control action */
#define MDIO_PMAPMD_10P_TONESTAT1     65 /* 10P tone status 1 */
#define MDIO_PMAPMD_10P_TONESTAT2     66 /*		    2 */
#define MDIO_PMAPMD_10P_TONESTAT3     67 /*		    3 */
#define MDIO_PMAPMD_10P_OUTINDICAT    68 /* 10P outgoing indicator bits */
#define MDIO_PMAPMD_10P_ININDICAT     69 /* 10P incoming indicator bits */
#define MDIO_PMAPMD_10P_CYCLICEXTCNF  70 /* 10P cyclic extension config. */
#define MDIO_PMAPMD_10P_ATTAINDSDR    71 /* 10P attainable downstream DR */
	/* Values from 72 to 79 are reserved */
#define MDIO_PMAPMD_2B_GENPARAM	      80 /* 2B general parameter */
#define MDIO_PMAPMD_2B_PMDPARAM1      81 /* 2B PMD parameter 1 */
#define MDIO_PMAPMD_2B_PMDPARAM2      82 /*		     2 */
#define MDIO_PMAPMD_2B_PMDPARAM3      83 /*		     3 */
#define MDIO_PMAPMD_2B_PMDPARAM4      84 /*		     4 */
#define MDIO_PMAPMD_2B_PMDPARAM5      85 /*		     5 */
#define MDIO_PMAPMD_2B_PMDPARAM6      86 /*		     6 */
#define MDIO_PMAPMD_2B_PMDPARAM7      87 /*		     7 */
#define MDIO_PMAPMD_2B_PMDPARAM8      88 /*		     8 */
#define MDIO_PMAPMD_2B_CODEVIOERRCNT  89 /* 2B code violation errors counter */
#define MDIO_PMAPMD_2B_LPCODEVIOERR   90 /* 2B LP code violation errors cnt */
#define MDIO_PMAPMD_2B_ERRSECCNT      91 /* 2B errored seconds counter */
#define MDIO_PMAPMD_2B_LPERRSEC	      92 /* 2B LP errored seconds */
#define MDIO_PMAPMD_2B_SEVERRSECCNT   93 /* 2B severely errored seconds cnt */
#define MDIO_PMAPMD_2B_LPSEVERRSECCNT 94 /* 2B LP severely errored secs cnt */
#define MDIO_PMAPMD_2B_LOSWCNT	      95 /* 2B LOSW counter */
#define MDIO_PMAPMD_2B_LPLOSW	      96 /* 2B LP LOSW */
#define MDIO_PMAPMD_2B_UNAVSECCNT     97 /* 2B unavailable seconds counter */
#define MDIO_PMAPMD_2B_LPUNAVSECCNT   98 /* 2B LP unavailable seconds count */
#define MDIO_PMAPMD_2B_STATDEFECT     99 /* 2B state defects */
#define MDIO_PMAPMD_2B_LPSTATDEFECT   100 /* 2B LP state defects */
#define MDIO_PMAPMD_2B_NEGOCONSTEL    101 /* 2B negotiated constellation */
#define MDIO_PMAPMD_2B_EXTPMDPARAM1   102 /* 2B extended PMD parameters 1 */
#define MDIO_PMAPMD_2B_EXTPMDPARAM2   103 /*				2 */
#define MDIO_PMAPMD_2B_EXTPMDPARAM3   104 /*				3 */
#define MDIO_PMAPMD_2B_EXTPMDPARAM4   105 /*				4 */
#define MDIO_PMAPMD_2B_EXTPMDPARAM5   106 /*				5 */
#define MDIO_PMAPMD_2B_EXTPMDPARAM6   107 /*				6 */
#define MDIO_PMAPMD_2B_EXTPMDPARAM7   108 /*				7 */
#define MDIO_PMAPMD_2B_EXTPMDPARAM8   109 /*				8 */
	/* Values from 110 to 128 are reserved */
#define MDIO_PMAPMD_MULTGT_STAT	       129 /* MultiGBASE-T status */

#define MDIO_PMAPMD_MULTGT_PASWPOLAR   130 /* MultiGBASE-T pair swap & polar */
#define   PMAPMD_MULTGT_PASWPOLAR_D	__BIT(11) /* Pair D polarity */
#define   PMAPMD_MULTGT_PASWPOLAR_C	__BIT(10) /*	C	   */
#define   PMAPMD_MULTGT_PASWPOLAR_B	__BIT(9)  /*	B	   */
#define   PMAPMD_MULTGT_PASWPOLAR_A	__BIT(8)  /*	A	   */
#define   PMAPMD_MULTGT_PASWPOLAR_MDI	__BITS(1, 0) /* MDI/MDI-X connection */
#define   PMAPMD_MULTGT_PASWPOLAR_MDI_NORMAL 3 /* No crossover */
#define   PMAPMD_MULTGT_PASWPOLAR_MDI_X	     0 /* Pair A/B and C/D crossover */

#define MDIO_PMAPMD_MULTGT_TXPWBOSHRCH 131 /*
					    * MultiGBASE-T power backoff and
					    * PHY short reach setting
					    */
#define MDIO_PMAPMD_MULTGT_TSTMODE     132 /* MultiGBASE-T test mode */
#define MDIO_PMAPMD_SNROMGNA	       133 /* SNR operating margin chann. A */
#define MDIO_PMAPMD_SNROMGNB	       134 /*				  B */
#define MDIO_PMAPMD_SNROMGNC	       135 /*				  C */
#define MDIO_PMAPMD_SNROMGND	       136 /*				  D */
#define MDIO_PMAPMD_MINMGNA	       137 /* Minimum margin channel A */
#define MDIO_PMAPMD_MINMGNB	       138 /*			     B */
#define MDIO_PMAPMD_MINMGNC	       139 /*			     C */
#define MDIO_PMAPMD_MINMGND	       140 /*			     D */
#define MDIO_PMAPMD_SIGPWRA	       141 /* RX signal power channel A */
#define MDIO_PMAPMD_SIGPWRB	       142 /*			      B */
#define MDIO_PMAPMD_SIGPWRC	       143 /*			      C */
#define MDIO_PMAPMD_SIGPWRD	       144 /*			      D */
#define MDIO_PMAPMD_MULTGT_SKEWDLY1    145 /* MultiGBASE-T skew delay 1 */
#define MDIO_PMAPMD_MULTGT_SKEWDLY2    146 /*			      2 */
#define MDIO_PMAPMD_MULTGT_FSTRETSTATCTRL 147 /*
					       * MultiGBASE-T fast retrain
					       * status & control
					       */
	/* Values from 148 to 149 are reserved */
#define MDIO_PMAPMD_BASER_PMDCTRL	150 /* BASE-R PMD control */
#define MDIO_PMAPMD_BASER_PMDSTAT	151 /* BASE-R PMD status */
#define MDIO_PMAPMD_BASER_LPCOEFUPDL0	152 /* BASE-R LP coeffici. update ln0*/
#define MDIO_PMAPMD_BASER_LPSTATRPTL0	153 /* BASE-R LP status report lane0 */
#define MDIO_PMAPMD_BASER_LDCOEFFUPDL0	154 /* BASE-R LD coeffici. update ln0*/
#define MDIO_PMAPMD_BASER_LDSTATRPTL0	155 /* BASE-R LD status report lane0 */
#define MDIO_PMAPMD_BASER_STAT2		156 /* BASE-R PMD status 2 */
#define MDIO_PMAPMD_BASER_STAT3		157 /*			 3 */
	/* Values from 158 to 159 are reserved */
#define MDIO_PMAPMD_1000KXCTRL		160 /* 1000BASE-KX control */
#define MDIO_PMAPMD_1000KXSTAT		161 /* 1000BASE-KX status */
#define MDIO_PMAPMD_PMAOVHDCTRL1	162 /* PMA Overhead Control 1 */
#define MDIO_PMAPMD_PMAOVHDCTRL2	163 /*			    2 */
#define MDIO_PMAPMD_PMAOVHDCTRL3	164 /*			    3 */
#define MDIO_PMAPMD_PMAOVHDSTAT1	165 /* PMA Overhead Status 1 */
#define MDIO_PMAPMD_PMAOVHDSTAT2	166 /*			   2 */
	/* Values from 167 to 169 are reserved */
#define MDIO_PMAPMD_BASER_FECABLTY	170 /* BASE-R FEC ability */
#define MDIO_PMAPMD_BASER_FECCTRL	171 /* BASE-R FEC control */
#define MDIO_PMAPMD_BASER_FECCOBLCNTL	172 /* BASE-R FEC correct. blks cntL */
#define MDIO_PMAPMD_BASER_FECCOBLCNTH	173 /*				cntH */
#define MDIO_PMAPMD_BASER_FECUNCOBLCNTL	174 /* BASE-R FEC uncorr. blks cnt L */
#define MDIO_PMAPMD_BASER_FECUNCOBLCNTH	175 /*                         cnt H */
	/* Values from 176 to 178 are reserved */
#define MDIO_PMAPMD_CAUI4C2MRECCTLE	179 /* CAUI-4 C2M recommended CTLE */
#define MDIO_PMAPMD_CAUI4C2CTERDIL0	180 /* CAUI-4 C2C TxEq RxDir lane 0 */
#define MDIO_PMAPMD_CAUI4C2CTERDIL1	181 /*				  1 */
#define MDIO_PMAPMD_CAUI4C2CTERDIL2	182 /*				  2 */
#define MDIO_PMAPMD_CAUI4C2CTERDIL3	183 /*				  3 */
#define MDIO_PMAPMD_CAUI4C2CTETDEL0	184 /* CAUI-4 C2C TxEq TxDet lane 0 */
#define MDIO_PMAPMD_CAUI4C2CTETDEL1	185 /*				  1 */
#define MDIO_PMAPMD_CAUI4C2CTETDEL2	186 /*				  2 */
#define MDIO_PMAPMD_CAUI4C2CTETDEL3	187 /*				  3 */
	/* Values from 188 to 199 are reserved */
#define MDIO_PMAPMD_RSFECCTRL		200 /* RS-FEC Control */
#define MDIO_PMAPMD_RSFECSTAT		201 /* RS-FEC Status */
#define MDIO_PMAPMD_RSFECCORRCWCNTL	202 /* RS-FEC correct. codeword cntL */
#define MDIO_PMAPMD_RSFECCORRCWCNTH	203 /*				   H */
#define MDIO_PMAPMD_RSFECUNCORRCWCNTL	204 /* RS-FEC uncorre. codeword cntL */
#define MDIO_PMAPMD_RSFECUNCORRCWCNTH	205 /*				   H */
#define MDIO_PMAPMD_RSFECLANEMAP	206 /* RS-FEC Lane Mapping */
	/* Values from 207 to 209 are reserved */
#define MDIO_PMAPMD_RSFECSMBLERRCNTL(x)	    /* RS-FEC Symbol Error CntLow */ \
					(210 + ((x) * 2)) /* lane 0 to 3 */
#define MDIO_PMAPMD_RSFECSMBLERRCNTH(x)	    /* RS-FEC Symbol Error CntHigh */ \
					(211 + ((x) * 2)) /* lane 0 to 3 */
	/* Values from 218 to 229 are reserved */
#define MDIO_PMAPMD_RSFECBIPERRCNT(x)	    /* RS-FEC BIP Error Counter */ \
					(230 + (x)) /* lane 0 to 19 */
#define MDIO_PMAPMD_RSFECPCSLMAP(x)	    /* RS-FEC PCS Lane Mapping */ \
					(250 + (x)) /* lane 0 to 19 */
	/* Values from 270 to 279 are reserved */
#define MDIO_PMAPMD_RSFECPCSALGNSTAT1	280 /* RS-FEC PCS Alignment Status 1 */
#define MDIO_PMAPMD_RSFECPCSALGNSTAT2	281 /*				   2 */
#define MDIO_PMAPMD_RSFECPCSALGNSTAT3	282 /*				   3 */
#define MDIO_PMAPMD_RSFECPCSALGNSTAT4	283 /*				   4 */
	/* Values from 284 to 299 are reserved */
#define MDIO_PMAPMD_BASER_FECCORBLKCNTL(x)  /* BASE-R FEC Corr. Blk. Cnt L */ \
					(300 + ((x) * 2)) /* lane0 to 19 */
#define MDIO_PMAPMD_BASER_FECCORBLKCNTH(x)  /* BASE-R FEC Corr. Blk. Cnt H */ \
					(301 + ((x) * 2)) /* lane0 to 19 */
	/* Values from 340 to 699 are reserved */
#define MDIO_PMAPMD_BASER_FECUNCORBLKCNTL(x) /* BASE-R FEC UnCorr. Blk. CntL*/ \
					(700 + ((x) * 2)) /* lane0 to 19 */
#define MDIO_PMAPMD_BASER_FECUNCORBLKCNTH(x) /* BASE-R FEC UnCorr. Blk. CntH*/ \
					(701 + ((x) * 2)) /* lane0 to 19 */
	/* Values from 740 to 1099 are reserved */
#define MDIO_PMAPMD_BASER_LPCOEFUPD(x)	    /* BASE-R LP coefficient update */\
					(1100 + (x)) /* lane0 to 9 */
	/* Values from 1110 to 1199 are reserved */
#define MDIO_PMAPMD_BASER_LPSTATRPT(x)	    /* BASE-R LP status report */ \
					(1200 + (x)) /* lane0 to 9 */
	/* Values from 1210 to 1299 are reserved */
#define MDIO_PMAPMD_BASER_LDCOEFUPD(x)	    /* BASE-R LD coefficient update */\
					(1300 + (x)) /* lane0 to 9 */
	/* Values from 1310 to 1399 are reserved */
#define MDIO_PMAPMD_BASER_LDSTATRPT(x)	    /* BASE-R LD status report */ \
					(1400 + (x)) /* lane0 to 9 */
	/* Values from 1410 to 1449 are reserved */
#define MDIO_PMAPMD_PMDTRAINPATTERN(x)	    /* PMD training pattern */	\
					(1450 + (x)) /* lane0 to 3 */
	/* Values from 1454 to 1499 are reserved */
#define MDIO_PMAPMD_TSTPAT		1500 /* Test-pattern ability */
#define MDIO_PMAPMD_PRBSPATTSTCTRL	1501 /* PRBS pattern testing control */
	/* Values from 1502 to 1509 are reserved */
#define MDIO_PMAPMD_SQWVTSTCTRL		1510 /* Square wave testing control */
	/* Values from 1511 to 1599 are reserved */
#define MDIO_PMAPMD_PRBSTXERRCNT(x)	     /* PRBS Tx Error Counter */ \
					(1600 + (x)) /* lane0 to 9 */
	/* Values from 1610 to 1699 are reserved */
#define MDIO_PMAPMD_PRBSRXERRCNT(x)	     /* PRBS Rx Error Counter */ \
					(1700 + (x)) /* lane0 to 9 */
	/* Values from 1710 to 1799 are reserved */
#define MDIO_PMAPMD_TSYNC_CAP	    1800 /* TimeSync PMA/PMD capability */
#define MDIO_PMAPMD_TSYNC_TXMAXDLYL 1801 /* TimeSync PMA/PMD TX MAX delay L */
#define MDIO_PMAPMD_TSYNC_TXMAXDLYH 1802 /*				  H */
#define MDIO_PMAPMD_TSYNC_TXMINDLYL 1803 /*		     TX MIN delay L */
#define MDIO_PMAPMD_TSYNC_TXMINDLYH 1804 /*				  H */
#define MDIO_PMAPMD_TSYNC_RXMAXDLYL 1805 /*		     RX MAX delay L */
#define MDIO_PMAPMD_TSYNC_RXMAXDLYH 1806 /*				  H */
#define MDIO_PMAPMD_TSYNC_RXMINDLYL 1807 /*		     RX MIN delay L */
#define MDIO_PMAPMD_TSYNC_RXMINDLYH 1808 /*				  H */
	/* Values from 1809 to 32767 are reserved */
	/* Values from 32768 to 65535 are vendor specific */

/*
 * WIS registers.
 * Table 45-213
 */
#define MDIO_WIS_CTRL1		0   /* WIS control 1 */
#define MDIO_WIS_STAT1		1   /* WIS status 1 */
#define MDIO_WIS_DEVID1		2   /* WIS device identifier 1 */
#define MDIO_WIS_DEVID2		3   /*			     2 */
#define MDIO_WIS_SPEED		4   /* WIS speed ability */
#define MDIO_WIS_DEVS1		5   /* WIS devices in package 1 */
#define MDIO_WIS_DEVS2		6   /*			      2 */
#define MDIO_WIS_10GCTRL2	7   /* 10G WIS control 2 */
#define MDIO_WIS_10GSTAT2	8   /* 10G WIS status 2 */
#define MDIO_WIS_10GTSTERRCNT	9   /* 10G WIS test-pattern error counter */
	/* Values from 10 to 13 are reserved */
#define MDIO_WIS_PKGID1		14  /* WIS package identifier 1 */
#define MDIO_WIS_PKGID2		15  /*			      2 */
	/* Values from 16 to 32 are reserved */
#define MDIO_WIS_10GSTAT3	33  /* 10G WIS status 3 */
	/* Values from 34 to 36 are reserved */
#define MDIO_WIS_10GFARENDPBERRCNT 37 /* 10G WIS far end path block errcnt */
	/* Value 38 is reserved */
#define MDIO_WIS_J1XMIT(x)	    /* 10G WIS J1 transmit */		      \
				(39 + ((x) / 2)) /* 0to15. L8=even, H8=odd */

#define MDIO_WIS_J1RCV(x)	    /* 10G WIS J1 receive */		      \
				(47 + ((x) / 2)) /* 0to15. L8=even, H8=odd */
#define MDIO_WIS_FARENDLBIPERR1	55  /* 10G WIS far end line BIP errors 1 */
#define MDIO_WIS_FARENDLBIPERR2	56  /*				       2 */
#define MDIO_WIS_LBIPERR1	57  /* 10G WIS line BIP errors 1 */
#define MDIO_WIS_LBIPERR2	58  /*			       2 */
#define MDIO_WIS_PBERRCNT	59  /* 10G WIS path block error count */
#define MDIO_WIS_SECBIPERRCNT	60  /* 10G WIS section BIP error count */
	/* Values from 61 to 63 are reserved */
#define MDIO_WIS_J0XMIT(x)	    /* 10G WIS J0 transmit */		      \
				(64 + ((x) / 2)) /* 0to15. L8=even, H8=odd */

#define MDIO_WIS_J0RCV(x)	    /* 10G WIS J0 receive */		      \
				(72 + ((x) / 2)) /* 0to15. L8=even, H8=odd */
	/* Values from 80 to 1799 are reserved */
#define MDIO_WIS_TSYNC_CAP	1800 /* TimeSync WIS capability */
#define MDIO_WIS_TSYNC_TXMAXDLYL 1801 /* TimeSync WIS TX MAX delay L */
#define MDIO_WIS_TSYNC_TXMAXDLYH 1802 /*			   H */
#define MDIO_WIS_TSYNC_TXMINDLYL 1803 /*		 MIN delay L */
#define MDIO_WIS_TSYNC_TXMINDLYH 1804 /*			   H */
#define MDIO_WIS_TSYNC_RXMAXDLYL 1805 /*	      RX MAX delay L */
#define MDIO_WIS_TSYNC_RXMAXDLYH 1806 /*			   H */
#define MDIO_WIS_TSYNC_RXMINDLYL 1807 /*		 MIN delay L */
#define MDIO_WIS_TSYNC_RXMINDLYH 1808 /*			   H */
	/* Values from 1809 to 32767 are reserved */
	/* Values from 32768 to 65535 are vendor specific */

/*
 * PCS registers.
 * Table 45-233
 */
#define MDIO_PCS_CTRL1		0   /* PCS control 1 */
#define MDIO_PCS_STAT1		1   /* PCS status 1 */
#define MDIO_PCS_DEVID1		2   /* PCS device identifier 1 */
#define MDIO_PCS_DEVID2		3   /*			     2 */
#define MDIO_PCS_SPEED		4   /* PCS speed ability */
#define MDIO_PCS_DEVS1		5   /* PCS devices in package 1 */
#define MDIO_PCS_DEVS2		6   /*			      2 */
#define MDIO_PCS_CTRL2		7   /* PCS control 2 */
#define MDIO_PCS_STAT2		8   /* PCS status 2 */
	/* Values from 9 to 13 are reserved */
#define MDIO_PCS_PKGID1		14  /* PCS package identifier 1 */
#define MDIO_PCS_PKGID2		15  /*			      2 */
	/* Values from 16 to 19 are reserved */
#define MDIO_PCS_EEECTRLCAP	20  /* EEE control and capability */
	/* Value 21 is reserved */
#define MDIO_PCS_EEEWKERRCNT	22  /* EEE wake error counter */
	/* Value 23 is reserved */
#define MDIO_PCS_10GXSTAT	24  /* 10G-X PCS status */
#define MDIO_PCS_10GXSTSCTRL	25  /* 10G-X PCS test control */
	/* Values from 26 to 31 are reserved */
#define MDIO_PCS_BASER_MULTGT_STAT1 32 /* BASE-R & MultiGBASE-T PCS status 1 */
#define MDIO_PCS_BASER_MULTGT_STAT2 33 /*				   2 */
#define MDIO_PCS_BASER_TPSEEDA(x)   /* BASE-R PCS test pattern seed A */ \
				(34 + (x)) /* 0 to 3 */
#define MDIO_PCS_BASER_TPSEEDB(x)   /* BASE-R PCS test pattern seed B */ \
				(38 + (x)) /* 0 to 3 */
#define MDIO_PCS_BASER_TPCTRL	42  /* BASE-R PCS test pattern control */
#define MDIO_PCS_BASER_TPERRCNT	43  /* BASE-R PCS test pattern error counter */
#define MDIO_PCS_BERHIORDERCNT	44  /* BER high order counter */
#define MDIO_PCS_ERRBHIORDERCNT	45  /* Errored blocks high order counter */
	/* Values from 46 to 49 are reserved */
#define MDIO_PCS_MLBRPCSALGNSTAT1 50 /* Multi-lane BASE-R PCS Align. stat 1 */
#define MDIO_PCS_MLBRPCSALGNSTAT2 51 /*					  2 */
#define MDIO_PCS_MLBRPCSALGNSTAT3 52 /*					  3 */
#define MDIO_PCS_MLBRPCSALGNSTAT4 53 /*					  4 */
	/* Values from 54 to 59 are reserved */
#define MDIO_PCS_10P2B_CAP	   60 /* 10P/2B capability */
#define MDIO_PCS_10P2B_CTRL	   61 /* 10P/2B PCS control */
#define MDIO_PCS_10P2B_PMEAVAIL1   62 /* 10P/2B PME available 1 */
#define MDIO_PCS_10P2B_PMEAVAIL2   63 /*		      2 */
#define MDIO_PCS_10P2B_PMEAGGRG1   64 /*	    aggregate 1 */
#define MDIO_PCS_10P2B_PMEAGGRG2   65 /*		      2 */
#define MDIO_PCS_10P2B_PAFRXERRCNT 66 /* 10P/2B PAF RX error counter */
#define MDIO_PCS_10P2B_PAFSMLFRCNT 67 /* 10P/2B PAF small fragment counter */
#define MDIO_PCS_10P2B_PAFLARFLCNT 68 /* 10P/2B PAF large fragment counter */
#define MDIO_PCS_10P2B_PAFOVFLCNT  69 /* 10P/2B PAF overflow counter */
#define MDIO_PCS_10P2B_PAFBADFLCNT 70 /* 10P/2B PAF bad fragments counter */
#define MDIO_PCS_10P2B_PAFLSTFLCNT 71 /* 10P/2B PAF lost fragments counter */
#define MDIO_PCS_10P2B_PAFLSTSTFLCNT 72 /* 10P/2B PAF lost starts of fr. cnt */
#define MDIO_PCS_10P2B_PAFLSTENFLCNT 73 /* 10P/2B PAF lost ends of fr. count */
#define MDIO_PCS_10GPRFECABLTY	    74 /* 10G-PR & 10/1G-PRX FEC ability */
#define MDIO_PCS_10GPRFECCTRL	    75 /* 10G-PR & 10/1G-PRX FEC control */
#define MDIO_PCS_10GPRCOFECCOCNT1   76 /* 10(/1)G-PR(X) corrected FECcodecnt1*/
#define MDIO_PCS_10GPRCOFECCOCNT2   77 /* 10(/1)G-PR(X) corrected FECcodecnt2*/
#define MDIO_PCS_10GPRUNCOFECCOCNT1 78 /* 10(/1)G-PR(X)uncorrected FECcdecnt1*/
#define MDIO_PCS_10GPRUNCOFECCOCNT2 79 /* 10(/1)G-PR(X)uncorrected FECcdecnt2*/
#define MDIO_PCS_10GPRBERMONTMRCTRL 80 /* 10(/1)G-PR(X) BER monitor tmr ctrl */
#define MDIO_PCS_10GPRBERMONSTAT    81 /* 10(/1)G-PR(X) BER monitor status */
#define MDIO_PCS_10GPRBERMONTHRCTRL 82 /* 10(/1)G-PR(X) BER mntr thresh ctrl */
	/* Values from 83 to 199 are reserved */
#define MDIO_PCS_BIPERRCNT(x)	    /* BIP Error Counter */	\
				(200 + (x)) /* lane 0 to 19 */
	/* Values from 220 to 399 are reserved */
#define MDIO_PCS_PCSLMAP(x)	    /* PCS Lane Mapping */	\
				(400 + (x)) /* lane 0 to 19 */
	/* Values from 420 to 1799 are reserved */
#define MDIO_PCS_TSYNC_CAP	 1800 /* TimeSync PCS capability */
#define MDIO_PCS_TSYNC_TXMAXDLYL 1801 /* TimeSync PCS TX MAX delay L */
#define MDIO_PCS_TSYNC_TXMAXDLYH 1802 /*			   H */
#define MDIO_PCS_TSYNC_TXMINDLYL 1803 /*		 MIN delay L */
#define MDIO_PCS_TSYNC_TXMINDLYH 1804 /*			   H */
#define MDIO_PCS_TSYNC_RXMAXDLYL 1805 /*	      RX MAX delay L */
#define MDIO_PCS_TSYNC_RXMAXDLYH 1806 /*			   H */
#define MDIO_PCS_TSYNC_RXMINDLYL 1807 /*		 MIN delay L */
#define MDIO_PCS_TSYNC_RXMINDLYH 1808 /*			   H */
	/* Values from 1809 to 32767 are reserved */
	/* Values from 32768 to 65535 are vendor specific */

/*
 * PHY XS registers.
 * Table 45-314
 */
#define MDIO_PHYXS_CTRL1	0   /* PHY XS control 1 */
#define MDIO_PHYXS_STAT1	1   /* PHY XS status 1 */
#define MDIO_PHYXS_DEVID1	2   /* PHY XS device identifier 1 */
#define MDIO_PHYXS_DEVID2	3   /*				2 */
#define MDIO_PHYXS_SPEED	4   /* PHY XS speed ability */
#define MDIO_PHYXS_DEVS1	5   /* PHY XS devices in package 1 */
#define MDIO_PHYXS_DEVS2	6   /*				 2 */
	/* Value 7 is reserved */
#define MDIO_PHYXS_STAT2	8   /* PHY XS status 2 */
	/* Values from 9 to 13 are reserved */
#define MDIO_PHYXS_PKGID1	14  /* PHY XS package identifier 1 */
#define MDIO_PHYXS_PKGID2	15  /*				 2 */
	/* Values from 16 to 19 are reserved */
#define MDIO_PHYXS_EEECAP	20  /* EEE capability */
	/* Value 21 is reserved */
#define MDIO_PHYXS_EEEWKERRCNT	22  /* EEE wake error counter */
	/* Value 23 is reserved */
#define MDIO_PHYXS_10GXGXSLNSTAT 24 /* 10G PHY XGXS lane status */
#define MDIO_PHYXS_10GXGXSTSCTRL 25 /* 10G PHY XGXS test control */
	/* Values from 26 to 1799 are reserved */
#define MDIO_PHYXS_TSYNC_CSP	   1800 /* TimeSync PHY XS capability */
#define MDIO_PHYXS_TSYNC_TXMAXDLYL 1801 /* TimeSync PHY XS TX MAX delay L */
#define MDIO_PHYXS_TSYNC_TXMAXDLYH 1802 /*				H */
#define MDIO_PHYXS_TSYNC_TXMINDLYL 1803 /*		      MIN delay L */
#define MDIO_PHYXS_TSYNC_TXMINDLYH 1804 /*				H */
#define MDIO_PHYXS_TSYNC_RXMAXDLYL 1805 /*		   RX MAX delay L */
#define MDIO_PHYXS_TSYNC_RXMAXDLYH 1806 /*				H */
#define MDIO_PHYXS_TSYNC_RXMINDLYL 1807 /*		      MIN delay L */
#define MDIO_PHYXS_TSYNC_RXMINDLYH 1808 /*				H */
	/* Values from 1809 to 32767 are reserved */
	/* Values from 32768 to 65535 are vendor specific */

/*
 * DTE XS registers.
 * Table 45-339
 */
#define MDIO_DTEXS_CTRL1	0   /* DTE XS control 1 */
#define MDIO_DTEXS_STAT1	1   /* DTE XS status 1 */
#define MDIO_DTEXS_DEVID1	2   /* DTE XS device identifier 1 */
#define MDIO_DTEXS_DEVID2	3   /*				2 */
#define MDIO_DTEXS_SPEED	4   /* DTE XS speed ability */
#define MDIO_DTEXS_DEVS1	5   /* DTE XS devices in package 1 */
#define MDIO_DTEXS_DEVS2	6   /*				 2 */
	/* Value 7 is reserved */
#define MDIO_DTEXS_STAT2	8   /* DTE XS status 2 */
	/* Values from 9 to 13 are reserved */
#define MDIO_DTEXS_PKGID1	14  /* DTE XS package identifier 1 */
#define MDIO_DTEXS_PKGID2	15  /*				 2 */
	/* Values from 16 to 19 are reserved */
#define MDIO_DTEXS_EEECAP	20  /* EEE capability */
	/* Value 21 is reserved */
#define MDIO_DTEXS_EEEWKERRCNT	22  /* EEE wake error counter */
	/* Value 23 is reserved */
#define MDIO_DTEXS_10GXGXSLNSTAT 24 /* 10G DTE XGXS lane status */
#define MDIO_DTEXS_10GXGXSTSCTRL 25 /* 10G DTE XGXS test control */
	/* Values from 26 to 1799 are reserved */
#define MDIO_DTEXS_TSYNC_CAP	   1800 /* TimeSync DTE XS capability */
#define MDIO_DTEXS_TSYNC_TXMAXDLYL 1801 /* TimeSync DTE XS TX MAX delay L */
#define MDIO_DTEXS_TSYNC_TXMAXDLYH 1802 /*				H */
#define MDIO_DTEXS_TSYNC_TXMINDLYL 1803 /*		      MIN delay L */
#define MDIO_DTEXS_TSYNC_TXMINDLYH 1804 /*				H */
#define MDIO_DTEXS_TSYNC_RXMAXDLYL 1805 /*		   RX MAX delay L */
#define MDIO_DTEXS_TSYNC_RXMAXDLYH 1806 /*				H */
#define MDIO_DTEXS_TSYNC_RXMINDLYL 1807 /*		      MIN delay L */
#define MDIO_DTEXS_TSYNC_RXMINDLYH 1808 /*				H */
	/* Values from 1809 to 32767 are reserved */
	/* Values from 32768 to 65535 are vendor specific */

/*
 * Transmission Convergence(TC) registers.
 * Table 45-364
 */
#define MDIO_TC_CTRL1		0   /* TC control 1 */
	/* Value 1 is reserved */
#define MDIO_TC_DEVID1		2   /* TC device identifier 1 */
#define MDIO_TC_DEVID2		3   /*			    2 */
#define MDIO_TC_SPEED		4   /* TC speed ability */
#define MDIO_TC_DEVS1		5   /* TC devices in package 1 */
#define MDIO_TC_DEVS2		6   /*			     2 */
	/* Values from 7 to 13 are reserved */
#define MDIO_TC_PKGID1		14  /* TC package identifier 1 */
#define MDIO_TC_PKGID2		15  /*			     2 */
#define MDIO_TC_10P2B_AGGDCCTRL	16  /* 10P/2B aggregation discovery control */
#define MDIO_TC_10P2B_AGGDCSTAT	17  /* 10P/2B aggregation&discovery status */
#define MDIO_TC_10P2B_AGGDCCODE1 18 /* 10P/2B aggregation discovery code 1 */
#define MDIO_TC_10P2B_AGGDCCODE2 19 /*					 2 */
#define MDIO_TC_10P2B_AGGDCCODE3 20 /*					 3 */
#define MDIO_TC_10P2B_LPPMEAGGCTRL 21 /* 10P/2B LP PME aggregate control */
#define MDIO_TC_10P2B_LPPMEAGGDAT1 22 /* 10P/2B LP PME aggregate data 1 */
#define MDIO_TC_10P2B_LPPMEAGGDAT2 23 /*			      2 */
#define MDIO_TC_10P2B_CRCERRCNT	   24 /* 10P/2B TC CRC error counter */
#define MDIO_TC_10P2B_TPSCOVIOCNT1 25 /* 10P/2B TPS-TC coding violat. cnt. 1 */
#define MDIO_TC_10P2B_TPSCOVIOCNT2 26 /*				   2 */
#define MDIO_TC_10P2B_INDIC	   27 /* 10P/2B TC indications */
	/* Values from 28 to 1799 are reserved */
#define MDIO_TC_TSYNC_CAP	1800 /* TimeSync TC capability */
#define MDIO_TC_TSYNC_TXMAXDLYL	1801 /* TimeSync TC TX MAX delay L */
#define MDIO_TC_TSYNC_TXMAXDLYH	1802 /*				 H */
#define MDIO_TC_TSYNC_TXMINDLYL	1803 /*		       MIN delay L */
#define MDIO_TC_TSYNC_TXMINDLYH	1804 /*				 H */
#define MDIO_TC_TSYNC_RXMAXDLYL	1805 /*		    RX MAX delay L */
#define MDIO_TC_TSYNC_RXMAXDLYH	1806 /*				 H */
#define MDIO_TC_TSYNC_RXMINDLYL	1807 /*		       MIN delay L */
#define MDIO_TC_TSYNC_RXMINDLYH	1808 /*				 H */
	/* Values from 1809 to 32767 are reserved */
	/* Values from 32768 to 65535 are vendor specific */

/*
 * Auto-Negotiation registers.
 * Table 45-378
 */
#define MDIO_AN_CTRL1		0   /* AN control 1 */
#define   AN_CTRL1_ANRESET	0x8000 /* AN reset */
#define   AN_CTRL1_ENP		0x2000 /* Extended Next Page */
#define   AN_CTRL1_AUTOEN	0x1000 /* Auto-Negotiation enable */
#define   AN_CTRL1_STARTNEG	0x0200 /* Restart Auto-Negotiation */

#define MDIO_AN_STAT1		1   /* AN status 1 */
#define MDIO_AN_DEVID1		2   /* AN device identifier 1 */
#define MDIO_AN_DEVID2		3   /*			    2 */
	/* Value 4 is reserved */
#define MDIO_AN_DEVS1		5   /* AN devices in package 1 */
#define MDIO_AN_DEVS2		6   /*			     2 */
	/* Values from 7 to 13 are reserved */
#define MDIO_AN_PKGID1		14  /* AN package identifier 1 */
#define MDIO_AN_PKGID2		15  /*			     2 */

#define MDIO_AN_ADVERT1		16  /* AN advertisement 1 */
#define   AN_ADVERT1_NP		0x8000  /* Next Page */
#define   AN_ADVERT1_ACK	0x4000  /* Ack. */
#define   AN_ADVERT1_RF		0x2000  /* Remote Fault */
#define   AN_ADVERT1_TA		0x1fe0  /* Technology Ability (28.2.1.2(28B.2)) */
#define   AN_ADVERT1_SELECT	0x001f  /* Selector (73.6(28A)) */

#define MDIO_AN_ADVERT2		17  /* AN advertisement 2 */
#define MDIO_AN_ADVERT3		18  /*			3 */
#define MDIO_AN_LPBPABLTY1	19  /* AN LP base page ability 1 */
#define MDIO_AN_LPBPABLTY2	20  /*			       2 */
#define MDIO_AN_LPBPABLTY3	21  /*			       3 */
#define MDIO_AN_XNPXMIT1	22  /* AN XNP transmit 1 */
#define MDIO_AN_XNPXMIT2	23  /*		       2 */
#define MDIO_AN_XNPXMIT3	24  /*		       3 */
#define MDIO_AN_LPXNPABLTY1	25  /* AN LP XNP ability 1 */
#define MDIO_AN_LPXNPABLTY2	26  /*			 2 */
#define MDIO_AN_LPXNPABLTY3	27  /*			 3 */
	/* Values from 28 to 31 are reserved */

#define MDIO_AN_MULTGT_ANCTRL	32  /* MultiGBASE-T AN control */
#define   AN_MULTGT_ANCTRL_MS_MANU	0x8000 /* MASTER-SLAVE manual enable */
#define   AN_MULTGT_ANCTRL_MASTER	0x4000 /* MASTER-SLAVE config value */
#define   AN_MULTGT_ANCTRL_MULTIPORT	0x2000 /* Port type */
#define   AN_MULTGT_ANCTRL_10G_T	0x1000
#define   AN_MULTGT_ANCTRL_40G_T	0x0800
#define   AN_MULTGT_ANCTRL_25G_T	0x0400
#define   AN_MULTGT_ANCTRL_25G_T_FR	0x0200 /* Fast retrain (25G) */
#define   AN_MULTGT_ANCTRL_5G_T		0x0100
#define   AN_MULTGT_ANCTRL_2P5G_T	0x0080
#define   AN_MULTGT_ANCTRL_5G_T_FR	0x0040 /* Fast retrain (5G) */
#define   AN_MULTGT_ANCTRL_2P5G_T_FR	0x0020 /* Fast retrain (2.5G) */
				/* Value 0x0010 is reserved */
#define   AN_MULTGT_ANCTRL_40G_T_FR	0x0008 /* Fast retrain (40G) */
#define   AN_MULTGT_ANCTRL_10G_T_PRBS	0x0004 /* (deprecated) */
#define   AN_MULTGT_ANCTRL_10G_T_FR	0x0002 /* Fast retrain (10G) */
#define   AN_MULTGT_ANCTRL_10G_T_LT	0x0001 /* Loop timing */
#define   AN_MULTGT_ANCTRL_ABILITIES_2P5G_T				\
	(AN_MULTGT_ANCTRL_2P5G_T | AN_MULTGT_ANCTRL_2P5G_T_FR)
#define   AN_MULTGT_ANCTRL_ABILITIES_5G_T				\
	(AN_MULTGT_ANCTRL_5G_T | AN_MULTGT_ANCTRL_5G_T_FR)
#define   AN_MULTGT_ANCTRL_ABILITIES_10G_T				\
	(AN_MULTGT_ANCTRL_10G_T | AN_MULTGT_ANCTRL_10G_T_FR)
#define   AN_MULTGT_ANCTRL_ABILITIES_25G_T				\
	(AN_MULTGT_ANCTRL_25G_T | AN_MULTGT_ANCTRL_25G_T_FR)
#define   AN_MULTGT_ANCTRL_ABILITIES_40G_T				\
	(AN_MULTGT_ANCTRL_40G_T | AN_MULTGT_ANCTRL_40G_T_FR)
#define   AN_MULTGT_ANCTRL_ABILITIES_SPEED				\
	(AN_MULTGT_ANCTRL_ABILITIES_2P5G_T |				\
	    AN_MULTGT_ANCTRL_ABILITIES_5G_T |				\
	    AN_MULTGT_ANCTRL_ABILITIES_10G_T |				\
	    AN_MULTGT_ANCTRL_ABILITIES_25G_T |				\
	    AN_MULTGT_ANCTRL_ABILITIES_40G_T)

#define MDIO_AN_MULTGT_ANSTAT	33  /* MultiGBASE-T AN status */
	/* Values from 34 to 47 are reserved */
#define MDIO_AN_BPETHSTAT	48  /* BP Ethernet status */
	/* Values from 49 to 59 are reserved */

#define MDIO_AN_EEEADVERT	60  /* EEE advertisement 1 */
#define   AN_EEEADVERT_25G_R	0x4000	/* 25GBASE-R */
#define   AN_EEEADVERT_100G_CR4	0x2000	/* 100GBASE-CR4 */
#define   AN_EEEADVERT_100G_KR4	0x1000	/* 100GBASE-KR4 */
#define   AN_EEEADVERT_100G_KP4	0x0800	/* 100GBASE-KP4 */
#define   AN_EEEADVERT_100G_CR10 0x0400	/* 100GBASE-CR10 */
#define   AN_EEEADVERT_40G_T	0x0200	/* 40GBASE-T */
#define   AN_EEEADVERT_40G_CR4	0x0100	/* 40GBASE-CR4 */
#define   AN_EEEADVERT_40G_KR4	0x0080	/* 40GBASE-KR4 */
#define   AN_EEEADVERT_10G_KR	0x0040	/* 10GBASE-KR */
#define   AN_EEEADVERT_10G_KX4	0x0020	/* 10GBASE-KX4 */
#define   AN_EEEADVERT_1000_KX	0x0010	/* 1000BASE-KX */
#define   AN_EEEADVERT_10G_T	0x0008	/* 10GBASE-T */
#define   AN_EEEADVERT_1000_T	0x0004	/* 1000BASE-T */
#define   AN_EEEADVERT_100_TX	0x0002	/* 100BASE-TX */
#define   AN_EEEADVERT_25G_T	0x0001	/* 25GBASE-T */

#define MDIO_AN_EEELPABLTY	61  /* EEE LP ability 1 */
	/* bitmap is the same as MDIO_AN_EEEADVERT(7.60) */

#define MDIO_AN_EEEADVERT2	62  /* EEE advertisement 2 */
#define   AN_EEEADVERT2_5G_KR	0x0008
#define   AN_EEEADVERT2_2P5G_KX	0x0004
#define   AN_EEEADVERT2_5G_T	0x0002
#define   AN_EEEADVERT2_2P5G_T	0x0001

#define MDIO_AN_EEELPABLTY2	63  /* EEE LP ability 2 */
	/* Bitmap is the same as MDIO_AN_EEEADVERT2(7.61). */

#define MDIO_AN_MULTGT_ANCTRL2	64  /* MultiGBASE-T AN control 2 */
#define MDIO_AN_MULTGT_ANSTAT2	65  /* MultiGBASE-T AN status 2 */
	/* Values from 66 to 32767 are reserved */
	/* Values from 32768 to 65535 are vendor specific */

/*
 * Clause 22 extension registers.
 * Table 45-416
 */
	/* Values from 0 to 4 are reserved */
#define MDIO_CL22E_DEVS1	5   /* Clause 22 ext. devices in package 1 */
#define MDIO_CL22E_DEVS2	6   /*					 2 */
#define MDIO_CL22E_FECCAP	7   /* FEC capability */
#define MDIO_CL22E_FECCTRL	8   /* FEC control */
#define MDIO_CL22E_FECBHCVIOCNT	9   /* FEC buffer head coding violation cnt. */
#define MDIO_CL22E_FECCOBLCNT	10  /* FEC corrected blocks counter */
#define MDIO_CL22E_FECUNCOBLCNT	11  /* FEC uncorrected blocks counter */
	/* Values from 12 to 32767 are reserved */

/*
 * Vendor specific MMD 1 registers.
 * Table 45-422
 */
	/* Values from 0 to 1 are vendor specific */
#define MDIO_VSMMD1_DEVID1	2   /* Vendor specific MMD 1 device ident. 1 */
#define MDIO_VSMMD1_DEVID2	3   /*					   2 */
	/* Values from 4 to 7 are vendor specific */
#define MDIO_VSMMD1_STAT	8   /* Vendor specific MMD 1 status */
	/* Values from 9 to 13 are vendor specific */
#define MDIO_VSMMD1_PKGID1	14  /* Vendor specific MMD 1 package ident 1 */
#define MDIO_VSMMD1_PKGID2	15  /*					   2 */
	/* Values from 16 to 65535 are vendor specific */

/*
 * Vendor specific MMD 2 registers.
 * Table 45-424
 */
	/* Values from 0 to 1 are vendor specific */
#define MDIO_VSMMD2_DEVID1	2   /* Vendor specific MMD 2 device ident. 1 */
#define MDIO_VSMMD2_DEVID2	3   /*					   2 */
	/* Values from 4 to 7 are vendor specific */
#define MDIO_VSMMD2_STAT	8   /* Vendor specific MMD 2 status */
	/* Values from 9 to 13 are vendor specific */
#define MDIO_VSMMD2_PKGID1	14  /* Vendor specific MMD 2 package ident 1 */
#define MDIO_VSMMD2_PKGID2	15  /*					   2 */
	/* Values from 16 to 65535 are vendor specific */

#endif /* _DEV_MII_MDIO_H_ */
