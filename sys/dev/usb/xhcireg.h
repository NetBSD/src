/* $NetBSD: xhcireg.h,v 1.20 2022/04/06 21:51:29 mlelstv Exp $ */

/*-
 * Copyright (c) 2010 Hans Petter Selasky. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _DEV_USB_XHCIREG_H_
#define	_DEV_USB_XHCIREG_H_

/* XHCI PCI config registers */
#define	PCI_CBMEM		0x10	/* configuration base MEM */
#define	PCI_INTERFACE_XHCI	0x30

#define	PCI_USBREV		0x60	/* RO USB protocol revision */
#define	 PCI_USBREV_MASK	0xFF
#define	 PCI_USBREV_3_0		0x30	/* USB 3.0 */
#define	 PCI_USBREV_3_1		0x31	/* USB 3.1 */

#define	PCI_XHCI_FLADJ		0x61	/* RW frame length adjust */

#define	PCI_XHCI_INTEL_XUSB2PR	0xd0    /* Intel USB2 Port Routing */
#define	PCI_XHCI_INTEL_USB2PRM	0xd4    /* Intel USB2 Port Routing Mask */
#define	PCI_XHCI_INTEL_USB3_PSSEN 0xd8  /* Intel USB3 Port SuperSpeed Enable */
#define	PCI_XHCI_INTEL_USB3PRM	0xdc    /* Intel USB3 Port Routing Mask */

/* XHCI capability registers */
#define	XHCI_CAPLENGTH		0x00	/* RO capability - 1 byte */
#define	XHCI_HCIVERSION		0x02	/* RO version - 2 bytes */
#define	 XHCI_HCIVERSION_0_9	0x0090	/* xHCI version 0.9 */
#define	 XHCI_HCIVERSION_0_96	0x0096	/* xHCI version 0.96 */
#define	 XHCI_HCIVERSION_1_0	0x0100	/* xHCI version 1.0 */
#define	 XHCI_HCIVERSION_1_1	0x0110	/* xHCI version 1.1 */
#define	 XHCI_HCIVERSION_1_2	0x0120	/* xHCI version 1.2 */

#define	XHCI_HCSPARAMS1		0x04	/* RO structual parameters 1 */
#define	 XHCI_HCS1_MAXSLOTS_MASK	__BITS(7, 0)
#define	 XHCI_HCS1_MAXSLOTS(x)		__SHIFTOUT((x), XHCI_HCS1_MAXSLOTS_MASK)
#define	 XHCI_HCS1_MAXINTRS_MASK	__BITS(18, 8)
#define	 XHCI_HCS1_MAXINTRS(x)		__SHIFTOUT((x), XHCI_HCS1_MAXINTRS_MASK)
#define	 XHCI_HCS1_MAXPORTS_MASK	__BITS(31, 24)
#define	 XHCI_HCS1_MAXPORTS(x)		__SHIFTOUT((x), XHCI_HCS1_MAXPORTS_MASK)

#define	XHCI_HCSPARAMS2		0x08	/* RO structual parameters 2 */
#define	 XHCI_HCS2_IST_MASK	__BITS(3, 0)
#define	 XHCI_HCS2_IST(x)	__SHIFTOUT((x), XHCI_HCS2_IST_MASK)
#define	 XHCI_HCS2_ERSTMAX_MASK	__BITS(7, 4)
#define	 XHCI_HCS2_ERSTMAX(x)	__SHIFTOUT((x), XHCI_HCS2_ERSTMAX_MASK)
#define  XHCI_HCS2_SPBUFHI_MASK	__BITS(25, 21)
#define	 XHCI_HCS2_SPR_MASK	__BIT(26)
#define	 XHCI_HCS2_SPR(x)	__SHIFTOUT((x), XHCI_HCS2_SPR_MASK)
#define  XHCI_HCS2_SPBUFLO_MASK	__BITS(31, 27)
#define	 XHCI_HCS2_MAXSPBUF(x)	\
    (__SHIFTOUT((x), XHCI_HCS2_SPBUFHI_MASK) << 5) | \
    (__SHIFTOUT((x), XHCI_HCS2_SPBUFLO_MASK))

#define	XHCI_HCSPARAMS3		0x0c	/* RO structual parameters 3 */
#define	 XHCI_HCS3_U1_DEL_MASK	__BITS(7, 0)
#define	 XHCI_HCS3_U1_DEL(x)	__SHIFTOUT((x), XHCI_HCS3_U1_DEL_MASK)
#define	 XHCI_HCS3_U2_DEL_MASK	__BITS(15, 8)
#define	 XHCI_HCS3_U2_DEL(x)	__SHIFTOUT((x), XHCI_HCS3_U2_DEL_MASK)

#define	XHCI_HCCPARAMS		0x10	/* RO capability parameters */
#define	 XHCI_HCC_AC64(x)	__SHIFTOUT((x), __BIT(0))	/* 64-bit capable */
#define	 XHCI_HCC_BNC(x)	__SHIFTOUT((x), __BIT(1))	/* BW negotiation */
#define	 XHCI_HCC_CSZ(x)	__SHIFTOUT((x), __BIT(2))	/* context size */
#define	 XHCI_HCC_PPC(x)	__SHIFTOUT((x), __BIT(3))	/* port power control */
#define	 XHCI_HCC_PIND(x)	__SHIFTOUT((x), __BIT(4))	/* port indicators */
#define	 XHCI_HCC_LHRC(x)	__SHIFTOUT((x), __BIT(5))	/* light HC reset */
#define	 XHCI_HCC_LTC(x)	__SHIFTOUT((x), __BIT(6))	/* latency tolerance msg */
#define	 XHCI_HCC_NSS(x)	__SHIFTOUT((x), __BIT(7))	/* no secondary sid */
#define	 XHCI_HCC_PAE(x)	__SHIFTOUT((x), __BIT(8))	/* Parse All Event Data */
#define	 XHCI_HCC_SPC(x)	__SHIFTOUT((x), __BIT(9))	/* Short packet */
#define	 XHCI_HCC_SEC(x)	__SHIFTOUT((x), __BIT(10))	/* Stopped EDTLA */
#define	 XHCI_HCC_CFC(x)	__SHIFTOUT((x), __BIT(11))	/* Configuous Frame ID */
#define	 XHCI_HCC_MAXPSASIZE_MASK __BITS(15, 12)	/* max pri. stream array size */
#define	 XHCI_HCC_MAXPSASIZE(x)	__SHIFTOUT((x), XHCI_HCC_MAXPSASIZE_MASK)
#define	 XHCI_HCC_XECP_MASK	__BITS(31, 16)		/* extended capabilities pointer */
#define	 XHCI_HCC_XECP(x)	__SHIFTOUT((x), XHCI_HCC_XECP_MASK)

#define	XHCI_DBOFF		0x14	/* RO doorbell offset */
#define	XHCI_RTSOFF		0x18	/* RO runtime register space offset */
#define XHCI_HCCPARAMS2		0x1c	/* RO capability parameters 2 */
#define	 XHCI_HCC2_U3C(x)	__SHIFTOUT((x), __BIT(0))	/* U3 Entry capable */
#define	 XHCI_HCC2_CMC(x)	__SHIFTOUT((x), __BIT(1))	/* CEC MaxExLatTooLg */
#define	 XHCI_HCC2_FSC(x)	__SHIFTOUT((x), __BIT(2))	/* Foce Save Context */
#define	 XHCI_HCC2_CTC(x)	__SHIFTOUT((x), __BIT(3))	/* Compliance Transc */
#define	 XHCI_HCC2_LEC(x)	__SHIFTOUT((x), __BIT(4))	/* Large ESIT Payload */
#define	 XHCI_HCC2_CIC(x)	__SHIFTOUT((x), __BIT(5))	/* Configuration Inf */
#define	 XHCI_HCC2_ETC(x)	__SHIFTOUT((x), __BIT(6))	/* Extended TBC */
#define	 XHCI_HCC2_ETC_TSC(x)	__SHIFTOUT((x), __BIT(7))	/* ExtTBC TRB Status */
#define	 XHCI_HCC2_GSC(x)	__SHIFTOUT((x), __BIT(8))	/* Get/Set Extended Property */
#define	 XHCI_HCC2_VTC(x)	__SHIFTOUT((x), __BIT(9))	/* Virt. Based Trust */

#define XHCI_VTIOSOFF		0x20	/* RO Virtualization Base Trusted IO Offset */

/* XHCI operational registers.  Offset given by XHCI_CAPLENGTH register */
#define	XHCI_USBCMD		0x00	/* XHCI command */
#define	 XHCI_CMD_RS		__BIT(0)	/* RW Run/Stop */
#define	 XHCI_CMD_HCRST		__BIT(1)	/* RW Host Controller Reset */
#define	 XHCI_CMD_INTE		__BIT(2)	/* RW Interrupter Enable */
#define	 XHCI_CMD_HSEE		__BIT(3)	/* RW Host System Error Enable */
#define	 XHCI_CMD_LHCRST	__BIT(7)	/* RO/RW Light Host Controller Reset */
#define	 XHCI_CMD_CSS		__BIT(8)	/* RW Controller Save State */
#define	 XHCI_CMD_CRS		__BIT(9)	/* RW Controller Restore State */
#define	 XHCI_CMD_EWE		__BIT(10)	/* RW Enable Wrap Event */
#define	 XHCI_CMD_EU3S		__BIT(11)	/* RW Enable U3 MFINDEX Stop */
#define	 XHCI_CMD_CME		__BIT(13)	/* RW CEM Enable */
#define	 XHCI_CMD_ETE		__BIT(14)	/* RW Extended TBC Enable */
#define	 XHCI_CMD_TSC_EN	__BIT(15)	/* RW Extended TBC TRB Status Enable */
#define	 XHCI_CMD_VTIOE		__BIT(16)	/* RW VTIO Enable */

#define	XHCI_WAIT_CNR		100		/* in 1ms */
#define	XHCI_WAIT_HCRST		100		/* in 1ms */
#define	XHCI_WAIT_RSS		100		/* in 1ms */
#define	XHCI_WAIT_SSS		100		/* in 1ms */
#define	XHCI_WAIT_PLS_U0	100		/* in 1ms */
#define	XHCI_WAIT_PLS_U3	10		/* in 1ms */

#define	XHCI_USBSTS		0x04	/* XHCI status */
#define	 XHCI_STS_HCH		__BIT(0)	/* RO - Host Controller Halted */
#define	 XHCI_STS_RSVDZ0	__BIT(1)	/* RsvdZ - 1:1 */
#define	 XHCI_STS_HSE		__BIT(2)	/* RW - Host System Error */
#define	 XHCI_STS_EINT		__BIT(3)	/* RW - Event Interrupt */
#define	 XHCI_STS_PCD		__BIT(4)	/* RW - Port Change Detect */
#define	 XHCI_STS_RSVDZ1	__BITS(7, 5)	/* RsvdZ - 7:5 */
#define	 XHCI_STS_SSS		__BIT(8)	/* RO - Save State Status */
#define	 XHCI_STS_RSS		__BIT(9)	/* RO - Restore State Status */
#define	 XHCI_STS_SRE		__BIT(10)	/* RW - Save/Restore Error */
#define	 XHCI_STS_CNR		__BIT(11)	/* RO - Controller Not Ready */
#define	 XHCI_STS_HCE		__BIT(12)	/* RO - Host Controller Error */
#define	 XHCI_STS_RSVDP0	__BITS(13, 31)	/* RsvdP - 31:13 */

#define	XHCI_PAGESIZE		0x08	/* XHCI page size mask */
#define	 XHCI_PAGESIZE_4K	__BIT(0)	/* 4K Page Size */
#define	 XHCI_PAGESIZE_8K	__BIT(1)	/* 8K Page Size */
#define	 XHCI_PAGESIZE_16K	__BIT(2)	/* 16K Page Size */
#define	 XHCI_PAGESIZE_32K	__BIT(3)	/* 32K Page Size */
#define	 XHCI_PAGESIZE_64K	__BIT(4)	/* 64K Page Size */
#define	 XHCI_PAGESIZE_128K	__BIT(5)	/* 128K Page Size */
#define	 XHCI_PAGESIZE_256K	__BIT(6)	/* 256K Page Size */
#define	 XHCI_PAGESIZE_512K	__BIT(7)	/* 512K Page Size */
#define	 XHCI_PAGESIZE_1M	__BIT(8)	/* 1M Page Size */
#define	 XHCI_PAGESIZE_2M	__BIT(9)	/* 2M Page Size */
/* ... extends to 128M */

#define	XHCI_DNCTRL		0x14	/* XHCI device notification control */
#define	 XHCI_DNCTRL_MASK(n)	__BIT((n))

/* 5.4.5 Command Ring Control Register */
#define	XHCI_CRCR		0x18	/* XHCI command ring control */
#define	 XHCI_CRCR_LO_RCS	__BIT(0)	/* RW - consumer cycle state */
#define	 XHCI_CRCR_LO_CS	__BIT(1)	/* RW - command stop */
#define	 XHCI_CRCR_LO_CA	__BIT(2)	/* RW - command abort */
#define	 XHCI_CRCR_LO_CRR	__BIT(3)	/* RW - command ring running */
#define	 XHCI_CRCR_LO_MASK	__BITS(31, 6)

#define	XHCI_CRCR_HI		0x1c	/* XHCI command ring control */

/* 5.4.6 Device Context Base Address Array Pointer Registers */
#define	XHCI_DCBAAP		0x30	/* XHCI dev context BA pointer */
#define	XHCI_DCBAAP_HI		0x34	/* XHCI dev context BA pointer */

/* 5.4.7 Configure Register */
#define	XHCI_CONFIG		0x38
#define	 XHCI_CONFIG_SLOTS_MASK	__BITS(7, 0)	/* RW - number of device slots enabled */
#define	 XHCI_CONFIG_U3E	__BIT(8)	/* RW - U3 Entry Enable */
#define	 XHCI_CONFIG_CIE	__BIT(9)	/* RW - Configuration Information Enable */

/* 5.4.8 XHCI port status registers */
#define	XHCI_PORTSC(n)		(0x3f0 + (0x10 * (n)))	/* XHCI port status */
#define	 XHCI_PS_CCS		__BIT(0)	/* RO - current connect status */
#define	 XHCI_PS_PED		__BIT(1)	/* RW - port enabled / disabled */
#define	 XHCI_PS_OCA		__BIT(3)	/* RO - over current active */
#define	 XHCI_PS_PR		__BIT(4)	/* RW - port reset */
#define	 XHCI_PS_PLS_MASK	__BITS(8, 5)	/* RW - port link state */
#define	 XHCI_PS_PLS_GET(x)	__SHIFTOUT((x), XHCI_PS_PLS_MASK)	/* RW - port link state */
#define	 XHCI_PS_PLS_SET(x)	__SHIFTIN((x), XHCI_PS_PLS_MASK)	/* RW - port link state */

#define  XHCI_PS_PLS_SETU0	0
#define  XHCI_PS_PLS_SETU2	2
#define  XHCI_PS_PLS_SETU3	3
#define  XHCI_PS_PLS_SETDISC	5
#define  XHCI_PS_PLS_SETCOMP	10
#define  XHCI_PS_PLS_SETRESUME	15

#define  XHCI_PS_PLS_U0		0
#define  XHCI_PS_PLS_U1		1
#define  XHCI_PS_PLS_U2		2
#define  XHCI_PS_PLS_U3		3
#define  XHCI_PS_PLS_DISABLED	4
#define  XHCI_PS_PLS_RXDETECT	5
#define  XHCI_PS_PLS_INACTIVE	6
#define  XHCI_PS_PLS_POLLING	7
#define  XHCI_PS_PLS_RECOVERY	8
#define  XHCI_PS_PLS_HOTRESET	9
#define  XHCI_PS_PLS_COMPLIANCE	10
#define  XHCI_PS_PLS_TEST	11
#define  XHCI_PS_PLS_RESUME	15

#define	 XHCI_PS_PP		__BIT(9)	/* RW - port power */
#define	 XHCI_PS_SPEED_MASK	__BITS(13, 10)	/* RO - port speed */
#define	 XHCI_PS_SPEED_GET(x)	__SHIFTOUT((x), XHCI_PS_SPEED_MASK)
#define	 XHCI_PS_SPEED_FS	1
#define	 XHCI_PS_SPEED_LS	2
#define	 XHCI_PS_SPEED_HS	3
#define	 XHCI_PS_SPEED_SS	4
#define	 XHCI_PS_PIC_MASK	__BITS(15, 14)	/* RW - port indicator */
#define	 XHCI_PS_PIC_GET(x)	__SHIFTOUT((x), XHCI_PS_PIC_MASK)
#define	 XHCI_PS_PIC_SET(x)	__SHIFTIN((x), XHCI_PS_PIC_MASK)
#define	 XHCI_PS_LWS		__BIT(16)	/* RW - port link state write strobe */
#define	 XHCI_PS_CSC		__BIT(17)	/* RW - connect status change */
#define	 XHCI_PS_PEC		__BIT(18)	/* RW - port enable/disable change */
#define	 XHCI_PS_WRC		__BIT(19)	/* RW - warm port reset change */
#define	 XHCI_PS_OCC		__BIT(20)	/* RW - over-current change */
#define	 XHCI_PS_PRC		__BIT(21)	/* RW - port reset change */
#define	 XHCI_PS_PLC		__BIT(22)	/* RW - port link state change */
#define	 XHCI_PS_CEC		__BIT(23)	/* RW - config error change */
#define	 XHCI_PS_CAS		__BIT(24)	/* RO - cold attach status */
#define	 XHCI_PS_WCE		__BIT(25)	/* RW - wake on connect enable */
#define	 XHCI_PS_WDE		__BIT(26)	/* RW - wake on disconnect enable */
#define	 XHCI_PS_WOE		__BIT(27)	/* RW - wake on over-current enable */
#define	 XHCI_PS_DR		__BIT(30)	/* RO - device removable */
#define	 XHCI_PS_WPR		__BIT(31)	/* RW - warm port reset */
#define	 XHCI_PS_CLEAR		0x80FF01FFU	/* command bits */

/* 5.4.9 Port PM Status and Control Register */
#define	XHCI_PORTPMSC(n)	(0x3f4 + (0x10 * (n)))	/* XHCI status and control */
/* 5.4.9.1 */
#define	 XHCI_PM3_U1TO_MASK	__BITS(7, 0)	/* RW - U1 timeout */
#define	 XHCI_PM3_U1TO_GET(x)	__SHIFTOUT((x), XHCI_PM3_U1TO_MASK)
#define	 XHCI_PM3_U1TO_SET(x)	__SHIFTIN((x), XHCI_PM3_U1TO_MASK)
#define	 XHCI_PM3_U2TO_MASK	__BITS(15, 8)	/* RW - U2 timeout */
#define	 XHCI_PM3_U2TO_GET(x)	__SHIFTOUT((x), XHCI_PM3_U2TO_MASK)
#define	 XHCI_PM3_U2TO_SET(x)	__SHIFTIN((x), XHCI_PM3_U2TO_MASK)
#define	 XHCI_PM3_FLA		__BIT(16)	/* RW - Force Link PM Accept */

/* 5.4.9.2 */
#define	 XHCI_PM2_L1S_MASK	__BITS(2, 0)	/* RO - L1 status */
#define	 XHCI_PM2_L1S_GET(x)	__SHIFTOUT((x), XHCI_PM2_L1S_MASK)
#define	 XHCI_PM2_RWE		__BIT(3)	/* RW - remote wakup enable */
#define	 XHCI_PM2_BESL_MASK	__BITS(7, 4)	/* RW - Best Effort Service Latency */
#define	 XHCI_PM2_BESL_GET(x)	__SHIFTOUT((x), XHCI_PM2_BESL_MASK)
#define	 XHCI_PM2_BESL_SET(x)	__SHIFTIN((x), XHCI_PM2_BESL_MASK)
#define	 XHCI_PM2_L1SLOT_MASK	__BITS(15, 8)	/* RW - L1 device slot */
#define	 XHCI_PM2_L1SLOT_GET(x)	__SHIFTOUT((x), XHCI_PM2_L1SLOT_MASK)
#define	 XHCI_PM2_L1SLOT_SET(x)	__SHIFTIN((x), XHCI_PM2_L1SLOT_MASK)
#define	 XHCI_PM2_HLE		__BIT(16)	/* RW - hardware LPM enable */
#define	 XHCI_PM2_PTC_MASK	__BITS(31, 28)	/* RW - port test control */
#define	 XHCI_PM2_PTC_GET(x)	__SHIFTOUT((x), XHCI_PM2_PTC_MASK)
#define	 XHCI_PM2_PTC_SET(x)	__SHIFTOUT((x), XHCI_PM2_PTC_MASK)

/* 5.4.10 Port Link Info Register */
#define	XHCI_PORTLI(n)		(0x3f8 + (0x10 * (n)))	/* XHCI port link info */
/* 5.4.10.1 */
#define	 XHCI_PLI3_ERR_MASK	__BITS(15, 0)	/* RW - port link errors */
#define	 XHCI_PLI3_ERR_GET(x)	__SHIFTOUT((x), XHCI_PLI3_ERR_MASK)
#define	 XHCI_PLI3_RLC_MASK	__BITS(19, 16)	/* RO - Rx Lane Count */
#define	 XHCI_PLI3_RLC_GET	__SHIFTOUT((x), XHCI_PLI3_RLC_MASK)
#define	 XHCI_PLI3_TLC_MASK	__BITS(23, 20)	/* RO - Tx Lane Count */
#define	 XHCI_PLI3_TLC_GET	__SHIFTOUT((x), XHCI_PLI3_TLC_MASK)

/* 5.4.11 */
#define	XHCI_PORTHLPMC(n)	(0x3fc + (0x10 * (n)))	/* XHCI port hardware LPM control */
/* 5.4.11.1 */
#define	XHCI_PLMC3_LSEC_MASK	__BITS(15, 0)	/* RW - Link Soft Error Count */
#define	XHCI_PLMC3_LSEC_GET(x)	__SHIFTOUT((x), XHCI_PLMC3_LSEC_MASK)

/* 5.5.1 */
/* XHCI runtime registers.  Offset given by XHCI_CAPLENGTH + XHCI_RTSOFF registers */
#define	XHCI_MFINDEX		0x0000
#define	 XHCI_MFINDEX_MASK	__BITS(13, 0)	/* RO - microframe index */
#define	 XHCI_MFINDEX_GET(x)	__SHIFTOUT((x), XHCI_MFINDEX_MASK)

/* 5.5.2 Interrupter Register set */
/* 5.5.2.1 interrupt management */
#define	XHCI_IMAN(n)		(0x0020 + (0x20 * (n)))
#define	 XHCI_IMAN_INTR_PEND	__BIT(0)	/* RW - interrupt pending */
#define	 XHCI_IMAN_INTR_ENA	__BIT(1)	/* RW - interrupt enable */

/* 5.5.2.2 Interrupter Moderation */
#define	XHCI_IMOD(n)		(0x0024 + (0x20 * (n)))	/* XHCI interrupt moderation */
#define	 XHCI_IMOD_IVAL_MASK	__BITS(15,0)	/* 250ns unit */
#define	 XHCI_IMOD_IVAL_GET(x)	__SHIFTOUT((x), XHCI_IMOD_IVAL_MASK)
#define	 XHCI_IMOD_IVAL_SET(x)	__SHIFTIN((x), XHCI_IMOD_IVAL_MASK)
#define	 XHCI_IMOD_ICNT_MASK	__BITS(31, 16)	/* 250ns unit */
#define	 XHCI_IMOD_ICNT_GET(x)	__SHIFTOUT((x), XHCI_IMOD_ICNT_MASK)
#define	 XHCI_IMOD_ICNT_SET(x)	__SHIFTIN((x), XHCI_IMOD_ICNT_MASK)
#define	 XHCI_IMOD_DEFAULT	0x000001F4U	/* 8000 IRQ/second */
#define	 XHCI_IMOD_DEFAULT_LP	0x000003E8U	/* 4000 IRQ/sec for LynxPoint */

/* 5.5.2.3 Event Ring */
/* 5.5.2.3.1 Event Ring Segment Table Size */
#define	XHCI_ERSTSZ(n)		(0x0028 + (0x20 * (n)))
#define	 XHCI_ERSTS_MASK	__BITS(15, 0)	/* Event Ring Segment Table Size */
#define	 XHCI_ERSTS_GET(x)	__SHIFTOUT((x), XHCI_ERSTS_MASK)
#define	 XHCI_ERSTS_SET(x)	__SHIFTIN((x), XHCI_ERSTS_MASK)

/* 5.5.2.3.2 Event Ring Segment Table Base Address Register */
#define	XHCI_ERSTBA(n)		(0x0030 + (0x20 * (n)))
#define	 XHCI_ERSTBA_MASK	__BIT(31,6)	/* RW - segment base address (low) */
#define	XHCI_ERSTBA_HI(n)	(0x0034 + (0x20 * (n)))

/* 5.5.2.3.3 Event Ring Dequeue Pointer */
#define	XHCI_ERDP(n)		(0x0038 + (0x20 * (n)))
#define	 XHCI_ERDP_DESI_MASK	__BITS(2,0)	/* RO - dequeue segment index */
#define	 XHCI_ERDP_GET_DESI(x)	__SHIFTOUT(x), XHCI_ERDP_DESI_MASK)
#define	 XHCI_ERDP_BUSY		__BIT(3)	/* RW - event handler busy */
#define	 XHCI_ERDP_PTRLO_MASK	__BIT(31,4)	/* RW - dequeue pointer (low) */
#define	XHCI_ERDP_HI(n)		(0x003C + (0x20 * (n)))

/* 5.6 XHCI doorbell registers. Offset given by XHCI_CAPLENGTH + XHCI_DBOFF registers */
#define	XHCI_DOORBELL(n)	(0x0000 + (4 * (n)))
#define	 XHCI_DB_TARGET_MASK	__BITS(7, 0)	/* RW - doorbell target */
#define	 XHCI_DB_TARGET_GET(x)	__SHIFTOUT((x), XHCI_DB_TARGET_MASK)
#define	 XHCI_DB_TARGET_SET(x)	__SHIFTIN((x), XHCI_DB_TARGET_MASK)
#define	 XHCI_DB_SID_MASK	__BITS(31, 16)	/* RW - doorbell stream ID */
#define	 XHCI_DB_SID_GET(x)	__SHIFTOUT((x), XHCI_DB_SID_MASK)
#define	 XHCI_DB_SID_SET(x)	__SHIFTIN((x), XHCI_DB_SID_MASK)

/* 7 xHCI Extendeded capabilities */
#define	XHCI_XECP_ID_MASK	__BITS(7, 0)
#define	XHCI_XECP_ID(x)		__SHIFTOUT((x), XHCI_XECP_ID_MASK)
#define	XHCI_XECP_NEXT_MASK	__BITS(15, 8)
#define	XHCI_XECP_NEXT(x)	__SHIFTOUT((x), XHCI_XECP_NEXT_MASK)

/* XHCI extended capability ID's */
#define	XHCI_ID_USB_LEGACY	0x0001	/* USB Legacy Support */
#define	 XHCI_XECP_USBLEGSUP	0x0000	/* Legacy Support Capability Reg */
#define	 XHCI_XECP_USBLEGCTLSTS	0x0004	/* Legacy Support Ctrl & Status Reg */
#define	XHCI_ID_PROTOCOLS	0x0002	/* Supported Protocol */
#define	XHCI_ID_POWER_MGMT	0x0003	/* Extended Power Management */
#define	XHCI_ID_VIRTUALIZATION	0x0004	/* I/O Virtualization */
#define	XHCI_ID_MSG_IRQ		0x0005	/* Message Interrupt */
#define	XHCI_ID_USB_LOCAL_MEM	0x0006	/* Local Memory */
#define	XHCI_ID_USB_DEBUG	0x000A	/* USB Debug Capability */
#define	XHCI_ID_XMSG_IRQ	0x0011	/* Extended Message Interrupt */

/* 7.1 xHCI legacy support */
#define	XHCI_XECP_BIOS_SEM	0x0002
#define	XHCI_XECP_OS_SEM	0x0003

/* 7.2 xHCI Supported Protocol Capability */
#define	XHCI_XECP_USBID 	0x20425355

#define	XHCI_XECP_SP_W0_MINOR_MASK	__BITS(23, 16)
#define	XHCI_XECP_SP_W0_MINOR(x)	__SHIFTOUT((x), XHCI_XECP_SP_W0_MINOR_MASK)
#define	XHCI_XECP_SP_W0_MAJOR_MASK	__BITS(31, 24)
#define	XHCI_XECP_SP_W0_MAJOR(x)	__SHIFTOUT((x), XHCI_XECP_SP_W0_MAJOR_MASK)

#define	XHCI_XECP_SP_W8_CPO_MASK	__BITS(7, 0)
#define	XHCI_XECP_SP_W8_CPO(x)		__SHIFTOUT((x), XHCI_XECP_SP_W8_CPO_MASK)
#define	XHCI_XECP_SP_W8_CPC_MASK	__BITS(15, 8)
#define	XHCI_XECP_SP_W8_CPC(x)		__SHIFTOUT((x), XHCI_XECP_SP_W8_CPC_MASK)
#define	XHCI_XECP_SP_W8_PD_MASK		__BITS(27, 16)
#define	XHCI_XECP_SP_W8_PD(x)		__SHIFTOUT((x), XHCI_XECP_SP_W8_PD_MASK)
#define	XHCI_XECP_SP_W8_PSIC_MASK	__BITS(31, 28)
#define	XHCI_XECP_SP_W8_PSIC(x)		__SHIFTOUT((x), XHCI_XECP_SP_W8_PSIC_MASK)

#define XHCI_PAGE_SIZE(sc) ((sc)->sc_pgsz)

/* Chapter 6, Table 49 */
#define XHCI_DEVICE_CONTEXT_BASE_ADDRESS_ARRAY_ALIGN	64
#define XHCI_DEVICE_CONTEXT_ALIGN			64
#define XHCI_INPUT_CONTROL_CONTEXT_ALIGN		64
#define XHCI_SLOT_CONTEXT_ALIGN				32
#define XHCI_ENDPOINT_CONTEXT_ALIGN			32
#define XHCI_STREAM_CONTEXT_ALIGN			16
#define XHCI_STREAM_ARRAY_ALIGN				16
#define XHCI_TRANSFER_RING_SEGMENTS_ALIGN		16
#define XHCI_COMMAND_RING_SEGMENTS_ALIGN		64
#define XHCI_EVENT_RING_SEGMENTS_ALIGN			64
#define XHCI_EVENT_RING_SEGMENT_TABLE_ALIGN		64
#define XHCI_SCRATCHPAD_BUFFER_ARRAY_ALIGN		64
#define XHCI_SCRATCHPAD_BUFFERS_ALIGN			XHCI_PAGE_SIZE

#define XHCI_ERSTE_ALIGN				16
#define XHCI_TRB_ALIGN					16

struct xhci_trb {
	uint64_t trb_0;
	uint32_t trb_2;
#define XHCI_TRB_2_ERROR_MASK		__BITS(31, 24)
#define XHCI_TRB_2_ERROR_GET(x)		__SHIFTOUT((x), XHCI_TRB_2_ERROR_MASK)
#define XHCI_TRB_2_ERROR_SET(x)		__SHIFTIN((x), XHCI_TRB_2_ERROR_MASK)

#define XHCI_TRB_2_TDSZ_MASK		__BITS(21, 17)	/* TD Size */
#define XHCI_TRB_2_TDSZ_GET(x)		__SHIFTOUT((x), XHCI_TRB_2_TDSZ_MASK)
#define XHCI_TRB_2_TDSZ_SET(x)		__SHIFTIN((x), XHCI_TRB_2_TDSZ_MASK)
#define XHCI_TRB_2_REM_MASK		__BITS(23, 0)
#define XHCI_TRB_2_REM_GET(x)		__SHIFTOUT((x), XHCI_TRB_2_REM_MASK)
#define XHCI_TRB_2_REM_SET(x)		__SHIFTIN((x), XHCI_TRB_2_REM_MASK)

#define XHCI_TRB_2_BYTES_MASK		__BITS(16, 0)
#define XHCI_TRB_2_BYTES_GET(x)		__SHIFTOUT((x), XHCI_TRB_2_BYTES_MASK)
#define XHCI_TRB_2_BYTES_SET(x)		__SHIFTIN((x), XHCI_TRB_2_BYTES_MASK)
#define XHCI_TRB_2_IRQ_MASK		__BITS(31, 22)
#define XHCI_TRB_2_IRQ_GET(x)		__SHIFTOUT((x), XHCI_TRB_2_IRQ_MASK)
#define XHCI_TRB_2_IRQ_SET(x)		__SHIFTIN((x), XHCI_TRB_2_IRQ_MASK)
#define XHCI_TRB_2_STREAM_MASK		__BITS(31, 16)
#define XHCI_TRB_2_STREAM_GET(x)	__SHIFTOUT((x), XHCI_TRB_2_STREAM_MASK)
#define XHCI_TRB_2_STREAM_SET(x)	__SHIFTIN((x), XHCI_TRB_2_STREAM_MASK)
	uint32_t trb_3;
#define XHCI_TRB_3_TYPE_MASK		__BITS(15, 10)
#define XHCI_TRB_3_TYPE_GET(x)		__SHIFTOUT((x), XHCI_TRB_3_TYPE_MASK)
#define XHCI_TRB_3_TYPE_SET(x)		__SHIFTIN((x), XHCI_TRB_3_TYPE_MASK)
#define XHCI_TRB_3_CYCLE_BIT		__BIT(0)
#define XHCI_TRB_3_TC_BIT		__BIT(1)       /* command ring only */
#define XHCI_TRB_3_ENT_BIT		__BIT(1)       /* transfer ring only */
#define XHCI_TRB_3_ISP_BIT		__BIT(2)
#define XHCI_TRB_3_NSNOOP_BIT		__BIT(3)
#define XHCI_TRB_3_CHAIN_BIT		__BIT(4)
#define XHCI_TRB_3_IOC_BIT		__BIT(5)
#define XHCI_TRB_3_IDT_BIT		__BIT(6)
#define XHCI_TRB_3_TBC_MASK		__BITS(8, 7)
#define XHCI_TRB_3_TBC_GET(x)		__SHIFTOUT((x), XHCI_TRB_3_TBC_MASK)
#define XHCI_TRB_3_TBC_SET(x)		__SHIFTIN((x), XHCI_TRB_3_TBC_MASK)
#define XHCI_TRB_3_BEI_BIT		__BIT(9)
#define XHCI_TRB_3_DCEP_BIT		__BIT(9)
#define XHCI_TRB_3_PRSV_BIT		__BIT(9)
#define XHCI_TRB_3_BSR_BIT		__BIT(9)

#define XHCI_TRB_3_TRT_MASK		__BITS(17, 16)
#define XHCI_TRB_3_TRT_NONE		__SHIFTIN(0U, XHCI_TRB_3_TRT_MASK)
#define XHCI_TRB_3_TRT_OUT		__SHIFTIN(2U, XHCI_TRB_3_TRT_MASK)
#define XHCI_TRB_3_TRT_IN		__SHIFTIN(3U, XHCI_TRB_3_TRT_MASK)
#define XHCI_TRB_3_DIR_IN		__BIT(16)
#define XHCI_TRB_3_TLBPC_MASK		__BITS(19, 16)
#define XHCI_TRB_3_TLBPC_GET(x)		__SHIFTOUT((x), XHCI_TRB_3_TLBPC_MASK)
#define XHCI_TRB_3_TLBPC_SET(x)		__SHIFTIN((x), XHCI_TRB_3_TLBPC_MASK)
#define XHCI_TRB_3_EP_MASK		__BITS(20, 16)
#define XHCI_TRB_3_EP_GET(x)		__SHIFTOUT((x), XHCI_TRB_3_EP_MASK)
#define XHCI_TRB_3_EP_SET(x)		__SHIFTIN((x), XHCI_TRB_3_EP_MASK)
#define XHCI_TRB_3_FRID_MASK		__BITS(30, 20)
#define XHCI_TRB_3_FRID_GET(x)		__SHIFTOUT((x), XHCI_TRB_3_FRID_MASK)
#define XHCI_TRB_3_FRID_SET(x)		__SHIFTIN((x), XHCI_TRB_3_FRID_MASK)
#define XHCI_TRB_3_ISO_SIA_BIT		__BIT(31)
#define XHCI_TRB_3_SUSP_EP_BIT		__BIT(23)
#define XHCI_TRB_3_VFID_MASK		__BITS(23, 16)
#define XHCI_TRB_3_VFID_GET(x)		__SHIFTOUT((x), XHCI_TRB_3_VFID_MASK)
#define XHCI_TRB_3_VFID_SET(x)		__SHIFTIN((x), XHCI_TRB_3_VFID_MASK)
#define XHCI_TRB_3_SLOT_MASK		__BITS(31, 24)
#define XHCI_TRB_3_SLOT_GET(x)		__SHIFTOUT((x), XHCI_TRB_3_SLOT_MASK)
#define XHCI_TRB_3_SLOT_SET(x)		__SHIFTIN((x), XHCI_TRB_3_SLOT_MASK)

	/* Commands */
#define XHCI_TRB_TYPE_RESERVED          0x00
#define XHCI_TRB_TYPE_NORMAL            0x01
#define XHCI_TRB_TYPE_SETUP_STAGE       0x02
#define XHCI_TRB_TYPE_DATA_STAGE        0x03
#define XHCI_TRB_TYPE_STATUS_STAGE      0x04
#define XHCI_TRB_TYPE_ISOCH             0x05
#define XHCI_TRB_TYPE_LINK              0x06
#define XHCI_TRB_TYPE_EVENT_DATA        0x07
#define XHCI_TRB_TYPE_NOOP              0x08
#define XHCI_TRB_TYPE_ENABLE_SLOT       0x09
#define XHCI_TRB_TYPE_DISABLE_SLOT      0x0A
#define XHCI_TRB_TYPE_ADDRESS_DEVICE    0x0B
#define XHCI_TRB_TYPE_CONFIGURE_EP      0x0C
#define XHCI_TRB_TYPE_EVALUATE_CTX      0x0D
#define XHCI_TRB_TYPE_RESET_EP          0x0E
#define XHCI_TRB_TYPE_STOP_EP           0x0F
#define XHCI_TRB_TYPE_SET_TR_DEQUEUE    0x10
#define XHCI_TRB_TYPE_RESET_DEVICE      0x11
#define XHCI_TRB_TYPE_FORCE_EVENT       0x12
#define XHCI_TRB_TYPE_NEGOTIATE_BW      0x13
#define XHCI_TRB_TYPE_SET_LATENCY_TOL   0x14
#define XHCI_TRB_TYPE_GET_PORT_BW       0x15
#define XHCI_TRB_TYPE_FORCE_HEADER      0x16
#define XHCI_TRB_TYPE_NOOP_CMD          0x17

	/* Events */
#define XHCI_TRB_EVENT_TRANSFER         0x20
#define XHCI_TRB_EVENT_CMD_COMPLETE     0x21
#define XHCI_TRB_EVENT_PORT_STS_CHANGE  0x22
#define XHCI_TRB_EVENT_BW_REQUEST       0x23
#define XHCI_TRB_EVENT_DOORBELL         0x24
#define XHCI_TRB_EVENT_HOST_CTRL        0x25
#define XHCI_TRB_EVENT_DEVICE_NOTIFY    0x26
#define XHCI_TRB_EVENT_MFINDEX_WRAP     0x27

	/* Error codes */
#define XHCI_TRB_ERROR_INVALID          0x00
#define XHCI_TRB_ERROR_SUCCESS          0x01
#define XHCI_TRB_ERROR_DATA_BUF         0x02
#define XHCI_TRB_ERROR_BABBLE           0x03
#define XHCI_TRB_ERROR_XACT             0x04
#define XHCI_TRB_ERROR_TRB              0x05
#define XHCI_TRB_ERROR_STALL            0x06
#define XHCI_TRB_ERROR_RESOURCE         0x07
#define XHCI_TRB_ERROR_BANDWIDTH        0x08
#define XHCI_TRB_ERROR_NO_SLOTS         0x09
#define XHCI_TRB_ERROR_STREAM_TYPE      0x0A
#define XHCI_TRB_ERROR_SLOT_NOT_ON      0x0B
#define XHCI_TRB_ERROR_ENDP_NOT_ON      0x0C
#define XHCI_TRB_ERROR_SHORT_PKT        0x0D
#define XHCI_TRB_ERROR_RING_UNDERRUN    0x0E
#define XHCI_TRB_ERROR_RING_OVERRUN     0x0F
#define XHCI_TRB_ERROR_VF_RING_FULL     0x10
#define XHCI_TRB_ERROR_PARAMETER        0x11
#define XHCI_TRB_ERROR_BW_OVERRUN       0x12
#define XHCI_TRB_ERROR_CONTEXT_STATE    0x13
#define XHCI_TRB_ERROR_NO_PING_RESP     0x14
#define XHCI_TRB_ERROR_EV_RING_FULL     0x15
#define XHCI_TRB_ERROR_INCOMPAT_DEV     0x16
#define XHCI_TRB_ERROR_MISSED_SERVICE   0x17
#define XHCI_TRB_ERROR_CMD_RING_STOP    0x18
#define XHCI_TRB_ERROR_CMD_ABORTED      0x19
#define XHCI_TRB_ERROR_STOPPED          0x1A
#define XHCI_TRB_ERROR_LENGTH           0x1B
#define XHCI_TRB_ERROR_STOPPED_SHORT    0x1C
#define XHCI_TRB_ERROR_BAD_MELAT        0x1D
#define XHCI_TRB_ERROR_ISOC_OVERRUN     0x1F
#define XHCI_TRB_ERROR_EVENT_LOST       0x20
#define XHCI_TRB_ERROR_UNDEFINED        0x21
#define XHCI_TRB_ERROR_INVALID_SID      0x22
#define XHCI_TRB_ERROR_SEC_BW           0x23
#define XHCI_TRB_ERROR_SPLIT_XACT       0x24
} __packed __aligned(XHCI_TRB_ALIGN);
#define XHCI_TRB_SIZE sizeof(struct xhci_trb)

/*
 * 6.2.2 Slot context
 */
#define XHCI_SCTX_0_ROUTE_MASK			__BITS(19, 0)
#define XHCI_SCTX_0_ROUTE_GET(x)		__SHIFTOUT((x), XHCI_SCTX_0_ROUTE_MASK)
#define XHCI_SCTX_0_ROUTE_SET(x)		__SHIFTIN((x), XHCI_SCTX_0_ROUTE_MASK)
#define XHCI_SCTX_0_SPEED_MASK			__BITS(23, 20)
#define XHCI_SCTX_0_SPEED_GET(x)		__SHIFTOUT((x), XHCI_SCTX_0_SPEED_MASK)
#define XHCI_SCTX_0_SPEED_SET(x)		__SHIFTIN((x), XHCI_SCTX_0_SPEED_MASK)
#define XHCI_SCTX_0_MTT_MASK			__BIT(25)
#define XHCI_SCTX_0_MTT_SET(x)			__SHIFTIN((x), XHCI_SCTX_0_MTT_MASK)
#define XHCI_SCTX_0_MTT_GET(x)                  __SHIFTOUT((x), XHCI_SCTX_0_MTT_MASK)
#define XHCI_SCTX_0_HUB_MASK			__BIT(26)
#define XHCI_SCTX_0_HUB_SET(x)			__SHIFTIN((x), XHCI_SCTX_0_HUB_MASK)
#define XHCI_SCTX_0_HUB_GET(x)			__SHIFTOUT((x), XHCI_SCTX_0_HUB_MASK)
#define XHCI_SCTX_0_CTX_NUM_MASK		__BITS(31, 27)
#define XHCI_SCTX_0_CTX_NUM_SET(x)		__SHIFTIN((x), XHCI_SCTX_0_CTX_NUM_MASK)
#define XHCI_SCTX_0_CTX_NUM_GET(x)		__SHIFTOUT((x), XHCI_SCTX_0_CTX_NUM_MASK)

#define XHCI_SCTX_1_MAX_EL_MASK			__BITS(15, 0)
#define XHCI_SCTX_1_MAX_EL_SET(x)		__SHIFTIN((x), XHCI_SCTX_1_MAX_EL_MASK)
#define XHCI_SCTX_1_MAX_EL_GET(x)		__SHIFTOUT((x), XHCI_SCTX_1_MAX_EL_MASK)
#define XHCI_SCTX_1_RH_PORT_MASK		__BITS(23, 16)
#define XHCI_SCTX_1_RH_PORT_SET(x)		__SHIFTIN((x), XHCI_SCTX_1_RH_PORT_MASK)
#define XHCI_SCTX_1_RH_PORT_GET(x)		__SHIFTOUT((x), XHCI_SCTX_1_RH_PORT_MASK)
#define XHCI_SCTX_1_NUM_PORTS_MASK		__BITS(31, 24)
#define XHCI_SCTX_1_NUM_PORTS_SET(x)		__SHIFTIN((x), XHCI_SCTX_1_NUM_PORTS_MASK)
#define XHCI_SCTX_1_NUM_PORTS_GET(x)		__SHIFTOUT((x), XHCI_SCTX_1_NUM_PORTS_MASK)

#define XHCI_SCTX_2_TT_HUB_SID_MASK		__BITS(7, 0)
#define XHCI_SCTX_2_TT_HUB_SID_SET(x)		__SHIFTIN((x), XHCI_SCTX_2_TT_HUB_SID_MASK)
#define XHCI_SCTX_2_TT_HUB_SID_GET(x)		__SHIFTOUT((x), XHCI_SCTX_2_TT_HUB_SID_MASK)
#define XHCI_SCTX_2_TT_PORT_NUM_MASK		__BITS(15, 8)
#define XHCI_SCTX_2_TT_PORT_NUM_SET(x)		__SHIFTIN((x), XHCI_SCTX_2_TT_PORT_NUM_MASK)
#define XHCI_SCTX_2_TT_PORT_NUM_GET(x)		__SHIFTOUT((x), XHCI_SCTX_2_TT_PORT_NUM_MASK)
#define XHCI_SCTX_2_TT_THINK_TIME_MASK		__BITS(17, 16)
#define XHCI_SCTX_2_TT_THINK_TIME_SET(x)	__SHIFTIN((x), XHCI_SCTX_2_TT_THINK_TIME_MASK)
#define XHCI_SCTX_2_TT_THINK_TIME_GET(x)	__SHIFTOUT((x), XHCI_SCTX_2_TT_THINK_TIME_MASK)
#define XHCI_SCTX_2_IRQ_TARGET_MASK		__BITS(31, 22)
#define XHCI_SCTX_2_IRQ_TARGET_SET(x)		__SHIFTIN((x), XHCI_SCTX_2_IRQ_TARGET_MASK)
#define XHCI_SCTX_2_IRQ_TARGET_GET(x)		__SHIFTOUT((x), XHCI_SCTX_2_IRQ_TARGET_MASK)

#define XHCI_SCTX_3_DEV_ADDR_MASK		__BITS(7, 0)
#define XHCI_SCTX_3_DEV_ADDR_SET(x)		__SHIFTIN((x), XHCI_SCTX_3_DEV_ADDR_MASK)
#define XHCI_SCTX_3_DEV_ADDR_GET(x)		__SHIFTOUT((x), XHCI_SCTX_3_DEV_ADDR_MASK)
#define XHCI_SCTX_3_SLOT_STATE_MASK		__BITS(31, 27)
#define XHCI_SCTX_3_SLOT_STATE_SET(x)		__SHIFTIN((x), XHCI_SCTX_3_SLOT_STATE_MASK)
#define XHCI_SCTX_3_SLOT_STATE_GET(x)		__SHIFTOUT((x), XHCI_SCTX_3_SLOT_STATE_MASK)
#define XHCI_SLOTSTATE_DISABLED			0 /* disabled or enabled */
#define XHCI_SLOTSTATE_ENABLED			0
#define XHCI_SLOTSTATE_DEFAULT			1
#define XHCI_SLOTSTATE_ADDRESSED		2
#define XHCI_SLOTSTATE_CONFIGURED		3

/*
 * 6.2.3 Endpoint Context
 * */
#define XHCI_EPCTX_0_EPSTATE_MASK		__BITS(2, 0)
#define XHCI_EPCTX_0_EPSTATE_SET(x)		__SHIFTIN((x), XHCI_EPCTX_0_EPSTATE_MASK)
#define XHCI_EPCTX_0_EPSTATE_GET(x)		__SHIFTOUT((x), XHCI_EPCTX_0_EPSTATE_MASK)
#define XHCI_EPSTATE_DISABLED			0
#define XHCI_EPSTATE_RUNNING			1
#define XHCI_EPSTATE_HALTED			2
#define XHCI_EPSTATE_STOPPED			3
#define XHCI_EPSTATE_ERROR			4
#define XHCI_EPCTX_0_MULT_MASK			__BITS(9, 8)
#define XHCI_EPCTX_0_MULT_SET(x)		__SHIFTIN((x), XHCI_EPCTX_0_MULT_MASK)
#define XHCI_EPCTX_0_MULT_GET(x)		__SHIFTOUT((x), XHCI_EPCTX_0_MULT_MASK)
#define XHCI_EPCTX_0_MAXP_STREAMS_MASK		__BITS(14, 10)
#define XHCI_EPCTX_0_MAXP_STREAMS_SET(x)	__SHIFTIN((x), XHCI_EPCTX_0_MAXP_STREAMS_MASK)
#define XHCI_EPCTX_0_MAXP_STREAMS_GET(x)	__SHIFTOUT((x), XHCI_EPCTX_0_MAXP_STREAMS_MASK)
#define XHCI_EPCTX_0_LSA_MASK			__BIT(15)
#define XHCI_EPCTX_0_LSA_SET(x)			__SHIFTIN((x), XHCI_EPCTX_0_LSA_MASK)
#define XHCI_EPCTX_0_LSA_GET(x)			__SHIFTOUT((x), XHCI_EPCTX_0_LSA_MASK)
#define XHCI_EPCTX_0_IVAL_MASK			__BITS(23, 16)
#define XHCI_EPCTX_0_IVAL_SET(x)                __SHIFTIN((x), XHCI_EPCTX_0_IVAL_MASK)
#define XHCI_EPCTX_0_IVAL_GET(x)                __SHIFTOUT((x), XHCI_EPCTX_0_IVAL_MASK)
#define XHCI_EPCTX_0_MAX_ESIT_PAYLOAD_HI_MASK	__BITS(31, 24)
#define XHCI_EPCTX_0_MAX_ESIT_PAYLOAD_HI_SET(x) __SHIFTIN((x), XHCI_EPCTX_0_MAX_ESIT_PAYLOAD_HI_MASK)
#define XHCI_EPCTX_0_MAX_ESIT_PAYLOAD_HI_GET(x) __SHIFTOUT((x), XHCI_EPCTX_0_MAX_ESIT_PAYLOAD_HI_MASK)

#define XHCI_EPCTX_1_CERR_MASK			__BITS(2, 1)
#define XHCI_EPCTX_1_CERR_SET(x)		__SHIFTIN((x), XHCI_EPCTX_1_CERR_MASK)
#define XHCI_EPCTX_1_CERR_GET(x)		__SHIFTOUT((x), XHCI_EPCTX_1_CERR_MASK)
#define XHCI_EPCTX_1_EPTYPE_MASK		__BITS(5, 3)
#define XHCI_EPCTX_1_EPTYPE_SET(x)		__SHIFTIN((x), XHCI_EPCTX_1_EPTYPE_MASK)
#define XHCI_EPCTX_1_EPTYPE_GET(x)		__SHIFTOUT((x), XHCI_EPCTX_1_EPTYPE_MASK)
#define XHCI_EPCTX_1_HID_MASK			__BIT(7)
#define XHCI_EPCTX_1_HID_SET(x)			__SHIFTIN((x), XHCI_EPCTX_1_HID_MASK)
#define XHCI_EPCTX_1_HID_GET(x)			__SHIFTOUT((x), XHCI_EPCTX_1_HID_MASK)
#define XHCI_EPCTX_1_MAXB_MASK			__BITS(15, 8)
#define XHCI_EPCTX_1_MAXB_SET(x)		__SHIFTIN((x), XHCI_EPCTX_1_MAXB_MASK)
#define XHCI_EPCTX_1_MAXB_GET(x)		__SHIFTOUT((x), XHCI_EPCTX_1_MAXB_MASK)
#define XHCI_EPCTX_1_MAXP_SIZE_MASK		__BITS(31, 16)
#define XHCI_EPCTX_1_MAXP_SIZE_SET(x)		__SHIFTIN((x), XHCI_EPCTX_1_MAXP_SIZE_MASK)
#define XHCI_EPCTX_1_MAXP_SIZE_GET(x)		__SHIFTOUT((x), XHCI_EPCTX_1_MAXP_SIZE_MASK)


#define XHCI_EPCTX_2_DCS_MASK			__BIT(0)
#define XHCI_EPCTX_2_DCS_SET(x)			__SHIFTIN((x), XHCI_EPCTX_2_DCS_MASK)
#define XHCI_EPCTX_2_DCS_GET(x)			__SHIFTOUT((x), XHCI_EPCTX_2_DCS_MASK)
#define XHCI_EPCTX_2_TR_DQ_PTR_MASK             0xFFFFFFFFFFFFFFF0ULL

#define XHCI_EPCTX_4_AVG_TRB_LEN_MASK		__BITS(15, 0)
#define XHCI_EPCTX_4_AVG_TRB_LEN_SET(x)		__SHIFTIN((x), XHCI_EPCTX_4_AVG_TRB_LEN_MASK)
#define XHCI_EPCTX_4_AVG_TRB_LEN_GET(x)         __SHIFTOUT((x), XHCI_EPCTX_4_AVG_TRB_LEN_MASK)
#define XHCI_EPCTX_4_MAX_ESIT_PAYLOAD_MASK	__BITS(16, 31)
#define XHCI_EPCTX_4_MAX_ESIT_PAYLOAD_SET(x)    __SHIFTIN((x), XHCI_EPCTX_4_MAX_ESIT_PAYLOAD_MASK)
#define XHCI_EPCTX_4_MAX_ESIT_PAYLOAD_GET(x)    __SHIFTOUT((x), XHCI_EPCTX_4_MAX_ESIT_PAYLOAD_MASK)
#define XHCI_EPCTX_MEP_FS_INTR			64U
#define XHCI_EPCTX_MEP_FS_ISOC			(1*1024U)
#define XHCI_EPCTX_MEP_HS_INTR			(3*1024U)
#define XHCI_EPCTX_MEP_HS_ISOC			(3*1024U)
#define XHCI_EPCTX_MEP_SS_INTR			(3*1024U)
#define XHCI_EPCTX_MEP_SS_ISOC			(48*1024U)
#define XHCI_EPCTX_MEP_SS_ISOC_LEC		(16*1024*1024U - 1)


#define XHCI_INCTX_NON_CTRL_MASK        0xFFFFFFFCU

#define XHCI_INCTX_0_DROP_MASK(n)       __BIT((n))

#define XHCI_INCTX_1_ADD_MASK(n)        __BIT((n))


struct xhci_erste {
	uint64_t       erste_0;		/* 63:6 base */
	uint32_t       erste_2;		/* 15:0 trb count (16 to 4096) */
	uint32_t       erste_3;		/* RsvdZ */
} __packed __aligned(XHCI_ERSTE_ALIGN);
#define XHCI_ERSTE_SIZE sizeof(struct xhci_erste)

#endif	/* _DEV_USB_XHCIREG_H_ */
