/*	$NetBSD: if_scx.c,v 1.37 2022/06/12 16:22:37 andvar Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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


/*
 * Socionext SC2A11 SynQuacer NetSec GbE driver
 *
 * Multiple Tx and Rx queues exist inside and dedicated descriptor
 * fields specifies which queue is to use. Three internal micro-processors
 * to handle incoming frames, outgoing frames and packet data crypto
 * processing. uP programs are stored in an external flash memory and
 * have to be loaded by device driver.
 * NetSec uses Synopsys DesignWare Core EMAC.  DWC implementation
 * register (0x20) is known to have 0x10.36 and feature register (0x1058)
 * reports 0x11056f37.
 *  <24> exdesc
 *  <18> receive IP type 2 checksum offload
 *  <17> (no) receive IP type 1 checksum offload
 *  <16> transmit checksum offload
 *  <11> event counter (mac management counter, MMC) 
 */

#define NOT_MP_SAFE	0

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_scx.c,v 1.37 2022/06/12 16:22:37 andvar Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/rndsource.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <net/bpf.h>

#include <dev/fdt/fdtvar.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

/* SC2A11 GbE 64-bit paddr descriptor */
struct tdes {
	uint32_t t0, t1, t2, t3;
};

struct rdes {
	uint32_t r0, r1, r2, r3;
};

#define T0_OWN		(1U<<31)	/* desc is ready to Tx */
#define T0_EOD		(1U<<30)	/* end of descriptor array */
#define T0_DRID		(24)		/* 29:24 desc ring id */
#define T0_PT		(1U<<21)	/* 23:21 "pass-through" */
#define T0_TDRID	(16)		/* 20:16 target desc ring id: GMAC=15 */
#define T0_FS		(1U<<9)		/* first segment of frame */
#define T0_LS		(1U<<8)		/* last segment of frame */
#define T0_CSUM		(1U<<7)		/* enable check sum offload */
#define T0_TSO		(1U<<6)		/* enable TCP segment offload */
#define T0_TRS		(1U<<4)		/* 5:4 "TRS" */
/* T1 frame segment address 63:32 */
/* T2 frame segment address 31:0 */
/* T3 31:16 TCP segment length, 15:0 frame segment length to transmit */

#define R0_OWN		(1U<<31)	/* desc is empty */
#define R0_EOD		(1U<<30)	/* end of descriptor array */
#define R0_SDRID	(24)		/* 29:24 source desc ring id */
#define R0_FR		(1U<<23)	/* found fragmented */
#define R0_ER		(1U<<21)	/* Rx error indication */
#define R0_ERR		(3U<<16)	/* 18:16 receive error code */
#define R0_TDRID	(12)		/* 15:12 target desc ring id */
#define R0_FS		(1U<<9)		/* first segment of frame */
#define R0_LS		(1U<<8)		/* last segment of frame */
#define R0_CSUM		(3U<<6)		/* 7:6 checksum status */
#define R0_CERR		(2U<<6)		/* 0: undone, 1: found ok, 2: bad */
/* R1 frame address 63:32 */
/* R2 frame address 31:0 */
/* R3 31:16 received frame length, 15:0 buffer length to receive */

/*
 * SC2A11 registers. 0x100 - 1204
 */
#define SWRESET		0x104
#define  SRST_RUN	(1U<<31)	/* instruct start, 0 to stop */
#define COMINIT		0x120
#define  INIT_DB	(1U<<2)		/* ???; self clear when done */
#define  INIT_CLS	(1U<<1)		/* ???; self clear when done */
#define PKTCTRL		0x140		/* pkt engine control */
#define  MODENRM	(1U<<28)	/* change mode to normal */
#define  ENJUMBO	(1U<<27)	/* allow jumbo frame */
#define  RPTCSUMERR	(1U<<3)		/* log Rx checksum error */
#define  RPTHDCOMP	(1U<<2)		/* log HD incomplete condition */
#define  RPTHDERR	(1U<<1)		/* log HD error */
#define  DROPNOMATCH	(1U<<0)		/* drop no match frames */
#define xINTSR		0x200		/* aggregated interrupt status */
#define  IRQ_RX		(1U<<1)		/* top level Rx interrupt */
#define  IRQ_TX		(1U<<0)		/* top level Rx interrupt */
#define  IRQ_UCODE	(1U<<20)	/* ucode load completed; W1C */
#define xINTAEN		0x204		/* INT_A enable */
#define xINTAE_SET	0x234		/* bit to set */
#define xINTAE_CLR	0x238		/* bit to clr */
#define xINTBEN		0x23c		/* INT_B enable */
#define xINTBE_SET	0x240		/* bit to set */
#define xINTBE_CLR	0x244		/* bit to clr */
#define TXISR		0x400		/* transmit status; W1C */
#define TXIEN		0x404		/* tx interrupt enable */
#define TXIE_SET	0x428		/* bit to set */
#define TXIE_CLR	0x42c		/* bit to clr */
#define  TXI_NTOWNR	(1U<<17)	/* ??? desc array got empty */
#define  TXI_TR_ERR	(1U<<16)	/* tx error */
#define  TXI_TXDONE	(1U<<15)	/* tx completed */
#define  TXI_TMREXP	(1U<<14)	/* coalesce timer expired */
#define RXISR		0x440		/* receive status; W1C */
#define RXIEN		0x444		/* rx interrupt enable */
#define RXIE_SET	0x468		/* bit to set */
#define RXIE_CLR	0x46c		/* bit to clr */
#define  RXI_RC_ERR	(1U<<16)	/* rx error */
#define  RXI_PKTCNT	(1U<<15)	/* rx counter has new value */
#define  RXI_TMREXP	(1U<<14)	/* coalesce timer expired */
/* 13 sets of special purpose desc interrupt handling register exist */
#define TDBA_LO		0x408		/* tdes array base addr 31:0 */
#define TDBA_HI		0x434		/* tdes array base addr 63:32 */
#define RDBA_LO		0x448		/* rdes array base addr 31:0 */
#define RDBA_HI		0x474		/* rdes array base addr 63:32 */
/* 13 pairs of special purpose desc array base address register exist */
#define TXCONF		0x430
#define RXCONF		0x470
#define  DESCNF_UP	(1U<<31)	/* up-and-running */
#define  DESCNF_CHRST	(1U<<30)	/* channel reset */
#define  DESCNF_TMR	(1U<<4)		/* coalesce timer mode select */
#define  DESCNF_LE	(1)		/* little endian desc format */
#define TXSUBMIT	0x410		/* submit frame(s) to transmit */
#define TXCLSCMAX	0x418		/* tx intr coalesce upper bound */
#define RXCLSCMAX	0x458		/* rx intr coalesce upper bound */
#define TXITIMER	0x420		/* coalesce timer usec, MSB to use */
#define RXITIMER	0x460		/* coalesce timer usec, MSB to use */
#define TXDONECNT	0x414		/* tx completed count, auto-zero */
#define RXDONECNT	0x454		/* rx available count, auto-zero */
#define UCODE_H2M	0x210		/* host2media engine ucode port */
#define UCODE_M2H	0x21c		/* media2host engine ucode port */
#define CORESTAT	0x218		/* engine run state */
#define  PKTSTOP	(1U<<2)
#define  M2HSTOP	(1U<<1)
#define  H2MSTOP	(1U<<0)
#define DMACTL_H2M	0x214		/* host2media engine control */
#define DMACTL_M2H	0x220		/* media2host engine control */
#define  DMACTL_STOP	(1U<<0)		/* instruct stop; self-clear */
#define UCODE_PKT	0x0d0		/* packet engine ucode port */
#define CLKEN		0x100		/* clock distribution enable */
#define  CLK_G		(1U<<5)		/* feed clk domain E */
#define  CLK_C		(1U<<1)		/* feed clk domain C */
#define  CLK_D		(1U<<0)		/* feed clk domain D */
#define  CLK_ALL	0x23		/* all above; 0x24 ??? 0x3f ??? */

/* GMAC register indirect access. thru MACCMD/MACDATA operation */
#define MACDATA		0x11c0		/* gmac register rd/wr data */
#define MACCMD		0x11c4		/* gmac register operation */
#define  CMD_IOWR	(1U<<28)	/* write op */
#define  CMD_BUSY	(1U<<31)	/* busy bit */
#define MACSTAT		0x1024		/* gmac status; ??? */
#define MACINTE		0x1028		/* interrupt enable; ??? */

#define FLOWTHR		0x11cc		/* flow control threshold */
/* 31:16 pause threshold, 15:0 resume threshold */
#define INTF_SEL	0x11d4		/* ??? */

#define DESC_INIT	0x11fc		/* write 1 for desc init, SC */
#define DESC_SRST	0x1204		/* write 1 for desc sw reset, SC */
#define MODE_TRANS	0x500		/* mode change completion status */
#define  N2T_DONE	(1U<<20)	/* normal->taiki change completed */
#define  T2N_DONE	(1U<<19)	/* taiki->normal change completed */
#define MACADRH		0x10c		/* ??? */
#define MACADRL		0x110		/* ??? */
#define MCVER		0x22c		/* micro controller version */
#define HWVER		0x230		/* hardware version */

/*
 * GMAC registers are mostly identical to Synopsys DesignWare Core
 * Ethernet. These must be handled by indirect access.
 */
#define GMACMCR		0x0000		/* MAC configuration */
#define  MCR_IBN	(1U<<30)	/* ??? */
#define  MCR_CST	(1U<<25)	/* strip CRC */
#define  MCR_TC		(1U<<24)	/* keep RGMII PHY notified */
#define  MCR_WD		(1U<<23)	/* allow long >2048 tx frame */
#define  MCR_JE		(1U<<20)	/* allow ~9018 tx jumbo frame */
#define  MCR_IFG	(7U<<17)	/* 19:17 IFG value 0~7 */
#define  MCR_DRCS	(1U<<16)	/* ignore (G)MII HDX Tx error */
#define  MCR_USEMII	(1U<<15)	/* 1: RMII/MII, 0: RGMII (_PS) */
#define  MCR_SPD100	(1U<<14)	/* force speed 100 (_FES) */
#define  MCR_DO		(1U<<13)	/* don't receive my own HDX Tx frames */
#define  MCR_LOOP	(1U<<12)	/* run loop back */
#define  MCR_USEFDX	(1U<<11)	/* force full duplex */
#define  MCR_IPCEN	(1U<<10)	/* handle checksum */
#define  MCR_DR		(1U<<9)		/* attempt no tx retry, send once */
#define  MCR_LUD	(1U<<8)		/* link condition report when RGMII */
#define  MCR_ACS	(1U<<7)		/* auto pad strip CRC */
#define  MCR_TE		(1U<<3)		/* run Tx MAC engine, 0 to stop */
#define  MCR_RE		(1U<<2)		/* run Rx MAC engine, 0 to stop */
#define  MCR_PREA	(3U)		/* 1:0 preamble len. 0~2 */
#define  _MCR_FDX	0x0000280c	/* XXX TBD */
#define  _MCR_HDX	0x0001a00c	/* XXX TBD */
#define GMACAFR		0x0004		/* frame DA/SA address filter */
#define  AFR_RA		(1U<<31)	/* accept all irrespective of filt. */
#define  AFR_HPF	(1U<<10)	/* hash+perfect filter, or hash only */
#define  AFR_SAF	(1U<<9)		/* source address filter */
#define  AFR_SAIF	(1U<<8)		/* SA inverse filtering */
#define  AFR_PCF	(2U<<6)		/* ??? */
#define  AFR_DBF	(1U<<5)		/* reject broadcast frame */
#define  AFR_PM		(1U<<4)		/* accept all multicast frame */
#define  AFR_DAIF	(1U<<3)		/* DA inverse filtering */
#define  AFR_MHTE	(1U<<2)		/* use multicast hash table */
#define  AFR_UHTE	(1U<<1)		/* use hash table for unicast */
#define  AFR_PR		(1U<<0)		/* run promisc mode */
#define GMACGAR		0x0010		/* MDIO operation */
#define  GAR_PHY	(11)		/* 15:11 mii phy */
#define  GAR_REG	(6)		/* 10:6 mii reg */
#define  GAR_CLK	(2)		/* 5:2 mdio clock tick ratio */
#define  GAR_IOWR	(1U<<1)		/* MDIO write op */
#define  GAR_BUSY	(1U<<0)		/* busy bit */
#define  GAR_MDIO_25_35MHZ	2
#define  GAR_MDIO_35_60MHZ	3
#define  GAR_MDIO_60_100MHZ	0
#define  GAR_MDIO_100_150MHZ	1
#define  GAR_MDIO_150_250MHZ	4
#define  GAR_MDIO_250_300MHZ	5
#define GMACGDR		0x0014		/* MDIO rd/wr data */
#define GMACFCR		0x0018		/* 802.3x flowcontrol */
/* 31:16 pause timer value, 5:4 pause timer threshold */
#define  FCR_RFE	(1U<<2)		/* accept PAUSE to throttle Tx */
#define  FCR_TFE	(1U<<1)		/* generate PAUSE to moderate Rx lvl */
#define GMACIMPL	0x0020		/* implementation id XX.YY (no use) */
#define GMACISR		0x0038		/* interrupt status indication */
#define GMACIMR		0x003c		/* interrupt mask to inhibit */
#define  ISR_TS		(1U<<9)		/* time stamp operation detected */
#define  ISR_CO		(1U<<7)		/* Rx checksum offload completed */
#define  ISR_TX		(1U<<6)		/* Tx completed */
#define  ISR_RX		(1U<<5)		/* Rx completed */
#define  ISR_ANY	(1U<<4)		/* any of above 5-7 report */
#define  ISR_LC		(1U<<0)		/* link status change detected */
#define GMACMAH0	0x0040		/* my own MAC address 47:32 */
#define GMACMAL0	0x0044		/* my own MAC address 31:0 */
#define GMACMAH(i) 	((i)*8+0x40)	/* supplemental MAC addr 1-15 */
#define GMACMAL(i) 	((i)*8+0x44)	/* 31:0 MAC address low part */
/* MAH bit-31: slot in use, 30: SA to match, 29:24 byte-wise don'care */
#define GMACAMAH(i)	((i)*8+0x800)	/* supplemental MAC addr 16-31 */
#define GMACAMAL(i)	((i)*8+0x804)	/* 31: MAC address low part */
/* supplimental MAH bit-31: slot in use, no other bit is effective */
#define GMACMHTH	0x0008		/* 64bit multicast hash table 63:32 */
#define GMACMHTL	0x000c		/* 64bit multicast hash table 31:0 */
#define GMACMHT(i)	((i)*4+0x500)	/* 256-bit alternative mcast hash 0-7 */
#define EMACVTAG	0x001c		/* VLAN tag control */
#define  VTAG_HASH	(1U<<19)	/* use VLAN tag hash table */
#define  VTAG_SVLAN	(1U<<18)	/* handle type 0x88A8 SVLAN frame */
#define  VTAG_INV	(1U<<17)	/* run inverse match logic */
#define  VTAG_ETV	(1U<<16)	/* use only 12bit VID field to match */
/* 15:0 concat of PRIO+CFI+VID */
#define GMACVHT		0x0588		/* 16-bit VLAN tag hash */
#define GMACMIISR	0x00d8		/* resolved xMII link status */
#define  MIISR_LUP	(1U<<3)		/* link up(1)/down(0) report */
#define  MIISR_SPD	(3U<<1)		/* 2:1 speed 10(0)/100(1)/1000(2) */
#define  MIISR_FDX	(1U<<0)		/* fdx detected */

#define GMACLPIS	0x0030		/* LPI control & status */
#define  LPIS_TXA	(1U<<19)	/* complete Tx in progress and LPI */
#define  LPIS_PLS	(1U<<17)
#define  LPIS_EN	(1U<<16)	/* 1: enter LPI mode, 0: exit */
#define  LPIS_TEN	(1U<<0)		/* Tx LPI report */
#define GMACLPIC	0x0034		/* LPI timer control */
#define  LPIC_LST	(5)		/* 16:5 ??? */
#define  LPIC_TWT	(0)		/* 15:0 ??? */
#define GMACTSC		0x0700		/* timestamp control */
#define GMACSTM		0x071c		/* start time */
#define GMACTGT		0x0720		/* target time */
#define GMACTSS		0x0728		/* timestamp status */
#define GMACPPS		0x072c		/* PPS control */
#define GMACPPS0	0x0764		/* PPS0 width */

#define GMACBMR		0x1000		/* DMA bus mode control */
/* 24    multiply by x8 for RPBL & PBL values
 * 23    use RPBL for Rx DMA
 * 22:17 RPBL
 * 16    fixed burst
 * 15:14 priority between Rx and Tx
 *  3    rxtx ratio 41
 *  2    rxtx ratio 31
 *  1    rxtx ratio 21
 *  0    rxtx ratio 11
 * 13:8  PBL possible DMA burst length
 *  7    select alternative 32-byte descriptor format for new features
 *  6:2  descriptor spacing. 0 for adjuscent
 *  0    GMAC reset op. self-clear
 */
#define  _BMR		0x00412080	/* XXX TBD */
#define  _BMR0		0x00020181	/* XXX TBD */
#define  BMR_RST	(1)		/* reset op. self clear when done */
#define GMACTPD		0x1004		/* write any to resume tdes */
#define GMACRPD		0x1008		/* write any to resume rdes */
#define GMACRDLA	0x100c		/* rdes base address 32bit paddr */
#define GMACTDLA	0x1010		/* tdes base address 32bit paddr */
#define  _RDLA		0x18000		/* system RAM for GMAC rdes */
#define  _TDLA		0x1c000		/* system RAM for GMAC tdes */
#define GMACDSR		0x1014		/* DMA status detail report; W1C */
#define GMACDIE		0x101c		/* DMA interrupt enable */
#define  DMAI_LPI	(1U<<30)	/* LPI interrupt */
#define  DMAI_TTI	(1U<<29)	/* timestamp trigger interrupt */
#define  DMAI_GMI	(1U<<27)	/* management counter interrupt */
#define  DMAI_GLI	(1U<<26)	/* xMII link change detected */
#define  DMAI_EB	(23)		/* 25:23 DMA bus error detected */
#define  DMAI_TS	(20)		/* 22:20 Tx DMA state report */
#define  DMAI_RS	(17)		/* 29:17 Rx DMA state report */
#define  DMAI_NIS	(1U<<16)	/* normal interrupt summary; W1C */
#define  DMAI_AIS	(1U<<15)	/* abnormal interrupt summary; W1C */
#define  DMAI_ERI	(1U<<14)	/* the first Rx buffer is filled */
#define  DMAI_FBI	(1U<<13)	/* DMA bus error detected */
#define  DMAI_ETI	(1U<<10)	/* single frame Tx completed */
#define  DMAI_RWT	(1U<<9)		/* longer than 2048 frame received */
#define  DMAI_RPS	(1U<<8)		/* Rx process is now stopped */
#define  DMAI_RU	(1U<<7)		/* Rx descriptor not available */
#define  DMAI_RI	(1U<<6)		/* frame Rx completed by !R1_DIC */
#define  DMAI_UNF	(1U<<5)		/* Tx underflow detected */
#define  DMAI_OVF	(1U<<4)		/* receive buffer overflow detected */
#define  DMAI_TJT	(1U<<3)		/* longer than 2048 frame sent */
#define  DMAI_TU	(1U<<2)		/* Tx descriptor not available */
#define  DMAI_TPS	(1U<<1)		/* transmission is stopped */
#define  DMAI_TI	(1U<<0)		/* frame Tx completed by T0_IC */
#define GMACOMR		0x1018		/* DMA operation mode */
#define  OMR_RSF	(1U<<25)	/* 1: Rx store&forward, 0: immed. */
#define  OMR_TSF	(1U<<21)	/* 1: Tx store&forward, 0: immed. */
#define  OMR_TTC	(14)		/* 16:14 Tx threshold */
#define  OMR_ST		(1U<<13)	/* run Tx DMA engine, 0 to stop */
#define  OMR_RFD	(11)		/* 12:11 Rx FIFO fill level */
#define  OMR_EFC	(1U<<8)		/* transmit PAUSE to throttle Rx lvl. */
#define  OMR_FEF	(1U<<7)		/* allow to receive error frames */
#define  OMR_SR		(1U<<1)		/* run Rx DMA engine, 0 to stop */
#define GMACEVCS	0x1020		/* missed frame or ovf detected */
#define GMACRWDT	0x1024		/* enable rx watchdog timer interrupt */
#define GMACAXIB	0x1028		/* AXI bus mode control */
#define GMACAXIS	0x102c		/* AXI status report */
/* 0x1048 current tx desc address */
/* 0x104c current rx desc address */
/* 0x1050 current tx buffer address */
/* 0x1054 current rx buffer address */
#define HWFEA		0x1058		/* DWC feature report */
#define  FEA_EXDESC	(1U<<24)	/* new desc layout */
#define  FEA_2COE	(1U<<18)	/* Rx type 2 IP checksum offload */
#define  FEA_1COE	(1U<<17)	/* Rx type 1 IP checksum offload */
#define  FEA_TXOE	(1U<<16)	/* Tx checksum offload */
#define  FEA_MMC	(1U<<11)	/* RMON event counter */

#define GMACEVCTL	0x0100		/* event counter control */
#define  EVC_FHP	(1U<<5)		/* full-half preset */
#define  EVC_CP		(1U<<4)		/* counter preset */
#define  EVC_MCF	(1U<<3)		/* counter freeze */
#define  EVC_ROR	(1U<<2)		/* auto-zero on counter read */
#define  EVC_CSR	(1U<<1)		/* counter stop rollover */
#define  EVC_CR		(1U<<0)		/* reset counters */
#define GMACEVCNT(i)	((i)*4+0x114)	/* 80 event counters 0x114 - 0x284 */

/*
 * flash memory layout
 * 0x00 - 07	48-bit MAC station address. 4 byte wise in BE order.
 * 0x08 - 0b	H->MAC xfer engine program start addr 63:32.
 * 0x0c - 0f	H2M program addr 31:0 (these are absolute addr, not offset)
 * 0x10 - 13	H2M program length in 4 byte count.
 * 0x14 - 0b	M->HOST xfer engine program start addr 63:32.
 * 0x18 - 0f	M2H program addr 31:0 (absolute addr, not relative)
 * 0x1c - 13	M2H program length in 4 byte count.
 * 0x20 - 23	packet engine program addr 31:0, (absolute addr, not offset)
 * 0x24 - 27	packet program length in 4 byte count.
 *
 * above ucode are loaded via mapped reg 0x210, 0x21c and 0x0c0.
 */

/*
 * all below are software constraction.
 */
#define MD_NTXSEGS		16		/* fixed */
#define MD_TXQUEUELEN		8		/* tunable */
#define MD_TXQUEUELEN_MASK	(MD_TXQUEUELEN - 1)
#define MD_TXQUEUE_GC		(MD_TXQUEUELEN / 4)
#define MD_NTXDESC		128
#define MD_NTXDESC_MASK	(MD_NTXDESC - 1)
#define MD_NEXTTX(x)		(((x) + 1) & MD_NTXDESC_MASK)
#define MD_NEXTTXS(x)		(((x) + 1) & MD_TXQUEUELEN_MASK)

#define MD_NRXDESC		64		/* tunable */
#define MD_NRXDESC_MASK	(MD_NRXDESC - 1)
#define MD_NEXTRX(x)		(((x) + 1) & MD_NRXDESC_MASK)

struct control_data {
	struct tdes cd_txdescs[MD_NTXDESC];
	struct rdes cd_rxdescs[MD_NRXDESC];
};
#define SCX_CDOFF(x)		offsetof(struct control_data, x)
#define SCX_CDTXOFF(x)		SCX_CDOFF(cd_txdescs[(x)])
#define SCX_CDRXOFF(x)		SCX_CDOFF(cd_rxdescs[(x)])

struct scx_txsoft {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	int txs_firstdesc;		/* first descriptor in packet */
	int txs_lastdesc;		/* last descriptor in packet */
	int txs_ndesc;			/* # of descriptors used */
};

struct scx_rxsoft {
	struct mbuf *rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

struct scx_softc {
	device_t sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_size_t sc_sz;		/* csr map size */
	bus_space_handle_t sc_eesh;	/* eeprom section handle */
	bus_size_t sc_eesz;		/* eeprom map size */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	struct ethercom sc_ethercom;	/* Ethernet common data */
	struct mii_data sc_mii;		/* MII */
	callout_t sc_callout;		/* PHY monitor callout */
	bus_dma_segment_t sc_seg;	/* descriptor store seg */
	int sc_nseg;			/* descriptor store nseg */
	void *sc_ih;			/* interrupt cookie */
	int sc_phy_id;			/* PHY address */
	int sc_flowflags;		/* 802.3x PAUSE flow control */
	uint32_t sc_mdclk;		/* GAR 5:2 clock selection */
	uint32_t sc_t0cotso;		/* T0_CSUM | T0_TSO to run */
	int sc_100mii;			/* 1 for RMII/MII, 0 for RGMII */
	int sc_phandle;			/* fdt phandle */
	uint64_t sc_freq;

	bus_dmamap_t sc_cddmamap;	/* control data DMA map */
#define sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	struct control_data *sc_control_data;
#define sc_txdescs	sc_control_data->cd_txdescs
#define sc_rxdescs	sc_control_data->cd_rxdescs

	struct scx_txsoft sc_txsoft[MD_TXQUEUELEN];
	struct scx_rxsoft sc_rxsoft[MD_NRXDESC];
	int sc_txfree;			/* number of free Tx descriptors */
	int sc_txnext;			/* next ready Tx descriptor */
	int sc_txsfree;			/* number of free Tx jobs */
	int sc_txsnext;			/* next ready Tx job */
	int sc_txsdirty;		/* dirty Tx jobs */
	int sc_rxptr;			/* next ready Rx descriptor/descsoft */

	krndsource_t rnd_source;	/* random source */
#ifdef GMAC_EVENT_COUNTERS
	/* 80 event counters exist */
#endif
};

#define SCX_CDTXADDR(sc, x)	((sc)->sc_cddma + SCX_CDTXOFF((x)))
#define SCX_CDRXADDR(sc, x)	((sc)->sc_cddma + SCX_CDRXOFF((x)))

#define SCX_CDTXSYNC(sc, x, n, ops)					\
do {									\
	int __x, __n;							\
									\
	__x = (x);							\
	__n = (n);							\
									\
	/* If it will wrap around, sync to the end of the ring. */	\
	if ((__x + __n) > MD_NTXDESC) {				\
		bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,	\
		    SCX_CDTXOFF(__x), sizeof(struct tdes) *		\
		    (MD_NTXDESC - __x), (ops));			\
		__n -= (MD_NTXDESC - __x);				\
		__x = 0;						\
	}								\
									\
	/* Now sync whatever is left. */				\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    SCX_CDTXOFF(__x), sizeof(struct tdes) * __n, (ops));	\
} while (/*CONSTCOND*/0)

#define SCX_CDRXSYNC(sc, x, ops)					\
do {									\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    SCX_CDRXOFF((x)), sizeof(struct rdes), (ops));		\
} while (/*CONSTCOND*/0)

#define SCX_INIT_RXDESC(sc, x)						\
do {									\
	struct scx_rxsoft *__rxs = &(sc)->sc_rxsoft[(x)];		\
	struct rdes *__rxd = &(sc)->sc_rxdescs[(x)];			\
	struct mbuf *__m = __rxs->rxs_mbuf;				\
	bus_addr_t __paddr =__rxs->rxs_dmamap->dm_segs[0].ds_addr;	\
	__m->m_data = __m->m_ext.ext_buf;				\
	__rxd->r3 = htole32(__rxs->rxs_dmamap->dm_segs[0].ds_len);	\
	__rxd->r2 = htole32(BUS_ADDR_LO32(__paddr));			\
	__rxd->r1 = htole32(BUS_ADDR_HI32(__paddr));			\
	__rxd->r0 = htole32(R0_OWN | R0_FS | R0_LS);			\
	if ((x) == MD_NRXDESC - 1) __rxd->r0 |= htole32(R0_EOD);	\
} while (/*CONSTCOND*/0)

/* memory mapped CSR register access */
#define CSR_READ(sc,off) \
	    bus_space_read_4((sc)->sc_st, (sc)->sc_sh, (off))
#define CSR_WRITE(sc,off,val) \
	    bus_space_write_4((sc)->sc_st, (sc)->sc_sh, (off), (val))

/* flash memory access */
#define EE_READ(sc,off) \
	    bus_space_read_4((sc)->sc_st, (sc)->sc_eesh, (off))

static int scx_fdt_match(device_t, cfdata_t, void *);
static void scx_fdt_attach(device_t, device_t, void *);
static int scx_acpi_match(device_t, cfdata_t, void *);
static void scx_acpi_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(scx_fdt, sizeof(struct scx_softc),
    scx_fdt_match, scx_fdt_attach, NULL, NULL);

CFATTACH_DECL_NEW(scx_acpi, sizeof(struct scx_softc),
    scx_acpi_match, scx_acpi_attach, NULL, NULL);

static void scx_attach_i(struct scx_softc *);
static void scx_reset(struct scx_softc *);
static int scx_init(struct ifnet *);
static void scx_stop(struct ifnet *, int);
static int scx_ioctl(struct ifnet *, u_long, void *);
static void scx_set_rcvfilt(struct scx_softc *);
static void scx_start(struct ifnet *);
static void scx_watchdog(struct ifnet *);
static int scx_intr(void *);
static void txreap(struct scx_softc *);
static void rxintr(struct scx_softc *);
static int add_rxbuf(struct scx_softc *, int);
static void rxdrain(struct scx_softc *sc);
static void mii_statchg(struct ifnet *);
static void scx_ifmedia_sts(struct ifnet *, struct ifmediareq *);
static int mii_readreg(device_t, int, int, uint16_t *);
static int mii_writereg(device_t, int, int, uint16_t);
static void phy_tick(void *);

static void loaducode(struct scx_softc *);
static void injectucode(struct scx_softc *, int, bus_addr_t, bus_size_t);

static int get_mdioclk(uint32_t);

#define WAIT_FOR_SET(sc, reg, set, fail) \
	wait_for_bits(sc, reg, set, ~0, fail)
#define WAIT_FOR_CLR(sc, reg, clr, fail) \
	wait_for_bits(sc, reg, 0, clr, fail)

static int
wait_for_bits(struct scx_softc *sc, int reg,
    uint32_t set, uint32_t clr, uint32_t fail)
{
	uint32_t val;
	int ntries;

	for (ntries = 0; ntries < 1000; ntries++) {
		val = CSR_READ(sc, reg);
		if ((val & set) || !(val & clr))
			return 0;
		if (val & fail)
			return 1;
		DELAY(1);
	}
	return 1;
}

/* GMAC register indirect access */
static int
mac_read(struct scx_softc *sc, int reg)
{

	CSR_WRITE(sc, MACCMD, reg | CMD_BUSY);
	(void)WAIT_FOR_CLR(sc, MACCMD, CMD_BUSY, 0);
	return CSR_READ(sc, MACDATA);
}

static void
mac_write(struct scx_softc *sc, int reg, int val)
{

	CSR_WRITE(sc, MACDATA, val);
	CSR_WRITE(sc, MACCMD, reg | CMD_IOWR | CMD_BUSY);
	(void)WAIT_FOR_CLR(sc, MACCMD, CMD_BUSY, 0);
}

/* dig and decode "clock-frequency" value for a given clkname */
static int
get_clk_freq(int phandle, const char *clkname)
{
	u_int index, n, cells;
	const u_int *p;
	int err, len, resid;
	unsigned int freq = 0;

	err = fdtbus_get_index(phandle, "clock-names", clkname, &index);
	if (err == -1)
		return -1;
	p = fdtbus_get_prop(phandle, "clocks", &len);
	if (p == NULL)
		return -1;
	for (n = 0, resid = len; resid > 0; n++) {
		const int cc_phandle =
		    fdtbus_get_phandle_from_native(be32toh(p[0]));
		if (of_getprop_uint32(cc_phandle, "#clock-cells", &cells))
			return -1;
		if (n == index) {
			if (of_getprop_uint32(cc_phandle,
			    "clock-frequency", &freq))
				return -1;
			return freq;
		}
		resid -= (cells + 1) * 4;
		p += (cells + 1) * 4;
	}
	return -1;
}

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "socionext,synquacer-netsec" },
	DEVICE_COMPAT_EOL
};
static const struct device_compatible_entry compatible[] = {
	{ .compat = "SCX0001" },
	DEVICE_COMPAT_EOL
};

static int
scx_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
scx_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct scx_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_handle_t bsh;
	bus_space_handle_t eebsh;
	bus_addr_t addr[2];
	bus_size_t size[2];
	char intrstr[128];
	int phy_phandle;
	bus_addr_t phy_id;
	const char *phy_type;
	long ref_clk;

	if (fdtbus_get_reg(phandle, 0, addr+0, size+0) != 0
	    || bus_space_map(faa->faa_bst, addr[0], size[0], 0, &bsh) != 0) {
		aprint_error_dev(self, "unable to map device csr\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error_dev(self, "failed to decode interrupt\n");
		goto fail;
	}
	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_NET,
		NOT_MP_SAFE, scx_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto fail;
	}
	if (fdtbus_get_reg(phandle, 1, addr+1, size+1) != 0
	    || bus_space_map(faa->faa_bst, addr[1], size[1], 0, &eebsh) != 0) {
		aprint_error_dev(self, "unable to map device eeprom\n");
		goto fail;
	}

	sc->sc_dev = self;
	sc->sc_st = faa->faa_bst;
	sc->sc_sh = bsh;
	sc->sc_sz = size[0];
	sc->sc_eesh = eebsh;
	sc->sc_eesz = size[1];
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_phandle = phandle;

	phy_type = fdtbus_get_string(phandle, "phy-mode");
	if (phy_type == NULL)
		aprint_error_dev(self, "missing 'phy-mode' property\n");
	phy_phandle = fdtbus_get_phandle(phandle, "phy-handle");	
	if (phy_phandle == -1
	    || fdtbus_get_reg(phy_phandle, 0, &phy_id, NULL) != 0)
		phy_id = MII_PHY_ANY;
	ref_clk = get_clk_freq(phandle, "phy_ref_clk");
	if (ref_clk == -1)
		ref_clk = 250 * 1000 * 1000;

	sc->sc_100mii = (phy_type && strncmp(phy_type, "rgmii", 5) != 0);
	sc->sc_phy_id = phy_id;
	sc->sc_freq = ref_clk;

	aprint_normal("%s", device_xname(self));
	scx_attach_i(sc);
	return;
 fail:
	if (sc->sc_eesz)
		bus_space_unmap(sc->sc_st, sc->sc_eesh, sc->sc_eesz);
	if (sc->sc_sz)
		bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_sz);
	return;
}

static int
scx_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args *aa = aux;

	return acpi_compatible_match(aa, compatible);
}

static void
scx_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct scx_softc * const sc = device_private(self);
	struct acpi_attach_args * const aa = aux;
	ACPI_HANDLE handle = aa->aa_node->ad_handle;
	bus_space_handle_t bsh, eebsh;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_INTEGER phy_type, phy_id, ref_freq;
	ACPI_STATUS rv;

	rv = acpi_resource_parse(self, handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;

	mem = acpi_res_mem(&res, 0);
	irq = acpi_res_irq(&res, 0);
	if (mem == NULL || irq == NULL || mem->ar_length == 0) {
		aprint_error_dev(self, "incomplete crs resources\n");
		return;
	}
	if (bus_space_map(aa->aa_memt, mem->ar_base, mem->ar_length, 0,
	    &bsh) != 0) {
		aprint_error_dev(self, "couldn't map registers\n");
		return;
	}
	sc->sc_sz = mem->ar_length;
	sc->sc_ih = acpi_intr_establish(self, (uint64_t)(uintptr_t)handle,
	    IPL_NET, NOT_MP_SAFE, scx_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto fail;
	}
	mem = acpi_res_mem(&res, 1); /* EEPROM for MAC address and ucode */
	if (mem == NULL || mem->ar_length == 0) {
		aprint_error_dev(self, "incomplete eeprom resources\n");
		goto fail;
	}
	if (bus_space_map(aa->aa_memt, mem->ar_base, mem->ar_length, 0,
	    &eebsh)) {
		aprint_error_dev(self, "couldn't map registers\n");
		goto fail;
	}
	sc->sc_eesz = mem->ar_length;

	rv = acpi_dsd_integer(handle, "max-speed", &phy_type);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "missing 'max-speed' property\n");
		phy_type = 1000;
	}
	rv = acpi_dsd_integer(handle, "phy-channel", &phy_id);
	if (ACPI_FAILURE(rv))
		phy_id = MII_PHY_ANY;
	rv = acpi_dsd_integer(handle, "socionext,phy-clock-frequency",
			&ref_freq);
	if (ACPI_FAILURE(rv))
		ref_freq = 250 * 1000 * 1000;

	sc->sc_dev = self;
	sc->sc_st = aa->aa_memt;
	sc->sc_sh = bsh;
	sc->sc_eesh = eebsh;
	sc->sc_dmat = aa->aa_dmat64;
	sc->sc_100mii = (phy_type != 1000);
	sc->sc_phy_id = (int)phy_id;
	sc->sc_freq = ref_freq;

aprint_normal_dev(self,
"phy type %d, phy id %d, freq %ld\n", (int)phy_type, (int)phy_id, ref_freq);

	aprint_normal("%s", device_xname(self));
	scx_attach_i(sc);

	acpi_resource_cleanup(&res);
	return;
 fail:
	if (sc->sc_eesz > 0)
		bus_space_unmap(sc->sc_st, sc->sc_eesh, sc->sc_eesz);
	if (sc->sc_sz > 0)
		bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_sz);
	acpi_resource_cleanup(&res);
	return;
}

static void
scx_attach_i(struct scx_softc *sc)
{
	struct ifnet * const ifp = &sc->sc_ethercom.ec_if;
	struct mii_data * const mii = &sc->sc_mii;
	struct ifmedia * const ifm = &mii->mii_media;
	uint32_t which, dwfea, dwimp;
	uint8_t enaddr[ETHER_ADDR_LEN];
	bus_dma_segment_t seg;
	uint32_t csr;
	int i, nseg, error = 0;

	aprint_naive("\n");
	aprint_normal(": Socionext Gigabit Ethernet controller\n");

	which = CSR_READ(sc, HWVER);	/* Socionext version 5.00xx */
	dwfea = mac_read(sc, HWFEA);	/* DWC feature bits */
	dwimp = mac_read(sc, GMACIMPL);	/* DWC implementation XX.YY */
	aprint_normal_dev(sc->sc_dev,
	    "NetSec %x.%x (feature 0x%x imp 0x%0x)\n",
	    which >> 16, which & 0xffff, dwfea, dwimp);

	/* fetch MAC address in flash 0:7, stored in big endian order */
	csr = EE_READ(sc, 0x00);
	enaddr[0] = csr >> 24;
	enaddr[1] = csr >> 16;
	enaddr[2] = csr >> 8;
	enaddr[3] = csr;
	csr = EE_READ(sc, 0x04);
	enaddr[4] = csr >> 24;
	enaddr[5] = csr >> 16;
	aprint_normal_dev(sc->sc_dev,
	    "Ethernet address %s\n", ether_sprintf(enaddr));

	sc->sc_mdclk = get_mdioclk(sc->sc_freq) << GAR_CLK; /* 5:2 clk ratio */

	loaducode(sc);

	mii->mii_ifp = ifp;
	mii->mii_readreg = mii_readreg;
	mii->mii_writereg = mii_writereg;
	mii->mii_statchg = mii_statchg;

	sc->sc_ethercom.ec_mii = mii;
	ifmedia_init(ifm, 0, ether_mediachange, scx_ifmedia_sts);
	mii_attach(sc->sc_dev, mii, 0xffffffff, sc->sc_phy_id,
	    MII_OFFSET_ANY, MIIF_DOPAUSE);
	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(ifm, IFM_ETHER | IFM_NONE, 0, NULL);
		ifmedia_set(ifm, IFM_ETHER | IFM_NONE);
	} else
		ifmedia_set(ifm, IFM_ETHER | IFM_AUTO);
	ifm->ifm_media = ifm->ifm_cur->ifm_media; /* as if user has requested */

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct control_data), PAGE_SIZE, 0, &seg, 1, &nseg, 0);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate control data, error = %d\n", error);
		goto fail_0;
	}
	error = bus_dmamem_map(sc->sc_dmat, &seg, nseg,
	    sizeof(struct control_data), (void **)&sc->sc_control_data,
	    BUS_DMA_COHERENT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to map control data, error = %d\n", error);
		goto fail_1;
	}
	error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct control_data), 1,
	    sizeof(struct control_data), 0, 0, &sc->sc_cddmamap);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to create control data DMA map, "
		    "error = %d\n", error);
		goto fail_2;
	}
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct control_data), NULL, 0);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to load control data DMA map, error = %d\n",
		    error);
		goto fail_3;
	}
	for (i = 0; i < MD_TXQUEUELEN; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    MD_NTXSEGS, MCLBYTES, 0, 0,
		    &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to create tx DMA map %d, error = %d\n",
			    i, error);
			goto fail_4;
		}
	}
	for (i = 0; i < MD_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    1, MCLBYTES, 0, 0, &sc->sc_rxsoft[i].rxs_dmamap)) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to create rx DMA map %d, error = %d\n",
			    i, error);
			goto fail_5;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}
	sc->sc_seg = seg;
	sc->sc_nseg = nseg;
#if 0
aprint_normal_dev(sc->sc_dev, "descriptor ds_addr %lx, ds_len %lx, nseg %d\n", seg.ds_addr, seg.ds_len, nseg);
#endif
	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = scx_ioctl;
	ifp->if_start = scx_start;
	ifp->if_watchdog = scx_watchdog;
	ifp->if_init = scx_init;
	ifp->if_stop = scx_stop;
	IFQ_SET_READY(&ifp->if_snd);

	sc->sc_flowflags = 0;

	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, enaddr);

	callout_init(&sc->sc_callout, 0);
	callout_setfunc(&sc->sc_callout, phy_tick, sc);

	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	return;

  fail_5:
	for (i = 0; i < MD_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
  fail_4:
	for (i = 0; i < MD_TXQUEUELEN; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cddmamap);
  fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cddmamap);
  fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_control_data,
	    sizeof(struct control_data));
  fail_1:
	bus_dmamem_free(sc->sc_dmat, &seg, nseg);
  fail_0:
	if (sc->sc_phandle)
		fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
	else
		acpi_intr_disestablish(sc->sc_ih);
	bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_sz);
	return;
}

static void
scx_reset(struct scx_softc *sc)
{
	int loop = 0, busy;

	mac_write(sc, GMACOMR, 0);
	mac_write(sc, GMACBMR, BMR_RST);
	do {
		DELAY(1);
		busy = mac_read(sc, GMACBMR) & BMR_RST;
	} while (++loop < 3000 && busy);
	mac_write(sc, GMACBMR, _BMR);
	mac_write(sc, GMACAFR, 0);

	CSR_WRITE(sc, CLKEN, CLK_ALL);		/* distribute clock sources */
	CSR_WRITE(sc, SWRESET, 0);		/* reset operation */
	CSR_WRITE(sc, SWRESET, SRST_RUN);	/* manifest run */
	CSR_WRITE(sc, COMINIT, INIT_DB | INIT_CLS);
	WAIT_FOR_CLR(sc, COMINIT, (INIT_DB | INIT_CLS), 0);

	CSR_WRITE(sc, TXISR, ~0);
	CSR_WRITE(sc, xINTAE_CLR, ~0);

	/* clear event counters, auto-zero after every read */
	mac_write(sc, GMACEVCTL, EVC_CR | EVC_ROR);
}

static int
scx_init(struct ifnet *ifp)
{
	struct scx_softc *sc = ifp->if_softc;
	const uint8_t *ea = CLLADDR(ifp->if_sadl);
	paddr_t paddr;
	uint32_t csr;
	int i, error;

	/* Cancel pending I/O. */
	scx_stop(ifp, 0);

	/* Reset the chip to a known state. */
	scx_reset(sc);

	/* build sane Tx */
	memset(sc->sc_txdescs, 0, sizeof(struct tdes) * MD_NTXDESC);
	sc->sc_txdescs[MD_NTXDESC - 1].t0 = T0_EOD; /* tie off the ring */
	SCX_CDTXSYNC(sc, 0, MD_NTXDESC,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	sc->sc_txfree = MD_NTXDESC;
	sc->sc_txnext = 0;
	for (i = 0; i < MD_TXQUEUELEN; i++)
		sc->sc_txsoft[i].txs_mbuf = NULL;
	sc->sc_txsfree = MD_TXQUEUELEN;
	sc->sc_txsnext = 0;
	sc->sc_txsdirty = 0;

	/* load Rx descriptors with fresh mbuf */
	for (i = 0; i < MD_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_mbuf == NULL) {
			if ((error = add_rxbuf(sc, i)) != 0) {
				aprint_error_dev(sc->sc_dev,
				    "unable to allocate or map rx "
				    "buffer %d, error = %d\n",
				    i, error);
				rxdrain(sc);
				goto out;
			}
		}
		else
			SCX_INIT_RXDESC(sc, i);
	}
	sc->sc_rxdescs[MD_NRXDESC - 1].r0 = R0_EOD; /* tie off the ring */
	sc->sc_rxptr = 0;

	paddr = SCX_CDTXADDR(sc, 0);		/* tdes array (ring#0) */
	mac_write(sc, TDBA_HI, BUS_ADDR_HI32(paddr));
	mac_write(sc, TDBA_LO, BUS_ADDR_LO32(paddr));
	paddr = SCX_CDRXADDR(sc, 0);		/* rdes array (ring#1) */
	mac_write(sc, RDBA_HI, BUS_ADDR_HI32(paddr));
	mac_write(sc, RDBA_LO, BUS_ADDR_LO32(paddr));

	CSR_WRITE(sc, TXCONF, DESCNF_LE);	/* little endian */
	CSR_WRITE(sc, RXCONF, DESCNF_LE);	/* little endian */

	/* set my address in perfect match slot 0. little endian order */
	csr = (ea[3] << 24) | (ea[2] << 16) | (ea[1] << 8) |  ea[0];
	mac_write(sc, GMACMAL0, csr);
	csr = (ea[5] << 8) | ea[4];
	mac_write(sc, GMACMAH0, csr);

	/* accept multicast frame or run promisc mode */
	scx_set_rcvfilt(sc);

	/* set current media */
	if ((error = ether_mediachange(ifp)) != 0)
		goto out;

	CSR_WRITE(sc, DESC_SRST, 01);
	WAIT_FOR_CLR(sc, DESC_SRST, 01, 0);

	CSR_WRITE(sc, DESC_INIT, 01);
	WAIT_FOR_CLR(sc, DESC_INIT, 01, 0);

	mac_write(sc, GMACRDLA, _RDLA);		/* GMAC rdes store */
	mac_write(sc, GMACTDLA, _TDLA);		/* GMAC tdes store */

	CSR_WRITE(sc, FLOWTHR, (48<<16) | 36);	/* pause|resume threshold */
	mac_write(sc, GMACFCR, 256 << 16);	/* 31:16 pause value */

	CSR_WRITE(sc, RXIE_CLR, ~0);	/* clear Rx interrupt enable */
	CSR_WRITE(sc, TXIE_CLR, ~0);	/* clear Tx interrupt enable */

	CSR_WRITE(sc, RXCLSCMAX, 8);	/* Rx coalesce upper bound */
	CSR_WRITE(sc, TXCLSCMAX, 8);	/* Tx coalesce upper bound */
	CSR_WRITE(sc, RXITIMER, 500);	/* Rx co. timer usec */
	CSR_WRITE(sc, TXITIMER, 500);	/* Tx co. timer usec */

	CSR_WRITE(sc, RXIE_SET, RXI_RC_ERR | RXI_PKTCNT | RXI_TMREXP);
	CSR_WRITE(sc, TXIE_SET, TXI_TR_ERR | TXI_TXDONE | TXI_TMREXP);
	
	CSR_WRITE(sc, xINTAE_SET, IRQ_RX | IRQ_TX);

	/* kick to start GMAC engine */
	csr = mac_read(sc, GMACOMR);
	mac_write(sc, GMACOMR, csr | OMR_SR | OMR_ST);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* start one second timer */
	callout_schedule(&sc->sc_callout, hz);
 out:
	return error;
}

static void
scx_stop(struct ifnet *ifp, int disable)
{
	struct scx_softc *sc = ifp->if_softc;

	/* Stop the one second clock. */
	callout_stop(&sc->sc_callout);

	/* Down the MII. */
	mii_down(&sc->sc_mii);

	/* Mark the interface down and cancel the watchdog timer. */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;

	CSR_WRITE(sc, xINTAE_CLR, ~0);
	CSR_WRITE(sc, TXISR, ~0);
	CSR_WRITE(sc, RXISR, ~0);

	if (CSR_READ(sc, CORESTAT) != 0) {
		CSR_WRITE(sc, DMACTL_H2M, DMACTL_STOP);
		CSR_WRITE(sc, DMACTL_M2H, DMACTL_STOP);

		WAIT_FOR_CLR(sc, DMACTL_H2M, DMACTL_STOP, 0);
		WAIT_FOR_CLR(sc, DMACTL_M2H, DMACTL_STOP, 0);
	}
}

static int
scx_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct scx_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifmedia *ifm = &sc->sc_mii.mii_media;
	int s, error;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
		/* Flow control requires full-duplex mode. */
		if (IFM_SUBTYPE(ifr->ifr_media) == IFM_AUTO ||
		    (ifr->ifr_media & IFM_FDX) == 0)
			ifr->ifr_media &= ~IFM_ETH_FMASK;
		if (IFM_SUBTYPE(ifr->ifr_media) != IFM_AUTO) {
			if ((ifr->ifr_media & IFM_ETH_FMASK) == IFM_FLOW) {
				/* We can do both TXPAUSE and RXPAUSE. */
				ifr->ifr_media |=
				    IFM_ETH_TXPAUSE | IFM_ETH_RXPAUSE;
			}
			sc->sc_flowflags = ifr->ifr_media & IFM_ETH_FMASK;
		}
		error = ifmedia_ioctl(ifp, ifr, ifm, cmd);
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error != ENETRESET)
			break;
		error = 0;
		if (cmd == SIOCSIFCAP)
			error = if_init(ifp);
		if (cmd != SIOCADDMULTI && cmd != SIOCDELMULTI)
			;
		else if (ifp->if_flags & IFF_RUNNING) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			scx_set_rcvfilt(sc);
		}
		break;
	}

	splx(s);
	return error;
}

static uint32_t
bit_reverse_32(uint32_t x)
{
	x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
	x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
	x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
	x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
	return (x >> 16) | (x << 16);
}

static void
scx_set_rcvfilt(struct scx_softc *sc)
{
	struct ethercom * const ec = &sc->sc_ethercom;
	struct ifnet * const ifp = &ec->ec_if;
	struct ether_multistep step;
	struct ether_multi *enm;
	uint32_t mchash[2]; 	/* 2x 32 = 64 bit */
	uint32_t csr, crc;
	int i;

	csr = mac_read(sc, GMACAFR);
	csr &= ~(AFR_PR | AFR_PM | AFR_MHTE | AFR_HPF);
	mac_write(sc, GMACAFR, csr);

	/* clear 15 entry supplemental perfect match filter */
	for (i = 1; i < 16; i++)
		 mac_write(sc, GMACMAH(i), 0);
	/* build 64 bit multicast hash filter */
	crc = mchash[1] = mchash[0] = 0;

	ETHER_LOCK(ec);
	if (ifp->if_flags & IFF_PROMISC) {
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		/* run promisc. mode */
		csr |= AFR_PR;
		goto update;
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_FIRST_MULTI(step, ec, enm);
	i = 1; /* slot 0 is occupied */
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			ec->ec_flags |= ETHER_F_ALLMULTI;
			ETHER_UNLOCK(ec);
			/* accept all multi */
			csr |= AFR_PM;
			goto update;
		}
printf("[%d] %s\n", i, ether_sprintf(enm->enm_addrlo));
		if (i < 16) {
			/* use 15 entry perfect match filter */
			uint32_t addr;
			uint8_t *ep = enm->enm_addrlo;
			addr = (ep[3] << 24) | (ep[2] << 16)
			     | (ep[1] <<  8) |  ep[0];
			mac_write(sc, GMACMAL(i), addr);
			addr = (ep[5] << 8) | ep[4];
			mac_write(sc, GMACMAH(i), addr | 1U<<31);
		} else {
			/* use hash table when too many */
			crc = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);
			crc = bit_reverse_32(~crc);
			/* 1(31) 5(30:26) bit sampling */
			mchash[crc >> 31] |= 1 << ((crc >> 26) & 0x1f);
		}
		ETHER_NEXT_MULTI(step, enm);
		i++;
	}
	ETHER_UNLOCK(ec);
	if (crc)
		csr |= AFR_MHTE;
	csr |= AFR_HPF; /* use hash+perfect */
	mac_write(sc, GMACMHTH, mchash[1]);
	mac_write(sc, GMACMHTL, mchash[0]);
 update:
	/* With PR or PM, MHTE/MHTL/MHTH are never consulted. really? */
	mac_write(sc, GMACAFR, csr);
	return;
}

static void
scx_start(struct ifnet *ifp)
{
	struct scx_softc *sc = ifp->if_softc;
	struct mbuf *m0;
	struct scx_txsoft *txs;
	bus_dmamap_t dmamap;
	int error, nexttx, lasttx, ofree, seg;
	uint32_t tdes0;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/* Remember the previous number of free descriptors. */
	ofree = sc->sc_txfree;

	/*
	 * Loop through the send queue, setting up transmit descriptors
	 * until we drain the queue, or use up all available transmit
	 * descriptors.
	 */
	for (;;) {
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;

		if (sc->sc_txsfree < MD_TXQUEUE_GC) {
			txreap(sc);
			if (sc->sc_txsfree == 0)
				break;
		}
		txs = &sc->sc_txsoft[sc->sc_txsnext];
		dmamap = txs->txs_dmamap;

		error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
		    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
		if (error) {
			if (error == EFBIG) {
				aprint_error_dev(sc->sc_dev,
				    "Tx packet consumes too many "
				    "DMA segments, dropping...\n");
				    IFQ_DEQUEUE(&ifp->if_snd, m0);
				    m_freem(m0);
				    continue;
			}
			/* Short on resources, just stop for now. */
			break;
		}

		if (dmamap->dm_nsegs > sc->sc_txfree) {
			/*
			 * Not enough free descriptors to transmit this
			 * packet.  We haven't committed anything yet,
			 * so just unload the DMA map, put the packet
			 * back on the queue, and punt.	 Notify the upper
			 * layer that there are not more slots left.
			 */
			ifp->if_flags |= IFF_OACTIVE;
			bus_dmamap_unload(sc->sc_dmat, dmamap);
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m0);

		/*
		 * WE ARE NOW COMMITTED TO TRANSMITTING THE PACKET.
		 */

		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		tdes0 = 0; /* to postpone 1st segment T0_OWN write */
		lasttx = -1;
		for (nexttx = sc->sc_txnext, seg = 0;
		     seg < dmamap->dm_nsegs;
		     seg++, nexttx = MD_NEXTTX(nexttx)) {
			struct tdes *tdes = &sc->sc_txdescs[nexttx];
			bus_addr_t paddr = dmamap->dm_segs[seg].ds_addr;
			/*
			 * If this is the first descriptor we're
			 * enqueueing, don't set the OWN bit just
			 * yet.	 That could cause a race condition.
			 * We'll do it below.
			 */
			tdes->t3 = htole32(dmamap->dm_segs[seg].ds_len);
			tdes->t2 = htole32(BUS_ADDR_LO32(paddr));
			tdes->t1 = htole32(BUS_ADDR_HI32(paddr));
			tdes->t0 = htole32(tdes0 | (tdes->t0 & T0_EOD) |
					(15 << T0_TDRID) | T0_PT |
					sc->sc_t0cotso | T0_TRS);
			tdes0 = T0_OWN; /* 2nd and other segments */
			/* NB; t0 DRID field contains zero */
			lasttx = nexttx;
		}

		/* Write deferred 1st segment T0_OWN at the final stage */
		sc->sc_txdescs[lasttx].t0 |= htole32(T0_LS);
		sc->sc_txdescs[sc->sc_txnext].t0 |= htole32(T0_FS | T0_OWN);
		SCX_CDTXSYNC(sc, sc->sc_txnext, dmamap->dm_nsegs,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		/* submit one frame to xmit */
		CSR_WRITE(sc, TXSUBMIT, 1);

		txs->txs_mbuf = m0;
		txs->txs_firstdesc = sc->sc_txnext;
		txs->txs_lastdesc = lasttx;
		txs->txs_ndesc = dmamap->dm_nsegs;

		sc->sc_txfree -= txs->txs_ndesc;
		sc->sc_txnext = nexttx;
		sc->sc_txsfree--;
		sc->sc_txsnext = MD_NEXTTXS(sc->sc_txsnext);
		/*
		 * Pass the packet to any BPF listeners.
		 */
		bpf_mtap(ifp, m0, BPF_D_OUT);
	}

	if (sc->sc_txsfree == 0 || sc->sc_txfree == 0) {
		/* No more slots left; notify upper layer. */
		ifp->if_flags |= IFF_OACTIVE;
	}
	if (sc->sc_txfree != ofree) {
		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
	}
}

static void
scx_watchdog(struct ifnet *ifp)
{
	struct scx_softc *sc = ifp->if_softc;

	/*
	 * Since we're not interrupting every packet, sweep
	 * up before we report an error.
	 */
	txreap(sc);

	if (sc->sc_txfree != MD_NTXDESC) {
		aprint_error_dev(sc->sc_dev,
		    "device timeout (txfree %d txsfree %d txnext %d)\n",
		    sc->sc_txfree, sc->sc_txsfree, sc->sc_txnext);
		if_statinc(ifp, if_oerrors);

		/* Reset the interface. */
		scx_init(ifp);
	}

	scx_start(ifp);
}

static int
scx_intr(void *arg)
{
	struct scx_softc *sc = arg;
	uint32_t enable, status;

	status = CSR_READ(sc, xINTSR); /* not W1C */
	enable = CSR_READ(sc, xINTAEN);
	if ((status & enable) == 0)
		return 0;
	if (status & (IRQ_TX | IRQ_RX)) {
		CSR_WRITE(sc, xINTAE_CLR, (IRQ_TX | IRQ_RX));

		status = CSR_READ(sc, RXISR);
		CSR_WRITE(sc, RXISR, status);
		if (status & RXI_RC_ERR)
			aprint_error_dev(sc->sc_dev, "Rx error\n");
		if (status & (RXI_PKTCNT | RXI_TMREXP)) {
			rxintr(sc);
			(void)CSR_READ(sc, RXDONECNT); /* clear RXI_RXDONE */
		}

		status = CSR_READ(sc, TXISR);
		CSR_WRITE(sc, TXISR, status);
		if (status & TXI_TR_ERR)
			aprint_error_dev(sc->sc_dev, "Tx error\n");
		if (status & (TXI_TXDONE | TXI_TMREXP)) {
			txreap(sc);
			(void)CSR_READ(sc, TXDONECNT); /* clear TXI_TXDONE */
		}

		CSR_WRITE(sc, xINTAE_SET, (IRQ_TX | IRQ_RX));
	}
	return 1;
}

static void
txreap(struct scx_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct scx_txsoft *txs;
	uint32_t txstat;
	int i;

	ifp->if_flags &= ~IFF_OACTIVE;

	for (i = sc->sc_txsdirty; sc->sc_txsfree != MD_TXQUEUELEN;
	     i = MD_NEXTTXS(i), sc->sc_txsfree++) {
		txs = &sc->sc_txsoft[i];

		SCX_CDTXSYNC(sc, txs->txs_firstdesc, txs->txs_ndesc,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		txstat = le32toh(sc->sc_txdescs[txs->txs_lastdesc].t0);
		if (txstat & T0_OWN) /* desc is still in use */
			break;

		/* There is no way to tell transmission status per frame */

		if_statinc(ifp, if_opackets);

		sc->sc_txfree += txs->txs_ndesc;
		bus_dmamap_sync(sc->sc_dmat, txs->txs_dmamap,
		    0, txs->txs_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;
	}
	sc->sc_txsdirty = i;
	if (sc->sc_txsfree == MD_TXQUEUELEN)
		ifp->if_timer = 0;
}

static void
rxintr(struct scx_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct scx_rxsoft *rxs;
	struct mbuf *m;
	uint32_t rxstat;
	int i, len;

	for (i = sc->sc_rxptr; /*CONSTCOND*/ 1; i = MD_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		SCX_CDRXSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		rxstat = le32toh(sc->sc_rxdescs[i].r0);
		if (rxstat & R0_OWN) /* desc is left empty */
			break;

		/* R0_FS | R0_LS must have been marked for this desc */

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		len = sc->sc_rxdescs[i].r3 >> 16; /* 31:16 received */
		len -= ETHER_CRC_LEN;	/* Trim CRC off */
		m = rxs->rxs_mbuf;

		if (add_rxbuf(sc, i) != 0) {
			if_statinc(ifp, if_ierrors);
			SCX_INIT_RXDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmat,
			    rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize,
			    BUS_DMASYNC_PREREAD);
			continue;
		}

		m_set_rcvif(m, ifp);
		m->m_pkthdr.len = m->m_len = len;

		if (rxstat & R0_CSUM) {
			uint32_t csum = M_CSUM_IPv4;
			if (rxstat & R0_CERR)
				csum |= M_CSUM_IPv4_BAD;
			m->m_pkthdr.csum_flags |= csum;
		}
		if_percpuq_enqueue(ifp->if_percpuq, m);
	}
	sc->sc_rxptr = i;
}

static int
add_rxbuf(struct scx_softc *sc, int i)
{
	struct scx_rxsoft *rxs = &sc->sc_rxsoft[i];
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return ENOBUFS;

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return ENOBUFS;
	}

	if (rxs->rxs_mbuf != NULL)
		bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);

	rxs->rxs_mbuf = m;

	error = bus_dmamap_load(sc->sc_dmat, rxs->rxs_dmamap,
	    m->m_ext.ext_buf, m->m_ext.ext_size, NULL, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "can't load rx DMA map %d, error = %d\n", i, error);
		panic("add_rxbuf");
	}

	bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
	    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
	SCX_INIT_RXDESC(sc, i);

	return 0;
}

static void
rxdrain(struct scx_softc *sc)
{
	struct scx_rxsoft *rxs;
	int i;

	for (i = 0; i < MD_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
	}
}

void
mii_statchg(struct ifnet *ifp)
{
	struct scx_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_mii;
	const int Mbps[4] = { 10, 100, 1000, 0 };
	uint32_t miisr, mcr, fcr;
	int spd;

	/* decode MIISR register value */
	miisr = mac_read(sc, GMACMIISR);
	spd = Mbps[(miisr & MIISR_SPD) >> 1];
#if 1
	static uint32_t oldmiisr = 0;
	if (miisr != oldmiisr) {
		printf("MII link status (0x%x) %s",
		    miisr, (miisr & MIISR_LUP) ? "up" : "down");
		if (miisr & MIISR_LUP) {
			printf(" spd%d", spd);
			if (miisr & MIISR_FDX)
				printf(",full-duplex");
		}
		printf("\n");
	}
#endif
	/* Get flow control negotiation result. */
	if (IFM_SUBTYPE(mii->mii_media.ifm_cur->ifm_media) == IFM_AUTO &&
	    (mii->mii_media_active & IFM_ETH_FMASK) != sc->sc_flowflags)
		sc->sc_flowflags = mii->mii_media_active & IFM_ETH_FMASK;

	/* Adjust speed 1000/100/10. */
	mcr = mac_read(sc, GMACMCR);
	if (spd == 1000)
		mcr &= ~MCR_USEMII; /* RGMII+SPD1000 */
	else {
		if (spd == 100 && sc->sc_100mii)
			mcr |= MCR_SPD100;
		mcr |= MCR_USEMII;
	}
	mcr |= MCR_CST | MCR_JE;
	if (sc->sc_100mii == 0)
		mcr |= MCR_IBN;

	/* Adjust duplexity and PAUSE flow control. */
	mcr &= ~MCR_USEFDX;
	fcr = mac_read(sc, GMACFCR) & ~(FCR_TFE | FCR_RFE);
	if (miisr & MIISR_FDX) {
		if (sc->sc_flowflags & IFM_ETH_TXPAUSE)
			fcr |= FCR_TFE;
		if (sc->sc_flowflags & IFM_ETH_RXPAUSE)
			fcr |= FCR_RFE;
		mcr |= MCR_USEFDX;
	}
	mac_write(sc, GMACMCR, mcr);
	mac_write(sc, GMACFCR, fcr);

#if 1
	if (miisr != oldmiisr) {
		printf("%ctxfe, %crxfe\n",
		    (fcr & FCR_TFE) ? '+' : '-',
		    (fcr & FCR_RFE) ? '+' : '-');
	}
	oldmiisr = miisr;
#endif
}

static void
scx_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct scx_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_mii;

	mii_pollstat(mii);
	ifmr->ifm_status = mii->mii_media_status;
	ifmr->ifm_active = sc->sc_flowflags |
	    (mii->mii_media_active & ~IFM_ETH_FMASK);
}

static int
mii_readreg(device_t self, int phy, int reg, uint16_t *val)
{
	struct scx_softc *sc = device_private(self);
	uint32_t miia;
	int ntries;

	miia = (phy << GAR_PHY) | (reg << GAR_REG) | sc->sc_mdclk;
	mac_write(sc, GMACGAR, miia | GAR_BUSY);
	for (ntries = 0; ntries < 1000; ntries++) {
		if ((mac_read(sc, GMACGAR) & GAR_BUSY) == 0)
			goto unbusy;
		DELAY(1);
	}
	return ETIMEDOUT;
 unbusy:
	*val = mac_read(sc, GMACGDR);
	return 0;
}

static int
mii_writereg(device_t self, int phy, int reg, uint16_t val)
{
	struct scx_softc *sc = device_private(self);
	uint32_t miia;
	uint16_t dummy;
	int ntries;

	miia = (phy << GAR_PHY) | (reg << GAR_REG) | sc->sc_mdclk;
	mac_write(sc, GMACGDR, val);
	mac_write(sc, GMACGAR, miia | GAR_IOWR | GAR_BUSY);
	for (ntries = 0; ntries < 1000; ntries++) {
		if ((mac_read(sc, GMACGAR) & GAR_BUSY) == 0)
			goto unbusy;
		DELAY(1);
	}
	return ETIMEDOUT;	
  unbusy:
	mii_readreg(self, phy, MII_PHYIDR1, &dummy); /* dummy read cycle */
	return 0;
}

static void
phy_tick(void *arg)
{
	struct scx_softc *sc = arg;
	struct mii_data *mii = &sc->sc_mii;
	int s;

	s = splnet();
	mii_tick(mii);
	splx(s);
#ifdef GMAC_EVENT_COUNTERS
	/* 80 event counters exist */
#endif
	callout_schedule(&sc->sc_callout, hz);
}

static void
reset_hardware(struct scx_softc *sc)
{

	if (CSR_READ(sc, CORESTAT) != 0) {
		CSR_WRITE(sc, DMACTL_H2M, DMACTL_STOP);
		CSR_WRITE(sc, DMACTL_M2H, DMACTL_STOP);

		WAIT_FOR_CLR(sc, DMACTL_H2M, DMACTL_STOP, 0);
		WAIT_FOR_CLR(sc, DMACTL_M2H, DMACTL_STOP, 0);
	}
	CSR_WRITE(sc, SWRESET, 0);		/* reset operation */
	CSR_WRITE(sc, SWRESET, SRST_RUN);	/* manifest run */
	CSR_WRITE(sc, COMINIT, INIT_DB | INIT_CLS);
	WAIT_FOR_CLR(sc, COMINIT, (INIT_DB | INIT_CLS), 0);
}

/*
 * 3 independent uengines exist to process host2media, media2host and
 * packet data flows.
 */
static void
loaducode(struct scx_softc *sc)
{
	uint32_t up, lo, sz;
	uint64_t addr;

	reset_hardware(sc);
	CSR_WRITE(sc, xINTSR, IRQ_UCODE);

	up = EE_READ(sc, 0x08); /* H->M ucode addr high */
	lo = EE_READ(sc, 0x0c); /* H->M ucode addr low */
	sz = EE_READ(sc, 0x10); /* H->M ucode size */
	sz *= 4;
	addr = ((uint64_t)up << 32) | lo;
aprint_normal_dev(sc->sc_dev, "0x%x H2M ucode %u\n", lo, sz);
	injectucode(sc, UCODE_H2M, (bus_addr_t)addr, (bus_size_t)sz);

	up = EE_READ(sc, 0x14); /* M->H ucode addr high */
	lo = EE_READ(sc, 0x18); /* M->H ucode addr low */
	sz = EE_READ(sc, 0x1c); /* M->H ucode size */
	sz *= 4;
	addr = ((uint64_t)up << 32) | lo;
	injectucode(sc, UCODE_M2H, (bus_addr_t)addr, (bus_size_t)sz);
aprint_normal_dev(sc->sc_dev, "0x%x M2H ucode %u\n", lo, sz);

	lo = EE_READ(sc, 0x20); /* PKT ucode addr */
	sz = EE_READ(sc, 0x24); /* PKT ucode size */
	sz *= 4;
	injectucode(sc, UCODE_PKT, (bus_addr_t)lo, (bus_size_t)sz);
aprint_normal_dev(sc->sc_dev, "0x%x PKT ucode %u\n", lo, sz);

	WAIT_FOR_SET(sc, xINTSR, IRQ_UCODE, 0);
	/* XXX may take long time to end ?! XXX */
	CSR_WRITE(sc, xINTSR, IRQ_UCODE);
}

static void
injectucode(struct scx_softc *sc, int port,
	bus_addr_t addr, bus_size_t size)
{
	bus_space_handle_t bsh;
	bus_size_t off;
	uint32_t ucode;

	if (bus_space_map(sc->sc_st, addr, size, 0, &bsh) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "eeprom map failure for ucode port 0x%x\n", port);
		return;
	}
	for (off = 0; off < size; off += 4) {
		ucode = bus_space_read_4(sc->sc_st, bsh, off);
		CSR_WRITE(sc, port, ucode);
	}
	bus_space_unmap(sc->sc_st, bsh, size);
}

/* GAR 5:2 MDIO frequency selection */
static int
get_mdioclk(uint32_t freq)
{

	freq /= 1000 * 1000;
	if (freq < 35)
		return GAR_MDIO_25_35MHZ;
	if (freq < 60)
		return GAR_MDIO_35_60MHZ;
	if (freq < 100)
		return GAR_MDIO_60_100MHZ;
	if (freq < 150)
		return GAR_MDIO_100_150MHZ;
	if (freq < 250)
		return GAR_MDIO_150_250MHZ;
	return GAR_MDIO_250_300MHZ;
}
