/*	$NetBSD: mdio.h,v 1.3.32.1 2017/11/21 14:43:17 martin Exp $	*/

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
#define	_DEV_MII_MDIO_H_

/*
 * IEEE 802.3 Clause 45 definitions.
 * From:
 *	IEEE 802.3 2015
 *	IEEE 802.3at
 *	IEEE 802.3av
 */

/*
 * MDIO Manageable Device (MMD) addresses.
 * Table 45-1
 */
#define	MDIO_MMD_PMAPMD		1
#define	MDIO_MMD_WIS		2
#define	MDIO_MMD_PCS		3
#define	MDIO_MMD_PHYXS		4
#define	MDIO_MMD_DTEXS		5
#define	MDIO_MMD_TC		6
#define	MDIO_MMD_AN		7
#define	MDIO_MMD_SEPPMA1	8
#define	MDIO_MMD_SEPPMA2	9
#define	MDIO_MMD_SEPPMA3	10
#define	MDIO_MMD_SEPPMA4	11
#define	MDIO_MMD_CL22EXT	29
#define	MDIO_MMD_VNDSP1		30
#define	MDIO_MMD_VNDSP2		31

/*
 * PMA/PMD registers.
 * Table 45-3
 */
#define MDIO_PMAPMD_CTRL1		0   /* PMA/PMD control 1 */
#define PMAPMD_CTRL1_RESET	0x8000		/* Reset */
#define PMAPMD_CTRL1_SPEED0	0x2000		/* Speed selection (LSB) */
#define PMAPMD_CTRL1_LOWPWR	0x0800		/* Low power */
#define PMAPMD_CTRL1_SPEED1	0x0040		/* Speed selection (MSB) */
#define PMAPMD_CTRL1_SPEED2	0x003c		/* Speed selection (over 1G) */
#define PMAPMD_CTRL1_LOOP_REM	0x0002		/* PMA remote loopback */
#define PMAPMD_CTRL1_LOOP_LOC	0x0001		/* PMA local loopback */
#define PMAPMD_CTRL1_SPEED_SEL52 (PMAPMD_CTRL1_SPEED0 | PMAPMD_CTRL1_SPEED1)
#define PMAPMD_CTRL1_SPEED_MASK	(PMAPMD_CTRL1_SPEED_SEL52 \
	    | PMAPMD_CTRL1_SPEED2)
#define PMAPMD_CTRL1_SPEED_10	   0
#define PMAPMD_CTRL1_SPEED_100	   PMAPMD_CTRL1_SPEED0
#define PMAPMD_CTRL1_SPEED_1G	   PMAPMD_CTRL1_SPEED1
#define PMAPMD_CTRL1_SPEED_10G     PMAPMD_CTRL1_SPEED_SEL52
#define PMAPMD_CTRL1_SPEED_10PASS (PMAPMD_CTRL1_SPEED_SEL52 | (1 << 2))
#define PMAPMD_CTRL1_SPEED_40G	  (PMAPMD_CTRL1_SPEED_SEL52 | (2 << 2))
#define PMAPMD_CTRL1_SPEED_100G   (PMAPMD_CTRL1_SPEED_SEL52 | (3 << 2))

#define MDIO_PMAPMD_STAT1		1   /* PMA/PMD status 1 */
#define MDIO_PMAPMD_DEVID1		2   /* PMA/PMD device identifier 1 */
#define MDIO_PMAPMD_DEVID2		3   /* PMA/PMD device identifier 2 */
#define MDIO_PMAPMD_SPEED		4   /* PMA/PMD speed ability */
#define MDIO_PMAPMD_DEVS1		5   /* PMA/PMD devices in package 1 */
#define MDIO_PMAPMD_DEVS2		6   /* PMA/PMD devices in package 2 */
#define MDIO_PMAPMD_CTRL2		7   /* PMA/PMD control 2 */
#define MDIO_PMAPMD_10GSTAT2		8   /* 10G PMA/PMD status 2 */
#define MDIO_PMAPMD_10GTXDIS		9   /* 10G PMA/PMD transmit disable */
#define MDIO_PMAPMD_RXSIGDTCT		10  /* 10G PMD receive signal detect */
#define MDIO_PMAPMD_EXTABLTY		11  /* 10G PMA/PMD ext. ability reg */
#define MDIO_PMAPMD_P2MPABLTY		12  /* P2MP ability register(802.3av)*/
#define MDIO_PMAPMD_40G100GEXTABLTY	13  /* 40G/100G extended ability */
#define MDIO_PMAPMD_PKGID1		14  /* PMA/PMD package identifier 1 */
#define MDIO_PMAPMD_PKGID2		15  /* PMA/PMD package identifier 2 */
#define	MDIO_PMAPMD_EEECAP		16  /* PMA/PMD EEE capability */
	/* Values 17 to 29 are reserved */
#define MDIO_PMAPMD_10P2BCTRL		30  /* 10P/2B PMA/PMD control */
#define MDIO_PMAPMD_10P2BSTAT		31  /* 10P/2B PMA/PMD status */
#define MDIO_PMAPMD_10P2BLPCTRL		32  /* 10P/2B link partner PMA/D ctrl*/
#define MDIO_PMAPMD_10P2BLPSTAT		33  /* 10P/2B link partner PMA/D stat*/
	/* Values 34 to 35 are reserved */
#define MDIO_PMAPMD_10P2BLLOSCNT	36  /* 10P/2B link loss counter */
#define MDIO_PMAPMD_10P2BRXSNMGN	37  /* 10P/2B RX SNR margin */
#define MDIO_PMAPMD_10P2BLPRXSNMG	38  /* 10P/2B link partner RX SNR mgn*/
#define MDIO_PMAPMD_10P2BLINEATTN	39  /* 10P/2B line attenuation */
#define MDIO_PMAPMD_10P2BLPLINEATTN	40  /* 10P/2B link partner line atten*/
#define MDIO_PMAPMD_10P2BLQTHRES	41  /* 10P/2B line quality thresholds*/
#define MDIO_PMAPMD_10P2BLPLQLTHRES	42  /* 10P/2B link partner LQ thresh.*/
#define MDIO_PMAPMD_10PFECCOERRS	43  /* 10P FEC correctable errors cnt*/
#define MDIO_PMAPMD_10PFECUNCOERRS	44  /* 10P FEC uncorrectable err cnt*/
#define MDIO_PMAPMD_10PLPFECCOERRS	45  /* 10P LP FEC correctable err cnt*/
#define MDIO_PMAPMD_10PLPFECUNCOERRS	46  /* 10P LP FEC uncorrectable errcn*/
#define MDIO_PMAPMD_10PELECLENGTH	47  /* 10P electrical length */
#define MDIO_PMAPMD_10PLPELECLENGTH	48  /* 10P LP electrical length */
#define MDIO_PMAPMD_10PGENCONFIG	49  /* 10P PMA/PMD general config. */
#define MDIO_PMAPMD_10PPSDCONFIG	50  /* 10P PSD configuration */
#define MDIO_PMAPMD_10PDSDRCONF1	51  /* 10P downstream data rate cnf1 */
#define MDIO_PMAPMD_10PDSDRCONF2	52  /* 10P downstream data rate cnf2 */
#define MDIO_PMAPMD_10PDSRSCONF		53  /* 10P downstream ReedSolomon cnf*/
#define MDIO_PMAPMD_10PUSDR1		54  /* 10P upstream data rate cnf1 */
#define MDIO_PMAPMD_10PUSDR2		55  /* 10P upstream data rate cnf2 */
#define MDIO_PMAPMD_10PUSRSCONF		56  /* 10P upstream ReedSolomon cnf */
#define MDIO_PMAPMD_10PTONEGROUP1	57  /* 10P tone group 1 */
#define MDIO_PMAPMD_10PTONEGROUP2	58  /* 10P tone group 2 */
#define MDIO_PMAPMD_10PTONEPARAM1	59  /* 10P tone parameter 1 */
#define MDIO_PMAPMD_10PTONEPARAM2	60  /* 10P tone parameter 2 */
#define MDIO_PMAPMD_10PTONEPARAM3	61  /* 10P tone parameter 3 */
#define MDIO_PMAPMD_10PTONEPARAM4	62  /* 10P tone parameter 4 */
#define MDIO_PMAPMD_10PTONEPARAM5	63  /* 10P tone parameter 5 */
#define MDIO_PMAPMD_10PTONECTLACTN	64  /* 10P tone control action */
#define MDIO_PMAPMD_10PTONESTAT1	65  /* 10P tone status 1 */
#define MDIO_PMAPMD_10PTONESTAT2	66  /* 10P tone status 2 */
#define MDIO_PMAPMD_10PTONESTAT3	67  /* 10P tone status 3 */
#define MDIO_PMAPMD_10POUTINDICAT	68  /* 10P outgoing indicator bits */
#define MDIO_PMAPMD_10PININDICAT	69  /* 10P incoming indicator bits */
#define MDIO_PMAPMD_10PCYCLICEXTCNF	70  /* 10P cyclic extension config. */
#define MDIO_PMAPMD_10PATTAINDSDR	71  /* 10P attainable downstream DR */
	/* Values 72 to 79 are reserved */
#define MDIO_PMAPMD_2BGENPARAM		80  /* 2B general parameter */
#define MDIO_PMAPMD_2BPMDPARAM1		81  /* 2B PMD parameter 1 */
#define MDIO_PMAPMD_2BPMDPARAM2		82  /* 2B PMD parameter 2 */
#define MDIO_PMAPMD_2BPMDPARAM3		83  /* 2B PMD parameter 3 */
#define MDIO_PMAPMD_2BPMDPARAM4		84  /* 2B PMD parameter 4 */
#define MDIO_PMAPMD_2BPMDPARAM5		85  /* 2B PMD parameter 5 */
#define MDIO_PMAPMD_2BPMDPARAM6		86  /* 2B PMD parameter 6 */
#define MDIO_PMAPMD_2BPMDPARAM7		87  /* 2B PMD parameter 7 */
#define MDIO_PMAPMD_2BPMDPARAM8		88  /* 2B PMD parameter 8 */
#define MDIO_PMAPMD_2BCODEVIOERRCNT	89  /* 2B code violation errors cnt. */
#define MDIO_PMAPMD_2BLPCODEVIOERR	90  /* 2B LP code violation errors */
#define MDIO_PMAPMD_2BERRSECCNT		91  /* 2B errored seconds counter */
#define MDIO_PMAPMD_2BLPERRSEC		92  /* 2B LP errored seconds */
#define MDIO_PMAPMD_2BSEVERRSECCNT	93  /* 2B severely errored seconds cn*/
#define MDIO_PMAPMD_2BLPSEVERRSECCNT	94 /* 2B LP severely errored secs cn*/
#define MDIO_PMAPMD_2BLOSWCNT		95  /* 2B LOSW counter */
#define MDIO_PMAPMD_2BLPLOSW		96  /* 2B LP LOSW */
#define MDIO_PMAPMD_2BUNAVSECCNT	97  /* 2B unavailable seconds counter*/
#define MDIO_PMAPMD_2BLPUNAVSECCNT	98  /* 2B LP unavailable seconds cnt */
#define MDIO_PMAPMD_2BSTATDEFECT	99  /* 2B state defects */
#define MDIO_PMAPMD_2BLPSTATDEFECT	100 /* 2B LP state defects */
#define MDIO_PMAPMD_2BNEGOCONSTEL	101 /* 2B negotiated constellation */
#define MDIO_PMAPMD_2BEXTPMDPARAM1	102 /* 2B extended PMD parameters 1 */
#define MDIO_PMAPMD_2BEXTPMDPARAM2	103 /* 2B extended PMD parameters 2 */
#define MDIO_PMAPMD_2BEXTPMDPARAM3	104 /* 2B extended PMD parameters 3 */
#define MDIO_PMAPMD_2BEXTPMDPARAM4	105 /* 2B extended PMD parameters 4 */
#define MDIO_PMAPMD_2BEXTPMDPARAM5	106 /* 2B extended PMD parameters 5 */
#define MDIO_PMAPMD_2BEXTPMDPARAM6	107 /* 2B extended PMD parameters 6 */
#define MDIO_PMAPMD_2BEXTPMDPARAM7	108 /* 2B extended PMD parameters 7 */
#define MDIO_PMAPMD_2BEXTPMDPARAM8	109 /* 2B extended PMD parameters 8 */
	/* Values 110 to 128 are reserved */
#define MDIO_PMAPMD_10GTSTAT		129 /* 10GBASE-T status */
#define MDIO_PMAPMD_10GTPASWPOLAR	130 /* 10G-T pair swap & polarity */
#define MDIO_PMAPMD_10GTTXPWBOSHRCH	131 /* 10G-T PWR backoff&PHY shrt rch*/
#define MDIO_PMAPMD_10GTTSTMODE		132 /* 10G-T test mode */
#define MDIO_PMAPMD_10GTSNROMARGA	133 /* 10G-T SNR operating margin chA*/
#define MDIO_PMAPMD_10GTSNROMARGB	134 /* 10G-T SNR operating margin chB*/
#define MDIO_PMAPMD_10GTSNROMARGC	135 /* 10G-T SNR operating margin chC*/
#define MDIO_PMAPMD_10GTSNROMARGD	136 /* 10G-T SNR operating margin chD*/
#define MDIO_PMAPMD_10GTMINMARGA	137 /* 10G-T minimum margin ch. A */
#define MDIO_PMAPMD_10GTMINMARGB	138 /* 10G-T minimum margin ch. B */
#define MDIO_PMAPMD_10GTMINMARGC	139 /* 10G-T minimum margin ch. C */
#define MDIO_PMAPMD_10GTMINMARGD	140 /* 10G-T minimum margin ch. D */
#define MDIO_PMAPMD_10GTSIGPWRA		141 /* 10G-T RX signal power ch. A */
#define MDIO_PMAPMD_10GTSIGPWRB		142 /* 10G-T RX signal power ch. B */
#define MDIO_PMAPMD_10GTSIGPWRC		143 /* 10G-T RX signal power ch. C */
#define MDIO_PMAPMD_10GTSIGPWRD		144 /* 10G-T RX signal power ch. D */
#define MDIO_PMAPMD_10GTSKEWDLY1	145 /* 10G-T skew delay 1 */
#define MDIO_PMAPMD_10GTSKEWDLY2	146 /* 10G-T skew delay 2 */
#define MDIO_PMAPMD_10GTFSTRETSTATCTRL	147 /* 10G-T fast retrain stat&ctrl */
	/* Values 148 to 149 are reserved */
#define MDIO_PMAPMD_BASERPMDCTRL	150 /* BASE-R PMD control */
#define MDIO_PMAPMD_BASERPMDSTAT	151 /* BASE-R PMD status */
#define MDIO_PMAPMD_BASERLPCOEFUPDL0	152 /* BASE-R LP coeffici. update ln0*/
#define MDIO_PMAPMD_BASERLPSTATRPTL0	153 /* BASE-R LP status report lane0 */
#define MDIO_PMAPMD_BASERLDCOEFFUPDL0	154 /* BASE-R LD coeffici. update ln0*/
#define MDIO_PMAPMD_BASERLDSTATRPTL0	155 /* BASE-R LD status report lane0 */
#define MDIO_PMAPMD_BASERSTAT2		156 /* BASE-R PMD status 2 */
#define MDIO_PMAPMD_BASERSTAT3		157 /* BASE-R PMD status 3 */
	/* Values 158 to 159 are reserved */
#define MDIO_PMAPMD_1000KXCTRL		160 /* 1000BASE-KX control */
#define MDIO_PMAPMD_1000KXSTAT		161 /* 1000BASE-KX status */
#define MDIO_PMAPMD_PMAOVHDCTRL1	162 /* PMA Overhead Control 1 */
#define MDIO_PMAPMD_PMAOVHDCTRL2	163 /* PMA Overhead Control 2 */
#define MDIO_PMAPMD_PMAOVHDCTRL3	164 /* PMA Overhead Control 3 */
#define MDIO_PMAPMD_PMAOVHDSTAT1	165 /* PMA Overhead Status 1 */
#define MDIO_PMAPMD_PMAOVHDSTAT2	166 /* PMA Overhead Status 2 */
	/* Values 167 to 169 are reserved */
#define MDIO_PMAPMD_BASERFECABLTY	170 /* BASE-R FEC ability */
#define MDIO_PMAPMD_BASERFECCTRL	171 /* BASE-R FEC control */
#define MDIO_PMAPMD_10GRFECCOBLCNTL	172 /* 10G-R FEC corrected blks cntL */
#define MDIO_PMAPMD_10GRFECCOBLCNTH	173 /* 10G-R FEC corrected blks cntH */
#define MDIO_PMAPMD_10GRFECUNCOBLCNTL	174 /* 10G-R FEC uncorrect blks cntL */
#define MDIO_PMAPMD_10GRFECUNCOBLCNTH	175 /* 10G-R FEC uncorrect blks cntH */
	/* Values 176 to 178 are reserved */
#define MDIO_PMAPMD_CAUI4C2MRECCTLE	179 /* CAUI-4 Chip2Mod recomme. CTLE */
#define MDIO_PMAPMD_CAUI4C2CTERDIL0	180 /* CAUI-4 Ch2Ch TxEq RxDir lane0 */
#define MDIO_PMAPMD_CAUI4C2CTERDIL1	181 /* lane1 */
#define MDIO_PMAPMD_CAUI4C2CTERDIL2	182 /* lane2 */
#define MDIO_PMAPMD_CAUI4C2CTERDIL3	183 /* lane3 */
#define MDIO_PMAPMD_CAUI4C2CTETDEL0	184 /* CAUI-4 Ch2Ch TxEq TxDet lane0 */
#define MDIO_PMAPMD_CAUI4C2CTETDEL1	185 /* lane1 */
#define MDIO_PMAPMD_CAUI4C2CTETDEL2	186 /* lane2 */
#define MDIO_PMAPMD_CAUI4C2CTETDEL3	187 /* lane3 */
	/* Values 188 to 199 are reserved */
#define MDIO_PMAPMD_RSFECCTRL		200 /* RS-FEC Control */
#define MDIO_PMAPMD_RSFECSTAT		201 /* RS-FEC Status */
#define MDIO_PMAPMD_RSFECCORRCWCNTL	202 /* RS-FEC correct. codeword cntL */
#define MDIO_PMAPMD_RSFECCORRCWCNTH	203 /* RS-FEC correct. codeword cntH */
#define MDIO_PMAPMD_RSFECUNCORRCWCNTL	204 /* RS-FEC uncorre. codeword cntL */
#define MDIO_PMAPMD_RSFECUNCORRCWCNTH	205 /* RS-FEC uncorre. codeword cntH */
#define MDIO_PMAPMD_RSFECLANEMAP	206 /* RS-FEC Lane Mapping */
	/* Values 207 to 209 are reserved */
#define MDIO_PMAPMD_RSFECSMBLERRCNTL(x)	    /* RS-FEC Symbol Error CntLow */ \
					(210 + ((x) * 2)) /* lane 0 to 3 */
#define MDIO_PMAPMD_RSFECSMBLERRCNTH(x)	    /* RS-FEC Symbol Error CntHigh */ \
					(211 + ((x) * 2)) /* lane 0 to 3 */
	/* Values 218 to 229 are reserved */
#define MDIO_PMAPMD_RSFECBIPERRCNT(x)	    /* RS-FEC BIP Error Counter */ \
					(230 + (x)) /* lane 0 to 19 */
#define MDIO_PMAPMD_RSFECPCSLMAP(x)	    /* RS-FEC PCS Lane Mapping */ \
					(250 + (x)) /* lane 0 to 19 */
	/* Values 270 to 279 are reserved */
#define MDIO_PMAPMD_RSFECPCSALGNSTAT1	280 /* RS-FEC PCS Alignment Status 1 */
#define MDIO_PMAPMD_RSFECPCSALGNSTAT2	281 /* RS-FEC PCS Alignment Status 2 */
#define MDIO_PMAPMD_RSFECPCSALGNSTAT3	282 /* RS-FEC PCS Alignment Status 3 */
#define MDIO_PMAPMD_RSFECPCSALGNSTAT4	283 /* RS-FEC PCS Alignment Status 4 */
	/* Values 284 to 299 are reserved */
#define MDIO_PMAPMD_BASERFECCORBLKCNTL(x)    /* BASE-R FEC Corr. Blk. CntL */ \
					(300 + ((x) * 2)) /* lane0 to 19 */
#define MDIO_PMAPMD_BASERFECCORBLKCNTH(x)    /* BASE-R FEC Corr. Blk. CntH */ \
					(301 + ((x) * 2)) /* lane0 to 19 */
	/* Values 340 to 699 are reserved */
#define MDIO_PMAPMD_BASERFECUNCORBLKCNTL(x) /* BASE-R FEC UnCorr. Blk. CntL*/ \
					(700 + ((x) * 2)) /* lane0 to 19 */
#define MDIO_PMAPMD_BASERFECUNCORBLKCNTH(x) /* BASE-R FEC UnCorr. Blk. CntH*/ \
					(701 + ((x) * 2)) /* lane0 to 19 */
	/* Values 740 to 1099 are reserved */
#define MDIO_PMAPMD_BASERLPCOEFUPD(x)	    /* BASE-R LP coefficient update */\
					(1100 + (x)) /* lane0 to 9 */
	/* Values 1110 to 1199 are reserved */
#define MDIO_PMAPMD_BASERLPSTATRPT(x)	    /* BASE-R LP status report */ \
					(1200 + (x)) /* lane0 to 9 */
	/* Values 1210 to 1299 are reserved */
#define MDIO_PMAPMD_BASERLDCOEFUPD(x)	    /* BASE-R LD coefficient update */\
					(1300 + (x)) /* lane0 to 9 */
	/* Values 1310 to 1399 are reserved */
#define MDIO_PMAPMD_BASERLDSTATRPT(x)	    /* BASE-R LD status report */ \
					(1400 + (x)) /* lane0 to 9 */
	/* Values 1410 to 1449 are reserved */
#define MDIO_PMAPMD_PMDTRAINPATTERN(x)	    /* PMD training pattern */	\
					(1450 + (x)) /* lane0 to 3 */
	/* Values 1454 to 1499 are reserved */
#define MDIO_PMAPMD_TSTPAT		1500 /* Test-pattern ability */
#define MDIO_PMAPMD_PRBSPATTSTCTRL	1501 /* PRBS pattern testing control */
	/* Values 1502 to 1509 are reserved */
#define MDIO_PMAPMD_SQWVTSTCTRL		1510 /* Square wave testing control */
	/* Values 1511 to 1599 are reserved */
#define MDIO_PMAPMD_PRBSTXERRCNT(x)	     /* PRBS Tx Error Counter */ \
					(1600 + (x)) /* lane0 to 9 */
	/* Values 1610 to 1699 are reserved */
#define MDIO_PMAPMD_PRBSRXERRCNT(x)	     /* PRBS Rx Error Counter */ \
					(1700 + (x)) /* lane0 to 9 */
	/* Values 1710 to 1799 are reserved */
#define MDIO_PMAPMD_TSYNCCAP		1800 /* TimeSync PMA/PMD capability */
#define MDIO_PMAPMD_TSYNCTXMAXDLYL	1801 /* TimeSync PMAPMD TX MAXdelay L*/
#define MDIO_PMAPMD_TSYNCTXMAXDLYH	1802 /* TimeSync PMAPMD TX MAXdelay H*/
#define MDIO_PMAPMD_TSYNCTXMINDLYL	1803 /* TimeSync PMAPMD TX MINdelay L*/
#define MDIO_PMAPMD_TSYNCTXMINDLYH	1804 /* TimeSync PMAPMD TX MINdelay H*/
#define MDIO_PMAPMD_TSYNCRXMAXDLYL	1805 /* TimeSync PMAPMD RX MAXdelay L*/
#define MDIO_PMAPMD_TSYNCRXMAXDLYH	1806 /* TimeSync PMAPMD RX MAXdelay H*/
#define MDIO_PMAPMD_TSYNCRXMINDLYL	1807 /* TimeSync PMAPMD RX MINdelay L*/
#define MDIO_PMAPMD_TSYNCRXMINDLYH	1808 /* TimeSync PMAPMD RX MINdelay H*/
	/* Values 1809 to 32767 are reserved */
	/* Values 32768 to 65535 are vendor specific */

/*
 * WIS registers.
 * Table 45-99
 */
#define	MDIO_WIS_CTRL1		0	/* WIS control 1 */
#define	MDIO_WIS_STAT1		1	/* WIS status 1 */
#define	MDIO_WIS_DEVID1		2	/* WIS device identifier 1 */
#define	MDIO_WIS_DEVID2		3	/* WIS device identifier 2 */
#define	MDIO_WIS_SPEED		4	/* WIS speed ability */
#define	MDIO_WIS_DEVS1		5	/* WIS devices in package 1 */
#define	MDIO_WIS_DEVS2		6	/* WIS devices in package 2 */
#define	MDIO_WIS_10GCTRL2	7	/* 10G WIS control 2 */
#define	MDIO_WIS_10GSTAT2	8	/* 10G WIS status 2 */
#define	MDIO_WIS_10GTSTERRCNT	9	/* 10G WIS test-pattern error counter*/
	/* Values 10 to 13 are reserved */
#define	MDIO_WIS_PKGID1		14	/* WIS package identifier 1 */
#define	MDIO_WIS_PKGID2		15	/* WIS package identifier 2 */
	/* Values 16 to 32 are reserved */
#define	MDIO_WIS_10GSTAT3	33	/* 10G WIS status 3 */
	/* Values 34 to 36 are reserved */
#define	MDIO_WIS_10GFARENDPBERRCNT 37	/* 10G WIS far end path block errcnt */
	/* Value 38 is reserved */
#define	MDIO_WIS_J1XMIT(x)		/* 10G WIS J1 transmit */	     \
				(39 + ((x) / 2))/* 0to15. L8=even, H8=odd */

#define	MDIO_WIS_J1RCV(x)		/* 10G WIS J1 receive */	     \
				(47 + ((x) / 2))/* 0to15. L8=even, H8=odd */
#define	MDIO_WIS_FARENDLBIPERR1	55	/* 10G WIS far end line BIP errors 1 */
#define	MDIO_WIS_FARENDLBIPERR2	56	/* 10G WIS far end line BIP errors 2 */
#define	MDIO_WIS_LBIPERR1	57	/* 10G WIS line BIP errors 1 */
#define	MDIO_WIS_LBIPERR2	58	/* 10G WIS line BIP errors 2 */
#define	MDIO_WIS_PBERRCNT	59	/* 10G WIS path block error count */
#define	MDIO_WIS_SECBIPERRCNT	60	/* 10G WIS section BIP error count */
	/* Values 61 to 63 are reserved */
#define	MDIO_WIS_J0XMIT(x)		/* 10G WIS J0 transmit */	     \
				(64 + ((x) / 2))/* 0to15. L8=even, H8=odd */

#define	MDIO_WIS_J0RCV(x)		/* 10G WIS J0 receive */	     \
				(72 + ((x) / 2))/* 0to15. L8=even, H8=odd */
	/* Values 80 to 1799 are reserved */
#define MDIO_WIS_TSYNCCAP	1800 /* TimeSync WIS capability */
#define MDIO_WIS_TSYNCTXMAXDLYL	1801 /* TimeSync WIS TX MAXdelay L*/
#define MDIO_WIS_TSYNCTXMAXDLYH	1802 /* TimeSync WIS TX MAXdelay H*/
#define MDIO_WIS_TSYNCTXMINDLYL	1803 /* TimeSync WIS TX MINdelay L*/
#define MDIO_WIS_TSYNCTXMINDLYH	1804 /* TimeSync WIS TX MINdelay H*/
#define MDIO_WIS_TSYNCRXMAXDLYL	1805 /* TimeSync WIS RX MAXdelay L*/
#define MDIO_WIS_TSYNCRXMAXDLYH	1806 /* TimeSync WIS RX MAXdelay H*/
#define MDIO_WIS_TSYNCRXMINDLYL	1807 /* TimeSync WIS RX MINdelay L*/
#define MDIO_WIS_TSYNCRXMINDLYH	1808 /* TimeSync WIS RX MINdelay H*/
	/* Values 1809 to 32767 are reserved */
	/* Values 32768 to 65535 are vendor specific */

/*
 * PCS registers.
 * Table 45-119
 */
#define	MDIO_PCS_CTRL1		0	/* PCS control 1 */
#define	MDIO_PCS_STAT1		1	/* PCS status 1 */
#define	MDIO_PCS_DEVID1		2	/* PCS device identifier 1 */
#define	MDIO_PCS_DEVID2		3	/* PCS device identifier 2 */
#define	MDIO_PCS_SPEED		4	/* PCS speed ability */
#define	MDIO_PCS_DEVS1		5	/* PCS devices in package 1 */
#define	MDIO_PCS_DEVS2		6	/* PCS devices in package 2 */
#define	MDIO_PCS_10GCTRL2	7	/* 10G PCS control 2 */
#define	MDIO_PCS_10GSTAT2	8	/* 10G PCS status 2 */
	/* Values 9 to 13 are reserved */
#define	MDIO_PCS_PKGID1		14	/* PCS package identifier 1 */
#define	MDIO_PCS_PKGID2		15	/* PCS package identifier 2 */
	/* Values 16 to 19 are reserved */
#define	MDIO_PCS_EEECTRLCAP	20	/* EEE control and capability */
	/* Value 21 is reserved */
#define	MDIO_PCS_EEEWKERRCNT	22	/* EEE wake error counter */
	/* Value 23 is reserved */
#define	MDIO_PCS_10GXSTAT	24	/* 10G-X PCS status */
#define	MDIO_PCS_10GXSTSCTRL	25	/* 10G-X PCS test control */
	/* Values 26 to 31 are reserved */
#define	MDIO_PCS_BASERTSTAT1	32	/* BASE-R & 10G-T PCS status 1 */
#define	MDIO_PCS_BASERTSTAT2	33	/* BASE-R & 10G-T PCS status 2 */
#define	MDIO_PCS_10GRTPSEEDA(x)		/* 10G-R PCS test pattern seed A */ \
				(34 + (x)) /* 0 to 3 */
#define	MDIO_PCS_10GRTPSEEDB		/* 10G-R PCS test pattern seed B */ \
				(38 + (x)) /* 0 to 3 */
#define	MDIO_PCS_10GRTPCTRL	42	/* 10G-R PCS test pattern control */
#define	MDIO_PCS_10GRTPERRCNT	43	/* 10G-R PCS test pattern err counter*/
#define	MDIO_PCS_BERHIORDERCNT	44	/* BER high order counter */
#define	MDIO_PCS_ERRBHIORDERCNT	45	/* Errored blocks high order counter */
	/* Values 46 to 49 are reserved */
#define MDIO_PCS_MLBRPCSALGNSTAT1 50	/* Mlt-lane BASE-R PCS Align. Stat1 */
#define MDIO_PCS_MLBRPCSALGNSTAT2 51	/* Mlt-lane BASE-R PCS Align. Stat2 */
#define MDIO_PCS_MLBRPCSALGNSTAT3 52	/* Mlt-lane BASE-R PCS Align. Stat3 */
#define MDIO_PCS_MLBRPCSALGNSTAT4 53	/* Mlt-lane BASE-R PCS Align. Stat4 */
	/* Values 54 to 59 are reserved */
#define	MDIO_PCS_10P2BCAP	60	/* 10P/2B capability */
#define	MDIO_PCS_10P2BCTRL	61	/* 10P/2B PCS control */
#define	MDIO_PCS_10P2BPMEAVAIL1	62	/* 10P/2B PME available 1 */
#define	MDIO_PCS_10P2BPMEAVAIL2	63	/* 10P/2B PME available 2 */
#define	MDIO_PCS_10P2BPMEAGGRG1	64	/* 10P/2B PME aggregate 1 */
#define	MDIO_PCS_10P2BPMEAGGRG2	65	/* 10P/2B PME aggregate 2 */
#define	MDIO_PCS_10P2BPAFRXERRCNT 66	/* 10P/2B PAF RX error counter */
#define	MDIO_PCS_10P2BPAFSMLFRCNT 67	/* 10P/2B PAF small fragment counter */
#define	MDIO_PCS_10P2BPAFLARFLCNT 68	/* 10P/2B PAF large fragment counter */
#define	MDIO_PCS_10P2BPAFOVFLCNT 69    /* 10P/2B PAF overflow counter */
#define	MDIO_PCS_10P2BPAFBADFLCNT 70   /* 10P/2B PAF bad fragments counter */
#define	MDIO_PCS_10P2BPAFLSTFLCNT 71   /* 10P/2B PAF lost fragments counter */
#define	MDIO_PCS_10P2BPAFLSTSTFLCNT 72 /* 10P/2B PAF lost starts of fr. cnt */
#define	MDIO_PCS_10P2BPAFLSTENFLCNT 73 /* 10P/2B PAF lost ends of fr. count */
#define	MDIO_PCS_10GPRFECABLTY	74	/* 10G-PR & 10/1G-PRX FEC ability */
#define	MDIO_PCS_10GPRFECCTRL	75	/* 10G-PR & 10/1G-PRX FEC control */
#define	MDIO_PCS_10GPRCOFECCOCNT1 76	/*10(/1)G-PR(X) corrected FECcodecnt1*/
#define	MDIO_PCS_10GPRCOFECCOCNT2 77	/*10(/1)G-PR(X) corrected FECcodecnt2*/
#define	MDIO_PCS_10GPRUNCOFECCOCNT1 78	/*10(/1)G-PR(X)uncorrected FECcdecnt1*/
#define	MDIO_PCS_10GPRUNCOFECCOCNT2 79	/*10(/1)G-PR(X)uncorrected FECcdecnt2*/
#define	MDIO_PCS_10GPRBERMONTMRCTRL 80	/*10(/1)G-PR(X) BER monitor tmr ctrl */
#define	MDIO_PCS_10GPRBERMONSTAT 81	/*10(/1)G-PR(X) BER monitor status */
#define	MDIO_PCS_10GPRBERMONTHRCTRL 82	/*10(/1)G-PR(X) BER mntr thresh ctrl */
	/* Values 83 to 199 are reserved */
#define MDIO_PCS_BIPERRCNT(x)	    	/* BIP Error Counter */ \
				    (200 + (x)) /* lane 0 to 19 */
	/* Values 220 to 399 are reserved */
#define MDIO_PCS_PCSLMAP(x)	    	/* PCS Lane Mapping */ \
				    (400 + (x)) /* lane 0 to 19 */
	/* Values 420 to 1799 are reserved */
#define MDIO_PCS_TSYNCCAP	1800 /* TimeSync PCS capability */
#define MDIO_PCS_TSYNCTXMAXDLYL	1801 /* TimeSync PCS TX MAXdelay L*/
#define MDIO_PCS_TSYNCTXMAXDLYH	1802 /* TimeSync PCS TX MAXdelay H*/
#define MDIO_PCS_TSYNCTXMINDLYL	1803 /* TimeSync PCS TX MINdelay L*/
#define MDIO_PCS_TSYNCTXMINDLYH	1804 /* TimeSync PCS TX MINdelay H*/
#define MDIO_PCS_TSYNCRXMAXDLYL	1805 /* TimeSync PCS RX MAXdelay L*/
#define MDIO_PCS_TSYNCRXMAXDLYH	1806 /* TimeSync PCS RX MAXdelay H*/
#define MDIO_PCS_TSYNCRXMINDLYL	1807 /* TimeSync PCS RX MINdelay L*/
#define MDIO_PCS_TSYNCRXMINDLYH	1808 /* TimeSync PCS RX MINdelay H*/
	/* Values 1809 to 32767 are reserved */
	/* Values 32768 to 65535 are vendor specific */

/*
 * PHY XS registers.
 * Table 45-164
 */
#define	MDIO_PHYXS_CTRL1	0	/* PHY XS control 1 */
#define	MDIO_PHYXS_STAT1	1	/* PHY XS status 1 */
#define	MDIO_PHYXS_DEVID1	2	/* PHY XS device identifier 1 */
#define	MDIO_PHYXS_DEVID2	3	/* PHY XS device identifier 2 */
#define	MDIO_PHYXS_SPEED	4	/* PHY XS speed ability */
#define	MDIO_PHYXS_DEVS1	5	/* PHY XS devices in package 1 */
#define	MDIO_PHYXS_DEVS2	6	/* PHY XS devices in package 2 */
	/* Value 7 is reserved */
#define	MDIO_PHYXS_STAT2	8	/* PHY XS status 2 */
	/* Values 9 to 13 are reserved */
#define	MDIO_PHYXS_PKGID1	14	/* PHY XS package identifier 1 */
#define	MDIO_PHYXS_PKGID2	15	/* PHY XS package identifier 2 */
	/* Values 16 to 19 are reserved */
#define	MDIO_PHYXS_EEECAP	20	/* EEE capability register */
	/* Value 21 is reserved */
#define	MDIO_PHYXS_EEEWKERRCNT	22	/* EEE wake error counter */
	/* Value 23 is reserved */
#define	MDIO_PHYXS_10GXGXLNSTAT	24	/* 10G-X PHY XGXS lane status */
#define	MDIO_PHYXS_10GXGXSTSCTRL 25	/* 10G-X PHY XGXS test control */
	/* Values 26 to 1799 are reserved */
#define MDIO_PHYXS_TSYNCCSP	  1800 /* TimeSync PHY XS capability */
#define MDIO_PHYXS_TSYNCTXMAXDLYL 1801 /* TimeSync PHY XS TX MAX delay L */
#define MDIO_PHYXS_TSYNCTXMAXDLYH 1802 /* TimeSync PHY XS TX MAX delay H */
#define MDIO_PHYXS_TSYNCTXMINDLYL 1803 /* TimeSync PHY XS TX MIN delay L */
#define MDIO_PHYXS_TSYNCTXMINDLYH 1804 /* TimeSync PHY XS TX MIN delay H */
#define MDIO_PHYXS_TSYNCRXMAXDLYL 1805 /* TimeSync PHY XS RX MAX delay L */
#define MDIO_PHYXS_TSYNCRXMAXDLYH 1806 /* TimeSync PHY XS RX MAX delay H */
#define MDIO_PHYXS_TSYNCRXMINDLYL 1807 /* TimeSync PHY XS RX MIN delay L */
#define MDIO_PHYXS_TSYNCRXMINDLYH 1808 /* TimeSync PHY XS RX MIN delay H */
	/* Values 1809 to 32767 are reserved */
	/* Values 32768 to 65535 are vendor specific */

/*
 * DTE XS registers.
 * Table 45-175
 */
#define	MDIO_DTEXS_CTRL1	0	/* DTE XS control 1 */
#define	MDIO_DTEXS_STAT1	1	/* DTE XS status 1 */
#define	MDIO_DTEXS_DEVID1	2	/* DTE XS device identifier 1 */
#define	MDIO_DTEXS_DEVID2	3	/* DTE XS device identifier 2 */
#define	MDIO_DTEXS_SPEED	4	/* DTE XS speed ability */
#define	MDIO_DTEXS_DEVS1	5	/* DTE XS devices in package 1 */
#define	MDIO_DTEXS_DEVS2	6	/* DTE XS devices in package 2 */
	/* Value 7 is reserved */
#define	MDIO_DTEXS_STAT2	8	/* DTE XS status 2 */
	/* Values 9 to 13 are reserved */
#define	MDIO_DTEXS_PKGID1	14	/* DTE XS package identifier 1 */
#define	MDIO_DTEXS_PKGID2	15	/* DTE XS package identifier 2 */
	/* Values 16 to 19 are reserved */
#define	MDIO_DTEXS_EEECAP	20	/* EEE capability register */
	/* Value 21 is reserved */
#define	MDIO_DTEXS_EEEWKERRCNT	22	/* EEE wake error counter */
	/* Value 23 is reserved */
#define	MDIO_DTEXS_10GXGXLNSTAT	24	/* 10G DTE XGXS lane status */
#define	MDIO_DTEXS_10GXGXSTSCTRL 25	/* 10G DTE XGXS test control */
	/* Values 26 to 1799 are reserved */
#define MDIO_DTEXS_TSYNCCAP	  1800 /* TimeSync DTE XS capability */
#define MDIO_DTEXS_TSYNCTXMAXDLYL 1801 /* TimeSync DTE XS TX MAX delay L */
#define MDIO_DTEXS_TSYNCTXMAXDLYH 1802 /* TimeSync DTE XS TX MAX delay H */
#define MDIO_DTEXS_TSYNCTXMINDLYL 1803 /* TimeSync DTE XS TX MIN delay L */
#define MDIO_DTEXS_TSYNCTXMINDLYH 1804 /* TimeSync DTE XS TX MIN delay H */
#define MDIO_DTEXS_TSYNCRXMAXDLYL 1805 /* TimeSync DTE XS RX MAX delay L */
#define MDIO_DTEXS_TSYNCRXMAXDLYH 1806 /* TimeSync DTE XS RX MAX delay H */
#define MDIO_DTEXS_TSYNCRXMINDLYL 1807 /* TimeSync DTE XS RX MIN delay L */
#define MDIO_DTEXS_TSYNCRXMINDLYH 1808 /* TimeSync DTE XS RX MIN delay H */
	/* Values 1809 to 32767 are reserved */
	/* Values 32768 to 65535 are vendor specific */

/*
 * TC registers.
 * Table 45-186
 */
#define	MDIO_TC_CTRL1		0	/* TC control 1 */
	/* Value 1 is reserved */
#define	MDIO_TC_DEVID1		2	/* TC device identifier 1 */
#define	MDIO_TC_DEVID2		3	/* TC device identifier 2 */
#define	MDIO_TC_SPEED		4	/* TC speed ability */
#define	MDIO_TC_DEVS1		5	/* TC devices in package 1 */
#define	MDIO_TC_DEVS2		6	/* TC devices in package 2 */
	/* Values 7 to 13 are reserved */
#define	MDIO_TC_PKGID1		14	/* TC package identifier 1 */
#define	MDIO_TC_PKGID2		15	/* TC package identifier 2 */
#define	MDIO_TC_10P2BAGGDCCTRL	16	/* 10P/2B aggregation discovery ctrl */
#define	MDIO_TC_10P2BAGGDCSTAT	17	/* 10P/2B aggregation&discovery stat */
#define	MDIO_TC_10P2BAGGDCCODE1	18	/* 10P/2B aggregation discovery code1*/
#define	MDIO_TC_10P2BAGGDCCODE2	19	/* 10P/2B aggregation discovery code2*/
#define	MDIO_TC_10P2BAGGDCCODE3	20	/* 10P/2B aggregation discovery code3*/
#define	MDIO_TC_10P2BLPPMEAGGCTRL 21	/* 10P/2B LP PME aggregate control */
#define	MDIO_TC_10P2BLPPMEAGGDAT1 22	/* 10P/2B LP PME aggregate data 1 */
#define	MDIO_TC_10P2BLPPMEAGGDAT2 23	/* 10P/2B LP PME aggregate data 2 */
#define	MDIO_TC_10P2BCRCERRCNT	24	/* 10P/2B TC CRC error counter  */
#define	MDIO_TC_10P2BTPSCOVIOCNT1 25	/* 10P/2B TPS-TC coding viol. cnt. 1 */
#define	MDIO_TC_10P2BTPSCOVIOCNT2 26	/* 10P/2B TPS-TC coding viol. cnt. 2 */
#define	MDIO_TC_10P2BINDIC	27	/* 10P/2B TC indications */
	/* Values 28 to 1799 are reserved */
#define MDIO_TC_TSYNCCAP	1800 /* TimeSync TC capability */
#define MDIO_TC_TSYNCTXMAXDLYL	1801 /* TimeSync TC TX MAX delay L */
#define MDIO_TC_TSYNCTXMAXDLYH	1802 /* TimeSync TC TX MAX delay H */
#define MDIO_TC_TSYNCTXMINDLYL	1803 /* TimeSync TC TX MIN delay L */
#define MDIO_TC_TSYNCTXMINDLYH	1804 /* TimeSync TC TX MIN delay H */
#define MDIO_TC_TSYNCRXMAXDLYL	1805 /* TimeSync TC RX MAX delay L */
#define MDIO_TC_TSYNCRXMAXDLYH	1806 /* TimeSync TC RX MAX delay H */
#define MDIO_TC_TSYNCRXMINDLYL	1807 /* TimeSync TC RX MIN delay L */
#define MDIO_TC_TSYNCRXMINDLYH	1808 /* TimeSync TC RX MIN delay H */
	/* Values 1809 to 32767 are reserved */
	/* Values 32768 to 65535 are vendor specific */

/*
 * Auto-Negotiation registers.
 * Table 45-200
 */
#define MDIO_AN_CTRL1		0   /* AN control 1 */
#define AN_CTRL1_ANRESET	0x8000 /* AN reset */
#define AN_CTRL1_ENP		0x2000 /* Extended Next Page  */
#define AN_CTRL1_AUTOEN		0x1000 /* Auto-Negotiation enable */
#define AN_CTRL1_STARTNEG	0x0200 /* Restart Auto-Negotiation */

#define MDIO_AN_STAT1		1   /* AN status 1 */
#define MDIO_AN_DEVID1		2   /* AN device identifier 1 */
#define MDIO_AN_DEVID2		3   /* AN device identifier 2 */
	/* Value 4 is reserved */
#define MDIO_AN_DEVS1		5   /* AN devices in package 1 */
#define MDIO_AN_DEVS2		6   /* AN devices in package 2 */
	/* Values 7 to 13 are reserved */
#define MDIO_AN_PKGID1		14  /* AN package identifier 1 */
#define MDIO_AN_PKGID2		15  /* AN package identifier 2 */
#define MDIO_AN_ADVERT1		16  /* AN advertisement 1 */
#define MDIO_AN_ADVERT2		17  /* AN advertisement 2 */
#define MDIO_AN_ADVERT3		18  /* AN advertisement 3 */
#define MDIO_AN_LPBPABLTY1	19  /* AN LP base page ability 1 */
#define MDIO_AN_LPBPABLTY2	20  /* AN LP base page ability 2 */
#define MDIO_AN_LPBPABLTY3	21  /* AN LP base page ability 3 */
#define MDIO_AN_XNPXMIT1	22  /* AN XNP transmit 1 */
#define MDIO_AN_XNPXMIT2	23  /* AN XNP transmit 2 */
#define MDIO_AN_XNPXMIT3	24  /* AN XNP transmit 3 */
#define MDIO_AN_LPXNPABLTY1	25  /* AN LP XNP ability 1 */
#define MDIO_AN_LPXNPABLTY2	26  /* AN LP XNP ability 2 */
#define MDIO_AN_LPXNPABLTY3	27  /* AN LP XNP ability 3 */
	/* Values 28 to 31 are reserved */
#define MDIO_AN_10GTANCTRL	32  /* 10G-T AN control */
#define MDIO_AN_10GTANSTAT	33  /* 10G-T AN status */
	/* Values 34 to 47 are reserved */
#define MDIO_AN_BPETHSTAT	48  /* BP Ethernet status */
	/* Values 49 to 59 are reserved */
#define	MDIO_AN_EEEADVERT	60  /* EEE advertisement */
#define	MDIO_AN_EEELPABLTY	61  /* EEE LP ability */
	/* Values 62 to 32767 are reserved */
	/* Values 32768 to 65535 are vendor specific */

/*
 * Clause 22 extension registers.
 * Table 45-212
 */
	/* Values 0 to 4 are reserved */
#define MDIO_CL22E_DEVS1	5   /* Clause 22 ext. devices in package 1 */
#define MDIO_CL22E_DEVS2	6   /* Clause 22 ext. devices in package 2 */
#define MDIO_CL22E_FECCAP	7   /* FEC capability */
#define MDIO_CL22E_FECCTRL	8   /* FEC control */
#define MDIO_CL22E_FECBHCVIOCNT	9   /* FEC buffer head coding violation cnt. */
#define MDIO_CL22E_FECCOBLCNT	10  /* FEC corrected blocks counter */
#define MDIO_CL22E_FECUNCOBLCNT	11  /* FEC uncorrected blocks counter */
	/* Values 12 to 32767 are reserved */

/*
 * Vendor specific MMD 1 registers.
 * Table 45-218
 */
	/* Values 0 to 1 are vendor specific */
#define MDIO_VSMMD1_DEVID1	2   /* Vendor specific MMD 1 device ident. 1 */
#define MDIO_VSMMD1_DEVID2	3   /* Vendor specific MMD 1 device ident. 2 */
	/* Values 4 to 7 are vendor specific */
#define MDIO_VSMMD1_STAT	8   /* Vendor specific MMD 1 status register */
	/* Values 9 to 13 are vendor specific */
#define MDIO_VSMMD1_PKGID1	14  /* Vendor specific MMD 1 package ident 1 */
#define MDIO_VSMMD1_PKGID2	15  /* Vendor specific MMD 1 package ident 2 */
	/* Values 16 to 65535 are vendor specific */

/*
 * Vendor specific MMD 2 registers.
 * Table 45-220
 */
	/* Values 0 to 1 are vendor specific */
#define MDIO_VSMMD2_DEVID1	2   /* Vendor specific MMD 2 device ident. 1 */
#define MDIO_VSMMD2_DEVID2	3   /* Vendor specific MMD 2 device ident. 2 */
	/* Values 4 to 7 are vendor specific */
#define MDIO_VSMMD2_STAT	8   /* Vendor specific MMD 2 status register */
	/* Values 9 to 13 are vendor specific */
#define MDIO_VSMMD2_PKGID1	14  /* Vendor specific MMD 2 package ident 1 */
#define MDIO_VSMMD2_PKGID2	15  /* Vendor specific MMD 2 package ident 2 */
	/* Values 16 to 65535 are vendor specific */

#endif /* _DEV_MII_MDIO_H_ */
